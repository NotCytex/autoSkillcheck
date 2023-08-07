#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <thread>
#include <chrono>
#include <Windows.h>
#include "screencapture.h"

screencapture sc;
//cv::Mat src = sc.captureScreenMat(hwnd);

cv::Mat space = cv::imread("./resources/space.png"); // Load the template image
bool detectSpace(cv::Mat& inputImage) {
	cv::Mat grayInput;
	cv::cvtColor(inputImage, grayInput, cv::COLOR_BGR2GRAY); // Convert the input image to grayscale

	cv::Mat result;
	cv::matchTemplate(grayInput, space, result, cv::TM_CCOEFF_NORMED); // Perform template matching

	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc); // Find the location with maximum similarity

	if (maxVal > 0.8) { // If the match is stronger than 80%
		return true;
	}

	return false;
}

cv::Scalar whiteLow = cv::Scalar(184, 185, 185);
cv::Scalar whiteHigh = cv::Scalar(193, 188, 188);
cv::Mat extractWhiteBoxContour(const cv::Mat& inputImage) {
	cv::Mat background;
	inputImage.copyTo(background);

	cv::cvtColor(inputImage, inputImage, cv::COLOR_BGR2RGB);

	cv::Mat mask;
	cv::inRange(inputImage, whiteLow, whiteHigh, mask);
	cv::imshow("mask", mask);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(mask, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	std::vector<cv::Point> whiteBoxContour;
	for (const auto& contour : contours) {
		double contourArea = cv::contourArea(contour);
		cv::Rect boundingRect = cv::boundingRect(contour);
		double aspectRatio = static_cast<double>(boundingRect.width) / boundingRect.height;

		// Criteria to filter the white box contour
		if (contourArea > 5 && contourArea < 200 &&
			boundingRect.width > 5 && boundingRect.height > 5 &&
			boundingRect.width < 200 && boundingRect.height < 200) {

			std::cout << contourArea << std::endl;
			std::cout << boundingRect.width << std::endl;
			std::cout << boundingRect.height << std::endl;

			whiteBoxContour = contour;
			break; // Assuming there is only one white box, exit the loop after finding it
		}
	}

	cv::Mat outputImage = inputImage.clone();
	if (!whiteBoxContour.empty()) {
		cv::drawContours(outputImage, std::vector<std::vector<cv::Point>>{whiteBoxContour}, -1, cv::Scalar(0, 255, 0), 2);
	}
	else {
		std::cout << "None" << std::endl;
	}

	return outputImage;
}
	

int main() {
	//LPCWSTR window_title = L"Skill Check Simulator - Brave";
	////LPCWSTR window_title = L"DeadByDaylight  ";
	//HWND hwnd = FindWindow(NULL, window_title);
	//if (hwnd == NULL) {
	//	std::cerr << "Could not find window. Error: " << GetLastError() << std::endl;
	//	return -1;
	//}
	cv::namedWindow("output", cv::WINDOW_NORMAL);

	//// Declare variables to calculate FPS
	//double fps;
	//cv::TickMeter tm;

	//while (true) {
	//	tm.reset();
	//	tm.start();

	//	cv::Mat src = captureScreenMat(hwnd);
	//	if (detectSpace(src)) {
	//		//std::cout << "Space detected" << std::endl;
	//	}
	//	cv::imshow("output", src);

	//	tm.stop();

	//// Calculate frames per second (FPS)
	//	fps = 1.0 / tm.getTimeSec();
	//	//std::cout << "FPS: " << fps << std::endl;

	//// Break the loop if 'ESC' key is pressed.
	//	if (cv::waitKey(1) == 27)
	//		break;
	//}
	
	
	cv::Mat test = cv::imread("./resources/nig4.png");
	//cv::Mat cropped = cropSpace(test);
	std::cout << detectSpace(test) << std::endl;
	cv::Mat outputImage = extractWhiteBoxContour(test);
	
	
	
	//cv::imshow("cropped", cropped);
	cv::imshow("output", outputImage);

	cv::waitKey(0);
	return 0;
}