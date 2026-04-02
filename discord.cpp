#include <chrono>
#include "Discord.h"
#include "bot_logic.h" 
#include "imgui.h" 
#include <iostream>
#include <fstream>
#include <thread>
#include <windows.h>
#include <winhttp.h>

#include "discord_rpc.h" 
#include <time.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Advapi32.lib")

extern char g_Username[64];

extern char g_WebhookURL[256];
extern bool g_EnableBarnWebhook;
extern char g_DiscordID[64];
extern int g_CurrentTab;


extern bool RunCmdHidden(const std::string& command);
extern void RunPremiumBot(int instanceId);
extern std::string GetRuntimeStr();
extern void AddLog(int instanceId, std::string message, ImVec4 color);
extern void StartMEmuAndGame(int instanceId);
extern cv::Mat CaptureInstanceScreen(int instanceId, const std::string& adbPath, const std::string& serial);

int g_LastTelegramUpdateId = 0;
static int64_t StartTime = time(0);

namespace Discord {

    void Initialize() {
        DiscordEventHandlers handlers;
        memset(&handlers, 0, sizeof(handlers));
        Discord_Initialize("1474459872719274046", &handlers, 1, NULL);
    }

    void Update(bool active) {
        static auto lastUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() < 2) {
            return;
        }
        lastUpdate = now;
        if (!active) {
            Discord_ClearPresence();
            return;
        }

        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));

        discordPresence.startTimestamp = StartTime;
        discordPresence.largeImageKey = "nxrth_logos";
        discordPresence.largeImageText = "NXRTH Premium";
        discordPresence.button1_label = "Discord Server";
        discordPresence.button1_url = "https://discord.gg/nxrth";

        switch (g_CurrentTab) {
        case 0: discordPresence.details = "📊 Checking Dashboard Tab"; break;
        case 1: discordPresence.details = "⚙️ Managing Bot Instances"; break;
        case 2: discordPresence.details = "☁️ Configuring Remote Control"; break;
        case 3: discordPresence.details = "🛠️ Adjusting Global Settings"; break;
        case 4: discordPresence.details = "📜 Reading System Logs"; break;
        case 5: discordPresence.details = "🖼️ Templates Tab"; break;
        default: discordPresence.details = "Idle"; break;
        }

        Discord_UpdatePresence(&discordPresence);
    }

    std::string SendTelegramAPI(const std::string& endpoint, const std::string& jsonPayload) {
        std::string responseStr = "";
        std::wstring wServer = L"api.telegram.org";
        std::wstring wPath = std::wstring(endpoint.begin(), endpoint.end());

        HINTERNET hSession = WinHttpOpen(L"NXRTH_BOT/3.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (hSession) {
            HINTERNET hConnect = WinHttpConnect(hSession, wServer.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
            if (hConnect) {
                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                if (hRequest) {
                    std::wstring headers = L"Content-Type: application/json\r\n";
                    BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), -1, (LPVOID)jsonPayload.c_str(), jsonPayload.length(), jsonPayload.length(), 0);
                    if (bResults) {
                        bResults = WinHttpReceiveResponse(hRequest, NULL);
                        if (bResults) {
                            DWORD dwSize = 0, dwDownloaded = 0;
                            do {
                                dwSize = 0;
                                WinHttpQueryDataAvailable(hRequest, &dwSize);
                                if (dwSize > 0) {
                                    char* pszOutBuffer = new char[dwSize + 1];
                                    WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);
                                    pszOutBuffer[dwDownloaded] = '\0';
                                    responseStr += pszOutBuffer;
                                    delete[] pszOutBuffer;
                                }
                            } while (dwSize > 0);
                        }
                    }
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
            }
            WinHttpCloseHandle(hSession);
        }
        return responseStr;
    }

    void SendWebhookMessage(std::string message) {
        if (g_EnableBarnWebhook && strlen(g_WebhookURL) > 5) {
            std::string safeMsg = message;
            size_t pos = 0;
            while ((pos = safeMsg.find("\n", pos)) != std::string::npos) { safeMsg.replace(pos, 1, "\\n"); pos += 2; }

            std::string jsonFile = "C:\\Users\\Public\\nxrth_wh_msg.json";
            std::ofstream out(jsonFile);
      
            out << "{\"username\":\"N X R T H  PREMIUM\", \"content\":\"" << safeMsg << "\"}";
            out.close();

            std::string cmd = "C:\\Windows\\System32\\curl.exe -m 10 -s -H \"Content-Type: application/json\" -X POST -d @\"" + jsonFile + "\" \"" + std::string(g_WebhookURL) + "\"";
            RunCmdHidden(cmd);
        }
    }

    void SendWebhookImage(std::string imagePath, std::string caption) {
        if (g_EnableBarnWebhook && strlen(g_WebhookURL) > 5) {
            std::string msgFile = "C:\\Users\\Public\\nxrth_msg.txt";
            std::ofstream out(msgFile, std::ios::binary);
            out << caption;
            out.close();
       
            std::string cmd = "C:\\Windows\\System32\\curl.exe -m 10 -s -H \"Content-Type: multipart/form-data\" -X POST -F \"username=N X R T H  PREMIUM\" -F \"file1=@" + imagePath + "\" -F \"content=<" + msgFile + "\" \"" + std::string(g_WebhookURL) + "\"";
            RunCmdHidden(cmd);
        }

    }
}