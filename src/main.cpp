#include <iostream>
#include "Image.h"
#include "Utility.h"
#include "Filter.h"
#include "Implementations.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

std::pair<double, double> ImageToCartesianCoord(const double& x, const double& y, const double& imageWidth, const double& imageHeight)
{
    if (x < 0 || x >= imageWidth || y < 0 || y >= imageHeight)
    {
        std::cout << "Invalid image coordinate for ImageToCartesianCoord(): " << x << ", " << y << " for size " << imageWidth << ", " << imageHeight << std::endl;
        exit(EXIT_FAILURE);
    }

    return std::make_pair(x - 0.5, imageHeight + 0.5 - y);
}

void print(cv::Mat& mat, const std::string& message)
{
    std::cout << message << std::endl;
    for (size_t r = 0; r < mat.rows; r++)
    {
        for (size_t c = 0; c < mat.cols; c++)
            std::cout << mat.row(r).col(c) << " ";
        std::cout << std::endl;
    }
}

std::pair<int, int> getPosition(double x, double y, const cv::Mat& mat, const Image& img)
{
    // convert to cartesian
    x -= 0.5;
    y = img.height + 0.5 - y;
    using namespace cv;
    double point[6] = {1, x, y, x * x, x * y, y * y};
    Mat pMat(6, 1, CV_64F, point);
    Mat result = mat * pMat;
    // convert cartesian to image
    double newX = result.at<double>(0, 0) + 0.5;
    double newY = -result.at<double>(1, 0) + 0.5 + img.height;
    newX = std::round(std::clamp(newX, 0.0, img.width - 1.0));
    newY = std::round(std::clamp(newY, 0.0, img.height - 1.0));
    return std::make_pair(newX, newY);
}

int main(int argc, char *argv[])
{
    using namespace cv;
    const Image image("D:\\Programming\\Github\\EE569_HW3\\images\\Forky.raw", 328, 328, 3);

    std::vector<std::pair<double, double>> points;
    points.reserve(6);
    points.push_back(std::make_pair(0.5 * image.width, 0.5 * image.height));   // p1
    points.push_back(std::make_pair(0.25 * image.width, 0.75 * image.height)); // p2
    points.push_back(std::make_pair(0.75 * image.width, 0.75 * image.height)); // p3
    points.push_back(std::make_pair(1, image.height));                         // p4
    points.push_back(std::make_pair(0.5 * image.width, image.height));         // p5
    points.push_back(std::make_pair(image.width, image.height));               // p6

    std::vector<std::pair<double, double>> transformedPoints;
    std::vector<std::pair<double, double>> targetPoints;
    transformedPoints.reserve(points.size());
    targetPoints.reserve(points.size());
    for (const auto& [x, y] : points)
    {
        transformedPoints.push_back(std::make_pair(x - 0.5, image.height + 0.5 - y));
        targetPoints.push_back(std::make_pair(x - 0.5, image.height + 0.5 - y));
    }

    targetPoints[4] = std::make_pair(targetPoints[4].first, targetPoints[4].second + 64);

    double *uv = new double[targetPoints.size() * 2];
    for (size_t i = 0; i < targetPoints.size(); i++)
    {
        const size_t offset = targetPoints.size();
        uv[i + 0 * offset] = targetPoints[i].first;
        uv[i + 1 * offset] = targetPoints[i].second;
    }
    Mat matUV(2, targetPoints.size(), CV_64F, uv);

    double *xy = new double[transformedPoints.size() * transformedPoints.size()];
    for (size_t i = 0; i < transformedPoints.size(); i++)
    {
        const size_t offset = transformedPoints.size();
        xy[i + 0 * offset] = 1;
        xy[i + 1 * offset] = transformedPoints[i].first;
        xy[i + 2 * offset] = transformedPoints[i].second;
        xy[i + 3 * offset] = transformedPoints[i].first * transformedPoints[i].first;
        xy[i + 4 * offset] = transformedPoints[i].first * transformedPoints[i].second;
        xy[i + 5 * offset] = transformedPoints[i].second * transformedPoints[i].second;
    }
    Mat matXY(transformedPoints.size(), transformedPoints.size(), CV_64F, xy);
    matXY = matXY.inv();
    print(matXY);

    Mat answer = matUV * matXY;

    Image image2(image.width, image.height, image.channels);
    for (size_t y = 0; y < image.height; y++)
        for (size_t x = 0; x < image.width; x++)
            for (size_t c = 0; c < image.channels; c++)
                image2(y, x, c) = 0;

    for (size_t y = image.height - 1, i = 0; y >= image.height / 2; y--, i++)
    {
        for (size_t x = i; x < image.width - i; x++)
        {
            // get image coordinate after transformation
            auto [newX, newY] = getPosition(x, y, answer, image);
            for (size_t c = 0; c < image.channels; c++)
                image2(newY, newX, c) = image(y, x, c);
        }
    }

    image2.ExportRAW("testpls.raw");

    std::cout << "Done" << std::endl;
    return 0;
}