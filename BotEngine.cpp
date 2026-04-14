#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include "BotEngine.h"
#include "Language.h"
#include "Discord.h"
#include "tesseract_ocr.h"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include "imgui.h"
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <windows.h>
#include <stdio.h>
#include <sstream>
#include <vector>


// GLOBAL VARIABLES
namespace fs = std::filesystem;
std::string g_StorageTag = ""; 
std::string kLDPlayerAdbPath = "C:\\LDPlayer\\LDPlayer9\\adb.exe";
std::string kLDConsolePath = "C:\\LDPlayer\\LDPlayer9\\ldconsole.exe";


extern TemplateThresholds g_Thresholds;
extern IntervalSettings g_Intervals;
extern BotInstance g_Bots[6];

extern std::string kAdbPath;
extern std::string kMEmuConsolePath;
extern std::string GetAppDataPath();
extern bool g_EnableBarnWebhook;

extern bool g_EnableWebhookImage;

// --- TEMPLATE PATHS ---
extern std::string f_templatePath; extern std::string w_templatePath; extern std::string s_templatePath;
extern std::string g_templatePath; extern std::string shop_templatePath; extern std::string wheatshop_templatePath;
extern std::string soldcrate_templatePath; extern std::string crate_templatePath; extern std::string arrows_templatePath;
extern std::string plus_templatePath; extern std::string cross_templatePath; extern std::string advertise_templatePath;
extern std::string create_sale_templatePath; extern std::string c_templatePath; extern std::string gc_templatePath;
extern std::string cornshop_templatePath; extern std::string barn_market_templatePath; extern std::string silo_market_templatePath;
extern std::string mailbox_templatePath; extern std::string crate_wheat_templatePath; extern std::string crate_corn_templatePath;
extern std::string levelup_templatePath; extern std::string levelup_continue_templatePath; extern std::string carrot_templatePath;
extern std::string grown_carrot_templatePath; extern std::string carrot_shop_templatePath; extern std::string soybean_templatePath;
extern std::string grown_soybean_templatePath; extern std::string soybean_shop_templatePath; extern std::string sugarcane_templatePath;
extern std::string grown_sugarcane_templatePath; extern std::string sugarcane_shop_templatePath; extern std::string silo_full_templatePath;
extern std::string crate_carrot_templatePath; extern std::string crate_soybean_templatePath; extern std::string crate_sugarcane_templatePath;
extern std::string silo_full_cross_templatePath; extern std::string market_close_crosstemplatePath; extern std::string market_close_crosstemplatePath;


// --- MUTUAL FUNCTIONS ---
extern void AddLog(int instanceId, std::string message, ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
extern void SaveConfig();
extern void SaveInventoryData();
extern std::string GetClipboardText();

// INJECT FOLDERS
const std::string GAME_DATA_PATH = "/data/data/com.supercell.hayday/shared_prefs/storage_new.xml";
const std::string ZOOM_DATA_PATH = "/data/data/com.supercell.hayday/update/data/game_config.csv";

// TRANSFER PART (NOT DONE )
int g_TransferThreshold = 10;
TransferRequest g_TransferRequest;


// ==============================================================================
std::string GetUniversalAdbPath(int instanceId) {
    if (g_Bots[instanceId].emulatorType == 1) return kLDPlayerAdbPath;
    // IF YOU WANT TO ADD BLUESTACKS SUPPORT, ADD else if (type == 2) HERE. I QUITTED THE PROJECT BEFORE ADDING BLUESTACKS.
    return kAdbPath; // MEMU AS DEFAULT
}
// ==============================================================================
// RUN CMD IN BACKGROUND BECAUSE WITHOUT THIS , THE CMD WINDOW WILL POP UP (FLICKER, OPEN AND CLOSE IMMEDIATELY) WHICH IS ANNOYING. THIS FUNCTION HIDES THE CMD WINDOW COMPLETELY.
bool RunCmdHidden(const std::string& command) {
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

    DWORD waitResult = WaitForSingleObject(pi.hProcess, 10000);
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
// GET ADB OUTPUT AND RETURN OUTPUT AS STRING TO READ STUFF LIKE INPUT DEVICE EVENT ETC.
std::string GetAdbOutput(int instanceId, std::string args) {
    std::string serial = g_Bots[instanceId].adbSerial; // EXAMPLE: 127.0.0.1:21503
    std::string cmd = "cmd.exe /c \"\"" + kAdbPath + "\" -s " + serial + " " + args + "\"";

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 10000);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    CloseHandle(hWrite);

    std::string result = "";
    DWORD read;
    char buffer[256];
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, NULL) && read != 0) {
        buffer[read] = '\0';
        result += buffer;
    }
    CloseHandle(hRead);
    return result;
}
// THIS FUNCTION HELPS TO MAKE SURE USER IS USING 640X480 100 DPI RESOLUTION. IF NOT, BOT WON'T START.
std::string GetMEmuConfig(int instanceId, std::string key) {
	// FIND MEMUC.EXE BASED ON THE ADB PATH (IN CASE USER MOVED MEmu FOLDER OR RENAMED IT, WE CAN STILL FIND MEMUC.EXE) BECAUSE MEMUC EXE IS IN THE SAME FOLDER AS ADB.EXE
    std::string memucPath = kAdbPath;
    size_t lastSlash = memucPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        memucPath = memucPath.substr(0, lastSlash + 1) + "memuc.exe";
        // EXAMPLE "D:\MEmu\adb.exe" -> "D:\MEmu\" + "memuc.exe"
    }

    // 2. PREPARE THE COMMAND
    std::string cmd = "cmd.exe /c \"\"" + memucPath + "\" getconfig -i " + std::to_string(instanceId) + " " + key + "\"";

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE; // CMD DOESNT POPS UP

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 3000); //WAIT UP TO 3 SECONDS
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    CloseHandle(hWrite);

    DWORD read;
    char buffer[128];
    ZeroMemory(buffer, sizeof(buffer));
    ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, NULL);
    CloseHandle(hRead);

    std::string rawResult = buffer;
    std::string cleanResult = "";

    // CHECK RETURNED STRING AND EXTRACT NECESSARY PART (WE NEED DIGITS ONLY)
    for (char c : rawResult) {
        if (isdigit(c)) { 
            cleanResult += c; 
        }
    }
    // FOR EXAMPLE: IF RETURNED STRING IS " 640\r\n", cleanResult WILL TURN TO "640".

    return cleanResult;
}


// ADB COMMAND RUNNER
void RunAdbCommand(int instanceId, std::string args) {
    std::string serial = g_Bots[instanceId].adbSerial;

    std::string cmd = "cmd.exe /c \"\"" + kAdbPath + "\" -s " + serial + " " + args + "\"";
    RunCmdHidden(cmd);
}
// TAP FUNCTION TO TAP ON THE SCREEN
void AdbTap(int instanceId, int x, int y) {
    RunAdbCommand(instanceId, "shell input tap " + std::to_string(x) + " " + std::to_string(y));
}

