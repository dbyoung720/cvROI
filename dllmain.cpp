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

/* �� HBITMAP ת�� Mat */
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

/* ��ȡ����Ȥ���� */
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

	GaussianBlur(img, blurred, Size(5, 5), 0);                                     // ��˹ģ��    
	cvtColor(blurred, hsv, COLOR_BGR2HSV);                                         // ɫ�ʿռ�ת�� 
	inRange(hsv, lower, upper, mask);                                              // ��ֵ��      
	erode(mask, mask, NULL, Point(-1, -1), 5);                                     // ��ʴͼ��    
	dilate(mask, mask, NULL, Point(-1, -1), 5);                                    // ����ͼ��    
	findContours(mask, contours, _hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);  // ������ȡ

	if (contours.size() == 0)
	{
		return false;
	}

	/* ȡ����������ֵ */
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
		maxpt.push_back(contours[max_i]);                    // �������
		Perimeter = arcLength(contours[max_i], true);        // �����ܳ�
		Area      = contourArea(contours[max_i]);            // �������

		Mat hole(img.size(), CV_8UC4, Scalar(0,0,0,0));             // ����ͼ�� 
		drawContours(hole, maxpt, -1, Scalar(255,255,255,255), -1); // ��ɫ�����������
		Mat dst(img.rows, img.cols, CV_8UC4);                       // ����Ŀ��ͼ��
		img.copyTo(dst, hole);                                      // ��ԭͼ�񿽱�������ͼ��  
		SetBitmapBits(hbmp, img.cols * img.rows * 4, dst.data);

		///* ���� ROI ����Ȥ���� */
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
