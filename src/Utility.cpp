#include "Utility.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Image.h"

// Returns the intensity saturated to the range [0, 255]
uint8_t Saturate(const double intensity)
{
    return static_cast<uint8_t>(std::clamp(std::round(intensity), 0.0, 255.0));
}

// Write the given array to the specified file where each line is the index followed by its value, separated by a space
bool WriteArrayToFile(const std::string &filename, const std::array<uint32_t, 256> &arr)
{
	// Open the file
	std::ofstream outStream;
	outStream.open(filename);

	// Check if file opened successfully
	if (!outStream.is_open())
	{
		std::cout << "Cannot open file for writing: " << filename << std::endl;
        return false;
	}

    // Write to the file
	for (uint32_t i = 0; i < 256; i++)
        outStream << i << "," << arr[i] << std::endl;

	outStream.close();
    return true;
}

// Write the given array to the specified file where each line is the index followed by its value, separated by a space
bool WriteArrayToFile(const std::string &filename, const std::array<double, 256> &arr)
{
	// Open the file
	std::ofstream outStream;
	outStream.open(filename);

	// Check if file opened successfully
	if (!outStream.is_open())
	{
		std::cout << "Cannot open file for writing: " << filename << std::endl;
        return false;
	}

	// Write to the file
	for (uint32_t i = 0; i < 256; i++)
		outStream << i << "," << arr[i] << std::endl;

	outStream.close();
    return true;
}

// Converts an image from RGB to Grayscale
Image RGB2Grayscale(const Image &image)
{
    Image result(image.width, image.height, 1);

    for (uint32_t v = 0; v < result.height; v++)
    {
        for (uint32_t u = 0; u < result.width; u++)
        {
            const double r = static_cast<double>(image(v, u, 0));
            const double g = static_cast<double>(image(v, u, 1));
            const double b = static_cast<double>(image(v, u, 2));
            const double y = 0.2989 * r + 0.5870 * g + 0.1140 * b;
            result(v, u, 0) = Saturate(y);
        }
    }

    return result;
}

// Converts an image from RGB to CMY
Image RGB2CMY(const Image &image)
{
    Image result(image.width, image.height, image.channels);

    for (uint32_t v = 0; v < result.height; v++)
    {
        for (uint32_t u = 0; u < result.width; u++)
        {
            const double r = static_cast<double>(image(v, u, 0));
            const double g = static_cast<double>(image(v, u, 1));
            const double b = static_cast<double>(image(v, u, 2));
            result(v, u, 0) = Saturate(255.0 - r); // c
            result(v, u, 1) = Saturate(255.0 - g); // m
            result(v, u, 2) = Saturate(255.0 - b); // y
        }
    }

    return result;
}

// Converts an image from CMY to RGB
Image CMY2RGB(const Image &image)
{
    Image result(image.width, image.height, image.channels);

    for (uint32_t v = 0; v < result.height; v++)
    {
        for (uint32_t u = 0; u < result.width; u++)
        {
            const double c = static_cast<double>(image(v, u, 0));
            const double m = static_cast<double>(image(v, u, 1));
            const double y = static_cast<double>(image(v, u, 2));
            result(v, u, 0) = Saturate(255.0 - c); // r
            result(v, u, 1) = Saturate(255.0 - m); // g
            result(v, u, 2) = Saturate(255.0 - y); // b
        }
    }

    return result;
}

// Calculates the magnitude (L2-norm), euclidean distance of the given x and y.
double Magnitude(const double x, const double y)
{
    return std::sqrt(x * x + y * y);
}

// Generate a bayer index matrix with the specified size. Size must be a multiple of 2.
std::vector<std::vector<double>> GenerateBayerMatrix(const uint32_t size)
{
    const uint32_t bayer2[2][2] = {{1, 2}, {3, 0}};
    const double bayerPatternIncrementor[2][2] = {{1, 2}, {3, 0}};

    // Allocate the final matrix
    std::vector<std::vector<double>> matrix(size, std::vector<double>(size));
    for (uint32_t v = 0; v < 2; v++)
        for (uint32_t u = 0; u < 2; u++)
            matrix[v][u] = bayer2[v][u];

    // Initialize the previous bayer index matrix, initially to bayer2
    std::vector<std::vector<double>> prevMatrix{{1, 2}, {3, 0}};
    uint32_t currentSize = 2;

    // Recursively expand the matrix
    while (currentSize < size)
    {
        for (uint32_t bv = 0; bv < 2; bv++)
        {
            for (uint32_t bu = 0; bu < 2; bu++)
            {
                const double increment = bayerPatternIncrementor[bv][bu];
                for (uint32_t v = 0; v < currentSize; v++)
                {
                    for (uint32_t u = 0; u < currentSize; u++)
                    {
                        const double value = prevMatrix[v][u];
                        const double result = 4.0 * value + increment;
                        const uint32_t rv = v + currentSize * bv;
                        const uint32_t ru = u + currentSize * bu;
                        matrix[rv][ru] = result;
                    }
                }
            }
        }

        const auto currentMatrixCopy(matrix);
        prevMatrix = currentMatrixCopy;
        currentSize *= 2;
    }

    // Normalize the matrix using the threshold equation
    const double numPixels = size * size;
    for (uint32_t v = 0; v < size; v++)
        for (uint32_t u = 0; u < size; u++)
            matrix[v][u] = 255 * (matrix[v][u] + 0.5) / numPixels;

    return matrix;
}

