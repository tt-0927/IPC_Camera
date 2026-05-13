/**
 * @FilePath     : stream_handler.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-13 17:35:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-28 17:38:55
 * @Description  : 处理AiAppStream的数据
 */

#include "stream_handler.hpp"
#include "stream_video.h"
#include "stream_audio.h"
#include "mpp_vgs.h"

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
 * @brief   : 接受媒体数据 (零拷贝)
 * @note    : 接收一个指向原始MPP帧的指针，并用带有自定义删除器的
 *            std::shared_ptr来包装它，从而实现生命周期自动管理和零拷贝
 * @param   {const ot_video_frame_info*} pFrameInfo: 从MPP获取的原始视频信息帧
 */
void CStreamHandler::recvDataProcess(const ot_video_frame_info *pFrameInfo)
{
    if (pFrameInfo == nullptr || pFrameInfo->video_frame.width == 0 || pFrameInfo->video_frame.height == 0)
    {
        dlog_error("无效的视频帧");
        return;
    }

    MediaData_S stRecvData;
    int nLen = pFrameInfo->video_frame.width * pFrameInfo->video_frame.height * 3 / 2;
    stRecvData.nSize = nLen; // YUV420 size
    stRecvData.enType = MediaDataType_E::VIDEO_DATA;
    stRecvData.stMediaParam.enPixelFormat = pFrameInfo->video_frame.pixel_format;
    stRecvData.stMediaParam.nVideoWidth = pFrameInfo->video_frame.width;
    stRecvData.stMediaParam.nVideoHeight = pFrameInfo->video_frame.height;

    /**
     * 使用 std::shared_ptr 和自定义删除器包装 pFrameInfo
     * 1. new ot_video_frame_info(*pFrameInfo): 创建一个 pFrameInfo 的拷贝，让 shared_ptr 管理一个新的对象。
     *    注意：这里的拷贝是浅拷贝结构体本身，而不是图像数据，开销极小。
     * 2. 自定义删除器 lambda 函数: [=](ot_video_frame_info* frameToDelete) {...}
     *    当最后一个指向该数据的 shared_ptr 被销毁时，这个 lambda 会被调用。
     *    它会正确地释放 MPP 视频缓冲区块（VB BLK），然后删除我们 new 出来的 frameToDelete 结构体。
     */
    auto deleter = [length = nLen](ot_video_frame_info *frameToDelete)
    {
        if (!frameToDelete)
            return;
        /* 调用海思SDK的VPSS接口释放VB Block */
        if (frameToDelete->pool_id != OT_VB_INVALID_POOL_ID)
        {
            auto pVpssHandle = CStreamVideo::instance()->get_vpssHandle();
            if (pVpssHandle)
            {
                td_void *virtAddr = (td_void *) frameToDelete->video_frame.virt_addr[0];
                if (virtAddr)
                {
                    /* memory存储解映射 */
                    if (ss_mpi_sys_munmap(virtAddr, length) != 0)
                        dlog_warn("ss_mpi_sys_munmap err, addr=%p", virtAddr);
                }
                /* 释放码流缓存 */
                pVpssHandle->mppVpss_release_chnFrame(pVpssHandle, VPSS_CHANNEL_AI, frameToDelete);
            }
        }
        delete frameToDelete;
    };
    stRecvData.pVideoFrameInfo = std::shared_ptr<ot_video_frame_info>(new ot_video_frame_info(*pFrameInfo), deleter);

    /* 发送至已绑定的槽 */
    send_videoData(stRecvData);
}

/**
 * @brief   : 接受媒体数据 (零拷贝)
 * @note    : 接收一个指向原始MPP帧的指针，并用带有自定义删除器的
 *            std::shared_ptr来包装它，从而实现生命周期自动管理和零拷贝
 * @param   {const ot_audio_frame*} pFrame: 从MPP获取的原始视频信息帧
 */