// FUNCTION USED IN AUTO ITEM TRANSFER, THIS FUNCTION CLOSES ALL THE MENUS BY SEARCHING CROSS AND TAP ON IT OVER AND OVER UNTIL THERES NO CROSS.
void ForceCloseAllMenus(int instanceId) {
    AddLog(instanceId, "Closing all menus to return to main screen...", ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
    for (int i = 0; i < 3; i++) {
        cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, g_Bots[instanceId].adbSerial);
        MatchResult crossRes = FindImage(screen, cross_templatePath, g_Thresholds.crossThreshold, false);
        if (crossRes.found) {
            AdbTap(instanceId, crossRes.x, crossRes.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
        }
        else {
			break; // IF THERES NO CROSS FOUND, ASSUME WE ARE BACK TO THE FARM AND STOP
        }
    }
}

// TEMPLATE TEST BUTTONS USED IN GUI TO HELP USERS IF THEIR TEMPLATES ARE WORKING PROPERLY OR NOT
void PerformTemplateTest(int instanceId, std::string templatePath, std::string testName, float threshold, bool useGrayscale) {
    AddLog(instanceId, std::string(Tr("Running ")) + Tr(testName.c_str()) + "...", ImVec4(1, 1, 0, 1));
    std::thread([instanceId, templatePath, testName, threshold, useGrayscale]() {
        cv::Mat frame = CaptureInstanceScreen(instanceId, kAdbPath, g_Bots[instanceId].adbSerial);
        MatchResult res = FindImage(frame, templatePath, threshold, useGrayscale);

        if (res.found) {
            AddLog(instanceId, Tr(testName.c_str()) + std::string(Tr(" FOUND! Score: ")) + std::to_string((int)(res.score * 100)) + "% at " + std::to_string(res.x) + "," + std::to_string(res.y), ImVec4(0, 1, 0, 1));
            AdbTap(instanceId, res.x, res.y);
        }
        else {
            AddLog(instanceId, Tr(testName.c_str()) + std::string(Tr(" NOT Found.")), ImVec4(1, 0.5f, 0, 1));
        }
        }).detach();
}

// ANOTHER IMPORTANT FUNCTION. HELPS US TO USE TWO FINGER SWIPE. MINITOUCH IS A TOOL THAT CREATES A VIRTUAL TOUCHSCREEN DEVICE ON THE EMULATOR AND LETS US CONTROL IT WITH ADB COMMANDS.
void StartMinitouchStealth(int instanceId) {
    int minitouchPort = 1111 + instanceId;
    RunAdbCommand(instanceId, "forward tcp:" + std::to_string(minitouchPort) + " localabstract:minitouch");

    std::string serial = g_Bots[instanceId].adbSerial;
    std::string cmd = "cmd.exe /c \"\"" + kAdbPath + "\" -s " + serial + " shell /data/local/tmp/minitouch\"";
    WinExec(cmd.c_str(), SW_HIDE);
}

typedef BOOL(WINAPI* IsHungAppWindowProc)(HWND);
// WATCHDOG FOR THE EMULATOR OR GAME CRASH
void EmulatorCrashWatchdog(int instanceId) {
    BotInstance& bot = g_Bots[instanceId];
    if (!bot.isRunning) return;

 
    static std::chrono::steady_clock::time_point hungStart[6];
    static bool isHungState[6] = { false };
    static int pidFailCount[6] = { 0 };

    // =====================================================================
	// 1. WINDOWS "NOT RESPONDING" WHITE SCREEN ERROR CHECK (EMULATOR CRASHED OR FROZE)
    // =====================================================================
    HWND hwnd = FindWindowA(NULL, bot.vmName);
    if (hwnd != NULL) {
        HMODULE hUser32 = GetModuleHandleA("user32.dll");
        if (hUser32) {
            IsHungAppWindowProc IsHung = (IsHungAppWindowProc)GetProcAddress(hUser32, "IsHungAppWindow");
            if (IsHung && IsHung(hwnd)) {
                if (!isHungState[instanceId]) {
                    
                    isHungState[instanceId] = true;
                    hungStart[instanceId] = std::chrono::steady_clock::now();
                    AddLog(instanceId, "[WATCHDOG] Emulator not responding! Waiting 15s to confirm...", ImVec4(1, 0.5f, 0, 1));
                }
                else {
					// CHECK HOW LONG IT HAS BEEN IN HUNG STATE
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - hungStart[instanceId]).count();

                    if (elapsed >= 15) { // IF ITS BEEN MORE THAN 15 SEC RESTART
                        AddLog(instanceId, "[WATCHDOG] CRITICAL: Emulator DEAD for 15s! Forcing restart...", ImVec4(1, 0, 0, 1));

                        if (bot.emulatorType == 1) { // LDPlayer
                            RunCmdHidden("cmd.exe /c \"\"" + kMEmuConsolePath + "\" quit --index " + std::to_string(bot.emuIndex) + "\"");
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                            RunCmdHidden("cmd.exe /c \"\"" + kMEmuConsolePath + "\" launch --index " + std::to_string(bot.emuIndex) + "\"");
                        }
                        else { // MEmu
                            RunCmdHidden("cmd.exe /c \"\"" + kMEmuConsolePath + "\" stop " + bot.vmName + "\"");
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                            RunCmdHidden("cmd.exe /c \"\"" + kMEmuConsolePath + "\" " + bot.vmName + "\"");
                        }

						isHungState[instanceId] = false; // CLEAR MEMORY OF HUNG STATE BECAUSE WE JUST RESTARTED THE EMULATOR
                        std::this_thread::sleep_for(std::chrono::seconds(30)); // WAIT FOR REBOOT
                        return;
                    }
                }
            }
            else {
				// EMULATOR IS RESPONDING FINE, BUT CHECK IF WE WERE IN HUNG STATE BEFORE. IF YES, LOG RECOVERY.
                if (isHungState[instanceId]) {
                    AddLog(instanceId, "[WATCHDOG] Emulator recovered. False alarm cancelled.", ImVec4(0, 1, 0, 1));
                    isHungState[instanceId] = false;
                }
            }
        }
    }

    // =====================================================================
    // CHECK IF GAME CRASHED AND EMULATOR IS IN THE HOME PAGE.
    // =====================================================================
    std::string serial = bot.adbSerial;
    std::string tempFile = "C:\\Users\\Public\\pid_check_" + std::to_string(instanceId) + ".txt";
    remove(tempFile.c_str());

    std::string cmd = "cmd.exe /c \"\"" + kAdbPath + "\" -s " + serial + " shell pidof com.supercell.hayday > \"" + tempFile + "\"\"";
    RunCmdHidden(cmd);
    std::string currentAdb = kAdbPath;
    std::ifstream file(tempFile);
    std::string pidStr;
    if (file.is_open()) {
        std::getline(file, pidStr);
        file.close();
    }

    if (pidStr.empty() || pidStr.length() < 2) {
		pidFailCount[instanceId]++; // INCREMENT FAIL COUNT IF PID NOT FOUND

        if (pidFailCount[instanceId] >= 3) { // IF CANT FIND PID FOR 3 TIMES
            AddLog(instanceId, "[WATCHDOG] Hay Day crashed to desktop (3 Fails)! Relaunching...", ImVec4(1, 0.5f, 0, 1));
            std::string launchCmd = "cmd /c \"\"" + currentAdb + "\" -s " + serial + " shell monkey -p com.supercell.hayday -c android.intent.category.LAUNCHER 1\"";
            RunCmdHidden(launchCmd);

            pidFailCount[instanceId] = 0; // RESET
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        else {
            AddLog(instanceId, "[WATCHDOG] Game process missing... Warning " + std::to_string(pidFailCount[instanceId]) + "/3", ImVec4(1, 1, 0, 1));
        }
    }
    else {
		// PID IS BACK, RESET FAIL COUNT
        pidFailCount[instanceId] = 0;
    }
}
void HandleReviveHeartbeat(int instanceId) {
    BotInstance& bot = g_Bots[instanceId];
    if (!bot.useReviveMode || strlen(bot.reviveTemplatePath) < 5) return;

    AddLog(instanceId, "Revive: Checking custom anchor...", ImVec4(1, 1, 0, 1));

    cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
    MatchResult res = FindImage(screen, bot.reviveTemplatePath, 0.70f);

    if (res.found) {
        AddLog(instanceId, "Revive: System OK.", ImVec4(0, 1, 0, 1));
		bot.reviveFailCounter = 0; // SUCCESS, RESET FAIL COUNTER 
    }
    else {
        bot.reviveFailCounter++;
        AddLog(instanceId, "Revive: Anchor not found! Strike " + std::to_string(bot.reviveFailCounter) + "/3", ImVec4(1, 0.5f, 0, 1));

        if (bot.reviveFailCounter >= 3) {
            AddLog(instanceId, "CRITICAL: 3 Strikes! Restarting game engine...", ImVec4(1, 0, 0, 1));
            // REOPEN HAYDAY
            RunAdbCommand(instanceId, "shell am force-stop com.supercell.hayday");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            RunAdbCommand(instanceId, "shell monkey -p com.supercell.hayday -c android.intent.category.LAUNCHER 1");

            bot.reviveFailCounter = 0;
            std::this_thread::sleep_for(std::chrono::seconds(15)); // BOOT SLEEP
        }
    }
}

//IMPORTANT FILES INJECTOR. 
// THIS FUNCTION INJECTS(PUSHES) FILES TO THE ROOT.
void InjectImportantFiles(int instanceId) {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    std::string exeDir = std::string(buffer).substr(0, pos);

    std::string fontFile = exeDir + "\\injecthacks\\languages.csv";
    std::string zoomFile = exeDir + "\\injecthacks\\game_config.csv";
    std::string minitouchFile = exeDir + "\\injecthacks\\minitouch";

   
    std::vector<std::string> nxrthFiles = {
        "inject.nxrth", "inject2.nxrth", "inject3.nxrth", "inject4.nxrth", "inject5.nxrth"
    };

    for (const auto& file : nxrthFiles) {
        if (!fs::exists(exeDir + "\\injecthacks\\" + file)) {
            AddLog(instanceId, "Error: Missing encrypted " + file + " in injecthacks folder!", ImVec4(1, 0, 0, 1));
            return;
        }
    }
    if (!fs::exists(fontFile) || !fs::exists(zoomFile) || !fs::exists(minitouchFile)) {
        AddLog(instanceId, Tr("Error: Basic hack files (zoom/font/minitouch) missing!"), ImVec4(1, 0, 0, 1));
        return;
    }

    AddLog(instanceId, Tr("Starting NXRTH Ultimate Injection (This will take 10-15 seconds)..."), ImVec4(0.8f, 0.4f, 1.0f, 1.0f));

    std::thread([instanceId, exeDir, fontFile, zoomFile, minitouchFile]() {
		// 1. FORCE CLOSE HAY DAY TO AVOID FILE LOCKS
        AddLog(instanceId, Tr("Force stopping Hay Day..."), ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        RunAdbCommand(instanceId, "shell am force-stop com.supercell.hayday");
        std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // WAIT FOR EMULATOR JUST IN CASE

        // 2. CREATE TARGET FOLDERS
        std::string dataDir = "/data/data/com.supercell.hayday/update/data/";
        std::string scDir = "/data/data/com.supercell.hayday/update/sc/";
        RunAdbCommand(instanceId, "shell \"su -c 'mkdir -p " + dataDir + "'\"");
        RunAdbCommand(instanceId, "shell \"su -c 'mkdir -p " + scDir + "'\"");
        std::this_thread::sleep_for(std::chrono::milliseconds(800));

        // 3. FONT & LANGUAGE HACK
        AddLog(instanceId, Tr("1/4: Injecting Font & Language Hack..."), ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
        std::string tempFont = "/sdcard/temp_languages.csv";
        RunAdbCommand(instanceId, "push \"" + fontFile + "\" " + tempFont);
        RunAdbCommand(instanceId, "shell \"su -c 'cp " + tempFont + " " + dataDir + "languages.csv'\"");
        RunAdbCommand(instanceId, "shell \"su -c 'chmod 777 " + dataDir + "languages.csv'\"");
        RunAdbCommand(instanceId, "shell rm " + tempFont);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 

        // 4. ZOOM HACK (game_config.csv)
        AddLog(instanceId, Tr("2/4: Optimizing View Distance (Zoom Hack)..."), ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
        std::string tempZoom = "/sdcard/temp_config.csv";
        RunAdbCommand(instanceId, "push \"" + zoomFile + "\" " + tempZoom);
        RunAdbCommand(instanceId, "shell \"su -c 'cp " + tempZoom + " " + ZOOM_DATA_PATH + "'\"");
        RunAdbCommand(instanceId, "shell \"su -c 'chmod 777 " + ZOOM_DATA_PATH + "'\"");
        RunAdbCommand(instanceId, "shell rm " + tempZoom);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // 5. NATURE HACK (DECRYPTION ON THE FLY)
        AddLog(instanceId, Tr("3/4: Decrypting & Injecting Secure Visuals (Nature Hack)..."), ImVec4(0.8f, 0.8f, 0.2f, 1.0f));

        std::vector<std::pair<std::string, std::string>> natureMap = {
            {"inject.nxrth", "nature_new.sc"},
            {"inject2.nxrth", "nature_new_0.sctx"},
            {"inject3.nxrth", "nature_new_1.sctx"},
            {"inject4.nxrth", "nature_new_2.sctx"},
            {"inject5.nxrth", "nature_new_3.sctx"}
        };

        for (const auto& pair : natureMap) {
            std::string nxPath = exeDir + "\\injecthacks\\" + pair.first;
            std::string tempPath = exeDir + "\\injecthacks\\temp_" + pair.second;

			// 1. READ ENCRYPTED .NXRTH FILE.
            std::ifstream inFile(nxPath, std::ios::binary);
            std::string encryptedData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();

			// 2. DECRYPT .NXRTH TO RAW DATA USING THE SUPER DUPER SECRET KEY LLMFAOOOOO
            std::string rawData = DecryptXORHex(encryptedData, "NXRTH_NATURE_KEY");

			// 3. MAKE A TEMP FILE WITH DECRYPTED RAW DATA (BECAUSE WE CANT PUSH DIRECTLY FROM MEMORY, WE NEED A REAL FILE TO PUSH) AND WRITE RAW DATA TO IT.
            std::ofstream outFile(tempPath, std::ios::binary);
            outFile.write(rawData.data(), rawData.size());
            outFile.close();

            // 4. PUSH FILES
            RunAdbCommand(instanceId, "push \"" + tempPath + "\" /data/local/tmp/" + pair.second);

            // 5. DELETE TEMP FILES
            fs::remove(tempPath);

            // FILES TOO BIG SO WE KINDA WAIT JUST IN CASE.
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
        }

        // USE EMULATOR'S TEMP FOLDER
        AddLog(instanceId, "Applying decrypted assets to game engine...", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        RunAdbCommand(instanceId, "shell su -c 'cp /data/local/tmp/nature_new* " + scDir + "'");
        RunAdbCommand(instanceId, "shell su -c 'chmod 777 " + scDir + "nature_new*'");

        // DELETE STUFF IN THAT TEMP FOLDER
        RunAdbCommand(instanceId, "shell rm /data/local/tmp/nature_new*");
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        // 6. MINITOUCH PUSH
        AddLog(instanceId, Tr("4/4: Installing Minitouch Input Agent..."), ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
        RunAdbCommand(instanceId, "push \"" + minitouchFile + "\" /data/local/tmp/minitouch");
        RunAdbCommand(instanceId, "shell chmod 777 /data/local/tmp/minitouch");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        StartMinitouchStealth(instanceId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        AddLog(instanceId, Tr("ALL HACKS INJECTED SUCCESSFULLY! Start the game and Set game Language To English."), ImVec4(0, 1, 0, 1));
        }).detach();
}
// ACCOUNT SAVER
void SaveAccountToSlot(int instanceId, int slotIndex) {
    AddLog(instanceId, Tr("Saving & Encrypting Account Data..."), ImVec4(1, 1, 0, 1));

    // DOĞRUDAN SENİN EXTERN FONKSİYONUNU KULLANIYORUZ
    std::string folderPath = GetAppDataPath() + "\\Backups\\Instance_" + std::to_string(instanceId);
    if (!fs::exists(folderPath)) fs::create_directories(folderPath);

    std::string pcFileName = folderPath + "\\account_" + std::to_string(slotIndex + 1) + ".nxrth";
    std::string tempRawFile = folderPath + "\\temp_raw.xml";
    std::string tempSdFile = "/sdcard/temp_backup_" + std::to_string(instanceId) + ".xml";

    std::string copyCmd = "shell \"su -c 'cat " + GAME_DATA_PATH + " > " + tempSdFile + "'\"";
    RunAdbCommand(instanceId, copyCmd);
    std::string pullCmd = "pull " + tempSdFile + " \"" + tempRawFile + "\"";
    RunAdbCommand(instanceId, pullCmd);
    RunAdbCommand(instanceId, "shell rm " + tempSdFile);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (fs::exists(tempRawFile) && fs::file_size(tempRawFile) > 0) {
        std::ifstream inFile(tempRawFile, std::ios::binary);
        std::string rawData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();

        std::string encryptedData = EncryptXORHex(rawData, "NXRTH_LOCAL_ACCOUNT_KEY"); // SUPER DUPER SECRET KEY

        std::ofstream outFile(pcFileName, std::ios::binary);
        outFile << encryptedData;
        outFile.close();

        fs::remove(tempRawFile);

        g_Bots[instanceId].accounts[slotIndex].hasFile = true;
        g_Bots[instanceId].accounts[slotIndex].fileName = pcFileName;
        AddLog(instanceId, Tr("Account Saved & Encrypted Successfully."), ImVec4(0, 1, 0, 1));
    }
    else {
        AddLog(instanceId, Tr("Save Failed. Check Settings."), ImVec4(1, 0, 0, 1));
    }
}

// =========================================================
// DECRYPT FUNCTION FOR ACCOUNTS N OTHER STUFF USED.
// =========================================================
std::string DecryptPureXORHex(const std::string& hexStr, const std::string& key) {
    std::string text;
    if (hexStr.length() % 2 != 0) return "";
    for (size_t i = 0; i < hexStr.length(); i += 2) {
        std::string byteString = hexStr.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), NULL, 16);
        text += (byte ^ key[(i / 2) % key.length()]);
    }
    return text;
}

// =========================================================
// ACCOUNT LOADER
// =========================================================
void LoadAccountFromSlot(int instanceId, int slotIndex) {
    // DOĞRUDAN SENİN EXTERN FONKSİYONUNU KULLANIYORUZ
    std::string folderPath = GetAppDataPath() + "\\Backups\\Instance_" + std::to_string(instanceId);
    std::string pcFileName = folderPath + "\\account_" + std::to_string(slotIndex + 1) + ".nxrth";

    if (!fs::exists(pcFileName)) {
        AddLog(instanceId, Tr("Error: Slot file empty or missing!"), ImVec4(1, 0, 0, 1));
        return;
    }
    AddLog(instanceId, Tr("Decrypting & Switching Account..."), ImVec4(1, 1, 0, 1));

    std::ifstream inFile(pcFileName, std::ios::binary);
    std::string encryptedData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    // 1. DECRYPT ACCOUNT FILES ACCOUNT*.NXRTH'S.
    std::string rawData = DecryptXORHex(encryptedData, "NXRTH_LOCAL_ACCOUNT_KEY");

    if (rawData.find("<?xml") == std::string::npos) {
        rawData = DecryptPureXORHex(encryptedData, "NXRTH_LOCAL_ACCOUNT_KEY");
    }

    if (rawData.empty() || rawData.find("<?xml") == std::string::npos) {
        AddLog(instanceId, Tr("Error: File decryption failed! Corrupted data."), ImVec4(1, 0, 0, 1));
        return;
    }

    std::string tempRawFile = folderPath + "\\temp_decrypted.xml";
    std::ofstream outFile(tempRawFile, std::ios::binary);
    outFile << rawData;
    outFile.close();

    RunAdbCommand(instanceId, "shell am force-stop com.supercell.hayday");
    std::string tempSdFile = "/sdcard/temp_restore_" + std::to_string(instanceId) + ".xml";

    std::string pushCmd = "push \"" + tempRawFile + "\" " + tempSdFile;
    RunAdbCommand(instanceId, pushCmd);

    // RENAME THE ACTUAL FILE TO STORAGE_NEW.XML
    std::string moveCmd = "shell \"su -c 'cat " + tempSdFile + " > /data/data/com.supercell.hayday/shared_prefs/storage_new.xml'\"";
    RunAdbCommand(instanceId, moveCmd);
    RunAdbCommand(instanceId, "shell \"su -c 'chmod 777 /data/data/com.supercell.hayday/shared_prefs/storage_new.xml'\"");
    RunAdbCommand(instanceId, "shell rm " + tempSdFile);

    fs::remove(tempRawFile);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    RunAdbCommand(instanceId, "shell monkey -p com.supercell.hayday -c android.intent.category.LAUNCHER 1");
    AddLog(instanceId, Tr("Account Switched. Game Restarting."), ImVec4(0, 1, 0, 1));
}
// AUTO DETECT FOR TOUCH DEVICE (EVENT NUMBER) BECAUSE SOME PEOPLE CAN FORGET USING MANUALLY.
bool AutoDetectTouchDevice(int instanceId) {
    AddLog(instanceId, Tr("Detecting Input..."), ImVec4(1, 1, 0, 1));
    std::string tempFile = "C:\\Users\\Public\\devicelist_" + std::to_string(instanceId) + ".txt";
    remove(tempFile.c_str());
    std::string cmd = "cmd /c \"\"" + kAdbPath + "\" -s " + std::string(g_Bots[instanceId].adbSerial) + " shell getevent -pl > \"" + tempFile + "\"\"";
    RunCmdHidden(cmd);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::ifstream file(tempFile);
    if (!file.is_open()) {
        AddLog(instanceId, Tr("Error: Could not read list. Check ADB path."), ImVec4(1, 0, 0, 1));
        return false;
    }
    std::string line;
    std::string currentDevice = "";
    bool found = false;
    while (std::getline(file, line)) {
        if (line.find("add device") != std::string::npos && line.find("/dev/input/") != std::string::npos) {
            size_t startPos = line.find("/dev/input/");
            currentDevice = line.substr(startPos);
            currentDevice.erase(std::remove(currentDevice.begin(), currentDevice.end(), '\r'), currentDevice.end());
            currentDevice.erase(std::remove(currentDevice.begin(), currentDevice.end(), '\n'), currentDevice.end());
        }
        if (!currentDevice.empty()) {
            if (line.find("ABS_MT_POSITION_X") != std::string::npos || line.find("0035") != std::string::npos) {
                strcpy(g_Bots[instanceId].inputDevice, currentDevice.c_str());
                AddLog(instanceId, std::string(Tr("Input Found: ")) + std::string(g_Bots[instanceId].inputDevice), ImVec4(0, 1, 0, 1));
                found = true;
                break;
            }
        }
    }
    file.close();
    if (!found) {
        strcpy(g_Bots[instanceId].inputDevice, "/dev/input/event1");
        AddLog(instanceId, Tr("Failed. Retrying next time..."), ImVec4(1, 0.5f, 0, 1));
        return false;
    }
    return true; // RETURN TRUE IF SUCCESS
}

// ==============================================================================
// 
// 2-FINGER SWIPE 
// 
// ==============================================================================

void ExecuteDenseGridGesture(int instanceId, int startX, int startY, const std::vector<MatchResult>& fields) {
    if (fields.empty()) return;

    AddLog(instanceId, "[INFO] Executing Sweep...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));

    // 1. FIND FIELD'S BORDER SO THE BOT CAN DRAG IT TO THE CORNER OF THE FIELDS.
    int minX = 99999, maxX = 0, minY = 99999, maxY = 0;
    for (const auto& f : fields) {
        if (f.x < minX) minX = f.x;
        if (f.x > maxX) maxX = f.x;
        if (f.y < minY) minY = f.y;
        if (f.y > maxY) maxY = f.y;
    }


    int marginX = 80;
    int marginY = 80;

    int targetMinX = std::max(0, minX - marginX);
    int targetMaxX = std::min(1280, maxX + marginX);
    int targetMinY = std::max(0, minY - marginY);
    int targetMaxY = std::min(720, maxY + marginY);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock != INVALID_SOCKET) {
        int port = 1111 + instanceId;
        sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == 0) {

            int p1_startX = startX;
            int p1_startY = startY;
            int p2_startX = startX + 5;
            int p2_startY = startY + 5;

            // PHASE 1: DRAG IT LIKE V SHAPE
            int p1_x1 = targetMinX; int p1_y1 = targetMaxY;
            int p2_x1 = targetMaxX; int p2_y1 = targetMaxY;

            // PHASE 2: LINEAR STRAIGHT TO THE TOP
            int p1_x2 = p1_x1; int p1_y2 = targetMinY;
            int p2_x2 = p2_x1; int p2_y2 = targetMinY;

            // PHASE 3: SWIPE BACK DOWN AGAIN JUST IN CASE.
            int p1_x3 = p1_x1; int p1_y3 = targetMaxY;
            int p2_x3 = p2_x1; int p2_y3 = targetMaxY;

            // PRESS WITH 2 FINGERS ON THE SCREEN
            std::string cmd = "d 0 " + std::to_string(p1_startX) + " " + std::to_string(p1_startY) + " 50\n" +
                "d 1 " + std::to_string(p2_startX) + " " + std::to_string(p2_startY) + " 50\nc\n";
            send(sock, cmd.c_str(), cmd.length(), 0);

            // WAIT FOR SEED MENU
            std::this_thread::sleep_for(std::chrono::milliseconds(350));

            auto interpolate = [&](int x0, int y0, int x1, int y1, int tX0, int tY0, int tX1, int tY1) {
                float dist0 = std::hypot(tX0 - x0, tY0 - y0);
                float dist1 = std::hypot(tX1 - x1, tY1 - y1);
                float maxDist = std::max(dist0, dist1);

                // THE LESSER THE SPEED, THE BETTER FOR THE BOT TO MISS EMPTY FIELDS.
                int steps = std::max(20, (int)(maxDist / 5.0f));

                for (int i = 1; i <= steps; ++i) {
                    float t = (float)i / steps;
                    int cx0 = x0 + (int)((tX0 - x0) * t);
                    int cy0 = y0 + (int)((tY0 - y0) * t);
                    int cx1 = x1 + (int)((tX1 - x1) * t);
                    int cy1 = y1 + (int)((tY1 - y1) * t);

                    std::string mCmd = "m 0 " + std::to_string(cx0) + " " + std::to_string(cy0) + " 50\n" +
                        "m 1 " + std::to_string(cx1) + " " + std::to_string(cy1) + " 50\nc\n";
                    send(sock, mCmd.c_str(), mCmd.length(), 0);

                   
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
                };

            interpolate(p1_startX, p1_startY, p2_startX, p2_startY, p1_x1, p1_y1, p2_x1, p2_y1);
            interpolate(p1_x1, p1_y1, p2_x1, p2_y1, p1_x2, p1_y2, p2_x2, p2_y2);
            interpolate(p1_x2, p1_y2, p2_x2, p2_y2, p1_x3, p1_y3, p2_x3, p2_y3);


            std::string cmdUp0 = "u 0\nc\n";
            send(sock, cmdUp0.c_str(), cmdUp0.length(), 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // RELEASE FIRST FINGER

            std::string cmdUp1 = "u 1\nc\n";
			send(sock, cmdUp1.c_str(), cmdUp1.length(), 0); // RELEASE SECOND FINGER 

            // GIVE SOME TIME TO ANDROID BEFORE CLOSING TCP CONNECTION
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            AddLog(instanceId, "[SUCCESS] 2-Finger Smart Sweep complete!", ImVec4(0, 1, 0, 1));
        }
        else {
            AddLog(instanceId, "[ERROR] Minitouch connection failed!", ImVec4(1, 0, 0, 1));
        }
        closesocket(sock);
    }
    WSACleanup();
}


// FUNCTION NAME TELLS IT ALL, SCANS STUFF WITH COLOR. BECAUSE BOT CHANGING COLORS OF THE STUFF IN THE GAME
MatchResult FindGrownCropByColor(const cv::Mat& screen, int cropMode, int anchorX, int anchorY) {
    MatchResult result = { false, -1, -1, 0.0 };
    if (screen.empty()) return result;

    // ROI
    int topMargin = 70;
    int bottomMargin = 40;
    int sideMargin = 30;

    if (screen.rows <= topMargin + bottomMargin || screen.cols <= sideMargin * 2) return result;

    cv::Rect roiRect(sideMargin, topMargin, screen.cols - (sideMargin * 2), screen.rows - (topMargin + bottomMargin));
    cv::Mat searchArea = screen(roiRect);

    // 2. CHANGE COLOR SPACE
    cv::Mat hsv;
    cv::cvtColor(searchArea, hsv, cv::COLOR_BGR2HSV);

    // 3. COLOR FILTER
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
        return result;
    }

    cv::Mat mask;
    cv::inRange(hsv, lowerBound, upperBound, mask);

    // =========================================================================
	// 4. CLEAN LITTLE DOTS TO AVOID FALSE DETECTIONS AND FILL SPACES IN BIGGER AREAS.
    // =========================================================================
    // A. DELETE DUST-LIKE TRASH (LIKE DOTS WITH 1-2 PIXEL BIG) USING (MORPH_OPEN)
    cv::Mat kernelOpen = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernelOpen);

    // B. FILL THE EMPTY SPACE IN FIELDS (MORPH_CLOSE)
    cv::Mat kernelClose = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernelClose);

	// 5. FIND CONTOURS
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double bestMetric = (anchorX != -1 && anchorY != -1) ? 9999999.0 : 0.0;
    cv::Point bestCenter(-1, -1);

    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);

        // AREA THRESHOLD
        if (area > 50) {
            cv::Moments m = cv::moments(contours[i]);
            if (m.m00 != 0) {
                int localX = (int)(m.m10 / m.m00);
                int localY = (int)(m.m01 / m.m00);

                // =====================================================================
                // DONUT HOLE PROTECTION
                // =====================================================================
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
                // =====================================================================

                int realX = localX + sideMargin;
                int realY = localY + topMargin;

                if (anchorX != -1 && anchorY != -1) {

                    double dist = std::sqrt(std::pow(realX - anchorX, 2) + std::pow(realY - anchorY, 2));
                    if (dist < bestMetric) {
                        bestMetric = dist;
                        bestCenter = cv::Point(realX, realY);
                    }
                }
                else {

                    if (area > bestMetric) {
                        bestMetric = area;
                        bestCenter = cv::Point(realX, realY);
                    }
                }
            }
        }
    }

    if (bestCenter.x != -1) {
        result.found = true;
        result.score = bestMetric;
        result.x = bestCenter.x;
        result.y = bestCenter.y;
    }

    return result;
}

// ==============================================================================
// EMPTY FIELD DETECTOR (MAGENTA - HSV)
// ==============================================================================
std::vector<MatchResult> FindEmptyFieldsByColor(const cv::Mat& screen) {
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

	// Clean little pixels to avoid false detections and fill spaces in bigger areas.
    cv::Mat kernelOpen = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernelOpen);

    cv::Mat kernelClose = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernelClose);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);

        // ignore trash, fields are big
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
// sends a report of what changed in the barn items count.
void SendRotationReport(int instanceId) {
    if (!g_EnableBarnWebhook) return;
    BotInstance& bot = g_Bots[instanceId];

    std::string msg = "🔄 **" + std::string(Tr("ROTATION COMPLETE | INSTANCE ")) + std::to_string(instanceId + 1) + "** 🔄\n";
    msg += "--------------------------------------\n";

    InventoryData totalCurrent;
    InventoryData totalPrev;

    for (int i = 0; i < 15; i++) {
        if (bot.accounts[i].hasFile) {
            totalCurrent.bolt += bot.accounts[i].currentInv.bolt;
            totalPrev.bolt += bot.accounts[i].previousInv.bolt;
            totalCurrent.tape += bot.accounts[i].currentInv.tape;
            totalPrev.tape += bot.accounts[i].previousInv.tape;
            totalCurrent.plank += bot.accounts[i].currentInv.plank;
            totalPrev.plank += bot.accounts[i].previousInv.plank;
            totalCurrent.nail += bot.accounts[i].currentInv.nail;
            totalPrev.nail += bot.accounts[i].previousInv.nail;
            totalCurrent.screw += bot.accounts[i].currentInv.screw;
            totalPrev.screw += bot.accounts[i].previousInv.screw;
            totalCurrent.panel += bot.accounts[i].currentInv.panel;
            totalPrev.panel += bot.accounts[i].previousInv.panel;

            bot.accounts[i].previousInv = bot.accounts[i].currentInv;
        }
    }

    auto formatDelta = [](int current, int prev) -> std::string {
        int diff = current - prev;
        if (diff > 0) return " (+**" + std::to_string(diff) + "**)";
        if (diff < 0) return " (" + std::to_string(diff) + ")";
        return " (+0)";
        };

    msg += "💰 **" + std::string(Tr("BARN INVENTORY TOTALS")) + "**\n";
    msg += "<:Bolt:304466477527072768> Bolt: **" + std::to_string(totalCurrent.bolt) + "**" + formatDelta(totalCurrent.bolt, totalPrev.bolt) + " | ";
    msg += "<:Plank:304466477409370113> Plank: **" + std::to_string(totalCurrent.plank) + "**" + formatDelta(totalCurrent.plank, totalPrev.plank) + " | ";
    msg += "<:DuctTape:304466477564690432> Tape: **" + std::to_string(totalCurrent.tape) + "**" + formatDelta(totalCurrent.tape, totalPrev.tape) + "\n";
    msg += "<:Nail:304466477489324042> Nail: **" + std::to_string(totalCurrent.nail) + "**" + formatDelta(totalCurrent.nail, totalPrev.nail) + " | ";
    msg += "<:Screw:304466477438992394> Screw: **" + std::to_string(totalCurrent.screw) + "**" + formatDelta(totalCurrent.screw, totalPrev.screw) + " | ";
    msg += "<:WoodPanel:304466476977356802> Wood Panel: **" + std::to_string(totalCurrent.panel) + "**" + formatDelta(totalCurrent.panel, totalPrev.panel) + "\n";

    std::thread([=]() { Discord::SendWebhookMessage(msg); }).detach();
    AddLog(instanceId, Tr("Rotation Delta Report Sent!"), ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
}

// sales cycle function.
void RunSalesCycle(int instanceId, int accountIndex,bool isEmergency) {
    BotInstance& bot = g_Bots[instanceId];
    AddLog(instanceId, Tr("Entering Sales Mode..."), ImVec4(0, 1, 1, 1));

    bot.accounts[accountIndex].salesCycleCount++;

    auto SmartSleep = [&](int ms) {
        int elapsed = 0;
        while (elapsed < ms) {
            if (!bot.isRunning) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            elapsed += 50;
        }
        return true;
        };

    MatchResult shopRes;
    bool shopOpened = false;

    for (int tryCount = 1; tryCount <= 3; tryCount++) {
        if (!bot.isRunning) return;

        bool shopFound = false;
        for (int k = 1; k <= 5; k++) {
            if (!bot.isRunning) return;
            cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
            shopRes = FindImage(screen, shop_templatePath, g_Thresholds.shopThreshold);

            if (shopRes.found) {
                shopFound = true;
                break;
            }
            AddLog(instanceId, std::string(Tr("Searching Shop... ")) + std::to_string(k) + "/5", ImVec4(1, 1, 0, 1));
            if (!SmartSleep(1000)) return;
        }

        if (!shopFound) {
            AddLog(instanceId, std::string(Tr("Shop NOT found. Retrying... (")) + std::to_string(tryCount) + "/3)", ImVec4(1, 0.5f, 0, 1));
            if (!SmartSleep(1000)) return;
            continue;
        }

        AdbTap(instanceId, shopRes.x, shopRes.y);
        if (!SmartSleep(g_Intervals.shopEnterWait)) return;

        cv::Mat verifyScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
        bool hasEmpty = FindImage(verifyScreen, crate_templatePath, g_Thresholds.crateThreshold).found;
        bool hasSold = !FindAllImages(verifyScreen, soldcrate_templatePath, 0.80f).empty();

        std::string filledTemplate = crate_wheat_templatePath;
        if (bot.testCropMode == 1) filledTemplate = crate_corn_templatePath;
        else if (bot.testCropMode == 2) filledTemplate = crate_carrot_templatePath;
        else if (bot.testCropMode == 3) filledTemplate = crate_soybean_templatePath;
        else if (bot.testCropMode == 4) filledTemplate = crate_sugarcane_templatePath;

        bool hasFilled = FindImage(verifyScreen, filledTemplate, 0.75f).found;

        if (hasEmpty || hasSold || hasFilled) {
            shopOpened = true;
            break;
        }
        else {
            AddLog(instanceId, std::string(Tr("Shop click missed (Bug)! Retrying... (")) + std::to_string(tryCount) + "/3)", ImVec4(1, 0.5f, 0, 1));
        }
    }

    if (!shopOpened) {
        AddLog(instanceId, Tr("Failed to open shop menu. Skipping sales."), ImVec4(1, 0.2f, 0.2f, 1));
        return;
    }

    bool doWebhook = (g_EnableBarnWebhook);
    bool webhookDoneThisCycle = false;

    int salesCount = 0;

    // Checks if it was in quarantine (shop full)
    bool wasInQuarantine = bot.accounts[accountIndex].isShopFullStuck;
    bool adPlacedThisCycle = false;

    while (bot.isRunning && salesCount < 10) {
        cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);

        // =========================================================
		// 1. COLLECT SOLD CRATES (IF ANY)
        // =========================================================
        std::vector<MatchResult> soldCrates = FindAllImages(screen, soldcrate_templatePath, 0.80f);
        if (!soldCrates.empty()) {

            bot.accounts[accountIndex].isShopFullStuck = false; // LIFT QUARANTINE.

            // COLLECT THE COINS AND GO BACK TO THE FIELDS          
            if (wasInQuarantine) {
                AddLog(instanceId, "Quarantine lifted via sold crates! Skipping coin collect to save time. Returning to farm.", ImVec4(0, 1, 0, 1));
                break; 
            }

          
            AddLog(instanceId, Tr("Collecting coins..."), ImVec4(0, 1, 0, 1));
            for (auto& crate : soldCrates) {
                if (!bot.isRunning) return;
                AdbTap(instanceId, crate.x, crate.y);
                if (!SmartSleep(300)) return;
            }
            if (!SmartSleep(g_Intervals.coinCollectWait)) return;
            continue;
        }

        // =========================================================
		// 2. FIND EMPTY CRATES. IF NONE, CHECK FOR ADVERTISEMENT POSSIBILITY.
        // =========================================================
        MatchResult emptyCrate = FindImage(screen, crate_templatePath, g_Thresholds.crateThreshold);
        if (!emptyCrate.found) {
            AddLog(instanceId, Tr("Shop is full."), ImVec4(1, 0.8f, 0, 1));
            if (g_TransferRequest.isPending && g_TransferRequest.senderInstanceId == instanceId) {
                AddLog(instanceId, "HEIST ABORTED: Shop is completely full!", ImVec4(1, 0, 0, 1));
                g_TransferRequest.isPending = false;
            }

			// IF WE CAN PLACE AN ADVERTISEMENT, DO IT. OTHERWISE, MARK ACCOUNT AS QUARANTINED (SHOP FULL STUCK)
            if (adPlacedThisCycle) {
                AddLog(instanceId, "Ad already placed this cycle. Closing shop normally.", ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
				break; // PRESS CROSS TO EXIT SHOP
            }

            // ---SHOP FULL PROTOCOL
            std::string filledTemplate = crate_wheat_templatePath;
            if (bot.testCropMode == 1) filledTemplate = crate_corn_templatePath;
            else if (bot.testCropMode == 2) filledTemplate = crate_carrot_templatePath;
            else if (bot.testCropMode == 3) filledTemplate = crate_soybean_templatePath;
            else if (bot.testCropMode == 4) filledTemplate = crate_sugarcane_templatePath;

            MatchResult filledCrate = FindImage(screen, filledTemplate, 0.75f);
            if (filledCrate.found) {
                AddLog(instanceId, "Checking advertisement availability on filled crate...", ImVec4(1, 1, 0, 1));
                AdbTap(instanceId, filledCrate.x, filledCrate.y);
                if (!SmartSleep(1200)) return;

                cv::Mat editScreen;
                MatchResult advNowRes;

                int cropLeft = 240, cropRight = 240, cropTop = 131, cropBottom = 170;

                for (int adTry = 0; adTry < 3; adTry++) {
                    editScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    int roiW = editScreen.cols - cropLeft - cropRight;
                    int roiH = editScreen.rows - cropTop - cropBottom;

                    if (roiW > 0 && roiH > 0 && cropLeft + roiW <= editScreen.cols && cropTop + roiH <= editScreen.rows) {
                        cv::Rect customRoi(cropLeft, cropTop, roiW, roiH);
                        cv::Mat croppedScreen = editScreen(customRoi);
                        advNowRes = FindImage(croppedScreen, advertise_templatePath, 0.60f, false, 1.0f, false);
                        if (advNowRes.found) {
                            advNowRes.x += cropLeft; advNowRes.y += cropTop;
                            break;
                        }
                    }
                    AddLog(instanceId, "Ad icon not found. Waiting... (" + std::to_string(adTry + 1) + "/3)", ImVec4(1, 0.5f, 0, 1));
                    SmartSleep(400);
                }

                if (advNowRes.found) {
                    AddLog(instanceId, "Ad available! Placing advertisement...", ImVec4(0, 1, 0, 1));
                    AdbTap(instanceId, advNowRes.x, advNowRes.y);
                    adPlacedThisCycle = true; // TURN ON ADVERTISE PLACED THIS CYCLE BECAUSE BOT CAN FALSE DETECT AFTER GIVING ADVERTISEMENT AND STILL HAVE SHOP FULL.
                    // SO IF THERES AN ACTIVE ADVERTISEMENT, ALL OF THE PRODUCTS GOING TO SELL ANYWAY, NO NEED TO ENABLE SHOP FULL
                    if (!SmartSleep(1000)) return;

                    cv::Mat adScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult createAdRes = FindImage(adScreen, "templates\\createad.png", 0.70f, false);
                    if (createAdRes.found) {
                        AdbTap(instanceId, createAdRes.x, createAdRes.y);
                        AddLog(instanceId, "Advertisement published!", ImVec4(0, 1, 0, 1));
                        if (!SmartSleep(1000)) return;
                    }

                    cv::Mat crossScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult crossRes = FindImage(crossScreen, cross_templatePath, g_Thresholds.crossThreshold);
                    if (crossRes.found) AdbTap(instanceId, crossRes.x, crossRes.y);

                    bot.accounts[accountIndex].isShopFullStuck = false;
                    bot.accounts[accountIndex].lastAdTime = std::chrono::steady_clock::now();

                    AddLog(instanceId, "Waiting 60 seconds for crops to be sold...", ImVec4(1, 0.5f, 0, 1));
                    for (int w = 0; w < 60; w++) {
                        if (!bot.isRunning) return;
                        bot.statusText = "Waiting Sales: " + std::to_string(60 - w) + "s";
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    continue;
                }
                else {
                    AddLog(instanceId, "Ad on cooldown! Marking account as QUARANTINED.", ImVec4(1, 0.2f, 0.2f, 1));
                    bot.accounts[accountIndex].isShopFullStuck = true;
                    bot.accounts[accountIndex].lastShopCheckTime = std::chrono::steady_clock::now();

                    for (int c = 0; c < 2; c++) {
                        cv::Mat crossScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                        MatchResult crossRes = FindImage(crossScreen, cross_templatePath, g_Thresholds.crossThreshold);
                        if (crossRes.found) AdbTap(instanceId, crossRes.x, crossRes.y);
                        SmartSleep(800);
                    }
                    break;
                }
            }
            break;
        }
        else {
			// IF THERES EMPTY CRATE THAT MEANS NO LONGER NEED TO BE IN QUARANTINE, UNMARK IT.
            bot.accounts[accountIndex].isShopFullStuck = false;


            if (wasInQuarantine) {
                AddLog(instanceId, "Quarantine lifted via empty crate! Returning to farm.", ImVec4(0, 1, 0, 1));
                break;
            }
        }

		// CLICK SELLING MENU BY CLICKING THE EMPTY CRATE
        AdbTap(instanceId, emptyCrate.x, emptyCrate.y);
        if (!SmartSleep(g_Intervals.crateClickWait)) return;

        // =========================================================
        // 3. WEBHOOK & ITEM COUNT ROUTINE.
        // =========================================================
        if (!isEmergency && doWebhook && !webhookDoneThisCycle) {
            AddLog(instanceId, Tr("Executing Webhook Routine (Checking for Transfer)..."), ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
            cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
            MatchResult barnRes = FindImage(screen, barn_market_templatePath, 0.80f);

            if (barnRes.found) {
                AdbTap(instanceId, barnRes.x, barnRes.y);
 
                if (!SmartSleep(2000)) return;

                cv::Mat fullScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                if (!fullScreen.empty()) {


                    std::string webhookImgPath = "C:\\Users\\Public\\webhook_capture_inst" + std::to_string(instanceId) + ".png";
                    cv::imwrite(webhookImgPath, fullScreen);

                    auto GetRealCount = [&](const std::string& tplPath) -> std::string {
                        // cropped yerine direkt fullScreen'den aratıyoruz
                        int count = ReadItemCountByAnchor(fullScreen, tplPath);
                        return std::to_string(count);
                        };

                    std::string boltCount = GetRealCount("templates\\bolt.png");
                    std::string tapeCount = GetRealCount("templates\\duct_tape.png");
                    std::string plankCount = GetRealCount("templates\\plank.png");
                    std::string nailCount = GetRealCount("templates\\nail.png");
                    std::string screwCount = GetRealCount("templates\\screw.png");
                    std::string panelCount = GetRealCount("templates\\wood_panel.png");

                    InventoryData& inv = bot.accounts[accountIndex].currentInv;
                    try {
                        inv.bolt = std::stoi(boltCount); inv.tape = std::stoi(tapeCount);
                        inv.plank = std::stoi(plankCount); inv.nail = std::stoi(nailCount);
                        inv.screw = std::stoi(screwCount); inv.panel = std::stoi(panelCount);
                        SaveInventoryData();
                    }
                    catch (...) {}


                    int cycleCount = bot.accounts[accountIndex].salesCycleCount;
                    int currentCoins = bot.accounts[accountIndex].coinAmount;


                    std::thread([instanceId, accountIndex, cycleCount, currentCoins, boltCount, plankCount, tapeCount, nailCount, screwCount, panelCount, webhookImgPath]() {


                        std::string msg = "📊 **[NXRTH] INSTANCE " + std::to_string(instanceId + 1) + " | SLOT " + std::to_string(accountIndex + 1) + "**\n";
                        msg += "🔄 Sales Cycle: **" + std::to_string(cycleCount) + "**\n";
                        msg += "<:Coin:305966854608912386> Total Coins: **" + std::to_string(currentCoins) + "**\n";
                        msg += "--------------------------------------\n";
                        msg += "💰 **BARN INVENTORY**\n";
                        msg += "<:Bolt:304466477527072768> Bolt: **" + boltCount + "** | ";
                        msg += "<:Plank:304466477409370113> Plank: **" + plankCount + "** | ";
                        msg += "<:DuctTape:304466477564690432> Tape: **" + tapeCount + "**\n";
                        msg += "<:Nail:304466477489324042> Nail: **" + nailCount + "** | ";
                        msg += "<:Screw:304466477438992394> Screw: **" + screwCount + "** | ";
                        msg += "<:WoodPanel:304466476977356802> Wood Panel: **" + panelCount + "**\n";

                        if (g_EnableWebhookImage) Discord::SendWebhookImage(webhookImgPath, msg);
                        else Discord::SendWebhookMessage(msg);
                        }).detach();
                }

                screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                MatchResult siloRes = FindImage(screen, silo_market_templatePath, 0.80f);
                if (siloRes.found) { AdbTap(instanceId, siloRes.x, siloRes.y); SmartSleep(1000); }
            }
            webhookDoneThisCycle = true;


            if (g_Bots[5].isRunning && instanceId != 5) {
                InventoryData& chk = bot.accounts[accountIndex].currentInv;
                if (chk.bolt >= g_TransferThreshold || chk.tape >= g_TransferThreshold || chk.plank >= g_TransferThreshold || chk.nail >= g_TransferThreshold || chk.screw >= g_TransferThreshold || chk.panel >= g_TransferThreshold) {
                    AdbTap(instanceId, barnRes.x, barnRes.y);
                    SmartSleep(500);
                }
            }
        }


        if (!isEmergency && instanceId != 5 && !g_TransferRequest.isPending && g_Bots[5].isRunning) {
            InventoryData& inv = bot.accounts[accountIndex].currentInv;
            std::string triggeredItem = "";
            int triggeredAmount = 0;

            if (inv.bolt >= g_TransferThreshold) { triggeredItem = "bolt"; triggeredAmount = inv.bolt; }
            else if (inv.tape >= g_TransferThreshold) { triggeredItem = "duct_tape"; triggeredAmount = inv.tape; }
            else if (inv.plank >= g_TransferThreshold) { triggeredItem = "plank"; triggeredAmount = inv.plank; }
            else if (inv.nail >= g_TransferThreshold) { triggeredItem = "nail"; triggeredAmount = inv.nail; }
            else if (inv.screw >= g_TransferThreshold) { triggeredItem = "screw"; triggeredAmount = inv.screw; }
            else if (inv.panel >= g_TransferThreshold) { triggeredItem = "wood_panel"; triggeredAmount = inv.panel; }

            if (triggeredItem != "") {
                g_TransferRequest.isPending = true;
                g_TransferRequest.senderInstanceId = instanceId;
                g_TransferRequest.senderAccountId = accountIndex;
                g_TransferRequest.sellerFarmName = bot.accounts[accountIndex].farmName;
                g_TransferRequest.sellerTag = bot.accounts[accountIndex].playerTag;
                g_TransferRequest.itemType = triggeredItem;
                g_TransferRequest.itemAmount = triggeredAmount;
                g_TransferRequest.needFriendship = !bot.accounts[accountIndex].isFriendWithStorage;

                AddLog(instanceId, "RADIO: Transfer triggered! (" + std::to_string(triggeredAmount) + "x " + triggeredItem + " remaining)", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
            }
        }


        if (g_TransferRequest.isPending && g_TransferRequest.senderInstanceId == instanceId) {
            // --- 1. AŞAMA: ARKADAŞLIK EL SIKIŞMASI ---
            if (g_TransferRequest.needFriendship) {
                AddLog(instanceId, "HEIST: Not friends with storage. Initiating handshake...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));

                if (!SendFriendRequestToStorage(instanceId, g_StorageTag)) {
                    AddLog(instanceId, "HEIST FAILED: Could not send request.", ImVec4(1, 0, 0, 1));
                    g_TransferRequest.isPending = false; g_TransferRequest.needFriendship = false;
                    break;
                }

                g_TransferRequest.friendRequestSent = true;
                bot.statusText = "WAITING FRIEND ACCEPT";

                int waitAccept = 0;
                while (g_TransferRequest.needFriendship && waitAccept < 600) {
                    if (!bot.isRunning) return;
                    if (!g_TransferRequest.isPending) {
                        AddLog(instanceId, "HEIST ABORTED: Storage bot cancelled the operation!", ImVec4(1, 0, 0, 1));
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    waitAccept++;
                }

                if (!g_TransferRequest.needFriendship) {
                    AddLog(instanceId, "HEIST: Friendship accepted! Closing menus and re-entering shop...", ImVec4(0, 1, 0, 1));
                    bot.accounts[accountIndex].isFriendWithStorage = true;
                    SaveConfig();

                    // 1. Close all menus
                    ForceCloseAllMenus(instanceId);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));


                    AdbTap(instanceId, 400, 50);
                    std::this_thread::sleep_for(std::chrono::milliseconds(2500));


                    bool shopOpened = false;
                    for (int k = 0; k < 4; k++) {
                        cv::Mat scr = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);

                        MatchResult shopRe = FindImage(scr, shop_templatePath, 0.75f);

                        if (shopRe.found) {
                            AddLog(instanceId, "HEIST: Shop found, tapping...", ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                            AdbTap(instanceId, shopRe.x, shopRe.y);
                            std::this_thread::sleep_for(std::chrono::milliseconds(2500));


                            cv::Mat verifyScr = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                            MatchResult verifyCrate = FindImage(verifyScr, crate_templatePath, g_Thresholds.crateThreshold);
                            MatchResult verifyCross = FindImage(verifyScr, cross_templatePath, g_Thresholds.crossThreshold);

                            if (verifyCrate.found || verifyCross.found) {
                                AddLog(instanceId, "HEIST: Shop verified as open!", ImVec4(0, 1, 0, 1));
                                shopOpened = true;
                                break;
                            }
                            else {
                                AddLog(instanceId, "HEIST: Shop didn't open. False positive or lag. Retrying...", ImVec4(1, 0.5f, 0, 1));
                            }
                        }
                        else {
                            AddLog(instanceId, "HEIST: Shop not visible yet, waiting...", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        }
                    }

                    if (!shopOpened) {
                        AddLog(instanceId, "HEIST ERROR: Could not verify shop is open after multiple tries!", ImVec4(1, 0, 0, 1));
                        g_TransferRequest.isPending = false;
                        break;
                    }

                    continue; 
                }
                else {
                    g_TransferRequest.isPending = false; break;
                }
            }

        
            g_TransferRequest.targetSlotX = emptyCrate.x;
            g_TransferRequest.targetSlotY = emptyCrate.y;

            bot.statusText = "WAITING STORAGE BOSS";
            int waitStorage = 0;
            while (!g_TransferRequest.storageReady && waitStorage < 600) {
                if (!bot.isRunning) return;
                if (!g_TransferRequest.isPending) {
                    AddLog(instanceId, "HEIST ABORTED: Storage bot failed to infiltrate!", ImVec4(1, 0, 0, 1));
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                waitStorage++;
            }

            if (g_TransferRequest.storageReady) {
                cv::Mat shopScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                std::string itemTemplate = "templates\\" + g_TransferRequest.itemType + ".png";
                MatchResult targetItemRes = FindImage(shopScreen, itemTemplate, 0.75f, false);

                if (targetItemRes.found) {
                    AdbTap(instanceId, targetItemRes.x, targetItemRes.y); SmartSleep(500);


                    for (int k = 0; k < 10; k++) { AdbTap(instanceId, 467, 173); SmartSleep(100); }

                    AdbTap(instanceId, 400, 240); SmartSleep(300); 

                    shopScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult saleBtn = FindImage(shopScreen, create_sale_templatePath, g_Thresholds.createSaleThreshold);
                    if (saleBtn.found) {
                        AdbTap(instanceId, saleBtn.x, saleBtn.y);
                        AddLog(instanceId, "HEIST: Item Listed! GO GO GO!", ImVec4(1, 0, 0, 1));
                        g_TransferRequest.itemListed = true;


                        int waitTransfer = 0;
                        while (!g_TransferRequest.transferComplete && waitTransfer < 200) {
                            if (!bot.isRunning) return;
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            waitTransfer++;
                        }

                        if (g_TransferRequest.transferComplete) {
                            AddLog(instanceId, "HEIST: Transfer complete. Collecting coins...", ImVec4(0, 1, 0, 1));
                            AdbTap(instanceId, g_TransferRequest.targetSlotX, g_TransferRequest.targetSlotY);
                            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

               
                            InventoryData& inv = bot.accounts[accountIndex].currentInv;
                            if (g_TransferRequest.itemType == "bolt") inv.bolt -= 10;
                            else if (g_TransferRequest.itemType == "duct_tape") inv.tape -= 10;
                            else if (g_TransferRequest.itemType == "plank") inv.plank -= 10;
                            else if (g_TransferRequest.itemType == "nail") inv.nail -= 10;
                            else if (g_TransferRequest.itemType == "screw") inv.screw -= 10;
                            else if (g_TransferRequest.itemType == "wood_panel") inv.panel -= 10;

                        
                            if (inv.bolt >= g_TransferThreshold || inv.tape >= g_TransferThreshold || inv.plank >= g_TransferThreshold ||
                                inv.nail >= g_TransferThreshold || inv.screw >= g_TransferThreshold || inv.panel >= g_TransferThreshold) {
                                g_TransferRequest.hasMoreItems = true;
                                AddLog(instanceId, "HEIST: I have more items! Telling Storage Bot to wait...", ImVec4(1, 1, 0, 1));
                            }
                            else {
                                g_TransferRequest.hasMoreItems = false;
                                AddLog(instanceId, "HEIST: All items transferred. Telling Storage Bot to go home.", ImVec4(0, 1, 0, 1));
                            }
                        }
                        else {
                            AddLog(instanceId, "HEIST ERROR: Storage bot didn't confirm receipt! (Timeout)", ImVec4(1, 0.5f, 0, 1));
                            g_TransferRequest.hasMoreItems = false; 
                        }

                       
                        g_TransferRequest.isPending = false; g_TransferRequest.storageReady = false;
                        g_TransferRequest.itemListed = false; g_TransferRequest.transferComplete = false;

                        salesCount++;
                        continue;
                    }
                }
            }
            g_TransferRequest.isPending = false;
            continue;
        }



        screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
        std::string productTemplate = wheatshop_templatePath;
        if (bot.testCropMode == 1) productTemplate = cornshop_templatePath;
        else if (bot.testCropMode == 2) productTemplate = carrot_shop_templatePath;
        else if (bot.testCropMode == 3) productTemplate = soybean_shop_templatePath;
        else if (bot.testCropMode == 4) productTemplate = sugarcane_shop_templatePath;

        MatchResult productRes = FindImage(screen, productTemplate, 0.75f);

        if (!productRes.found) {
            AddLog(instanceId, Tr("No crops to sell."), ImVec4(1, 0.5f, 0, 1));
            MatchResult crossRes = FindImage(screen, cross_templatePath, g_Thresholds.crossThreshold);
            if (crossRes.found) {
                AdbTap(instanceId, crossRes.x, crossRes.y);
                SmartSleep(700); 
                MatchResult crossRes = FindImage(screen, cross_templatePath, g_Thresholds.crossThreshold);
                if (crossRes.found) {
                    AdbTap(instanceId, crossRes.x, crossRes.y);
					SmartSleep(700);
                }
            }
            break; 
        }

        AdbTap(instanceId, productRes.x, productRes.y);
        if (!SmartSleep(g_Intervals.productSelectWait)) return;

        screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);


        for (int a = 0; a < 6; a++) {
			AdbTap(instanceId, 524, 161); // Plus
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

		// MAX PRICE BUTTON (ARROWS TO BE EXACT)
        AdbTap(instanceId, 491, 247);

        // Randomizer (Anti-Ban PRICE DROPPER) 
        if (g_Bots[instanceId].enableShopRandomizer) {
            int randomClicks = rand() % (g_Bots[instanceId].shopRandomizerMax + 1);
            if (randomClicks > 0) {
                AddLog(instanceId, std::string(Tr("Anti-Ban: Decreasing price ")) + std::to_string(randomClicks) + Tr(" times."), ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
                for (int m = 0; m < randomClicks; m++) {
                    if (!bot.isRunning) return;
                    AdbTap(instanceId, 411, 218); // Minus BUTTON
                    if (!SmartSleep(100 + (rand() % 150))) return;
                }
            }
        }

        // PLACE AD
        auto now = std::chrono::steady_clock::now();
        auto elapsedMinutes = std::chrono::duration_cast<std::chrono::minutes>(now - bot.accounts[accountIndex].lastAdTime).count();

        if (elapsedMinutes >= 5) {
            screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
            MatchResult adRes = FindImage(screen, advertise_templatePath, g_Thresholds.advertiseThreshold);
            if (adRes.found) {
                AddLog(instanceId, Tr("Placing Advertisement..."), ImVec4(0, 1, 0, 1));
                AdbTap(instanceId, adRes.x, adRes.y);
                adPlacedThisCycle = true; // CONFIRM THAT AD WAS PLACED IN THIS CYCLE.
                if (!SmartSleep(300)) return;
                bot.accounts[accountIndex].lastAdTime = now;
            }
        }

		// PUT ON SALE BUTTON
        screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
        MatchResult saleBtn = FindImage(screen, create_sale_templatePath, g_Thresholds.createSaleThreshold);
        if (saleBtn.found) {
            AdbTap(instanceId, saleBtn.x, saleBtn.y);
            AddLog(instanceId, Tr("Item Sold."), ImVec4(0, 1, 0, 1));
            salesCount++;
            bot.totalSales++;
            if (!SmartSleep(g_Intervals.createSaleWait)) return;
        }
        else {
			AddLog(instanceId, Tr("Put on Sale button not found, using custom x,y coordinates."), ImVec4(1, 0, 0, 1));
			AdbTap(instanceId, 464, 384); // COORDINATES JUST IN CASE.    
        }
    }
	SmartSleep(1000);
    cv::Mat finalScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
    MatchResult crossRes = FindImage(finalScreen, market_close_crosstemplatePath, g_Thresholds.marketCloseCrossThreshold);
    if (crossRes.found) AdbTap(instanceId, crossRes.x, crossRes.y);
	AdbTap(instanceId, 480, 130); //tap cross in shop menu (not seed menu) to exit shop completely.
}
// MAIN BOT FUNCTION. THIS FUNCTION RUNS THE ENTIRE BOT LOGIC FOR ONE INSTANCE IN A LOOP UNTIL THE BOT IS STOPPED.
void RunPremiumBot(int instanceId) {
    BotInstance& bot = g_Bots[instanceId];
    // --- MEMUC STRICT INSPECTOR ---
    //AddLog(instanceId, "[INSPECTOR] Checking MEmu engine configuration...");

    //std::string width = GetMEmuConfig(instanceId, "resolution_width");
    //std::string height = GetMEmuConfig(instanceId, "resolution_height");
    //std::string dpi = GetMEmuConfig(instanceId, "vbox_dpi");

	// STOP THE BOT IF THE RESOLUTION OR DPI SETTINGS ARE NOT CORRECT TO AVOID UNEXPECTED BEHAVIOR AND FALSE DETECTIONS.
    //if (width != "640" || height != "480" || dpi != "100") {
        //AddLog(instanceId, "[FATAL ERROR] Emulator Settings are WRONG!");
        //AddLog(instanceId, ("-> Current: " + width + "x" + height + " | DPI: " + dpi).c_str());
        //AddLog(instanceId, "-> REQUIRED: 640x480 | DPI: 100");
        //AddLog(instanceId, "Please change settings in MEmu, restart emulator and try again.");
       // bot.isRunning = false;
      //  return;
    //}
    //AddLog(instanceId, "[INSPECTOR] Settings verified (640x480 | 100 DPI)."); 
    AddLog(instanceId, Tr("Bot Started."), ImVec4(0, 1, 0, 1));
    if (strcmp(bot.inputDevice, "/dev/input/event1") == 0) {
        if (AutoDetectTouchDevice(instanceId)) {
            SaveConfig();
        }
    }
    AddLog(instanceId, "Waking up Minitouch agent and opening ports...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
    StartMinitouchStealth(instanceId);

    // WAITING 2 SECONDS FOR THE MINITOUCH TO WAKE UP PROPERLY.
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    while (bot.isRunning) {
        EmulatorCrashWatchdog(instanceId);
        int activeAccountCount = 0;
        for (int k = 0; k < 15; k++) {
            if (bot.accounts[k].hasFile) activeAccountCount++;
        }

        bool isSingleAccountMode = bot.useSingleMode || (activeAccountCount <= 1);

        if (isSingleAccountMode) {
            bot.statusText = Tr("Single Account Mode");
        }
        bool processedOneSingleAccount = false;
        for (int i = 0; i < 15; i++) {
            if (!bot.isRunning) break;
            if (isSingleAccountMode && processedOneSingleAccount) {
                break;
            }
            if (activeAccountCount == 0) {
                if (i > 0) break;
            }
            else {
                if (!bot.accounts[i].hasFile) continue;
            }

            bot.currentAccountIndex = i;
            processedOneSingleAccount = true; 
            // =========================================================================
            // 1. SHOP FULL (TIME) CONTROL
            // =========================================================================
            if (bot.accounts[i].isShopFullStuck) {
                auto now = std::chrono::steady_clock::now();
                auto elapsedMins = std::chrono::duration_cast<std::chrono::minutes>(now - bot.accounts[i].lastShopCheckTime).count();

                if (elapsedMins < 2) { 
                    if (isSingleAccountMode) {
                        int waitSecs = 120 - std::chrono::duration_cast<std::chrono::seconds>(now - bot.accounts[i].lastShopCheckTime).count();
                        AddLog(instanceId, "Single Account Quarantine: Waiting " + std::to_string(waitSecs) + "s for Ad Cooldown...", ImVec4(1, 0.5f, 0, 1));
                        for (int w = 0; w < waitSecs; w++) {
                            if (!bot.isRunning) break;
                            bot.statusText = "Ad Cooldown: " + std::to_string(waitSecs - w) + "s";
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                        }
                    }
                    else {
                        AddLog(instanceId, "Account is in Shop Full Quarantine. Skipping to next account...", ImVec4(1, 0.2f, 0.2f, 1));
                        continue; 
                    }
                }
            }
            int currentCropTime = 120; // Default Wheat
            if (bot.testCropMode == 1) currentCropTime = 300;       
            else if (bot.testCropMode == 2) currentCropTime = 600;  
            else if (bot.testCropMode == 3) currentCropTime = 1200; 
            else if (bot.testCropMode == 4) currentCropTime = 1800; 

            if (!bot.accounts[i].isFirstRun) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - bot.accounts[i].lastPlantTime).count();
                int remaining = currentCropTime - (int)elapsed;

                if (remaining > 0) {
                    AddLog(instanceId, std::string(Tr("Waiting for growth (")) + std::to_string(remaining) + Tr("s)..."), ImVec4(1, 1, 0, 1));
                    for (int t = 0; t < remaining; t++) {
                        if (!bot.isRunning) return;
                        bot.statusText = std::string(Tr("Waiting Slot ")) + std::to_string(i + 1) + ": " + std::to_string(remaining - t) + "s";

                        if (t > 0 && (t % bot.reviveCheckInterval == 0)) {
                            HandleReviveHeartbeat(instanceId);
                        }
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                }
            }

            if (!isSingleAccountMode) {
                bot.statusText = std::string(Tr("Loading Slot ")) + std::to_string(i + 1);
                LoadAccountFromSlot(instanceId, i);

                AddLog(instanceId, std::string(Tr("Waiting ")) + std::to_string(g_Intervals.gameLoadWait) + Tr("s for game logic..."), ImVec4(1, 1, 0, 1));
                for (int w = 0; w < g_Intervals.gameLoadWait; w++) {
                    if (!bot.isRunning) break;
                    bot.statusText = std::string(Tr("Loading Game... ")) + std::to_string(g_Intervals.gameLoadWait - w) + "s";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                if (!bot.isRunning) break;

                bot.statusText = Tr("Game Ready.");
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            else {
                bot.statusText = Tr("Single Account Mode Active.");
                if (bot.accounts[i].isFirstRun) {
                    AddLog(instanceId, Tr("Single mode auto-detected. Skipping game restart."), ImVec4(0, 1, 1, 1));
                }
            }
            bot.statusText = Tr("Checking Game Load...");
            MatchResult mailRes;
            bool gameLoaded = false;

            // =========================================================================
     //      SKIP MAILBOX SCAN IN SINGLE MODE COMPLETELY
            // =========================================================================
            if (isSingleAccountMode) {
                gameLoaded = true; // ACCEPTING THE GAME IS OPEN ALREADY.
                if (bot.accounts[i].isFirstRun) {
                    AddLog(instanceId, "Single Account Mode: Skipping Mailbox scan, starting immediately!", ImVec4(0, 1, 0, 1));
                }
            }
            else {
                // ONLY WAIT IN MULTI ACCOUNT MODE.
                bot.statusText = Tr("Checking Game Load...");
                MatchResult mailRes;
                for (int k = 0; k < 45; k++) {
                    if (!bot.isRunning) break;
                    cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    mailRes = FindImage(screen, mailbox_templatePath, g_Thresholds.mailboxThreshold, false);

                    if (mailRes.found) {
                        gameLoaded = true;
                        if (bot.accounts[i].isFirstRun) {
                            AddLog(instanceId, Tr("Game Loaded! Mailbox found."), ImVec4(0, 1, 0, 1));
                        }
                        break;
                    }
                    bot.statusText = std::string(Tr("Waiting for Game... ")) + std::to_string(k) + "/45";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }

            if (!gameLoaded) {
                AddLog(instanceId, Tr("Timeout (Mailbox not found). Skipping account."), ImVec4(1, 0.4f, 0.4f, 1));
                continue;
            }
            cv::Mat curScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);

            // ACCOUNT INFORMATION READING GOES HERE.
            if (!curScreen.empty() && curScreen.cols > 600 && curScreen.rows > 100) {

               
                bot.accounts[i].coinAmount = ReadGameNumber(curScreen, cv::Rect(507, 2, 72, 21), "coin");
                bot.accounts[i].diamondAmount = ReadGameNumber(curScreen, cv::Rect(547, 22, 34, 21), "dia");

                if (bot.accounts[i].playerTag == "Unknown" || bot.accounts[i].playerTag == "NO_TAG") {
                    bot.statusText = Tr("Extracting Profile Data...");
                    AddLog(instanceId, Tr("Reading Account Profile Data..."), ImVec4(0.8f, 0.8f, 0.2f, 1.0f));

                   
                    bot.accounts[i].level = ReadGameNumber(curScreen, cv::Rect(16, 8, 22, 20), "lvl");

                    AdbTap(instanceId, 25, 18);
                    std::this_thread::sleep_for(std::chrono::milliseconds(800)); 
                    cv::Mat profileScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);

                    
                    if (!profileScreen.empty() && profileScreen.cols > 450 && profileScreen.rows > 150) {
                        bot.accounts[i].farmName = ReadFarmName(profileScreen, cv::Rect(203, 63, 220, 29));

                        AdbTap(instanceId, 482, 112);
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                        bot.accounts[i].playerTag = GetClipboardText();
                        SaveConfig();
                    }
                    else {
                        AddLog(instanceId, "Warning: Profile screen not loaded properly, skipping tag extraction.", ImVec4(1, 0.5f, 0, 1));
                    }

                    AdbTap(instanceId, 595, 45);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
           
            if (bot.accounts[i].isShopFullStuck) {
                bot.statusText = "Checking Ad Availability...";
                AddLog(instanceId, "Account in quarantine. Checking shop immediately to clear status...", ImVec4(1, 0.5f, 0, 1));

             
                RunSalesCycle(instanceId, i, true);

                if (bot.accounts[i].isShopFullStuck) {
                    AddLog(instanceId, "Still on Ad cooldown. Returning to quarantine...", ImVec4(1, 0.2f, 0.2f, 1));
                    continue; 
                }
                else {
                    AddLog(instanceId, "Quarantine lifted! Resuming normal farm operations.", ImVec4(0, 1, 0, 1));
                }
            }
            bool outOfSeeds = false;
            auto TryHarvest = [&]() -> int {
                bot.statusText = Tr("Scanning for Grown Crops...");

                cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                if (screen.empty()) return 0;

                std::vector<MatchResult> allGrown;

                
                if (bot.testCropMode == 0 || bot.testCropMode == 1 || bot.testCropMode == 2 || bot.testCropMode == 4) {
                    allGrown = FindGrownCrops(screen, bot.testCropMode);
                }
                else {
                    
                    MatchResult res = FindImage(screen, grown_soybean_templatePath, g_Thresholds.grownSoybeanThreshold, false);
                    if (res.found) allGrown.push_back(res);
                }

                if (allGrown.empty()) {
                    return 0; 
                }

                AddLog(instanceId, Tr("Grown crops detected via Color! Opening sickle menu..."), ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

                
                int anchorX = allGrown[0].x;
                int anchorY = allGrown[0].y;
                AdbTap(instanceId, anchorX, anchorY);
                std::this_thread::sleep_for(std::chrono::milliseconds(1200)); 

                // 3. FIND SICKLE
                MatchResult sickleRes;
                bool foundSickle = false;
                for (int k = 1; k <= 2; k++) {
                    if (!bot.isRunning) return 0;
                    cv::Mat verifyScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    sickleRes = FindImage(verifyScreen, s_templatePath, g_Thresholds.sickleThreshold, false);
                    if (sickleRes.found) { foundSickle = true; break; }
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }

                if (foundSickle) {
                    AddLog(instanceId, Tr("Sickle Found! Calculating harvest zone..."), ImVec4(0, 1, 0, 1));

                    // 4. HARVEST WITH Minitouch 
                    ExecuteDenseGridGesture(instanceId, sickleRes.x, sickleRes.y, allGrown);
					std::this_thread::sleep_for(std::chrono::milliseconds(g_Intervals.afterHarvestWait));

                    bot.totalHarvest++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(g_Intervals.afterHarvestWait));

                    // ==========================================================
                    // SILO FULL CONTROL
                    // ==========================================================
                    cv::Mat siloScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult siloFullRes = FindImage(siloScreen, "templates\\silo_full.png", 0.75f, false);
                    if (siloFullRes.found) {
                        AddLog(instanceId, Tr("SILO FULL DETECTED! Harvest interrupted."), ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                        MatchResult crossRes = FindImage(siloScreen, silo_full_cross_templatePath, g_Thresholds.siloFullCrossThreshold, false);
                        if (crossRes.found) AdbTap(instanceId, crossRes.x, crossRes.y);
                        else AdbTap(instanceId, siloFullRes.x, siloFullRes.y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        return 2;
                    }

                    return 1;
                }
                else {
                    
                    cv::Mat verifyScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult crossRes = FindImage(verifyScreen, cross_templatePath, g_Thresholds.crossThreshold, false);
                    if (crossRes.found) { AdbTap(instanceId, crossRes.x, crossRes.y); std::this_thread::sleep_for(std::chrono::milliseconds(500)); }
                }
                return 0;
                };

            auto TryAutoTom = [&]() -> bool {
                if (!bot.accounts[i].autoTomEnabled) return false;

                auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                if (bot.accounts[i].tomStartTime != 0) {
                    long long expireTime = bot.accounts[i].tomStartTime + (bot.accounts[i].tomRemainingHours * 3600);
                    if (now > expireTime) {
                        bot.accounts[i].autoTomEnabled = false;
                        SaveConfig();
                        AddLog(instanceId, Tr("Tom contract expired! Auto Tom disabled."), ImVec4(1, 0.2f, 0.2f, 1));
                        return false;
                    }
                }

                if (now < bot.accounts[i].nextTomTime) {
                    return false;
                }

                bot.statusText = Tr("Deploying Auto Tom...");
                AddLog(instanceId, Tr("Initiating Auto Tom sequence..."), ImVec4(0.8f, 0.4f, 1.0f, 1.0f));

                bool menuOpened = false;
                int savedTomX = 0;
                int savedTomY = 0;

                for (int retry = 1; retry <= 3; retry++) {
                    if (!bot.isRunning) return false;

                    cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);

                    MatchResult barnCheck = FindImage(screen, "templates\\tom_barn.png", 0.80f, false);
                    MatchResult siloCheck = FindImage(screen, "templates\\tom_silo.png", 0.80f, false);

                    if (barnCheck.found || siloCheck.found) {
                        menuOpened = true;
                        break;
                    }

                    MatchResult tomRes = FindImage(screen, "templates\\tom_crate.png", 0.75f, false);
                    if (tomRes.found) {
                        savedTomX = tomRes.x;
                        savedTomY = tomRes.y;

                        AdbTap(instanceId, tomRes.x, tomRes.y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

                        screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                        barnCheck = FindImage(screen, "templates\\tom_barn.png", 0.80f, false);
                        siloCheck = FindImage(screen, "templates\\tom_silo.png", 0.80f, false);

                        if (barnCheck.found || siloCheck.found) {
                            menuOpened = true;
                            break;
                        }
                    }

                    AddLog(instanceId, std::string(Tr("Tom menu not found. Retrying... (")) + std::to_string(retry) + "/3)", ImVec4(1, 0.5f, 0, 1));
                    MatchResult crossRes = FindImage(screen, cross_templatePath, g_Thresholds.crossThreshold, false);
                    if (crossRes.found) {
                        AdbTap(instanceId, crossRes.x, crossRes.y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                }

                if (!menuOpened) {
                    AddLog(instanceId, Tr("Tom menu failed to open after 3 retries! Aborting sequence."), ImVec4(1, 0.2f, 0.2f, 1));
                    cv::Mat failScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult failCross = FindImage(failScreen, cross_templatePath, g_Thresholds.crossThreshold, false);
                    if (failCross.found) {
                        AddLog(instanceId, Tr("Closing stuck menu before returning to farm..."), ImVec4(1, 0.5f, 0, 1));
                        AdbTap(instanceId, failCross.x, failCross.y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                    return false;
                }

                std::string catTemplate = (bot.accounts[i].tomCategory == 0) ? "templates\\tom_barn.png" : "templates\\tom_silo.png";
                cv::Mat finalScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                MatchResult catRes = FindImage(finalScreen, catTemplate, 0.80f, false);
                if (catRes.found) {
                    AdbTap(instanceId, catRes.x, catRes.y);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }

                int SEARCH_ICON_X = 150;   
                int SEARCH_ICON_Y = 120;   
                int TEXT_BOX_X = 250;    
                int TEXT_BOX_Y = 175;    
                int FIRST_ITEM_X = 196;    
                int FIRST_ITEM_Y = 234;    
                int YES_BUTTON_X = 436;    
                int YES_BUTTON_Y = 291;    

                AdbTap(instanceId, SEARCH_ICON_X, SEARCH_ICON_Y);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                AddLog(instanceId, Tr("Focusing on search text box..."), ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
                AdbTap(instanceId, TEXT_BOX_X, TEXT_BOX_Y);
                std::this_thread::sleep_for(std::chrono::milliseconds(800));

                RunAdbCommand(instanceId, "shell input text '" + bot.accounts[i].tomItemName + "'");
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));

                AdbTap(instanceId, FIRST_ITEM_X, FIRST_ITEM_Y);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                AdbTap(instanceId, YES_BUTTON_X, YES_BUTTON_Y);

                AddLog(instanceId, Tr("Tom deployed. Waiting 30s for him to return..."), ImVec4(1, 1, 0, 1));
                for (int w = 0; w < 30; w++) {
                    if (!bot.isRunning) return false;
                    bot.statusText = std::string(Tr("Tom returning... ")) + std::to_string(30 - w) + "s";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                if (savedTomX != 0 && savedTomY != 0) {
                    AddLog(instanceId, Tr("Tom is back! Clicking saved Crate location..."), ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
                    bot.statusText = Tr("Opening Tom Boxes...");
                    AdbTap(instanceId, savedTomX, savedTomY);
                    std::this_thread::sleep_for(std::chrono::milliseconds(2500)); 
                }
                else {
                    AddLog(instanceId, Tr("Saved location missing, searching crate again..."), ImVec4(1, 0.5f, 0, 1));
                    cv::Mat retScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult retTom = FindImage(retScreen, "templates\\tom_crate.png", 0.75f, false);
                    if (retTom.found) {
                        AdbTap(instanceId, retTom.x, retTom.y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
                    }
                }

                int BOX_RIGHT_X = 525;
                int BOX_RIGHT_Y = 310;
                int BOX_MID_X = 428;
                int BOX_MID_Y = 313;
                int BOX_LEFT_X = 332;
                int BOX_LEFT_Y = 316;

                AdbTap(instanceId, BOX_RIGHT_X, BOX_RIGHT_Y);
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));

                cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                MatchResult crossRes = FindImage(screen, cross_templatePath, g_Thresholds.crossThreshold, false);

                if (crossRes.found) {
                    AddLog(instanceId, Tr("Not enough coins for Max Stack! Falling back to Mid..."), ImVec4(1, 0.5f, 0, 1));
                    AdbTap(instanceId, crossRes.x, crossRes.y);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                    AdbTap(instanceId, BOX_MID_X, BOX_MID_Y);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

                    screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult crossRes2 = FindImage(screen, cross_templatePath, g_Thresholds.crossThreshold, false);

                    if (crossRes2.found) {
                        AddLog(instanceId, Tr("Still poor! Falling back to Min Stack..."), ImVec4(1, 0.5f, 0, 1));
                        AdbTap(instanceId, crossRes2.x, crossRes2.y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                        AdbTap(instanceId, BOX_LEFT_X, BOX_LEFT_Y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                    }
                }

                AddLog(instanceId, Tr("Purchase confirmed! Waiting 30s for Tom to deliver..."), ImVec4(1, 1, 0, 1));
                for (int w = 0; w < 30; w++) {
                    if (!bot.isRunning) return false;
                    bot.statusText = std::string(Tr("Tom delivering... ")) + std::to_string(30 - w) + "s";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                if (savedTomX != 0 && savedTomY != 0) {
                    AddLog(instanceId, Tr("Tom delivered! Collecting items..."), ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
                    bot.statusText = Tr("Collecting Items...");
                    AdbTap(instanceId, savedTomX, savedTomY);
                    std::this_thread::sleep_for(std::chrono::milliseconds(2500)); 
                }
                else {
                    AddLog(instanceId, Tr("Saved location missing, searching crate to collect..."), ImVec4(1, 0.5f, 0, 1));
                    cv::Mat retScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                    MatchResult retTom = FindImage(retScreen, "templates\\tom_crate.png", 0.75f, false);
                    if (retTom.found) {
                        AdbTap(instanceId, retTom.x, retTom.y);
                        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
                    }
                }

                if (bot.accounts[i].tomStartTime == 0) {
                    bot.accounts[i].tomStartTime = now;
                }

                bot.accounts[i].nextTomTime = now + 7260; // 2HRS AND 1MIN
                SaveConfig();

                AddLog(instanceId, Tr("Auto Tom cycle complete! He will wake up in 2 hours."), ImVec4(0, 1, 0, 1));
                return true;
                };

            bot.statusText = Tr("Checking Auto Tom...");
            TryAutoTom(); 
            // PLANTING LAMBDA.
            auto TryPlant = [&]() -> bool {
                bool atLeastOnePlantDone = false;
                int tempAnchorX = -1;
                int tempAnchorY = -1;

                std::string currentSeedTemplate = w_templatePath;
                float currentSeedThresh = g_Thresholds.wheatThreshold;

                if (bot.testCropMode == 1) { currentSeedTemplate = c_templatePath; currentSeedThresh = g_Thresholds.cornThreshold; }
                else if (bot.testCropMode == 2) { currentSeedTemplate = carrot_templatePath; currentSeedThresh = g_Thresholds.carrotThreshold; }
                else if (bot.testCropMode == 3) { currentSeedTemplate = soybean_templatePath; currentSeedThresh = g_Thresholds.soybeanThreshold; }
                else if (bot.testCropMode == 4) { currentSeedTemplate = sugarcane_templatePath; currentSeedThresh = g_Thresholds.sugarcaneThreshold; }

                for (int plantAttempt = 1; plantAttempt <= 3; plantAttempt++) {
                    if (!bot.isRunning) return atLeastOnePlantDone;

                    cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);

                    // --- LEVEL UP CONTROL.
                    MatchResult lvlRes = FindImage(screen, levelup_templatePath, g_Thresholds.levelUpThreshold, false);
                    if (lvlRes.found) {
                        AddLog(instanceId, Tr("Account Leveled Up! Claiming rewards..."), ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                        bot.accounts[i].level += 1;
                        SaveConfig();
                        AddLog(instanceId, std::string(Tr("Level updated to: ")) + std::to_string(bot.accounts[i].level), ImVec4(0, 1, 0, 1));

                        MatchResult contRes = FindImage(screen, levelup_continue_templatePath, g_Thresholds.levelUpThreshold, false);
                        if (contRes.found) AdbTap(instanceId, contRes.x, contRes.y);
                        else AdbTap(instanceId, screen.cols / 2, screen.rows - 50);

                        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
                        plantAttempt--;
                        continue;
                    }

                    // SEARCH FOR NEON PINK FIELDS.
                    bot.statusText = Tr("Scanning Fields...");
                    bool isRecheck = (plantAttempt > 1);
                    std::vector<MatchResult> allFields = FindEmptyFields(screen, isRecheck);

                    if (!allFields.empty()) {
                        tempAnchorX = allFields[0].x;
                        tempAnchorY = allFields[0].y;

                        AddLog(instanceId, std::string(Tr("Neon pink fields detected! Opening seed menu (Swipe ")) + std::to_string(plantAttempt) + "/3)...", ImVec4(1.0f, 0.0f, 0.6f, 1.0f));

						// TAP THE FIRST FIELD TO OPEN SEED MENU
                        AdbTap(instanceId, tempAnchorX, tempAnchorY);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1200));

                        screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                        MatchResult seedRes = FindImage(screen, currentSeedTemplate, currentSeedThresh, false);

                        if (seedRes.found) {
                            AddLog(instanceId, Tr("Seed Found. Executing dense grid..."), ImVec4(0, 1, 0, 1));

                            ExecuteDenseGridGesture(instanceId, seedRes.x, seedRes.y, allFields);
                            std::this_thread::sleep_for(std::chrono::milliseconds(g_Intervals.afterPlantWait));

                            // =========================================================================
                            // OUT OF SEEDS CONTROL (I GUESS THIS GOT DIFFERENT WARNING IN NEW VERSION, CUZ YOU CAN BUY WITH COINS THIS TIME.)
                            // =========================================================================
                            cv::Mat verifyScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                            MatchResult crossRes = FindImage(verifyScreen, cross_templatePath, g_Thresholds.crossThreshold, false);

                            if (crossRes.found) {
                                AddLog(instanceId, Tr("OUT OF SEEDS! Diamond pop-up detected. Skipping plant phase."), ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                                AdbTap(instanceId, crossRes.x, crossRes.y);
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                outOfSeeds = true; 
								return false; // CANCEL PLANTING IF OUT OF SEEDS.
                            }

                            atLeastOnePlantDone = true;
                        }
                        else {
                            AddLog(instanceId, Tr("Seed menu NOT opened or seed missing. Breaking plant loop."), ImVec4(1, 0.5f, 0, 1));
                            MatchResult crossRes = FindImage(screen, cross_templatePath, g_Thresholds.crossThreshold, false);
                            if (crossRes.found) AdbTap(instanceId, crossRes.x, crossRes.y);
                            break;
                        }
                    }
                    else {
                        if (plantAttempt > 1) {
                            AddLog(instanceId, Tr("All visible fields are planted successfully."), ImVec4(0, 1, 0, 1));
                        }
                        break;
                    }
                }

                if (atLeastOnePlantDone) {
                    bot.accounts[i].anchorX = tempAnchorX;
                    bot.accounts[i].anchorY = tempAnchorY;
                    AddLog(instanceId, Tr("Field position mapped and saved!"), ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                    return true;
                }

                return false;
                };

            bot.statusText = Tr("Checking Harvest...");
            int harvestStatus = TryHarvest();

            bool emergencySaleExecuted = false;

            // =========================================================================
            // SILO FULL CONTROL
            // =========================================================================
            if (harvestStatus == 2) {
                AddLog(instanceId, Tr("SILO FULL! Executing Emergency Protocol (Plant -> Sell -> Harvest)..."), ImVec4(1.0f, 0.5f, 0.0f, 1.0f));

                bot.statusText = Tr("Emergency Plant...");
                TryPlant(); // PLANT EMPTY FIELDS AGAIN SO BOT DOESNT SELL EVERYTHING AND END UP WITH 0 SEEDS.

                bot.statusText = Tr("Emergency Sales...");
				RunSalesCycle(instanceId, i, true); // GO TO MARKET AND SELL EVERYTHING TO FREE UP SILO SPACE (WE ALREADY FILLED THE EMPTY FIELDS, SO THIS MEANS THERES NO EMPTY FIELD. ALL PLANTED. WE CAN SELL ALL SEEDS.)

                AddLog(instanceId, Tr("Space freed. Resuming harvest after emergency actions..."), ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
                bot.statusText = Tr("Resuming Harvest...");
                harvestStatus = TryHarvest(); // COLLECT HARVESTS
            }

            // =========================================================================
            // PLANTING
            // =========================================================================
            bot.statusText = Tr("Checking Plant...");
            bool plantDone = false;
            if (!outOfSeeds) {
				plantDone = TryPlant(); // FILL EMPTY FIELDS IF THERES SKIPPED
            }

            // CHECK IF THERES ANY SKIPPED FIELDS
            if (!plantDone && !outOfSeeds) {
                AddLog(instanceId, Tr("Plant incomplete or failed. Suspecting false positive or stuck menu. Retrying..."), ImVec4(1, 0.5f, 0, 1));
                cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                MatchResult crossRes = FindImage(screen, cross_templatePath, g_Thresholds.crossThreshold, false);
                if (crossRes.found) AdbTap(instanceId, crossRes.x, crossRes.y);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                bot.statusText = Tr("Retrying Harvest...");
                int retryHarvestStatus = TryHarvest();

                if (retryHarvestStatus == 2) {
                    bot.statusText = Tr("Emergency Sales...");
                    RunSalesCycle(instanceId, i, true);
                    retryHarvestStatus = TryHarvest();
                }

                bot.statusText = Tr("Retrying Plant...");
                if (!outOfSeeds) plantDone = TryPlant();

                if (!plantDone && retryHarvestStatus == 0) {
                    AddLog(instanceId, Tr("All retries failed. Moving on."), ImVec4(1, 0.2f, 0.2f, 1));
                }
            }

            bot.accounts[i].lastPlantTime = std::chrono::steady_clock::now();
            bot.accounts[i].isFirstRun = false;

            // =========================================================================
            // FİNAL SELL, WE HAVE TO SELL BECAUSE WE ARE HARVESTING AGAIN.
            // =========================================================================
            if (bot.enableRandomSaleCycle) {
                if (bot.accounts[i].targetCyclesBeforeSale <= 0) {
                    bot.accounts[i].targetCyclesBeforeSale = (rand() % 3) + 1;
                }

                bot.accounts[i].currentCyclesWithoutSale++;

                if (bot.accounts[i].currentCyclesWithoutSale >= bot.accounts[i].targetCyclesBeforeSale) {
                    AddLog(instanceId, std::string(Tr("Random Sales Triggered! (")) + std::to_string(bot.accounts[i].currentCyclesWithoutSale) + "/" + std::to_string(bot.accounts[i].targetCyclesBeforeSale) + ")", ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
                    bot.statusText = Tr("Checking Sales...");
                    RunSalesCycle(instanceId, i, false);

                    bot.accounts[i].currentCyclesWithoutSale = 0;
                    bot.accounts[i].targetCyclesBeforeSale = (rand() % 3) + 1;
                }
                else {
                    AddLog(instanceId, std::string(Tr("Skipping Sales to mimic human behavior... (")) + std::to_string(bot.accounts[i].currentCyclesWithoutSale) + "/" + std::to_string(bot.accounts[i].targetCyclesBeforeSale) + ")", ImVec4(0.5f, 0.7f, 0.5f, 1.0f));
                    bot.statusText = Tr("Skipped Sales (Random)");
                }
            }
            else {
                bot.statusText = Tr("Checking Sales...");
                RunSalesCycle(instanceId, i, false);
            }

            AddLog(instanceId, Tr("Account Cycle Done. Moving to next..."), ImVec4(0.5f, 0.5f, 0.5f, 1));
            std::this_thread::sleep_for(std::chrono::milliseconds(g_Intervals.nextAccountWait));
        }

        SendRotationReport(instanceId);
        if (bot.enableJanitor) {
            bot.currentJanitorCycles++;

            if (bot.currentJanitorCycles >= bot.janitorLimit) {
                bot.statusText = "JANITOR: DEEP CLEANING RAM...";
                AddLog(instanceId, "[JANITOR] Cycle limit (" + std::to_string(bot.janitorLimit) + ") reached! Halting operations for Deep Clean...", ImVec4(0, 1, 1, 1));

                // CLOSE THE GAME 
                RunAdbCommand(instanceId, "shell am force-stop com.supercell.hayday");
                std::this_thread::sleep_for(std::chrono::seconds(2));

                // 2. CLOSE EMULATOR
                AddLog(instanceId, "[JANITOR] Shutting down emulator to flush Memory Leaks...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));

                // --- LDPLAYER & MEMU CHECK ---
                if (bot.emulatorType == 1) { // LDPLAYER
                    RunCmdHidden("cmd.exe /c \"\"" + kLDConsolePath + "\" quit --index " + std::to_string(bot.emuIndex) + "\"");
                }
                else { // MEMU
                    RunCmdHidden("cmd.exe /c \"\"" + kMEmuConsolePath + "\" stop " + bot.vmName + "\"");
                }

				// 10 SECONDS WAIT FOR EMULATOR TO PROPERLY SHUT DOWN AND FLUSH RAM (BECAUSE JUST KILLING THE PROCESS DOESNT FLUSH RAM, YOU HAVE TO PROPERLY SHUT IT DOWN)
                bot.statusText = "JANITOR: FLUSHING WINDOWS RAM...";
                std::this_thread::sleep_for(std::chrono::seconds(10));

                // 3. FRESH START THE EMULATOR
                AddLog(instanceId, "[JANITOR] Booting up a fresh emulator instance...", ImVec4(0, 1, 0, 1));

                // --- LDPLAYER & MEMU CONTROL ---
                if (bot.emulatorType == 1) { // LDPLAYER
                    RunCmdHidden("cmd.exe /c \"\"" + kLDConsolePath + "\" launch --index " + std::to_string(bot.emuIndex) + "\"");
                }
                else { // MEMU
                    RunCmdHidden("cmd.exe /c \"\"" + kMEmuConsolePath + "\" start " + bot.vmName + "\"");
                }
                //WAIT OR ANDROID TO WAKE UP PROPERLY.
                bot.statusText = "JANITOR: BOOTING ANDROID OS...";
                for (int w = 0; w < 35; w++) {
                    if (!bot.isRunning) break;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                // 4. START MINITOUCH BECAUSE WE RESTARTED THE AGENT AND NEED TO WAKE UP AGAIN.
                if (bot.isRunning) {
                    bot.statusText = "JANITOR: INJECTING AGENTS...";
                    AddLog(instanceId, "[JANITOR] Re-injecting Minitouch input agent...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
                    StartMinitouchStealth(instanceId);
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }

                bot.currentJanitorCycles = 0; //CLEANING IS DONE,RESET TTIMER.
                AddLog(instanceId, "[JANITOR] System completely refreshed! Resuming normal operations.", ImVec4(0, 1, 0, 1));
            }
            else {
                AddLog(instanceId, "[JANITOR] Cycles until next deep clean: " + std::to_string(bot.janitorLimit - bot.currentJanitorCycles), ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            }
        }
        if (!isSingleAccountMode) {
            bot.statusText = Tr("Cycle End Cleanup...");
            AddLog(instanceId, Tr("Performing deep system cleanup to prevent Game Not Responding Error..."), ImVec4(0.8f, 0.4f, 1.0f, 1.0f));

            RunAdbCommand(instanceId, "shell am force-stop com.supercell.hayday");
            RunAdbCommand(instanceId, "shell am kill-all");
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        else {
            bot.statusText = Tr("Cycle Done");
            AddLog(instanceId, Tr("Single Account Cycle Done. Waiting for crops..."), ImVec4(0.5f, 0.5f, 0.5f, 1));
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }

    }
}


// ==============================================================================
// FUZZY MATCHER FOR TESSERACT OCR TO READ ITEMS COUNTS IN BARN (FUZZY MATCH)
// ==============================================================================
float GetSimilarityScore(const std::string& s1, const std::string& s2) {
    if (s1.empty() || s2.empty()) return 0.0f;

    std::string str1 = s1; std::string str2 = s2;

	// 1. TRANSFORM TO LOWERCASE
    std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
    std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);

    // =========================================================
	// 2. OCR CHARACTER NORMALIZATION (TO HANDLE COMMON OCR MISTAKES) 
    // =========================================================
    auto NormalizeOCR = [](std::string& s) {
        for (char& c : s) {
            if (c == '0') c = 'o';                               // 0 AND o, O
            else if (c == '1' || c == 'l' || c == '!' || c == '|') c = 'i'; // 1, L, i, !, |
            else if (c == '5') c = 's';                          // 5 AND s, S
            else if (c == '2') c = 'z';                          // 2 AND z, Z
            else if (c == '8') c = 'b';                          // 8 AND b, B
            else if (c == 'q') c = 'g';                          // q AND g
        }
        };


    NormalizeOCR(str1);
    NormalizeOCR(str2);

	// 3. CLASSIC FUZZY MATCHING USING LEVENSHTEIN DISTANCE
    int len1 = (int)str1.size();
    int len2 = (int)str2.size();
    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

    for (int i = 0; i <= len1; ++i) d[i][0] = i;
    for (int i = 0; i <= len2; ++i) d[0][i] = i;

    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (str1[i - 1] == str2[j - 1]) ? 0 : 1;
            int a = d[i - 1][j] + 1;
            int b = d[i][j - 1] + 1;
            int c = d[i - 1][j - 1] + cost;
            d[i][j] = std::min(a, std::min(b, c));
        }
    }

    int maxLen = std::max(len1, len2);
    int distance = d[len1][len2];
    return (1.0f - ((float)distance / (float)maxLen)) * 100.0f;
}

// ==============================================================================
//RADAR AND CLICK STUFF AND BTW THIS IS NOT DONE SO I LITERALLY QUITTED MID-DEVELOPING SO IF YOUR BUILDING YOUR OWN BOT JUST DELETE THESE AND FORGET ABOUT IT
// HASTA HISLER KOPTU İPLER HEMDE KENDİLİĞİNDEN, BİLMEM NASIL İLERLEDİ BU, ACI KEDER DERKEN STRES OLDUM KENDİLİĞİMDEN VE HİÇ KİMSEYE İYİ GELMEDİ BU.
// ==============================================================================

bool NXRTH_Radar(int instanceId, std::string targetName, int mode, bool skipMenuOpen) {
    BotInstance& bot = g_Bots[instanceId];

    cv::Rect roi;
    if (mode == 0) roi = cv::Rect(171, 131, 180, 265);
    else roi = cv::Rect(170, 251, 194, 140);

    int whiteMin = 200;
    AddLog(instanceId, "RADAR: Initializing System...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));


    if (!skipMenuOpen) {
        ForceCloseAllMenus(instanceId);
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        cv::Mat scr = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
        MatchResult fRes = FindImage(scr, "templates\\friends.png", 0.65f, false, 1.0f, false);
        if (fRes.found) {
            AdbTap(instanceId, fRes.x, fRes.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        scr = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
        MatchResult bookRes = FindImage(scr, "templates\\friends_book.png", 0.70f, false, 1.0f, false);
        if (bookRes.found) {
            AdbTap(instanceId, bookRes.x, bookRes.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
    }
    else {
        AddLog(instanceId, "RADAR: Skipping menu open, already inside Friend Book.", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    }

    if (mode == 0) {
        AddLog(instanceId, "RADAR: Mode 0 Active. Switching to 'In-game friends' tab...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
        AdbTap(instanceId, 280, 117);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        AdbTap(instanceId, 280, 117); 
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
    else {
        AddLog(instanceId, "RADAR: Mode 1 Active. Scanning 'Requests' tab...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
    }

    AddLog(instanceId, "RADAR: Scanning for -> [" + targetName + "]...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));

    for (int scanTry = 1; scanTry <= 10; scanTry++) {
        if (!bot.isRunning) return false;

        cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
        if (screen.empty() || roi.x + roi.width > screen.cols || roi.y + roi.height > screen.rows) continue;

        cv::Mat crop = screen(roi);
        cv::Mat resizedImage, bw;

        cv::resize(crop, resizedImage, cv::Size(), 3.0, 3.0, cv::INTER_CUBIC);
        cv::inRange(resizedImage, cv::Scalar(whiteMin, whiteMin, whiteMin), cv::Scalar(255, 255, 255), bw);
        cv::bitwise_not(bw, bw);
        cv::copyMakeBorder(bw, bw, 10, 10, 10, 10, cv::BORDER_CONSTANT, cv::Scalar(255));

        char exePathBuf[MAX_PATH];
        GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
        std::string exePath = std::string(exePathBuf);
        std::string tessDataPath = exePath.substr(0, exePath.find_last_of("\\/")) + "\\tessdata";

        tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
        if (api->Init(tessDataPath.c_str(), "eng")) {
            delete api; return false;
        }

        api->SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 ");
        api->SetPageSegMode(tesseract::PSM_SPARSE_TEXT);
        api->SetImage((uchar*)bw.data, bw.cols, bw.rows, 1, bw.step);
        api->Recognize(0);

        tesseract::ResultIterator* ri = api->GetIterator();
        tesseract::PageIteratorLevel level = tesseract::RIL_TEXTLINE;

        bool found = false;

        if (ri != 0) {
            do {
                char* word = ri->GetUTF8Text(level);
                if (word != nullptr) {
                    std::string readName(word);
                    while (!readName.empty() && (readName.back() == '\r' || readName.back() == '\n' || readName.back() == ' ')) readName.pop_back();

                    int x1, y1, x2, y2;
                    ri->BoundingBox(level, &x1, &y1, &x2, &y2);

                    float score = GetSimilarityScore(readName, targetName);

                    
                    if (score >= 60.0f) {
                        int realX1 = x1 / 3;
                        int realY1 = y1 / 3;
                        int realX2 = x2 / 3;
                        int realY2 = y2 / 3;

                        int clickX = roi.x + realX1 + ((realX2 - realX1) / 2);
                        int clickY = roi.y + realY1 + ((realY2 - realY1) / 2);

                        if (mode == 0) {
							// DOUBLE CLICK TO VISIT FARM.
                            AddLog(instanceId, "RADAR: Target found! Double clicking: [" + readName + "]", ImVec4(0, 1, 0, 1));
                            AdbTap(instanceId, clickX, clickY);
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            AdbTap(instanceId, clickX, clickY);
                            found = true;
                        }
                        else if (mode == 1) {
                            // ACCEPT FRIEND REQUEST
                            AddLog(instanceId, "RADAR: Request found! Scanning for green tick on the same row...", ImVec4(0.8f, 0.8f, 0.2f, 1.0f));

                            std::vector<MatchResult> greenTicks = FindAllImages(screen, "templates\\acceptfq.png", 0.70f, 10, false);
                            bool tickFound = false;

                            for (auto& tick : greenTicks) {
                                if (std::abs(tick.y - clickY) <= 40) {
                                    AddLog(instanceId, "RADAR: Green tick matched with name! Accepting...", ImVec4(0, 1, 0, 1));
                                    AdbTap(instanceId, tick.x, tick.y);
									std::this_thread::sleep_for(std::chrono::milliseconds(100));
									AdbTap(instanceId, tick.x, tick.y); // ACCEPT BY DOUBLE CLICKING GREEN TICK.
                                    tickFound = true;
                                    break;
                                }
                            }

                            if (!tickFound) {
                                AddLog(instanceId, "RADAR ERROR: Name found but acceptfq.png missing on that row!", ImVec4(1, 0.3f, 0.3f, 1.0f));
                                AdbTap(instanceId, clickX + 230, clickY); // Fallback: CLICK TO THE POSITION.
                            }
                            found = true;
                        }

                        delete[] word;
                        break;
                    }
                    delete[] word;
                }
            } while (ri->Next(level));
        }
        delete ri;
        api->End();
        delete api;

        if (found) {
            std::this_thread::sleep_for(std::chrono::seconds(2)); 
            return true;
        }

        AddLog(instanceId, "RADAR: Target not found here. Scrolling 50 pixels down...", ImVec4(1, 1, 0, 1));
        RunAdbCommand(instanceId, "shell input swipe 300 350 300 300 1000");
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    AddLog(instanceId, "RADAR ERROR: Target [" + targetName + "] not found!", ImVec4(1, 0.2f, 0.2f, 1));
    return false;
}

void RunStorageMaster(int instanceId) {
    BotInstance& bot = g_Bots[instanceId];
    AddLog(instanceId, "Storage Master is online. Waiting for signals...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
    if (strcmp(bot.inputDevice, "/dev/input/event1") == 0) {
        if (AutoDetectTouchDevice(instanceId)) {
            SaveConfig();
        }
    }
    bool friendMenuOpen = false;

    while (bot.isRunning) {
        EmulatorCrashWatchdog(instanceId);
        if (g_TransferRequest.isPending && !g_TransferRequest.storageReady) {


            if (g_TransferRequest.needFriendship && g_TransferRequest.friendRequestSent) {
                bot.statusText = "ACCEPTING FRIEND REQUEST";
                AddLog(instanceId, "Friend request signal received. Accepting...", ImVec4(0.8f, 0.8f, 0.2f, 1.0f));

                if (NXRTH_Radar(instanceId, g_TransferRequest.sellerFarmName, 1, false)) {
                    AddLog(instanceId, "Friend request accepted! Moving directly to infiltration...", ImVec4(0, 1, 0, 1));

                
                    g_TransferRequest.needFriendship = false;
                    friendMenuOpen = true;

                    std::this_thread::sleep_for(std::chrono::seconds(4)); 
                }
                else {
                    ForceCloseAllMenus(instanceId);
                    g_TransferRequest.isPending = false;
                    friendMenuOpen = false;
                }
                continue;
            }

           
            if (!g_TransferRequest.needFriendship) {
                AddLog(instanceId, "SIGNAL RECEIVED! Target: [" + g_TransferRequest.sellerFarmName + "]", ImVec4(1, 1, 0, 1));
                bot.statusText = "INFILTRATING: " + g_TransferRequest.sellerFarmName;

                if (NXRTH_Radar(instanceId, g_TransferRequest.sellerFarmName, 0, friendMenuOpen)) {
                    friendMenuOpen = false; 
                    std::this_thread::sleep_for(std::chrono::seconds(4)); 

              
                    bool shopVerified = false;
                    for (int retry = 0; retry < 3; retry++) {
                        cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                        MatchResult shopRes = FindImage(screen, shop_templatePath, g_Thresholds.shopThreshold);

                        if (shopRes.found) {
                            AdbTap(instanceId, shopRes.x, shopRes.y);
                            std::this_thread::sleep_for(std::chrono::milliseconds(2500)); 

                            
                            cv::Mat verifyScr = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                            if (FindImage(verifyScr, cross_templatePath, g_Thresholds.crossThreshold, false, 1.0f, false).found ||
                                FindImage(verifyScr, crate_templatePath, g_Thresholds.crateThreshold, false, 1.0f, false).found) {
                                shopVerified = true;
                                break;
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }

                    if (shopVerified) {
                        AddLog(instanceId, "Entered the shop. Initiating Heist Session...", ImVec4(0, 1, 1, 1));

                    
                        
                        while (bot.isRunning && g_TransferRequest.isPending) {
                            g_TransferRequest.storageReady = true; 

                            
                            int waitTimeout = 0;
                            while (!g_TransferRequest.itemListed && waitTimeout < 400) {
                                if (!bot.isRunning || !g_TransferRequest.isPending) break;
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                waitTimeout++;
                            }

                            if (g_TransferRequest.itemListed) {
                                AddLog(instanceId, "ITEM LISTED! Waiting 800ms for network sync...", ImVec4(1, 1, 0, 1));
                                std::this_thread::sleep_for(std::chrono::milliseconds(800));

                                int visitorX = g_TransferRequest.targetSlotX + 30; 
                                int visitorY = g_TransferRequest.targetSlotY;

                                AddLog(instanceId, "TARGET LOCKED! Applying +30 X Offset & Executing Snipe...", ImVec4(1, 0, 0, 1));

                                for (int spam = 0; spam < 15; spam++) {
                                    AdbTap(instanceId, visitorX, visitorY);
                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                }

                                AddLog(instanceId, "Heist Successful! Item secured.", ImVec4(0, 1, 0, 1));
                            }
                            else {
                                AddLog(instanceId, "Heist Failed! Seller didn't list item (Timeout).", ImVec4(1, 0.5f, 0, 1));
                            }

                            g_TransferRequest.transferComplete = true;

                           
                            int waitAcknowledge = 0;
                            while (g_TransferRequest.isPending && waitAcknowledge < 50) {
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                waitAcknowledge++;
                            }

                           
                            if (g_TransferRequest.hasMoreItems) {
                                AddLog(instanceId, "SIGNAL RECEIVED: Farm bot has more items! Waiting in shop...", ImVec4(1, 1, 0, 1));

                                int waitNextItem = 0;
                                while (!g_TransferRequest.isPending && waitNextItem < 450) {
                                    if (!bot.isRunning) break;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                    waitNextItem++;
                                }

                                if (g_TransferRequest.isPending) {
                                    continue; 
                                }
                                else {
                                    AddLog(instanceId, "ERROR: Waited 45s but Farm bot got stuck. Going home.", ImVec4(1, 0, 0, 1));
                                    break;
                                }
                            }
                            else {
                                AddLog(instanceId, "No more items to transfer. Finishing heist session.", ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                                break; 
                            }
                        } 

                        ForceCloseAllMenus(instanceId);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

                        cv::Mat homeScreen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
                        MatchResult homeRes = FindImage(homeScreen, "templates\\home.png", 0.70f);

                        if (homeRes.found) {
                            AdbTap(instanceId, homeRes.x, homeRes.y);
                            AddLog(instanceId, "Returning to Home Base...", ImVec4(0.8f, 0.4f, 1.0f, 1.0f));
                        }
                        else {
                            AddLog(instanceId, "WARNING: home.png not found! Using fallback coordinate.", ImVec4(1, 0.5f, 0.0f, 1.0f));
                            AdbTap(instanceId, 30, 450);
                        }

                        bot.statusText = "LISTENING FOR SIGNALS";
                    }
                    else {
                        AddLog(instanceId, "ERROR: Shop not found or didn't open on target farm!", ImVec4(1, 0, 0, 1));
                        g_TransferRequest.isPending = false;
                    }
                }
                else {
                    AddLog(instanceId, "ERROR: Target not found on Radar!", ImVec4(1, 0, 0, 1));
                    g_TransferRequest.isPending = false;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool SendFriendRequestToStorage(int instanceId, std::string targetTag) {
    AddLog(instanceId, "Sending friend request to Storage (" + targetTag + ")...", ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
    ForceCloseAllMenus(instanceId); 

  
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    BotInstance& bot = g_Bots[instanceId];
    bool friendsOpened = false;

    
    for (int k = 0; k < 3; k++) {
        cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
        MatchResult friendsRes = FindImage(screen, "templates\\friends.png", 0.65f, false, 1.0f, false);

        if (friendsRes.found) {
            AdbTap(instanceId, friendsRes.x, friendsRes.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            friendsOpened = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!friendsOpened) {
        AddLog(instanceId, "ERROR: friends.png NOT FOUND on screen!", ImVec4(1, 0, 0, 1));
        return false;
    }


    bool bookOpened = false;
    for (int k = 0; k < 3; k++) {
        cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
        MatchResult bookRes = FindImage(screen, "templates\\friends_book.png", 0.70f, false);

        if (bookRes.found) {
            AdbTap(instanceId, bookRes.x, bookRes.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            bookOpened = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!bookOpened) return false;

    AdbTap(instanceId, 210, 170); 
	RunAdbCommand(instanceId, "shell input keyevent 67");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    RunAdbCommand(instanceId, "shell input text '" + targetTag + "'");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));


	AdbTap(instanceId, 445,167 ); 
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

    cv::Mat screen = CaptureInstanceScreen(instanceId, kAdbPath, bot.adbSerial);
    MatchResult addRes = FindImage(screen, "templates\\addfriend.png", 0.75f, false);
    if (addRes.found) {
        AdbTap(instanceId, addRes.x, addRes.y);
        AddLog(instanceId, "Friend request sent successfully!", ImVec4(0, 1, 0, 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    ForceCloseAllMenus(instanceId); 
    return true;
}

extern int g_TargetTabToSelect;


std::string ExecuteMEmuCommandHidden(std::string args) {
    std::string memucPath = kAdbPath;
    size_t lastSlash = memucPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        memucPath = memucPath.substr(0, lastSlash + 1) + "memuc.exe";
    }

    std::string cmd = "cmd.exe /c \"\"" + memucPath + "\" " + args + "\"";
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 45000); 
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    
    CloseHandle(hWrite);

    std::string result = "";
    DWORD read;
    char buffer[256];

 
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, NULL) && read != 0) {
        buffer[read] = '\0';
        result += buffer;
    }
    CloseHandle(hRead);

    return result;
}

std::vector<int> GetExistingMEmuInstances() {
    std::vector<int> instances;
    std::string listOut = ExecuteMEmuCommandHidden("list");
    std::stringstream ss(listOut);
    std::string line;

    while (std::getline(ss, line)) {
        if (line.empty() || line.find("Step:") != std::string::npos) continue; 
        size_t commaPos = line.find(',');
        if (commaPos != std::string::npos) {
            try {
             
                int index = std::stoi(line.substr(0, commaPos));
                instances.push_back(index);
            }
            catch (...) {}
        }
    }
    return instances;
}
// ========================================================================
// THE EMULATOR FACTORY: 1-CLICK MEMU CREATOR
// ========================================================================
void RunEmulatorFactory(int requestedInstance) {
    AddLog(requestedInstance, "Forging a new MEmu instance in the background... Please wait.", ImVec4(1, 1, 0, 1));

 
    std::string createOut = ExecuteMEmuCommandHidden("create");

  
    std::string lowerOut = createOut;
    for (char& c : lowerOut) c = tolower(c);

  
    int actualVm = -1;
    size_t idxPos = lowerOut.find("index:");
    if (idxPos != std::string::npos) {
        try { actualVm = std::stoi(lowerOut.substr(idxPos + 6)); }
        catch (...) {}
    }

    if (actualVm == -1) {
        std::string errMsg = "Failed to parse VM index!\n\n[MEMU RAW OUTPUT]:\n" + createOut;
        MessageBoxA(NULL, errMsg.c_str(), "NXRTH - Fatal Error", MB_ICONERROR | MB_TOPMOST);
        g_Bots[requestedInstance].isCreatingEmulator = false;
        return;
    }

    int targetInstance = actualVm;
    bool didShift = false;

    if (targetInstance >= 6) {
        std::string limitMsg = "You created VM " + std::to_string(actualVm) + ", but NXRTH supports max 6 instances!";
        MessageBoxA(NULL, limitMsg.c_str(), "NXRTH - Limit Reached", MB_ICONWARNING | MB_TOPMOST);
        g_Bots[requestedInstance].isCreatingEmulator = false;
        return;
    }

    if (actualVm != requestedInstance) {
        didShift = true;
        g_TargetTabToSelect = targetInstance; 
    }

 
    AddLog(targetInstance, "VM created. Waiting for MEmu to unlock config files...", ImVec4(1, 1, 0, 1));
    std::this_thread::sleep_for(std::chrono::seconds(2));

    AddLog(targetInstance, "Applying NXRTH strict configurations (640x480, DPI 100, Root, DirectX)...", ImVec4(1, 1, 0, 1));
    std::string vmStr = std::to_string(actualVm);

    ExecuteMEmuCommandHidden("setconfig -i " + vmStr + " is_customed_resolution 1");
    ExecuteMEmuCommandHidden("setconfig -i " + vmStr + " resolution_width 640");
    ExecuteMEmuCommandHidden("setconfig -i " + vmStr + " resolution_height 480");
    ExecuteMEmuCommandHidden("setconfig -i " + vmStr + " vbox_dpi 100");
    ExecuteMEmuCommandHidden("setconfig -i " + vmStr + " root 1");
    ExecuteMEmuCommandHidden("setconfig -i " + vmStr + " vbox_graph_mode 1"); // 1 = DirectX, 0 = OpenGL


    int calculatedPort = 21503 + (actualVm * 10);
    strncpy(g_Bots[targetInstance].adbSerial, ("127.0.0.1:" + std::to_string(calculatedPort)).c_str(), 64);
    std::string vmNameStr = (actualVm == 0) ? "MEmu" : "MEmu_" + std::to_string(actualVm);
    strncpy(g_Bots[targetInstance].vmName, vmNameStr.c_str(), 64);


    AddLog(targetInstance, " Starting Emulator Engine...", ImVec4(0, 1, 0, 1));
    ExecuteMEmuCommandHidden("start -i " + vmStr);


    if (didShift) {
        std::string msg = " SMART SHIFT ALERT\n\nYou clicked 'Create' on Instance " + std::to_string(requestedInstance + 1) +
            ", but MEmu actually created VM " + std::to_string(actualVm) +
            ".\n\nDon't worry! NXRTH automatically linked it and moved you to the correct tab.";
        MessageBoxA(NULL, msg.c_str(), "NXRTH - Auto Linker", MB_ICONINFORMATION | MB_TOPMOST);
    }
    else {
        std::string msg = " MEmu created successfully!\n\nTarget: VM " + std::to_string(actualVm) +
            "\nConfig: 640x480, 100 DPI, Root, DirectX.\n\nThe emulator is booting up now.";
        MessageBoxA(NULL, msg.c_str(), "NXRTH - Emulator Factory", MB_ICONINFORMATION | MB_TOPMOST);
    }

    g_Bots[requestedInstance].isCreatingEmulator = false;
}

