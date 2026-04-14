#pragma once
#include <string>
#include <vector>
#include <chrono> // Zaman ayarları için eklendi
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <windows.h>
//-------------------------

//------------------------- 
// --- PREMIUM GUI'DEN TAŞINAN BOT YAPILARI ---

struct InventoryData {
    int bolt = 0, tape = 0, plank = 0;
    int nail = 0, screw = 0, panel = 0;
    int deed = 0, mallet = 0, marker = 0, map = 0;
};
struct AccountSlot {
    std::string name = "Empty Slot";
    bool hasFile = false;
    std::string fileName = "";
    std::chrono::steady_clock::time_point lastPlantTime;
    std::chrono::steady_clock::time_point lastAdTime;
    int salesCycleCount = 0;
    bool isFirstRun = true;

    // --- NXRTH HAFIZA DEPOSU ---
    InventoryData currentInv;
    InventoryData previousInv;

    // YENİ EKLENEN: ONAYLANMIŞ ÇIPA (ANCHOR) KOORDİNATLARI
    int anchorX = -1;
    int anchorY = -1;
    std::string playerTag = "Unknown";
    std::string farmName = "Unknown";
    int coinAmount = 0;
    int diamondAmount = 0;
    int level= 0;
    // Tom
    bool autoTomEnabled = false;
    int tomRemainingHours = 0;
    long long tomStartTime = 0; // İlk tıklamanın zaman damgası (Saniye)
    long long nextTomTime = 0;  // Bir sonraki uyanış (Saniye)
    int tomCategory = 0;        // 0 = Barn, 1 = Silo
    std::string tomItemName = "";
    int targetCyclesBeforeSale = 0;
    int currentCyclesWithoutSale = 0;
    bool isFriendWithStorage = false; // Depo ile arkadaş mı?
    bool isShopFullStuck = false; // Dükkan tıkandı mı?
    std::chrono::steady_clock::time_point lastShopCheckTime;

};
struct BotInstance {
    int id;
    bool isActive = false;
    bool isRunning = false;
    
    int emulatorType = 0; // 0 = MEmu, 1 = LDPlayer, 2 = BlueStacks (Gelecek 
    int emuIndex = 0;
    
    char adbSerial[64] = "";
    char inputDevice[64] = "/dev/input/event1";
    char vmName[64] = "";

    std::string statusText = "IDLE";

    AccountSlot accounts[15];
    int currentAccountIndex = 0;

    int testCropMode = 0;
    int totalHarvest = 0;
    int totalSales = 0;
    bool enableShopRandomizer = true;
    int shopRandomizerMax = 8;
    bool enableRandomSaleCycle = false;

    bool isCreatingEmulator = false;
    bool useSingleMode = true;
    bool useMultiMode = false;
    bool useReviveMode = false;
    char reviveTemplatePath[MAX_PATH] = ""; 
    int reviveCheckInterval = 180; 
    int reviveFailCounter = 0;      
    
    bool enableJanitor = false;       
    int janitorLimit = 15;          
    int currentJanitorCycles = 0;
};


struct TemplateThresholds {
    float soilThreshold = 0.80f;
    float grownThreshold = 0.7f;
    float wheatThreshold = 0.80f;
    float sickleThreshold = 0.90f;
    float shopThreshold = 0.80f;
    float crateThreshold = 0.80f;
    float arrowsThreshold = 0.85f;
    float plusThreshold = 0.80f;
    float crossThreshold = 0.80f;
    float advertiseThreshold = 0.72f;
    float createSaleThreshold = 0.80f;
    float mailboxThreshold = 0.75f;
    float cornThreshold = 0.70f;
    float grownCornThreshold = 0.70f;
    float levelUpThreshold = 0.75f;


    float carrotThreshold = 0.75f;
    float grownCarrotThreshold = 0.70f;
    float soybeanThreshold = 0.75f;
    float grownSoybeanThreshold = 0.70f;
    float sugarcaneThreshold = 0.75f;
    float grownSugarcaneThreshold = 0.70f;
	float marketCloseCrossThreshold = 0.70f;
	float siloFullCrossThreshold = 0.70f;
};

struct IntervalSettings {
    int gameLoadWait = 15;
    int afterHarvestWait = 1500;
    int afterPlantWait = 1300;
    int shopEnterWait = 2000;
    int crateClickWait = 800;
    int nextAccountWait = 2000;
    int coinCollectWait = 700;
    int productSelectWait = 500;
    int createSaleWait = 1000;

};




extern BotInstance g_Bots[6];


struct MatchResult {
    bool found;
    int x;
    int y;
    double score;
};

cv::Mat CaptureInstanceScreen(int instanceId, const std::string& adbPath, const std::string& serial);
MatchResult FindImage(const cv::Mat& screen, const std::string& templatePath, float threshold = 0.70f, bool useGrayscale = false, float roiPercent = 1.0f, bool useMargins = true);
std::vector<MatchResult> FindAllImages(const cv::Mat& screen, const std::string& templatePath, float threshold = 0.80f, int minDist = 10, bool useMargins = true);
std::vector<MatchResult> FindGrownCrops(const cv::Mat& screen, int cropMode);
std::vector<MatchResult> FindEmptyFields(const cv::Mat& screen, bool isRecheck = false);


std::string EncryptXORHex(const std::string& text, const std::string& key = "ISpentDaysForThisApp");
std::string DecryptXORHex(const std::string& hexStr, const std::string& key = "ISpentDaysForThisApp");
