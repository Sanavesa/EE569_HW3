/*
#################################################################################################################

# EE569 Homework Assignment #3
# Date: March 10, 2022
# Name: Mohammad Alali
# ID: 5661-9219-82
# email: alalim@usc.edu

#################################################################################################################

    CONSOLE APPLICATION : Object Segmentation and Analysis
	
#################################################################################################################

This file will load an RGB image, converted into it grayscale and then compute the number of objects (beans)
present in the image. Finally, it will also output a segmentation mask for the objects.

#################################################################################################################

Arguments:
    programName inputFilenameNoExtension width height channels
    inputFilenameNoExtension is the .raw image without the extension
Example:
    .\EE569_HW3_Q3c.exe beans 494 82 3

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
void Explore(const Image& image, const size_t row, const size_t column, std::unordered_set<std::pair<size_t, size_t>, PairHash>& visited, const uint8_t intensity, const int32_t sizeLimit)
{
    // If we already reached the size limit of exploration, abort
    if (sizeLimit > 0 && visited.size() >= sizeLimit)
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
            // Only explore in-bounds white neighboring dots
            if (image.IsInBounds(neighborRow, neighborColumn) && image.GetPixelValue(neighborRow, neighborColumn) == intensity)
            {
                Explore(image, static_cast<size_t>(neighborRow), static_cast<size_t>(neighborColumn), visited, intensity, sizeLimit);
            }
        }
    }
}

// Calculates the connected region of the given position
std::unordered_set<std::pair<size_t, size_t>, PairHash> FindIsland(const Image &image, const size_t row, const size_t column, const uint8_t intensity, const int32_t sizeLimit = -1)
{
    std::unordered_set<std::pair<size_t, size_t>, PairHash> visited;
    Explore(image, row, column, visited, intensity, sizeLimit);
    return visited;
}

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

    // Convert input image to grayscale
    Image inputGrayscaleImage = RGB2Grayscale(inputImage);
    if (!inputGrayscaleImage.ExportRAW(inputFilenameNoExtension + "_gray.raw"))
        return -1;

    // Binarize grayscale image
    Image binarizedInputImage = BinarizeImage(inputGrayscaleImage, 220);
    if (!binarizedInputImage.ExportRAW(inputFilenameNoExtension + "_binarized.raw"))
        return -1;

    // Invert image
    Image invertedBinarizedInputImage = Invert(binarizedInputImage);
    if (!invertedBinarizedInputImage.ExportRAW(inputFilenameNoExtension + "_inv_binarized.raw"))
        return -1;

    // --- Shrinking

    // Create a shrinking conditional filter for first stage
    const std::vector<Filter> filters1 = GenerateShrinkingConditionalFilter();

    // Create a shrinking unconditional filter for second stage
    const std::vector<Filter> filters2 = GenerateThinningShrinkingUnconditionalFilter();

    constexpr int maxIterations = 100;
    bool converged = false;
    Image img(invertedBinarizedInputImage);

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

    // Count the beans by checking neighbors of each whiteDot. Only count an island once
    // island = connected component analysis
    std::unordered_set<std::pair<size_t, size_t>, PairHash> visited;
    std::vector<std::pair<size_t, size_t>> beanPoints;
    for (const auto &[v, u] : whiteDots)
    {
        const auto pair = std::make_pair(v, u);
        // If not already visited, visit
        if (visited.find(pair) == visited.end())
        {
            Explore(img, v, u, visited, 255, -1);
            beanPoints.push_back(std::make_pair(v, u));
        }
    }
    std::cout << "There are " << beanPoints.size() << " beans present." << std::endl;

    // Construct segmentation mask
    Image segmentationImage(invertedBinarizedInputImage);
    std::unordered_set<std::pair<size_t, size_t>, PairHash> segmentationVisited;
    for (size_t v = 0; v < segmentationImage.height; v++)
    {
        for (size_t u = 0; u < segmentationImage.width; u++)
        {
            // Skip white pixels
            if (invertedBinarizedInputImage(v, u, 0) == 255)
                continue;

            // Skip visited pixels
            const std::pair<size_t, size_t> pair = std::make_pair(v, u);
            if (segmentationVisited.find(pair) != segmentationVisited.end())
                continue;

            // Only fill closed-in black islands with white
            const auto island = FindIsland(invertedBinarizedInputImage, v, u, invertedBinarizedInputImage(v, u, 0), 200);
            for (const auto& point : island)
            {
                segmentationVisited.insert(point);
                if (island.size() < 200)
                    segmentationImage(point.first, point.second) = 255;
            }
        }
    }

    // Export segmentation mask
    if (!segmentationImage.ExportRAW(inputFilenameNoExtension + "_segmask.raw"))
        return -1;

    // Using the bean points, get each beans connected region size
    for (const auto &[v, u] : beanPoints)
    {
        const auto island = FindIsland(segmentationImage, v, u, segmentationImage(v, u, 0));
        std::cout << "Bean at " << u << ", " << v << " has a size of " << island.size() << std::endl;
    }

    std::cout << "Done" << std::endl;
    return 0;
}

