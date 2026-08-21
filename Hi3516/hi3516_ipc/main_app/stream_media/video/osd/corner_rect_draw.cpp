/**
 * @FilePath     : corner_rect_draw.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-11-17 09:18:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-30 15:14:08
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
    if (nDetectWidth <= 0 || nDetectHeight <= 0 || stGeometry.nSourceWidth <= 0 ||
        stGeometry.nSourceHeight <= 0 || stGeometry.nOutputWidth <= 0 || stGeometry.nOutputHeight <= 0)
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
        size_t nChn = i % VPSS_CHN_MAX; /* 码流通道 */
   
        if (vRectInfo.size() > nIndex)
        {
            /* 将算法坐标转换到 VPSS Crop 后、RGN 实际显示的码流坐标系。 */
            Video_NS::StreamGeometry_S stGeometry;
            Common::RectInfo_S stRectInfo;
            if (OK != CStreamVideo::instance()->get_stream_geometry(static_cast<int>(nChn), stGeometry) ||
                !convert_rect_to_stream_output(vRectInfo[nIndex], nWidth, nHeight, stGeometry, stRectInfo))
            {
                m_pVecRgns[i]->mppRgn_showOrHide(m_pVecRgns[i], TD_FALSE);
                continue;
            }
            /* 更新rgn */
            m_pVecRgns[i]->mppRgn_update(m_pVecRgns[i], set_rgn(m_pVecRgns[i]->unChnId, m_pVecRgns[i]->unHandle, stRectInfo));
            m_pVecRgns[i]->mppRgn_changePos(m_pVecRgns[i], m_pVecRgns[i]->unStartX, m_pVecRgns[i]->unStartY);
            m_pVecRgns[i]->mppRgn_changeRect(m_pVecRgns[i], m_pVecRgns[i]->unWidth, m_pVecRgns[i]->unHeight);
            m_pVecRgns[i]->mppRgn_showOrHide(m_pVecRgns[i], TD_TRUE);
        }
        else
        {
            m_pVecRgns[i]->mppRgn_showOrHide(m_pVecRgns[i], TD_FALSE);
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
            pHandle->mppRgn_showOrHide(pHandle, TD_FALSE);
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
