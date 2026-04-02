#pragma once
#include <string>

namespace Discord {

    void Initialize();
    void Update(bool active);


    void SendWebhookImage(std::string imagePath, std::string caption);
    void SendWebhookMessage(std::string message);
}