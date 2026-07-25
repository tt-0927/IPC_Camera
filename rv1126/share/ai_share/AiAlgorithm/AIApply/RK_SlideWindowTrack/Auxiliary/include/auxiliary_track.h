
#ifndef __AUXILLARY_TRACK_H__
#define __AUXILLARY_TRACK_H__

#include "head_detect.h"
#include "rk_slidewindow_track.h"

struct SLIDETRACKPARAM
{
	/* 人头检测模型 */
	char* HeadModelPath = "./weights/HeadDetect.rknn";
	/* 人体提取模型地址 */
	char* FeatureModelPath = "./weights/Deepsort_facenet.rknn";
	/* 初始化窗口的信息 */
	float CLide_cx = 960.0;
	float CLide_cy = 400.0;
	float CLide_cw = 320.0;
	float CLide_ch = 320.0;

	int   nImgWidth = 1920;
	int   nImgHeight = 1024;
	/* 特征余弦相似度阈值 */
	float fSimilarityThreshold = 0.9;
	/* 人头检测的置信度 */
	float fConfidence = 0.8;
	/* 多少帧识别不到自动删除被跟踪人的特征 */
	int nRFreature = 30 ;
};


class AUXILLARY_TRACK
{
	private:
		/*人头检测*/ 
		HEAD_DETECT HeadDetect;

		/*滑动窗口跟踪*/ 
		cv::Mat aSrcImg; 

		/*定义窗口大小*/ 
		RK_ClIDEWINDOW_TRACK ClideTrack;
		
		/*全景图片*/ 
		cv::Mat aFrame;
		
		/* 参数信息初始化 */
		SLIDETRACKPARAM PStrack;
		
		/* 上一帧的人头坐标容器 */
		std::vector<float> vPoints;
		
		/* 用于“多少帧识别不到自动删除被跟踪人的特征”的计数 */
		int nChangeNum = 0;
	public:
		/* 被跟踪人的特征清除 */
		void Clean_feature();
		/* 获取人头坐标和窗口的大小 
		@ fHead_cx,fHead_cy: 对应被跟踪的人头坐标(也是窗口的中心坐标)
		@ fWin_cx,fWin_ch  : 对应窗口的大小
		*/
		void GetDatas(float& fHead_cx,float& fHead_cy,float& fWin_cw,float& fWin_ch){fHead_cx=PStrack.CLide_cx,fHead_cy=PStrack.CLide_cy,fWin_cw=PStrack.CLide_cw,fWin_ch=PStrack.CLide_ch;};
		
		/* 滑动跟踪算法 */
		void AI(char* pInputFrame, std::vector<float>& vResultPoints);

		/* 构造函数 */
		AUXILLARY_TRACK();
		AUXILLARY_TRACK(SLIDETRACKPARAM& Pslidetrack);
		/* 析构函数 */
		~AUXILLARY_TRACK();
};

#endif // __AUXILLARY_TRACK_H__
