
#ifndef __AUXILLARY_TRACK_H__
#define __AUXILLARY_TRACK_H__

#include <cmath>
#include "head_detect.h"
#include "rk_slidewindow_track.h"

#include <unistd.h>
// extern "C" {
// #include "camera.h"
// };

struct SLIDETRACKPARAMS
{
	/* 人头检测模型 */
	char* HeadModelPath = "./weights/HeadDetect.rknn";
	/* 人体提取模型地址 */
	char* FeatureModelPath = "./weights/Deepsort_facenet.rknn";
	/* 视频流输入的像素大小 */
	int   nImgWidth = 1920;
	int   nImgHeight = 1024;
	/* 特征余弦相似度阈值 */
	float fSimilarityThreshold = 0.6;
	/* 人头检测的置信度 */
	float fConfidence = 0.7;
	/* 多少帧识别不到自动删除被跟踪人的特征 */
	int nRFreature = 30 ;
};

struct PRIORITYFEATUES
{
	/* 优先级，0-99（0最大） */
	int Priority;
	/* 人体提取特征 */
	float* vPriorityFeatures;
};


class AUXILLARY_TRACK
{
	private:
		/* ==================== IP和端口初始化 ========================*/
		// char *ip = "172.16.25.207";
		// int nControlPort = 1259;
		// /* 初始化 */
		// Strategy_CamControl_t* cam;
		/* ==================== IP和端口初始化 ========================*/

		/*人头检测*/ 
		HEAD_DETECT HeadDetect;;
		/*滑动窗口跟踪*/ 
		RK_ClIDEWINDOW_TRACK ClideTrack;
		
		/*全景图片*/ 
		cv::Mat aFrame;
		
		/* 参数信息初始化 */
		SLIDETRACKPARAMS PStrack;
		
		/* 用于“多少帧识别不到自动删除被跟踪人的特征”的计数 */
		int m_nChangeNum = 0;
		
		/* 是否启动人头框选功能 */
		bool bChoose = false;
		
		/* 优先级人的特征链表（包含人脸识别+页面点击的） */
		std::vector<PRIORITYFEATUES> vPriorityDatas;

		/*上一次是不是一个人的标志位*/
		int m_nOneFlg = 0;
		
	public:
		/* 被跟踪人的特征清除 */
		void Clean_feature();

		/* 摄像头滑动跟踪算法 */
		void AI(char* pInputFrame, std::vector<float>& vResultPoints);
		/**
		 * 摄像头滑动跟踪算法+框选
		 * @param pInputFrame 视频帧
		 * @param vResultPoints 框选的左上角和右下角坐标（x1,y1,x2,y2），没有框选则为空。
		 * @param vSelectPoints 框选的坐标，没有框选则为空
		 * @return 返回值为非负数则成功，否则失败
		 */
		int AI(char* pInputFrame, std::vector<float>& vResultPoints, std::vector<float>& vSelectPoints);
		/* 构造函数 */
		AUXILLARY_TRACK();
		AUXILLARY_TRACK(SLIDETRACKPARAMS& Pslidetrack);
		/* 析构函数 */
		~AUXILLARY_TRACK();
};

#endif // __AUXILLARY_TRACK_H__
