#pragma once

#ifndef FILTER_H
#define FILTER_H

#include "Image.h"

// Filter: Dont care
#define F_DC 2
// Filter: center
#define F_M 3
// Filter: A
#define F_A 4
// Filter: B
#define F_B 5
// Filter: C
#define F_C 6

class Filter
{
public:
    // The filter data, stored as a 2D array in the format [v][u]
    int32_t **data;
    // The size of the filter in pixels
    const uint32_t size;

    // Creates a new filter with 0s with the specified filter size
    Filter(const uint32_t size);
    // Creates a filter from the given flatten array with the specified square size.
    Filter(const uint32_t size, const std::initializer_list<int32_t> values);
    // Copy constructor
    Filter(const Filter &other);
    // Frees all dynamically allocated memory resources
    ~Filter();

    // Print the contents of the filter to console
    void Print() const;

    // Applies the filter on the specified center pixel of the given image, returns true if matches, false otherwise
    bool Match01(const Image &image, const int32_t row, const int32_t column, const size_t channel, const BoundaryExtension &boundaryExtension) const;
    bool Match(const Image &image, const int32_t row, const int32_t column, const size_t channel = 0, const BoundaryExtension &boundaryExtension = BoundaryExtension::Zero) const;
};

#endif // FILTER_H