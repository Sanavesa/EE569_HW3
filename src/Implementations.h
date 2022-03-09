#pragma once

#ifndef IMPLEMENTATIONS_H
#define IMPLEMENTATIONS_H

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "Image.h"
#include "Utility.h"

using namespace cv;

// Indicates the triangle's position for Q1 as part of spatial wraping algorithm.
enum TrianglePosition
{
    None = 0,
    Left = 1,
    Top = 2,
    Right = 4,
    Bottom = 8,
    TopLeft = Top | Left,
    TopRight = Top | Right,
    BottomLeft = Bottom | Left,
    BottomRight = Bottom | Right,
};

// Returns the image coordinate after applying a transformation matrix on the given image coordinate
std::pair<double, double> TransformPosition(const Image &image, const Mat &matrix, const double &imageX, const double &imageY)
{
    // Convert cartesian
    const auto [x, y] = ImageToCartesianCoord(image, imageX, imageY);

    // Apply matrix to the given x,y
    double point[6] = {1, x, y, x * x, x * y, y * y};
    Mat pointMat(6, 1, CV_64F, point);

    // Perform transformation and retrieve answer
    Mat result = matrix * pointMat;
    const double resultX = result.at<double>(0, 0);
    const double resultY = result.at<double>(1, 0);

    // Return answer as image coordinates (zero-based)
    return CartesianToImageCoord(image, resultX, resultY);
}

// Calculate wrapping transformation matrix from original to wrapped
Mat CalcWrapMatrix(const Image &image, const TrianglePosition &position)
{
    // Extract image dimensions
    double w = static_cast<double>(image.width);
    double h = static_cast<double>(image.height);

    // Preprare the set of points in image coord to use to calculate the transformation matrices
    std::vector<std::pair<double, double>> imagePoints;
    imagePoints.reserve(6);

    // Add center point
    imagePoints.push_back(std::make_pair(0.5 * w - 1, 0.5 * h - 1));

    // Add top-left and median top-left
    if (position & TopLeft)
    {
        imagePoints.push_back(std::make_pair(0, 0));
        imagePoints.push_back(std::make_pair(0.25 * w - 1, 0.25 * h - 1));
    }

    // Add top-right and median top-right
    if (position & TopRight)
    {
        imagePoints.push_back(std::make_pair(w - 1, 0));
        imagePoints.push_back(std::make_pair(0.75 * w - 1, 0.25 * h - 1));
    }

    // Add bottom-left and median bottom-right
    if (position & BottomLeft)
    {
        imagePoints.push_back(std::make_pair(0, h - 1));
        imagePoints.push_back(std::make_pair(0.25 * w - 1, 0.75 * h - 1));
    }

    // Add bottom-right and median bottom-right
    if (position & BottomRight)
    {
        imagePoints.push_back(std::make_pair(w - 1, h - 1));
        imagePoints.push_back(std::make_pair(0.75 * w - 1, 0.75 * h - 1));
    }

    // Add triangle center's base
    if (position & Left)
    {
        imagePoints.push_back(std::make_pair(0, 0.5 * h - 1));
    }
    else if (position & Right)
    {
        imagePoints.push_back(std::make_pair(w - 1, 0.5 * h - 1));
    }
    else if (position & Top)
    {
        imagePoints.push_back(std::make_pair(0.5 * w - 1, 0));
    }
    else if (position & Bottom)
    {
        imagePoints.push_back(std::make_pair(0.5 * w - 1, h - 1));
    }
    else
    {
        std::cout << "Invalid position type given: " << (int)position << std::endl;
        exit(EXIT_FAILURE);
    }

    // Calculate the points and their target positions in cartesian coordinates
    double *srcPoints = new double[6 * 6];  // 6x6 of positions, format for each position: 1 x y x^2 xy y^2
    double *destPoints = new double[2 * 6]; // 2x6 of u,v positions, format: x0 y0 x1 y1 .. x5 y5

    // Convert each point to cartesian
    for (size_t i = 0; i < imagePoints.size(); i++)
    {
        const auto imageCoord = imagePoints[i];
        const auto [x, y] = ImageToCartesianCoord(image, imageCoord.first, imageCoord.second);

        // matrix of x,y positions
        srcPoints[i + 0 * 6] = 1.0;
        srcPoints[i + 1 * 6] = x;
        srcPoints[i + 2 * 6] = y;
        srcPoints[i + 3 * 6] = x * x;
        srcPoints[i + 4 * 6] = x * y;
        srcPoints[i + 5 * 6] = y * y;

        // matrix of u,v positions
        destPoints[i + 0 * 6] = x;
        destPoints[i + 1 * 6] = y;
    }

    // Move the center pixel of the triangle's base by the radius of the circle, 64 pixels
    // Index 5 is x5 and 11 is y5
    if (position & Left)
        destPoints[5] += 64;
    else if (position & Right)
        destPoints[5] -= 64;
    else if (position & Top)
        destPoints[11] -= 64;
    else if (position & Bottom)
        destPoints[11] += 64;

    // Convert to OpenCV's matrix
    Mat srcMat(6, 6, CV_64F, srcPoints); // the x,y matrix
    srcMat = srcMat.inv();
    Mat destMat(2, 6, CV_64F, destPoints); // the u,v matrix

    return destMat * srcMat;
}

