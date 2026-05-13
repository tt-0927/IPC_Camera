#include "BridgeFracture.h"

using namespace BRIDGEFRACTURE_NS;

int main()
{
	/* 读取图片 */
	cv::Mat img = cv::imread("image.jpg");
	/* 断桥算法类 */
	/* 1、设置 标准线 左上右下 （可以多条标准线） */
	CBridgeFracture demo(10,10,10);
	std::vector<int> vnInputLinePoints = {72,930, 650,290};
	demo.setData(vnInputLinePoints);
	/* 2、调用算法判断断桥的位置 */
	demo.detectBridgeFracture(img);
	cv::line(img, cv::Point(vnInputLinePoints[0], vnInputLinePoints[1]), cv::Point(vnInputLinePoints[2], vnInputLinePoints[3]), cv::Scalar(0, 0, 255), 2);
	/* 3、获取断桥的信息 */
	std::vector<std::vector<int>> vBridgeFractureAreas;
	demo.getBridgeFractureAreas(vBridgeFractureAreas);
	std::cout<<"断桥的数量："<<vBridgeFractureAreas.size()<<std::endl;
	/* 断桥的位置 */
	for(int i=0;i<vBridgeFractureAreas.size();i++)
	{
		std::cout<<"断桥的数量（左上右下）："<<vBridgeFractureAreas[i][0]<<" "<<vBridgeFractureAreas[i][1]<<" "<<vBridgeFractureAreas[i][2]<<" "<<vBridgeFractureAreas[i][3]<<std::endl;
	}
	
	cv::imwrite("result.jpg",img);
	return 0;
}
