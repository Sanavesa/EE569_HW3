/*
#################################################################################################################

# EE569 Homework Assignment #3
# Date: March 10, 2022
# Name: Mohammad Alali
# ID: 5661-9219-82
# email: alalim@usc.edu

#################################################################################################################

    CONSOLE APPLICATION : Homographic Transformation and Image Stitching
	
#################################################################################################################

This file will load 3 RGB image that are left, middle, and right and construct a panorama view out of them.

#################################################################################################################

Arguments:
    programName leftInputFilenameNoExtension middleInputFilenameNoExtension rightInputFilenameNoExtension width height channels
    *InputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q2.exe left middle right 576 432 3

########################################### Notes on Arguments ####################################################

1- The file paths can be either relative to the executable or absolute paths.
2- If 'NoExtension' is on an argument, the image filename SHOULD NOT include an extension like .raw, the program will add that automatically.
3- All arguments are mandatory, only arguments marked with [varName] have defaults.

############################################### Other Files #######################################################

Image.h, Image.cpp
	These files contain an abstraction for handling RAW images to simplify programming and for ease of readability.

Utility.h, Utility.cpp
	These files provide auxiliary helper functions used through the program.

Implementations.h
	This file contains the concrete implementation of the algorithms required in the assignment.

#################################################################################################################
*/

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
#include "opencv2/core/utils/logger.hpp"

#include "Image.h"
#include "Implementations.h"

using namespace cv;
using namespace cv::xfeatures2d;

