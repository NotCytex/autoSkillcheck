#pragma once
#include <opencv2/core.hpp>
#include <Windows.h>

class screencapture {
public:
	BITMAPINFOHEADER createBitmapHeader(float width, float height);
	cv::Mat captureScreenMat(HWND hwnd);
private:
	BITMAPINFOHEADER bi;
};