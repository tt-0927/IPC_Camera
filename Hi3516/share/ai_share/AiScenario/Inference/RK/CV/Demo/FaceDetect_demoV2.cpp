
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "FaceDetect.hpp"

using namespace InferenceV1_0_NS;

int main()
{
	CCVInferenceBase* demo = new CFaceDetect("./MB_FaceDetector1920x1024.rknn");
	bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
	
	AiScenario_NS::CVData_S stInData;
	stInData.inMat = cv::imread("test.jpg");
	cv::resize(stInData.inMat,stInData.inMat,cv::Size(1920,1024));
    std::vector<float> vOutData;
    bool result = demo->inference(stInData,vOutData);

	printf("输出vOutData.size[%d]\n",vOutData.size());
    for(int i=0;i<vOutData.size()/14;i++)
    {
    	printf("%f ,%f ,%f ,%f\n",vOutData[i*14+0],vOutData[i*14+1],vOutData[i*14+2],vOutData[i*14+3]);
    	cv::rectangle(stInData.inMat, cv::Point(vOutData[i*14+0],vOutData[i*14+1]), cv::Point(vOutData[i*14+2],vOutData[i*14+3]), cv::Scalar(255,0,0), 2);
    }
    cv::imwrite("./FaceDetectDemoV2.jpg", stInData.inMat);
    
    delete demo;
    
	return 0;
}