int main(int argc, char *argv[])
{
    // Make OpenCV silent
    utils::logging::setLogLevel(utils::logging::LogLevel::LOG_LEVEL_SILENT);

    // Read the console arguments
    // Check for proper syntax
    if (argc != 7)
    {
        std::cout << "Syntax Error - Arguments must be:" << std::endl;
        std::cout << "programName leftInputFilenameNoExtension middleInputFilenameNoExtension rightInputFilenameNoExtension width height channels" << std::endl;
        std::cout << "*InputFilenameNoExtension is the .raw image without the extension" << std::endl;
        return -1;
    }

	// Parse console arguments
	const std::string leftInputFilenameNoExtension = argv[1];
	const std::string middleInputFilenameNoExtension = argv[2];
	const std::string rightInputFilenameNoExtension = argv[3];
	const uint32_t width = (uint32_t)atoi(argv[4]);
	const uint32_t height = (uint32_t)atoi(argv[5]);
	const uint8_t channels = (uint8_t)atoi(argv[6]);

    // Load left input image
    Image leftInputImage(width, height, channels);
	if (!leftInputImage.ImportRAW(leftInputFilenameNoExtension + ".raw"))
		return -1;
    
    // Load middle input image
    Image middleInputImage(width, height, channels);
	if (!middleInputImage.ImportRAW(middleInputFilenameNoExtension + ".raw"))
		return -1;

    // Load right input image
    Image rightInputImage(width, height, channels);
	if (!rightInputImage.ImportRAW(rightInputFilenameNoExtension + ".raw"))
		return -1;

    // Calculate transformation matrices
    auto leftControlPoints = FindControlPoints(leftInputImage, middleInputImage, 40);
    auto rightControlPoints = FindControlPoints(rightInputImage, middleInputImage, 25);

    // Visualize the control points and export them as images
    imwrite("left-mid.png", std::get<2>(leftControlPoints));
    imshow("left-mid", std::get<2>(leftControlPoints));
    imwrite("right-mid.png", std::get<2>(rightControlPoints));
    imshow("right-mid", std::get<2>(rightControlPoints));
    waitKey(0);

    // Compute the transformation matrix, H, for left/right to middle given the control points above
    Mat left2MiddleMat = CalculateHMatrix(std::get<0>(leftControlPoints), std::get<1>(leftControlPoints));
    Mat right2MiddleMat = CalculateHMatrix(std::get<0>(rightControlPoints), std::get<1>(rightControlPoints));

    // Calculate offsets for the boundary of the canvas
    double minX = 99999999999;
    double maxX = -minX, minY = minX, maxY = -minX;
    CalculateExtremas(leftInputImage, left2MiddleMat, minX, maxX, minY, maxY);
    CalculateExtremas(rightInputImage, right2MiddleMat, minX, maxX, minY, maxY);
    std::cout << "Min X: " << minX << std::endl;
    std::cout << "Max X: " << maxX << std::endl;
    std::cout << "Min Y: " << minY << std::endl;
    std::cout << "Max Y: " << maxY << std::endl;

    // Create a large enough canvas, filled with black
    const double offsetX = std::max(0.0, -minX);
    const double offsetY = std::max(0.0, -minY);
    const size_t canvasWidth = static_cast<size_t>(std::round(maxX + offsetX + 1));
    const size_t canvasHeight = static_cast<size_t>(std::round(maxY + offsetY + 1));
    std::cout << "Canvas dimensions: " << canvasWidth << ", " << canvasHeight << std::endl;
    std::cout << "Offsets: " << offsetX << ", " << offsetY << std::endl;
    Image panoramaImage(canvasWidth, canvasHeight, 3);
    panoramaImage.Fill(0);

    // Hold the occupied pixels, so we can average out if more than one image draws to the same location
    std::unordered_set<std::pair<size_t, size_t>, PairHash> occupiedPixels;

    // Blit each image into the canvas using inverse address mapping
    BlitInverse(leftInputImage, panoramaImage, static_cast<size_t>(std::round(offsetX)), static_cast<size_t>(std::round(offsetY)), occupiedPixels, left2MiddleMat);
    BlitInverse(rightInputImage, panoramaImage, static_cast<size_t>(std::round(offsetX)), static_cast<size_t>(std::round(offsetY)), occupiedPixels, right2MiddleMat);

    // Blit the middle image onto the canvas
    Blit(middleInputImage, panoramaImage, static_cast<size_t>(std::round(offsetX)), static_cast<size_t>(std::round(offsetY)), occupiedPixels);

    // Export panorama image
    if (!panoramaImage.ExportRAW("panorama.raw"))
        return -1;

    // Used for local debugging: export and show each image separately as well as altogether
    Mat tempMat = RGBImageToMat(panoramaImage);
    imshow("panorama", tempMat);
    imwrite("panorama.png", tempMat);
    Image tempImage(canvasWidth, canvasHeight, 3);
    tempImage.Fill(0);
    std::unordered_set<std::pair<size_t, size_t>, PairHash> tempS;
    
    // Show and export middle image alone
    Blit(middleInputImage, tempImage, static_cast<size_t>(std::round(offsetX)), static_cast<size_t>(std::round(offsetY)), tempS);
    tempMat = RGBImageToMat(tempImage);
    tempImage.ExportRAW("solo_mid.raw");
    imwrite("solo_mid.png", tempMat);
    imshow("mid", tempMat);

    // Show and export left image alone
    tempS.clear();
    tempImage.Fill(0);
    BlitInverse(leftInputImage, tempImage, static_cast<size_t>(std::round(offsetX)), static_cast<size_t>(std::round(offsetY)), tempS, left2MiddleMat);
    tempMat = RGBImageToMat(tempImage);
    tempImage.ExportRAW("solo_left.raw");
    imwrite("solo_left.png", tempMat);
    imshow("left", tempMat);

    // Show and export right image alone
    tempS.clear();
    tempImage.Fill(0);
    BlitInverse(rightInputImage, tempImage, static_cast<size_t>(std::round(offsetX)), static_cast<size_t>(std::round(offsetY)), tempS, right2MiddleMat);
    tempMat = RGBImageToMat(tempImage);
    tempImage.ExportRAW("solo_right.raw");
    imwrite("solo_right.png", tempMat);
    imshow("right", tempMat);
    waitKey(0);

    std::cout << "Done" << std::endl;
    return 0;
}