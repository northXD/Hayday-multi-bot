#define _WIN32_WINNT 0x0600
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <ctime>
#include <regex>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <intrin.h>
#include <filesystem>
#include <direct.h>
#include <mutex>
#include <intrin.h> 
// OpenCV Headers
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")

// --- Function to help navigate appdata path ---
std::string GetAppDataPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::string nxrthPath = std::string(path) + "\\NXRTH_Premium";

		// If theres no NXRTH_Premium folder in AppData, create it along with the 6 instance backup folders
        if (!std::filesystem::exists(nxrthPath)) {
            std::filesystem::create_directories(nxrthPath);
            // Creates 6 instance folders
            for (int i = 0; i < 6; i++) {
                std::filesystem::create_directories(nxrthPath + "\\Backups\\Instance_" + std::to_string(i));
            }
        }
        return nxrthPath;
    }
	return "."; // Remain in .exe directory if AppData path retrieval fails
}
// --- IMGUI & GLAD & GLFW ---
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <glad/glad.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "tesseract_ocr.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "BotEngine.h"
#include "Discord.h"
#include "XorStr.h"
#include "bot_logic.h"
#include "Language.h"
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#pragma comment(lib, "comdlg32.lib")

// Emulator Mode
int g_GlobalEmulatorMode = 0; // 0 = MEmu, 1 = LDPlayer


namespace fs = std::filesystem;
std::wstring ToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


char g_Username[64] = "Open Source";
char g_DiscordID[64] = "";
int g_TargetTabToSelect = -1; // To change tabs

extern int g_TransferThreshold; // auto item transfer min amount count
extern std::string g_StorageTag; // storage tag for the auto item transfer
char g_StorageTagBuf[64] = ""; // empty buffer for imgui


// --- GLOBAL TEXTURE ID's ---
GLuint icon_dashboard = 0;
GLuint icon_config = 0;
GLuint icon_logs = 0;
GLuint icon_cloud = 0;
GLuint icon_settings = 0;
GLuint icon_templates = 0;
GLuint icon_logo = 0;
GLuint icon_warning = 0;
GLuint icon_coin = 0;
GLuint icon_dia = 0;
GLuint icon_question = 0;
// --- GLOBAL SETTINGS ---
char g_AdbPathBuf[260] = "C:\\Program Files\\Microvirt\\MEmu\\adb.exe";
char g_MEmuPathBuf[260] = "C:\\Program Files\\Microvirt\\MEmu\\MEmuConsole.exe";
bool g_AdbValid = true;
bool g_MEmuValid = true;
bool g_EnableDiscordRPC = true;
bool g_SeparateTemplates = false;
bool g_EnableWebhookImage = false; 


// Path's
 std::string kAdbPath = g_AdbPathBuf;
 std::string kMEmuConsolePath = g_MEmuPathBuf;
 // paths for injecting
 std::string GAME_DATA_PATH = "/data/data/com.supercell.hayday/shared_prefs/storage_new.xml";
 std::string ZOOM_DATA_PATH = "/data/data/com.supercell.hayday/update/data/game_config.csv";

// --- TEMPLATE THRESHOLDS ---

TemplateThresholds g_Thresholds;

// --- TEMPLATE PATHS ---
std::string f_templatePath = "templates\\field.png";
std::string w_templatePath = "templates\\wheat.png";
std::string s_templatePath = "templates\\sickle.png";
std::string g_templatePath = "templates\\grown.png";
std::string shop_templatePath = "templates\\shop.png";
std::string wheatshop_templatePath = "templates\\wheat_shop.png";
std::string soldcrate_templatePath = "templates\\sold_crate.png";
std::string crate_templatePath = "templates\\shop_crate.png";
std::string arrows_templatePath = "templates\\arrows.png";
std::string plus_templatePath = "templates\\plus.png";
std::string cross_templatePath = "templates\\cross.png";
std::string advertise_templatePath = "templates\\advertise.png";
std::string create_sale_templatePath = "templates\\create_sale.png";
std::string c_templatePath = "templates\\corn.png";
std::string gc_templatePath = "templates\\grown_corn.png";
std::string cornshop_templatePath = "templates\\corn_shop.png";
std::string barn_market_templatePath = "templates\\barn_market.png";
std::string silo_market_templatePath = "templates\\silo_market.png";
std::string mailbox_templatePath = "templates\\mailbox.png";
std::string crate_wheat_templatePath = "templates\\crate_wheat.png";
std::string crate_corn_templatePath = "templates\\crate_corn.png";
std::string levelup_templatePath = "templates\\levelup.png";
std::string levelup_continue_templatePath = "templates\\levelup_continue.png";
std::string carrot_templatePath = "templates\\carrot.png";
std::string grown_carrot_templatePath = "templates\\grown_carrot.png";
std::string carrot_shop_templatePath = "templates\\carrot_shop.png";
std::string crate_carrot_templatePath = "templates\\crate_carrot.png";
std::string crate_soybean_templatePath = "templates\\crate_soybean.png";
std::string crate_sugarcane_templatePath = "templates\\crate_sugarcane.png";
std::string soybean_templatePath = "templates\\soybean.png";
std::string grown_soybean_templatePath = "templates\\grown_soybean.png";
std::string soybean_shop_templatePath = "templates\\soybean_shop.png";

std::string sugarcane_templatePath = "templates\\sugarcane.png";
std::string grown_sugarcane_templatePath = "templates\\grown_sugarcane.png";
std::string sugarcane_shop_templatePath = "templates\\sugarcane_shop.png";
std::string silo_full_templatePath = "templates\\silo_full.png";
std::string silo_full_cross_templatePath = "templates\\silo_full_cross.png";
std::string market_close_crosstemplatePath = "templates\\market_close_cross.png";

// Buffers
char g_fieldPathBuf[260] = "templates\\field.png";
char g_wheatPathBuf[260] = "templates\\wheat.png";
char g_sicklePathBuf[260] = "templates\\sickle.png";
char g_grownPathBuf[260] = "templates\\grown.png";
char g_shopPathBuf[260] = "templates\\shop.png";
char g_wheatshopPathBuf[260] = "templates\\wheat_shop.png";
char g_soldcratePathBuf[260] = "templates\\sold_crate.png";
char g_cratePathBuf[260] = "templates\\shop_crate.png";
char g_arrowsPathBuf[260] = "templates\\arrows.png";
char g_plusPathBuf[260] = "templates\\plus.png";
char g_crossPathBuf[260] = "templates\\cross.png";
char g_advertisePathBuf[260] = "templates\\advertise.png";
char g_createSalePathBuf[260] = "templates\\create_sale.png";
char g_cornPathBuf[260] = "templates\\corn.png";
char g_grownCornPathBuf[260] = "templates\\grown_corn.png";
char g_cornShopPathBuf[260] = "templates\\corn_shop.png";
char g_barnMarketBuf[260] = "templates\\barn_market.png";
char g_siloMarketBuf[260] = "templates\\silo_market.png";
char g_mailboxPathBuf[260] = "templates\\mailbox.png";
char g_crateWheatPathBuf[260] = "templates\\crate_wheat.png";
char g_crateCornPathBuf[260] = "templates\\crate_corn.png";
char g_levelupPathBuf[260] = "templates\\levelup.png";
char g_levelupContinuePathBuf[260] = "templates\\levelup_continue.png";
char g_carrotPathBuf[260] = "templates\\carrot.png";
char g_grownCarrotPathBuf[260] = "templates\\grown_carrot.png";
char g_carrotShopPathBuf[260] = "templates\\carrot_shop.png";

char g_soybeanPathBuf[260] = "templates\\soybean.png";
char g_grownSoybeanPathBuf[260] = "templates\\grown_soybean.png";
char g_soybeanShopPathBuf[260] = "templates\\soybean_shop.png";

char g_sugarcanePathBuf[260] = "templates\\sugarcane.png";
char g_grownSugarcanePathBuf[260] = "templates\\grown_sugarcane.png";
char g_sugarcaneShopPathBuf[260] = "templates\\sugarcane_shop.png";
char g_siloFullPathBuf[260] = "templates\\silo_full.png";
char g_siloFullCrossPathBuf[260] = "templates\\silo_full_cross.png";
char g_marketCloseCrossPathBuf[260] = "templates\\market_close_cross.png";

IntervalSettings g_Intervals;



// =========================================================
// LOG SYSTEM
// =========================================================
struct LogEntry {
    std::string timestamp;
    std::string message;
    int instanceId;
    ImVec4 color;
};

std::vector<LogEntry> g_GlobalLogs;
std::mutex g_LogMutex;
int g_LogFilter = -1;
bool g_AutoScrollLogs = true;
// gets clipboard text, used for quickly pasting player tags into the gui. dont worry this is not working randomly or used for malicious stuff. only used when bot pressese copy player tag button
std::string GetClipboardText() {
    if (!OpenClipboard(nullptr)) return "NO_TAG";
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr) { CloseClipboard(); return "NO_TAG"; }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    std::string text;
    if (pszText) {
        
        SIZE_T max_len = GlobalSize(hData);
        text = std::string(pszText, strnlen(pszText, max_len));
    }
    else {
        text = "NO_TAG";
    }

    if (hData) GlobalUnlock(hData);
    CloseClipboard();

    std::string cleanText = "";
    for (char c : text) {
        if (c != '\r' && c != '\n' && c != ' ') cleanText += std::toupper(c);
    }
    return cleanText.empty() ? "NO_TAG" : cleanText;
}
// gets time for the logs
std::string GetTimeStr() {
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return std::string(buffer);
}
// Log adding function
void AddLog(int instanceId, std::string message, ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f)) {
    std::lock_guard<std::mutex> lock(g_LogMutex);
    LogEntry newLog;
    newLog.timestamp = GetTimeStr();
    newLog.message = message;
    newLog.instanceId = instanceId;
    newLog.color = color;
    g_GlobalLogs.push_back(newLog);
    if (g_GlobalLogs.size() > 1000) g_GlobalLogs.erase(g_GlobalLogs.begin());
}

BotInstance g_Bots[6];

// --- Variables ---

char g_WebhookURL[256] = "";
bool g_EnableBarnWebhook = false; // off by default


char g_LicenseKey[256] = "";
int g_CurrentTab = 0; // current tab in the GUI
int g_SelectedInstanceUI = 0; // selected instance
extern void RunEmulatorFactory(int requestedInstance); // Defining the function in botengine.cpp

std::chrono::steady_clock::time_point g_BotStartTime;

