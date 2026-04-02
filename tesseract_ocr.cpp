#include "tesseract_ocr.h"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <sstream>
#include <windows.h>
#include <mutex> 

#pragma comment(lib, "tesseract55.lib")
#pragma comment(lib, "leptonica-1.85.0.lib")

// =========================================================================
// GLOBAL TESSERACT ENGINE
// =========================================================================
tesseract::TessBaseAPI* g_tessApi = nullptr;
std::mutex g_ocrMutex;

void InitGlobalTesseract() {
    if (g_tessApi != nullptr) return;

    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::string exePath = std::string(exePathBuf);
    std::string tessDataPath = exePath.substr(0, exePath.find_last_of("\\/")) + "\\tessdata";

    g_tessApi = new tesseract::TessBaseAPI();
    if (g_tessApi->Init(tessDataPath.c_str(), "eng")) {
        delete g_tessApi;
        g_tessApi = nullptr;
    }
}

// =========================================================================
//  TRASH CLEANER
// =========================================================================
std::string AdvancedOCRRead(cv::Mat crop, int threshVal, int invertColor, int noiseArea, const char* whitelist, tesseract::PageSegMode psm) {
    if (crop.empty() || g_tessApi == nullptr) return "";

    std::lock_guard<std::mutex> lock(g_ocrMutex);

    cv::Mat processed;
    cv::cvtColor(crop, processed, cv::COLOR_BGR2GRAY);
    cv::resize(processed, processed, cv::Size(), 2.0, 2.0, cv::INTER_CUBIC);

    if (invertColor == 0) {
        cv::threshold(processed, processed, threshVal, 255, cv::THRESH_BINARY);
    }
    else {
        cv::threshold(processed, processed, threshVal, 255, cv::THRESH_BINARY_INV);
    }

    if (noiseArea > 0) {
        cv::Mat invertedForNoise;
        cv::bitwise_not(processed, invertedForNoise);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(invertedForNoise, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

        for (size_t i = 0; i < contours.size(); i++) {
            if (cv::contourArea(contours[i]) < noiseArea) {
                cv::drawContours(invertedForNoise, contours, (int)i, cv::Scalar(0), cv::FILLED);
            }
        }
        cv::bitwise_not(invertedForNoise, processed);
    }

    g_tessApi->SetVariable("tessedit_char_whitelist", whitelist);
    g_tessApi->SetPageSegMode(psm);
    g_tessApi->SetImage((uchar*)processed.data, processed.cols, processed.rows, 1, processed.step);

    char* outText = g_tessApi->GetUTF8Text();
    std::string result(outText ? outText : "");
    delete[] outText;

    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());

    return result;
}

// =========================================================================
// 1. COIN AND DIAMOND COUNT READER
// =========================================================================
int ReadGameNumber(cv::Mat screen, cv::Rect roi, std::string debugName) {
    if (screen.empty() || roi.x < 0 || roi.y < 0 || roi.width <= 0 || roi.height <= 0 ||
        roi.x + roi.width > screen.cols || roi.y + roi.height > screen.rows) return 0;

    cv::Mat crop = screen(roi);
    cv::Mat resizedImage, bw;


    cv::resize(crop, resizedImage, cv::Size(), 3.1, 3.1, cv::INTER_CUBIC);


    cv::inRange(resizedImage, cv::Scalar(132, 132, 132), cv::Scalar(255, 255, 255), bw);

    //  PIXEL CLEANER
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bw.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);
        cv::Rect box = cv::boundingRect(contours[i]);
        if (area < 80 || box.height < 10) {
            cv::drawContours(bw, contours, (int)i, cv::Scalar(0), cv::FILLED);
        }
    }

   
    cv::bitwise_not(bw, bw);
    cv::copyMakeBorder(bw, bw, 10, 10, 10, 10, cv::BORDER_CONSTANT, cv::Scalar(255));


    cv::imwrite("C:\\Users\\Public\\debug_" + debugName + ".png", bw);


    if (g_tessApi == nullptr) InitGlobalTesseract();
    std::lock_guard<std::mutex> lock(g_ocrMutex);

    g_tessApi->SetVariable("tessedit_char_whitelist", "0123456789Il|i!OoSsB/Zz");
    g_tessApi->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    g_tessApi->SetImage((uchar*)bw.data, bw.cols, bw.rows, 1, bw.step);

    char* outText = g_tessApi->GetUTF8Text();
    std::string finalResult(outText ? outText : "");
    delete[] outText;


    for (char& c : finalResult) {
        if (c == 'I' || c == 'l' || c == '|' || c == 'i' || c == '!') c = '1';
        if (c == 'O' || c == 'o') c = '0';
        if (c == 'S' || c == 's') c = '5';
        if (c == 'B') c = '8';
        if (c == '/') c = '7';
        if (c == 'Z' || c == 'z') c = '2';
    }

    std::string cleanNum = "";
    for (char c : finalResult) {
        if (isdigit(c)) cleanNum += c;
    }

    if (cleanNum.empty()) return 0;
    try { return std::stoi(cleanNum); }
    catch (...) { return 0; }
}

