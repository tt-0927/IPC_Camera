/*
 * @Author       : EasonLu
 * @Date         : 2023-10-11 11:32:45
 * @LastEditors  : EasonLu
 * @LastEditTime : 2023-10-12 08:50:43
 * @FilePath     : ai_stu_behavior_interface.cpp
 * @Description  : AI学生行为分析c调用c++接口
 */
#include "ai_stu_behavior_interface_v1.h"
#include "rk_student_behavior_v1.h"
#include <list>
#include <vector>

/* 一组数据的大小 */
#define DATA_GROUP_SIZE 6

/* 全局句柄 */
RK_STUDENT_BEHAVIOR_V1 *g_pStuBehavior = NULL;
/* 按顺序存储分析完毕的数据 */
std::list<std::vector<float>> g_listPos;

int stu_behavior_init(char *pModelPath)
{
    if (g_pStuBehavior == NULL)
    {
        g_pStuBehavior = new RK_STUDENT_BEHAVIOR_V1(pModelPath);
        if (g_listPos.size() > 0)
        {
            g_listPos.clear();
        }
    }
    return 0;
}

int stu_behavior_uninit()
{
    if (g_pStuBehavior)
    {
        delete g_pStuBehavior;
        g_pStuBehavior = NULL;
        if (g_listPos.size() > 0)
        {
            g_listPos.clear();
        }
    }
    return 0;
}

int stu_behavior_get_pos(AiStuBehaviorPos_S **pstPos, int *pTotal)
{
    if (g_listPos.size() > 0)
    {
        /* 获取首个元素数据 */
        auto vec = g_listPos.front();
        /* 移除链表 */
        g_listPos.pop_front();
        /* TODO:转化操作 */
        int nVecSize = vec.size() / DATA_GROUP_SIZE;
        int nMallocSize = sizeof(AiStuBehaviorPos_S) * (nVecSize);
        *pstPos = (AiStuBehaviorPos_S *)malloc(nMallocSize);
        memset(*pstPos, 0, nMallocSize);
        int nIndex = 0;
        for (nIndex = 0; nIndex < nVecSize; nIndex++)
        {
            if (vec.size() > ((nIndex * DATA_GROUP_SIZE) + 1))
            {
                (*pstPos)[nIndex].fX1 = vec[nIndex * DATA_GROUP_SIZE];
                (*pstPos)[nIndex].fY1 = vec[(nIndex * DATA_GROUP_SIZE) + 1];
                (*pstPos)[nIndex].fX2 = vec[(nIndex * DATA_GROUP_SIZE) + 2];
                (*pstPos)[nIndex].fY2 = vec[(nIndex * DATA_GROUP_SIZE) + 3];
                (*pstPos)[nIndex].fIsObject = vec[(nIndex * DATA_GROUP_SIZE) + 4];
                /* 进位强转操作，防止0.999强转时进位为0的问题 */
                (*pstPos)[nIndex].nClass = int(vec[(nIndex * DATA_GROUP_SIZE) + 5] + 0.5);
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

int stu_behavior_send_image(char *pImageData)
{
    if (g_pStuBehavior)
    {
        std::vector<float> vecPos;
        /* 送分析 */
        g_pStuBehavior->StudentBehaviorRgb(pImageData, vecPos);
        /* 存储分析完毕的数据 */
        g_listPos.push_back(vecPos);
        return 0;
    }
    return -1;
}
