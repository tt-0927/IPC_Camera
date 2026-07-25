/*
 * @Author       : chenchl
 * @Date         : 2023-10-11 13:46:06
 * @LastEditors  : chenchl
 * @LastEditTime : 2023-10-11 15:40:38
 * @FilePath     : ai_slideWindow_track_interface.cpp
 * @Description  : 滑动窗口跟踪的c调用c++接口
 */
#include "ai_slideWindow_track_interface.h"
#include "auxiliary_track.h"
#include <vector>
#include <list>


/* 一组数据的大小 */
#define TRACK_DATA_GROUP_SIZE 4

AUXILLARY_TRACK *g_pTrackBehavior = NULL;
/* 按顺序存储分析完毕的数据 */
std::list<std::vector<float>> g_listTrackPos;
SLIDETRACKPARAM Pslidetrack;

/*滑动窗口分析初始化*/
int slideWindow_track_init()
{
    if (g_pTrackBehavior == NULL)
    {
        Pslidetrack.FeatureModelPath = "/opt/rk/modle/Deepsort_facenet.rknn";
        Pslidetrack.HeadModelPath = "/opt/rk/modle/HeadDetect.rknn";
        g_pTrackBehavior = new AUXILLARY_TRACK(Pslidetrack);
        if (g_listTrackPos.size() > 0)
        {
            g_listTrackPos.clear();
        }
    }

    return 0;
}

/*滑动窗口分析反初始化*/
int slideWindow_track_uninit()
{
    if (g_pTrackBehavior)
    {
        delete g_pTrackBehavior;
        g_pTrackBehavior = NULL;
        if (g_listTrackPos.size() > 0)
        {
            g_listTrackPos.clear();
        }
    }
    return 0;
}

int slideWindow_track_get_pos(AiSlideTacksePos_S **pTrackPos, int *pTotal)
{
    if (g_listTrackPos.size() > 0)
    {
        /* 获取首个元素数据 */
        auto vec = g_listTrackPos.front();
        /* 移除链表 */
        g_listTrackPos.pop_front();
        /* TODO:转化操作 */
        int nVecSize = vec.size() / TRACK_DATA_GROUP_SIZE;
        int nMallocSize = sizeof(AiSlideTacksePos_S) * (nVecSize);
        *pTrackPos = (AiSlideTacksePos_S *)malloc(nMallocSize);
        memset(*pTrackPos, 0, nMallocSize);
        int nIndex = 0;
        for (nIndex = 0; nIndex < nVecSize; nIndex++)
        {
            if (vec.size() > ((nIndex * TRACK_DATA_GROUP_SIZE) + 1))
            {
                (*pTrackPos)[nIndex].fX = vec[nIndex * TRACK_DATA_GROUP_SIZE];
                (*pTrackPos)[nIndex].fY = vec[(nIndex * TRACK_DATA_GROUP_SIZE) + 1];
                (*pTrackPos)[nIndex].fW = vec[(nIndex * TRACK_DATA_GROUP_SIZE) + 2];
                (*pTrackPos)[nIndex].fH = vec[(nIndex * TRACK_DATA_GROUP_SIZE) + 3];
            }
        }

        if(pTotal)
        {
            *pTotal = nVecSize;
        }
        /* 返回已获取的坐标组个数 */
        return nIndex;
    }
    return 0;
}


/*送图像数据进行分析*/
int slideWindow_track_send_image(char *pImageData)
{
    if (g_pTrackBehavior)
    {
        std::vector<float> vecPos;
        /* 送分析 */
        g_pTrackBehavior->AI(pImageData, vecPos);

        /*用之前清理掉*/
        g_listTrackPos.clear();
        /* 存储分析完毕的数据 */
        g_listTrackPos.push_back(vecPos);
        return 0;
    }
    return -1;
}