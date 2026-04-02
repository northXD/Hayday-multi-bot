#define _CRT_SECURE_NO_WARNINGS
#include "bot_logic.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>
#include <time.h>
#include <winhttp.h>
#include "BotEngine.h"
#pragma comment(lib, "winhttp.lib")
std::string EncryptXORHex(const std::string& text, const std::string& key) {
    std::string hexStr;
    char buf[3];
    for (size_t i = 0; i < text.size(); i++) {
        sprintf(buf, "%02x", (unsigned char)(text[i] ^ key[i % key.length()]));
        hexStr += buf;
    }

  
    static bool seeded = false;
    if (!seeded) { srand((unsigned int)time(NULL)); seeded = true; }

    const char charset[] = "0123456789abcdef";
    std::string prefix = "", suffix = "";
    for (int i = 0; i < 16; i++) prefix += charset[rand() % 16];
    for (int i = 0; i < 16; i++) suffix += charset[rand() % 16];


    return prefix + hexStr + suffix;
}

std::string DecryptXORHex(const std::string& hexStr, const std::string& key) {
    if (hexStr.length() <= 32) return ""; 

    
    std::string cleanHex = hexStr.substr(16, hexStr.length() - 32);

    std::string text;
    if (cleanHex.length() % 2 != 0) return "";
    for (size_t i = 0; i < cleanHex.length(); i += 2) {
        std::string byteString = cleanHex.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), NULL, 16);
        text += (byte ^ key[(i / 2) % key.length()]);
    }
    return text;
}

// --- FILE CONTROL ---
bool IsFileValid(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    return file.tellg() > 1024;
}

