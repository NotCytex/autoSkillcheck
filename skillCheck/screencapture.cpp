#include "screencapture.h";

BITMAPINFOHEADER screencapture::createBitmapHeader(float width, float height) {
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

cv::Mat screencapture::captureScreenMat(HWND hwnd) {
	cv::Mat src;

	// get handles to a device context (DC)
	HDC hwindowDC = GetDC(hwnd);
	HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	// define scale, height and width
	RECT windowRect;
	GetClientRect(hwnd, &windowRect);

	// Set capture area to full dimensions of the window
	int topLeftX = 0;
	int topLeftY = 0;
	int bottomRightX = windowRect.right;
	int bottomRightY = windowRect.bottom;

	int captureWidth = bottomRightX - topLeftX;
	int captureHeight = bottomRightY - topLeftY;

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
