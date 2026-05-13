#include "auxiliary_track.h"

#include <cmath>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

/* 构造函数 */
AUXILLARY_TRACK::AUXILLARY_TRACK()
    : HeadDetect("./weights/HeadDetect.rknn"), ClideTrack("./weights/Deepsort_facenet.rknn")
{
    /* 定义摄像机模块 */
    // cam = init_udp_cam(ip, nControlPort);
}

AUXILLARY_TRACK::AUXILLARY_TRACK(SLIDETRACKPARAMS& Pslidetrack)
    : HeadDetect(Pslidetrack.HeadModelPath), ClideTrack(Pslidetrack.FeatureModelPath)
{
    /* 信息初始化 */
    PStrack = Pslidetrack;
    aFrame.create(int(Pslidetrack.nImgHeight), int(Pslidetrack.nImgWidth), CV_8UC3);
    ClideTrack.fSimilarityThreshold = PStrack.fSimilarityThreshold;

    /* 定义摄像机模块 */
    // cam = init_udp_cam(ip, nControlPort);
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
    ClideTrack.clear_features();
    // 将内存区域清零
    memset(ClideTrack.pMyFeatures, 0, 512 * sizeof(float));
}

void AUXILLARY_TRACK::AI(char* pInputFrame, std::vector<float>& vResultPoints)
{
    memcpy(aFrame.data, pInputFrame, PStrack.nImgWidth * PStrack.nImgHeight * 3);
    /* 人头检测算法 */
    std::vector<float> vAIPoints;
    HeadDetect.fBoxThreshold = PStrack.fConfidence;
    HeadDetect.DetectFaceRgb(aFrame, vAIPoints);
    if (1 == vAIPoints.size() / 6)
    {
        ClideTrack.m_nNumFeatures = 2;
    }
    else
    {
        ClideTrack.m_nNumFeatures = 20;
    }

    /* 特征匹配得到下一帧被跟踪人头的位置 */
    ClideTrack.nextImg(aFrame, vAIPoints, vResultPoints);

}

/* 判断框选坐标是否存在人 */
bool PeopleChoose(std::vector<float> vResultPoints, std::vector<float> vSelectPoints)
{
    for (int i = 0; i < vSelectPoints.size() / 6; i++)
    {
        float w     = fmax(0.f, fmin(vResultPoints[2], vResultPoints[2 + i * 6]) - fmax(vResultPoints[0], vResultPoints[0 + i * 6]) + 1.0);
        float h     = fmax(0.f, fmin(vResultPoints[3], vResultPoints[3 + i * 6]) - fmax(vResultPoints[1], vResultPoints[1 + i * 6]) + 1.0);
        float fArea = w * h;
        float u     = (vResultPoints[2] - vResultPoints[0] + 1.0) * (vResultPoints[3] - vResultPoints[1] + 1.0) + (vResultPoints[2 + i * 6] - vResultPoints[0 + i * 6] + 1.0) * (vResultPoints[3 + i * 6] - vResultPoints[1 + i * 6] + 1.0) - fArea;
        if (u > 0.2)
        {
            return true;
        }
    }
    return false;
}

/* 优先级人物链表的匹配算法 */


