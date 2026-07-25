#pragma once

#include "ClipTextV2_0.hpp"
#include "SaveImage.hpp"

ClipText_NS::CClipTextV2_0::CClipTextV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

ClipText_NS::CClipTextV2_0::~CClipTextV2_0()
{
    unInit();
}

/* 初始化 */
bool ClipText_NS::CClipTextV2_0::init()
{
    bool bRet = false;

    m_pClipText = new Inference_NS::CTextFeature(m_stInParam.strModelPath);
    if (m_pClipText)
    {
        bRet = m_pClipText->init();
    }
    else
    {
        printf("模型初始化失败 [%s]\n",
               m_stInParam.strModelPath.c_str());
        goto FAIL;
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool ClipText_NS::CClipTextV2_0::unInit()
{
    if (m_pClipText)
    {
        delete m_pClipText;
        m_pClipText = nullptr;
    }
    return true;
}

/* 处理数据 */
bool ClipText_NS::CClipTextV2_0::process(
    InData_S stInData,
    std::vector<float> &vResult)
{
    vResult.clear();
    if (stInData.sText.empty())
    {
        printf("传入文字为空\n");
        return false;
    }

    if (!m_pClipText)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

    // if (m_stInParam.bDebug)
    {
        /* 输出输入的文字 */
        printf("输入的文字为:[%s]\n",stInData.sText.c_str());
    }

    /* 推理+后处理 */
    Inference_NS::InputData_S stInputData;
    stInputData.strText = stInData.sText;

    std::vector<Inference_NS::ClsData_S> vClsDatas;
    bRet = m_pClipText->inference(stInputData, vClsDatas);

    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }
    
    printf("文本特征向量为:");
    for(int i=0;i<10;i++)
    {
        printf("%f ",vClsDatas[0].vFeature[i]);
    }
    printf("\n");
    
    vResult = vClsDatas[0].vFeature;
    return true;
}
