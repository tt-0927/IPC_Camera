/**
 * @FilePath     : corner_rect_draw.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-11-17 09:18:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 17:07:19
 * @Description  : 角框类型绘制：AI动态分析用
 */

#include "corner_rect_draw.h"

#include "osd_manage.h"
#include "stream_video.h"
#include "mpp_rgn.h"

#include <algorithm>

namespace
{
/**
 * @brief   : 将 AI 检测框转换到指定码流裁剪后的有效输出坐标系
 * @param    {Common::RectInfo_S} stSourceRect：检测结果坐标
 * @param    {int} nDetectWidth：检测结果参考宽度
 * @param    {int} nDetectHeight：检测结果参考高度
 * @param    {Video_NS::StreamGeometry_S} stGeometry：目标码流有效几何
 * @param    {Common::RectInfo_S &} stOutputRect：输出的目标坐标
 * @return   {bool} true：存在可见区域 false：检测框完全落在裁剪区外
 * @note    : 先缩放到裁剪前 VPSS 源画面，再减去 Crop 偏移并缩放到实际 VENC 输出。
 */
bool convert_rect_to_stream_output(const Common::RectInfo_S &stSourceRect,
                                   int nDetectWidth,
                                   int nDetectHeight,
                                   const Video_NS::StreamGeometry_S &stGeometry,
                                   Common::RectInfo_S &stOutputRect)
{
    if (nDetectWidth <= 0 || nDetectHeight <= 0 || stGeometry.nSourceWidth <= 0 || stGeometry.nSourceHeight <= 0 ||
        stGeometry.nOutputWidth <= 0 || stGeometry.nOutputHeight <= 0)
    {
        return false;
    }

    int nX1 = stSourceRect.nX1 * stGeometry.nSourceWidth / nDetectWidth;
    int nY1 = stSourceRect.nY1 * stGeometry.nSourceHeight / nDetectHeight;
    int nX2 = stSourceRect.nX2 * stGeometry.nSourceWidth / nDetectWidth;
    int nY2 = stSourceRect.nY2 * stGeometry.nSourceHeight / nDetectHeight;

    int nCropX = 0;
    int nCropY = 0;
    int nCropWidth = stGeometry.nSourceWidth;
    int nCropHeight = stGeometry.nSourceHeight;
    if (stGeometry.bCropEnable)
    {
        nCropX = stGeometry.nCropX;
        nCropY = stGeometry.nCropY;
        nCropWidth = stGeometry.nCropWidth;
        nCropHeight = stGeometry.nCropHeight;
    }
    if (nCropWidth <= 0 || nCropHeight <= 0)
    {
        return false;
    }

    const int nCropRight = nCropX + nCropWidth;
    const int nCropBottom = nCropY + nCropHeight;
    nX1 = std::max(nX1, nCropX);
    nY1 = std::max(nY1, nCropY);
    nX2 = std::min(nX2, nCropRight);
    nY2 = std::min(nY2, nCropBottom);
    if (nX2 <= nX1 || nY2 <= nY1)
    {
        return false;
    }

    stOutputRect.nX1 = (nX1 - nCropX) * stGeometry.nOutputWidth / nCropWidth;
    stOutputRect.nY1 = (nY1 - nCropY) * stGeometry.nOutputHeight / nCropHeight;
    stOutputRect.nX2 = (nX2 - nCropX) * stGeometry.nOutputWidth / nCropWidth;
    stOutputRect.nY2 = (nY2 - nCropY) * stGeometry.nOutputHeight / nCropHeight;
    return stOutputRect.nX2 > stOutputRect.nX1 && stOutputRect.nY2 > stOutputRect.nY1;
}

/**
 * @brief   : 隐藏角框区域，已处于隐藏状态时不再重复设置
 * @param    {HiRgn_S *} pHandle：区域句柄
 * @return   无
 */
void hide_region(HiRgn_S *pHandle)
{
    if (NULL == pHandle || !pHandle->bIsShow)
    {
        return;
    }
    pHandle->mppRgn_showOrHide(pHandle, TD_FALSE);
}
}

CCornerRectDraw::CCornerRectDraw()
{
    m_pVecRgns.resize(RGN_CORNER_RECT_MAX_NUM * VPSS_CHN_MAX); /* 初始化rgn指针向量 */
}

CCornerRectDraw::~CCornerRectDraw()
{
}

