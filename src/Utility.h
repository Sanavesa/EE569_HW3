#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include "Image.h"

// Returns the intensity saturated to the range [0, 255]
uint8_t Saturate(const double intensity);

// Converts the given image coordinate to cartesian coordinates
std::pair<double, double> ImageToCartesianCoord(const Image &image, const double &x, const double &y);

// Converts the given cartesian coordinate to image coordinates
std::pair<double, double> CartesianToImageCoord(const Image &image, const double &x, const double &y);

#endif // UTILITY_H