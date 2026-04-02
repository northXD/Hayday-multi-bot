#pragma once
#include <string>
#include <curl/curl.h>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);

std::string sendGetRequest(const std::string& url);
