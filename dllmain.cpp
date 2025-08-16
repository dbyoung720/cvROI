// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace cv;
using namespace std;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/* 把 HBITMAP 转成 Mat */
bool HBitmap2Mat(HBITMAP& hbmp, Mat& mat)
{
    BITMAP bmp;
    Mat des_mat;
    GetObject(hbmp, sizeof(BITMAP), &bmp);
    int nChannels = bmp.bmBitsPixel == 1 ? 1 : bmp.bmBitsPixel / 8;
    des_mat = cv::Mat(bmp.bmWidth, bmp.bmHeight, nChannels, (void*)bmp.bmBits, bmp.bmWidthBytes);	
    mat = des_mat;
	  DeleteObject(hbmp);
    return true;
}

/* 获取感兴趣区域 */
extern "C" __declspec(dllexport) bool GetROI(HBITMAP hbmp, double& Perimeter, double& Area, const int lowerR = 0, const int lowerG = 0, const int lowerB = 100, const int upperR = 255, const int upperG = 255, const int upperB = 120)
{
	Mat img;
	HBitmap2Mat(hbmp, img);

	Mat blurred, hsv, mask;
	vector<vector<Point>> contours;
	vector<Vec4i>         _hierarchy;
	InputArray lower = Scalar(lowerB, lowerG, lowerR);
	InputArray upper = Scalar(upperB, upperG, upperR);
	size_t i, cnt;
	int max_i = 0;

	GaussianBlur(img, blurred, Size(5, 5), 0);                                     // 高斯模糊    
	cvtColor(blurred, hsv, COLOR_BGR2HSV);                                         // 色彩空间转换 
	inRange(hsv, lower, upper, mask);                                              // 二值化      
	erode(mask, mask, NULL, Point(-1, -1), 5);                                     // 腐蚀图像    
	dilate(mask, mask, NULL, Point(-1, -1), 5);                                    // 膨胀图像    
	findContours(mask, contours, _hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);  // 轮廓提取

	if (contours.size() == 0)
	{
		return false;
	}

	/* 取最大面积轮廓值 */
	for (cnt = 0, i = 0; i < contours.size(); i++, cnt++)
	{
		if (contourArea(contours[cnt]) > contourArea(contours[max_i]))
			max_i = (int)i;
	}

	if (max_i == 0)
	{
		return false;
	}
	else
	{
		vector<vector<Point>> maxpt;
		maxpt.push_back(contours[max_i]);                    // 最大轮廓
		Perimeter = arcLength(contours[max_i], true);        // 轮廓周长
		Area      = contourArea(contours[max_i]);            // 轮廓面积

		Mat hole(img.size(), CV_8UC4, Scalar(0,0,0,0));             // 遮罩图层 
		drawContours(hole, maxpt, -1, Scalar(255,255,255,255), -1); // 白色像素填充轮廓
		Mat dst(img.rows, img.cols, CV_8UC4);                       // 创建目标图像
		img.copyTo(dst, hole);                                      // 将原图像拷贝进遮罩图层  
		SetBitmapBits(hbmp, img.cols * img.rows * 4, dst.data);

		///* 返回 ROI 感兴趣区域 */
		//vector<Point> poly;
		//approxPolyDP(maxpt, poly, 5, true);

		//Mat tempMask = Mat::zeros(img.size(), CV_8UC4);
		//Mat tempPloy = img.clone();
		//fillConvexPoly(tempMask, poly, Scalar(255, 255, 255, 255));
		//Mat dst(tempPloy & tempMask);
		//SetBitmapBits(hbmp, img.cols * img.rows * 4, dst.data);

		return true;
	}
}
