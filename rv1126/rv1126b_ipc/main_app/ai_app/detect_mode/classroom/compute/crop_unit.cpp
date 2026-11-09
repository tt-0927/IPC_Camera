/**
 * @FilePath     : crop_unit.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-08 10:35:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-08 10:35:00
 * @Description  : 模型输入生成实现（RGA 搬运 + 对齐/越界的显式处理）
 */

#include "crop_unit.h"

#include <cstring>

#include "common_process.h"
#include "crop_geom.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* NV12 整帧字节数：Y 平面 W×H + 交织 UV 平面 W×H/2 */
inline size_t nv12FrameBytes(int nW, int nH)
{
    return static_cast<size_t>(nW) * static_cast<size_t>(nH) * 3 / 2;
}

}

CCropUnit::~CCropUnit()
{
    deinit();
}

bool CCropUnit::setup(uint32_t u32ModelW, uint32_t u32ModelH, uint32_t nBufNum, ChannelOrder_E enOrder,
                      bool bLetterbox)
{
    deinit();

    if (0 == u32ModelW || 0 == u32ModelH)
    {
        dlog_warn("classroom crop: 模型尺寸 %ux%u 非法", u32ModelW, u32ModelH);
        return false;
    }
    /* RGB888 目标的 dw 必须 4 对齐 */
    if (0 != (u32ModelW % 4) || 0 != (u32ModelH % 2))
    {
        dlog_error("classroom crop: 模型尺寸 %ux%u 违反 RGA 对齐约束 (W%%4, H%%2)",
                   u32ModelW, u32ModelH);
        return false;
    }
    if (nBufNum < 2)
    {
        dlog_error("classroom crop: %s 需要 >=2 个缓冲，实得 %u", "input pool", nBufNum);
        return false;
    }

    const uint64_t ullBytes = static_cast<uint64_t>(u32ModelW) * u32ModelH * 3;
    if (ullBytes > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()))
    {
        dlog_error("classroom crop: 模型尺寸 %ux%u 过大", u32ModelW, u32ModelH);
        return false;
    }

    m_nDstFormat = (ChannelOrder_E::CH_BGR == enOrder) ? RK_FORMAT_BGR_888 : RK_FORMAT_RGB_888;
    m_u32W = u32ModelW;
    m_u32H = u32ModelH;
    m_nBufBytes = static_cast<uint32_t>(ullBytes);
    m_bLetterbox = bLetterbox;

    if (!m_stPool.init(nBufNum, m_nBufBytes))
    {
        dlog_error("classroom crop: 内存池分配失败，%u B x %u", m_nBufBytes, nBufNum);
        m_u32W = 0;
        m_u32H = 0;
        m_nBufBytes = 0;
        return false;
    }

    m_stStat = CCropStat_S();
    m_bReady = true;
    dlog_info("classroom crop: %ux%u %s x%u，每缓冲 %.1f KB", m_u32W, m_u32H,
              (ChannelOrder_E::CH_BGR == enOrder) ? "BGR888" : "RGB888", nBufNum,
              static_cast<double>(m_nBufBytes) / 1024.0);
    return true;
}

void CCropUnit::deinit()
{
    /* 顺序要紧：先放池（借出的缓冲由调用方 shared_ptr 续命，不会被踩），再清几何参数 */
    m_stPool.deinit();
    m_bReady = false;
    m_bLetterbox = false;
    m_u32W = 0;
    m_u32H = 0;
    m_nBufBytes = 0;
}

bool CCropUnit::cropWhole(const CFrame_S &stFrame, RgaBufPtr &rspBuf)
{
    rspBuf.reset();

    if (!m_bReady)
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64RejectArg;
        return false;
    }
    if (nullptr == stFrame.pucNv12 || stFrame.nSrcW <= 0 || stFrame.nSrcH <= 0 ||
        stFrame.nPixelBytes < nv12FrameBytes(stFrame.nSrcW, stFrame.nSrcH))
    {
        /* 字节数对不上 = 上游给了半帧或带 stride 的空洞缓冲；后者会让 RGA 按 stride==width
           的假定算错偏移，画面表现为规律性的斜切，所以宁可拒绝 */
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64RejectArg;
        return false;
    }

    rspBuf = m_stPool.acquire(m_nBufBytes);
    if (!rspBuf)
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64RejectNoBuf;
        return false;
    }

    /* letterbox：计划在 crop_geom（纯函数，与 inferWaitOne 建映射用的是同一个），
       这里只照做——先整块铺 114 灰，再把源帧等比写进顶部内容行。
       计划不可行（竖构图/退化尺寸）该帧回退拉伸：像素侧与坐标侧同用一份计划，
       两侧绝不会各退各的 */
    int nDstH = 0;
    if (m_bLetterbox)
    {
        const CLetterboxPlan_S stPlan =
            letterboxPlan(stFrame.nSrcW, stFrame.nSrcH, static_cast<int>(m_u32W), static_cast<int>(m_u32H));
        if (stPlan.bEnable)
        {
            std::memset(rspBuf->data(), CLASSROOM_LETTERBOX_PAD_GRAY, m_nBufBytes);
            nDstH = stPlan.nContentH;
        }
    }

    /* rotation 恒 0：整帧档服务的是检测/姿态，它们对镜像与倒置不敏感 */
    return transform(stFrame, 0, 0, stFrame.nSrcW, stFrame.nSrcH, rspBuf, 0, nDstH);
}

