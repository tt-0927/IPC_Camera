/**
 * @FilePath     : algo_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-28 17:49:06
 * @Description  : AI_APP 初始化和通讯转发
 */

#include "algo_detect.h"
#include "algo_stream_deal.h"
#include "algo_control_deal.h"
#include "event_manage.h"
#include "IpcRet.h"

int algo_detect_init()
{
    int nRet = OK;
    
    nRet = CAlgoStreamDeal::instance()->init();
    
    if (nRet < OK)
    {
        dlog_error("AI_APP: 算法检测初始化失败");
        return ERR;
    }

    /*设置 AI 算法控制处理的回调*/
    CEventManage::instance()->set_algoControlDeal_callback(
        [](int nCode, const char *strData, void *pData) -> void
        {
            return algo_send_controlData(nCode, strData, pData);
        });

    dlog_info("AI_APP: 算法检测初始化成功");
    return OK;
}

int algo_detect_deinit()
{
    CAlgoStreamDeal::instance()->deinit();
    dlog_info("AI_APP: 算法检测去初始化成功");
    return OK;
}

void algo_send_streamData(const ot_video_frame_info *pFrameInfo)
{
    CAlgoStreamDeal::instance()->deal_message(pFrameInfo);
}

void algo_send_videoStreamData(const ot_video_frame_info *pFrameInfo)
{
    CAlgoStreamDeal::instance()->deal_message(pFrameInfo);
}

void algo_send_videoStreamData(const void *pData, int nLength, int nWidth, int nHeight)
{
    CAlgoStreamDeal::instance()->deal_videoStreamData(pData, nLength, nWidth, nHeight);
}

void algo_send_audioStreamData(const ot_audio_frame *pFrame)
{
    CAlgoStreamDeal::instance()->deal_message(pFrame);
}

void algo_send_audioStreamData(const void *pData, int nLength)
{
    CAlgoStreamDeal::instance()->deal_audioStreamData(pData, nLength);
}

void algo_send_controlData(int nCode, const char *strData, void *pData)
{
    if(strData == nullptr)
    {
        std::string stringData("{}");
        AlgoControlDeal::instance()->deal_message(nCode, stringData, pData);    
    }else{
        std::string stringData(strData);
        AlgoControlDeal::instance()->deal_message(nCode, stringData, pData);
    }
}
