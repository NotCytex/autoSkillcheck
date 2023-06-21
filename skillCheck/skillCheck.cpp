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

BITMAPINFOHEADER createBitmapHeader(float width, float height) {
	BITMAPINFOHEADER bi;

	// create a bitmap
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 1;
	bi.biYPelsPerMeter = 2;
	bi.biClrUsed = 3;
	bi.biClrImportant = 4;

	return bi;
}

cv::Mat captureScreenMat(HWND hwnd) {
	cv::Mat src;

	// get handles to a device context (DC)
	HDC hwindowDC = GetDC(hwnd);
	HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	// define scale, height and width
	RECT windowRect;
	GetClientRect(hwnd, &windowRect);

	// Define the capture area as a percentage of the window size
	double leftRatio = 0.5781;
	double topRatio = 0.4930;
	double rightRatio = 0.6718;
	double bottomRatio = 0.6527;

	int topLeftX = static_cast<int>(windowRect.right * leftRatio);
	int topLeftY = static_cast<int>(windowRect.bottom * topRatio);
	int bottomRightX = static_cast<int>(windowRect.right * rightRatio);
	int bottomRightY = static_cast<int>(windowRect.bottom * bottomRatio);

	float captureWidth = bottomRightX - topLeftX;
	float captureHeight = bottomRightY - topLeftY;

	// create mat object
	src.create(captureHeight, captureWidth, CV_8UC4);

	// create a bitmap
	HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, captureWidth, captureHeight);
	BITMAPINFOHEADER bi = createBitmapHeader(captureWidth, captureHeight);

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);

	// copy from the window device context to the bitmap device context
	BitBlt(hwindowCompatibleDC, 0, 0, captureWidth, captureHeight, hwindowDC, topLeftX, topLeftY, SRCCOPY);
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, captureHeight, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	// avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}

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

cv::Mat extractWhiteBoxContour(const cv::Mat& inputImage) {
	// Step 1: Preprocess the image
	cv::Mat grayImage;
	cv::cvtColor(inputImage, grayImage, cv::COLOR_BGR2GRAY);

	cv::Mat binaryImage;
	cv::threshold(grayImage, binaryImage, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

	// Step 2: Find contours
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(binaryImage, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	// Step 3: Filter contours based on criteria
	std::vector<cv::Point> whiteBoxContour;

	for (const auto& contour : contours) {
		double contourArea = cv::contourArea(contour);
		cv::Rect boundingRect = cv::boundingRect(contour);
		double aspectRatio = static_cast<double>(boundingRect.width) / boundingRect.height;

		// Criteria to filter the white box contour
		if (contourArea > 100 && contourArea < 200 &&
			boundingRect.width > 10 && boundingRect.height > 10 &&
			boundingRect.width < 80 && boundingRect.height < 80) {
			
			std::cout << contourArea << std::endl;
			std::cout << boundingRect.width << std::endl;
			std::cout << boundingRect.height << std::endl;

			whiteBoxContour = contour;
			break; // Assuming there is only one white box, exit the loop after finding it
		}
	}

	// Step 4: Create an output image with the white box contour
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

	cv::Mat test = cv::imread("./resources/test.png");
	cv::Mat outputImage = extractWhiteBoxContour(test);

	cv::imshow("output", outputImage);

	cv::waitKey(0);
	return 0;
}