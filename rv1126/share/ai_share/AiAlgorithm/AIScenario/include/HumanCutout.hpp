/*
*  File Name:HumanCutout.hpp
*  Created on: 2024年5月17日
*  Author: liaoet
*  description :人像抠图方法 	
*  Modify date: 
*/

#pragma once

#include <AIScenario.hpp>
#include <Model.h>

class HumanCutout : public AIScenario
{
    private:
        /* 图片缩放的参考值，在图片前处理时用到 */
        int m_nRefSize = 512;

        /* 输入图片的高 */
        int m_nImgHeight = 0; 

        /* 输入图片的宽 */
        int m_nImgWidth = 0;

        /* 缩放处理后图片的高 */
        int m_nImgRh = 0;

        /* 缩放处理后图片的宽 */
        int m_nImgRw = 0;

        /* onnx模型路径 */
        std::string m_strModelPath = "/home/user/liaoel/code/c_demo/工厂模式/model_methods/onnx-model/weights/modnet.onnx";

        /* 加载的模型名 */
        Model *m_model;

    public:
        /* 模型预加载 */
        HumanCutout();

        /* 对图片人像抠图 */
        void detection(cv::Mat image) override;
};




