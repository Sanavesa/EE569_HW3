#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include "Image.h"
#include <opencv2/core.hpp>

// Returns the intensity saturated to the range [0, 255]
uint8_t Saturate(const double intensity);

// Converts the given image coordinate to cartesian coordinates
std::pair<double, double> ImageToCartesianCoord(const Image &image, const double &x, const double &y);

// Converts the given cartesian coordinate to image coordinates
std::pair<double, double> CartesianToImageCoord(const Image &image, const double &x, const double &y);

// Converts the given RGB image into an OpenCV Mat object
cv::Mat RGBImageToMat(const Image &image);

// Credit: https://stackoverflow.com/questions/15160889/how-can-i-make-an-unordered-set-of-pairs-of-integers-in-c
// Used to make a std::pair hashable for std::unordered_set
struct PairHash
{
    inline std::size_t operator()(const std::pair<size_t, size_t>& v) const
    {
        return v.first * 31 + v.second;
    }
};

#endif // UTILITY_H