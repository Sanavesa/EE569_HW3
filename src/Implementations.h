#pragma once

#ifndef IMPLEMENTATIONS_H
#define IMPLEMENTATIONS_H

#include <iostream>
#include <unordered_set>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/calib3d.hpp>

#include "Image.h"
#include "Utility.h"

using namespace cv;
using namespace cv::xfeatures2d;


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


Mat CalculateHMatrix(const std::vector<Point2f> srcPoints, const std::vector<Point2f> destPoints)
{
    // H\lambda [3x3] * src [x,y,1] = dest [x,y,1]
    const int pointsCount = srcPoints.size();
    double *srcArray = new double[3 * pointsCount];
    double *destArray = new double[3 * pointsCount];
    for (int i = 0; i < pointsCount; i++)
    {
        srcArray[i + 0 * pointsCount] = static_cast<double>(srcPoints.at(i).x);
        srcArray[i + 1 * pointsCount] = static_cast<double>(srcPoints.at(i).y);
        srcArray[i + 2 * pointsCount] = 1.0;

        destArray[i + 0 * pointsCount] = static_cast<double>(destPoints.at(i).x);
        destArray[i + 1 * pointsCount] = static_cast<double>(destPoints.at(i).y);
        destArray[i + 2 * pointsCount] = 1.0;
    }

    Mat srcMat(3, pointsCount, CV_64FC1, srcArray);
    std::cout << "src mat" << std::endl;
    print(srcMat);
    Mat destMat(3, pointsCount, CV_64FC1, destArray);
    std::cout << "\ndest mat" << std::endl;
    print(destMat);
    
    std::cout << "\nsrc inv mat" << std::endl;
    Mat srcInvMat;
    invert(srcMat, srcInvMat, DECOMP_SVD);
    print(srcInvMat);

    std::cout << "\nh mat" << std::endl;
    Mat h = destMat * srcInvMat;
    print(h);

    return h;
}

Mat CalculatePanoramaMatrix(const Image &fromImage, const Image &toImage, const float ratioThreshold = 0.5f, const int maxPointsCount = -1)
{
    // Load images as OpenCV Mat
    Mat fromMat = RGBImageToMat(fromImage);
    Mat toMat = RGBImageToMat(toImage);

    //-- Step 1: Detect the keypoints using SURF Detector, compute the descriptors
    int minHessian = 400;
    Ptr<SURF> detector = SURF::create(minHessian);
    std::vector<KeyPoint> fromKeypoints, toKeypoints;
    Mat fromDescriptors, toDescriptors;
    detector->detectAndCompute(fromMat, noArray(), fromKeypoints, fromDescriptors);
    detector->detectAndCompute(toMat, noArray(), toKeypoints, toDescriptors);

    //-- Step 2: Matching descriptor vectors with a FLANN based matcher
    // Since SURF is a floating-point descriptor NORM_L2 is used
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::BRUTEFORCE);
    // Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::FLANNBASED);
    std::vector<std::vector<DMatch>> knnMatches;
    matcher->knnMatch(fromDescriptors, toDescriptors, knnMatches, 2);

    //-- Filter matches using the Lowe's ratio test
    std::vector<Point2f> fromPoints, toPoints;
    for (size_t i = 0; i < knnMatches.size(); i++)
    {
        if (knnMatches[i][0].distance < ratioThreshold * knnMatches[i][1].distance)
        {
            if (maxPointsCount == -1 || fromPoints.size() < maxPointsCount)
            {
                fromPoints.push_back(fromKeypoints[knnMatches[i][0].queryIdx].pt);
                toPoints.push_back(toKeypoints[knnMatches[i][0].trainIdx].pt);
            }
        }
    }

    // Build transformation calculation
    return CalculateHMatrix(fromPoints, toPoints);
}

void CalculateExtremas(const Image &src, const Mat matrix, double& minX, double& maxX, double& minY, double& maxY)
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

            minX = std::min(minX, resultX);
            maxX = std::max(maxX, resultX);
            minY = std::min(minY, resultY);
            maxY = std::max(maxY, resultY);
        }
    }
}

void Blit(const Image& src, Image& dest, const size_t offsetX, const size_t offsetY, std::unordered_set<std::pair<size_t, size_t>, PairHash>& occupiedPixels)
{
    for (size_t v = 0; v < src.height; v++)
    {
        for (size_t u = 0; u < src.width; u++)
        {
            const size_t x = u + offsetX;
            const size_t y = v + offsetY;

            if (dest.IsInBounds(static_cast<int32_t>(y), static_cast<int32_t>(x)))
            {
                const auto pos = std::make_pair(y, x);

                // It is the first time drawing at this position
                if (occupiedPixels.find(pos) == occupiedPixels.end())
                {
                    for (size_t c = 0; c < src.channels; c++)
                        dest(y, x, c) = src(v, u, c);
                }
                // Already has been drawn there before, lets average
                else
                {
                    for (size_t c = 0; c < src.channels; c++)
                        dest(y, x, c) = Saturate(((double)dest(y, x, c) + (double)src(v, u, c)) / 2.0);
                }

                occupiedPixels.insert(pos);
            }
        }
    }
}

