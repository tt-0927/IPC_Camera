
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "HumanCount.hpp"

using namespace InferenceV1_0_NS;

int main()
{
	CCVInferenceBase* demo = new CHumanCount("./HR_human768x640_NWPU.rknn");
	bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
	
	AiScenario_NS::CVData_S stInData;
	stInData.inMat = cv::imread("test.jpg");
	cv::resize(stInData.inMat,stInData.inMat,cv::Size(768,640));
    std::vector<float> vOutData;
    bool result = demo->inference(stInData,vOutData);

    for(int i=0;i<vOutData.size()/2;i++)
    {
    	cv::circle(stInData.inMat, cv::Point(vOutData[i*2+0],vOutData[i*2+1]), 5, cv::Scalar(255,0,0), -1);
    }
    printf("一种有[%d]人\n",vOutData.size()/2);
    cv::imwrite("./HumanCountDemo.jpg", stInData.inMat);
    
    delete demo;
    
	return 0;
}
