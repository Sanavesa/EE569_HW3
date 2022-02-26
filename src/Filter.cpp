#define _USE_MATH_DEFINES

#include "Filter.h"
#include "Utility.h"
#include <iostream>
#include <cmath>
#include <limits>

// Creates a new filter with the specified size
Filter::Filter(const uint32_t _size) : size(_size)
{
    if (_size % 2 == 0)
        std::cout << "ERROR: No support for even-sized filters: " << _size << std::endl;

    // Allocate image data array
    // data is double[size][size]
    data = new double *[size];
    for (uint32_t v = 0; v < size; v++)
    {
        data[v] = new double[size];
        for (uint32_t u = 0; u < size; u++)
            data[v][u] = 0.0;
    }
}

// Copy constructor
Filter::Filter(const Filter &other) : size(other.size)
{
    // Allocate image data array
    // data is double[size][size]
    data = new double *[size];
    for (uint32_t v = 0; v < size; v++)
    {
        data[v] = new double[size];
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

// Applies the filter on the specified center pixel of the given image
double Filter::Apply(const Image &image, const int32_t u, const int32_t v, const uint8_t channel, const BoundaryExtension &boundaryExtension) const
{
    const int32_t centerIndex = size / 2;
    double sum = 0.0;

    for (int32_t dv = -centerIndex; dv <= centerIndex; dv++)
        for (int32_t du = -centerIndex; du <= centerIndex; du++)
            sum += data[centerIndex + dv][centerIndex + du] * static_cast<double>(image.GetPixelValue(v + dv, u + du, channel, boundaryExtension));

    return sum;
}

// Applies the filter on the entire image
Image Filter::Convolve(const Image &image, const BoundaryExtension &boundaryExtension) const
{
    Image result(image.width, image.height, image.channels);

    // Convolve across the image, using reflection padding
    for (uint32_t v = 0; v < result.height; v++)
        for (uint32_t u = 0; u < result.width; u++)
            for (uint8_t c = 0; c < result.channels; c++)
                result(v, u, c) = Saturate(Apply(image, u, v, c, boundaryExtension));

    return result;
}

// Applies the filter on the entire image and normalizes using min-max normalization
Image Filter::ConvolveAndNormalize(const Image &image, const BoundaryExtension &boundaryExtension) const
{
    // Keep track of all magnitudes as a flattened array
    const uint32_t numPixels = image.numPixels * static_cast<uint32_t>(image.channels);
    double *gradMagnitudes = new double[numPixels];

    // Keep of min and max gradient magnitude so we can normalize
    double minGradMag = std::numeric_limits<double>::max();
    double maxGradMag = std::numeric_limits<double>::min();

    // Compute magnitudes
    for (uint32_t v = 0, i = 0; v < image.height; v++)
    {
        for (uint32_t u = 0; u < image.width; u++)
        {
            for (uint8_t c = 0; c < image.channels; c++, i++)
            {
                const double value = Apply(image, u, v, c, boundaryExtension);
                gradMagnitudes[i] = value;
                minGradMag = std::min(minGradMag, value);
                maxGradMag = std::max(maxGradMag, value);
            }
        }
    }

    // Normalize magnitudes to 0-1 then scaled by 255, using min-max normalization
    for (uint32_t i = 0; i < numPixels; i++)
        gradMagnitudes[i] = 255.0 * (gradMagnitudes[i] - minGradMag) / (maxGradMag - minGradMag);

    // Create the image from the normalized&scaled magnitudes
    Image result(image.width, image.height, image.channels);
    for (uint32_t v = 0, i = 0; v < result.height; v++)
        for (uint32_t u = 0; u < result.width; u++)
            for (uint8_t c = 0; c < result.channels; c++, i++)
                result(v, u, c) = Saturate(gradMagnitudes[i]);

    // Free allocated memory
    delete[] gradMagnitudes;

    return result;
}

// Apply Error Diffusion on the given image
Image Filter::ApplyErrorDiffusion(const Image &image, const Filter &filter, const double threshold, bool useSerpentine)
{
    const int32_t halfFilterSize = static_cast<int32_t>(filter.size) / 2;
    const Filter flippedFilter = Filter::FlipHorizontal(filter);

    Image result(image);

    double *errors = new double[result.channels];
    double ***data = new double **[result.height];
    for (uint32_t v = 0; v < result.height; v++)
    {
        data[v] = new double *[result.width];
        for (uint32_t u = 0; u < result.width; u++)
        {
            data[v][u] = new double[result.channels];
            for (uint8_t c = 0; c < result.channels; c++)
                data[v][u][c] = static_cast<double>(result(v, u, c));
        }
    }

    for (uint32_t v = 0; v < result.height; v++)
    {
        const Filter curFilter = (v % 2 == 1 && useSerpentine) ? flippedFilter : filter;

        for (uint32_t u = 0; u < result.width; u++)
        {
            const uint32_t uOpp = (v % 2 == 1 && useSerpentine) ? result.width - u - 1 : u;
            for (uint8_t c = 0; c < result.channels; c++)
            {
                const double intensity = data[v][uOpp][c];
                const double quantizedIntensity = (intensity >= threshold) ? 255.0 : 0.0;

                errors[c] = intensity - quantizedIntensity;
                data[v][uOpp][c] = quantizedIntensity;

                for (uint32_t fv = 0; fv < curFilter.size; fv++)
                {
                    for (uint32_t fu = 0; fu < curFilter.size; fu++)
                    {
                        const int32_t v2 = static_cast<int32_t>(v) - halfFilterSize + static_cast<int32_t>(fv);
                        const int32_t u2 = static_cast<int32_t>(uOpp) - halfFilterSize + static_cast<int32_t>(fu);
                        if (result.IsInBounds(v2, u2, c))
                        {
                            const double neighborIntensity = data[v2][u2][c];
                            const double newIntensity = std::clamp(neighborIntensity + errors[c] * curFilter.data[fv][fu], 0.0, 255.0);
                            data[v2][u2][c] = newIntensity;
                        }
                    }
                }
            }
        }
    }

    // Update image
    for (uint32_t v = 0; v < result.height; v++)
        for (uint32_t u = 0; u < result.width; u++)
            for (uint8_t c = 0; c < result.channels; c++)
                result(v, u, c) = Saturate(data[v][u][c]);

    // Free dynamic resources
    for (uint32_t v = 0; v < result.height; v++)
    {
        for (uint32_t u = 0; u < result.width; u++)
            delete[] data[v][u];

        delete[] data[v];
    }

    delete[] data;
    delete[] errors;

    return result;
}

// Apply MBVQ Error Diffusion on the given image
Image Filter::ApplyMBVQErrorDiffusion(const Image &image, const Filter &filter, bool useSerpentine)
{
    const int32_t halfFilterSize = static_cast<int32_t>(filter.size) / 2;
    const Filter flippedFilter = Filter::FlipHorizontal(filter);

    Image result(image);

    double *errors = new double[result.channels];
    double ***data = new double **[result.height];
    for (uint32_t v = 0; v < result.height; v++)
    {
        data[v] = new double *[result.width];
        for (uint32_t u = 0; u < result.width; u++)
        {
            data[v][u] = new double[result.channels];
            for (uint8_t c = 0; c < result.channels; c++)
                data[v][u][c] = static_cast<double>(result(v, u, c));
        }
    }

    for (uint32_t v = 0; v < result.height; v++)
    {
        const Filter curFilter = (v % 2 == 1 && useSerpentine) ? flippedFilter : filter;

        for (uint32_t u = 0; u < result.width; u++)
        {
            const uint32_t uOpp = (v % 2 == 1 && useSerpentine) ? result.width - u - 1 : u;

            const uint8_t r = static_cast<uint8_t>(data[v][uOpp][0]);
            const uint8_t g = static_cast<uint8_t>(data[v][uOpp][1]);
            const uint8_t b = static_cast<uint8_t>(data[v][uOpp][2]);

            const MBVQType mbvq = DetermineMBVQ(r, g, b);
            const VertexType vertex = DetermineVertex(mbvq, r, g, b);

            // Extract color from 32-bit unsigned integer of 0xRRGGBBAA
            // Compute quantized pixel
            const uint8_t newR = static_cast<uint8_t>((vertex & Red) >> 24);
            const uint8_t newG = static_cast<uint8_t>((vertex & Green) >> 16);
            const uint8_t newB = static_cast<uint8_t>((vertex & Blue) >> 8);
            result(v, uOpp, 0) = newR;
            result(v, uOpp, 1) = newG;
            result(v, uOpp, 2) = newB;

            // Compute quantized error
            errors[0] = static_cast<double>(r) - static_cast<double>(newR);
            errors[1] = static_cast<double>(g) - static_cast<double>(newG);
            errors[2] = static_cast<double>(b) - static_cast<double>(newB);

            // Diffuse error among neighbors
            for (uint32_t fv = 0; fv < curFilter.size; fv++)
            {
                for (uint32_t fu = 0; fu < curFilter.size; fu++)
                {
                    const int32_t v2 = static_cast<int32_t>(v) - halfFilterSize + static_cast<int32_t>(fv);
                    const int32_t u2 = static_cast<int32_t>(uOpp) - halfFilterSize + static_cast<int32_t>(fu);
                    for (uint8_t c = 0; c < result.channels; c++)
                    {
                        if (result.IsInBounds(v2, u2, c))
                        {
                            const double neighborIntensity = data[v2][u2][c];
                            const double newIntensity = std::clamp(neighborIntensity + errors[c] * curFilter.data[fv][fu], 0.0, 255.0);
                            data[v2][u2][c] = newIntensity;
                        }
                    }
                }
            }
        }
    }

    // Free dynamic resources
    for (uint32_t v = 0; v < result.height; v++)
    {
        for (uint32_t u = 0; u < result.width; u++)
            delete[] data[v][u];

        delete[] data[v];
    }

    delete[] data;
    delete[] errors;

    return result;
}

// Create uniform filter with the specified filter size
Filter Filter::CreateUniform(const uint32_t size)
{
    Filter filter(size);
    const double value = 1.0 / (size * size);
    for (uint32_t v = 0; v < size; v++)
        for (uint32_t u = 0; u < size; u++)
            filter.data[v][u] = value;
    return filter;
}

// Create gaussian filter with the specified filter size and standard deviation
Filter Filter::CreateGaussian(const uint32_t size, const double stdev)
{
    Filter filter(size);

    const int32_t centerIndex = size / 2;
    const double variance = stdev * stdev;
    const double coeff = 1.0 / (2.0 * M_PI * variance);
    const double expCoeff = -1.0 / (2 * variance);

    // Using gaussian 2D formula, compute values
    double sum = 0.0;
    for (int32_t dv = -centerIndex; dv <= centerIndex; dv++)
        for (int32_t du = -centerIndex; du <= centerIndex; du++)
        {
            uint32_t v = centerIndex + dv;
            uint32_t u = centerIndex + du;
            double value = coeff * std::exp(expCoeff * (dv * dv + du * du));
            filter.data[v][u] = value;
            sum += value;
        }

    // Normalize
    for (uint32_t v = 0; v < size; v++)
        for (uint32_t u = 0; u < size; u++)
            filter.data[v][u] /= sum;

    return filter;
}

// Create 3x3 Sobel - Gradient X filter
Filter Filter::CreateSobelX()
{
    const static int32_t sobelFilterX[3][3] = {
        {1, 0, -1},
        {2, 0, -2},
        {1, 0, -1}};

    Filter filter(3);
    for (uint32_t v = 0; v < filter.size; v++)
        for (uint32_t u = 0; u < filter.size; u++)
            filter.data[v][u] = sobelFilterX[v][u];

    return filter;
}

// Create 3x3 Sobel - Gradient Y filter
Filter Filter::CreateSobelY()
{
    const static int32_t sobelFilterY[3][3] = {
        {-1, -2, -1},
        {0, 0, 0},
        {1, 2, 1}};

    Filter filter(3);
    for (uint32_t v = 0; v < filter.size; v++)
        for (uint32_t u = 0; u < filter.size; u++)
            filter.data[v][u] = sobelFilterY[v][u];

    return filter;
}

// Create Error Diffusion Floyd-Steinberg filter
Filter Filter::CreateFloydSteinberg()
{
    const static int32_t floyd[3][3] = {
        {0, 0, 0},
        {0, 0, 7},
        {3, 5, 1}};

    Filter filter(3);
    for (uint32_t v = 0; v < filter.size; v++)
        for (uint32_t u = 0; u < filter.size; u++)
            filter.data[v][u] = floyd[v][u] / 16.0;

    return filter;
}

// Create Error Diffusion JJN filter
Filter Filter::CreateJJN()
{
    const static int32_t jjn[5][5] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 7, 5},
        {3, 5, 7, 5, 3},
        {1, 3, 5, 3, 1}};

    Filter filter(5);
    for (uint32_t v = 0; v < filter.size; v++)
        for (uint32_t u = 0; u < filter.size; u++)
            filter.data[v][u] = jjn[v][u] / 48.0;

    return filter;
}

