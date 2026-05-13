
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "FallDetect.hpp"

using namespace InferenceV1_0_NS;

int main()
{
	CCVInferenceBase* demo = new CFallDetect("./FallDetect.rknn");
	bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
	
	AiScenario_NS::CVData_S stInData;
	stInData.inMat = cv::imread("test.png");
	cv::resize(stInData.inMat,stInData.inMat,cv::Size(640,640));
    std::vector<float> vOutData;
    bool result = demo->inference(stInData,vOutData);

    for(int i=0;i<vOutData.size()/6;i++)
    {
    	// printf("%f ,%f ,%f ,%f ,%f ,%f\n",vOutData[i*6+0],vOutData[i*6+1],vOutData[i*6+2],vOutData[i*6+3],vOutData[i*6+4],vOutData[i*6+5]);
    	cv::rectangle(stInData.inMat, cv::Point(vOutData[i*6+0],vOutData[i*6+1]), cv::Point(vOutData[i*6+2],vOutData[i*6+3]), cv::Scalar(255,0,0), 2);
    }
    cv::imwrite("./FallDetectDemo.jpg", stInData.inMat);
    
    delete demo;
    
	return 0;
}
