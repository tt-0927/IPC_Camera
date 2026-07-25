
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "HumanDetect.hpp"
#include <chrono>

using namespace InferenceV1_0_NS;




class HumanAreaDetect
{
	private:
        /* 模型初始化参数 */
        CCVInferenceBase* model;
        bool bT;
        /* 模型推理的参数 */
        AiScenario_NS::CVData_S stInData;
        std::vector<float> vOutData;
        /* 拌线、入侵、进入、离开的参数 */
        std::map<std::string, std::vector<cv::Point>> tripLinePolygons;       // 拌线检测参数
		std::map<std::string, std::vector<cv::Point>> intrusionZonePolygons;  // 入侵检测参数
		std::map<std::string, std::vector<cv::Point>> entryZonePolygons;      // 进入检测参数
		std::map<std::string, std::vector<cv::Point>> leaveZonePolygons;      // 离开检测参数
        /* 图片前处理 */
        int imageWidth = 0;         // 图片的宽
        int imageHeight = 0;        // 图片的高
        int xOffset = 0;            // 缩放填充后左上角的x坐标
        int yOffset = 0;            // 缩放填充后左上角的y坐标
        float resize_scale = 0.0;   // 缩放比例
        cv::Mat imageRGB;           // 图片的RGB格式
        
    public:
        HumanAreaDetect()
        {
            model = new CHumanDetect("./HumanDetect.rknn");
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

		bool tripLineDetection(
			std::vector<std::vector<int>> Doxes,
			std::vector<cv::Point> LinePoints)
		{
			for (int i = 0; i < Doxes.size(); ++i)
			{
				cv::Point test_point(static_cast<int>((Doxes[i][0] + Doxes[i][2]) / 2),
									 static_cast<int>((Doxes[i][1] + Doxes[i][3]) / 2));
				
			}
		}

		// 判断叉积是否为正，负或者为零
		int crossProduct(const cv::Point &p1, const cv::Point &p2, const cv::Point &p3) {
			return (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
		}

		// 判断一个点是否在两点组成的线段上
		bool isPointOnSegment(const cv::Point &A, const cv::Point &B, const cv::Point &P) {
			return std::min(A.x, B.x) <= P.x && P.x <= std::max(A.x, B.x) &&
				std::min(A.y, B.y) <= P.y && P.y <= std::max(A.y, B.y);
		}

		// Bounding Box 快速排除
		bool isBoundingBoxIntersecting(const cv::Point &A, const cv::Point &B, const cv::Point &C, const cv::Point &D) {
			return std::max(A.x, B.x) >= std::min(C.x, D.x) &&
				std::max(C.x, D.x) >= std::min(A.x, B.x) &&
				std::max(A.y, B.y) >= std::min(C.y, D.y) &&
				std::max(C.y, D.y) >= std::min(A.y, B.y);
		}

		// 判断两条线段 AB 和 CD 是否有交点
		bool isIntersecting(const cv::Point &A, const cv::Point &B, const cv::Point &C, const cv::Point &D) {
			// 首先通过 Bounding Box 快速排除不可能相交的情况
			if (!isBoundingBoxIntersecting(A, B, C, D)) {
				return false;
			}

			// 计算叉积
			int d1 = crossProduct(A, B, C);
			int d2 = crossProduct(A, B, D);
			int d3 = crossProduct(C, D, A);
			int d4 = crossProduct(C, D, B);

			// 如果叉积符号相反，说明线段相交
			if ((d1 * d2 < 0) && (d3 * d4 < 0)) {
				return true;
			}

			// 如果叉积为 0，进一步判断端点是否在线段上
			if (d1 == 0 && isPointOnSegment(A, B, C)) return true; // C 在 AB 上
			if (d2 == 0 && isPointOnSegment(A, B, D)) return true; // D 在 AB 上
			if (d3 == 0 && isPointOnSegment(C, D, A)) return true; // A 在 CD 上
			if (d4 == 0 && isPointOnSegment(C, D, B)) return true; // B 在 CD 上

			return false; // 无交点
		}

		/* 入侵检测 */
		bool intrusionZoneDetection(
			const cv::Point &LastPoint,
			std::vector<cv::Point> Polygons)
		{
			double intrusionResult = cv::pointPolygonTest(Polygons, LastPoint, false);
			if (intrusionResult >= 0)
			{
                return true;   // 入侵
            } else {
				return false;  // 未入侵
			}
		}

		/* 进入检测 */
		bool entryZoneDetection(
			const cv::Point &StartPoint,
			const cv::Point &LastPoint,
			std::vector<cv::Point> Polygons)
		{
			double StartResult = cv::pointPolygonTest(Polygons, StartPoint, false);
			if (StartResult >= 0)
			{
                return false;
            } else {
				double LastResult = cv::pointPolygonTest(Polygons, LastPoint, false);
				if (LastResult >= 0)
				{
					return true;
				} else {
					return false;
				}
			}
		}

		/* 离开检测 */
		bool leaveZoneDetection(
			const cv::Point &StartPoint,
			const cv::Point &LastPoint,
			std::vector<cv::Point> Polygons)
		{
			double StartResult = cv::pointPolygonTest(Polygons, StartPoint, false);
			if (StartResult >= 0)
			{
                double LastResult = cv::pointPolygonTest(Polygons, LastPoint, false);
				if (LastResult >= 0)
				{
					return false;
				} else {
					return true;
				}
            } else {
				return false;
			}
		}


		/* 检测 */
		void Detection(
			const cv::Mat inputImage,
			std::vector<std::vector<int>>& boxes)
        {
			imageWidth = inputImage.cols;
            imageHeight = inputImage.rows;

			/* 前处理1：图像从BGR转换为RGB */
            cv::Mat imageRGB;
            cv::cvtColor(inputImage, imageRGB, cv::COLOR_BGR2RGB);
            /* 前处理2：图片缩放填充 */
            cv::Mat resizeImage = resizeAndPadImage(imageRGB);

			/* 推理 */
			int boxResult = singleImageInference(resizeImage, boxes);

		}
};

int main()
{
	/* 初始化攀爬检测 */
    HumanAreaDetect* demo = new HumanAreaDetect();

	cv::Point A1(100, 100), A2(200, 100); // 线段 A1A2
    cv::Point B1(100, 50), B2(100, 200); // 线段 B1B2
	auto start = std::chrono::high_resolution_clock::now();
	if (demo->isIntersecting(A1, A2, B1, B2)) {
        std::cout << "两条线段相交！" << std::endl;
    } else {
        std::cout << "两条线段不相交！" << std::endl;
    }
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "拌线检测运行时间: " << elapsed.count() << " 毫秒" << std::endl;
    
	std::vector<cv::Point> area = {cv::Point(100, 100), cv::Point(1820, 100), cv::Point(1820, 1050), cv::Point(100, 1050)};
	cv::Point Startp(1900, 500);
	cv::Point Lastp(500, 500);

	auto start2 = std::chrono::high_resolution_clock::now();
	if (demo->intrusionZoneDetection(Lastp, area)) {
        std::cout << "该区域有人入侵！" << std::endl;
    } else {
        std::cout << "该区域无人入侵！" << std::endl;
    }
	auto end2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed2 = end2 - start2;
    std::cout << "入侵检测运行时间: " << elapsed2.count() << " 毫秒" << std::endl;

	auto start3 = std::chrono::high_resolution_clock::now();
	if (demo->entryZoneDetection(Startp, Lastp, area)) {
        std::cout << "这个人是进入！！" << std::endl;
    } else {
        std::cout << "这个人不是进入！" << std::endl;
    }
	auto end3 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed3 = end3 - start3;
    std::cout << "进入检测运行时间: " << elapsed3.count() << " 毫秒" << std::endl;

	auto start4 = std::chrono::high_resolution_clock::now();
	if (demo->leaveZoneDetection(Lastp, Startp, area)) {
        std::cout << "这个人是离开！！" << std::endl;
    } else {
        std::cout << "这个人不是离开！" << std::endl;
    }
	auto end4 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed4 = end4 - start4;
    std::cout << "进入检测运行时间: " << elapsed4.count() << " 毫秒" << std::endl;



	
    delete demo;
    
	return 0;
}
