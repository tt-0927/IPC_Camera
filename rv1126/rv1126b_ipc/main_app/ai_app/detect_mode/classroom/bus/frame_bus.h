/**
 * @FilePath     : frame_bus.h
 * @Author       : leiyy
 * @Date         : 2026-10-23 16:45:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 16:45:00
 * @Description  : 帧总线—— 全链路唯一的像素入口，内存与稳定的第一道闸
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "classroom_define.h"
#include "frame_geometry.h"

namespace Classroom
{
/**
 * @brief setupGeometry 的三种结局
 */
typedef enum class GeomSwap : int
{
    SAME = 0,       /* < 六项与现值全同：无动作 */
    SWAPPED = 1,    /* < 镜像/旋转/画布已换入，下一帧起生效（槽缓冲未动） */
    REJECTED = 2,   /* < 未就绪 / 参数非法 / 源尺寸变化：现几何原样保留 */
} GeomSwap_E;

/**
 * @brief 一帧分析图像的只读句柄
 */
struct CFrame_S
{
    std::shared_ptr<std::vector<uint8_t>> spPixel; /* NV12 缓冲，引用计数持有；归零即回槽 */
    const uint8_t *pucNv12 = nullptr;              /* = spPixel->data()，热路径免解引用 */
    size_t  nPixelBytes = 0;                       /* 实际有效字节数（= W×H×3/2） */

    int      nSrcW = 0;                            /* 像素所属的源帧宽 */
    int      nSrcH = 0;
    uint64_t u64PtsUs = 0;                         /* 推流侧 PTS（微秒），仅用于对齐视频时间轴 */
    int64_t  llMonotonicMs = 0;                    /* 入总线时刻（单调时钟），时长门与陈旧判定用它 */
    uint32_t u32Seq = 0;                           /* 帧序号，消费者据此判断特征是否新鲜 */

    CFrameGeometry stGeom;                        /* 本帧可用的源帧→分析帧变换（值拷贝，无生命周期耦合） */
};

typedef std::shared_ptr<const CFrame_S> FramePtr;

/**
 * @brief 总线运行计数（供 metrics 与「内存闸门后是否需要降帧」决策）
 */
typedef struct _FrameBusStat_
{
    uint64_t u64Published = 0;      /* 成功发布帧数 */
    uint64_t u64DroppedNoSlot = 0;  /* 因所有槽被占用而丢帧（慢消费者堆积的直接指标） */
    uint64_t u64RejectedArg = 0;    /* 参数非法 / 长度不足一帧（上游异常，需要查） */
    uint32_t u32BusySlotsMax = 0;   /* 历史最大同时在外部持有的槽数，逼近槽数说明该扩容或提速 */
    uint32_t u32Slots = 0;
    uint32_t u32PixelBytesPerSlot = 0;
} CFrameBusStat_S;

/**
 * @brief   : 固定槽位帧总线
 */
class CFrameBus
{
public:
    CFrameBus() = default;
    ~CFrameBus();

    CFrameBus(const CFrameBus &) = delete;
    CFrameBus &operator=(const CFrameBus &) = delete;

    /**
     * @brief   : 初始化并一次性分配全部像素缓冲
     * @param    {int} nSrcW / nSrcH：AI 分析帧宽高（现网 VPSS AI 通道固定 1920×1080）
     * @param    {int} nDstW / nDstH：分析画布宽高，与源帧一致时不发生缩放
     * @param    {MirrorMode_E} enMirror / {int} nRotate：画面镜像与旋转
     * @return   {bool} 失败（参数非法 / 几何自校验不过 / 内存不足）返回 false，总线保持未就绪
     */
    bool init(int nSrcW, int nSrcH, int nDstW, int nDstH, MirrorMode_E enMirror, int nRotate);

    /* 释放全部缓冲。必须在所有消费者线程停止之后调用（放掉引用后仍安全，但会留下短暂驻留） */
    void deinit();

    bool isReady() const;

    /**
     * @brief   : 生产者投递一帧（非阻塞）
     * @param    {const void*} pNv12：源帧像素
     * @param    {int64_t} nLen：可用字节数，小于 W×H×1.5 直接拒绝（半帧进模型比丢帧更糟）
     * @param    {uint64_t} u64PtsUs：推流侧 PTS
     * @param    {uint32_t} u32Seq：帧序号，由生产者维护单调递增
     * @return   {bool} true 已发布；false 丢帧（无空闲槽 / 参数非法 / 未就绪）
     */
    bool publish(const void *pNv12, int64_t nLen, uint64_t u64PtsUs, uint32_t u32Seq);

    /**
     * @brief   : 消费者取当前最新帧
     * @return   {FramePtr} 未发布过任何帧时返回 nullptr（上层须按「证据不足」处理，不得当 0 人）
     */
    FramePtr latest() const;

    /* 几何变换副本，供尚未拿到帧的模块（如配置校验）使用 */
    CFrameGeometry geometry() const;

    /**
     * @brief   : 几何参数热换入：镜像/旋转/画布尺寸三项当场生效，槽缓冲不动
     * @param    {int} nSrcW / nSrcH：必须与现值一致——槽缓冲按源尺寸一次分配，
     *           且上游 VPSS AI 通道就是按这个分辨率建的，本总线热换了也没处拿到新尺寸的像素
     * @param    {int} nDstW / nDstH：画布尺寸只是逐帧下发的坐标口径，随便换
     * @return   {GeomSwap_E} SAME / SWAPPED / REJECTED（被拒时现几何原样保留，总线不掉线）
     */
    GeomSwap_E setupGeometry(int nSrcW, int nSrcH, int nDstW, int nDstH,
                             MirrorMode_E enMirror, int nRotate);

    CFrameBusStat_S stat() const;
    void resetStat();

private:
    /* 从最新槽之后开始环形查找空闲槽；无空闲返回 -1。调用方必须持锁 */
    int findFreeSlotLocked() const;
    /* 统计当前被外部持有的槽数并更新峰值。调用方必须持锁 */
    void updateBusyStatLocked();

private:
    mutable std::mutex m_mtx;

    bool m_bReady = false;
    int  m_nSrcW = 0;
    int  m_nSrcH = 0;
    int  m_nDstW = 0;
    int  m_nDstH = 0;
    MirrorMode_E m_enMirror = MirrorMode_E::MIRROR_NONE;
    int  m_nRotate = 0;
    size_t m_nFrameBytes = 0;

    /* 永久持有的像素缓冲：槽位复用同一块内存，是「运行期零像素分配」的实现方式 */
    std::vector<std::shared_ptr<std::vector<uint8_t>>> m_apPixel;
    /* 每槽当前帧句柄；use_count()==1 表示没有消费者引用，该槽可复用 */
    std::vector<FramePtr> m_apSlot;
    int m_nLatest = -1;

    CFrameGeometry m_stGeom;
    CFrameBusStat_S m_stStat;
};
}
