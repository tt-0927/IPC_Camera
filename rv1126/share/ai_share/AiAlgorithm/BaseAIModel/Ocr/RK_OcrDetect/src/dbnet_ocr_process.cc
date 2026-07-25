#include "dbnet_ocr_process.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <set>
#include <vector>


void unclip(std::vector<cv::Point> approx, std::vector<cv::Point2f>& box, float unclip_ratio=1.5) 
{
	std::vector<ClipperLib::IntPoint> vInput;
    for (const auto& point : approx) {
        vInput.push_back(ClipperLib::IntPoint(point.x, point.y));
    }


    ClipperLib::Path poly;
    for (const auto& p : vInput) {
        poly.push_back(p);
    }
    float perimeter = 0.0;
    for (int i = 0; i < poly.size() - 1; i++) 
    {
        double dx = poly[i + 1].X - poly[i].X;
        double dy = poly[i + 1].Y - poly[i].Y;
        perimeter += std::sqrt(dx * dx + dy * dy);
    }
    
    float distance = std::abs(ClipperLib::Area(poly)) * unclip_ratio / perimeter;

    ClipperLib::ClipperOffset offset;
    offset.AddPath(poly, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
    std::vector<ClipperLib::Path> expanded;
    offset.Execute(expanded, distance);
    box.clear();
    for (const auto& path : expanded) {
        for (const auto& p : path) {
            box.push_back(cv::Point(p.X, p.Y));
            //printf("(%d,%d) ",p.X, p.Y);
        }
        //printf("\n");
    }
}
double box_score_fast(const cv::Mat& aBitmap, std::vector<cv::Point2f> _box) 
{
    int h = aBitmap.rows;
    int w = aBitmap.cols;
    std::vector<cv::Point2f> box = _box;
    int xmin = std::min(std::min(static_cast<int>(std::floor(box[0].x)), 0), w - 1);
    int xmax = std::min(std::max(static_cast<int>(std::ceil(box[1].x)), 0), w - 1);
    int ymin = std::min(std::min(static_cast<int>(std::floor(box[0].y)), 0), h - 1);
    int ymax = std::min(std::max(static_cast<int>(std::ceil(box[1].y)), 0), h - 1);
    cv::Mat mask = cv::Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);
    for (int i = 0; i < 4; i++) {
        box[i].x -= xmin;
        box[i].y -= ymin;
    }
    std::vector<std::vector<cv::Point>> pts = {{cv::Point(box[0].x, box[0].y), cv::Point(box[1].x, box[1].y), cv::Point(box[2].x, box[2].y), cv::Point(box[3].x, box[3].y)}};
    cv::fillPoly(mask, pts, cv::Scalar(1));
    cv::Scalar mean_val = cv::mean(aBitmap(cv::Range(ymin, ymax + 1), cv::Range(xmin, xmax + 1)), mask);
    return mean_val[0];
}

void get_mini_boxes(const std::vector<cv::Point2f>& contour,std::vector<cv::Point2f>& box,float& min_size) 
{
    cv::RotatedRect bounding_box = cv::minAreaRect(contour);
    std::cout << "Center: " << bounding_box.center.x << ", " << bounding_box.center.y << std::endl;
	std::cout << "Size: " << bounding_box.size.width << ", " << bounding_box.size.height << std::endl;
	std::cout << "Angle: " << bounding_box.angle << std::endl;
    
    std::vector<cv::Point2f> points;
    cv::Mat Rpoints;
    cv::boxPoints(bounding_box, Rpoints);
    for (int i = 0; i < Rpoints.rows; i++) {
		for (int j = 0; j < Rpoints.cols; j++) {
		    points.push_back(cv::Point2f(Rpoints.at<float>(i,j), Rpoints.at<float>(i,j+1)));
		    j++;
		}
	}
    std::sort(points.begin(), points.end(), [](const cv::Point2f& a, const cv::Point2f& b) { 
		return a.x < b.x; 
	});
    int index_1, index_2, index_3, index_4;
    if (points[1].y > points[0].y) {
        index_1 = 0;
        index_4 = 1;
    } else {
        index_1 = 1;
        index_4 = 0;
    }
    if (points[3].y > points[2].y) {
        index_2 = 2;
        index_3 = 3;
    } else {
        index_2 = 3;
        index_3 = 2;
    }
    box = {points[index_1], points[index_2], points[index_3], points[index_4]};
    min_size = std::min(bounding_box.size.width, bounding_box.size.height);
}

int post_process(float *input0,int dest_width,int dest_height,float fThreshold,float fBoxThresh,int nMaxCandidates,float fUnclipRatio,std::vector<std::vector<cv::Point2f>>& boxes,std::vector<float>& new_scores
)
{
	//int dest_width=640;
	//int dest_height=640; 

	int nMinSize = 3;
	//float fThreshold=0.3;
	//float fBoxThresh=0.3;
	//int nMaxCandidates=1000;
	//float fUnclipRatio=2;

    cv::Mat aResultImg(640, 640, CV_32FC1, input0);
	cv::Mat aBitmap(640, 640, CV_32FC1, input0);
	aResultImg.convertTo(aBitmap, CV_8U, 255.0); 
	cv::threshold(aBitmap, aBitmap, fThreshold, 1, cv::THRESH_BINARY);
	
	int height = aResultImg.rows;
    int width = aResultImg.cols;
    
    std::vector<std::vector<cv::Point>> contours;
    
    std::vector<cv::Point> approx;
    cv::findContours(aBitmap * 255, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    for (int i = 0; i < std::min(static_cast<int>(contours.size()), nMaxCandidates); i++) 
    {
        double epsilon = 0.005 * cv::arcLength(contours[i], true); 
        cv::approxPolyDP(contours[i], approx, epsilon, true);
        if (approx.size() < 4) {
            continue;
        }
        float score = box_score_fast(aResultImg, std::vector<cv::Point2f>(contours[i].begin(), contours[i].end()));
        if (fBoxThresh > score) {
            continue;
        }
        std::vector<cv::Point2f> box;
        if (approx.size() > 2) {
            unclip(approx,box, fUnclipRatio);
        } else {
            continue;
        }
		std::vector<cv::Point2f> boxf;
		for (const auto& point : box) {
			boxf.emplace_back(cv::Point2f(point.x, point.y));
		}

		std::vector<cv::Point2f> four_point_box;
		float sside;
		get_mini_boxes(boxf,four_point_box,sside);
        if (sside < nMinSize + 2) {
            continue;
        }
        printf("=============== (%d,%d)\n",width ,dest_width);
        double scale = std::min(static_cast<double>(width) / dest_width, static_cast<double>(height) / dest_height);
        for (auto& point : four_point_box) {
            point.x = std::max(0, std::min(static_cast<int>(std::round(point.x*1.0 / scale)), dest_width));
            point.y = std::max(0, std::min(static_cast<int>(std::round(point.y*1.0 / scale)), dest_height));
        }
        boxes.push_back(four_point_box);
        new_scores.push_back(score);
	}
	printf("(boxes:%d, new_scores:%d)",boxes.size(),new_scores.size());
	return 0;	
	
}