// Calculate unwrapping transformation matrix from wrapped to original
Mat CalcUnwrapMatrix(const Image &image, const TrianglePosition &position)
{
    // Extract image dimensions
    double w = static_cast<double>(image.width);
    double h = static_cast<double>(image.height);

    // Preprare the set of points in image coord to use to calculate the transformation matrices
    std::vector<std::pair<double, double>> imagePoints;
    imagePoints.reserve(6);

    // Add center point
    imagePoints.push_back(std::make_pair(0.5 * w - 1, 0.5 * h - 1));

    // Add top-left and median top-left
    if (position & TopLeft)
    {
        imagePoints.push_back(std::make_pair(0, 0));
        imagePoints.push_back(std::make_pair(0.25 * w - 1, 0.25 * h - 1));
    }

    // Add top-right and median top-right
    if (position & TopRight)
    {
        imagePoints.push_back(std::make_pair(w - 1, 0));
        imagePoints.push_back(std::make_pair(0.75 * w - 1, 0.25 * h - 1));
    }

    // Add bottom-left and median bottom-right
    if (position & BottomLeft)
    {
        imagePoints.push_back(std::make_pair(0, h - 1));
        imagePoints.push_back(std::make_pair(0.25 * w - 1, 0.75 * h - 1));
    }

    // Add bottom-right and median bottom-right
    if (position & BottomRight)
    {
        imagePoints.push_back(std::make_pair(w - 1, h - 1));
        imagePoints.push_back(std::make_pair(0.75 * w - 1, 0.75 * h - 1));
    }

    // Add triangle center's base
    if (position & Left)
    {
        imagePoints.push_back(std::make_pair(64, 0.5 * h - 1));
    }
    else if (position & Right)
    {
        imagePoints.push_back(std::make_pair(w - 1 - 64, 0.5 * h - 1));
    }
    else if (position & Top)
    {
        imagePoints.push_back(std::make_pair(0.5 * w - 1, 64));
    }
    else if (position & Bottom)
    {
        imagePoints.push_back(std::make_pair(0.5 * w - 1, h - 1 - 64));
    }
    else
    {
        std::cout << "Invalid position type given: " << (int)position << std::endl;
        exit(EXIT_FAILURE);
    }

    // Calculate the points and their target positions in cartesian coordinates
    double *srcPoints = new double[6 * 6];  // 6x6 of positions, format for each position: 1 x y x^2 xy y^2
    double *destPoints = new double[2 * 6]; // 2x6 of u,v positions, format: x0 y0 x1 y1 .. x5 y5

    // Convert each point to cartesian
    for (size_t i = 0; i < imagePoints.size(); i++)
    {
        const auto imageCoord = imagePoints[i];
        const auto [x, y] = ImageToCartesianCoord(image, imageCoord.first, imageCoord.second);

        // matrix of x,y positions
        srcPoints[i + 0 * 6] = 1.0;
        srcPoints[i + 1 * 6] = x;
        srcPoints[i + 2 * 6] = y;
        srcPoints[i + 3 * 6] = x * x;
        srcPoints[i + 4 * 6] = x * y;
        srcPoints[i + 5 * 6] = y * y;

        // matrix of u,v positions
        destPoints[i + 0 * 6] = x;
        destPoints[i + 1 * 6] = y;
    }

    // Move the center pixel of the triangle's base by the radius of the circle, 64 pixels
    // Index 5 is x5 and 11 is y5
    if (position & Left)
        destPoints[5] -= 64;
    else if (position & Right)
        destPoints[5] += 64;
    else if (position & Top)
        destPoints[11] += 64;
    else if (position & Bottom)
        destPoints[11] -= 64;

    // Convert to OpenCV's matrix
    Mat srcMat(6, 6, CV_64F, srcPoints); // the x,y matrix
    srcMat = srcMat.inv();
    Mat destMat(2, 6, CV_64F, destPoints); // the u,v matrix

    return destMat * srcMat;
}

