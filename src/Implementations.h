#pragma once

#ifndef IMPLEMENTATIONS_H
#define IMPLEMENTATIONS_H

#include <iostream>
#include <random>
#include "Image.h"
#include "Utility.h"
#include "Filter.h"

// Applies the Sobel edge detection algorithm on the given image with the specified threshold.
// Will return a tuple of images:
// 1st is the final edge map (edges are black, background is white)
// 2nd is the final edge map (edges are white, background is black)
// 3rd is the normalized gradient magnitude map
std::tuple<Image, Image, Image> ApplySobel(const Image &image, const double threshold = 0.95)
{
    Image result(image.width, image.height, 1);

    const Filter xFilter = Filter::CreateSobelX();
    const Filter yFilter = Filter::CreateSobelY();

    // Keep track of all magnitudes as a flattened array
    double *gradMagnitudes = new double[result.numPixels];

    // Keep of min and max gradient magnitude so we can normalize
    double minGradMag = std::numeric_limits<double>::max();
    double maxGradMag = std::numeric_limits<double>::min();

    // Compute magnitudes
    for (uint32_t v = 0, i = 0; v < result.height; v++)
    {
        for (uint32_t u = 0; u < result.width; u++, i++)
        {
            const double gradX = xFilter.Apply(image, u, v, 0, BoundaryExtension::Reflection);
            const double gradY = yFilter.Apply(image, u, v, 0, BoundaryExtension::Reflection);
            const double gradMag = Magnitude(gradX, gradY);
            gradMagnitudes[i] = gradMag;
            minGradMag = std::min(minGradMag, gradMag);
            maxGradMag = std::max(maxGradMag, gradMag);
        }
    }

    // Normalize magnitudes to 0-1 then scale by 255, using min-max normalization
    for (uint32_t i = 0; i < result.numPixels; i++)
        gradMagnitudes[i] = 255.0 * (gradMagnitudes[i] - minGradMag) / (maxGradMag - minGradMag);

    // Create the image from the normalized&scaled magnitudes
    for (uint32_t v = 0, i = 0; v < result.height; v++)
        for (uint32_t u = 0; u < result.width; u++, i++)
            result(v, u, 0) = Saturate(gradMagnitudes[i]);

    // Free allocated memory
    delete[] gradMagnitudes;

    // Copy the result into another image to store the normalized gradient magnitude map
    const Image normalizedGradientMap(result);

    // Compute CDF of the image
    const auto cdf = result.CalculateCumulativeProbabilityHistogram();

    // Find intensity that meets the threshold (intensity >= threshold% of CDF)
    uint8_t cutoffIntensity = 255;
    for (uint8_t intensity = 0; intensity < 256; intensity++)
    {
        if (cdf[intensity] >= threshold)
        {
            // We found the cutoff intensity
            cutoffIntensity = intensity;
            break;
        }
    }

    // Copy the result into another image to have both options of white edge on black and vice versa
    Image resultAsWhiteEdge(result);

    // Apply the cutoff intensity to the image, and make the pixels either black (0) or white (255)
    for (uint32_t v = 0, i = 0; v < result.height; v++)
    {
        for (uint32_t u = 0; u < result.width; u++, i++)
        {
            // Keep pixel as black if greater than cuttoff, white otherwise
            // Edge=black, background=white
            result(v, u, 0) = (result(v, u, 0) >= cutoffIntensity) ? 0 : 255;

            // Edge=white, background=black
            resultAsWhiteEdge(v, u, 0) = (resultAsWhiteEdge(v, u, 0) >= cutoffIntensity) ? 255 : 0;
        }
    }

    return std::make_tuple(result, resultAsWhiteEdge, normalizedGradientMap);
}

// Applies dithering via Fixed Thresholding on the given image with the specified threshold
Image DitherByFixedThresholding(const Image &image, const uint8_t threshold = 128)
{
    Image result(image.width, image.height, image.channels);
    for (uint32_t v = 0; v < image.height; v++)
        for (uint32_t u = 0; u < image.width; u++)
            for (uint8_t c = 0; c < image.channels; c++)
                result(v, u, c) = (image(v, u, c) < threshold) ? 0 : 255;

    return result;
}

// Applies dithering via Random Thresholding on the given image
Image DitherByRandomThresholding(const Image &image)
{
    // Prepare uniform distributon RNG
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(0, 255);

    Image result(image.width, image.height, image.channels);
    for (uint32_t v = 0; v < image.height; v++)
        for (uint32_t u = 0; u < image.width; u++)
            for (uint8_t c = 0; c < image.channels; c++)
            {
                const uint8_t threshold = static_cast<uint8_t>(distribution(gen));
                result(v, u, c) = (image(v, u, c) < threshold) ? 0 : 255;
            }

    return result;
}

// Applies dithering via Dithering Matrix on the given image with the specified matrix size
Image DitherByMatrix(const Image &image, const uint32_t matrixSize)
{
    const auto matrix = GenerateBayerMatrix(matrixSize);

    Image result(image.width, image.height, image.channels);
    for (uint32_t v = 0; v < result.height; v++)
        for (uint32_t u = 0; u < result.width; u++)
            for (uint8_t c = 0; c < result.channels; c++)
                result(v, u, c) = (image(v, u, c) <= matrix[v % matrixSize][u % matrixSize]) ? 0 : 255;

    return result;
}

// Applies error diffusion by Floyd-Steinberg with serpentine scanning
Image ErrorDiffusionByFloyd(const Image &image, const double threshold = 128.0)
{
    return Filter::ApplyErrorDiffusion(image, Filter::CreateFloydSteinberg(), threshold, true);
}

// Applies error diffusion by Jarvis, Judice, and Ninke (JJN) with serpentine scanning
Image ErrorDiffusionByJJN(const Image &image, const double threshold = 128.0)
{
    return Filter::ApplyErrorDiffusion(image, Filter::CreateJJN(), threshold, true);
}

// Applies error diffusion by Stucki with serpentine scanning
Image ErrorDiffusionByStucki(const Image &image, const double threshold = 128.0)
{
    return Filter::ApplyErrorDiffusion(image, Filter::CreateStucki(), threshold, true);
}

// Applies error diffusion by Alali with serpentine scanning
Image ErrorDiffusionByAlali(const Image &image)
{
    double threshold = 0.0;
    for (uint32_t v = 0; v < image.height; v++)
        for (uint32_t u = 0; u < image.width; u++)
            for (uint8_t c = 0; c < image.channels; c++)
                threshold += static_cast<double>(image(v, u, c));
    threshold /= static_cast<double>(image.numPixels) * static_cast<double>(image.channels);
    std::cout << "Adaptive Threshold: " << threshold << std::endl;
    const Image result = Filter::ApplyErrorDiffusion(image, Filter::CreateAlali(), threshold, true);
    return result;
}

// Applies separable error diffusion on the given image
Image SeparableErrorDiffusion(const Image &image, const double threshold = 128.0)
{
    const Image imageCMY = RGB2CMY(image);
    const Image resultCMY = ErrorDiffusionByFloyd(imageCMY, threshold);
    const Image resultRGB = CMY2RGB(resultCMY);
    return resultRGB;
}

// Applies Minimum Brightness Variation Quadrants (MBVQ) error diffusion on the given image
Image MBVQErrorDiffusion(const Image &image)
{
    return Filter::ApplyMBVQErrorDiffusion(image, Filter::CreateFloydSteinberg(), true);
}

#endif // IMPLEMENTATIONS_H