// Compute the Peak-Signal-to-Noise-Ratio (PSNR) quality metric to assess performance of denoising algorithm
double PSNR(const Image &originalImage, const Image &filteredImage, const uint8_t channel)
{
    // Calculate Mean Squared Error (MSE)
    // MSE = (1/NM) * sum( (filtered[i,j] - image[i,j])^2 )
    double mse = 0.0;
    for (uint32_t v = 0; v < originalImage.height; v++)
    {
        for (uint32_t u = 0; u < originalImage.width; u++)
        {
            const double originalPixel = static_cast<double>(originalImage.GetPixelValue(v, u, channel));
            const double filteredPixel = static_cast<double>(filteredImage.GetPixelValue(v, u, channel));
            const double difference = filteredPixel - originalPixel;
            mse += difference * difference;
        }
    }
    mse /= static_cast<double>(originalImage.numPixels);

    // Calculate psnr = 10 * log_10 (MAX^2 / MSE), where MAX = 255
    const double psnr = 10 * std::log10 (255.0 * 255.0 / mse);
    return psnr;
}

// Determines the MBVQ quadrant given a pixel color
MBVQType DetermineMBVQ(const uint8_t r, const uint8_t g, const uint8_t b)
{
    const int32_t _r = static_cast<int32_t>(r);
    const int32_t _g = static_cast<int32_t>(g);
    const int32_t _b = static_cast<int32_t>(b);

    if ((_r + _g) > 255)
    {
        if ((_g + _b) > 255)
        {
            if ((_r + _g + _b) > 510)
                return CMYW;
            else
                return MYGC;
        }
        else
            return RGMY;
    }
    else
    {
        // if (!((_g + _b) > 255))
        if ((_g + _b) <= 255)
        {
            // if (!((_r + _g + _b) > 255))
            if ((_r + _g + _b) <= 255)
                return KRGB;
            else
                return RGBM;
        }
        else
            return CMGB;
    }
}

// Determines the vertex in a MBVQ quadrant
VertexType DetermineVertex(const MBVQType mbvq, const uint8_t r, const uint8_t g, const uint8_t b)
{
    const int32_t _r = static_cast<int32_t>(r);
    const int32_t _g = static_cast<int32_t>(g);
    const int32_t _b = static_cast<int32_t>(b);
    VertexType vertex;

    // No.1 for CMYW
    if (mbvq == CMYW)
    {
        vertex = White;
        if (_b < 127)
            if (_b <= _r)
                if (_b <= _g)
                    vertex = Yellow;

        if (_g < 127)
            if (_g <= _b)
                if (_g <= _r)
                    vertex = Magenta;

        if (_r < 127)
            if (_r <= _b)
                if (_r <= _g)
                    vertex = Cyan;
    }

    // No.2 for MYGC
    if (mbvq == MYGC)
    {
        vertex = Magenta;
        if (_g >= _b)
        {
            if (_r >= _b)
            {
                if (_r >= 127)
                    vertex = Yellow;
                else
                    vertex = Green;
            }
        }

        if (_g >= _r)
        {
            if (_b >= _r)
            {
                if (_b >= 127)
                    vertex = Cyan;
                else
                    vertex = Green;
            }
        }
    }

    // No.3 for RGMY
    if (mbvq == RGMY)
    {
        if (_b > 127)
        {
            if (_r > 127)
            {
                if (_b >= _g)
                    vertex = Magenta;
                else
                    vertex = Yellow;
            }
            else
            {
                if (_g > (_b + _r))
                    vertex = Green;
                else
                    vertex = Magenta;
            }
        }
        else
        {
            if (_r >= 127)
            {
                if (_g >= 127)
                    vertex = Yellow;
                else
                    vertex = Red;
            }
            else
            {
                if (_r >= _g)
                    vertex = Red;
                else
                    vertex = Green;
            }
        }
    }

    // No.4 for KRGB
    if (mbvq == KRGB)
    {
        vertex = Black;
        if (_b > 127)
            if (_b >= _r)
                if (_b >= _g)
                    vertex = Blue;

        if (_g > 127)
            if (_g >= _b)
                if (_g >= _r)
                    vertex = Green;

        if (_r > 127)
            if (_r >= _b)
                if (_r >= _g)
                    vertex = Red;
    }

    // No.5 for RGBM
    if (mbvq == RGBM)
    {
        vertex = Green;
        if (_r > _g)
        {
            if (_r >= _b)
            {
                if (_b < 127)
                    vertex = Red;
                else
                    vertex = Magenta;
            }
        }

        if (_b > _g)
        {
            if (_b >= _r)
            {
                if (_r < 127)
                    vertex = Blue;
                else
                    vertex = Magenta;
            }
        }
    }

    // No.6 for CMGB
    if (mbvq == CMGB)
    {
        if (_b > 127)
        {
            if (_r > 127)
            {
                if (_g >= _r)
                    vertex = Cyan;
                else
                    vertex = Magenta;
            }
            else
            {
                if (_g > 127)
                    vertex = Cyan;
                else
                    vertex = Blue;
            }
        }
        else
        {
            if (_r > 127)
            {
                if ((_r - _g + _b) >= 127)
                    vertex = Magenta;
                else
                    vertex = Green;
            }
            else
            {
                if (_g >= _b)
                    vertex = Green;
                else
                    vertex = Blue;
            }
        }
    }

    return vertex;
}