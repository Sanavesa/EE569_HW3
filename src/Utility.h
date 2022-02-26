#pragma once

#ifndef UTILITY_H
#define UTILITY_H

#include "Image.h"
#include <vector>

// The MBVQ quadrant used in Color Error Diffusion
enum MBVQType
{
    CMYW,
    MYGC,
    RGMY,
    KRGB,
    RGBM,
    CMGB
};

// The vertex type (color) used in classifying points in MBVQ Color Error Diffusion
enum VertexType : uint32_t
{
    // Colors in uint32 RGB format (A in RGBA left out as 0)
    // 0xRRGGBBAA
    Black = 0,
    Red = 0xFF000000,
    Green = 0x00FF0000,
    Blue = 0x0000FF00,
    White = 0xFFFFFF00,
    Cyan = 0x00FFFF00,
    Magenta = 0xFF00FF00,
    Yellow = 0xFFFF0000,
};


// Returns the intensity saturated to the range [0, 255]
uint8_t Saturate(const double intensity);

// Write the given array to the specified file where each line is the index followed by its value, separated by a space
bool WriteArrayToFile(const std::string &filename, const std::array<uint32_t, 256> &arr);

// Write the given array to the specified file where each line is the index followed by its value, separated by a space
bool WriteArrayToFile(const std::string &filename, const std::array<double, 256> &arr);

// Converts an image from RGB to Grayscale
Image RGB2Grayscale(const Image &image);

// Converts an image from RGB to CMY
Image RGB2CMY(const Image &image);

// Converts an image from CMY to RGB
Image CMY2RGB(const Image &image);

// Generates a random integer from the given range, both inclusive.
int32_t RandomRange(const int32_t min, const int32_t max);

// Calculates the magnitude (L2-norm), euclidean distance of the given x and y.
double Magnitude(const double x, const double y);

// Generate a bayer index matrix with the specified size. Size must be a multiple of 2.
std::vector<std::vector<double>> GenerateBayerMatrix(const uint32_t size);

// Compute the Peak-Signal-to-Noise-Ratio (PSNR) quality metric to assess performance of denoising algorithm
double PSNR(const Image &originalImage, const Image &filteredImage, const uint8_t channel = 0);

// Determines the MBVQ quadrant given a pixel color
MBVQType DetermineMBVQ(const uint8_t r, const uint8_t g, const uint8_t b);

// Determines the vertex in a MBVQ quadrant
VertexType DetermineVertex(const MBVQType mbvq, const uint8_t r, const uint8_t g, const uint8_t b);

#endif // UTILITY_H