void Blit(const Image& src, Image& dest, const double offsetX, const double offsetY, std::unordered_set<std::pair<size_t, size_t>, PairHash>& occupiedPixels, const Mat matrix)
{
    for (size_t v = 0; v < src.height; v++)
    {
        for (size_t u = 0; u < src.width; u++)
        {
            // // Apply matrix to the given x,y
            double point[3] = {static_cast<double>(u), static_cast<double>(v), 1.0};
            Mat pointMat(3, 1, CV_64F, point);

            // // Perform transformation and retrieve answer
            Mat result = matrix * pointMat;
            const double resultX = result.at<double>(0, 0);
            const double resultY = result.at<double>(1, 0);

            const int32_t destX = static_cast<int32_t>(std::round(resultX + offsetX));
            const int32_t destY = static_cast<int32_t>(std::round(resultY + offsetY));

            // Only copy pixels if the pixel is within bounds
            if (dest.IsInBounds(destY, destX))
            {
                const auto pos = std::make_pair(destY, destX);
                
                // It is the first time drawing at this position
                if (occupiedPixels.find(pos) == occupiedPixels.end())
                {
                    for (size_t c = 0; c < src.channels; c++)
                        dest(destY, destX, c) = src(v, u, c);
                }
                // Already has been drawn there before, lets average
                else
                {
                    for (size_t c = 0; c < src.channels; c++)
                        dest(destY, destX, c) = Saturate(((double)dest(destY, destX, c) + (double)src(v, u, c)) / 2.0);
                }

                occupiedPixels.insert(pos);
            }
            else
            {
                std::cout << "Out of bounds. Should not happen: " << destX << ", " << destY << " from " << u << ", " << v << std::endl;
            }
        }
    }
}


// bool IsPixelOccupied(const Image& image, const int32_t row, const int32_t column, const std::unordered_set<std::pair<size_t, size_t>, PairHash>& occupiedPixels)
// {
//     // Ignore out of bounds
//     if (row < 0 || column < 0 || row >= image.height || column >= image.width)
//         return true;

//     // Check if position is occupied
//     const auto pos = std::make_pair(static_cast<size_t>(row), static_cast<size_t>(column));
//     return occupiedPixels.find(pos) != occupiedPixels.begin();
// }


// void Interpolate(Image& image, const std::unordered_set<std::pair<size_t, size_t>, PairHash>& occupiedPixels)
// {
//     double *channels = new double[image.channels];

//     for (int32_t v = 0; v < image.height; v++)
//     {
//         for (int32_t u = 0; u < image.width; u++)
//         {
//             if (image(v, u, 0) == 0 && image(v, u, 1) == 0 && image(v, u, 2) == 0)

//             // If center pixel is not occupied...
//             // if (!IsPixelOccupied(image, v, u, occupiedPixels))
//             {
//                 // std::cout << u << ", " << v << " not occupied" << std::endl;
//                 int neighborCount = 0;

//                 for (size_t c = 0; c < image.channels; c++)
//                     channels[c] = 0;

//                 // And all of its neighbors are occupied...
//                 for (int32_t dv = -1; dv <= 1; dv++)
//                 {
//                     for (int32_t du = -1; du <= 1; du++)
//                     {
//                         if (dv == 0 && du == 0)
//                             continue;

//                         const int32_t v2 = v + dv;
//                         const int32_t u2 = u + du;
//                         if (image.IsInBounds(v2, u2) && IsPixelOccupied(image, v2, u2, occupiedPixels))
//                         {
//                             neighborCount++;
//                             for (size_t c = 0; c < image.channels; c++)
//                                 channels[c] += image(static_cast<size_t>(v2), static_cast<size_t>(u2), c);
//                         }
//                     }
//                 }

//                 if (neighborCount >= 1)
//                 {
//                     // std::cout << " OK! " << u << ", " << v << std::endl;
//                     for (size_t c = 0; c < image.channels; c++)
//                         image(v, u, c) = Saturate(channels[c] / neighborCount);
//                 }
//             }
//         }
//     }

//     delete[] channels;
// }




#endif // IMPLEMENTATIONS_H