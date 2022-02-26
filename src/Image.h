#pragma once

#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <array>

// Specifies numerous ways to handle out of bound pixels
enum BoundaryExtension
{
    // Replace the invalid pixels with zeros
    Zero,

    // Reflect the invalid pixels with respect to the main diagonal line
    Reflection,

    // Replicate the invalid pixels (symmetric padding)
    Replication
};

class Image
{
private:
    // The image data, stored as a 3D array in the format [row][column][channel]
    uint8_t ***data;

public:
    // The width of the image in pixels in the image
    const uint32_t width;
    // The height of the image in pixels in the image
    const uint32_t height;
    // The number of channels in the image
    const uint8_t channels;
    // The total number of pixels (width*height) in the image
    const uint32_t numPixels;

    // Creates a new image with the specified dimensions
    Image(const uint32_t _width, const uint32_t _height, const uint8_t _channels);
    // Copy constructor
    Image(const Image &other);
    // Reads and loads the image in raw format, row-by-row RGB interleaved, from the specified filename
    Image(const std::string &filename, const uint32_t _width, const uint32_t _height, const uint8_t _channels);
    // Frees all dynamically allocated memory resources
    ~Image();

    // Exports the image in raw format, row-by-row RGB interleaved, to the specified filename
    bool ExportRAW(const std::string &filename) const;
    // Reads and loads the image in raw format, row-by-row RGB interleaved, from the specified filename
    bool ImportRAW(const std::string &filename);

    // Determines if the given location is in a valid position in the image
    bool IsInBounds(const int32_t row, const int32_t column, const uint8_t channel = 0) const;
    // Retrieves the pixel value at the specified location; if out of bounds, will utilize the specified boundary extension method
    uint8_t GetPixelValue(const int32_t row, const int32_t column, const uint8_t channel = 0,
                          const BoundaryExtension &boundaryExtension = BoundaryExtension::Reflection) const;

    // Calculates the number of pixels for each intensity in the image
    std::array<uint32_t, 256> CalculateHistogram(const uint8_t channel = 0) const;
    // Calculates the cumulative number of pixels for each intensity in the image
    std::array<uint32_t, 256> CalculateCumulativeHistogram(const uint8_t channel = 0) const;
    // Calculates the cumulative density function of pixels for each intensity in the image
    std::array<double, 256> CalculateCumulativeProbabilityHistogram(const uint8_t channel = 0) const;

    // Retrieves the pixel value at the specified location; does not check for out of bounds
    uint8_t operator()(const uint32_t row, const uint32_t column, const uint8_t channel = 0) const;
    // Retrieves the pixel value at the specified location; does not check for out of bounds
    uint8_t &operator()(const uint32_t row, const uint32_t column, const uint8_t channel = 0);
};

#endif // IMAGE_H