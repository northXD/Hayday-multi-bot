#include "C:\Users\ebuxd\Downloads\imgui-1.92.5\imgui-1.92.5\examples\htpp1.h"
#include <curl/curl.h>
#include <string>
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string sendGetRequest(const std::string& url) {
    std::string response;
    CURL* curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            response = "Hata: " + std::string(curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    return response;
}