IpcRet_E CCornerRectDraw::init()
{
    /* 一个osd模板需要根据vpss通道数创建对应数量的rgn */
    for (size_t i = 0; i < RGN_CORNER_RECT_MAX_NUM * VPSS_CHN_MAX; i++)
    {
        int nRet = 0;
        int nChn = i % VPSS_CHN_MAX; /* 码流通道 */
        Common::RectInfo_S stRectInfo;
        stRectInfo.nX1 = -2;
        stRectInfo.nY1 = -2;
        stRectInfo.nX2 = 0;
        stRectInfo.nY2 = 0;
        /* 为rgn分配空间 */
        m_pVecRgns[i] = mppRgn_alloc(set_rgn(nChn, i, stRectInfo));
        if (NULL == m_pVecRgns[i])
        {
            dlog_error("句柄ID:%d, 分配区域句柄失败", i);
            return ERR;
        }
        /* 创建rgn */
        nRet = m_pVecRgns[i]->mppRgn_create(m_pVecRgns[i]);
        if (OK != nRet)
        {
            dlog_error("句柄ID:%d, 创建rgn失败: %d", i, nRet);
            return ERR;
        }
        /* rgn加入通道 */
        if (m_pVecRgns[i]->mppRgn_attachToChn(m_pVecRgns[i]))
        {
            dlog_error("添加rgn到通道失败");
            return ERR;
        }
    }

    dlog_info("CornerRectDraw 初始化成功");
    return OK;
}

IpcRet_E CCornerRectDraw::deinit()
{
    /* 销毁rgn */
    /* 将相应的rgn从不同通道中撤出并释放空间 */
    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (NULL == m_pVecRgns[i])
        {
            continue;
        }
        if (m_pVecRgns[i]->mppRgn_detachFromChn(m_pVecRgns[i]))
        {
            dlog_error("从通道中撤出rgn失败");
        }
        /* 销毁rgn */
        if (m_pVecRgns[i]->mppRgn_destroy(m_pVecRgns[i]))
        {
            dlog_error("销毁rgn失败");
        }
        /* 释放rgn句柄 */
        mppRgn_release(m_pVecRgns[i]);
        m_pVecRgns[i] = NULL;
    }

    dlog_info("CornerRectDraw 去初始化成功");
    return OK;
}

void CCornerRectDraw::update_ai_result(int nWidth, int nHeight, const std::vector<Common::RectInfo_S> &vRectInfo)
{
    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (!m_pVecRgns[i])
        {
            continue;
        }

        size_t nIndex = i / VPSS_CHN_MAX; /* 下标 */
        size_t nChn = i % VPSS_CHN_MAX;   /* 码流通道 */

        if (vRectInfo.size() > nIndex)
        {
            /* 将算法坐标转换到 VPSS Crop 后、RGN 实际显示的码流坐标系。 */
            Video_NS::StreamGeometry_S stGeometry;
            Common::RectInfo_S stRectInfo;
            if (OK != CStreamVideo::instance()->get_stream_geometry(static_cast<int>(nChn), stGeometry) ||
                !convert_rect_to_stream_output(vRectInfo[nIndex], nWidth, nHeight, stGeometry, stRectInfo))
            {
                hide_region(m_pVecRgns[i]);
                continue;
            }

            /* 框位置与尺寸都没变且处于显示状态时跳过，避免高频AI回调反复驱动调用 */
            if (m_pVecRgns[i]->bIsShow && m_pVecRgns[i]->unStartX == stRectInfo.nX1 && m_pVecRgns[i]->unStartY == stRectInfo.nY1 &&
                m_pVecRgns[i]->unWidth == (uint32_t) (stRectInfo.nX2 - stRectInfo.nX1) &&
                m_pVecRgns[i]->unHeight == (uint32_t) (stRectInfo.nY2 - stRectInfo.nY1))
            {
                continue;
            }

            /* update会覆盖内存中的显隐状态，先保存驱动当前真实显隐 */
            const td_bool bWasShow = m_pVecRgns[i]->bIsShow;
            /* 参数变更统一走 隐藏→改参数→显示：
             * 实测卷绕通道上RGN在叠加运行中直接收缩参数（大面积改小）会让
             * VENC数秒后停止编码且不可恢复；隐藏期间硬件不叠加该RGN，
             * 参数就位后再按新参数从零开始叠加，规避参数突变窗口 */
            if (bWasShow && m_pVecRgns[i]->mppRgn_showOrHide(m_pVecRgns[i], TD_FALSE))
            {
                dlog_error("隐藏rgn失败");
            }
            /* 更新rgn内存参数 */
            m_pVecRgns[i]->mppRgn_update(m_pVecRgns[i], set_rgn(m_pVecRgns[i]->unChnId, m_pVecRgns[i]->unHandle, stRectInfo));
            if (!m_pVecRgns[i]->bIsAttached)
            {
                /* 首次挂载：attach按句柄当前参数一次性写入通道显示属性 */
                if (m_pVecRgns[i]->mppRgn_attachToChn(m_pVecRgns[i]))
                {
                    dlog_error("添加rgn到通道失败");
                }
            }
            else
            {
                /* 已挂载时只原地更新位置与尺寸，不做摘挂：
                 * attach/detach会增删通道RGN列表结构，卷绕在线通道行级直送VENC
                 * 无帧边界同步点，读到增删瞬间的中间状态会导致VENC编码停摆；
                 * 原地改属性不改变列表结构，硬件按帧读取的始终是完整配置 */
                if (m_pVecRgns[i]->mppRgn_changeAttr(m_pVecRgns[i],
                                                     m_pVecRgns[i]->unStartX,
                                                     m_pVecRgns[i]->unStartY,
                                                     m_pVecRgns[i]->unWidth,
                                                     m_pVecRgns[i]->unHeight))
                {
                    dlog_error("更新rgn位置尺寸失败");
                }
                /* 此时驱动侧必为隐藏态：参数变更路径是刚才主动隐藏，
                 * 隐藏恢复路径是之前AI无目标时隐藏的，统一恢复显示 */
                if (m_pVecRgns[i]->mppRgn_showOrHide(m_pVecRgns[i], TD_TRUE))
                {
                    dlog_error("恢复rgn显示失败");
                }
            }
        }
        else
        {
            hide_region(m_pVecRgns[i]);
        }
    }
}

