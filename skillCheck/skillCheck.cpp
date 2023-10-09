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
cv::Mat space = cv::imread("./resources/space.png"); // Load the template image
cv::Mat graySpace;
cv::Rect globalROI;
cv::Rect windowROI;
int prevWhiteBoxWidth = 0;
int prevWhiteBoxHeight = 0;
std::chrono::steady_clock::time_point startTime;
bool timerStarted = false;
const std::chrono::milliseconds resetTime(1580);

std::pair<cv::Mat, cv::Rect> cropSpace(cv::Mat& inputImage) {
	cv::Mat grayInput;
	cv::cvtColor(inputImage, grayInput, cv::COLOR_BGR2GRAY); // Convert the input image to grayscale

	cv::Mat result;
	cv::matchTemplate(grayInput, graySpace, result, cv::TM_CCOEFF_NORMED); // Perform template matching

	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc); // Find the location with maximum similarity

	constexpr double MATCH_THRESHOLD = 0.7;
	if (maxVal <= MATCH_THRESHOLD) {
		return { cv::Mat(), cv::Rect() };
	}

	constexpr int PADDING_X = 25; // Horizontal padding
	constexpr int PADDING_Y = 68; // Vertical padding
	constexpr int TEMPLATE_WIDTH = 128;
	constexpr int TEMPLATE_HEIGHT = 39;

	cv::Point topLeft(max(maxLoc.x - PADDING_X, 0), max(maxLoc.y - PADDING_Y, 0));
	cv::Point bottomRight(min(maxLoc.x + TEMPLATE_WIDTH + PADDING_X, inputImage.cols), min(maxLoc.y + TEMPLATE_HEIGHT + PADDING_Y, inputImage.rows));

	// Ensure that the ROI is within the bounds of the image
	topLeft.x = max(topLeft.x, 0);
	topLeft.y = max(topLeft.y, 0);
	bottomRight.x = min(bottomRight.x, inputImage.cols - 1);
	bottomRight.y = min(bottomRight.y, inputImage.rows - 1);

	// Check if the roi dimensions are valid (non-negative and width and height are positive)
	if (topLeft.x >= bottomRight.x || topLeft.y >= bottomRight.y) {
		// Invalid roi dimensions, return an empty mat
		std::cout << "Invalid roi dimensions: " << std::endl;
		std::cout << "topLeft - x: " << topLeft.x << ", y: " << topLeft.y << ", bottomRight - x: " << bottomRight.x << ", y: " << bottomRight.y << std::endl;
		return { cv::Mat(), cv::Rect() };
	}

	// Now create the ROI with the corrected points
	cv::Rect roi(topLeft, bottomRight);

	try {
		cv::Mat croppedImage = inputImage(roi).clone();
		return { croppedImage, roi };
	}
	catch (...) {
		std::cout << "Crop space issue" << std::endl;
	}

	// Return an empty Mat if we reach here
	return { cv::Mat(), cv::Rect() };
}


cv::Rect extractWhiteBoxContour(const cv::Mat& croppedImage) {

	//cv::Mat croppedImage = cropSpace(inputImage);
	if (croppedImage.empty()) {
		return cv::Rect();
	}

	cv::Mat grayCroppedImage;
	cv::cvtColor(croppedImage, grayCroppedImage, cv::COLOR_BGR2GRAY);

	cv::imshow("Gray Image", grayCroppedImage);

	cv::Mat mask;
	cv::threshold(grayCroppedImage, mask, 180, 190, cv::THRESH_BINARY);

	// Show the binary image
	cv::imshow("Binary Image", mask);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(mask, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	// Create an image to draw the contours
	cv::Mat contourImage = cv::Mat::zeros(croppedImage.size(), CV_8UC3);


	for (const auto& contour : contours) {
		double contourArea = cv::contourArea(contour);
		cv::Rect boundingRect = cv::boundingRect(contour);
		int width = boundingRect.width;
		int height = boundingRect.height;

		//cv::drawContours(contourImage, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 255, 0), 2);

		// Criteria to filter the white box contour
		if (contourArea > 4 && width > 8 && height > 8 && width < 50 && height < 50) {
			std::cout << "Found" << std::endl;

			cv::rectangle(contourImage, boundingRect, cv::Scalar(0, 0, 255), 2);
			cv::imshow("Contours", contourImage);

			return cv::Rect(boundingRect);
		}
	}

	std::cout << "Failed" << std::endl;
	return cv::Rect();
}

void resetROI() {
	globalROI = windowROI;
	timerStarted = false;
	std::cout << "Reset ROI" << std::endl;
}

bool detectRed(const cv::Mat& inputImage) {
	cv::Mat mask;
	cv::inRange(inputImage, cv::Scalar(0, 0, 150), cv::Scalar(30, 30, 175), mask);

	return cv::countNonZero(mask) > 0;
}

void pressSpace() {
	keybd_event(VK_SPACE, 0, 0, 0); // Press
	keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0); // Release
	std::cout << "Space" << std::endl;
}