// =========================================================================
// 2. FARM name reader
// =========================================================================
std::string ReadFarmName(cv::Mat screen, cv::Rect roi) {
    if (screen.empty() || roi.x < 0 || roi.y < 0 || roi.width <= 0 || roi.height <= 0 ||
        roi.x + roi.width > screen.cols || roi.y + roi.height > screen.rows) return "Unknown";

    if (g_tessApi == nullptr) InitGlobalTesseract();
    cv::Mat crop = screen(roi);

    std::string finalResult = AdvancedOCRRead(crop, 200, 1, 0,
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 ", tesseract::PSM_SINGLE_BLOCK);

    return finalResult.empty() ? "Unknown" : finalResult;
}


int ReadItemCountByAnchor(cv::Mat screen, const std::string& templatePath) {
    if (screen.empty()) return 0;

    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::string exeDir = std::string(exePathBuf).substr(0, std::string(exePathBuf).find_last_of("\\/"));

    cv::Mat templ = cv::imread(exeDir + "\\" + templatePath);
    if (templ.empty()) return 0;

    cv::Mat matchResult;
    cv::matchTemplate(screen, templ, matchResult, cv::TM_CCOEFF_NORMED);
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(matchResult, &minVal, &maxVal, &minLoc, &maxLoc);

    if (maxVal < 0.70) return 0;

    int offsetX = 24;
    int offsetY = 9;
    int roiW = 26;
    int roiH = 23;

    int numX = maxLoc.x + offsetX;
    int numY = maxLoc.y + offsetY;

    if (numX < 0) numX = 0; if (numY < 0) numY = 0;
    int curW = roiW; int curH = roiH;
    if (numX + curW > screen.cols) curW = screen.cols - numX;
    if (numY + curH > screen.rows) curH = screen.rows - numY;

    cv::Rect numRect(numX, numY, curW, curH);
    cv::Mat crop = screen(numRect);
    if (crop.empty()) return 0;

    if (g_tessApi == nullptr) InitGlobalTesseract();
    std::lock_guard<std::mutex> lock(g_ocrMutex);

    cv::Mat resizedImage, bw;

    cv::resize(crop, resizedImage, cv::Size(), 3.4, 3.4, cv::INTER_CUBIC);
    cv::inRange(resizedImage, cv::Scalar(221, 221, 221), cv::Scalar(255, 255, 255), bw);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bw.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        cv::Rect box = cv::boundingRect(contours[i]);
        if (cv::contourArea(contours[i]) < 58 || box.height < 10) {
            cv::drawContours(bw, contours, (int)i, cv::Scalar(0), cv::FILLED);
        }
    }

    cv::bitwise_not(bw, bw);
    cv::copyMakeBorder(bw, bw, 10, 10, 10, 10, cv::BORDER_CONSTANT, cv::Scalar(255));

    cv::imwrite("C:\\Users\\Public\\debug_barn_" + templatePath.substr(10) + ".png", bw);

    g_tessApi->SetVariable("tessedit_char_whitelist", "0123456789");
    g_tessApi->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    g_tessApi->SetImage((uchar*)bw.data, bw.cols, bw.rows, 1, bw.step);

    char* outText = g_tessApi->GetUTF8Text();
    std::string rawText(outText ? outText : "");
    delete[] outText;

    while (!rawText.empty() && (rawText.back() == '\r' || rawText.back() == '\n' || rawText.back() == ' ')) {
        rawText.pop_back();
    }

    if (rawText.empty()) return 0;
    try { return std::stoi(rawText); }
    catch (...) { return 0; }
}