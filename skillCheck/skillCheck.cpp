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
cv::Mat cropSpace(cv::Mat& inputImage) {
	cv::Mat grayInput, graySpace;
	cv::cvtColor(inputImage, grayInput, cv::COLOR_BGR2GRAY); // Convert the input image to grayscale
	cv::cvtColor(space, graySpace, cv::COLOR_BGR2GRAY); // Convert the space image to grayscale

	cv::Mat result;
	cv::matchTemplate(grayInput, graySpace, result, cv::TM_CCOEFF_NORMED); // Perform template matching

	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc); // Find the location with maximum similarity

	if (maxVal > 0.7) {
		int paddingX = 25;  // horizontal padding
		int paddingY = 68; // vertical padding
		cv::Point topLeft(max(maxLoc.x - paddingX, 0), max(maxLoc.y - paddingY, 0));
		cv::Point bottomRight(min(maxLoc.x + 128 + paddingX, inputImage.cols),
			min(maxLoc.y + 39 + paddingY, inputImage.rows));

		// Create and return the cropped image
		cv::Rect roi(topLeft, bottomRight);
		cv::Mat croppedImage = inputImage(roi).clone();

		return croppedImage;
	}

	return cv::Mat();
}

cv::Rect extractWhiteBoxContour(cv::Mat& inputImage) {
	// Crop the image
	cv::Mat croppedImage = cropSpace(inputImage);
	if (croppedImage.empty()) {
		return cv::Rect();
	}

	cv::Mat background;
	croppedImage.copyTo(background);

	cv::Mat grayCroppedImage;
	cv::cvtColor(croppedImage, grayCroppedImage, cv::COLOR_BGR2GRAY);

	cv::Mat mask;
	cv::threshold(grayCroppedImage, mask, 180, 190, cv::THRESH_BINARY);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(mask, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	cv::Rect whiteBoxRect;
	std::vector<cv::Point> whiteBoxContour;
	for (const auto& contour : contours) {
		double contourArea = cv::contourArea(contour);
		cv::Rect boundingRect = cv::boundingRect(contour);
		double aspectRatio = static_cast<double>(boundingRect.width) / boundingRect.height;

		// Criteria to filter the white box contour
		if (contourArea > 4 &&
			boundingRect.width > 8 && boundingRect.height > 8 &&
			boundingRect.width < 30 && boundingRect.height < 30) {

			std::cout << "Found" << std::endl;

			whiteBoxContour = contour;
			whiteBoxRect = boundingRect;
			break; // Assuming there is only one white box, exit the loop after finding it
		}
	}

	cv::Mat outputImage = croppedImage.clone();
	if (!whiteBoxContour.empty()) {
		cv::drawContours(outputImage, std::vector<std::vector<cv::Point>>{whiteBoxContour}, -1, cv::Scalar(0, 255, 0), 1);
		cv::imshow("test", outputImage);

		return whiteBoxRect;
	}
	
	std::cout << "Failed" << std::endl;
	return cv::Rect();
}
	
bool foundGreatZone = false;
int main() {
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

	// Declare variables to calculate FPS
	double fps;
	cv::TickMeter tm;
	
	while (true) {
		//tm.reset();
		//tm.start();
		cv::Mat src = sc.captureScreenMat(hwnd, roi);

		if (!foundGreatZone) {
			roi = extractWhiteBoxContour(src);
			if (roi.area() > 0) {
				foundGreatZone = true;
			}
			std::cout << roi << std::endl;
		}
		else {
			cv::Mat redMask;
			cv::inRange(src, cv::Scalar(0, 0, 150), cv::Scalar(50, 50, 255), redMask);
			if (cv::countNonZero(redMask) > 10) {
				// Simulate pressing the spacebar
				INPUT input;
				input.type = INPUT_KEYBOARD;
				input.ki.wVk = VK_SPACE;
				input.ki.dwFlags = 0;  // key down
				SendInput(1, &input, sizeof(INPUT));
				input.ki.dwFlags = KEYEVENTF_KEYUP;  // key up
				SendInput(1, &input, sizeof(INPUT));

				foundGreatZone = false;
				roi = cv::Rect(topLeftX, topLeftY, width, height);
			}
		}
		

		//tm.stop();

	// Calculate frames per second (FPS)
		//fps = 1.0 / tm.getTimeSec();
		//std::cout << "FPS: " << fps << std::endl;

	// Break the loop if 'ESC' key is pressed.
		if (cv::waitKey(1) == 27)
			break;
	}

	cv::waitKey(0);
	return 0;
}