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
		int paddingY = 65; // vertical padding
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


cv::Scalar whiteLow = cv::Scalar(180, 180, 180);
cv::Scalar whiteHigh = cv::Scalar(195, 195, 195);
cv::Mat extractWhiteBoxContour(cv::Mat& inputImage) {
	// Crop the image
	cv::Mat croppedImage = cropSpace(inputImage);
	if (croppedImage.empty()) {
		return cv::Mat();
	}

	cv::Mat background;
	croppedImage.copyTo(background);

	cv::cvtColor(croppedImage, croppedImage, cv::COLOR_BGR2RGB);

	cv::Mat mask;
	cv::inRange(croppedImage, whiteLow, whiteHigh, mask);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(mask, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	std::vector<cv::Point> whiteBoxContour;
	for (const auto& contour : contours) {
		double contourArea = cv::contourArea(contour);
		cv::Rect boundingRect = cv::boundingRect(contour);
		double aspectRatio = static_cast<double>(boundingRect.width) / boundingRect.height;

		// Criteria to filter the white box contour
		if (boundingRect.width > 8 && boundingRect.height > 8 &&
			boundingRect.width < 20 && boundingRect.height < 20) {

			std::cout << "Found" << std::endl;

			whiteBoxContour = contour;
			break; // Assuming there is only one white box, exit the loop after finding it
		}
	}

	cv::Mat outputImage = croppedImage.clone();
	if (!whiteBoxContour.empty()) {
		cv::drawContours(outputImage, std::vector<std::vector<cv::Point>>{whiteBoxContour}, -1, cv::Scalar(0, 255, 0), 2);
		//cv::imshow("test", outputImage);]
		return outputImage;
	}
	
	std::cout << "Failed" << std::endl;
	return cv::Mat();
}
	

int main() {
	LPCWSTR window_title = L"Skill Check Simulator - Brave";
	//LPCWSTR window_title = L"DeadByDaylight  ";
	HWND hwnd = FindWindow(NULL, window_title);
	if (hwnd == NULL) {
		std::cerr << "Could not find window. Error: " << GetLastError() << std::endl;
		return -1;
	}

	// Declare variables to calculate FPS
	double fps;
	cv::TickMeter tm;

	while (true) {
		//tm.reset();
		//tm.start();

		cv::Mat src = sc.captureScreenMat(hwnd);
		cv::Mat output = extractWhiteBoxContour(src);
		cv::imshow("Output", src);

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