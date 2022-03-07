#include "Utility.h"
#include <cmath>
#include <iostream>
#include "Image.h"

// Returns the intensity saturated to the range [0, 255]
uint8_t Saturate(const double intensity)
{
    return static_cast<uint8_t>(std::clamp(std::round(intensity), 0.0, 255.0));
}

// Converts the given image coordinate to cartesian coordinates, [x,y] are in [0, w) and [0, h)
std::pair<double, double> ImageToCartesianCoord(const Image& image, const double& x, const double& y)
{
    const double imageWidth = static_cast<double>(image.width);
    const double imageHeight = static_cast<double>(image.height);

    if (x < 0 || x >= imageWidth || y < 0 || y >= imageHeight)
    {
        std::cout << "Invalid image coordinate for ImageToCartesianCoord(): " << x << ", " << y << " for size " << imageWidth << ", " << imageHeight << std::endl;
        exit(EXIT_FAILURE);
    }

    // Original one-based equations
    // x_k = k - 0.5
    // y_j = J + 0.5 - j

    // Modified zero-based equations
    // x_k = k + 0.5
    // y_j = J - 0.5 - j
    return std::make_pair(x + 0.5, imageHeight - 0.5 - y);
}

// Converts the given cartesian coordinate to image coordinates
std::pair<double, double> CartesianToImageCoord(const Image& image, const double& x, const double& y)
{
    // Original one-based equations
    // k = x_k + 0.5
    // j = J + 0.5 - y_j

    // Modified zero-based equations
    // k = x_k - 0.5
    // j = J - 0.5 - y_j
    const double imageHeight = static_cast<double>(image.height);
    return std::make_pair(x - 0.5, imageHeight - 0.5 - y);
}