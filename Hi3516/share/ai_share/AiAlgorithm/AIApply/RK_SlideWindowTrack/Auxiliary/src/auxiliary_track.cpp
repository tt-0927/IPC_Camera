#include "auxiliary_track.h"

/* 构造函数 */
AUXILLARY_TRACK::AUXILLARY_TRACK():HeadDetect("./weights/HeadDetect.rknn"),ClideTrack(960.0,400.0,320.0,320.0,"./weights/Deepsort_facenet.rknn")
{
	/* 用于存放滑动窗口 */
	aSrcImg.create(int(PStrack.nImgHeight), int(PStrack.nImgWidth), CV_8UC3);
}
AUXILLARY_TRACK::AUXILLARY_TRACK(SLIDETRACKPARAM& Pslidetrack)
:HeadDetect(Pslidetrack.HeadModelPath),ClideTrack(Pslidetrack.CLide_cx,Pslidetrack.CLide_cy,Pslidetrack.CLide_cw,Pslidetrack.CLide_ch,Pslidetrack.FeatureModelPath)
{
	/* 用于存放滑动窗口 */
	aSrcImg.create(int(Pslidetrack.CLide_ch), int(Pslidetrack.CLide_cw), CV_8UC3);
	/* 信息初始化 */
	PStrack = Pslidetrack;
	
	aFrame.create(int(Pslidetrack.nImgHeight), int(Pslidetrack.nImgWidth), CV_8UC3);
	
	ClideTrack.fSimilarityThreshold = PStrack.fSimilarityThreshold;
}
/* 析构函数 */
AUXILLARY_TRACK::~AUXILLARY_TRACK()
{
	
}

/* 被跟踪人特征清除算法 */
void AUXILLARY_TRACK::Clean_feature()
{
	// 回到第一个特征选取的状态
	ClideTrack._flag = true;
	// 将内存区域清零
	memset(ClideTrack.pMyFeatures, 0, 512 * sizeof(float));  
}

void AUXILLARY_TRACK::AI(char* pInputFrame, std::vector<float>& vResultPoints)
{
	memcpy(aFrame.data, pInputFrame, PStrack.nImgWidth * PStrack.nImgHeight * 3);
	/* 滑动窗口截取 */
	ClideTrack.getImg(aFrame,aSrcImg);
	/* AI算法 */
	std::vector<float> vAIPoints;
	HeadDetect.fBoxThreshold=PStrack.fConfidence;
	HeadDetect.DetectFaceRgb(aSrcImg,vAIPoints);
	/* 识别到任务，更新结果容器 */
	if(vAIPoints.size()/6 != 0)
	{
		vPoints = vAIPoints;
	}
	/* 更新滑动窗口的信息,并获取上一帧的窗口 */
	ClideTrack.nextImg(aFrame, vAIPoints, aSrcImg);
	/* 获取上一帧滑动窗口的位置 */
	int nx = PStrack.CLide_cx;
	int ny = PStrack.CLide_cy;
	/* 获取窗口（被跟踪人头）的位置 */
	ClideTrack.getClide(PStrack.CLide_cx,PStrack.CLide_cy,PStrack.CLide_cw,PStrack.CLide_ch);
	vResultPoints = std::vector<float>{PStrack.CLide_cx,PStrack.CLide_cy,PStrack.CLide_cw,PStrack.CLide_ch};
	/* 判断是否需要重置特征 */
	if((++nChangeNum) >= PStrack.nRFreature)
	{
		/* 达到重置特征的次数，则置0并重置特征 */
		nChangeNum = 0;
		Clean_feature();
	}
	else if(nx==PStrack.CLide_cx && ny==PStrack.CLide_cy)
	{
		nChangeNum += 1; // 重置帧数倒数
	}
	else
	{
		nChangeNum = 0;
	}
}
