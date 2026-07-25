#include <iostream>
#include "dbnet_ocr_detect.h"
#include "ocr_repvgg.h"

int main()
{
	/* 1、初始化模型 */
    RK_OCR_DETECT demo("./weights/DBnetOcrDetect.rknn");
    RK_OCR_REPVGG demoRep("./weights/VggOcrREPt.rknn","./weights/TextDict.txt");

    cv::Mat image = cv::imread("./11.jpg", cv::IMREAD_COLOR);
    /* 2、调用文字检测模型，进行检测 */
    std::vector<std::vector<cv::Point2f>> boxes; // 框的坐标
    std::vector<float> new_scores; // 框的得分
    demo.DetectOcrRgb(image,boxes,new_scores);
    
	for(int j=0;j<boxes.size();j++)
	{
		for(int i=0;i<4;i++)
		{
			printf("(%f,%f) ",boxes[j][i].x,boxes[j][i].y);
		}
		printf("\n");
	}
	
	cv::Mat aResult;
	std::vector<std::string> sText;
	std::vector<float> fBox_;
    /* 3、调用文字框，进行识别得到文字 */	
    OCRPRERESULTS OcrData;
	for(int i=0;i<boxes.size();i++)
	{
		/* 文字识别模型 */
		demo.PerspectiveTransform(image, boxes[i],aResult);
		demoRep.DetectOcrRgb(aResult,OcrData);
		// 显示识别到的文字
		std::cout << "文字:" << OcrData.TextRest << std::endl;
		sText.push_back(OcrData.TextRest);
		fBox_.push_back(boxes[i][0].y);
		// 每个文字的得分
		for(int i=0;i<OcrData.vScore.size();i++)
		{
			std::cout << OcrData.vScore[i] <<" ";
		}
		printf("\n");
	}
	
	std::vector<std::pair<float, int>> fBox_y;
	for (int i = 0; i < fBox_.size(); i++) {
        fBox_y.push_back(std::make_pair(fBox_[i], i));
    }
	std::sort(fBox_y.begin(), fBox_y.end());
	for (int i = 0; i < fBox_y.size(); i++) {
        std::cout << "Aare:  (" << boxes[fBox_y[i].second][0].x<<","<< boxes[fBox_y[i].second][0].y  << ")  Data: " << sText[fBox_y[i].second] << std::endl;
    }
	
	// 新的 boxes 向量，用于存储转换后的数据
	std::vector<std::vector<cv::Point>> new_boxes;
	// 遍历原始的 boxes 向量
	for (const auto& box : boxes) {
		// 临时存储转换后的点
		std::vector<cv::Point> new_box;
		for (const auto& point : box) {
		    new_box.push_back(cv::Point(static_cast<int>(point.x), static_cast<int>(point.y)));
		}
		// 将转换后的四边形添加到新的向量中
		new_boxes.push_back(new_box);
	}
	cv::polylines(image, new_boxes, true, cv::Scalar(0, 255, 0), 2);	
    // 将调整后的截图保存到文件
    cv::imwrite("output_image.jpg", image);
    

    return 0;
}

