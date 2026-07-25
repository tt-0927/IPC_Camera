#include "MoveDetectExt.hpp"
#include "MoveDetectV1_0.hpp"
#include "SaveImage.hpp"

using namespace MoveDetect_NS;
using namespace Inference_NS;

MoveDetect_NS::CMoveDetectV1_0::CMoveDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

MoveDetect_NS::CMoveDetectV1_0::~CMoveDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool MoveDetect_NS::CMoveDetectV1_0::init()
{
    bool bRet = false;

    m_pMoveDetect = new Inference_NS::CMoveDetect();
    if (m_pMoveDetect)
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("[%s] 算法初始化失败 \n",
               "CMoveDetectV1_0");
        goto FAIL;
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool MoveDetect_NS::CMoveDetectV1_0::unInit()
{
    if (m_pMoveDetect)
    {
        delete m_pMoveDetect;
        m_pMoveDetect = nullptr;
    }
    return true;
}

/* 处理数据 */
bool MoveDetect_NS::CMoveDetectV1_0::process(
    InData_S stInData,
    std::vector<std::vector<int>> &vecResult)
{

    if (stInData.inMat.empty())
    {
        printf("传入当前帧为空\n");
        return false;
    }

    if (!m_pMoveDetect)
    {
        printf("未初始化算法类\n");
        return false;
    }

    if (stInData.stParam.erode_size != m_pMoveDetect->erode_size || stInData.stParam.dilate_size != m_pMoveDetect->dilate_size)
    {
        m_pMoveDetect->setParam(stInData.stParam.erode_size, stInData.stParam.dilate_size);
    }

    bool bRet = true;

    if (m_stInParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
        {
            if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
            {
                printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
            }
        }
    }

    /* 前处理 */
    // if (stInData.inMat.channels() != m_nLimitChannel)
    // {
    //     printf("模型需要的通道数和输入当前帧图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]\n",
    //            stInData.inMat.channels(),
    //            m_nLimitChannel);
    //     return false;
    // }
    
    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
    }

    /* 移动帧检测算法 */
    m_pMoveDetect->inference(LastFrameCache, stInData.inMat, vecResult);

    return true;
}
