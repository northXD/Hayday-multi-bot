#pragma once
#include <string>
#include "bot_logic.h"

extern std::string g_StorageTag;

void RunPremiumBot(int instanceId);
void RunSalesCycle(int instanceId, int accountIndex, bool isEmergency = false);
void StartMEmuAndGame(int instanceId);


void ExecuteDenseGridGesture(int instanceId, int startX, int startY, const std::vector<MatchResult>& fields);

bool RunCmdHidden(const std::string& command);
void RunAdbCommand(int instanceId, std::string args);
void AdbTap(int instanceId, int x, int y);
void PerformTemplateTest(int instanceId, std::string templatePath, std::string testName, float threshold, bool useGrayscale);

void InjectImportantFiles(int instanceId); 
void StartMinitouchStealth(int instanceId);

void SaveAccountToSlot(int instanceId, int slotIndex);
void LoadAccountFromSlot(int instanceId, int slotIndex);
bool AutoDetectTouchDevice(int instanceId);
void SendRotationReport(int instanceId);


// TRANSFER 
extern int g_TransferThreshold;

void ForceCloseAllMenus(int instanceId);
bool SendFriendRequestToStorage(int instanceId, std::string targetTag);
bool NXRTH_Radar(int instanceId, std::string targetName, int mode, bool skipMenuOpen = false);

struct TransferRequest {
    bool isPending = false;          
    int senderInstanceId = -1;       
    int senderAccountId = -1;        
    std::string sellerFarmName = ""; 
    std::string sellerTag = "";      
    std::string itemType = "";       
    int itemAmount = 0;             

    int targetSlotX = 0;           
    int targetSlotY = 0;             
    bool storageReady = false;       
    bool itemListed = false;        
    bool transferComplete = false;  

    bool needFriendship = false;     
    bool friendRequestSent = false;  
    
    bool hasMoreItems = false;
};

extern TransferRequest g_TransferRequest;
void RunStorageMaster(int instanceId);

extern std::string kLDPlayerAdbPath;
extern std::string kLDConsolePath;

void EmulatorCrashWatchdog(int instanceId);
std::string GetUniversalAdbPath(int instanceId);