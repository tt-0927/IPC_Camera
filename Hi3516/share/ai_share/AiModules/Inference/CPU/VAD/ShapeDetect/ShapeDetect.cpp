/**
 * @file ShapeDetect.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-24
 *
 * @brief 形状检测（只支持多边形检测）
 */

#include "ShapeDetect.hpp"
using namespace Inference_NS;

CShapeDetect::CShapeDetect()
{
}
CShapeDetect::~CShapeDetect()
{
}

bool CShapeDetect::inference(cv::Mat &aImage, std::vector<std::vector<cv::Point>> &vApproxPolygons)
{
	cv::Mat aGray;
	/* 转为灰度图 */
	cv::cvtColor(aImage, aGray, cv::COLOR_BGR2GRAY);
	/* 二值化 */
	cv::threshold(aGray, aGray, 200, 255, cv::THRESH_BINARY);

	/* 查找轮廓 */
	std::vector<std::vector<cv::Point>> vContours;
	cv::findContours(aGray, vContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	/* 存储逼近的多边形 */
	// std::vector<std::vector<cv::Point>> vApproxPolygons1;
	/* 遍历轮廓 */
	for (size_t i = 0; i < vContours.size(); i++)
	{
		/* 计算轮廓的周长 */
		double dPeri = cv::arcLength(vContours[i], true);

		/* 使用 approxPolyDP 逼近多边形 */
		std::vector<cv::Point> vApprox;
		cv::approxPolyDP(vContours[i], vApprox, 0.02 * dPeri, true);

		/* 将逼近的多边形存入容器 */
		vApproxPolygons.push_back(vApprox);
	}
	return true;
}

bool CShapeDetect::shapeDetect(cv::Mat &aImage, InferParam_S stInferParam, InferRelust_S &stInferRelust)
{
	if (aImage.empty())
	{
		std::cerr << "算法输入图像为空" << std::endl;
		return false;
	}

	/* 转换为灰度图 */
	cv::Mat aGray;
	cv::cvtColor(aImage, aGray, cv::COLOR_BGR2GRAY);
	/* 图像二值化 */
	cv::Mat aBin;
	cv::threshold(aGray, aBin, 125, 255, cv::THRESH_OTSU | cv::THRESH_BINARY_INV);
	/* 查找轮廓 */
	std::vector<std::vector<cv::Point>> vContours;
	cv::findContours(aBin.clone(), vContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	if (vContours.empty())
	{
		return true;
	}

	for (const auto &aContour : vContours)
	{
		/* 计算凸包 */
		std::vector<cv::Point> hull;
		cv::convexHull(aContour, hull);

		/* 计算圆度 */
		double dArea = cv::contourArea(hull);
		double dPerimeter = cv::arcLength(hull, true);
		double dCircularity = (4 * CV_PI * dArea) / (dPerimeter * dPerimeter);
		// std::cout << "圆度: " << dCircularity << std::endl;

		/* 判断是否为圆形或椭圆 */
		if (dCircularity > stInferParam.dRCircularity)
		{
			cv::RotatedRect ellipse = cv::fitEllipse(aContour);
			double dAspectRatio = std::max(ellipse.size.width, ellipse.size.height) / std::min(ellipse.size.width, ellipse.size.height);

			/* 近似圆 */
			if (dAspectRatio < 1.2)
			{
				stInferRelust.nShapeType = Rotundity;
				// cv::circle(aImage, ellipse.center, static_cast<int>(std::max(ellipse.size.width, ellipse.size.height) / 2), cv::Scalar(255, 0, 0), 2);
				// std::cout << "检测到圆形" << std::endl;
			}
			/* 近似椭圆 */
			else
			{
				stInferRelust.nShapeType = Ellipse;
				// cv::ellipse(aImage, ellipse, cv::Scalar(0, 255, 0), 2);
				// std::cout << "检测到椭圆" << std::endl;
			}
			stInferRelust.aEllipse = ellipse;
		}
		else if (dCircularity > stInferParam.dPCircularity)
		{
			stInferRelust.vBoxPoints.clear();

			/* 使用较小的epsilon值提高逼近精度 */
			double epsilon = stInferParam.fEpsilonNum * dPerimeter;
			cv::approxPolyDP(aContour, stInferRelust.vBoxPoints, epsilon, true);

			/* 判断是否为三角形或矩形 */
			if (stInferRelust.vBoxPoints.size() == 3)
			{
				stInferRelust.nShapeType = Triangle;
				// std::cout << "检测到三角形" << std::endl;
			}
			else if (stInferRelust.vBoxPoints.size() == 4)
			{
				/* 检查是否为近似矩形 */
				cv::RotatedRect rect = cv::minAreaRect(aContour);
				cv::Point2f box[4];
				rect.points(box);
				/* 存储端点 */
				stInferRelust.vBoxPoints.clear();
				for (int i = 0; i < 4; i++)
				{
					stInferRelust.vBoxPoints.push_back(box[i]);
				}
				// std::cout << "检测到矩形" << std::endl;
				stInferRelust.nShapeType = Rectangle;
			}
		}
		else
		{
			stInferRelust.nShapeType = NullShape;
		}
	}
	return true;
}