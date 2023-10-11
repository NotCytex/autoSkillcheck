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
cv::Mat red = cv::imread("./resources/test.png");
cv::Mat graySpace;
cv::Rect globalROI;
cv::Rect windowROI;
int prevWhiteBoxWidth = 0;
int prevWhiteBoxHeight = 0;
std::chrono::steady_clock::time_point startTime;
bool timerStarted = false;
const std::chrono::milliseconds resetTime(1590);

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

	constexpr int PADDING_X = 33; // Horizontal padding
	constexpr int PADDING_Y = 73 ; // Vertical padding
	constexpr int TEMPLATE_WIDTH = 127;
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


	cv::Mat croppedImage = inputImage(roi);  

	cv::Point rectTopLeft((croppedImage.cols - 127) / 2, (croppedImage.rows - 39) / 2);
	cv::Point rectBottomRight(rectTopLeft.x + 127, rectTopLeft.y + 39);

	cv::rectangle(croppedImage, rectTopLeft, rectBottomRight, cv::Scalar(0, 0, 0), -1); // -1 means fill the rectangle
	return { croppedImage, roi };
}


cv::Rect extractWhiteBoxContour(const cv::Mat& croppedImage) {
	if (croppedImage.empty()) {
		return cv::Rect();
	}

	cv::Mat grayCroppedImage;
	cv::cvtColor(croppedImage, grayCroppedImage, cv::COLOR_BGR2GRAY);

	//cv::imshow("Gray Image", grayCroppedImage);

	cv::Mat mask;
	cv::threshold(grayCroppedImage, mask, 250 , 255, cv::THRESH_BINARY);

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

		cv::drawContours(contourImage, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(0, 255, 0), 2);

		// Criteria to filter the white box contour
		//if (contourArea > 40 && width > 18 && height > 18 && width < 30 && height < 30) {
		if (contourArea >= 70 && width >= 8 && height >= 8 && width <= 100 && height <= 100) {
			std::cout << "Found" << std::endl;

			cv::rectangle(contourImage, boundingRect, cv::Scalar(0, 0, 255), 2);
			cv::imshow("Contours", contourImage);

			/*std::cout << "Width" << width << std::endl;
			std::cout << "Height" << height << std::endl;
			std::cout << "Area" << contourArea << std::endl;*/

			return cv::Rect(boundingRect);
		}
	}

	std::cout << "Failed" << std::endl;
	return cv::Rect();
}

void resetROI() {
	globalROI = windowROI;
	timerStarted = false;
	//std::cout << "Reset ROI" << std::endl;
}

bool detectRed(const cv::Mat& inputImage) {
	const int rows = inputImage.rows;
	const int cols = inputImage.cols;

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			cv::Vec3b pixel = inputImage.at<cv::Vec3b>(y, x);

			if (pixel[2] > 160 && pixel[2] < 255 && pixel[0] > 0 && pixel[1] < 30 && pixel[0] > 0  && pixel[0] < 30) {
				return true;
			}
		}
	}
	return false;
}

void pressSpace() {
	// Setup the input event
	INPUT input[2] = {};
	input[0].type = INPUT_KEYBOARD;
	input[0].ki.wVk = VK_SPACE;

	input[1] = input[0];
	input[1].ki.dwFlags = KEYEVENTF_KEYUP;

	// Send the input event (press and release space key)
	SendInput(2, input, sizeof(INPUT));
}

int main() {
	cv::cvtColor(space, graySpace, cv::COLOR_BGR2GRAY);

	//LPCWSTR window_title = L"Skill Check Simulator - Brave";
	LPCWSTR window_title = L"DeadByDaylight  ";
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
	//double fps;
	//cv::TickMeter tm;

	extractWhiteBoxContour(red);

	while (true) { 
		//tm.reset();
		//tm.start();

		//std::cout << "globalROI - x: " << globalROI.x << ", y: " << globalROI.y << ", width: " << globalROI.width << ", height: " << globalROI.height << std::endl;
		cv::Mat src = sc.captureScreenMat(hwnd, globalROI);
		//cv::imshow("Source", src);
		//std::cout << "Width: " << src.cols << ", Height: " << src.rows << std::endl;

		if (timerStarted) {
			if (detectRed(src)) {
				pressSpace();
				cv::imshow("Source", src); 
				resetROI(); 
			}
			else if ((std::chrono::steady_clock::now() - startTime) > resetTime) {
				resetROI();
			}

		}

		else {
			std::pair<cv::Mat, cv::Rect> croppedData = cropSpace(src);
			cv::Mat croppedImage = croppedData.first;
			cv::Rect croppedRoi = croppedData.second;
			cv::Rect whiteBoxRect = extractWhiteBoxContour(croppedImage);

			if (!whiteBoxRect.empty()) {
				startTime = std::chrono::steady_clock::now();
				timerStarted = true;
				globalROI = cv::Rect(croppedRoi.x + whiteBoxRect.x, croppedRoi.y + whiteBoxRect.y, whiteBoxRect.width - 4, whiteBoxRect.height - 4);
			}
		}

		//tm.stop();

		//Calculate frames per second (FPS)
		//fps = 1.0 / tm.getTimeSec();
		//std::cout << "FPS: " << fps << std::endl;
		
		if (cv::waitKey(5) == 27)
			break;
	}

	cv::waitKey(0);
	return 0;
}