bool CCropUnit::cropRoi(const CFrame_S &stFrame, const CRectf_S &stRectSrc, RgaBufPtr &rspBuf,
                        CRectf_S *pstUsedSrc, int nRotation)
{
    rspBuf.reset();

    /* 先清出参：失败时调用方拿到的必须是空矩形，而不是上一次那帧的区域——
       拿旧区域建新映射的结果是「关键点整体偏移」这种既不复现也不报错的错 */
    if (nullptr != pstUsedSrc)
    {
        *pstUsedSrc = CRectf_S();
    }

    if (!m_bReady || nullptr == stFrame.pucNv12 || stFrame.nSrcW <= 0 || stFrame.nSrcH <= 0 ||
        stFrame.nPixelBytes < nv12FrameBytes(stFrame.nSrcW, stFrame.nSrcH))
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64RejectArg;
        return false;
    }

    /* 算式在 `crop_geom.cpp`：那一篇是纯函数，进得了主机闸门，因此"取偶方向、越界收缩、
       下限 2 像素、NaN 挡在转换之前"这四条都有逐条断言 */
    CRoiAlign_S stAlign;
    const RoiAlignBad_E enBad = alignRoiToRga(stFrame.nSrcW, stFrame.nSrcH, stRectSrc, stAlign);
    if (RoiAlignBad_E::ROI_OK != enBad)
    {
        /* 三档分开计：「框太小」是远焦小目标的正常业务，「框坏了」要去查下发的区域数据，
           「帧非法」要去查帧总线——混成一格就读不出方向 */
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        if (RoiAlignBad_E::ROI_TOO_SMALL == enBad)
        {
            ++m_stStat.u64RejectTooSmall;
        }
        else if (RoiAlignBad_E::ROI_BAD_RECT == enBad)
        {
            ++m_stStat.u64RejectBadRect;
        }
        else
        {
            ++m_stStat.u64RejectArg;
        }
        return false;
    }

    rspBuf = m_stPool.acquire(m_nBufBytes);
    if (!rspBuf)
    {
        std::lock_guard<std::mutex> stLock(m_mtxStat);
        ++m_stStat.u64RejectNoBuf;
        return false;
    }

    /* 与模型输入一一对应的就是这一块：起点是取偶后的 nCx/nCy，不是调用方给的浮点框。
       姿态关键点换算回分析帧时以它为准，否则偏移量随框大小变化（详见头文件出参说明） */
    if (nullptr != pstUsedSrc)
    {
        *pstUsedSrc = CRectf_S{ static_cast<float>(stAlign.nCx), static_cast<float>(stAlign.nCy),
                                static_cast<float>(stAlign.nCx + stAlign.nCw),
                                static_cast<float>(stAlign.nCy + stAlign.nCh) };
    }

    return transform(stFrame, stAlign.nCx, stAlign.nCy, stAlign.nCw, stAlign.nCh, rspBuf, nRotation);
}

bool CCropUnit::transform(const CFrame_S &stFrame, int nCx, int nCy, int nCw, int nCh, RgaBufPtr rspBuf,
                          int nRotation, int nDstH)
{
    /* nRotation 由调用方给 */
    const int nDstRows = (nDstH > 0 && nDstH < static_cast<int>(m_u32H)) ? nDstH : static_cast<int>(m_u32H);
    const bool bOk = rga_image_transform(const_cast<uint8_t *>(stFrame.pucNv12), stFrame.nSrcW, stFrame.nSrcH,
                                         RK_FORMAT_YCbCr_420_SP, rspBuf->data(),
                                         static_cast<int>(m_u32W), nDstRows, m_nDstFormat,
                                         nCx, nCy, nCw, nCh, nRotation);

    std::lock_guard<std::mutex> stLock(m_mtxStat);
    if (!bOk)
    {
        ++m_stStat.u64RgaFail;
        rspBuf.reset();
        return false;
    }
    ++m_stStat.u64Ok;
    return true;
}

CCropStat_S CCropUnit::stat() const
{
    std::lock_guard<std::mutex> stLock(m_mtxStat);
    return m_stStat;
}
}
