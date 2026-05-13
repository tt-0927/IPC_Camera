
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "BYTETracker.h"

int main()
{
	cv::Mat aImage = cv::imread("test.jpg");
	cBYTETracker* tracker = new cBYTETracker();
	/* （1）配置相关的信息 */
	/**
	* @brief 设置跟踪相关的信息
	* @param [float] fTrackThresh[0-1,0.8]: 追踪阈值，这个值用于设置初始目标检测的置信度阈值。
	* @param [float] fHighThresh[0-1]: 高置信度阈值，用于确定哪些检测结果非常可靠。
	* @param [float] fMatchThresh[0-1]: 匹配阈值，在目标跟踪过程中，这个值用于决定两帧之间跟踪目标是否匹配。
	* @param [int] nFrameId: 起始的ID
	* @param [int] nMaxTimeLost[>0]: 最大丢失时间，这个变量决定跟踪对象在连续几帧未能匹配到检测结果时，会被认为丢失。
	* @return 
	* @note
	*/
	tracker->setValue(0.8, 0.6, 0.8, 0, 30);

	/* （2）模拟目标检测数据 */
	std::vector<DetectResult_S> results;
	/* 2个框 */
	for (int i = 0; i < 2; i++)
	{
		DetectResult_S result;
		float x1,y1,x2,y2;
		if(i==0)
		{
			x1 = 100;
			y1 = 100;
			x2 = 200;
			y2 = 200;
			result.fConfidence = 0.8;
			result.nClassId = 0;  
		}
		else
		{
			x1 = 120;
			y1 = 120;
			x2 = 220;
			y2 = 220;
			result.fConfidence = 0.9;
			result.nClassId = 0;  
		}
		result.vfBox = cv::Rect_<float>{x1, y1, x2-x1, y2-y1};       
		results.push_back(result);   
	}


	/* （3）更新坐标，重新获取ID */
    std::vector<cSTrack> output_stracks = tracker->update(results);

	/* （4）遍历更新后的容器，绘制框 */
    for (int i = 0; i < output_stracks.size(); i++)
    {
		/* 得到[左上x，坐上y，宽，高] 值 */
        std::vector<float> tlwh = output_stracks[i].tlwh;
		/* 随机获取绘制颜色 */
        cv::Scalar s = tracker->getColor(output_stracks[i].track_id);

        cv::putText(aImage, cv::format("%d", output_stracks[i].track_id), cv::Point(tlwh[0], tlwh[1] - 5),
                0, 0.6, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        cv::rectangle(aImage, cv::Rect(tlwh[0], tlwh[1], tlwh[2], tlwh[3]), s, 2); 
    }

    cv::imwrite("./ByteTrackDemo.jpg", aImage);
    
    delete tracker;
    
	return 0;
}
