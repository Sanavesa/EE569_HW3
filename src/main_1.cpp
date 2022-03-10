/*
#################################################################################################################

# EE569 Homework Assignment #3
# Date: March 10, 2022
# Name: Mohammad Alali
# ID: 5661-9219-82
# email: alalim@usc.edu

#################################################################################################################

    CONSOLE APPLICATION : Geometric Image Modification
	
#################################################################################################################

This file will load an RGB image and then wrap the image as well as unwrap it back to original using a spatial
wrapping technique.

#################################################################################################################

Arguments:
    programName inputFilenameNoExtension width height channels
    inputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q1.exe Forky 328 328 3
    .\EE569_HW3_Q1.exe 22 328 328 3

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
    if (argc != 5)
    {
        std::cout << "Syntax Error - Arguments must be:" << std::endl;
        std::cout << "programName inputFilenameNoExtension width height channels" << std::endl;
        std::cout << "inputFilenameNoExtension is the .raw image without the extension" << std::endl;
        return -1;
    }

	// Parse console arguments
	const std::string inputFilenameNoExtension = argv[1];
	const uint32_t width = (uint32_t)atoi(argv[2]);
	const uint32_t height = (uint32_t)atoi(argv[3]);
	const uint8_t channels = (uint8_t)atoi(argv[4]);

    // Load input image
    Image inputImage(width, height, channels);
	if (!inputImage.ImportRAW(inputFilenameNoExtension + ".raw"))
		return -1;

    // Create the wrapped image and fill with black
    Image wrappedImage(inputImage.width, inputImage.height, inputImage.channels);
    wrappedImage.Fill(0);

    // For each of the triangle sides, calculate the matrices and apply them on the image
    Mat leftMatrix = CalcWrapMatrix(inputImage, Left);
    Mat rightMatrix = CalcWrapMatrix(inputImage, Right);
    Mat topMatrix = CalcWrapMatrix(inputImage, Top);
    Mat bottomMatrix = CalcWrapMatrix(inputImage, Bottom);
    ApplyForwardMapping(inputImage, wrappedImage, leftMatrix, Left);
    ApplyForwardMapping(inputImage, wrappedImage, rightMatrix, Right);
    ApplyForwardMapping(inputImage, wrappedImage, topMatrix, Top);
    ApplyForwardMapping(inputImage, wrappedImage, bottomMatrix, Bottom);

    // Export the wrapped image
    if (!wrappedImage.ExportRAW(inputFilenameNoExtension + "_wrapped.raw"))
        return -1;

    // Create the unwrapped image and fill with black
    Image unwrappedImage(inputImage.width, inputImage.height, inputImage.channels);
    unwrappedImage.Fill(0);

    // For each of the triangle sides, used the same transformation matrices and apply them on the wrapped image
    ApplyInverseMapping(wrappedImage, unwrappedImage, leftMatrix, Left);
    ApplyInverseMapping(wrappedImage, unwrappedImage, rightMatrix, Right);
    ApplyInverseMapping(wrappedImage, unwrappedImage, topMatrix, Top);
    ApplyInverseMapping(wrappedImage, unwrappedImage, bottomMatrix, Bottom);

    // Export the unwrapped image
    if (!unwrappedImage.ExportRAW(inputFilenameNoExtension + "_unwrapped.raw"))
        return -1;

    std::cout << "Done" << std::endl;
    return 0;
}