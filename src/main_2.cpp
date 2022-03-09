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

#include "Image.h"
#include "Implementations.h"

using namespace cv;
using namespace cv::xfeatures2d;

int main(int argc, char *argv[])
{
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
    Mat left2MiddleMat = CalculatePanoramaMatrix(leftInputImage, middleInputImage, 0.5f, -1);
    Mat right2MiddleMat = CalculatePanoramaMatrix(rightInputImage, middleInputImage, 0.5f, -1);

    // Calculate offsets
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

    // Blit the middle image onto the canvas
    Blit(middleInputImage, panoramaImage, static_cast<size_t>(std::round(offsetX)), static_cast<size_t>(std::round(offsetY)), occupiedPixels);

    // Blit each image into the canvas
    Blit(leftInputImage, panoramaImage, offsetX, offsetY, occupiedPixels, left2MiddleMat);
    Blit(rightInputImage, panoramaImage, offsetX, offsetY, occupiedPixels, right2MiddleMat);

    // Export panorama image
    if (!panoramaImage.ExportRAW("panorama.raw"))
        return -1;

    // // Export interpolated panorama image
    // std::cout << "occupied pixels: " << occupiedPixels.size() << std::endl;
    // Interpolate(panoramaImage, occupiedPixels);
    // if (!panoramaImage.ExportRAW("panorama_interpolated.raw"))
    //     return -1;

    Image temp1(canvasWidth, canvasHeight, 3);
    temp1.Fill(0);
    std::unordered_set<std::pair<size_t, size_t>, PairHash> tempS;
    
    Blit(middleInputImage, temp1, static_cast<size_t>(std::round(offsetX)), static_cast<size_t>(std::round(offsetY)), tempS);
    temp1.ExportRAW("temp_mid.raw");

    tempS.clear();
    temp1.Fill(0);
    Blit(leftInputImage, temp1, offsetX, offsetY, tempS, left2MiddleMat);
    temp1.ExportRAW("temp_left.raw");

    tempS.clear();
    temp1.Fill(0);
    Blit(rightInputImage, temp1, offsetX, offsetY, tempS, right2MiddleMat);
    temp1.ExportRAW("temp_right.raw");

    std::cout << "Done" << std::endl;
    return 0;
}