// --- FUNCTIONS ---
std::string OpenFileDialog(const char* filter = "PNG Files\0*.png\0All Files\0*.*\0") {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "png";
    if (GetOpenFileNameA(&ofn)) return std::string(fileName);
    return "";
}
// Checks if the paths are correct
void ValidatePaths() {
    g_AdbValid = fs::exists(g_AdbPathBuf);
    g_MEmuValid = fs::exists(g_MEmuPathBuf);
    if (g_AdbValid) kAdbPath = std::string(g_AdbPathBuf);
    if (g_MEmuValid) kMEmuConsolePath = std::string(g_MEmuPathBuf);
}
// settings saver
void SaveConfig() {
    std::string realPath = GetAppDataPath() + "\\nxrth_config.ini";
    std::string tempPath = GetAppDataPath() + "\\nxrth_config.tmp"; 

    std::ofstream out(tempPath);
    if (out.is_open()) {
        out << "AdbPath=" << g_AdbPathBuf << "\n";
        out << "MEmuPath=" << g_MEmuPathBuf << "\n";
        out << "WebhookURL=" << g_WebhookURL << "\n";
        
        out << "EnableBarnWebhook=" << (g_EnableBarnWebhook ? "1" : "0") << "\n";
        out << "EnableWebhookImage=" << (g_EnableWebhookImage ? "1" : "0") << "\n"; 
        out << "GameLoadWait=" << g_Intervals.gameLoadWait << "\n";
        out << "AfterHarvestWait=" << g_Intervals.afterHarvestWait << "\n";
        out << "AfterPlantWait=" << g_Intervals.afterPlantWait << "\n";
        out << "ShopEnterWait=" << g_Intervals.shopEnterWait << "\n";
        out << "CrateClickWait=" << g_Intervals.crateClickWait << "\n";
        out << "NextAccountWait=" << g_Intervals.nextAccountWait << "\n";
       
        out << "CoinCollectWait=" << g_Intervals.coinCollectWait << "\n";
        out << "ProductSelectWait=" << g_Intervals.productSelectWait << "\n";
        out << "CreateSaleWait=" << g_Intervals.createSaleWait << "\n";
        out << "TransferThreshold=" << g_TransferThreshold << "\n";
        out << "StorageTag=" << g_StorageTagBuf << "\n";
        out << "GlobalEmuMode=" << g_GlobalEmulatorMode << "\n";
        for (int i = 0; i < 6; i++) {
            out << "Inst_" << i << "_Touch=" << g_Bots[i].inputDevice << "\n";
        }
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 15; j++) {
                AccountSlot& acc = g_Bots[i].accounts[j];
                std::string pfx = "Tom_" + std::to_string(i) + "_" + std::to_string(j) + "_";
                out << pfx << "En=" << (acc.autoTomEnabled ? "1" : "0") << "\n";
                out << pfx << "Rem=" << acc.tomRemainingHours << "\n";
                out << pfx << "Start=" << acc.tomStartTime << "\n";
                out << pfx << "Next=" << acc.nextTomTime << "\n";
                out << pfx << "Cat=" << acc.tomCategory << "\n";
                out << pfx << "Item=" << acc.tomItemName << "\n";
                std::string accPfx = "Acc_" + std::to_string(i) + "_" + std::to_string(j) + "_";
                out << accPfx << "CustomName=" << acc.name << "\n";
                out << accPfx << "Lvl=" << acc.level << "\n";
                out << accPfx << "Tag=" << acc.playerTag << "\n";
                out << accPfx << "Friend=" << (acc.isFriendWithStorage ? "1" : "0") << "\n";
                out << accPfx << "Name=" << acc.farmName << "\n";
                out << accPfx << "Coin=" << acc.coinAmount << "\n";
                out << accPfx << "Dia=" << acc.diamondAmount << "\n";
                
            
            }
        }
        
        out.close();
        std::error_code ec;
        std::filesystem::remove(realPath, ec); // Delete old path
        std::filesystem::rename(tempPath, realPath, ec);
    }
}
//config loader
void LoadConfig() {
    std::string configPath = GetAppDataPath() + "\\nxrth_config.ini";
    std::ifstream in(configPath);
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string val = line.substr(eqPos + 1);

                if (key == "AdbPath") strncpy(g_AdbPathBuf, val.c_str(), 260);
                else if (key == "MEmuPath") strncpy(g_MEmuPathBuf, val.c_str(), 260);
                else if (key == "WebhookURL") strncpy(g_WebhookURL, val.c_str(), 256);
               
                else if (key == "EnableBarnWebhook") g_EnableBarnWebhook = (val == "1");
               
                else if (key == "EnableWebhookImage") g_EnableWebhookImage = (val == "1"); 
                else if (key == "DiscordID") strncpy(g_DiscordID, val.c_str(), 64);
                else if (key == "GameLoadWait") g_Intervals.gameLoadWait = std::stoi(val);
                else if (key == "AfterHarvestWait") g_Intervals.afterHarvestWait = std::stoi(val);
                else if (key == "AfterPlantWait") g_Intervals.afterPlantWait = std::stoi(val);
                else if (key == "ShopEnterWait") g_Intervals.shopEnterWait = std::stoi(val);
                else if (key == "CrateClickWait") g_Intervals.crateClickWait = std::stoi(val);
                else if (key == "NextAccountWait") g_Intervals.nextAccountWait = std::stoi(val);
                
                else if (key == "GlobalEmuMode") g_GlobalEmulatorMode = std::stoi(val);
                else if (key == "CoinCollectWait") g_Intervals.coinCollectWait = std::stoi(val);
                else if (key == "ProductSelectWait") g_Intervals.productSelectWait = std::stoi(val);
                else if (key == "CreateSaleWait") g_Intervals.createSaleWait = std::stoi(val);
                else if (key == "TransferThreshold") g_TransferThreshold = std::stoi(val);

                else if (key == "StorageTag") {
                    strncpy(g_StorageTagBuf, val.c_str(), sizeof(g_StorageTagBuf));
                    g_StorageTag = val; 
                }
                else if (key.rfind("Inst_", 0) == 0) {
                    int i = key[5] - '0';
                    std::string subKey = key.substr(7);
                    if (i >= 0 && i < 6) {
                        if (subKey == "Touch") strncpy(g_Bots[i].inputDevice, val.c_str(), 64);
                    }
                }
               else if (key.rfind("Tom_", 0) == 0) { // If starts with tom
                    int i, j;
                    char subKeyBuf[64];
                    
                    if (sscanf(key.c_str(), "Tom_%d_%d_%s", &i, &j, subKeyBuf) == 3) {
                        if (i >= 0 && i < 6 && j >= 0 && j < 15) {
                            AccountSlot& acc = g_Bots[i].accounts[j];
                            std::string subKey(subKeyBuf);
                            if (subKey == "En") acc.autoTomEnabled = (val == "1");
                            else if (subKey == "Rem") acc.tomRemainingHours = std::stoi(val);
                            else if (subKey == "Start") acc.tomStartTime = std::stoll(val);
                            else if (subKey == "Next") acc.nextTomTime = std::stoll(val);
                            else if (subKey == "Cat") acc.tomCategory = std::stoi(val);
                            else if (subKey == "Item") acc.tomItemName = val;
                        }
                    }
                }
               else if (key.rfind("Acc_", 0) == 0) { 
                    int i, j;
                    char subKeyBuf[64];
                    if (sscanf(key.c_str(), "Acc_%d_%d_%s", &i, &j, subKeyBuf) == 3) {
                        if (i >= 0 && i < 6 && j >= 0 && j < 15) {
                            AccountSlot& acc = g_Bots[i].accounts[j];
                            std::string subKey(subKeyBuf);
                            try {
                                if (subKey == "CustomName") acc.name = val;
                                else if (subKey == "Lvl") acc.level = std::stoi(val);
                                else if (subKey == "Tag") acc.playerTag = val;
                                else if (subKey == "Name") acc.farmName = val;
                                else if (subKey == "Coin") acc.coinAmount = std::stoi(val);
                                else if (subKey == "Dia") acc.diamondAmount = std::stoi(val);
                                else if (subKey == "Friend") acc.isFriendWithStorage = (val == "1");
                            }
                            catch (...) {} 
                        }
                    }
                }
            }
        }
        in.close();
    }
}

// Saves barn info to the config file. so it can be displayed or be viewed in GUI
void SaveInventoryData() {
    std::string path = GetAppDataPath() + "\\nxrth_inventory.ini";
    std::ofstream out(path);
    if (out.is_open()) {
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 15; j++) {
                InventoryData& inv = g_Bots[i].accounts[j].currentInv;
                out << i << "," << j << "," << inv.bolt << "," << inv.tape << "," << inv.plank << ","
                    << inv.nail << "," << inv.screw << "," << inv.panel << ","
                    << inv.deed << "," << inv.mallet << "," << inv.marker << "," << inv.map << "\n";
            }
        }
        out.close();
    }
}
// Loads barn info from the config file.
void LoadInventoryData() {
    std::string path = GetAppDataPath() + "\\nxrth_inventory.ini";
    std::ifstream in(path);
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            std::stringstream ss(line);
            std::string item;
            std::vector<int> vals;
            while (std::getline(ss, item, ',')) {
                try { vals.push_back(std::stoi(item)); }
                catch (...) { vals.push_back(0); }
            }
            if (vals.size() == 12) {
                int i = vals[0]; int j = vals[1];
                if (i >= 0 && i < 6 && j >= 0 && j < 15) {
                    InventoryData& inv = g_Bots[i].accounts[j].currentInv;
                    inv.bolt = vals[2]; inv.tape = vals[3]; inv.plank = vals[4];
                    inv.nail = vals[5]; inv.screw = vals[6]; inv.panel = vals[7];
                    inv.deed = vals[8]; inv.mallet = vals[9]; inv.marker = vals[10]; inv.map = vals[11];
                    
                    g_Bots[i].accounts[j].previousInv = inv;
                }
            }
        }
        in.close();
    }
}

std::mutex g_VisionMutex;
cv::Mat g_VisionMat; // 
GLuint g_VisionTexture = 0; // 
int g_VisionTexWidth = 0;
int g_VisionTexHeight = 0;
std::atomic<bool> g_VisionLiveRunning = false;
int g_VisionSelectedInst = 0;
// Crash logger

LONG WINAPI NxrthCrashHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    std::string logPath = GetAppDataPath() + "\\crash_log.txt";
    std::ofstream out(logPath, std::ios::app); 
    if (out.is_open()) {
        time_t now = time(0);
        char dt[80];
        strftime(dt, sizeof(dt), "%Y-%m-%d %H:%M:%S", localtime(&now));

        out << "\n=========================================\n";
        out << "[CRASH TIME] : " << dt << "\n";
        out << "[FATAL ERROR]: NXRTH encountered a critical exception!\n";
        out << "[EXC CODE]   : 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << "\n";
        out << "[EXC ADDRESS]: 0x" << ExceptionInfo->ExceptionRecord->ExceptionAddress << "\n";
        out << "=========================================\n";
        out.close();
    }
	// Close the program after logging the crash details
    return EXCEPTION_EXECUTE_HANDLER;
}

// Bridge to convert openCV Mat to OpenGL Texture.
void UpdateVisionTexture(const cv::Mat& mat) {
    if (mat.empty()) return;
    cv::Mat rgba;
	// OPencv uses BGR, OpenGL uses RGB(A), so we need to convert the color format. 
    if (mat.channels() == 3) cv::cvtColor(mat, rgba, cv::COLOR_BGR2RGBA);
    else if (mat.channels() == 4) cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
    else cv::cvtColor(mat, rgba, cv::COLOR_GRAY2RGBA);

    if (g_VisionTexture != 0) {
        glDeleteTextures(1, &g_VisionTexture); // delete old image, so ram usage drops.
    }
    glGenTextures(1, &g_VisionTexture);
    glBindTexture(GL_TEXTURE_2D, g_VisionTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba.cols, rgba.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data);
    g_VisionTexWidth = rgba.cols;
    g_VisionTexHeight = rgba.rows;
}

