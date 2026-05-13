/**
 * @FilePath     : stream_ai_detect.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-04-29 13:42:14
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-05-12 19:43:46
 * @Description  : AI 检测
 */

#include "stream_ai_detect.h"
#include "dlog.h"

HiAiDetect_S *streamAiDetect_init(int nChn, const char *pModelPath)
{
    HiAiDetect_S *pHandle = (HiAiDetect_S *)malloc(sizeof(HiAiDetect_S));
    if (pHandle == NULL)
    {
        dlog_error("申请内存失败");
        return nullptr;
    }
    memset(pHandle, 0, sizeof(HiAiDetect_S));

    HiAiDetectNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(HiAiDetectNeedParam_S));
    stNeedParam.nChn = nChn;
    stNeedParam.enModelLoadMode = OT_AIDETECT_MODEL_LOAD_FROM_PATH;
    memcpy(stNeedParam.aModelPath, pModelPath, strlen(pModelPath));

    pHandle = svpAiDetect_alloc(stNeedParam);
    int nRet = OK;
    /*初始化vi*/
    nRet = pHandle->svpAiDetect_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("streamAiDetect mppVi_init error");
        return nullptr;
    }

    dlog_info("streamAiDetect 初始化成功");
    return pHandle;
}

int streamAiDetect_uninit(HiAiDetect_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }
    int nRet = OK;
    /*反初始化vi*/
    nRet = pHandle->svpAiDetect_uninit(pHandle);
    if (nRet < OK)
    {
        dlog_error("streamAiDetect mppVi_uninit error");
        return nRet;
    }
    svpAiDetect_release(pHandle);

    return nRet;
}
