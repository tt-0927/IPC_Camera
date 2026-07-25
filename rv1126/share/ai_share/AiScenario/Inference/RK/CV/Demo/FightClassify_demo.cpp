
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK_V1_0.hpp"
#include "FightClassify.hpp"
#include <algorithm>

using namespace InferenceV1_0_NS;

int main()
{
	CCVInferenceBase* demo = new CFightClassify("./FightClassify.rknn");
	bool bT = demo->init();
	if(!bT)
	{
		printf("初始化参数识别\n");
		exit(0);
	}
	
	AiScenario_NS::CVData_S stInData;
    cv::Mat img = cv::imread("test.jpg");

    cv::Mat img_resize;
    cv::resize(img, img_resize, cv::Size(320,320));

    // 创建一个大的4D矩阵 (8, 320, 320, 3)
    cv::Mat combined(8, 320 * 320 * 3, CV_8UC1);
    for (int i = 0; i < 8; i++) {
        // 将图像展平为一维向量，并复制到大的4D矩阵中
        cv::Mat flat_img = img_resize.reshape(1, 1);
        flat_img.copyTo(combined.row(i));
    }
    // 将矩阵调整为 (1, 8, 320, 320, 3)
    combined = combined.reshape(3, {1, 8, 320, 320});

    std::cout << "Final shape: " << combined.size << std::endl;



	stInData.inMat = combined;
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
