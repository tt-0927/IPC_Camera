/**
 * @file StreamHandler.cpp
 * @author xiejh (xiejh@kfb.cn)
 * @date 2024-10-29
 *
 * @brief 处理AiAppStreamClient的数据
 */
#include "stream_handler.hpp"

/* 开始获取数据 */
bool CStreamHandler::start()
{

    return true;
}

/* 停止获取数据 */
bool CStreamHandler::stop()
{

    return true;
}

/**
 * @brief 接受媒体数据
 * @param pData 数据结构体
 */
void CStreamHandler::recvDataProcess(const void *pData, int nLen, int nH, int nW)
{
    if (pData == nullptr || nH == 0 || nW == 0)
    {
        dlog_error("无效数据或大小");
        return;
    }
    MediaData_S stRecvData;
    stRecvData.nSize = nLen;

    if (stRecvData.nSize > 0)
    {
        stRecvData.pData = std::shared_ptr<char[]>(new char[stRecvData.nSize]);
        if (NULL == stRecvData.pData || NULL == stRecvData.pData.get())
        {
            dlog_error("创建智能指针失败");
            return;
        }

        memcpy(stRecvData.pData.get(), pData, stRecvData.nSize);
        stRecvData.enType = MediaDataType_E::VIDEO_DATA;
        stRecvData.stMediaParam.enPixelFormat = RK_FMT_YUV420SP;
        stRecvData.stMediaParam.nVideoWidth = nW;
        stRecvData.stMediaParam.nVideoHeight = nH;

        send_videoData(stRecvData);
    }
}

void CStreamHandler::recvDataProcess(const Video_NS::VideoFrame_S *pFrame, int nH, int nW)
{
    if (pFrame == nullptr || nH == 0 || nW == 0)
    {
        dlog_error("无效数据或大小");
        return;
    }

    MediaData_S stRecvData;
    stRecvData.nSize = pFrame->nLen;

    if (stRecvData.nSize > 0)
    {
        stRecvData.pData = std::shared_ptr<char[]>(new char[stRecvData.nSize]);
        if (NULL == stRecvData.pData || NULL == stRecvData.pData.get())
        {
            dlog_error("创建智能指针失败");
            return;
        }

        memcpy(stRecvData.pData.get(), pFrame->pData, stRecvData.nSize);

        stRecvData.enType = MediaDataType_E::VIDEO_DATA;
        stRecvData.stMediaParam.enPixelFormat = RK_FMT_YUV420SP;
        stRecvData.stMediaParam.nVideoWidth = nW;
        stRecvData.stMediaParam.nVideoHeight = nH;

        send_videoData(stRecvData);
    }
}

void CStreamHandler::recvDataProcess(const void *pData, int nLength)
{
    if (pData == nullptr || nLength <= 0)
    {
        dlog_error("无效的音频帧");
        return;
    }

    MediaData_S stRecvData;
    stRecvData.nSize = nLength;
    stRecvData.enType = MediaDataType_E::AUDIO_DATA;
    stRecvData.stMediaParam.enBitWidth = AUDIO_BIT_WIDTH_16; 

    stRecvData.pData = std::shared_ptr<char[]>(new char[stRecvData.nSize]);
    if (NULL == stRecvData.pData || NULL == stRecvData.pData.get())
    {
        dlog_error("创建智能指针失败");
        return;
    }

    memcpy(stRecvData.pData.get(), pData, stRecvData.nSize);

    /* 发送至已绑定的槽 */
    send_audioData(stRecvData);
}
