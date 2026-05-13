
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "NumberOcr.hpp"
#include <algorithm>

using namespace InferenceV1_0_NS;

int main()
{
	CCVInferenceBase* demo = new CNumberOcr("./NumberOcr.rknn");
	bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
	
	AiScenario_NS::CVData_S stInData;
    cv::Mat img = cv::imread("test.jpg");

    cv::Mat img_resize;
    cv::resize(img, img_resize, cv::Size(128,64));
	cv::cvtColor(img_resize,img_resize,cv::COLOR_RGB2GRAY);
	/* 二值化，这里将128以下的变为0，128以上的变为255 */
	cv::threshold(img_resize, img_resize, 128, 255, cv::THRESH_BINARY);

	stInData.inMat = img_resize;
    std::vector<float> vOutData;
    bool result = demo->inference(stInData,vOutData);

    for(int i=0;i<vOutData.size();i++)
    {
    	printf("识别到的数字：%d ", (int) vOutData[i]);
    }
    printf("\n");
    
    delete demo;
    
	return 0;
}
