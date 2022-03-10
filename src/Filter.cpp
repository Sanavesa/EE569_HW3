#define _USE_MATH_DEFINES

#include "Filter.h"
#include "Utility.h"

#include <iostream>
#include <cmath>
#include <limits>

// Creates a new filter with the specified size
Filter::Filter(const uint32_t size) : size(size)
{
    // Allocate image data array
    // data is int32_t[size][size]
    data = new int32_t *[size];
    for (uint32_t v = 0; v < size; v++)
    {
        data[v] = new int32_t[size];
        for (uint32_t u = 0; u < size; u++)
            data[v][u] = 0;
    }
}

// Creates a filter from the given flatten array with the specified square size.
Filter::Filter(const uint32_t size, const std::initializer_list<int32_t> values) : size(size)
{
    // Allocate image data array
    // data is int32_t[3][3]
    data = new int32_t *[size];
    for (uint32_t v = 0, i = 0; v < size; v++)
        data[v] = new int32_t[size];

    uint32_t i = 0;
    for (const auto element : values)
    {
        data[i / size][i % size] = element;
        i++;
    }
}

// Copy constructor
Filter::Filter(const Filter &other) : size(other.size)
{
    // Allocate image data array
    // data is int32_t[size][size]
    data = new int32_t *[size];
    for (uint32_t v = 0; v < size; v++)
    {
        data[v] = new int32_t[size];
        for (uint32_t u = 0; u < size; u++)
            data[v][u] = other.data[v][u];
    }
}

// Frees all dynamically allocated memory resources
Filter::~Filter()
{
    // Free image data resources
    for (uint32_t v = 0; v < size; v++)
        delete[] data[v];

    delete[] data;
}

// Print the contents of the filter to console
void Filter::Print() const
{
    std::cout << "Filter (" << size << " x " << size << ")" << std::endl;
    for (uint32_t v = 0; v < size; v++)
    {
        for (uint32_t u = 0; u < size; u++)
            std::cout << data[v][u] << "\t";
        std::cout << std::endl;
    }
}

// Applies the filter on the specified center pixel of the given image, returns true if matches, false otherwise
bool Filter::Match01(const Image &image, const int32_t row, const int32_t column, const size_t channel, const BoundaryExtension &boundaryExtension) const
{
    const int32_t centerIndex = size / 2;

    for (int32_t dv = -centerIndex; dv <= centerIndex; dv++)
        for (int32_t du = -centerIndex; du <= centerIndex; du++)
        {
            const int32_t filterCase = data[centerIndex + dv][centerIndex + du];
            const uint8_t intensity = image.GetPixelValue(row + dv, column + du, channel, boundaryExtension); // [0, 255]

            switch(filterCase)
            {
                case 0:
                    if (intensity != 0)
                        return false;
                    break;
                case 1:
                    if (intensity != 255)
                        return false;
                    break;
                default:
                    std::cout << "Invalid filter case used: " << filterCase << std::endl;
                    break;
            }
        }

    return true;
}

// Applies the filter on the specified center pixel of the given image, returns true if matches, false otherwise
bool Filter::Match(const Image &image, const int32_t row, const int32_t column, const size_t channel, const BoundaryExtension &boundaryExtension) const
{
    const int32_t centerIndex = size / 2;
    const int32_t centerIntensity = image.GetPixelValue(row, column, channel, boundaryExtension); // [0, 255]

    // Used to compute if ABC constraint is met, if any
    uint32_t unionABCValue = 0;
    bool hasABC = false;

    for (int32_t dv = -centerIndex; dv <= centerIndex; dv++)
        for (int32_t du = -centerIndex; du <= centerIndex; du++)
        {
            const int32_t filterCase = data[centerIndex + dv][centerIndex + du];
            const uint8_t intensity = image.GetPixelValue(row + dv, column + du, channel, boundaryExtension); // [0, 255]

            switch(filterCase)
            {
                // Generic 0 case
                case 0:
                    if (intensity != 0)
                        return false;
                    break;
                // Generic 1 case
                case 1:
                    if (intensity != 255)
                        return false;
                    break;
                // Skip dont cares
                case F_DC:
                    break;
                // Special Case: M, must match center
                case F_M:
                    if (intensity != centerIntensity)
                        return false;
                    break;
                // Special Cases: ABC, one must be true atleast
                case F_A:
                case F_B:
                case F_C:
                    unionABCValue += intensity;
                    hasABC = true;
                    break;
                default:
                    std::cout << "Invalid filter case used: " << filterCase << std::endl;
                    break;
            }
        }

    // Ensure that A or B or C >= 1 is satisfied
    if (hasABC && unionABCValue == 0)
        return false;

    return true;
}