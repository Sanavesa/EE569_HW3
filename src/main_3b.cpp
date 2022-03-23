/*
#################################################################################################################

# EE569 Homework Assignment #3
# Date: March 10, 2022
# Name: Mohammad Alali
# ID: 5661-9219-82
# email: alalim@usc.edu

#################################################################################################################

    CONSOLE APPLICATION : Defect detection and counting
	
#################################################################################################################

This file will load a grayscale image and detect any defects as well as count them and then finally correct the
input image and export without any defects.

#################################################################################################################

Arguments:
    programName inputFilenameNoExtension width height channels [defectSizeThreshold=50]
    inputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q3b.exe deer 550 691 1 50

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
#include <unordered_set>

#include "Image.h"
#include "Implementations.h"
#include "Filter.h"

// Explores recurisvely the neighbors of the specified position, while ensuring no position is visited twice
void Explore(const Image& image, const size_t row, const size_t column, const uint32_t defectSizeThreshold, std::unordered_set<std::pair<size_t, size_t>, PairHash>& visited)
{
    // If visited is already so large, abort early
    if (visited.size() >= defectSizeThreshold)
        return;

    const auto pair = std::make_pair(row, column);

    // If already visited, dont revisit
    if (visited.find(pair) != visited.end())
        return;

    visited.insert(pair);

    // Explore neighbors
    for (int32_t dv = -1; dv <= 1; dv++)
    {
        for (int32_t du = -1; du <= 1; du++)
        {
            // Skip center
            if (dv == 0 && du == 0)
                continue;

            const int32_t neighborRow = static_cast<int32_t>(row) + dv;
            const int32_t neighborColumn = static_cast<int32_t>(column) + du;
            // Only explore in-bounds black neighboring dots
            if (image.IsInBounds(neighborRow, neighborColumn) && image.GetPixelValue(neighborRow, neighborColumn) == 0)
                Explore(image, static_cast<size_t>(neighborRow), static_cast<size_t>(neighborColumn), defectSizeThreshold, visited);
        }
    }
}

// Calculates the connected region of the given position
std::unordered_set<std::pair<size_t, size_t>, PairHash> FindDefect(const Image& image, const size_t row, const size_t column, const uint32_t defectSizeThreshold)
{
    std::unordered_set<std::pair<size_t, size_t>, PairHash> visited;
    Explore(image, row, column, defectSizeThreshold, visited);
    return visited;
}

// Removes the defect by setting all pixels in the defect to white (255)
void RemoveDefect(Image& image, const std::unordered_set<std::pair<size_t, size_t>, PairHash> defects)
{
    for (const auto& [v, u] : defects)
        image(v, u, 0) = 255;
}

int main(int argc, char *argv[])
{
    // Read the console arguments
    // Check for proper syntax
    if (argc != 5 && argc != 6)
    {
        std::cout << "Syntax Error - Arguments must be:" << std::endl;
        std::cout << "programName inputFilenameNoExtension width height channels [defectSizeThreshold=50]" << std::endl;
        std::cout << "inputFilenameNoExtension is the .raw image without the extension" << std::endl;
        return -1;
    }

	// Parse console arguments
	const std::string inputFilenameNoExtension = argv[1];
	const uint32_t width = (uint32_t)atoi(argv[2]);
	const uint32_t height = (uint32_t)atoi(argv[3]);
	const uint8_t channels = (uint8_t)atoi(argv[4]);
    uint32_t defectSizeThreshold = 50;

    // Parse optional defectSizeThreshold console argument
    if (argc == 6)
        defectSizeThreshold = (uint32_t)atoi(argv[5]);

    // Load input image
    Image inputImage(width, height, channels);
	if (!inputImage.ImportRAW(inputFilenameNoExtension + ".raw"))
		return -1;

    // Binarize the input image
    Image binarizedInputImage = BinarizeImage(inputImage);
    if (!binarizedInputImage.ExportRAW(inputFilenameNoExtension + "_binarized.raw"))
        return -1;

    // Invert the input image
    Image invertedInputImage = Invert(binarizedInputImage);
    if (!invertedInputImage.ExportRAW(inputFilenameNoExtension + "_inv_binarized.raw"))
        return -1;

    // Create a shrinking conditional filter for first stage
    const std::vector<Filter> filters1 = GenerateShrinkingConditionalFilter();

    // Create a shrinking unconditional filter for second stage
    const std::vector<Filter> filters2 = GenerateThinningShrinkingUnconditionalFilter();

    constexpr int maxIterations = 2000;
    bool converged = false;
    Image img(invertedInputImage);

    int iteration = 0;
    while (!converged && iteration < maxIterations)
    {
        ApplyMorphological(img, filters1, filters2, converged); // actually shrinking :P
        if (!img.ExportRAW(inputFilenameNoExtension + "_shrink_" + std::to_string(iteration + 1) + ".raw"))
            return -1;
        iteration++;
        std::cout << "Completed iteration " << iteration << " / " << maxIterations << std::endl;
    }

    // Count number of white dots after shrinking
    std::vector<std::pair<size_t, size_t>> whiteDots;
    for (size_t v = 0; v < img.height; v++)
        for (size_t u = 0; u < img.width; u++)
            if (img(v, u, 0) == 255)
                whiteDots.push_back(std::make_pair(v, u));
    std::cout << "There are " << whiteDots.size() << " white dots." << std::endl;
    
    // Count the defects by checking neighbors of each whiteDot in the original image (defect is <50 px)
    // Also, remove the defects from the binarized image
    Image correctedImage(binarizedInputImage);
    std::vector<std::pair<size_t, size_t>> defects;
    for (const auto &[v, u] : whiteDots)
    {
        const auto defect = FindDefect(binarizedInputImage, v, u, defectSizeThreshold);
        if (defect.size() < defectSizeThreshold)
        {
            std::cout << "Detected defect at (" << u << ", " << v << ") of size " << defect.size() << std::endl;
            RemoveDefect(correctedImage, defect);
            defects.push_back(std::make_pair(v, u));
        }
    }
    std::cout << "There are " << defects.size() << " defects present." << std::endl;

    // Export corrected image
    if (!correctedImage.ExportRAW(inputFilenameNoExtension + "_corrected.raw"))
        return -1;

    std::cout << "Done" << std::endl;
    return 0;
}