void CStreamHandler::recvDataProcess(const ot_audio_frame *pFrame)
{
    if (pFrame == nullptr || pFrame->len <= 0)
    {
        dlog_error("无效的音频帧");
        return;
    }

    MediaData_S stRecvData;
    stRecvData.nSize = pFrame->len;
    stRecvData.enType = MediaDataType_E::AUDIO_DATA;
    stRecvData.stMediaParam.enBitWidth = pFrame->bit_width;

    auto deleter = [](ot_audio_frame *frameToDelete)
    {
        if (!frameToDelete)
            return;
        auto pAiHandle = CStreamAudio::instance()->get_aiHandle(AI_MIC_CHN);
        if (pAiHandle)
        {
            /* 释放码流缓存 */
            pAiHandle->mppAi_releaseFrame(pAiHandle, pAiHandle->stNeedParam.nChn, frameToDelete, NULL);
        }
        delete frameToDelete;
    };
    stRecvData.pAudioFrame = std::shared_ptr<ot_audio_frame>(new ot_audio_frame(*pFrame), deleter);

    /* 发送至已绑定的槽 */
    send_audioData(stRecvData);
}

void CStreamHandler::recvDataProcess(const void *pData, int nLength, int nWidth, int nHeight)
{
    if (pData == nullptr || nLength <= 0)
    {
        dlog_error("无效的视频帧");
        return;
    }

    /* 创建自定义删除器 */
    auto deleter = [nLength](ot_video_frame_info *frameToDelete)
    {
        if (!frameToDelete)
            return;

        /* 调用海思SDK的VPSS接口释放VB Block */
        if (frameToDelete->pool_id != OT_VB_INVALID_POOL_ID)
        {
            /* 如果pool_id无效,说明是我们自己创建的,需要手动释放 */
            td_void *virtAddr = (td_void *) frameToDelete->video_frame.virt_addr[0];
            if (virtAddr)
            {
                /* memory存储解映射 */
                if (ss_mpi_sys_munmap(virtAddr, nLength) != 0)
                    dlog_warn("memory存储解映射失败, 地址:[%p]", virtAddr);
            }

            /* 销毁video_frame_info结构 */
            mppVgs_destroy_video_frame_info(frameToDelete);
        }

        /* 释放frameToDelete本身 */
        delete frameToDelete;
    };

    /* 创建video_frame_info结构 */
    ot_video_frame_info *pFrameInfo = new (std::nothrow) ot_video_frame_info;
    if (!pFrameInfo)
    {
        dlog_error("分配ot_video_frame_info内存失败");
        return;
    }

    /* 使用mppVgs_create_video_frame_info创建帧结构并分配内存 */
    if (TD_SUCCESS != mppVgs_create_video_frame_info(nWidth, nHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, pFrameInfo))
    {
        dlog_error("创建video_frame_info失败");
        delete pFrameInfo;
        return;
    }

    /* 映射物理地址到虚拟地址 */
    pFrameInfo->video_frame.virt_addr[0] = ss_mpi_sys_mmap(pFrameInfo->video_frame.phys_addr[0], nLength);
    if (!pFrameInfo->video_frame.virt_addr[0])
    {
        dlog_error("ss_mpi_sys_mmap失败");
        mppVgs_destroy_video_frame_info(pFrameInfo);
        delete pFrameInfo;
        return;
    }

    /* 拷贝数据到映射的虚拟地址 */
    if (memcpy_s(pFrameInfo->video_frame.virt_addr[0], nLength, pData, nLength) != 0)
    {
        dlog_error("memcpy_s失败");
        ss_mpi_sys_munmap(pFrameInfo->video_frame.virt_addr[0], nLength);
        mppVgs_destroy_video_frame_info(pFrameInfo);
        delete pFrameInfo;
        return;
    }

    /* 构建MediaData_S结构 */
    MediaData_S stRecvData;
    stRecvData.nSize = nLength; /* YUV420 size */
    stRecvData.enType = MediaDataType_E::VIDEO_DATA;
    stRecvData.stMediaParam.enPixelFormat = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stRecvData.stMediaParam.nVideoWidth = nWidth;
    stRecvData.stMediaParam.nVideoHeight = nHeight;

    /* 创建智能指针,使用自定义删除器 */
    stRecvData.pVideoFrameInfo = std::shared_ptr<ot_video_frame_info>(pFrameInfo, deleter);

    /* 发送至已绑定的槽 */
    send_videoData(stRecvData);
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
    stRecvData.stMediaParam.enBitWidth = OT_AUDIO_BIT_WIDTH_16;

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
