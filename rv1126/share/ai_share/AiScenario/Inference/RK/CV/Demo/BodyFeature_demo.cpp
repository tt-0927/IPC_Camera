
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "BodyFeature.hpp"

using namespace InferenceV1_0_NS;

int main()
{
	CCVInferenceBase* demo = new CBodyFeature("./Deepsort_facenet.rknn");
	bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
	
	AiScenario_NS::CVData_S stInData;
	stInData.inMat = cv::imread("test.jpg");
	cv::resize(stInData.inMat,stInData.inMat,cv::Size(64,128));
    std::vector<float> vOutData;
    bool result = demo->inference(stInData,vOutData);

    for(int i=0;i<vOutData.size();i++)
    {
    	printf("%f ",vOutData[i]);
    }
    printf("\n");
    
    delete demo;
    
	return 0;
}
