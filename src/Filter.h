#pragma once

#ifndef FILTER_H
#define FILTER_H

#include "Image.h"

class Filter
{
public:
    // The filter data, stored as a 2D array in the format [v][u]
    double **data;
    // The size of the filter in pixels
    const uint32_t size;

    // Creates a new filter with 0s with the specified filter size
    Filter(const uint32_t _size);
    // Copy constructor
    Filter(const Filter &other);
    // Frees all dynamically allocated memory resources
    ~Filter();

    // Print the contents of the filter to console
    void Print() const;

    // Create uniform filter with the specified filter size
    static Filter CreateUniform(const uint32_t size);
    // Create gaussian filter with the specified filter size and standard deviation
    static Filter CreateGaussian(const uint32_t size, const double stdev = 1.0);
    // Create 3x3 Sobel - Gradient X filter
    static Filter CreateSobelX();
    // Create 3x3 Sobel - Gradient Y filter
    static Filter CreateSobelY();

    // Create Error Diffusion Floyd-Steinberg filter
    static Filter CreateFloydSteinberg();
    // Create Error Diffusion JJN filter
    static Filter CreateJJN();
    // Create Error Diffusion Stucki filter
    static Filter CreateStucki();
    // Create Error Diffusion Alali filter
    static Filter CreateAlali();
    // Return the filter flipped horizontally
    static Filter FlipHorizontal(const Filter &filter);
    // Return the filter flipped vertically
    static Filter FlipVertical(const Filter &filter);
    // Apply Error Diffusion on the given image
    static Image ApplyErrorDiffusion(const Image &image, const Filter &filter, const double threshold, bool useSerpentine = false);
    // Apply MBVQ Error Diffusion on the given image
    static Image ApplyMBVQErrorDiffusion(const Image &image, const Filter &filter, bool useSerpentine);

    // Applies the filter on the specified center pixel of the given image
    double Apply(const Image &image, const int32_t u, const int32_t v, const uint8_t channel = 0,
                 const BoundaryExtension &boundaryExtension = BoundaryExtension::Reflection) const;
    // Applies the filter on the entire image
    Image Convolve(const Image &image, const BoundaryExtension &boundaryExtension = BoundaryExtension::Reflection) const;
    // Applies the filter on the entire image and normalizes using min-max normalization
    Image ConvolveAndNormalize(const Image &image, const BoundaryExtension &boundaryExtension = BoundaryExtension::Reflection) const;
};

#endif // FILTER_H