// Bot's eye: Takes image, Draws points and grids
cv::Mat ProcessVisionFrame(int instanceId, cv::Mat rawScreen) {
    if (rawScreen.empty()) return rawScreen;
    cv::Mat drawMat = rawScreen.clone();

    // 1. SINGLE TARGETS (static Stuff that needs to be found once - Example: Shop, Sickle)
    auto DrawSingleTarget = [&](const std::string& tplPath, float thresh, cv::Scalar color, const std::string& label) {
        // we use useMargins = false so bot can scan EVERYTHING on the screen, not just the center.
        MatchResult res = FindImage(rawScreen, tplPath, thresh, false, 1.0f, false);
        if (res.found) {
			cv::circle(drawMat, cv::Point(res.x, res.y), 12, color, -1); // Circle with color fill
			cv::circle(drawMat, cv::Point(res.x, res.y), 16, cv::Scalar(0, 0, 0), 2); // Circle with black border
            cv::putText(drawMat, label, cv::Point(res.x + 20, res.y + 5), cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        }
        };

    // 2. Multi targets (Stuff that can be more than one in the frame - Example: Crates in the shop)
    auto DrawMultiTargets = [&](const std::string& tplPath, float thresh, cv::Scalar color, const std::string& label) {
        std::vector<MatchResult> results = FindAllImages(rawScreen, tplPath, thresh, 20, false);
        for (auto& res : results) {
            cv::rectangle(drawMat, cv::Point(res.x - 20, res.y - 20), cv::Point(res.x + 20, res.y + 20), color, 2);
            cv::putText(drawMat, label, cv::Point(res.x - 15, res.y - 25), cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
        }
        };

    // =====================================================================
    // BOT VISION TAB STUFF
    // =====================================================================

    // --- A. Buildings ---
    DrawSingleTarget(mailbox_templatePath, g_Thresholds.mailboxThreshold, cv::Scalar(255, 0, 0), "Mailbox"); // Blue dot
    DrawSingleTarget(shop_templatePath, g_Thresholds.shopThreshold, cv::Scalar(0, 255, 255), "Shop");        // Yellow Dot

    // --- B. UI BUTTONS ---
    DrawSingleTarget(cross_templatePath, g_Thresholds.crossThreshold, cv::Scalar(0, 0, 255), "Cross (X)");
    DrawSingleTarget(plus_templatePath, g_Thresholds.plusThreshold, cv::Scalar(0, 255, 0), "Plus (+)");
    DrawSingleTarget(create_sale_templatePath, g_Thresholds.createSaleThreshold, cv::Scalar(0, 255, 0), "Put on Sale");
    DrawSingleTarget(advertise_templatePath, g_Thresholds.advertiseThreshold, cv::Scalar(200, 200, 200), "Advertise Btn");
    DrawSingleTarget(levelup_templatePath, g_Thresholds.levelUpThreshold, cv::Scalar(255, 215, 0), "Level Up!");

    // --- C. SHOP CRATES (MULTI SCAN) ---
    DrawMultiTargets(crate_templatePath, g_Thresholds.crateThreshold, cv::Scalar(200, 150, 100), "Empty Crate");
    DrawMultiTargets(soldcrate_templatePath, 0.80f, cv::Scalar(50, 200, 50), "Sold Crate");

    // --- D. SEEDS, BUT ONLY SEARCH THE SELECTED CROP MODE. USER CAN CHOOSE THIS IN BOT CONFIGURATION TAB ---
    int cropMode = g_Bots[instanceId].testCropMode;
    std::string seedTpl = w_templatePath;
    float seedThresh = g_Thresholds.wheatThreshold;
    std::string seedName = "Wheat Seed";

    if (cropMode == 1) { seedTpl = c_templatePath; seedThresh = g_Thresholds.cornThreshold; seedName = "Corn Seed"; }
    else if (cropMode == 2) { seedTpl = carrot_templatePath; seedThresh = g_Thresholds.carrotThreshold; seedName = "Carrot Seed"; }
    else if (cropMode == 3) { seedTpl = soybean_templatePath; seedThresh = g_Thresholds.soybeanThreshold; seedName = "Soybean Seed"; }
    else if (cropMode == 4) { seedTpl = sugarcane_templatePath; seedThresh = g_Thresholds.sugarcaneThreshold; seedName = "Cane Seed"; }

    DrawSingleTarget(seedTpl, seedThresh, cv::Scalar(0, 255, 0), seedName);

    // --- E. DYNAMIC AREA TARGETS (COLOR FILTERED) ---
    // 1. GROWN CROPS (GREEN Grid)
    std::vector<MatchResult> grownCrops = FindGrownCrops(rawScreen, cropMode);
    for (auto& g : grownCrops) {
        cv::rectangle(drawMat, cv::Point(g.x - 15, g.y - 15), cv::Point(g.x + 15, g.y + 15), cv::Scalar(0, 255, 0), 2);
        cv::putText(drawMat, "CROP", cv::Point(g.x - 10, g.y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
    }

    // 2. EMPTY FIELDS (PINK Grid)
    std::vector<MatchResult> emptyFields = FindEmptyFields(rawScreen, false);
    for (auto& e : emptyFields) {
        cv::rectangle(drawMat, cv::Point(e.x - 15, e.y - 15), cv::Point(e.x + 15, e.y + 15), cv::Scalar(255, 0, 255), 2);
        cv::putText(drawMat, "EMPTY", cv::Point(e.x - 10, e.y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 0, 255), 1);
    }

    return drawMat;
}
//====================================================================
// GUI STUFF GOES HERE
//====================================================================
void RenderCustomTitleBar(GLFWwindow* window) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.07f, 1.0f));
    ImGui::BeginChild("TitleBar", ImVec2(ImGui::GetWindowWidth(), 30), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::SetCursorPosY(6);
    ImGui::SetCursorPosX(15);
    ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), "NXRTH");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "MULTI BOT");

    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        static POINT offset;
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            int wx, wy;
            glfwGetWindowPos(window, &wx, &wy);
            offset.x = cursorPos.x - wx;
            offset.y = cursorPos.y - wy;
        }
        glfwSetWindowPos(window, cursorPos.x - offset.x, cursorPos.y - offset.y);
    }

    // =========================================================
	// EMULATOR SELECTION 
    // =========================================================
    ImGui::SameLine(ImGui::GetWindowWidth() - 250);
    ImGui::SetCursorPosY(4);
    ImGui::SetNextItemWidth(140);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    if (ImGui::Combo("##GlobalEmuMode", &g_GlobalEmulatorMode, "MEmu\0LDPlayer\0")) {
        if (g_GlobalEmulatorMode == 0) { // MEmu SELECTED
            strncpy(g_AdbPathBuf, "C:\\Program Files\\Microvirt\\MEmu\\adb.exe", 260);
            strncpy(g_MEmuPathBuf, "C:\\Program Files\\Microvirt\\MEmu\\MEmuConsole.exe", 260);

            // UPDATE PATHS IN THE BOT TOO.
            kAdbPath = "C:\\Program Files\\Microvirt\\MEmu\\adb.exe";
            kMEmuConsolePath = "C:\\Program Files\\Microvirt\\MEmu\\MEmuConsole.exe";

            for (int i = 0; i < 6; i++) {
                g_Bots[i].emulatorType = 0;
                std::string dp = "127.0.0.1:" + std::to_string(21503 + (i * 10));
                strncpy(g_Bots[i].adbSerial, dp.c_str(), 64);
                std::string vm = (i == 0) ? "MEmu" : "MEmu_" + std::to_string(i);
                strncpy(g_Bots[i].vmName, vm.c_str(), 64);
            }
        }
        else { // LDPlayer SELECTED
            strncpy(g_AdbPathBuf, "C:\\LDPlayer\\LDPlayer9\\adb.exe", 260);
            strncpy(g_MEmuPathBuf, "C:\\LDPlayer\\LDPlayer9\\ldconsole.exe", 260);

            // UPDATE PATHS IN THE BOT TOO.
            kAdbPath = "C:\\LDPlayer\\LDPlayer9\\adb.exe";
            kMEmuConsolePath = "C:\\LDPlayer\\LDPlayer9\\ldconsole.exe";

            for (int i = 0; i < 6; i++) {
                g_Bots[i].emulatorType = 1;
                g_Bots[i].emuIndex = i;
                std::string dp = "127.0.0.1:" + std::to_string(5555 + (i * 2));
                strncpy(g_Bots[i].adbSerial, dp.c_str(), 64);
                std::string vm = "LDPlayer-" + std::to_string(i);
                strncpy(g_Bots[i].vmName, vm.c_str(), 64);
            }
        }
        SaveConfig();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(Tr("Select Emulator Engine"));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // --- MINIMIZE AND CROSS BUTTONS ---
    ImGui::SameLine(ImGui::GetWindowWidth() - 75);
    ImGui::SetCursorPosY(4);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("_", ImVec2(30, 22))) {
        glfwIconifyWindow(window);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine(ImGui::GetWindowWidth() - 40);
    ImGui::SetCursorPosY(4);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("X", ImVec2(30, 22))) {
        ExitProcess(0);
    }
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::PopStyleColor();
}




void InitializeBots() {


    for (int i = 0; i < 6; i++) {
        g_Bots[i].id = i;
        g_Bots[i].emulatorType = g_GlobalEmulatorMode;
        g_Bots[i].emuIndex = i;

		// PORTS CHANGE BASED ON THE EMULATOR CHOICE, SO WE SET THEM ACCORDINGLY.
        if (g_GlobalEmulatorMode == 1) { // LDPlayer
            std::string defaultPort = "127.0.0.1:" + std::to_string(5555 + (i * 2));
            strcpy(g_Bots[i].adbSerial, defaultPort.c_str());
            std::string defaultVM = "LDPlayer-" + std::to_string(i);
            strcpy(g_Bots[i].vmName, defaultVM.c_str());
        }
        else { // MEmu
            std::string defaultPort = "127.0.0.1:" + std::to_string(21503 + (i * 10));
            strcpy(g_Bots[i].adbSerial, defaultPort.c_str());
            std::string defaultVM = (i == 0) ? "MEmu" : "MEmu_" + std::to_string(i);
            strcpy(g_Bots[i].vmName, defaultVM.c_str());
        }

        for (int j = 0; j < 15; j++) {
            if (g_Bots[i].accounts[j].name.empty()) {
                g_Bots[i].accounts[j].name = "Account " + std::to_string(j + 1);
            }

            std::string path = GetAppDataPath() + "\\Backups\\Instance_" + std::to_string(i) + "\\account_" + std::to_string(j + 1) + ".nxrth";

            if (fs::exists(path) && fs::file_size(path) > 0) {
                g_Bots[i].accounts[j].hasFile = true;
                g_Bots[i].accounts[j].fileName = path;
            }
        }
    }
    g_Bots[0].isActive = true;
}

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    *out_texture = image_texture;
    if (out_width) *out_width = image_width;
    if (out_height) *out_height = image_height;
    return true;
}
// TOTAL RUNTIME OF THE BOT
std::string GetRuntimeStr() {
    bool anyRunning = false;
    for (int i = 0; i < 6; i++) if (g_Bots[i].isRunning) anyRunning = true;
    if (!anyRunning) return "00:00:00";
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - g_BotStartTime).count();
    int h = elapsed / 3600;
    int m = (elapsed % 3600) / 60;
    int s = elapsed % 60;
    char buf[32];
    sprintf(buf, "%02d:%02d:%02d", h, m, s);
    return std::string(buf);
}
// GUI THEME: DARK BACKGROUND WITH GOLD ACCENTS
void SetPremiumTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.85f, 0.65f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.95f, 0.75f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.75f, 0.55f, 0.02f, 1.00f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
}

void RenderLogo() {
    ImGui::SetWindowFontScale(2.0f);
    ImGui::TextColored(ImVec4(0.5f, 0.0f, 0.8f, 1.0f), "NX");
    ImGui::SameLine(0, 0);
    ImGui::TextColored(ImVec4(0.7f, 0.3f, 1.0f, 1.0f), "RTH");

    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
// BUTTONS ON THE GUI, WITH GOLDEN HIGHLIGHT WHEN SELECTED. ALSO HANDLES THE SELECTION LOGIC.
void ModernButton(const char* label, GLuint icon, int id, int& current) {
    bool isSelected = (current == id);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float btn_width = 250.0f;
    float btn_height = 50.0f;

    ImU32 bg_col = isSelected ? IM_COL32(217, 165, 30, 50) : IM_COL32(40, 40, 40, 255);
    if (ImGui::IsMouseHoveringRect(p, ImVec2(p.x + btn_width, p.y + btn_height))) {
        bg_col = IM_COL32(217, 165, 30, 80);
        if (ImGui::IsMouseClicked(0)) current = id;
    }
    draw_list->AddRectFilled(p, ImVec2(p.x + btn_width, p.y + btn_height), bg_col, 8.0f);
    if (icon != 0) draw_list->AddImage((void*)(intptr_t)icon, ImVec2(p.x + 15, p.y + 10), ImVec2(p.x + 45, p.y + 40));
    ImU32 text_col = isSelected ? IM_COL32(217, 165, 30, 255) : IM_COL32(230, 230, 230, 255);
    draw_list->AddText(ImVec2(p.x + 60, p.y + 15), text_col, label);
    ImGui::Dummy(ImVec2(btn_width, btn_height));
    ImGui::Spacing();
}

// STARTS EMULATOR AND THE GAME! FUNCTION NAME IS MEMUANDGAME BUT IT ALSO WORKS WITH LDPLAYER BECAUSE I AM TOO LAZY TO CHANGE IT.
void StartMEmuAndGame(int instanceId) {
    AddLog(instanceId, "Starting Emulator Environment...", ImVec4(0, 1, 1, 1));

    std::thread([instanceId]() {
        BotInstance& bot = g_Bots[instanceId];

		// 1. KILL ADB SERVER TO PREVENT ANY CONNECTION ISSUES.
        RunCmdHidden("cmd.exe /c \"\"" + kAdbPath + "\" kill-server\"");

        // 2. STARTS EMULATOR
        std::string cmd;
        if (bot.emulatorType == 1) { // LDPlayer
            cmd = "cmd.exe /c \"\"" + kMEmuConsolePath + "\" launch --index " + std::to_string(bot.emuIndex) + "\"";
        }
        else { // MEmu
            cmd = "cmd.exe /c \"\"" + kMEmuConsolePath + "\" " + std::string(bot.vmName) + "\"";
        }
        RunCmdHidden(cmd);

        AddLog(instanceId, "Waiting for Android to boot (Takes 15-30s)...", ImVec4(1, 1, 0, 1));

        // 3. WAIT FOR THE ANDROID TO LOAD AND CONNCET ADB!
        // WARNING: THIS IS VERY IMPORTANT FOR LDPLAYER BECAUSE LDPLAYER NOT WORKING LIKE MEMU, YOU HAVE TO CONNECT ADB AND STAY CONNECTED. IN MEMU YOU JUST SEND COMMANDS BUT THIS IS NOT LIKE THAT
        // SO PLEASE WATCH OUT FOR THIS PART BECAUSE ITS THE MOST IMPORTANT IN MY OPINION.
        bool booted = false;
		for (int i = 0; i < 40; i++) { // MAX 80 SECONDS WAIT (40*2=80)
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // FORCE CONNECTION
            RunCmdHidden("cmd.exe /c \"\"" + kAdbPath + "\" connect " + std::string(bot.adbSerial) + "\"");

            // CHECK BOOT STATUS
            std::string tempFile = "C:\\Users\\Public\\boot_check_" + std::to_string(instanceId) + ".txt";
            remove(tempFile.c_str());
            std::string checkCmd = "cmd.exe /c \"\"" + kAdbPath + "\" -s " + std::string(bot.adbSerial) + " shell getprop sys.boot_completed > \"" + tempFile + "\"\"";
            RunCmdHidden(checkCmd);

            std::ifstream file(tempFile);
            std::string result;
            if (file.is_open()) {
                std::getline(file, result);
                file.close();
            }

            if (result.find("1") != std::string::npos) {
                booted = true;
                break;
            }
        }

        if (!booted) {
            AddLog(instanceId, "ERROR: Android boot timeout! ADB offline.", ImVec4(1, 0, 0, 1));
            return;
        }

        AddLog(instanceId, "Android is ready. Launching Hay Day...", ImVec4(0, 1, 0, 1));
        std::this_thread::sleep_for(std::chrono::seconds(3)); // WAIT 3 SECS FOR ANDROID HOMEPAGE LOAD.

        // 4. START THE GAME
        std::string launchCmd = "cmd.exe /c \"\"" + kAdbPath + "\" -s " + std::string(bot.adbSerial) + " shell monkey -p com.supercell.hayday -c android.intent.category.LAUNCHER 1\"";
        RunCmdHidden(launchCmd);

        AddLog(instanceId, "App Launched successfully.", ImVec4(0, 1, 0, 1));
        }).detach();
}




void RenderTemplateRow(const char* label, char* buffer, size_t bufferSize, std::string& targetPath, float* threshold = nullptr) {
    ImGui::PushID(label);
    float availWidth = ImGui::GetContentRegionAvail().x;
    float loadButtonWidth = 80.0f;
    float thresholdWidth = threshold ? 150.0f : 0.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float inputWidth = availWidth - loadButtonWidth - thresholdWidth - spacing * (threshold ? 2 : 1);

    ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "%s", label);
    ImGui::SetNextItemWidth(inputWidth);
    if (ImGui::InputText("##path", buffer, bufferSize)) {
        targetPath = buffer;
    }
    ImGui::SameLine();
    if (ImGui::Button(Tr("Load"), ImVec2(loadButtonWidth, 0))) {
        std::string filePath = OpenFileDialog("PNG Files\0*.png\0All Files\0*.*\0");
        if (!filePath.empty()) {
            strncpy(buffer, filePath.c_str(), bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            targetPath = filePath;
            AddLog(0, std::string(Tr("Loaded template: ")) + filePath, ImVec4(0, 1, 0, 1));
        }
    }
    if (threshold) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(thresholdWidth);
        ImGui::SliderFloat("##threshold", threshold, 0.1f, 0.99f, "%.2f");
    }
    ImGui::PopID();
}