// Applies a forward mapping with rounding on dest u,v positions
void ApplyForwardMapping(const Image &src, Image &dest, const Mat matrix, const TrianglePosition &position)
{
    if (position & Bottom)
    {
        for (size_t y = src.height - 1, i = 0; y >= src.height / 2; y--, i++)
        {
            for (size_t x = i; x < src.width - i; x++)
            {
                // Convert image coordinate in src (x,y) to image coordinate in dest (destX, destY i.e. u,v)
                const auto destPosition = TransformPosition(src, matrix, static_cast<double>(x), static_cast<double>(y));
                const int32_t destX = static_cast<int32_t>(std::round(destPosition.first));
                const int32_t destY = static_cast<int32_t>(std::round(destPosition.second));

                // Only copy pixels if the pixel is within bounds
                if (dest.IsInBounds(destY, destX))
                {
                    for (size_t c = 0; c < dest.channels; c++)
                        dest(destY, destX, c) = src(y, x, c);
                }
            }
        }
    }
    else if (position & Top)
    {
        for (size_t y = 0, i = 0; y < src.height / 2; y++, i++)
        {
            for (size_t x = i; x < src.width - i; x++)
            {
                // Convert image coordinate in src (x,y) to image coordinate in dest (destX, destY i.e. u,v)
                const auto destPosition = TransformPosition(src, matrix, static_cast<double>(x), static_cast<double>(y));
                const int32_t destX = static_cast<int32_t>(std::round(destPosition.first));
                const int32_t destY = static_cast<int32_t>(std::round(destPosition.second));

                // Only copy pixels if the pixel is within bounds
                if (dest.IsInBounds(destY, destX))
                {
                    for (size_t c = 0; c < dest.channels; c++)
                        dest(destY, destX, c) = src(y, x, c);
                }
            }
        }
    }
    else if (position & Left)
    {
        for (size_t x = 0, i = 0; x < src.width / 2; x++, i++)
        {
            for (size_t y = i; y < src.height - i; y++)
            {
                // Convert image coordinate in src (x,y) to image coordinate in dest (destX, destY i.e. u,v)
                const auto destPosition = TransformPosition(src, matrix, static_cast<double>(x), static_cast<double>(y));
                const int32_t destX = static_cast<int32_t>(std::round(destPosition.first));
                const int32_t destY = static_cast<int32_t>(std::round(destPosition.second));

                // Only copy pixels if the pixel is within bounds
                if (dest.IsInBounds(destY, destX))
                {
                    for (size_t c = 0; c < dest.channels; c++)
                        dest(destY, destX, c) = src(y, x, c);
                }
            }
        }
    }
    else if (position & Right)
    {
        for (size_t x = src.width - 1, i = 0; x >= src.width / 2; x--, i++)
        {
            for (size_t y = i; y < src.height - i; y++)
            {
                // Convert image coordinate in src (x,y) to image coordinate in dest (destX, destY i.e. u,v)
                const auto destPosition = TransformPosition(src, matrix, static_cast<double>(x), static_cast<double>(y));
                const int32_t destX = static_cast<int32_t>(std::round(destPosition.first));
                const int32_t destY = static_cast<int32_t>(std::round(destPosition.second));

                // Only copy pixels if the pixel is within bounds
                if (dest.IsInBounds(destY, destX))
                {
                    for (size_t c = 0; c < dest.channels; c++)
                        dest(destY, destX, c) = src(y, x, c);
                }
            }
        }
    }
}