void CCornerRectDraw::clear_channel(int nChn)
{
    if (nChn < 0 || nChn >= VPSS_CHN_MAX)
    {
        return;
    }

    for (HiRgn_S *pHandle : m_pVecRgns)
    {
        if (pHandle && pHandle->unChnId == nChn)
        {
            hide_region(pHandle);
        }
    }
}

HiRgnNeedParam_S CCornerRectDraw::set_rgn(int nChn, uint32_t unHandle, const Common::RectInfo_S &stRectInfo)
{
    /* 配置rgn所需参数 */
    HiRgnNeedParam_S stuRgnNeedParam;
    memset(&stuRgnNeedParam, 0, sizeof(stuRgnNeedParam));
    stuRgnNeedParam.unOpFlag = REGION_OP_CHN;
    stuRgnNeedParam.unModId = OT_ID_VPSS;
    stuRgnNeedParam.unDevId = RGN_OSD_VPSS;
    stuRgnNeedParam.unType = OT_RGN_CORNER_RECT;

    /* 句柄号 */
    stuRgnNeedParam.unHandle = unHandle;
    /* 通道号 */
    stuRgnNeedParam.unChnId = nChn;
    /* 是否显示 */
    stuRgnNeedParam.bIsShow = TD_TRUE;
    /* 背景颜色 */
    stuRgnNeedParam.unBgColor = 0xFFFFFF;
    /* 前景颜色 */
    stuRgnNeedParam.unFgColor = 0x00FF00;
    /* 叠加层次 */
    stuRgnNeedParam.unLayer = 1;
    /* 角框水平线长 */
    stuRgnNeedParam.uHorLen = 2;
    /* 角框竖直线长 */
    stuRgnNeedParam.uVerLen = 2;
    /* 角框线宽 */
    stuRgnNeedParam.uThick = 4;
    /* 闪烁 */
    stuRgnNeedParam.bIsFlicker = TD_TRUE;
    /* 起始坐标 */
    stuRgnNeedParam.unStartX = stRectInfo.nX1;
    stuRgnNeedParam.unStartY = stRectInfo.nY1;
    /* 宽高信息 */
    stuRgnNeedParam.unWidth = stRectInfo.nX2 - stRectInfo.nX1;
    stuRgnNeedParam.unHeight = stRectInfo.nY2 - stRectInfo.nY1;
    if (stuRgnNeedParam.unWidth <= 2)
    {
        stuRgnNeedParam.unWidth = 2;
        stuRgnNeedParam.unStartX = -2;
    }
    if (stuRgnNeedParam.unHeight <= 2)
    {
        stuRgnNeedParam.unHeight = 2;
        stuRgnNeedParam.unStartY = -2;
    }

    return stuRgnNeedParam;
}