// --- HIDDEN CMD ---
static bool RunCmdHiddenLogic(const std::string& command) {
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::string finalCmd = command;
    std::vector<char> cmdBuffer(finalCmd.begin(), finalCmd.end());
    cmdBuffer.push_back(0);

    BOOL ok = CreateProcessA(nullptr, cmdBuffer.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (!ok) return false;


    DWORD waitResult = WaitForSingleObject(pi.hProcess, 3000);

    if (waitResult == WAIT_TIMEOUT) {

        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

// SCREEN CAPTURING
cv::Mat CaptureInstanceScreen(int instanceId, const std::string& adbPath, const std::string& serial) {
    std::string tempFile = "C:\\Users\\Public\\adb_screen_" + std::to_string(instanceId) + ".png";
    int maxRetries = 2;
    cv::Mat img;

    for (int i = 0; i < maxRetries; ++i) {
        remove(tempFile.c_str());
        std::string cmd = "cmd.exe /c \"\"" + adbPath + "\" -s " + serial + " exec-out screencap -p > \"" + tempFile + "\"\"";

        if (RunCmdHiddenLogic(cmd)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (IsFileValid(tempFile)) {
                img = cv::imread(tempFile);
                if (!img.empty()) return img;
            }
        }
        // RETRY IF FAILED
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    return cv::Mat();
}

// --- SINGLE IMAGE SCAN
MatchResult FindImage(const cv::Mat& screen, const std::string& templatePath, float threshold, bool useGrayscale, float roiPercent, bool useMargins) {
    MatchResult result = { false, -1, -1, 0.0 };
    if (screen.empty()) return result;

	// ROI TO CUT OUT UNNECESSARY AREAS (LIKE CHAT, MENUS, ETC.)
    int topMargin = useMargins ? 60 : 0;
    int sideMargin = useMargins ? 50 : 0;

    int maxRoiHeight = (int)(screen.rows * roiPercent);
    if (screen.cols < 200 || screen.rows < 200) { topMargin = 0; sideMargin = 0; }

    int roiWidth = screen.cols - (2 * sideMargin);
    int roiHeight = maxRoiHeight - topMargin;

    if (roiWidth <= 0 || roiHeight <= 0) return result;

    cv::Rect roiRect(sideMargin, topMargin, roiWidth, roiHeight);
    cv::Mat searchArea = screen(roiRect);

    int flags = useGrayscale ? cv::IMREAD_GRAYSCALE : cv::IMREAD_COLOR;
    cv::Mat templ = cv::imread(templatePath, flags);
    if (templ.empty()) return result;

    cv::Mat processedScreen;
    if (useGrayscale) {
        if (searchArea.channels() == 3) cv::cvtColor(searchArea, processedScreen, cv::COLOR_BGR2GRAY);
        else processedScreen = searchArea;
    }
    else {
        processedScreen = searchArea;
    }

    cv::Mat matchResult;
    try {
        cv::matchTemplate(processedScreen, templ, matchResult, cv::TM_CCOEFF_NORMED);
    }
    catch (...) { return result; }

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(matchResult, &minVal, &maxVal, &minLoc, &maxLoc);

    if (maxVal >= threshold) {

        int finalX = maxLoc.x + sideMargin + (templ.cols / 2);
        int finalY = maxLoc.y + topMargin + (templ.rows / 2);

        result.found = true;
        result.score = maxVal;
        result.x = finalX;
        result.y = finalY;
    }
    return result;
}

// --- MULTIPLE SCAN (FOR CROPS, CRATES, ETC.)
std::vector<MatchResult> FindAllImages(const cv::Mat& screen, const std::string& templatePath, float threshold, int minDist, bool useMargins) {
    std::vector<MatchResult> results;
    if (screen.empty()) return results;

    int topMargin = useMargins ? 60 : 0;
    int sideMargin = useMargins ? 50 : 0;

    if (screen.cols < 200 || screen.rows < 200) { topMargin = 0; sideMargin = 0; }

    int roiWidth = screen.cols - (2 * sideMargin);
    int roiHeight = screen.rows - topMargin;

    cv::Rect roiRect(sideMargin, topMargin, roiWidth, roiHeight);
    cv::Mat searchArea = screen(roiRect);

    cv::Mat templ = cv::imread(templatePath);
    if (templ.empty()) return results;

    cv::Mat matchResult;
    try {
        cv::matchTemplate(searchArea, templ, matchResult, cv::TM_CCOEFF_NORMED);
    }
    catch (...) { return results; }

    for (int y = 0; y < matchResult.rows; y++) {
        for (int x = 0; x < matchResult.cols; x++) {
            if (matchResult.at<float>(y, x) >= threshold) {
                int realCenterX = x + sideMargin + (templ.cols / 2);
                int realCenterY = y + topMargin + (templ.rows / 2);

                bool isTooClose = false;
                for (const auto& existing : results) {
                    double dist = std::sqrt(std::pow(realCenterX - existing.x, 2) + std::pow(realCenterY - existing.y, 2));
                    if (dist < minDist) { isTooClose = true; break; }
                }

                if (!isTooClose) {
                    MatchResult res;
                    res.found = true;
                    res.x = realCenterX;
                    res.y = realCenterY;
                    res.score = matchResult.at<float>(y, x);
                    results.push_back(res);
                }
            }
        }
    }
    return results;
}
std::vector<MatchResult> FindGrownCrops(const cv::Mat& screen, int cropMode) {
    std::vector<MatchResult> results;
    if (screen.empty()) return results;


    int topMargin = 70;
    int bottomMargin = 40;
    int sideMargin = 30;

    if (screen.rows <= topMargin + bottomMargin || screen.cols <= sideMargin * 2) return results;

    cv::Rect roiRect(sideMargin, topMargin, screen.cols - (sideMargin * 2), screen.rows - (topMargin + bottomMargin));
    cv::Mat searchArea = screen(roiRect);


    cv::Mat hsv;
    cv::cvtColor(searchArea, hsv, cv::COLOR_BGR2HSV);


    cv::Scalar lowerBound, upperBound;
    if (cropMode == 0) { // WHEAT (Cyan)
        lowerBound = cv::Scalar(85, 200, 200); upperBound = cv::Scalar(95, 255, 255);
    }
    else if (cropMode == 1) { // CORN (Pure Blue)
        lowerBound = cv::Scalar(115, 200, 200); upperBound = cv::Scalar(125, 255, 255);
    }
    else if (cropMode == 2) { // CARROT (Hot Pink)
        lowerBound = cv::Scalar(160, 150, 200); upperBound = cv::Scalar(175, 255, 255);
    }
    else if (cropMode == 3) { // SOYBEAN (Mint Green)
        lowerBound = cv::Scalar(70, 200, 200); upperBound = cv::Scalar(80, 255, 255);
    }
    else if (cropMode == 4) { // SUGARCANE (Electric Purple)
        lowerBound = cv::Scalar(130, 200, 200); upperBound = cv::Scalar(140, 255, 255);
    }
    else {
        return results;
    }

    cv::Mat mask;
    cv::inRange(hsv, lowerBound, upperBound, mask);

    cv::Mat kernelOpen = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernelOpen);

    cv::Mat kernelClose = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernelClose);


    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);

       
        if (area > 50) {
            cv::Moments m = cv::moments(contours[i]);
            if (m.m00 != 0) {
                int localX = (int)(m.m10 / m.m00);
                int localY = (int)(m.m01 / m.m00);

              // DONUT HOLE PROTECTOR
                if (localX >= 0 && localX < mask.cols && localY >= 0 && localY < mask.rows) {
                    if (mask.at<uchar>(localY, localX) == 0) {
                        double minDist = 9999999.0;
                        int safeX = localX, safeY = localY;

                        for (int y = 0; y < mask.rows; y += 2) {
                            for (int x = 0; x < mask.cols; x += 2) {
                                if (mask.at<uchar>(y, x) > 0) {
                                    double dist = std::pow(x - localX, 2) + std::pow(y - localY, 2);
                                    if (dist < minDist) {
                                        minDist = dist;
                                        safeX = x;
                                        safeY = y;
                                    }
                                }
                            }
                        }
                        localX = safeX;
                        localY = safeY;
                    }
                }

              
                MatchResult res;
                res.found = true;
                res.x = localX + sideMargin;
                res.y = localY + topMargin;
                res.score = area;
                results.push_back(res);
            }
        }
    }

    return results;
}

