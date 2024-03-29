/*
#################################################################################################################

# EE569 Homework Assignment #3
# Date: March 10, 2022
# Name: Mohammad Alali
# ID: 5661-9219-82
# email: alalim@usc.edu

#################################################################################################################

    CONSOLE APPLICATION : Basic morphological process implementation
	
#################################################################################################################

This file will load grayscale images and apply the thinning filter on them.

#################################################################################################################

Arguments:
    programName inputFilenameNoExtension width height channels
    inputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q3a.exe spring 252 252 1
    .\EE569_HW3_Q3a.exe flower 247 247 1
    .\EE569_HW3_Q3a.exe jar 252 252 1

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
#include <vector>

#include "Image.h"
#include "Implementations.h"
#include "Filter.h"

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

    // Binarize the given image
    Image binarizedImage = BinarizeImage(inputImage);
    if (!binarizedImage.ExportRAW(inputFilenameNoExtension + "_binarized.raw"))
        return -1;

    // Create a thinning conditional filter for first stage
    const std::vector<Filter> filters1 = GenerateThinningConditionalFilter();

    // Create a thinning unconditional filter for second stage
    const std::vector<Filter> filters2 = GenerateThinningShrinkingUnconditionalFilter();

    constexpr int maxIterations = 200;
    bool converged = false;
    Image img(binarizedImage);

    int iteration = 0;
    while (!converged && iteration < maxIterations)
    {
        ApplyMorphological(img, filters1, filters2, converged);
        if (!img.ExportRAW(inputFilenameNoExtension + "_thin_" + std::to_string(iteration + 1) + ".raw"))
            return -1;
        iteration++;
        std::cout << "Completed iteration " << iteration << " / " << maxIterations << std::endl;
    }

    std::cout << "Done" << std::endl;
    return 0;
}