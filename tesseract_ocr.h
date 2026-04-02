#pragma once
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

int ReadItemCountByAnchor(cv::Mat screen, const std::string& templatePath);

int ReadGameNumber(cv::Mat screen, cv::Rect roi, std::string debugName);
std::string ReadFarmName(cv::Mat screen, cv::Rect roi);