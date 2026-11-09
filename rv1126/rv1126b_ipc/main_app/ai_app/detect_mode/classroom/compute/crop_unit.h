/**
 * @FilePath     : crop_unit.h
 * @Author       : leiyy
 * @Date         : 2026-10-08 10:35:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-08 10:35:00
 * @Description  : 模型输入生成单元
 */

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>

#include "classroom_define.h"
#include "frame_bus.h"
#include "rga_pool.h"

namespace Classroom
{
/**
 * @brief 裁剪档统计
 */
typedef struct _CropStat_
{
    uint64_t u64Ok = 0;           /* 成功生成的输入数 */
    uint64_t u64RejectTooSmall = 0; /* 对齐后宽高不足 2 像素（远处小目标 / 跟踪框抖没） */
    uint64_t u64RejectNoBuf = 0;  /* 池借空：该降采样，而不是加大队列 */
    uint64_t u64RgaFail = 0;      /* RGA 返回失败：必须查格式/对齐，不会自愈 */
    uint64_t u64RejectArg = 0;    /* 帧或参数非法（未就绪、指针为空、尺寸不符） */
    uint64_t u64RejectBadRect = 0; /* 框本身坏了（退化/反向/NaN）：查下发数据，不是查相机 */
} CCropStat_S;

/**
 * @brief   : 单一模型尺寸的输入生成档
 * @note    : 线程安全取决于 CRgaPool（其内部加锁）；本类自身成员在 setup() 后只读，
 *            因此一个档可以被多条线程共用，但一次裁剪的缓冲在推理完成前不得复用
 */
class CCropUnit
{
public:
    CCropUnit() = default;
    ~CCropUnit();

    CCropUnit(const CCropUnit &) = delete;
    CCropUnit &operator=(const CCropUnit &) = delete;

    /**
     * @brief   : 建立一个模型输入档
     * @param    {uint32_t} u32ModelW / u32ModelH：模型输入宽高（RGA 目标尺寸）
     * @param    {uint32_t} nBufNum：缓冲块数，至少 2
     * @param    {ChannelOrder_E} enOrder：送模型的通道顺序（配置项，见 ChannelOrder_E 注释）
     * @param    {bool} bLetterbox：等比缩放开关
     * @return   {bool} 尺寸不满足对齐要求或分配失败返回 false，档保持未就绪
     */
    bool setup(uint32_t u32ModelW, uint32_t u32ModelH, uint32_t nBufNum,
               ChannelOrder_E enOrder = ChannelOrder_E::CH_RGB, bool bLetterbox = false);
    void deinit();

    bool isReady() const { return m_bReady; }
    bool letterbox() const { return m_bLetterbox; }

    uint32_t inputWidth() const  { return m_u32W; }
    uint32_t inputHeight() const { return m_u32H; }
    uint32_t inputBytes() const  { return m_nBufBytes; }

    /**
     * @brief   : 整帧档：源帧 → 模型输入
     * @param    {CFrame_S} stFrame：帧总线取到的帧
     * @param    {RgaBufPtr&} rspBuf：OUT=借出的输入缓冲，调用方持有到推理结束
     * @return   {bool} false 时 rspBuf 为空，调用方按「本帧放弃」处理并计一次失败
     */
    bool cropWhole(const CFrame_S &stFrame, RgaBufPtr &rspBuf);

    /**
     * @brief   : 区域档：源帧坐标系里的一个框 → 模型输入
     * @param    {CFrame_S} stFrame：帧
     * @param    {CRectf_S} stRectSrc：源帧坐标系下的框（左上/右下，允许浮点、允许越界）
     * @param    {RgaBufPtr&} rspBuf：OUT=借出的输入缓冲
     * @param    {CRectf_S*} pstUsedSrc：OUT=**钳制取偶后真正喂进模型的那块源帧矩形**，可为空。
     *            要把模型输出换算回分析帧的调用方（姿态关键点）必须拿它建映射：
     *            用原始框会让整体偏移最多半个框，且偏移量随框大小变化，表现为「偶尔判错」
     * @param    {int} nRotation：0/90/180/270 顺时针
     * @return   {bool} 框经钳制+取偶后不足 2×2、或借不到缓冲、或 RGA 失败时 false
     */
    bool cropRoi(const CFrame_S &stFrame, const CRectf_S &stRectSrc, RgaBufPtr &rspBuf,
                 CRectf_S *pstUsedSrc = nullptr, int nRotation = 0);

    CRgaPoolStat_S poolStat() const { return m_stPool.stat(); }
    int poolIdle() const { return m_stPool.nIdle(); }   /* -1=未建档/未就绪，0=块全被在途请求占着 */
    CCropStat_S stat() const;

private:
    /* 源帧 → 目标缓冲的一次搬运；调用方负责已借到缓冲。返回 false 时已计入对应计数 */
    bool transform(const CFrame_S &stFrame, int nCx, int nCy, int nCw, int nCh, RgaBufPtr rspBuf,
                   int nRotation, int nDstH = 0);

private:
    /* 池与计数器各自加锁：几何参数在 setup() 之后只读，可被任意线程并发读 */
    mutable std::mutex m_mtxStat;

    CRgaPool m_stPool;

    bool     m_bReady = false;
    bool     m_bLetterbox = false;
    uint32_t m_u32W = 0;
    uint32_t m_u32H = 0;
    uint32_t m_nBufBytes = 0;
    int      m_nDstFormat = 0;   /* RK_FORMAT_RGB_888 / RK_FORMAT_BGR_888，setup() 定死后只读 */

    CCropStat_S m_stStat;
};

typedef std::shared_ptr<CCropUnit> CropUnitPtr;
}