void RenderApp() {
    ValidatePaths();
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + 30.0f));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - 30.0f));

    ImGui::Begin("MainApp", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::BeginChild("Sidebar", ImVec2(270, 0), true, ImGuiWindowFlags_NoScrollbar);
    {
        if (icon_logo != 0) {
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 100) * 0.5f);
            ImGui::Image((void*)(intptr_t)icon_logo, ImVec2(100, 100));
        }
        else RenderLogo();

        if (icon_logo != 0) {
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 120) * 0.5f);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "PREMIUM v2.0");
        }
        ImGui::Spacing();

        ModernButton(Tr("DASHBOARD"), icon_dashboard, 0, g_CurrentTab);
        ModernButton(Tr("BOT MANAGER"), icon_config, 1, g_CurrentTab);
        ModernButton(Tr("LOGS"), icon_logs, 4, g_CurrentTab);
        ModernButton(Tr("TEMPLATES"), icon_templates, 5, g_CurrentTab);
        ModernButton(Tr("REMOTE & WEBHOOK"), icon_cloud, 2, g_CurrentTab);

        GLuint settingsIconToUse = icon_settings;
        if (!g_AdbValid || !g_MEmuValid) settingsIconToUse = icon_warning;
        ModernButton(Tr("SETTINGS"), settingsIconToUse, 3, g_CurrentTab);
        ModernButton(Tr("HOW TO USE"), icon_question, 6, g_CurrentTab);
        ModernButton(Tr("BOT VISION"), icon_dashboard, 7, g_CurrentTab);
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 120);
        ImGui::Separator();

        ImGui::SetNextItemWidth(180);
        ImGui::Combo("##Lang", &g_Language, "English\0Türkçe\0Español\0Português\0Русский\0Deutsch\0");
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();

        ImGui::Spacing();
        int activeCount = (int)g_Bots[0].isActive + (int)g_Bots[1].isActive + (int)g_Bots[2].isActive + (int)g_Bots[3].isActive;
        ImGui::TextColored(ImVec4(0, 1, 0, 1), Tr("Active Instances: %d/4"), activeCount);
        ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("User: %s"), g_Username);

    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("Content", ImVec2(0, 0), true);
    {
        if (g_CurrentTab == 0) {
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("INSTANCES OVERVIEW"));
            ImGui::Separator(); ImGui::Spacing();

            int globalHarvests = 0, globalSales = 0, globalCoins = 0, globalDiamonds = 0;
            for (int i = 0; i < 6; i++) {
                if (g_Bots[i].isActive) {
                    globalHarvests += g_Bots[i].totalHarvest;
                    globalSales += g_Bots[i].totalSales;
                    globalCoins += g_Bots[i].accounts[g_Bots[i].currentAccountIndex].coinAmount;
                    globalDiamonds += g_Bots[i].accounts[g_Bots[i].currentAccountIndex].diamondAmount;
                }
            }

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.09f, 1.0f));
            ImGui::BeginChild("GlobalStats", ImVec2(0, 60), true);
            ImGui::Columns(4, "GlobalCols", false);

            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), Tr("TOTAL RUNTIME"));
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", GetRuntimeStr().c_str());
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), Tr("TOTAL HARVEST / SALES"));
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d / %d", globalHarvests, globalSales);
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), Tr("TOTAL COINS"));
            if (icon_coin != 0) { ImGui::Image((void*)(intptr_t)icon_coin, ImVec2(14.0f, 14.0f)); ImGui::SameLine(); }
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%d", globalCoins);
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), Tr("TOTAL DIAMONDS"));
            if (icon_dia != 0) { ImGui::Image((void*)(intptr_t)icon_dia, ImVec2(14.0f, 14.0f)); ImGui::SameLine(); }
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "%d", globalDiamonds);

            ImGui::Columns(1);
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            float availY = ImGui::GetContentRegionAvail().y;
            float cardHeight = (availY - ImGui::GetStyle().ItemSpacing.y) / 2.0f;

            ImGui::Columns(2, "DashGrid", false);
            for (int i = 0; i < 6; i++) {
                if (!g_Bots[i].isActive) ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 0.5f));
                else ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.14f, 0.12f, 1.0f));

                std::string cardId = "InstanceCard" + std::to_string(i);
                ImGui::BeginChild(cardId.c_str(), ImVec2(0, cardHeight), true);

                ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("INSTANCE #%d"), i + 1);
                ImGui::SameLine();
                if (g_Bots[i].isActive) ImGui::TextColored(ImVec4(0, 1, 0, 1), Tr("[ONLINE]"));
                else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), Tr("[OFFLINE]"));
                ImGui::Separator(); ImGui::Spacing();

                if (g_Bots[i].isActive) {
                    AccountSlot& activeAcc = g_Bots[i].accounts[g_Bots[i].currentAccountIndex];

                    ImGui::Text(Tr("ADB: %s"), g_Bots[i].adbSerial);
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), Tr("Slot: %s"), activeAcc.name.c_str());
                    ImGui::Spacing();

                    ImGui::SetWindowFontScale(1.1f);
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 1.0f, 1.0f), Tr("Farm: %s | Lvl: %d"), activeAcc.farmName.c_str(), activeAcc.level);
                    ImGui::SetWindowFontScale(1.0f);

                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), Tr("Tag: %s"), activeAcc.playerTag.c_str());
                    ImGui::Spacing();

                    if (icon_coin != 0) { ImGui::Image((void*)(intptr_t)icon_coin, ImVec2(16.0f, 16.0f)); ImGui::SameLine(); }
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%d", activeAcc.coinAmount);
                    ImGui::SameLine(); ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " | "); ImGui::SameLine();
                    if (icon_dia != 0) { ImGui::Image((void*)(intptr_t)icon_dia, ImVec2(16.0f, 16.0f)); ImGui::SameLine(); }
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "%d", activeAcc.diamondAmount);

                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                    InventoryData& activeInv = activeAcc.currentInv;
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), Tr("Barn: Bolt: %d | Tape: %d | Plank: %d"), activeInv.bolt, activeInv.tape, activeInv.plank);
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), Tr("Silo: Nail: %d | Screw: %d | Panel: %d"), activeInv.nail, activeInv.screw, activeInv.panel);
                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("Harvests: %d | Sales: %d"), g_Bots[i].totalHarvest, g_Bots[i].totalSales);
                    ImGui::Spacing();
                    ImVec4 stColor = g_Bots[i].isRunning ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0.5f, 0, 1);
                    ImGui::TextColored(stColor, Tr("Status: %s"), Tr(g_Bots[i].statusText.c_str()));
                }
                else {
                    ImGui::TextDisabled(Tr("Enable this instance in 'Bot Manager'"));
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::NextColumn(); 
            }
            ImGui::Columns(1);
        }

        if (g_CurrentTab == 1) {
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("BOT INSTANCE MANAGER"));
            ImGui::Separator();
            if (ImGui::BeginTabBar("InstanceTabs")) {
                for (int i = 0; i < 6; i++) {
                    std::string tabName = std::string(Tr("Instance #")) + std::to_string(i + 1);

                   
                    ImGuiTabItemFlags tabFlags = 0;
                    if (g_TargetTabToSelect == i) {
                        tabFlags |= ImGuiTabItemFlags_SetSelected;
                        g_TargetTabToSelect = -1; 
                    }

                    if (ImGui::BeginTabItem(tabName.c_str(), nullptr, tabFlags)) {
                        g_SelectedInstanceUI = i;
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("CONNECTION SETTINGS"));
                        ImGui::Checkbox(Tr("Enable This Instance"), &g_Bots[i].isActive);

                        if (g_Bots[i].isActive) {
                            ImGui::Spacing();
                            BotInstance& bot = g_Bots[i];
                            int savedAccs = 0;
                            for (int i = 0; i < 10; i++) if (bot.accounts[i].hasFile) savedAccs++;

                            ImGui::TextColored(ImVec4(0, 1, 1, 1), "SYSTEM ARCHITECTURE & MAINTENANCE");
                            ImGui::Separator();
                            ImGui::Spacing();

                            ImGui::Columns(2, "ArchAndMaintCols", false); 

                            // ---------------------------------------------------------
                            // LEFT COLUMN
                            // ---------------------------------------------------------
                            // SINGLE MODE
                            if (ImGui::Checkbox("Enable Single Mode", &bot.useSingleMode)) {
                                bot.useMultiMode = !bot.useSingleMode;
                            }

                            // MULTI MODE 
                            bool multiLock = (savedAccs < 2);
                            if (multiLock) {
                                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                                bot.useMultiMode = false;
                            }

                            if (ImGui::Checkbox("Enable Multi Mode (Account rotation)", &bot.useMultiMode)) {
                                bot.useSingleMode = !bot.useMultiMode;
                            }

                            if (multiLock && ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("You have to Save at least 2 farms to be able to enable this!");
                            }
                            if (multiLock) ImGui::PopStyleVar();

                            // REVIVE MODE
                            if (bot.useSingleMode) {
                                ImGui::Spacing();
                                ImGui::Checkbox("Enable Revive Mode", &bot.useReviveMode);
                                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scans for the template you enter(could be anything & custom) after X seconds. If cant find 3 times, re-opens game.");

                                if (bot.useReviveMode) {
                                    ImGui::SetNextItemWidth(140); 
                                    ImGui::SliderInt("Check (Sec)", &bot.reviveCheckInterval, 60, 600);

                                    ImGui::SetNextItemWidth(140);
                                    ImGui::InputText("Custom Template", bot.reviveTemplatePath, MAX_PATH);
                                    ImGui::SameLine();
                                    if (ImGui::Button("Browse##revive")) {
                                        std::string path = OpenFileDialog();
                                        if (!path.empty()) strncpy(bot.reviveTemplatePath, path.c_str(), MAX_PATH);
                                    }
                                }
                            }

                            ImGui::NextColumn();

                            // ---------------------------------------------------------
                            // RIGHT COLUMN
                            // ---------------------------------------------------------
                            ImGui::BeginChild("JanitorSet", ImVec2(0, 130), true); 
                            ImGui::TextColored(ImVec4(0.35f, 0.65f, 0.98f, 1.0f), "System Maintenance (The Janitor)");
                            ImGui::Separator();
                            ImGui::Spacing();

                            ImGui::Checkbox("Enable Auto RAM & Cache Cleaner", &bot.enableJanitor);
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Prevents Memory Leaks by restarting the emulator completely after X cycles.");

                            if (bot.enableJanitor) {
                                ImGui::Spacing();
                                ImGui::SetNextItemWidth(180);
                                ImGui::SliderInt("Clean Every X Cycles", &bot.janitorLimit, 3, 50);
                            }
                            ImGui::EndChild();

                            ImGui::Columns(1); 
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                           
                             
                            ImGui::Columns(2, "ConnectionCols", false);

                           
                            ImGui::SetColumnWidth(0, 380.0f);

                            
                            ImGui::TextDisabled(Tr("(Target Port: %s)"), g_Bots[i].adbSerial);
                            ImGui::PushItemWidth(200);
                            ImGui::InputText("##adb", g_Bots[i].adbSerial, 64);
                            ImGui::PopItemWidth();
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip(Tr("Example: 127.0.0.1:21503 for MEmu 1"));
                            ImGui::Spacing();

                            ImGui::Text(Tr("Input Device (Touchscreen):"));
                            ImGui::PushItemWidth(120);
                            ImGui::InputText("##inputdev", g_Bots[i].inputDevice, 64);
                            ImGui::PopItemWidth();
                            ImGui::SameLine();
                            ImGui::PushItemWidth(120);
                            ImGui::InputText("##vm", g_Bots[i].vmName, 64);
                            ImGui::PopItemWidth();

                            ImGui::NextColumn();

                          
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
                            ImGui::TextDisabled(Tr("Don't have an emulator?"));

                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f); 
                            if (g_Bots[i].isCreatingEmulator) {
                                ImGui::BeginDisabled();
                                ImGui::Button(Tr(" CREATING... (~20s)"), ImVec2(220, 50));
                                ImGui::EndDisabled();
                            }
                            else {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.85f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.95f, 1.0f));
                                if (ImGui::Button(Tr(" CREATE NEW MEMU\n& AUTO-LINK"), ImVec2(220, 50))) {
                                    g_Bots[i].isCreatingEmulator = true;
                                    std::thread([i]() { RunEmulatorFactory(i); }).detach();
                                }
                                ImGui::PopStyleColor(2);
                            }

                            ImGui::Columns(1);
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                            
                            ImGui::Columns(2, "ToolsAndActions", false);

                            
                            if (i != 5) { 
                                ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("TOOLS & DIAGNOSTICS"));
                                ImGui::Spacing();
                                ImGui::Text(Tr("Select Mode:")); ImGui::SameLine();
                                const char* crops[] = { Tr("Wheat (2m)"), Tr("Corn (5m)"), Tr("Carrot (10m)"), Tr("Soybean (20m)"), Tr("Sugarcane (30m)") };
                                ImGui::PushItemWidth(160);
                                ImGui::Combo("##cropselector", &g_Bots[i].testCropMode, crops, IM_ARRAYSIZE(crops));
                                ImGui::PopItemWidth();
                                ImGui::SameLine(0, 15);
                                ImGui::Checkbox(Tr("Enable Random Salecycle"), &g_Bots[i].enableRandomSaleCycle);

                                if (ImGui::Button(Tr("TEST SEED"), ImVec2(120, 30))) {
                                    std::string tPath = (g_Bots[i].testCropMode == 0) ? w_templatePath : c_templatePath;
                                    float thresh = (g_Bots[i].testCropMode == 0) ? g_Thresholds.wheatThreshold : g_Thresholds.cornThreshold;
                                    PerformTemplateTest(i, tPath, "Seed Check", thresh, false);
                                }
                                ImGui::SameLine();
                                if (ImGui::Button(Tr("TEST SICKLE"), ImVec2(120, 30))) {
                                    PerformTemplateTest(i, s_templatePath, "Sickle Check", g_Thresholds.sickleThreshold, false);
                                }

                                ImGui::Spacing();
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
                                if (ImGui::Button("Inject Important Files", ImVec2(200, 60))) {
                                    InjectImportantFiles(i);
                                }
                                ImGui::PopStyleColor();
                            }
                            else { // SPECIAL SETTINGS FOR 6TH INSTANCE (STORAGE ACCOUNT (AUTO ITEM TRANSFER STUFF....))
                                ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), Tr("STORAGE & TRANSFER MODULE"));
                                ImGui::TextDisabled(Tr("This instance acts as the main warehouse."));
                                ImGui::Spacing();

                                // STORAGE ACC TAG INPUT
                                ImGui::PushItemWidth(200);
                                ImGui::InputText(Tr("Storage Account Tag"), g_StorageTagBuf, IM_ARRAYSIZE(g_StorageTagBuf));
                                ImGui::PopItemWidth();

                                // SAVE
                                if (ImGui::IsItemDeactivatedAfterEdit()) {
                                    g_StorageTag = g_StorageTagBuf;
                                    SaveConfig();
                                }

                                ImGui::Spacing();
                                ImGui::SliderInt(Tr("Auto-Transfer Threshold (X)"), &g_TransferThreshold, 3, 150);
                                ImGui::TextDisabled(Tr("If any farming account has >= X items, it will signal this warehouse."));
                            }

                           
                            ImGui::NextColumn();
                            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("MAIN ACTIONS"));
                            ImGui::Spacing();

                            if (ImGui::Button(Tr("LAUNCH EMULATOR + HAY DAY"), ImVec2(250, 45))) {
                                StartMEmuAndGame(i);
                            }
                            ImGui::Spacing();

                            // =========================================================
                            // START / STOP BUTTONS
                            // =========================================================
                            if (i == 5) { //PURPLE START BOT BUTTON FOR STORAGE ACCOUNT.
                                if (g_Bots[i].isRunning) {
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.5f, 1.0f));
                                    if (ImGui::Button(Tr("STOP AUTO TRANSFER"), ImVec2(250, 55))) {
                                        g_Bots[i].isRunning = false;
                                        g_Bots[i].statusText = "TRANSFER STOPPED";
                                        AddLog(i, "Storage Auto-Transfer Mode Halted.", ImVec4(1, 0.5f, 0.5f, 1));
                                    }
                                    ImGui::PopStyleColor();
                                }
                                else {
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.8f, 1.0f)); 
                                    if (ImGui::Button(Tr("START AUTO TRANSFER"), ImVec2(250, 55))) {
                                        g_Bots[i].isRunning = true;
                                        g_Bots[i].statusText = "LISTENING FOR SIGNALS";
                                        AddLog(i, "Storage Auto-Transfer Activated. Listening on encrypted radio channel...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
                                        std::thread([i]() { RunStorageMaster(i); }).detach();
                                    }
                                    ImGui::PopStyleColor();
                                }
                            }
                            else { 
                                if (g_Bots[i].isRunning) {
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
                                    if (ImGui::Button(Tr("STOP BOT"), ImVec2(250, 55))) {
                                        g_Bots[i].isRunning = false;
                                        g_Bots[i].statusText = "STOPPED";
                                        AddLog(i, "BOT Stopped by User.", ImVec4(1, 0.5f, 0.5f, 1));
                                    }
                                    ImGui::PopStyleColor();
                                }
                                else {
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                                    if (ImGui::Button(Tr("START BOT"), ImVec2(250, 55))) {
                                        g_Bots[i].isRunning = true;
                                        g_Bots[i].statusText = "STARTING...";
                                        AddLog(i, "BOT Started.", ImVec4(0, 1, 0, 1));
                                        std::thread([i]() { RunPremiumBot(i); }).detach();
                                    }
                                    ImGui::PopStyleColor();
                                }
                            }

                           
                            ImGui::Columns(1);
                            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                            
                            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("ACCOUNT MANAGER & INSPECTOR"));
                            ImGui::TextDisabled(Tr("Click on a slot to expand details, edit names, and manage saves."));
                            ImGui::Separator(); ImGui::Spacing();

                            
                            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

                            for (int acc = 0; acc < 15; acc++) {
                                AccountSlot& currAcc = g_Bots[i].accounts[acc];
                                ImGui::PushID(acc); // PREVENT ID COLLISIONS

                                
                                std::string headerLabel = "SLOT " + std::to_string(acc + 1) + " : " + currAcc.name;
                                if (currAcc.hasFile) {
                                    headerLabel += " | " + (currAcc.farmName.empty() ? "Unknown" : currAcc.farmName) + " (Lvl " + std::to_string(currAcc.level) + ")";
                                }
                                else {
                                    headerLabel += " | [EMPTY]";
                                }
                                headerLabel += "###AccHeader_" + std::to_string(acc);
                               
                                bool isCurrent = (g_Bots[i].currentAccountIndex == acc);
                                if (isCurrent) {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.65f, 0.12f, 1.0f));
                                }
                                else if (!currAcc.hasFile) {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // GREY IF EMPTY | GREY FIVE NINE TO THE GRAVE
                                }

                                bool isOpen = ImGui::CollapsingHeader(headerLabel.c_str());

                                if (isCurrent || !currAcc.hasFile) ImGui::PopStyleColor(); 

                                if (isOpen) {
                                    ImGui::Indent(15.0f);
                                    ImGui::Spacing();

                                    ImGui::TextDisabled(Tr("Custom Name:"));
                                    ImGui::SameLine();
                                    char nameBuf[64];
                                    strncpy(nameBuf, currAcc.name.c_str(), sizeof(nameBuf));
                                    ImGui::PushItemWidth(250.0f);
                                    if (ImGui::InputText("##accname", nameBuf, sizeof(nameBuf))) {
                                        currAcc.name = nameBuf;
                                        SaveConfig();
                                    }
                                    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
                                    ImGui::PopItemWidth();

                                    ImGui::SameLine(0, 30.0f); 

                                 
                                    if (!isCurrent) {
                                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                                        if (ImGui::Button(Tr("SET AS ACTIVE SLOT"), ImVec2(150, 25))) {
                                            g_Bots[i].currentAccountIndex = acc;
                                        }
                                        ImGui::PopStyleColor();
                                    }
                                    else {
                                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), Tr("[ CURRENTLY ACTIVE ]"));
                                    }

                                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                                    
                                    if (currAcc.hasFile) {
                                        ImGui::Columns(3, "AccDetailsCols", false);

                                        
                                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("PROFILE"));
                                        ImGui::Text(Tr("Tag: %s"), currAcc.playerTag.c_str());
                                        if (icon_coin != 0) { ImGui::Image((void*)(intptr_t)icon_coin, ImVec2(14, 14)); ImGui::SameLine(); }
                                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%d", currAcc.coinAmount);
                                        if (icon_dia != 0) { ImGui::Image((void*)(intptr_t)icon_dia, ImVec2(14, 14)); ImGui::SameLine(); }
                                        ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "%d", currAcc.diamondAmount);
                                        ImGui::NextColumn();

                                      
                                        InventoryData& inv = currAcc.currentInv;
                                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), Tr("BARN"));
                                        ImGui::Text(Tr("Bolt: %d"), inv.bolt);
                                        ImGui::Text(Tr("Plank: %d"), inv.plank);
                                        ImGui::Text(Tr("Tape: %d"), inv.tape);
                                        ImGui::NextColumn();

                                      
                                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), Tr("SILO"));
                                        ImGui::Text(Tr("Nail: %d"), inv.nail);
                                        ImGui::Text(Tr("Screw: %d"), inv.screw);
                                        ImGui::Text(Tr("Panel: %d"), inv.panel);

                                        ImGui::Columns(1);
                                    }
                                    else {
                                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), Tr("EMPTY SLOT - NO DATA SAVED YET."));
                                        ImGui::TextDisabled(Tr("Enter game, skip tutorial, reach level 7 and click 'SAVE GAME' below."));
                                    }

                                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                                    
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.50f, 0.10f, 1.0f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.60f, 0.20f, 1.0f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.40f, 0.00f, 1.0f));
                                    if (ImGui::Button(Tr("SAVE GAME TO THIS SLOT"), ImVec2(250, 40))) {
                                        std::thread([i, acc]() { SaveAccountToSlot(i, acc); }).detach();
                                    }
                                    ImGui::PopStyleColor(3);

                                    ImGui::SameLine();

                                    
                                    if (!currAcc.hasFile) ImGui::BeginDisabled();
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.60f, 0.85f, 1.0f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.70f, 0.95f, 1.0f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.50f, 0.75f, 1.0f));
                                    if (ImGui::Button(Tr("LOAD SLOT TO EMULATOR"), ImVec2(250, 40))) {
                                        std::thread([i, acc]() { LoadAccountFromSlot(i, acc); }).detach();
                                    }
                                    ImGui::PopStyleColor(3);
                                    if (!currAcc.hasFile) ImGui::EndDisabled();

                                    ImGui::Spacing(); ImGui::Spacing();
                                    ImGui::Unindent(15.0f);
                                }
                                ImGui::PopID();
                            }
                            ImGui::PopStyleColor(3); 
                        }
                        else {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), Tr("Instance is disabled."));
                        }
                        ImGui::EndTabItem();
                    }
                }

                if (ImGui::BeginTabItem(Tr("Account Management"))) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("TRANSFER ACCOUNTS BETWEEN INSTANCES"));
                    ImGui::TextDisabled(Tr("Move an account data file from one slot to another."));
                    ImGui::Separator(); ImGui::Spacing();

                    static int swpSrcInst = 0, swpSrcSlot = 0;
                    static int swpDstInst = 1, swpDstSlot = 0;

                    const char* instItems[] = {
     Tr("Instance 1"), Tr("Instance 2"), Tr("Instance 3"),
     Tr("Instance 4"), Tr("Instance 5"), Tr("Instance 6 (STORAGE)")
                    };
                    const char* slotItems[] = {
                        Tr("Slot 1"), Tr("Slot 2"), Tr("Slot 3"), Tr("Slot 4"), Tr("Slot 5"),
                        Tr("Slot 6"), Tr("Slot 7"), Tr("Slot 8"), Tr("Slot 9"), Tr("Slot 10"),
                        Tr("Slot 11"), Tr("Slot 12"), Tr("Slot 13"), Tr("Slot 14"), Tr("Slot 15")
                    };

                    ImGui::Columns(2, "SwapperCols", false);
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), Tr("SOURCE (From)"));
                    ImGui::Combo(Tr("Instance##src"), &swpSrcInst, instItems, IM_ARRAYSIZE(instItems));
                    ImGui::Combo(Tr("Slot##src"), &swpSrcSlot, slotItems, IM_ARRAYSIZE(slotItems));

                    ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), Tr("DESTINATION (To)"));
                    ImGui::Combo(Tr("Instance##dst"), &swpDstInst, instItems, IM_ARRAYSIZE(instItems));
                    ImGui::Combo(Tr("Slot##dst"), &swpDstSlot, slotItems, IM_ARRAYSIZE(slotItems));
                    ImGui::Columns(1);

                    ImGui::Spacing();
                    if (ImGui::Button(Tr("MOVE ACCOUNT"), ImVec2(200, 40))) {
                        if (swpSrcInst == swpDstInst && swpSrcSlot == swpDstSlot) {
                            AddLog(-1, Tr("Swapper Error: Source and Destination cannot be the same!"), ImVec4(1, 0, 0, 1));
                        }
                        else {
                            std::string srcFile = GetAppDataPath() + "\\Backups\\Instance_" + std::to_string(swpSrcInst) + "\\account_" + std::to_string(swpSrcSlot + 1) + ".nxrth";
                            std::string dstFolder = GetAppDataPath() + "\\Backups\\Instance_" + std::to_string(swpDstInst);
                            std::string dstFile = dstFolder + "\\account_" + std::to_string(swpDstSlot + 1) + ".nxrth";

                            if (!fs::exists(srcFile)) {
                                AddLog(-1, Tr("Swapper Error: Source slot is empty!"), ImVec4(1, 0, 0, 1));
                            }
                            else {
                                if (!fs::exists(dstFolder)) fs::create_directories(dstFolder);
                                try {
                                    fs::copy_file(srcFile, dstFile, fs::copy_options::overwrite_existing);
                                    fs::remove(srcFile);
                                    g_Bots[swpSrcInst].accounts[swpSrcSlot].hasFile = false;
                                    g_Bots[swpSrcInst].accounts[swpSrcSlot].fileName = "";
                                    g_Bots[swpDstInst].accounts[swpDstSlot].hasFile = true;
                                    g_Bots[swpDstInst].accounts[swpDstSlot].fileName = dstFile;
                                    AddLog(-1, Tr("Account successfully moved!"), ImVec4(0, 1, 0, 1));
                                }
                                catch (const std::exception& e) {
                                    AddLog(-1, std::string("Swapper Error: ") + e.what(), ImVec4(1, 0, 0, 1));
                                }
                            }
                        }
                    }

                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("CREATE NEW ACCOUNT (WIPE GAME DATA)"));
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), Tr("BEWARE! This will delete current game data. Make sure you saved the account!"));
                    ImGui::Separator(); ImGui::Spacing();

                    static int wipeInst = 0;
                    ImGui::PushItemWidth(200);
                    ImGui::Combo("##wipe", &wipeInst, instItems, IM_ARRAYSIZE(instItems));
                    ImGui::PopItemWidth();

                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.0f, 0.0f, 1.0f));
                    if (ImGui::Button(Tr("WIPE DATA & CREATE NEW"), ImVec2(250, 40))) {
                        int targetInst = wipeInst;
                        std::thread([targetInst]() {
                            AddLog(targetInst, Tr("Wiping game data to create new account..."), ImVec4(1, 0.5f, 0, 1));
                            RunAdbCommand(targetInst, "shell am force-stop com.supercell.hayday");
                            std::this_thread::sleep_for(std::chrono::milliseconds(500));
                            RunAdbCommand(targetInst, "shell rm /data/data/com.supercell.hayday/shared_prefs/storage.xml");
                            RunAdbCommand(targetInst, "shell rm /data/data/com.supercell.hayday/shared_prefs/storage_new.xml");
                            std::this_thread::sleep_for(std::chrono::milliseconds(500));
                            AddLog(targetInst, Tr("Data wiped successfully! Launch game to start fresh."), ImVec4(0, 1, 0, 1));
                            }).detach();
                    }
                    ImGui::PopStyleColor(3);
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(Tr("Auto Tom Config"))) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("AUTOMATED TOM MANAGER"));
                    ImGui::TextDisabled(Tr("Tom will be triggered every 2 hours if contract is active."));
                    ImGui::Separator(); ImGui::Spacing();

                    static int tomInst = 0, tomSlot = 0;
                    const char* instItems[] = { Tr("Instance 1"), Tr("Instance 2"), Tr("Instance 3"), Tr("Instance 4"), Tr("Instance 5"), Tr("Instance 6 (STORAGE)") };
                    const char* slotItems[] = { Tr("Slot 1"), Tr("Slot 2"), Tr("Slot 3"), Tr("Slot 4"), Tr("Slot 5") };

                    ImGui::Combo(Tr("Instance##tom"), &tomInst, instItems, IM_ARRAYSIZE(instItems));
                    ImGui::Combo(Tr("Slot##tom"), &tomSlot, slotItems, IM_ARRAYSIZE(slotItems));
                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                    AccountSlot& tAcc = g_Bots[tomInst].accounts[tomSlot];
                    ImGui::Checkbox(Tr("Enable Auto Tom For This Slot"), &tAcc.autoTomEnabled);
                    ImGui::Spacing();

                    ImGui::SetNextItemWidth(150);
                    ImGui::InputInt(Tr("Remaining Hours"), &tAcc.tomRemainingHours);
                    ImGui::TextDisabled(Tr("Enter how many hours left on Tom's contract."));

                    ImGui::Spacing();
                    ImGui::SetNextItemWidth(150);
                    const char* catItems[] = { Tr("Barn"), Tr("Silo") };
                    ImGui::Combo(Tr("Search Category"), &tAcc.tomCategory, catItems, IM_ARRAYSIZE(catItems));

                    ImGui::Spacing();
                    static char itemBuf[64] = "";
                    ImGui::SetNextItemWidth(200);
                    ImGui::InputText("##itemsearch", itemBuf, IM_ARRAYSIZE(itemBuf));
                    ImGui::SameLine();
                    if (ImGui::Button(Tr("ACCEPT ITEM NAME"), ImVec2(150, 25))) {
                        tAcc.tomItemName = itemBuf;
                        SaveConfig();
                    }

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), Tr("Current Target Item: [%s]"), tAcc.tomItemName.empty() ? "NONE" : tAcc.tomItemName.c_str());

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(Tr("Farm Inspector"))) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("LIVE FARM OVERVIEW & INVENTORY"));
                    ImGui::TextDisabled(Tr("Select an instance and slot to view real-time AI-extracted data."));
                    ImGui::Separator(); ImGui::Spacing();

                    static int infoInst = 0, infoSlot = 0;
                    const char* instItems[] = { Tr("Instance 1"), Tr("Instance 2"), Tr("Instance 3"), Tr("Instance 4"), Tr("Instance 5"), Tr("Instance 6 (STORAGE)") };
                    const char* slotItems[] = { Tr("Slot 1"), Tr("Slot 2"), Tr("Slot 3"), Tr("Slot 4"), Tr("Slot 5") };

                    ImGui::PushItemWidth(150);
                    ImGui::Combo(Tr("Instance##info"), &infoInst, instItems, IM_ARRAYSIZE(instItems));
                    ImGui::SameLine();
                    ImGui::Combo(Tr("Slot##info"), &infoSlot, slotItems, IM_ARRAYSIZE(slotItems));
                    ImGui::PopItemWidth();
                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

                    AccountSlot& viewAcc = g_Bots[infoInst].accounts[infoSlot];

                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.15f, 0.12f, 1.0f));
                    ImGui::BeginChild("ID_Card", ImVec2(0, 110), true);

                    ImGui::SetWindowFontScale(1.5f);
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[%d] %s", viewAcc.level, viewAcc.farmName.c_str());
                    ImGui::SetWindowFontScale(1.0f);

                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), Tr("Player Tag: "));
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", viewAcc.playerTag.c_str());
                    ImGui::Spacing();

                    if (icon_coin != 0) {
                        ImGui::Image((void*)(intptr_t)icon_coin, ImVec2(16.0f, 16.0f));
                        ImGui::SameLine();
                    }
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), Tr("Coins: %d"), viewAcc.coinAmount);

                    ImGui::SameLine(200);

                    if (icon_dia != 0) {
                        ImGui::Image((void*)(intptr_t)icon_dia, ImVec2(16.0f, 16.0f));
                        ImGui::SameLine();
                    }
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), Tr("Diamonds: %d"), viewAcc.diamondAmount);

                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                    ImGui::Spacing();

                    InventoryData& curr = viewAcc.currentInv;
                    InventoryData& prev = viewAcc.previousInv;

                    auto formatDiff = [](int c, int p) -> std::string {
                        int d = c - p;
                        if (d > 0) return " (+" + std::to_string(d) + ")";
                        if (d < 0) return " (" + std::to_string(d) + ")";
                        return " (+0)";
                        };

                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
                    ImGui::BeginChild("BarnDataDisplay", ImVec2(0, 0), true);
                    ImGui::Columns(3, "BarnDataCols", false);

                    ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("[ BARN EXPANSION ]"));
                    ImGui::Separator();
                    ImGui::Text(Tr("Bolt: %d %s"), curr.bolt, formatDiff(curr.bolt, prev.bolt).c_str());
                    ImGui::Text(Tr("Plank: %d %s"), curr.plank, formatDiff(curr.plank, prev.plank).c_str());
                    ImGui::Text(Tr("Tape: %d %s"), curr.tape, formatDiff(curr.tape, prev.tape).c_str());
                    ImGui::NextColumn();

                    ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("[ SILO EXPANSION ]"));
                    ImGui::Separator();
                    ImGui::Text(Tr("Nail: %d %s"), curr.nail, formatDiff(curr.nail, prev.nail).c_str());
                    ImGui::Text(Tr("Screw: %d %s"), curr.screw, formatDiff(curr.screw, prev.screw).c_str());
                    ImGui::Text(Tr("Panel: %d %s"), curr.panel, formatDiff(curr.panel, prev.panel).c_str());
                    ImGui::NextColumn();

                    ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("[ LAND EXPANSION ]"));
                    ImGui::Separator();
                    ImGui::Text(Tr("Deed: %d %s"), curr.deed, formatDiff(curr.deed, prev.deed).c_str());
                    ImGui::Text(Tr("Mallet: %d %s"), curr.mallet, formatDiff(curr.mallet, prev.mallet).c_str());
                    ImGui::Text(Tr("Marker: %d %s"), curr.marker, formatDiff(curr.marker, prev.marker).c_str());
                    ImGui::Text(Tr("Map: %d %s"), curr.map, formatDiff(curr.map, prev.map).c_str());

                    ImGui::Columns(1);
                    ImGui::EndChild();
                    ImGui::PopStyleColor();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }

        if (g_CurrentTab == 4) {
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("SYSTEM EVENT LOGS"));
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button(Tr("CLEAR LOGS"), ImVec2(100, 30))) {
                std::lock_guard<std::mutex> lock(g_LogMutex);
                g_GlobalLogs.clear();
            }
            ImGui::SameLine();
            ImGui::Checkbox(Tr("Auto-Scroll"), &g_AutoScrollLogs);

            ImGui::Spacing();
            ImGui::Text(Tr("Filter:")); ImGui::SameLine();

            auto FilterButton = [&](const char* label, int id) {
                bool isSelected = (g_LogFilter == id);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.65f, 0.12f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                }
                if (ImGui::Button(label, ImVec2(60, 25))) g_LogFilter = id;
                if (isSelected) ImGui::PopStyleColor(2);
                ImGui::SameLine();
                };

            FilterButton(Tr("ALL"), -1);
            FilterButton(Tr("INST #1"), 0);
            FilterButton(Tr("INST #2"), 1);
            FilterButton(Tr("INST #3"), 2);
            FilterButton(Tr("INST #4"), 3);
            FilterButton(Tr("INST #5"), 4); 
            FilterButton(Tr("INST #6"), 5);
            ImGui::Separator();

            ImGui::BeginChild("LogRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            {
                std::lock_guard<std::mutex> lock(g_LogMutex);
                for (const auto& log : g_GlobalLogs) {
                    if (g_LogFilter == -1 || g_LogFilter == log.instanceId) {
                        std::string prefix = (log.instanceId == -1) ? Tr("[SYSTEM]") : std::string(Tr("[INST ")) + std::to_string(log.instanceId + 1) + "]";
                        ImGui::TextColored(log.color, "[%s] %s %s", log.timestamp.c_str(), prefix.c_str(), Tr(log.message.c_str()));
                    }
                }
                if (g_AutoScrollLogs && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();
        }

        if (g_CurrentTab == 5) {
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("TEMPLATE CONFIGURATION & MAKER"));
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::BeginTabBar("TemplateTabBar")) {
                if (ImGui::BeginTabItem(Tr("Configuration"))) {
                    ImGui::Spacing();
                    ImGui::Checkbox(Tr("Separate all templates for each instance"), &g_SeparateTemplates);
                    ImGui::TextDisabled(Tr("(Checked = Use templates_0, templates_1...)"));

                    ImGui::Spacing();
                    ImGui::BeginChild("TemplatesScroll", ImVec2(0, 0), false);
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("GAME CHECKERS"));
                    ImGui::Separator();
                    RenderTemplateRow(Tr("Mailbox"), g_mailboxPathBuf, IM_ARRAYSIZE(g_mailboxPathBuf), mailbox_templatePath, &g_Thresholds.mailboxThreshold);
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("FARMING TEMPLATES"));
                    ImGui::Separator();
                    RenderTemplateRow(Tr("Field"), g_fieldPathBuf, IM_ARRAYSIZE(g_fieldPathBuf), f_templatePath, &g_Thresholds.soilThreshold);
                    RenderTemplateRow(Tr("Wheat"), g_wheatPathBuf, IM_ARRAYSIZE(g_wheatPathBuf), w_templatePath, &g_Thresholds.wheatThreshold);
                    RenderTemplateRow(Tr("Sickle"), g_sicklePathBuf, IM_ARRAYSIZE(g_sicklePathBuf), s_templatePath, &g_Thresholds.sickleThreshold);

                    RenderTemplateRow(Tr("Corn Seed"), g_cornPathBuf, IM_ARRAYSIZE(g_cornPathBuf), c_templatePath, &g_Thresholds.cornThreshold);
                   
                    RenderTemplateRow(Tr("Carrot Seed"), g_carrotPathBuf, IM_ARRAYSIZE(g_carrotPathBuf), carrot_templatePath, &g_Thresholds.carrotThreshold);
                   
                    RenderTemplateRow(Tr("Carrot Shop"), g_carrotShopPathBuf, IM_ARRAYSIZE(g_carrotShopPathBuf), carrot_shop_templatePath, &g_Thresholds.shopThreshold);
                    RenderTemplateRow(Tr("Soybean Seed"), g_soybeanPathBuf, IM_ARRAYSIZE(g_soybeanPathBuf), soybean_templatePath, &g_Thresholds.soybeanThreshold);
                    
                    RenderTemplateRow(Tr("Soybean Shop"), g_soybeanShopPathBuf, IM_ARRAYSIZE(g_soybeanShopPathBuf), soybean_shop_templatePath, &g_Thresholds.shopThreshold);
                    RenderTemplateRow(Tr("Sugarcane Seed"), g_sugarcanePathBuf, IM_ARRAYSIZE(g_sugarcanePathBuf), sugarcane_templatePath, &g_Thresholds.sugarcaneThreshold);
                   
                    RenderTemplateRow(Tr("Sugarcane Shop"), g_sugarcaneShopPathBuf, IM_ARRAYSIZE(g_sugarcaneShopPathBuf), sugarcane_shop_templatePath, &g_Thresholds.shopThreshold);
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("SHOP TEMPLATES"));
                    ImGui::Separator();
                    RenderTemplateRow(Tr("Shop"), g_shopPathBuf, IM_ARRAYSIZE(g_shopPathBuf), shop_templatePath, &g_Thresholds.shopThreshold);
                    RenderTemplateRow(Tr("Wheat Shop"), g_wheatshopPathBuf, IM_ARRAYSIZE(g_wheatshopPathBuf), wheatshop_templatePath, &g_Thresholds.shopThreshold);
                    RenderTemplateRow(Tr("Corn Shop"), g_cornShopPathBuf, IM_ARRAYSIZE(g_cornShopPathBuf), cornshop_templatePath, &g_Thresholds.shopThreshold);
                    RenderTemplateRow(Tr("Crate"), g_cratePathBuf, IM_ARRAYSIZE(g_cratePathBuf), crate_templatePath, &g_Thresholds.crateThreshold);
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("UI TEMPLATES"));
                    ImGui::Separator();
                    RenderTemplateRow(Tr("Cross"), g_crossPathBuf, IM_ARRAYSIZE(g_crossPathBuf), cross_templatePath, &g_Thresholds.crossThreshold);
                    RenderTemplateRow(Tr("Create Sale"), g_createSalePathBuf, IM_ARRAYSIZE(g_createSalePathBuf), create_sale_templatePath, &g_Thresholds.createSaleThreshold);
                    RenderTemplateRow(Tr("Level Up"), g_levelupPathBuf, IM_ARRAYSIZE(g_levelupPathBuf), levelup_templatePath, &g_Thresholds.levelUpThreshold);
                    RenderTemplateRow(Tr("Level Up Cont."), g_levelupContinuePathBuf, IM_ARRAYSIZE(g_levelupContinuePathBuf), levelup_continue_templatePath, &g_Thresholds.levelUpThreshold);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(Tr("Template Maker"))) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("CREATE YOUR OWN TEMPLATES"));
                    ImGui::TextDisabled(Tr("If the bot cannot find objects, use this tool to capture a fresh screen."));
                    ImGui::TextDisabled(Tr("Open the saved screenshot in Paint, crop the item, and overwrite it in /templates."));
                    ImGui::Separator(); ImGui::Spacing();

                    static int tmplInst = 0;
                    ImGui::SetNextItemWidth(200);
                    const char* instItems[] = { Tr("Instance 1"), Tr("Instance 2"), Tr("Instance 3"), Tr("Instance 4"), Tr("Instance 5"), Tr("Instance 6") };
                    ImGui::Combo(Tr("Select Emulator to Capture##tmpl"), &tmplInst, instItems, IM_ARRAYSIZE(instItems));

                    static char tmplStatus[128] = "Ready.";
                    static ImVec4 tmplColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

                    ImGui::Spacing(); ImGui::Spacing();

                    if (ImGui::Button(Tr("TAKE SCREENSHOT (Save to /templates)"), ImVec2(320, 50))) {
                        strcpy(tmplStatus, Tr("Capturing..."));
                        tmplColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);

                        std::thread([]() {
                            int inst = tmplInst;
                            std::string currentAdb = GetUniversalAdbPath(inst);
                            cv::Mat rawScreen = CaptureInstanceScreen(inst, currentAdb, g_Bots[inst].adbSerial);

                            if (rawScreen.empty()) {
                                strcpy(tmplStatus, Tr("Error: Could not capture screen!"));
                                tmplColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                                AddLog(inst, Tr("Screenshot failed: Empty frame."), ImVec4(1, 0, 0, 1));
                                return;
                            }

                            time_t now = time(0);
                            struct tm tstruct;
                            char timeBuf[80];
                            localtime_s(&tstruct, &now);
                            strftime(timeBuf, sizeof(timeBuf), "%H%M%S", &tstruct);

                            if (!fs::exists("templates")) fs::create_directories("templates");
                            std::string filename = "screenshot_inst" + std::to_string(inst + 1) + "_" + std::string(timeBuf) + ".png";
                            std::string fullPath = "templates\\" + filename;

                            try {
                                if (cv::imwrite(fullPath, rawScreen)) {
                                    std::string msg = std::string(Tr("Saved: ")) + fullPath;
                                    strncpy(tmplStatus, msg.c_str(), sizeof(tmplStatus) - 1);
                                    tmplColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                                    AddLog(inst, std::string(Tr("Screenshot saved to: ")) + fullPath, ImVec4(0, 1, 0, 1));
                                }
                                else {
                                    strcpy(tmplStatus, Tr("Error: Write failed (Permissions?)"));
                                    tmplColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                                }
                            }
                            catch (...) {
                                strcpy(tmplStatus, Tr("Exception: File write error."));
                                tmplColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                            }
                            }).detach();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button(Tr("Watch Tutorial"), ImVec2(150, 50))) {
                        ShellExecuteA(0, "open", "https://youtu.be/2gfU65gIaEE", 0, 0, SW_SHOW);
                    }

                    ImGui::Spacing(); ImGui::Spacing();
                    ImGui::TextColored(tmplColor, Tr("Status: %s"), Tr(tmplStatus));

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }

        if (g_CurrentTab == 2) {
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("WEBHOOK & NOTIFICATIONS"));
            ImGui::Separator(); ImGui::Spacing();

            ImGui::BeginChild("NotificationConfig", ImVec2(0, 160), true);
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("BARN & SILO NOTIFICATIONS"));
            ImGui::Separator(); ImGui::Spacing();
            ImGui::Checkbox(Tr("Enable Webhook Notification"), &g_EnableBarnWebhook);
            if (g_EnableBarnWebhook) {
                ImGui::Spacing();
                ImGui::Text(Tr("Webhook URL:"));
                ImGui::PushItemWidth(-1);
                ImGui::InputText("##webhook", g_WebhookURL, IM_ARRAYSIZE(g_WebhookURL));
                ImGui::PopItemWidth();
                ImGui::Spacing();
                ImGui::Checkbox(Tr("Send Screenshot Image with Webhook"), &g_EnableWebhookImage);
                ImGui::TextDisabled(Tr("If disabled, only the text report will be sent (Faster)."));
            }
            ImGui::EndChild();
			ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("This tab used to have remote commands, which helped users to control their bot from away. since i drop this project, i deleted that part.");
        }

        if (g_CurrentTab == 3) {
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("GLOBAL APPLICATION SETTINGS"));
            ImGui::Separator(); ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button(Tr("SAVE ALL SETTINGS"), ImVec2(-1, 35))) {
                SaveConfig();
                AddLog(-1, "Settings saved to nxrth_config.ini", ImVec4(0, 1, 0, 1));
            }
            ImGui::PopStyleColor();
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            ImGui::Text(Tr("ADB Executable Path:"));
            float inputWidth = ImGui::GetContentRegionAvail().x - 130;
            ImGui::PushItemWidth(inputWidth);
            if (!g_AdbValid) ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));
            ImGui::InputText("##adbpath", g_AdbPathBuf, IM_ARRAYSIZE(g_AdbPathBuf));
            if (!g_AdbValid) ImGui::PopStyleColor();
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button(Tr("Browse"), ImVec2(80, 0))) {
                std::string p = OpenFileDialog("Executable\0*.exe\0All Files\0*.*\0");
                if (!p.empty()) strncpy(g_AdbPathBuf, p.c_str(), 260);
            }
            if (!g_AdbValid && icon_warning != 0) {
                ImGui::SameLine();
                ImGui::Image((void*)(intptr_t)icon_warning, ImVec2(20, 20));
                if (ImGui::IsItemHovered()) ImGui::SetTooltip(Tr("File not found!"));
            }

            ImGui::Spacing();

            ImGui::Text(Tr("Emulator Console Path:"));
            ImGui::PushItemWidth(inputWidth);
            if (!g_MEmuValid) ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));
            ImGui::InputText("##memupath", g_MEmuPathBuf, IM_ARRAYSIZE(g_MEmuPathBuf));
            if (!g_MEmuValid) ImGui::PopStyleColor();
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button(Tr("Browse##memu"), ImVec2(80, 0))) {
                std::string p = OpenFileDialog("Executable\0*.exe\0All Files\0*.*\0");
                if (!p.empty()) strncpy(g_MEmuPathBuf, p.c_str(), 260);
            }
            if (!g_MEmuValid && icon_warning != 0) {
                ImGui::SameLine();
                ImGui::Image((void*)(intptr_t)icon_warning, ImVec2(20, 20));
                if (ImGui::IsItemHovered()) ImGui::SetTooltip(Tr("File not found!"));
            }

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
           

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("TIMING & DELAY SETTINGS (Advanced)"));
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled(Tr("Adjust these values if your emulator is lagging or running too fast."));
            ImGui::SliderInt(Tr("Game Load Wait (Seconds)"), &g_Intervals.gameLoadWait, 5, 45);
            ImGui::SliderInt(Tr("Harvest Cooldown (ms)"), &g_Intervals.afterHarvestWait, 500, 5000);
            ImGui::SliderInt(Tr("Planting Cooldown (ms)"), &g_Intervals.afterPlantWait, 500, 5000);
            ImGui::SliderInt(Tr("Shop Open Wait (ms)"), &g_Intervals.shopEnterWait, 500, 5000);
            ImGui::SliderInt(Tr("Next Account Wait (ms)"), &g_Intervals.nextAccountWait, 500, 5000);
            ImGui::Spacing();
            ImGui::TextDisabled(Tr("Shop Automation Speeds:"));
            ImGui::SliderInt(Tr("Crate Menu Wait (ms)"), &g_Intervals.crateClickWait, 300, 3000);
            ImGui::SliderInt(Tr("Coin Collect Wait (ms)"), &g_Intervals.coinCollectWait, 200, 2000);
            ImGui::SliderInt(Tr("Product Select Wait (ms)"), &g_Intervals.productSelectWait, 200, 2000);
            ImGui::SliderInt(Tr("Create Sale Wait (ms)"), &g_Intervals.createSaleWait, 300, 3000);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::Checkbox(Tr("Enable Discord Rich Presence"), &g_EnableDiscordRPC);
            ImGui::TextDisabled(Tr("(Show your bot status on Discord profile)"));
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("ABOUT"));
            ImGui::Separator();
            ImGui::Text(Tr("Made by North"));
            ImGui::Spacing();
            ImGui::TextWrapped(Tr("Special thanks to: Nuron, Dext3r, K T and HugoAyaz for their contributions."));
        }
        if (g_CurrentTab == 6) {
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("NXRTH BOT - USER MANUAL"));
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextWrapped(Tr("Want to ask something? ping me @Nxr in server. i will reply asap."));
            ImGui::Spacing(); ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.12f, 1.00f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
            ImGui::BeginChild("HowToUseArea", ImVec2(0, 0), true, 0);

            // 1. INITIAL SETUP
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            if (ImGui::CollapsingHeader(Tr("1. Initial Setup & Minitouch"), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();
                ImGui::TextWrapped(Tr("Before starting, you must click Inject Important Files. Without this you won't be able to use the bot. it injects Minitouch, Zoom, Font, Field color changer all in one."));
                ImGui::Unindent();
                ImGui::Spacing();
            }

            // 2. BOT MANAGER
            if (ImGui::CollapsingHeader(Tr("2. Bot Manager & Accounts"))) {
                ImGui::Indent();
                ImGui::TextWrapped(Tr("Modes: Single Account Mode: It just plants, harvests, sells and repeats. Multi Account Mode: Bot Plants, Harvests, sells, changes account, repeats. After all accounts done, return to first account and so on."));
                ImGui::Spacing();
                ImGui::TextWrapped(Tr("To enable Multi account mode, you must Save at least 2 accounts and enable in bot config tab."));
                ImGui::Spacing();
                ImGui::TextWrapped(Tr("How to save account : Create New account in Account management tab, After skipping tutorials and getting to farm level 7, Press \"Save Slot Data\"."));
                ImGui::TextWrapped(Tr("If you want to change accounts manually you can press \"Load Slot Data\". Dont press this if you haven't saved account yet."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), Tr("IMPORTANT NOTE: DO NOT SAVE SUPERCELL ID ACCOUNTS BECAUSE YOU WILL GET COOKIES POP-UP AND BOT WONT WORK."));
                ImGui::Unindent();
                ImGui::Spacing();
            }

            // 3. AUTO TOM
            if (ImGui::CollapsingHeader(Tr("3. Auto Tom"))) {
                ImGui::Indent();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), Tr("Read These Carefully or your bot can break."));
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), Tr("WARNING!!! I Quitted developing this bot project, and auto tom was not done. IT MAY NOT WORK PERFECTLY."));
                ImGui::TextWrapped(Tr("To use Auto Tom, make sure Tom is not on Cooldown."));
                ImGui::TextWrapped(Tr("if bot can't find tom's crate, then make your own template."));
                ImGui::Unindent();
                ImGui::Spacing();
            }

            // 4. AUTO TRANSFER BEM/SEM
            if (ImGui::CollapsingHeader(Tr("4. Auto Transfer Bem/Sem"))) {
                ImGui::Indent();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), Tr("WARNING!!! I Quitted developing this bot project, and auto transfer bem/sem was not done. IT MAY NOT WORK PERFECTLY."));
                ImGui::TextWrapped(Tr("You have to enable instance 6 and put an account there. it will Work as storage account there."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), Tr("DONT ADD FRIEND YOUR BOTS MANUALLY. yes, you heard it. IF you already added your bots, please remove them. my bot will automatically add friend."));
                ImGui::Spacing();
                ImGui::TextWrapped(Tr("TEST IF FARM NAME READING FUNCTION WORKS PROPERLY. IF BOTS ADDED EACH OTHER AS FRIENDS WITHOUT PROBLEM, THAT MEANS EVERYTHING IS FINE. IF NOT, CHANGE YOUR FARM NAME TO SOMETHING READABLE (IF BOT READED YOUR FARM NAME WRONG, PLEASE OPEN NXRTH_CONFIG.INI AND DELETE FARM NAME INFO)."));
                ImGui::Unindent();
                ImGui::Spacing();
            }

            // 5. REMOTE & WEBHOOK
            if (ImGui::CollapsingHeader(Tr("5. Remote & Webhook"))) {
                ImGui::Indent();
                ImGui::TextWrapped(Tr("You can always configure Remote & Webhook feature in Remote & Webhook Tab."));
                ImGui::TextWrapped(Tr("Type !id in my server to get your Discord id. To use Remote Control in Discord, You have to enter your discord id and Press \"Save Settings\" in Settings Tab."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("Current remote controls are:"));
                ImGui::TextDisabled(Tr("[Note: <instanceid> is 1,2,3,4,5,6. For example 1 returns screenshot of first Memu.]"));
                ImGui::BulletText(Tr("!status returns status of the bots to the Webhook."));
                ImGui::BulletText(Tr("!start <instanceid> (starts memu + hayday + bot. if both memu and hayday already open its ok.)"));
                ImGui::BulletText(Tr("!ss <instanceid> (sends screenshot of the instance to the webhook address.)"));
                ImGui::BulletText(Tr("!ssall sends screenshots of the active instances at the same time."));
                ImGui::BulletText(Tr("!stop <instanceid> stops the bot."));
                ImGui::BulletText(Tr("!stopall This is emergency command. Stops all bots at once."));
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), Tr("For Telegram:"));
                ImGui::TextWrapped(Tr("Create your own telegram bot. You can search on Google for this."));
                ImGui::TextWrapped(Tr("Enter your telegram bot token and chat id"));
                ImGui::TextWrapped(Tr("Press Save settings in Settings tab."));
                ImGui::Spacing();
                ImGui::TextWrapped(Tr("Enabling Webhook only sends status of the barn after each sale cycle."));
                ImGui::TextWrapped(Tr("My number reading can make mistakes, you better enable send screenshot with webhook if you really care."));
                ImGui::Unindent();
                ImGui::Spacing();
            }

            ImGui::PopStyleColor(); 
            ImGui::EndChild();
            ImGui::PopStyleVar();   
            ImGui::PopStyleColor(); 
        }
        if (g_CurrentTab == 7) {
            ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.12f, 1.0f), Tr("BOT VISION"));
            ImGui::TextDisabled(Tr("See the world through the bot's eyes."));
            ImGui::Separator(); ImGui::Spacing();

            
            const char* instItems[] = { "Instance 1", "Instance 2", "Instance 3", "Instance 4", "Instance 5", "Instance 6" };
            ImGui::SetNextItemWidth(200);
            ImGui::Combo(Tr("Target Instance"), &g_VisionSelectedInst, instItems, IM_ARRAYSIZE(instItems));
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            if (ImGui::Button(Tr("TAKE SNAPSHOT (Single Scan)"), ImVec2(250, 40))) {
                g_VisionLiveRunning = false; 
                std::thread([]() {
                    int inst = g_VisionSelectedInst;
                    std::string currentAdb = GetUniversalAdbPath(inst);
                    cv::Mat raw = CaptureInstanceScreen(inst, currentAdb, g_Bots[inst].adbSerial);
                    if (!raw.empty()) {
                        cv::Mat processed = ProcessVisionFrame(inst, raw);
                        std::lock_guard<std::mutex> lock(g_VisionMutex);
                        g_VisionMat = processed; 
                    }
                    }).detach();
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();

            if (g_VisionLiveRunning) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button(Tr("STOP LIVE DETECTION"), ImVec2(250, 40))) {
                    g_VisionLiveRunning = false;
                }
                ImGui::PopStyleColor();
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
                if (ImGui::Button(Tr("START LIVE DETECTION (1 FPS)"), ImVec2(250, 40))) {
                    g_VisionLiveRunning = true;
                    std::thread([]() {
                        while (g_VisionLiveRunning) {
                            int inst = g_VisionSelectedInst;
                            std::string currentAdb = GetUniversalAdbPath(inst);
                            cv::Mat raw = CaptureInstanceScreen(inst, currentAdb, g_Bots[inst].adbSerial);
                            if (!raw.empty()) {
                                cv::Mat processed = ProcessVisionFrame(inst, raw);
                                std::lock_guard<std::mutex> lock(g_VisionMutex);
                                g_VisionMat = processed;
                            }
                           
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                        }
                        }).detach();
                }
                ImGui::PopStyleColor();
            }

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

          
            {
                std::lock_guard<std::mutex> lock(g_VisionMutex);
                if (!g_VisionMat.empty()) {
                    UpdateVisionTexture(g_VisionMat);
                    g_VisionMat.release(); 
                }
            }

            
            if (g_VisionTexture != 0) {
                
                float availWidth = ImGui::GetContentRegionAvail().x;
                float aspect = (float)g_VisionTexHeight / (float)g_VisionTexWidth;
                float renderWidth = availWidth * 0.9f;
                float renderHeight = renderWidth * aspect;

                ImGui::SetCursorPosX((availWidth - renderWidth) * 0.5f); 
                ImGui::Image((void*)(intptr_t)g_VisionTexture, ImVec2(renderWidth, renderHeight));
            }
            else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), Tr("No vision data available. Click a button above to scan."));
            }
        }


}
    ImGui::EndChild();
    ImGui::End();
}

