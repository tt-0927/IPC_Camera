/*
*  File Name:AIScenario.hpp
*  Created on: 2024年5月17日
*  Author: liaoet
*  description :AI场景的基类 	
*  Modify date: 
*/

#pragma once
#include <iostream>
#include <opencv2/opencv.hpp>

class AIScenario {
public:
    virtual void detection(cv::Mat image) = 0;
};
