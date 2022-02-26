#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <string>
#include "Image.h"

// Creates a new image with the specified dimensions
Image::Image(const uint32_t _width, const uint32_t _height, const uint8_t _channels)
    : width(_width), height(_height), channels(_channels), numPixels(_width * _height)
{
    // Allocate image data array
    data = new uint8_t **[height];
    for (uint32_t v = 0; v < height; v++)
    {
        data[v] = new uint8_t *[width];
        for (uint32_t u = 0; u < width; u++)
            data[v][u] = new uint8_t[channels];
    }
}

// Copy constructor
Image::Image(const Image &other)
    : width(other.width), height(other.height), channels(other.channels), numPixels(other.numPixels)
{
    // Allocate image data array
    data = new uint8_t **[height];
    for (uint32_t v = 0; v < height; v++)
    {
        data[v] = new uint8_t *[width];
        for (uint32_t u = 0; u < width; u++)
        {
            data[v][u] = new uint8_t[channels];
            for (uint8_t c = 0; c < channels; c++)
                data[v][u][c] = other.data[v][u][c];
        }
    }
}

// Reads and loads the image in raw format, row-by-row RGB interleaved, from the specified filename
Image::Image(const std::string &filename, const uint32_t _width, const uint32_t _height, const uint8_t _channels)
    : width(_width), height(_height), channels(_channels), numPixels(_width * _height)
{
    // Allocate image data array
    data = new uint8_t **[height];
    for (uint32_t v = 0; v < height; v++)
    {
        data[v] = new uint8_t *[width];
        for (uint32_t u = 0; u < width; u++)
            data[v][u] = new uint8_t[channels];
    }

    // Open the file
    std::ifstream inStream(filename, std::ios::binary);

    // Check if file opened successfully
    if (!inStream.is_open())
    {
        std::cout << "Cannot open file for reading: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    // Read from the file: row-by-row, RGB interleaved
    for (uint32_t v = 0; v < height; v++)
        for (uint32_t u = 0; u < width; u++)
            inStream.read((char *)data[v][u], channels);

    inStream.close();
}

// Frees all dynamically allocated memory resources
Image::~Image()
{
    // Free image data resources
    for (uint32_t v = 0; v < height; v++)
    {
        for (uint32_t u = 0; u < width; u++)
            delete[] data[v][u];

        delete[] data[v];
    }

    delete[] data;
}

// Exports the image in raw format, row-by-row RGB interleaved, to the specified filename
bool Image::ExportRAW(const std::string &filename) const
{
    // Open the file
    std::ofstream outStream(filename, std::ofstream::binary | std::ofstream::trunc);

    // Check if file opened successfully
    if (!outStream.is_open())
    {
        std::cout << "Cannot open file for writing: " << filename << std::endl;
        return false;
    }

    // Write to the file: row-by-row, RGB interleaved
    for (uint32_t v = 0; v < height; v++)
        for (uint32_t u = 0; u < width; u++)
            outStream.write((char *)data[v][u], channels);

    outStream.close();
    return true;
}

// Reads and loads the image in raw format, row-by-row RGB interleaved, from the specified filename
bool Image::ImportRAW(const std::string &filename)
{
    // Open the file
    std::ifstream inStream(filename, std::ios::binary);

    // Check if file opened successfully
    if (!inStream.is_open())
    {
        std::cout << "Cannot open file for reading: " << filename << std::endl;
        return false;
    }

    // Read from the file: row-by-row, RGB interleaved
    for (uint32_t v = 0; v < height; v++)
        for (uint32_t u = 0; u < width; u++)
            inStream.read((char *)data[v][u], channels);

    inStream.close();
    return true;
}

// Determines if the given location is in a valid position in the image
bool Image::IsInBounds(const int32_t row, const int32_t column, const uint8_t channel) const
{
    // True if the pixel is in a valid position in the image, false otherwise
    return row >= 0 &&
           row < static_cast<int32_t>(height) &&
           column >= 0 &&
           column < static_cast<int32_t>(width) &&
           channel < channels;
}

// Retrieves the pixel value at the specified location; if out of bounds, will utilize the specified boundary extension method
uint8_t Image::GetPixelValue(const int32_t row, const int32_t column, const uint8_t channel, const BoundaryExtension &boundaryExtension) const
{
    // If valid position, get the pixel directly
    if (IsInBounds(row, column, channel))
        return data[row][column][channel];
    // Otherwise, retrieve the pixel using the specified boundary extension method
    else
    {
        switch (boundaryExtension)
        {
        case BoundaryExtension::Replication:
        {
            // Compute the replicated/symmetrical coordinate.
            // If we look at a single row, it should look [ORIGINAL] [REVERSED] [ORIGINAL] [REVERSED] ...
            // where the first [ORIGINAL] is the the image and the rest are out of bound extensions
            // Note: There is probably a better more compact version, but I'm only one day from submission, so this'll do!

            // The final index after applying the replication algorithm
            int32_t u = column, v = row;

            // Whether the u or v is on a reversed cycle
            bool uReversed = false, vReversed = false;

            // The amount of extra pixels on either side, starting from 0; i.e. u=-1 gives uExtra=0, -2 gives 1, etc.
            uint32_t uExtra = 0, vExtra = 0;

            // If out of bounds from the left
            if (column < 0)
            {
                uExtra = std::abs(column) - 1;
                uReversed = (uExtra / width) % 2 == 1;

                // Compute the u index of the boundary extension
                if (uReversed)
                    u = width - 1 - uExtra % 3;
                else
                    u = uExtra % 3;
            }
            // If out of bounds from the right
            else if (column >= (int32_t)width)
            {
                uExtra = column - width;
                uReversed = (uExtra / width) % 2 == 0;

                // Compute the u index of the boundary extension
                if (uReversed)
                    u = width - 1 - uExtra % 3;
                else
                    u = uExtra % 3;
            }

            // If out of bounds from the top
            if (row < 0)
            {
                vExtra = std::abs(row) - 1;
                vReversed = (vExtra / height) % 2 == 1;

                // Compute the v index of the boundary extension
                if (vReversed)
                    v = height - 1 - vExtra % 3;
                else
                    v = vExtra % 3;
            }
            // If out of bounds from the bottom
            else if (row >= (int32_t)height)
            {
                vExtra = column - height;
                vReversed = (vExtra / height) % 2 == 0;

                // Compute the v index of the boundary extension
                if (vReversed)
                    v = height - 1 - vExtra % 3;
                else
                    v = vExtra % 3;
            }

            return data[row][column][channel];
        }

        case BoundaryExtension::Reflection:
        {
            int32_t u = column, v = row;
            if (u < 0)
                u = std::abs(u);
            if (u >= (int32_t)width)
                u = (int32_t)(2 * (width - 1) - u);
            if (v < 0)
                v = std::abs(v);
            if (v >= (int32_t)height)
                v = (int32_t)(2 * (height - 1) - v);

            return data[v][u][channel];
        }

        case BoundaryExtension::Zero:
        default:
            return 0;
        }
    }
}

// Calculates the number of pixels for each intensity in the image
std::array<uint32_t, 256> Image::CalculateHistogram(const uint8_t channel) const
{
    // Count the number of pixels for each intensity
    std::array<uint32_t, 256> histogram;
    histogram.fill(0);

    for (uint32_t v = 0; v < height; v++)
    {
        for (uint32_t u = 0; u < width; u++)
        {
            const uint8_t intensity = data[v][u][channel];
            histogram[intensity]++;
        }
    }

    return histogram;
}

// Calculates the cumulative number of pixels for each intensity in the image
std::array<uint32_t, 256> Image::CalculateCumulativeHistogram(const uint8_t channel) const
{
    // Calculate the cumulative number of pixels for each intensity
    const auto histogram = CalculateHistogram(channel);
    std::array<uint32_t, 256> cumulativeHistogram;
    cumulativeHistogram.fill(0);

    cumulativeHistogram[0] = histogram[0];
    for (uint32_t i = 1; i < 256; i++)
        cumulativeHistogram[i] = cumulativeHistogram[i - 1] + histogram[i];

    return cumulativeHistogram;
}

// Calculates the cumulative density function of pixels for each intensity in the image
std::array<double, 256> Image::CalculateCumulativeProbabilityHistogram(const uint8_t channel) const
{
    // Convert the cumulative number of pixels for each intensity into a cumulative density function (CDF)
    const auto cumulativeHistogram = CalculateCumulativeHistogram(channel);
    std::array<double, 256> cumulativeProbability;
    cumulativeProbability.fill(0.0);

    const double numPixelsDouble = (double)numPixels;
    for (uint32_t i = 0; i < 256; i++)
        cumulativeProbability[i] = cumulativeHistogram[i] / numPixelsDouble;

    return cumulativeProbability;
}

// Retrieves the pixel value at the specified location; does not check for out of bounds
uint8_t Image::operator()(const uint32_t row, const uint32_t column, const uint8_t channel) const
{
    return data[row][column][channel];
}

// Retrieves the pixel value at the specified location; does not check for out of bounds
uint8_t &Image::operator()(const uint32_t row, const uint32_t column, const uint8_t channel)
{
    return data[row][column][channel];
}