int main() {
	cv::cvtColor(space, graySpace, cv::COLOR_BGR2GRAY);

	LPCWSTR window_title = L"Skill Check Simulator - Brave";
	//LPCWSTR window_title = L"DeadByDaylight  ";
	HWND hwnd = FindWindow(NULL, window_title);
	if (hwnd == NULL) {
		std::cerr << "Could not find window. Error: " << GetLastError() << std::endl;
		return -1;
	}

	RECT windowRect;
	GetClientRect(hwnd, &windowRect);

	// Set initial values to capture the entire window
	int topLeftX = 0;
	int topLeftY = 0;
	int width = windowRect.right;
	int height = windowRect.bottom;
	
	cv::Rect roi(topLeftX, topLeftY, width, height);
	windowROI = roi;
	globalROI = windowROI;

	// Declare variables to calculate FPS
	double fps;
	cv::TickMeter tm;

	while (true) {
		try {


			tm.reset();
			tm.start();

			cv::Mat src = sc.captureScreenMat(hwnd, globalROI);
			std::pair<cv::Mat, cv::Rect> croppedData = cropSpace(src);
			cv::Mat croppedImage = croppedData.first;
			cv::Rect croppedRoi = croppedData.second;
			cv::Rect whiteBoxRect = extractWhiteBoxContour(croppedImage);

			//std::cout << "globalROI - x: " << globalROI.x << ", y: " << globalROI.y << ", width: " << globalROI.width << ", height: " << globalROI.height << std::endl;

			if (!whiteBoxRect.empty() && !timerStarted) {
				startTime = std::chrono::steady_clock::now();
				timerStarted = true;
				globalROI = cv::Rect(croppedRoi.x + whiteBoxRect.x, croppedRoi.y + whiteBoxRect.y, whiteBoxRect.width, whiteBoxRect.height);
				//cv::rectangle(src, globalROI, cv::Scalar(0, 255, 0), 2); // Draw globalROI in green
				//cv::imshow("Debug", src);
				/*cv::Mat roi = src(globalROI).clone();
				imshow("ROI", roi);*/

				/*if (detectRed(src(globalROI))) {
					pressSpace();
					resetROI();
				}*/
			}

			if (timerStarted) {
				cv::Mat roi = src(globalROI);
				cv::imshow("Debugggg", roi);
				if (detectRed(roi)) {
					pressSpace();
					resetROI();
				}
				else if ((std::chrono::steady_clock::now() - startTime) > resetTime) {
					resetROI();
				}

			}

			tm.stop();

			//Calculate frames per second (FPS)
			fps = 1.0 / tm.getTimeSec();
			//std::cout << "FPS: " << fps << std::endl;
		}
		catch (const cv::Exception& e) {
			std::cerr << "OpenCV Exception caught: " << e.what() << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "Standard exception caught: " << e.what() << std::endl;
		}
		catch (...) {
			std::cerr << "Unknown exception caught!" << std::endl;
		}
		if (cv::waitKey(5) == 27)
			break;
	}

	cv::waitKey(0);
	return 0;
}