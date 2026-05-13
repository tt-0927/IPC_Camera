/*
 * @Author       : EasonLu
 * @Date         : 2023-07-27 13:46:06
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-01-29 17:13:40
 * @FilePath     : opencv_face_analyse_interface.cpp
 * @Description  : 人脸分析的c调用c++接口
 */
#include "opencv_face_analyse_interface.h"
#include "rk_human_count_detect.h"
#include "rk_human_count_process.h"
#include <vector>

RK_COUNT_DETECT *g_rk_detect = NULL;

int face_analyse_init(char *pModelPath)
{
    g_rk_detect = new RK_COUNT_DETECT(pModelPath);
    g_rk_detect->fInstance = 1.0;
    g_rk_detect->nControl = 4;
    g_rk_detect->nDelControl = 4;
    return 0;
}

int face_analyse_uninit()
{
    if (g_rk_detect)
    {
        delete g_rk_detect;
        g_rk_detect = NULL;
    }
    return 0;
}

// int face_analyse_set_param(TunableParam_S stInfo)
// {
//     if (g_rk_detect)
//     {
//         memcpy(g_rk_detect->stTunableParam, &stInfo, sizeof(TunableParam_S));
//     }
//     return 0;
// }

int face_analyse_get_people()
{
    int nPeopleNum = 0;
    if (g_rk_detect)
    {
        g_rk_detect->GetPepleNum(nPeopleNum);
    }
    return nPeopleNum;
}

int face_analyse_get_pos(CvFaceAnalysePos_S **pstPos, int *pPeopleNum)
{
    if (g_rk_detect)
    {
        std::vector<float> vecPos;
        g_rk_detect->GetPeplePoints(vecPos);
        g_rk_detect->MultiFrameResultFusionAlgorithm(vecPos, vecPos);
        int nVecSize = vecPos.size() / 2;
        int nMallocSize = nVecSize * sizeof(CvFaceAnalysePos_S);
        CvFaceAnalysePos_S *pNewPos = (CvFaceAnalysePos_S *)malloc(nMallocSize);
        memset(pNewPos, 0, nMallocSize);
        /* 只获取结构体指针长度的数量，防止内存溢出 */
        int nIndex = 0;
        for (nIndex = 0; nIndex < nVecSize; nIndex++)
        {
            if (vecPos.size() > ((nIndex * 2) + 1))
            {
                pNewPos[nIndex].fX = vecPos[nIndex * 2];
                pNewPos[nIndex].fY = vecPos[(nIndex * 2) + 1];
            }
        }
        *pstPos = pNewPos;
        if(pPeopleNum)
        {
            *pPeopleNum = nVecSize;
        }
        /* 返回已获取的坐标组个数 */
        return nIndex + 1;
    }
    return 0;
}

int face_analyse_bgr(char *pInputData, int nDataLen, unsigned char *pOutData)
{
    if (g_rk_detect)
    {
        g_rk_detect->DetectHumanBgr(pInputData, nDataLen);
    }
    return 0;
}