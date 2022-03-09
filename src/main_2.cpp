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

Mat CalculateHMatrix(const std::vector<Point2f> srcPoints, const std::vector<Point2f> destPoints);
void RenderOn(const Image &src, Image &dest, size_t startX, size_t startY);

// Converts the given RGB image into an OpenCV Mat object
Mat RGBImageToMat(const Image& image)
{
    if (image.channels != 3)
    {
        std::cout << "Cannot convert non-RGB image to OpenCV Mat." << std::endl;
        exit(-1);
    }

    Mat mat = Mat::zeros(static_cast<int>(image.height), static_cast<int>(image.width), CV_8UC3);
    for (uint32_t v = 0; v < image.height; v++)
        for (uint32_t u = 0; u < image.width; u++)
        {
            cv::Vec3b &color = mat.at<cv::Vec3b>(v, u);
            // OpenCV uses BGR not RGB
            for (uint32_t c = 0; c < image.channels; c++)
                color[c] = image(v, u, image.channels - c - 1);
            mat.at<cv::Vec3b>(v, u) = color;
        }

    return mat;
}


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

    // Load images as OpenCV Mat
    Mat leftMat = RGBImageToMat(leftInputImage);
    Mat middleMat = RGBImageToMat(middleInputImage);
    Mat rightMat = RGBImageToMat(rightInputImage);




    //-- Step 1: Detect the keypoints using SURF Detector, compute the descriptors
    int minHessian = 400;
    Ptr<SURF> detector = SURF::create( minHessian );
    std::vector<KeyPoint> keypoints1, keypoints2;
    Mat descriptors1, descriptors2;
    detector->detectAndCompute( leftMat, noArray(), keypoints1, descriptors1 );
    detector->detectAndCompute( middleMat, noArray(), keypoints2, descriptors2 );

    //-- Step 2: Matching descriptor vectors with a FLANN based matcher
    // Since SURF is a floating-point descriptor NORM_L2 is used
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::FLANNBASED);
    std::vector< std::vector<DMatch> > knn_matches;
    matcher->knnMatch( descriptors1, descriptors2, knn_matches, 2 );

    //-- Filter matches using the Lowe's ratio test
    const float ratio_thresh = 0.5f;
    std::vector<DMatch> good_matches;
    for (size_t i = 0; i < knn_matches.size(); i++)
    {
        if (knn_matches[i][0].distance < ratio_thresh * knn_matches[i][1].distance)
        {
            good_matches.push_back(knn_matches[i][0]);
        }
    }
    
    //-- Draw matches
    std::cout << "Found " << good_matches.size() << " matches" << std::endl;
    // Mat img_matches;
    // drawMatches( leftMat, keypoints1, middleMat, keypoints2, good_matches, img_matches, Scalar::all(-1),
    //              Scalar::all(-1), std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

    //-- Show detected matches
    // imshow("Good Matches", img_matches );
    // waitKey(0);

    // Print keypoints
    std::vector<Point2f> leftPoints, middlePoints;
    for (const auto &match : good_matches)
    {
        std::cout << "Match: " << match.imgIdx << ", " << match.queryIdx << ", " << match.trainIdx << std::endl;
        std::cout << "keypoints1q[" << match.queryIdx << "] = " << keypoints1[match.queryIdx].pt << std::endl; // left img coord
        std::cout << "keypoints2t[" << match.trainIdx << "] = " << keypoints2[match.trainIdx].pt << std::endl; // middle img coord
        leftPoints.push_back(keypoints1[match.queryIdx].pt);
        middlePoints.push_back(keypoints2[match.trainIdx].pt);
    }

    // Build transformation calculation
    // Mat H = findHomography(leftPoints, middlePoints);
    Mat H = CalculateHMatrix(leftPoints, middlePoints);
    print(H);

    // Create a large enough canvas
    // Output image
    // Mat im_out;
    // // Warp source image to destination based on homography
    // warpPerspective(leftMat, im_out, H, middleMat.size());

    // imshow("out", im_out);
    // waitKey(0);

    Image outputImage(leftInputImage.width * 10, leftInputImage.height * 2, leftInputImage.channels);
    outputImage.Fill(0);

    RenderOn(middleInputImage, outputImage, middleInputImage.width, 0);
    ApplyMatrix(leftInputImage, outputImage, H);
    outputImage.ExportRAW("output.raw");

    TestMe(leftInputImage, H);

    std::cout << "Done" << std::endl;
    return 0;
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

void RenderOn(const Image& src, Image& dest, size_t startX, size_t startY)
{
    for (size_t v = 0; v < src.height; v++)
    {
        for (size_t u = 0; u < src.width; u++)
        {
            size_t x = u + startX;
            size_t y = v + startY;

            if (dest.IsInBounds(static_cast<int32_t>(y), static_cast<int32_t>(x)))
            {
                for (size_t c = 0; c < src.channels; c++)
                    dest(y, x, c) = src(v, u, c);
            }
        }
    }
}