int main() {
    SetUnhandledExceptionFilter(NxrthCrashHandler);
    if (!glfwInit()) return 1;
    Discord::Initialize();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1350, 820, "NXRTH Premium", NULL, NULL);

    if (!window) return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImFontConfig font_cfg;
	static const ImWchar global_ranges[] = { // ALPHABET EXTENSIONS
		0x0020, 0x00FF, // BASIC LATIN + LATIN SUPPLEMENT
		0x0100, 0x017F, // LATIN EXTENSION-A
		0x0400, 0x04FF, // KYRIL ALPHABET
        0,
    };
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Tahoma.ttf", 16.f, &font_cfg, global_ranges);

    SetPremiumTheme();
    LoadConfig();
    if (g_Language == -1) {
        AutoDetectLanguage(); 
        SaveConfig();
    }
    InitializeBots();
    LoadInventoryData();
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::string currentExeDir = std::string(exePathBuf).substr(0, std::string(exePathBuf).find_last_of("\\/"));

    int w, h;
    LoadTextureFromFile((currentExeDir + "\\icons\\chart-bar-regular.png").c_str(), &icon_dashboard, &w, &h);
    LoadTextureFromFile((currentExeDir + "\\icons\\robot-solid.png").c_str(), &icon_config, &w, &h);
    LoadTextureFromFile((currentExeDir + "\\icons\\message-solid.png").c_str(), &icon_logs, &w, &h);
    LoadTextureFromFile((currentExeDir + "\\icons\\images-solid.png").c_str(), &icon_templates, &w, &h);
    LoadTextureFromFile((currentExeDir + "\\icons\\globe-solid.png").c_str(), &icon_cloud, &w, &h);
    LoadTextureFromFile((currentExeDir + "\\icons\\gears-solid.png").c_str(), &icon_settings, &w, &h);
    LoadTextureFromFile((currentExeDir + "\\icons\\triangle-exclamation-solid.png").c_str(), &icon_warning, &w, &h);
    LoadTextureFromFile((currentExeDir + "\\icons\\coin.png").c_str(), &icon_coin, &w, &h);
    LoadTextureFromFile((currentExeDir + "\\icons\\diamond.png").c_str(), &icon_dia, &w, &h);
    LoadTextureFromFile((currentExeDir +  "\\icons\\question-solid.png").c_str(), &icon_question, &w, &h);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    while (!glfwWindowShouldClose(window)) {
        if (g_EnableDiscordRPC) {
            Discord::Update(true);
        }
        else {
            Discord::Update(false);
        }

        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 30.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));

        ImGui::Begin("NXRTH_TITLE_BAR", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
        RenderCustomTitleBar(window);
        ImGui::End();

        ImGui::PopStyleVar(2);

        RenderApp();

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.05f, 0.05f, 0.05f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    ExitProcess(0);
}
