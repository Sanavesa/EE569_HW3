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
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "Image.h"
#include "Implementations.h"


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

    // Do stuff

    std::cout << "Done" << std::endl;
    return 0;
}