// Create Error Diffusion Stucki filter
Filter Filter::CreateStucki()
{
    const static int32_t jjn[5][5] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 8, 4},
        {2, 4, 8, 4, 2},
        {1, 2, 4, 2, 1}};

    Filter filter(5);
    for (uint32_t v = 0; v < filter.size; v++)
        for (uint32_t u = 0; u < filter.size; u++)
            filter.data[v][u] = jjn[v][u] / 42.0;

    return filter;
}

// Create Error Diffusion Alali filter
Filter Filter::CreateAlali()
{
    const static int32_t alali[5][5] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 16, 8},
        {2, 12, 16, 12, 4},
        {1, 4, 8, 2, 1}};

    Filter filter(5);
    for (uint32_t v = 0; v < filter.size; v++)
        for (uint32_t u = 0; u < filter.size; u++)
            filter.data[v][u] = alali[v][u] / 86.0;

    return filter;
}

// Return the filter flipped horizontally
Filter Filter::FlipHorizontal(const Filter &filter)
{
    Filter flippedFilter(filter.size);
    for (uint32_t v = 0; v < filter.size; v++)
        for (uint32_t u = 0; u < filter.size; u++)
            flippedFilter.data[v][u] = filter.data[v][filter.size - u - 1];

    return flippedFilter;
}

// Return the filter flipped vertically
Filter Filter::FlipVertical(const Filter &filter)
{
    Filter flippedFilter(filter.size);
    for (uint32_t v = 0; v < filter.size; v++)
        for (uint32_t u = 0; u < filter.size; u++)
            flippedFilter.data[v][u] = filter.data[filter.size - v - 1][u];

    return flippedFilter;
}