// ==============================================================================
// EMPTY FIELD DETECTOR (MAGENTA - HSV)
// ==============================================================================
std::vector<MatchResult> FindEmptyFields(const cv::Mat& screen, bool isRecheck) {
    std::vector<MatchResult> results;
    if (screen.empty()) return results;

    int topMargin = 70;
    int bottomMargin = 40;
    int sideMargin = 30;

    if (screen.rows <= topMargin + bottomMargin || screen.cols <= sideMargin * 2) return results;

    cv::Rect roiRect(sideMargin, topMargin, screen.cols - (sideMargin * 2), screen.rows - (topMargin + bottomMargin));
    cv::Mat searchArea = screen(roiRect);

    cv::Mat hsv, mask;
    cv::cvtColor(searchArea, hsv, cv::COLOR_BGR2HSV);

    // FIELD (Magenta)
    cv::Scalar lowerBound(145, 200, 200);
    cv::Scalar upperBound(155, 255, 255);

    cv::inRange(hsv, lowerBound, upperBound, mask);


    if (isRecheck) {
  
        cv::Mat kernelOpen = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(11, 11)); 
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernelOpen);
    }
    else {
      
        cv::Mat kernelOpen = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernelOpen);

        cv::Mat kernelClose = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernelClose);
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);

        if (area > 50) {
            cv::Moments M = cv::moments(contour);
            if (M.m00 > 0) {
                int cx = static_cast<int>(M.m10 / M.m00);
                int cy = static_cast<int>(M.m01 / M.m00);

                // DONUT HOLE PROTECTION
                if (cx >= 0 && cx < mask.cols && cy >= 0 && cy < mask.rows) {
                    if (mask.at<uchar>(cy, cx) == 0) {
                        double minDist = 9999999.0;
                        int safeX = cx, safeY = cy;

                        for (int y = 0; y < mask.rows; y += 2) {
                            for (int x = 0; x < mask.cols; x += 2) {
                                if (mask.at<uchar>(y, x) > 0) {
                                    double dist = std::pow(x - cx, 2) + std::pow(y - cy, 2);
                                    if (dist < minDist) {
                                        minDist = dist;
                                        safeX = x;
                                        safeY = y;
                                    }
                                }
                            }
                        }
                        cx = safeX;
                        cy = safeY;
                    }
                }

                MatchResult res;
                res.found = true;
                res.x = cx + sideMargin;
                res.y = cy + topMargin;
                res.score = area;
                results.push_back(res);
            }
        }
    }
    return results;
}