/* 框选跟踪算法 */
int AUXILLARY_TRACK::AI(char* pInputFrame, std::vector<float>& vResultPoints, std::vector<float>& vSelectPoints)
{
    memcpy(aFrame.data, pInputFrame, PStrack.nImgWidth * PStrack.nImgHeight * 3);
    /* 人头检测算法 */
    HeadDetect.fBoxThreshold = PStrack.fConfidence;
    HeadDetect.DetectFaceRgb(aFrame, vSelectPoints);
    
    /* 去掉画面下面四分之一的人框 */
    std::vector<float> vDeleteBox;
    for(int SIndex=0;SIndex<vSelectPoints.size()/6;SIndex++)
    {
    	if(vSelectPoints[SIndex*6+3]>PStrack.nImgHeight*4.0/5)
    	{
    		vDeleteBox.push_back(SIndex);
    	}
    }
    /* 抹去画面下面四分之一的人框 */
    int vDeleteIdex = 0;
    for(int i = vDeleteBox.size()-1; i >= 0; i--)
    {
    	vDeleteIdex = vDeleteBox[i];
    	for (int j = 5; j >= 0; j--)
    	{
			vSelectPoints.erase(vSelectPoints.begin() + vDeleteIdex*6+j); // 删除指定位置的元素
    	}
    }

    std::vector<float> vTargetPos;
    vTargetPos.clear();

    /* 判断是否有框选 */
    if (vResultPoints.size() > 0)
    {
        /* 判断框选坐标是否存在人 */
        if (PeopleChoose(vResultPoints, vSelectPoints))
        {

            /* 框选功能启动 */
            bChoose = true;
            /* 优先级人体特征插入 */
            PRIORITYFEATUES pOnePriorityData;
            pOnePriorityData.vPriorityFeatures = new float[512];
            /* 获取当前人物的特征 */
            ClideTrack.get_head2body_feature(aFrame, vResultPoints[0], vResultPoints[1], vResultPoints[2], vResultPoints[3], pOnePriorityData.vPriorityFeatures);
			// cv::Rect aRc = cv::Rect(int(vResultPoints[0]), int(vResultPoints[1]), int(vResultPoints[2] - vResultPoints[0]), int(vResultPoints[3] - vResultPoints[1]));
			// cv::Mat aResultImg = aFrame.clone()(aRc);
			// cv::imwrite( "./vResultPoints111.jpg",aResultImg);
			pOnePriorityData.Priority = 0;
            if (vPriorityDatas.size() == 0)
            {
                vPriorityDatas.push_back(pOnePriorityData);
            }
            else
            {
                delete[] vPriorityDatas[0].vPriorityFeatures;
                vPriorityDatas[0] = pOnePriorityData;
            }
            /*else
            {
                for(int index=0;index<vPriorityDatas.size();index++)
                {
                    if(pOnePriorityData.Priority < vPriorityDatas[index].Priority)
                    {
                        vPriorityDatas.insert(vPriorityDatas.begin() + index, pOnePriorityData); // 插入指定位置
                        break;
                    }
                    else if(vPriorityDatas[index].Priority==pOnePriorityData.Priority)
                    {
                        vPriorityDatas[index] = pOnePriorityData;
                        break;
                    }
                }
            }*/
        }
        else
        {
            return -1;
        }
    }

    /*满足优先级的标志位*/
    int nflg = 0;

    vResultPoints.clear();
    /* 优先级人物的匹配 */
    if (vSelectPoints.size() / 6 > 0 && vPriorityDatas.size() >= 1)
    {
        float* detectFeatrue  = new float[512];
        float  fMaxSimilarity = 0.0f;
        float  sPointData[7];
        for (int i = 0; i < vSelectPoints.size() / 6; i++)
        {
            ClideTrack.get_head2body_feature(aFrame, vSelectPoints[i * 6 + 0], vSelectPoints[i * 6 + 1], vSelectPoints[i * 6 + 2], vSelectPoints[i * 6 + 3], detectFeatrue);

            float fSimilarity = ClideTrack.cosine_similarity(detectFeatrue, vPriorityDatas[0].vPriorityFeatures, 512);
            if (fSimilarity > 0.98)    // PStrack.fSimilarityThreshold)
            {
                if (fMaxSimilarity <= fSimilarity)
                {
                    fMaxSimilarity = fSimilarity;
                    printf("======满足======%f\n", fSimilarity);
                    nflg = 1;
                    /* 将框选的坐标特征初始化为第一个特征 */
                    for (int nPIndex = 0; nPIndex < 6; nPIndex++)
                    {
                        sPointData[nPIndex] = vSelectPoints[i * 6 + nPIndex];
                        printf("%f ", sPointData[nPIndex]);
                    }
                    printf("\n");

                }
            }
        }
        if (nflg != 0)
        {
            /* 被跟踪人特征清除算法 */
            Clean_feature();
            vTargetPos.clear();
            /* 将框选的坐标特征初始化为第一个特征 */
            for (int nPIndex = 0; nPIndex < 6; nPIndex++)
            {
                vTargetPos.push_back(sPointData[nPIndex]);
            }
			// cv::Rect aRc = cv::Rect(int(vTargetPos[0]), int(vTargetPos[1]), int(vTargetPos[2] - vTargetPos[0]), int(vTargetPos[3] - vTargetPos[1]));
			// cv::Mat aResultImg2 = aFrame.clone()(aRc);
			// cv::imwrite( "./vResultPoints2.jpg",aResultImg2);
        }
        delete[] detectFeatrue;
    }

	/*没有人的时候，回全景*/
	if(0  == vSelectPoints.size() / 6 )
	{
		m_nChangeNum++;

	}
	/*一个人的时候，直接返回该人的位置*/
    else if (1 == vSelectPoints.size() / 6 && nflg == 0)
    {
        /* 将框选的坐标特征初始化为第一个特征 */
        for (int nPIndex = 0; nPIndex < 4; nPIndex++)
        {
            vResultPoints.push_back(vSelectPoints[nPIndex]);
        }
        m_nOneFlg = 1;

    }
	/*多人且没有优先级的时候，就使用上一个人的特征列表进行匹配*/
    else if (vSelectPoints.size() / 6 > 1 && vTargetPos.size() == 0)
    {
        ClideTrack.m_nNumFeatures = 20;
        /*特征匹配得到下一帧被跟踪人头的位置*/ 
        ClideTrack.nextImg(aFrame, vSelectPoints, vResultPoints);
        if(1 == m_nOneFlg)
        {
            /* 被跟踪人特征清除算法 */
            Clean_feature();
            vTargetPos.clear();
            m_nOneFlg = 0;
        }

    }
	/*多人且满足优先级的人的时候，直接返回优先级的位置*/
    else if (nflg != 0)
    {
        ClideTrack.m_nNumFeatures = 20;
        /*特征匹配得到下一帧被跟踪人头的位置*/ 
        ClideTrack.nextImg(aFrame, vTargetPos, vResultPoints);

    }




    return 0;
}
