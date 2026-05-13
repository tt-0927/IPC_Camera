#include "MoveDetectExt.hpp"
#include "MoveDetectV2_0.hpp"
#include "SaveImage.hpp"

using namespace MoveDetect_NS;
using namespace Inference_NS;

MoveDetect_NS::CMoveDetectV2_0::CMoveDetectV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{

}

MoveDetect_NS::CMoveDetectV2_0::~CMoveDetectV2_0()
{
    unInit();
}

/* 初始化 */
bool MoveDetect_NS::CMoveDetectV2_0::init()
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
               "CMoveDetectV2_0");
        goto FAIL;
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool MoveDetect_NS::CMoveDetectV2_0::unInit()
{
    if (m_pMoveDetect)
    {
        delete m_pMoveDetect;
        m_pMoveDetect = nullptr;
    }
    return true;
}

/* 处理数据 */
bool MoveDetect_NS::CMoveDetectV2_0::process(
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

    // 准备当前帧数据 (处理缩放)
    cv::Mat matCurrent;
    
    // 检查是否需要缩放
    bool bNeedResize = (m_nLimitWidth > 0 && m_nLimitHeight > 0) &&
                       (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight);

    if (bNeedResize)
    {
        cv::resize(stInData.inMat, matCurrent, cv::Size(m_nLimitWidth, m_nLimitHeight));
    }
    else
    {
        // 如果不需要缩放，直接克隆一份
        matCurrent = stInData.inMat.clone();
    }

    //动态参数更新
    if (stInData.stParam.erode_size != m_pMoveDetect->erode_size || 
        stInData.stParam.dilate_size != m_pMoveDetect->dilate_size)
    {
        m_pMoveDetect->setParam(stInData.stParam.erode_size, stInData.stParam.dilate_size);
    }

    // 保存图片
    if (m_stInParam.bDebug)
    {
        if (!stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
        {
            if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
            {
                printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
            }
        }
    }

    // 管理上一帧缓存 
    if (LastFrameCache.empty() || LastFrameCache.size() != matCurrent.size())
    {
        // 如果是第一帧，或者分辨率发生变化，重置缓存
        matCurrent.copyTo(LastFrameCache);
        // 第一帧没有前一帧做对比，直接返回，不进行推理
        return true; 
    }

    //执行移动侦测推理
    m_pMoveDetect->inference(LastFrameCache, matCurrent, vecResult);

    // 更新缓存
    matCurrent.copyTo(LastFrameCache);

    return true;
}
