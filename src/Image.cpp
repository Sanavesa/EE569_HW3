#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <string>
#include "Image.h"

// Creates a new image with the specified dimensions
Image::Image(const size_t _width, const size_t _height, const size_t _channels)
    : width(_width), height(_height), channels(_channels), numPixels(_width * _height)
{
    // Allocate image data array
    data = new uint8_t **[height];
    for (size_t v = 0; v < height; v++)
    {
        data[v] = new uint8_t *[width];
        for (size_t u = 0; u < width; u++)
            data[v][u] = new uint8_t[channels];
    }
}

// Copy constructor
Image::Image(const Image &other)
    : width(other.width), height(other.height), channels(other.channels), numPixels(other.numPixels)
{
    // Allocate image data array
    data = new uint8_t **[height];
    for (size_t v = 0; v < height; v++)
    {
        data[v] = new uint8_t *[width];
        for (size_t u = 0; u < width; u++)
        {
            data[v][u] = new uint8_t[channels];
            for (size_t c = 0; c < channels; c++)
                data[v][u][c] = other.data[v][u][c];
        }
    }
}

// Reads and loads the image in raw format, row-by-row RGB interleaved, from the specified filename
Image::Image(const std::string &filename, const size_t _width, const size_t _height, const size_t _channels)
    : width(_width), height(_height), channels(_channels), numPixels(_width * _height)
{
    // Allocate image data array
    data = new uint8_t **[height];
    for (size_t v = 0; v < height; v++)
    {
        data[v] = new uint8_t *[width];
        for (size_t u = 0; u < width; u++)
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
    for (size_t v = 0; v < height; v++)
        for (size_t u = 0; u < width; u++)
            inStream.read((char *)data[v][u], channels);

    inStream.close();
}

// Frees all dynamically allocated memory resources
Image::~Image()
{
    // Free image data resources
    for (size_t v = 0; v < height; v++)
    {
        for (size_t u = 0; u < width; u++)
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
    for (size_t v = 0; v < height; v++)
        for (size_t u = 0; u < width; u++)
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
    for (size_t v = 0; v < height; v++)
        for (size_t u = 0; u < width; u++)
            inStream.read((char *)data[v][u], channels);

    inStream.close();
    return true;
}

// Determines if the given location is in a valid position in the image
bool Image::IsInBounds(const int32_t row, const int32_t column, const size_t channel) const
{
    // True if the pixel is in a valid position in the image, false otherwise
    return row >= 0 &&
           row < static_cast<int32_t>(height) &&
           column >= 0 &&
           column < static_cast<int32_t>(width) &&
           channel < channels;
}

// Retrieves the pixel value at the specified location; if out of bounds, will utilize the specified boundary extension method
uint8_t Image::GetPixelValue(const int32_t row, const int32_t column, const size_t channel, const BoundaryExtension &boundaryExtension) const
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
            const int32_t w = static_cast<int32_t>(width);
            const int32_t h = static_cast<int32_t>(height);

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
                uReversed = (uExtra / w) % 2 == 1;

                // Compute the u index of the boundary extension
                if (uReversed)
                    u = w - 1 - uExtra % 3;
                else
                    u = uExtra % 3;
            }
            // If out of bounds from the right
            else if (column >= w)
            {
                uExtra = column - w;
                uReversed = (uExtra / w) % 2 == 0;

                // Compute the u index of the boundary extension
                if (uReversed)
                    u = w - 1 - uExtra % 3;
                else
                    u = uExtra % 3;
            }

            // If out of bounds from the top
            if (row < 0)
            {
                vExtra = std::abs(row) - 1;
                vReversed = (vExtra / h) % 2 == 1;

                // Compute the v index of the boundary extension
                if (vReversed)
                    v = h - 1 - vExtra % 3;
                else
                    v = vExtra % 3;
            }
            // If out of bounds from the bottom
            else if (row >= h)
            {
                vExtra = column - h;
                vReversed = (vExtra / h) % 2 == 0;

                // Compute the v index of the boundary extension
                if (vReversed)
                    v = h - 1 - vExtra % 3;
                else
                    v = vExtra % 3;
            }

            return data[v][u][channel];
        }

        case BoundaryExtension::Reflection:
        {
            const int32_t w = static_cast<int32_t>(width);
            const int32_t h = static_cast<int32_t>(height);
            int32_t u = column, v = row;
            if (u < 0)
                u = std::abs(u);
            if (u >= w)
                u = 2 * (w - 1) - u;
            if (v < 0)
                v = std::abs(v);
            if (v >= h)
                v = 2 * (h - 1) - v;

            return data[v][u][channel];
        }

        case BoundaryExtension::Zero:
        default:
            return 0;
        }
    }
}

// Retrieves the pixel value at the specified location; applies reflection padding for out of bounds
uint8_t Image::operator()(const size_t row, const size_t column, const size_t channel) const
{
    return data[row][column][channel];
}

// Retrieves the pixel value at the specified location; does not check for out of bounds
uint8_t &Image::operator()(const size_t row, const size_t column, const size_t channel)
{
    return data[row][column][channel];
}

// Sets the entire image across all channels to the specified value
void Image::Fill(const uint8_t value)
{
    for (size_t v = 0; v < height; v++)
        for (size_t u = 0; u < width; u++)
            for (size_t c = 0; c < channels; c++)
                data[v][u][c] = value;
}

// Copy the other image
void Image::Copy(const Image &other)
{
    for (size_t v = 0; v < height; v++)
        for (size_t u = 0; u < width; u++)
            for (size_t c = 0; c < channels; c++)
                data[v][u][c] = other.data[v][u][c];
}