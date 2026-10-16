/**
 * @FilePath     : jpeg_encode_service.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-20
 * @Description  : VENC 硬件 JPEG 编码服务（全景图 + 目标小图批量抠图）。
 *
 * 基于 hi_pipeline 的 mppVencJpeg（ss_mpi_venc_send_frame_batch_crop）封装：
 *   - encodePanorama：整帧 1080p 编码为单张 JPEG（全景图）；
 *   - encodeCrops   ：整帧 + 抠图框数组一次硬件编码出多张目标小图，
 *                     输出顺序与 rects 下标一致（省去 VGS 物理裁剪）。
 *
 * 约束与设计：
 *   - 帧必须与通道同尺寸（1920x1080，VPSS AI 通道原始帧）且为非压缩 NV21，
 *     不满足时接口直接返回失败（调用方多为算法线程，失败即跳过本帧出图）；
 *   - 单通道硬件串行编码，内部互斥锁串行化，满负荷时排队等待（超时失败）；
 *   - 通道懒初始化（首次编码时创建），分辨率固定取 VPSS AI 通道，不随帧变化；
 *   - 输出缓冲复用（w*h*2 上限），编码完成后拷贝交付，无每帧分配。
 */

#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "ot_common_video.h"

namespace AiAppCommon
{

/* 用户态 JPEG 编码通道默认配置（与 VPSS AI 通道对齐） */
constexpr uint32_t JPEG_ENCODE_DEFAULT_WIDTH = 1920;
constexpr uint32_t JPEG_ENCODE_DEFAULT_HEIGHT = 1080;
/* 品质因数默认值 [1,99]，对齐旧 FFmpeg 路径 quality 95 的高质量档位 */
constexpr uint32_t JPEG_ENCODE_DEFAULT_QFACTOR = 90;
/* 单个抠图框最小宽高（CV610 batch_crop 硬件约束 32x32），不足时接口返回失败 */
constexpr uint32_t JPEG_ENCODE_CROP_MIN_SIZE = 32;
/* 编码超时：硬编码单帧 1080p 实测约 10ms，500ms 上限覆盖排队与异常恢复 */
constexpr int JPEG_ENCODE_TIMEOUT_MS = 500;

class CJpegEncodeService
{
public:
    static CJpegEncodeService *instance();

    /**
     * @brief   整帧编码为单张 JPEG（全景图）。
     * @param   stFrameInfo  视频帧（宽高须等于 1920x1080，非压缩 NV21）
     * @param   vecJpeg      输出 JPEG 数据（成功时有效）
     * @param   unQfactor    品质因数 [1,99]，0 表示沿用当前通道设置
     * @return  {int} 0：成功，非0：失败
     */
    int encodePanorama(const ot_video_frame_info &stFrameInfo, std::vector<uint8_t> &vecJpeg, uint32_t unQfactor = 0);

    /**
     * @brief   批量抠图编码（目标小图）。
     *          一次送入全部抠图框，硬件一次完成编码，省去 VGS 物理裁剪。
     * @param   stFrameInfo  视频帧（宽高须等于 1920x1080，非压缩 NV21）
     * @param   vecRects     抠图框数组（x 8 对齐 / y 2 对齐 / 宽高 >= 32x32 / 不越界，
     *                       不满足时接口返回失败，由调用方保证对齐）
     * @param   vecJpegs     输出 JPEG 数组，与 vecRects 下标一一对应
     * @param   unQfactor    品质因数 [1,99]，0 表示沿用当前通道设置
     * @return  {int} 0：成功，非0：失败（部分失败整体返回失败）
     */
    int encodeCrops(const ot_video_frame_info &stFrameInfo,
                    const std::vector<ot_rect> &vecRects,
                    std::vector<std::vector<uint8_t>> &vecJpegs,
                    uint32_t unQfactor = 0);

    /**
     * @brief   释放编码通道与输出缓冲（人脸算法全部关闭时调用）。
     *          通道码流池约 4MB 常驻 MMZ，出图才用到；全关不释放会让
     *          media-mem 占用比算法开启前多约 4MB。下次编码自动重建
     *          （mppVencJpeg_create 毫秒级，出图频率低无感知）。
     * @return  {int} 0：成功或本就未创建，非0：销毁失败（通道保留待进程退出回收）
     */
    int releaseChannel();

private:
    CJpegEncodeService() = default;
    ~CJpegEncodeService() = default;
    CJpegEncodeService(const CJpegEncodeService &) = delete;
    CJpegEncodeService &operator=(const CJpegEncodeService &) = delete;

    /* 懒初始化编码通道，成功后置 m_bInited；仅持锁调用 */
    int ensureChannelLocked();

    std::mutex m_mutex;
    bool m_bInited = false;
    /* 复用输出缓冲（按 w*h*2 预留，全景与小图共用） */
    std::vector<std::vector<uint8_t>> m_vecCropBufs;
};

} // namespace AiAppCommon
