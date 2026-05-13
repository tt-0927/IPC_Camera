
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "Expression.hpp"
#include <algorithm>

using namespace InferenceV1_0_NS;

int main()
{
	CCVInferenceBase* demo = new CExpression("./Expression.rknn");
	bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
	
	AiScenario_NS::CVData_S stInData;
	stInData.inMat = cv::imread("test.jpg");
	cv::resize(stInData.inMat,stInData.inMat,cv::Size(112,112));
    std::vector<float> vOutData;
    bool result = demo->inference(stInData,vOutData);

    for(int i=0;i<vOutData.size();i++)
    {
    	printf("%f ",vOutData[i]);
    }
    printf("\n");
    
    /* 使用std::max_element找到最大元素的迭代器 */
    auto max_it = std::max_element(vOutData.begin(), vOutData.end());
    // 计算最大元素的下标
    if (max_it != vOutData.end()) 
    {
        size_t max_index = std::distance(vOutData.begin(), max_it);
        std::cout << "最大值的下标是：" << max_index << std::endl;
    } else {
        std::cout << "容器为空，没有最大值" << std::endl;
    }
    
    delete demo;
    
	return 0;
}
