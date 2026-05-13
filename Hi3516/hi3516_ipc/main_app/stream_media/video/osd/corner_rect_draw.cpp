/**
 * @FilePath     : corner_rect_draw.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-11-17 09:18:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-12-08 17:33:27
 * @Description  : 角框类型绘制：AI动态分析用
 */

#include "corner_rect_draw.h"

#include "osd_manage.h"
#include "stream_video.h"
#include "mpp_rgn.h"

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
    /* 获取码流的分辨率大小 */
    std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
    CStreamVideo::instance()->getVideoConfig(vstVideoConfig);

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
            /* 转换坐标 */
            Common::RectInfo_S stRectInfo = vRectInfo[nIndex];
            stRectInfo.ConvertResolution(nWidth, nHeight, vstVideoConfig[nChn].stVideoResolution.nWidth, vstVideoConfig[nChn].stVideoResolution.nHeight);
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
