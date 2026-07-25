
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "HeadDetect.hpp"
#include <iostream>

using namespace InferenceV1_0_NS;

class ClassroomMoveDetect
{
    private:
        /* 模型初始化参数 */
        CCVInferenceBase* model;
        bool bT;
        /* 模型推理的参数 */
        AiScenario_NS::CVData_S stInData;
        std::vector<float> vOutData;
        /* 存放上一帧的目标框 */
        std::vector<std::vector<int>> Foreboxs;
        /* 图片前处理 */
        int imageWidth = 0;         // 图片的宽
        int imageHeight = 0;        // 图片的高
        int xOffset = 0;            // 缩放填充后左上角的x坐标
        int yOffset = 0;            // 缩放填充后左上角的y坐标
        float resize_scale = 0.0;   // 缩放比例
        cv::Mat imageRGB;           // 图片的RGB格式
        
    public:
        ClassroomMoveDetect()
        {
            model = new CHeadDetect("./HeadDetect.rknn");
            bT = model->init();
            if(!bT)
            {
                printf("初始化参数识别\n");
                exit(0);
            }
        }

		/* 单张图片的推理 */
        bool singleImageInference(
			const cv::Mat& inputImage,
			std::vector<std::vector<int>>& nfOutputLocations)
        {
            /* 推理 */
            stInData.inMat = inputImage;
            bool result = model->inference(stInData,vOutData);
            for (int i=0;i<vOutData.size()/6;i++)
            {
                /* 将检测结果返回到原图的坐标 */
                int x1 = static_cast<int>((vOutData[i*6+0] - xOffset) / resize_scale);
                int y1 = static_cast<int>((vOutData[i*6+1] - yOffset) / resize_scale);
                int x2 = static_cast<int>((vOutData[i*6+2] - xOffset) / resize_scale);
                int y2 = static_cast<int>((vOutData[i*6+3] - yOffset) / resize_scale);

				std::vector<int> to_add = {x1, y1, x2, y2};
                nfOutputLocations.push_back(to_add);

            }
            vOutData.clear();
            return true;
        }

		/* 将图片进行缩放填充 */
        cv::Mat resizeAndPadImage(const cv::Mat inputImage)
        {
            resize_scale = 640.0f / std::max(imageWidth, imageHeight);

            int newWidth = static_cast<int>(imageWidth * resize_scale);
            int newHeight = static_cast<int>(imageHeight * resize_scale);

            cv::Mat resizedImage;
            cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

            cv::Mat outputImage = cv::Mat::zeros(640, 640, inputImage.type());

            xOffset = static_cast<int>((640 - newWidth) / 2);
            yOffset = static_cast<int>((640 - newHeight) / 2);

            resizedImage.copyTo(outputImage(cv::Rect(xOffset, yOffset, newWidth, newHeight)));

            return outputImage;
        }

		/* 计算IOU */
        double CalculateOverlap(
			const std::vector<int>& box1,
			const std::vector<int>& box2)
        {
			// 计算交集区域的宽和高
			int inter_width = fmax(0, fmin(box1[2], box2[2]) - fmax(box1[0], box2[0]));
			int inter_height = fmax(0, fmin(box1[3], box2[3]) - fmax(box1[1], box2[1]));
			// 交集面积
			int inter_area = inter_width * inter_height;
			// 计算两个框的面积
			int box1_area = (box1[2] - box1[0]) * (box1[3] - box1[1]);
			int box2_area = (box2[2] - box2[0]) * (box2[3] - box2[1]);
			// 并集面积
			int union_area = box1_area + box2_area - inter_area;

            return (union_area != 0) ? static_cast<double>(inter_area) / union_area : 0.0;
        }

		double iou_filter(
			std::vector<std::vector<int>>& boxes1,
			std::vector<std::vector<int>>& boxes2,
			double iou_threshold = 0.25)
		{
			int size1 = boxes1.size();
			int size2 = boxes2.size();
			// 标记数组，用于记录哪些框已经被移除
			std::vector<bool> remove_flag1(size1, false);
			std::vector<bool> remove_flag2(size2, false);
			// 遍历两个列表的矩形框
			for (int i = 0; i < size1; ++i) {
				if (remove_flag1[i]) continue;  // 如果框已经被标记为移除，跳过

				for (int j = 0; j < size2; ++j) {
					if (remove_flag2[j]) continue;  // 如果框已经被标记为移除，跳过

					// 计算IOU
					double iou = CalculateOverlap(boxes1[i], boxes2[j]);
					// std::cout << "iou: " << iou << std::endl;

					if (iou > iou_threshold) {
						// 如果IOU大于阈值，标记这两个框为移除
						remove_flag1[i] = true;
						remove_flag2[j] = true;
						break;  // 找到匹配后，跳出内层循环，避免不必要的计算
					}
				}
			}
			// 计算剩余未被移除的框数量
			int remaining1 = std::count(remove_flag1.begin(), remove_flag1.end(), false);
			int remaining2 = std::count(remove_flag2.begin(), remove_flag2.end(), false);
			// 返回过滤后的比例
			return static_cast<double>(remaining1 + remaining2) / (size1 + size2);
		}

		/* 检测 */
		double Detection(const cv::Mat inputImage)
        {
			imageWidth = inputImage.cols;
            imageHeight = inputImage.rows;

			/* 前处理1：图像从BGR转换为RGB */
            cv::Mat imageRGB;
            cv::cvtColor(inputImage, imageRGB, cv::COLOR_BGR2RGB);
            /* 前处理2：图片缩放填充 */
            cv::Mat resizeImage = resizeAndPadImage(imageRGB);

			/* 推理 */
			std::vector<std::vector<int>> boxes;
			int boxResult = singleImageInference(resizeImage, boxes);

			if (Foreboxs.empty())
			{
				Foreboxs = boxes;
			}

			double result = iou_filter(boxes, Foreboxs, 0.5);
			/* 替换目标框 */
			Foreboxs = boxes;
			return result;
		}
};



int main()
{
	/* 初始化攀爬检测 */
    ClassroomMoveDetect* demo = new ClassroomMoveDetect();

    /* 读取图片 */
    cv::Mat img1 = cv::imread("test1.png");
	cv::Mat img2 = cv::imread("test2.png");

    /* 检测 */
    double result1 = demo->Detection(img1);
	double result2 = demo->Detection(img2);

	std::string text1 = "Probability: " + std::to_string(result1);
	cv::putText(img1, text1, cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);

	std::string text2 = "Probability: " + std::to_string(result2);
	cv::putText(img2, text2, cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);


    cv::imwrite("./result1.jpg", img1);
	cv::imwrite("./result2.jpg", img2);
    
    delete demo;
    
	return 0;
}
