/*
 * @FilePath     : AudioAnomalyDetectV1_0.cpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-23 20:19:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 17:38:56
 * @Description  : 人少场景
 */
#include "AudioAnomalyDetectExt.hpp"
#include "AudioAnomalyDetectV1_0.hpp"
#include "SavePcm.hpp"
#include <cmath>
#include <vector>

using namespace AudioAnomalyDetect_NS;

AudioAnomalyDetect_NS::CAudioAnomalyDetectV1_0::CAudioAnomalyDetectV1_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

AudioAnomalyDetect_NS::CAudioAnomalyDetectV1_0::~CAudioAnomalyDetectV1_0()
{
    unInit();
}

/* 初始化 */
bool AudioAnomalyDetect_NS::CAudioAnomalyDetectV1_0::init()
{
    bool bRet = false;

    m_pAudioAnomaly = new Inference_NS::cAudioAnomaly;
    if (m_pAudioAnomaly)
    {
        bRet = true;
    }

    if (!bRet)
    {
        printf("[%s] 算法初始化失败 \n",
               "CAudioAnomalyDetectV1_0");
        goto FAIL;
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool AudioAnomalyDetect_NS::CAudioAnomalyDetectV1_0::unInit()
{
    if (m_pAudioAnomaly)
    {
        delete m_pAudioAnomaly;
        m_pAudioAnomaly = nullptr;
    }
    return true;
}

/* 处理数据 */
bool AudioAnomalyDetect_NS::CAudioAnomalyDetectV1_0::process(
    InData_S stInData,
    Result_S &vecResult)
{

    if (!stInData.pData || stInData.nLength <= 0)
    {
        printf("传入PCM数据为空\n");
        return false;
    }

    if (!m_pAudioAnomaly)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

    if (m_stInParam.bDebug)
    {
        /* 保存PCM数据 */
        Modules_NS::savePcm(stInData.pData, stInData.nLength, m_stInParam.strAnalyzeDataPath);
    }
    /* 前处理 */
    /* 2个字节一个声道，数据存储（左声道、右声道、左声道、右声道，...） */
    short *pPcmData = (short *)stInData.pData;
    stInData.nLength /= 2;

    /* 遍历声音，获取左声道进行推理 */
    std::vector<double> vInputData;
    for (int nIndex = 0; nIndex < stInData.nLength; nIndex += 2)
    {
        vInputData.push_back((double)pPcmData[nIndex]);
    }

#if 0
    /* 静音检测算法 */
    if (stInData.stParam.stSilentParam.bEnable)
    {
        m_pAudioAnomaly->silentDetecte((double *)vInputData.data(), vInputData.size(), stInData.stParam.stSilentParam.fAbilityThres, vecResult.bSilentFlag);
    }
    /* 声音大忽小检测算法 */
    if (stInData.stParam.stFluctuateParam.bEnable)
    {
        m_pAudioAnomaly->fluctuateDetect((double *)vInputData.data(), vInputData.size(), stInData.stParam.stFluctuateParam.fAbilityThres, vecResult.bFluctuateFlag);
    }
#else
    /* 音频分贝调用 */
    double dRes = m_pAudioAnomaly->calculateDB((double *)vInputData.data(), vInputData.size());
    if(dLastDB == -1e100)
    {
        dLastDB = dRes;
    }
    /* 静音检测算法 */
    if (stInData.stParam.stSilentParam.bEnable)
    {
        if (dRes < stInData.stParam.stSilentParam.fAbilityThres)
        {
            vecResult.bSilentFlag = true;
        }
    }
    /* 声音大忽小检测算法 */
    if (stInData.stParam.stFluctuateParam.bEnable)
    {
        /* 如果声音能量过低，默认20分贝(静音) */
        if(dRes < 20)
        {
            dRes = 20.0; 
        }
        /* 判断忽大忽小逻辑 */
        if (dRes - dLastDB > stInData.stParam.stFluctuateParam.fAbilityThres)
        {
            vecResult.bFluctuateHighFlag = true;
        }
        else if (dLastDB - dRes > stInData.stParam.stFluctuateParam.fAbilityThres)
        {
            vecResult.bFluctuateLowFlag = true;
        }
    }

    // printf("========== LastDB: [%f]  DB: [%f] ==========\n", dLastDB, dRes);

    dLastDB = dRes;

#endif

    if (m_stInParam.bDebug)
    {
        /* 保存PCM数据 */
    }
    return true;
}