// Applies a inverse mapping with rounding on src x,y positions
void ApplyInverseMapping(const Image &src, Image &dest, const Mat matrix, const TrianglePosition &position)
{
    if (position & Bottom)
    {
        for (size_t v = dest.height - 1, i = 0; v >= dest.height / 2; v--, i++)
        {
            for (size_t u = i; u < dest.width - i; u++)
            {
                // Convert image coordinate in dest (u,v) to image coordinate in src (x,y)
                const auto srcPosition = TransformPosition(src, matrix, static_cast<double>(u), static_cast<double>(v));
                const int32_t srcX = static_cast<int32_t>(std::round(srcPosition.first));
                const int32_t srcY = static_cast<int32_t>(std::round(srcPosition.second));

                // Only copy pixels if the pixel is within bounds
                if (src.IsInBounds(srcY, srcX))
                {
                    for (size_t c = 0; c < dest.channels; c++)
                        dest(v, u, c) = src(srcY, srcX, c);
                }
            }
        }
    }
    else if (position & Top)
    {
        for (size_t v = 0, i = 0; v < src.height / 2; v++, i++)
        {
            for (size_t u = i; u < src.width - i; u++)
            {
                // Convert image coordinate in dest (u,v) to image coordinate in src (x,y)
                const auto srcPosition = TransformPosition(src, matrix, static_cast<double>(u), static_cast<double>(v));
                const int32_t srcX = static_cast<int32_t>(std::round(srcPosition.first));
                const int32_t srcY = static_cast<int32_t>(std::round(srcPosition.second));

                // Only copy pixels if the pixel is within bounds
                if (src.IsInBounds(srcY, srcX))
                {
                    for (size_t c = 0; c < dest.channels; c++)
                        dest(v, u, c) = src(srcY, srcX, c);
                }
            }
        }
    }
    else if (position & Left)
    {
        for (size_t u = 0, i = 0; u < src.width / 2; u++, i++)
        {
            for (size_t v = i; v < src.height - i; v++)
            {
                // Convert image coordinate in dest (u,v) to image coordinate in src (x,y)
                const auto srcPosition = TransformPosition(src, matrix, static_cast<double>(u), static_cast<double>(v));
                const int32_t srcX = static_cast<int32_t>(std::round(srcPosition.first));
                const int32_t srcY = static_cast<int32_t>(std::round(srcPosition.second));

                // Only copy pixels if the pixel is within bounds
                if (src.IsInBounds(srcY, srcX))
                {
                    for (size_t c = 0; c < dest.channels; c++)
                        dest(v, u, c) = src(srcY, srcX, c);
                }
            }
        }
    }
    else if (position & Right)
    {
        for (size_t u = src.width - 1, i = 0; u >= src.width / 2; u--, i++)
        {
            for (size_t v = i; v < src.height - i; v++)
            {
                // Convert image coordinate in dest (u,v) to image coordinate in src (x,y)
                const auto srcPosition = TransformPosition(src, matrix, static_cast<double>(u), static_cast<double>(v));
                const int32_t srcX = static_cast<int32_t>(std::round(srcPosition.first));
                const int32_t srcY = static_cast<int32_t>(std::round(srcPosition.second));

                // Only copy pixels if the pixel is within bounds
                if (src.IsInBounds(srcY, srcX))
                {
                    for (size_t c = 0; c < dest.channels; c++)
                        dest(v, u, c) = src(srcY, srcX, c);
                }
            }
        }
    }
}


void ApplyMatrix(const Image &src, Image& dest, const Mat matrix)
{
    for (size_t v = 0; v < src.height; v++)
    {
        for (size_t u = 0; u < src.width; u++)
        {
            // Apply matrix to the given x,y
            double point[3] = {static_cast<double>(u), static_cast<double>(v), 1.0};
            Mat pointMat(3, 1, CV_64F, point);

            // Perform transformation and retrieve answer
            Mat result = matrix * pointMat;
            const double resultX = result.at<double>(0, 0);
            const double resultY = result.at<double>(1, 0);

            const int32_t destX = static_cast<int32_t>(std::round(resultX));
            const int32_t destY = static_cast<int32_t>(std::round(resultY));

            // Only copy pixels if the pixel is within bounds
            if (dest.IsInBounds(destY, destX))
            {
                // std::cout << u << "," << v << " went to " << destX << ", " << destY << std::endl;
                for (size_t c = 0; c < dest.channels; c++)
                    dest(destY, destX, c) = src(v, u, c);
            }
        }
    }
}

void TestMe(const Image &src, const Mat matrix)
{
    double minX = 999999;
    double maxX = -9999999;
    double minY = minX;
    double maxY = maxX;

    for (size_t v = 0; v < src.height; v++)
    {
        for (size_t u = 0; u < src.width; u++)
        {
            // Apply matrix to the given x,y
            double point[3] = {static_cast<double>(u), static_cast<double>(v), 1.0};
            Mat pointMat(3, 1, CV_64F, point);

            // Perform transformation and retrieve answer
            Mat result = matrix * pointMat;
            const double resultX = result.at<double>(0, 0);
            const double resultY = result.at<double>(1, 0);

            minX = std::min(minX, resultX);
            maxX = std::max(maxX, resultX);
            minY = std::min(minY, resultY);
            maxY = std::max(maxY, resultY);
        }
    }

    std::cout << "Min X: " << minX << std::endl;
    std::cout << "Max X: " << maxX << std::endl;
    std::cout << "Min Y: " << minY << std::endl;
    std::cout << "Max Y: " << maxY << std::endl;
}


#endif // IMPLEMENTATIONS_H