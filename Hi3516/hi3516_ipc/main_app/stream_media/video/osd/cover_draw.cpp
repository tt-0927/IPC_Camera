/**
 * @FilePath     : cover_draw.cpp
 * @Author       : huangjunda
 * @Date         : 2025-06-20 11:05:06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-30 15:14:08
 * @Description  : 遮挡绘制
 */

#include "cover_draw.h"

#include "osd_manage.h"
#include "stream_video.h"
#include "mpp_rgn.h"

CCoverDraw::CCoverDraw()
{
    m_bIsRunning = false; /* 运行标志 */
    m_pVecRgns.resize(RGN_COVER_MAX_NUM * VPSS_CHN_MAX); /* 初始化rgn指针向量 */
    m_bIsUpdate = true;                  /* rgn是否需要更新 */
}

CCoverDraw::~CCoverDraw()
{
}

IpcRet_E CCoverDraw::init()
{
    std::vector<Osd::CoverInfo_S> vecCoverInfo;
    COsdManage::instance()->get_cover_info(vecCoverInfo);

    /* 判断是否存在osd信息 */
    if (!vecCoverInfo.size())
    {
        return ERR_PARAM_NULL;
    }

    /* 一个osd模板需要根据vpss通道数创建对应数量的rgn */
    for (size_t i = 0; i < vecCoverInfo.size() * VPSS_CHN_MAX; i++)
    {
        int nRet = 0;
        int nIndex = i / VPSS_CHN_MAX; /* 下标 */
        int nChn = i % VPSS_CHN_MAX;   /* 码流通道 */

        /* 为rgn分配空间 */
        m_pVecRgns.at(i) = mppRgn_alloc(set_rgn(vecCoverInfo.at(nIndex), nChn, i));
        if (NULL == m_pVecRgns.at(i))
        {
            dlog_error("HandleId:%d, mppRgn_alloc error", i);
            return ERR;
        }
        /* 创建rgn */
        nRet = m_pVecRgns.at(i)->mppRgn_create(m_pVecRgns.at(i));
        if (OK != nRet)
        {
            dlog_error("HandleId:%d, mppRgn_create error: %d", i, nRet);
            return ERR;
        }
        /* rgn加入通道 */
        if (m_pVecRgns.at(i)->mppRgn_attachToChn(m_pVecRgns.at(i)))
        {
            dlog_error("添加rgn到通道失败");
            return ERR;
        }
    }

    /* 开始启动rgn */
    start();

    dlog_info("osd_cover初始化成功");

    return OK;
}

IpcRet_E CCoverDraw::deinit()
{
    /* 停止rgn */
    stop();

    return OK;
}

void CCoverDraw::set_update_flag(bool bIsUpdate)
{
    m_bIsUpdate = bIsUpdate;
}

void CCoverDraw::start()
{
    m_bIsRunning = true;

    m_thread = std::thread(&CCoverDraw::osd_cover, this);
    // osd_cover();

    return;
}

void CCoverDraw::stop()
{
    m_bIsRunning = false;

    if(m_thread.joinable())
    {
        m_thread.join();
    }

    /* 销毁rgn */
    destroy_rgn();

    return;
}

HiRgnNeedParam_S CCoverDraw::set_rgn(Osd::CoverInfo_S stuCoverInfo, int nChn, uint32_t unHandle)
{
    /* 获取码流配置，仅在运行时几何不可用时兜底。 */
    std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
    CStreamVideo::instance()->getVideoConfig(vstVideoConfig);

    /* 配置rgn所需参数 */
    HiRgnNeedParam_S stuRgnNeedParam;
    memset(&stuRgnNeedParam, 0, sizeof(stuRgnNeedParam));
    stuRgnNeedParam.unOpFlag = REGION_OP_CHN;
    stuRgnNeedParam.unModId = OT_ID_VPSS;
    stuRgnNeedParam.unDevId = RGN_OSD_VPSS;
    stuRgnNeedParam.unType = OT_RGN_COVER;

    /*
     * Cover 挂到 VPSS 的输出通道。VPSS 通道 Crop 生效后，Cover 的参考坐标必须落在
     * 裁剪输出画面，而不是持久化 VideoConfig 所表示的裁剪前画面。
     */
    Video_NS::StreamGeometry_S stGeometry;
    int nActualWidth = vstVideoConfig.at(nChn).stVideoResolution.nWidth;
    int nActualHeight = vstVideoConfig.at(nChn).stVideoResolution.nHeight;
    if (OK == CStreamVideo::instance()->get_stream_geometry(nChn, stGeometry) &&
        stGeometry.nOutputWidth > 0 && stGeometry.nOutputHeight > 0)
    {
        nActualWidth = stGeometry.nOutputWidth;
        nActualHeight = stGeometry.nOutputHeight;
    }
    int nReferenceWidth = 0;
    int nReferenceHeight = 0;

    /* 获取模板参考分辨率宽高 */
    get_reference_size(stuCoverInfo.stuInfo.enRefSize, nReferenceWidth, nReferenceHeight);
    
    /* 句柄号 */
    stuRgnNeedParam.unHandle = unHandle;

    /* 通道号 */
    stuRgnNeedParam.unChnId = nChn;

    /* 是否显示 */
    stuRgnNeedParam.bIsShow = (td_bool)stuCoverInfo.stuInfo.bEnable;

    /* 是否实心 */
    stuRgnNeedParam.bIsSolid = (td_bool)stuCoverInfo.stuCover.bEnableSolid;

    /* 是否为矩形 */
    stuRgnNeedParam.bIsRectangle = (td_bool)stuCoverInfo.stuCover.bEnableRectangle;

    /* 背景颜色 */
    stuRgnNeedParam.unBgColor = std::stoul(stuCoverInfo.stuCover.strBackColor, NULL, 16); /* 16 表示十六进制 */

    /* 模板尺寸需要根据模板坐标等参数进行公式计算 */
    calculate_template_size(stuCoverInfo.stuCover.stuCoordinate, stuRgnNeedParam.unWidth, stuRgnNeedParam.unHeight, nActualWidth, nActualHeight, nReferenceWidth, nReferenceHeight);

    /* 计算模板坐标 */
    calculate_coordinate(stuCoverInfo.stuCover.stuCoordinate, nActualWidth, nActualHeight, nReferenceWidth, nReferenceHeight);
    // dlog_debug("nActualWidth:%d*%d,%d*%d", nActualWidth, nActualHeight, nReferenceWidth, nReferenceHeight);
    // dlog_debug("nActualWidth:%d*%d,%d*%d", stuCoverInfo.stuCover.stuCoordinate[Osd::POS_START].nX, stuCoverInfo.stuCover.stuCoordinate[Osd::POS_START].nY, stuCoverInfo.stuCover.stuCoordinate[Osd::POS_END].nX, stuCoverInfo.stuCover.stuCoordinate[Osd::POS_END].nY);
    if (stuCoverInfo.stuCover.bEnableRectangle)
    {
        stuRgnNeedParam.unStartX = stuCoverInfo.stuCover.stuCoordinate.at(Osd::Pos_E::POS_START).nX;
        stuRgnNeedParam.unStartY = stuCoverInfo.stuCover.stuCoordinate.at(Osd::Pos_E::POS_START).nY;
    }
    else
    {
        for (int i = 0; i < OT_QUAD_POINT_NUM; i++)
        {
            stuRgnNeedParam.stuPoints[i].x = stuCoverInfo.stuCover.stuCoordinate.at(i).nX;
            stuRgnNeedParam.stuPoints[i].y = stuCoverInfo.stuCover.stuCoordinate.at(i).nY;
        }
    }

    return stuRgnNeedParam;
}

void CCoverDraw::destroy_rgn()
{
    /* 将相应的rgn从不同通道中撤出并释放空间 */
    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (NULL == m_pVecRgns.at(i))
        {
            continue;
        }
        if (m_pVecRgns.at(i)->mppRgn_detachFromChn(m_pVecRgns.at(i)))
        {
            dlog_error("从通道中撤出rgn失败");
        }
        /* 销毁rgn */
        if (m_pVecRgns.at(i)->mppRgn_destroy(m_pVecRgns.at(i)))
        {
            dlog_error("销毁rgn失败");
        }
        /* 释放rgn句柄 */
        mppRgn_release(m_pVecRgns.at(i));
        m_pVecRgns.at(i) = NULL;
    }
    return;
}

void CCoverDraw::osd_cover()
{
    pthread_setname_np(pthread_self(), "OsdCover");

    /* 获取osd信息 */
    std::vector<Osd::CoverInfo_S> vecCoverInfo;

    /* 保持显示 */
    while (m_bIsRunning)
    {
        /* 判断是否需要更新 */
        if (!m_bIsUpdate)
        {
            /* 睡1秒判断一次 */
            sleep(1);
            continue;
        }

        vecCoverInfo.clear();
        COsdManage::instance()->get_cover_info(vecCoverInfo);
        if (!vecCoverInfo.size())
        {
            /* 睡1秒判断一次 */
            sleep(1);
            continue;
        }

        for (size_t i = 0; i < m_pVecRgns.size(); i++)
        {
            if (!m_pVecRgns.at(i))
            {
                continue;
            }

            int nIndex = i / VPSS_CHN_MAX; /* 下标 */
            if (vecCoverInfo.at(nIndex).stuInfo.bEnable)
            {
                if (!m_pVecRgns.at(i)->bIsShow)
                {
                    if (m_pVecRgns.at(i)->mppRgn_attachToChn(m_pVecRgns.at(i)))
                    {
                        dlog_error("添加rgn到通道失败");
                        continue;
                    }
                }
                /* 判断osd信息是否更新 */
                if (m_bIsUpdate)
                {
                    /* 更新rgn */
                    m_pVecRgns.at(i)->mppRgn_update(m_pVecRgns.at(i), set_rgn(vecCoverInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
                    m_pVecRgns.at(i)->mppRgn_changeRect(m_pVecRgns.at(i), m_pVecRgns.at(i)->unWidth, m_pVecRgns.at(i)->unHeight);
                    m_pVecRgns.at(i)->mppRgn_changePos(m_pVecRgns.at(i), m_pVecRgns.at(i)->unStartX, m_pVecRgns.at(i)->unStartY);
                    m_pVecRgns.at(i)->mppRgn_detachFromChn(m_pVecRgns.at(i));
                    m_pVecRgns.at(i)->mppRgn_attachToChn(m_pVecRgns.at(i));
                }
            }
            else if (!vecCoverInfo.at(nIndex).stuInfo.bEnable)
            {
                if (m_pVecRgns.at(i)->bIsShow)
                {
                    if (m_pVecRgns.at(i)->mppRgn_detachFromChn(m_pVecRgns.at(i)))
                    {
                        dlog_error("从通道中撤出rgn失败");
                        continue;
                    }
                }
                /* 判断osd信息是否更新 */
                if (m_bIsUpdate)
                {
                    /* 更新rgn */
                    m_pVecRgns.at(i)->mppRgn_update(m_pVecRgns.at(i), set_rgn(vecCoverInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
                }
            }
        }

        if (m_bIsUpdate)
        {
            m_bIsUpdate = !m_bIsUpdate;
        }

        /* 休眠200ms */
        usleep(200 * 1000);
    }
}

void CCoverDraw::get_reference_size(Osd::ReferenceSize_E enReferenceSize, int &nWidth, int &nHeight)
{
    switch (enReferenceSize)
    {
    case Osd::ReferenceSize_E::REFERENCE_SIZE_640_384:
        nWidth = PIXEL_WIDTH_640;
        nHeight = PIXEL_HEIGHT_384;
        break;
    case Osd::ReferenceSize_E::REFERENCE_SIZE_480P:
        nWidth = PIXEL_WIDTH_640;
        nHeight = PIXEL_HEIGHT_480;
        break;
    case Osd::ReferenceSize_E::REFERENCE_SIZE_640_640:
        nWidth = PIXEL_WIDTH_640;
        nHeight = PIXEL_HEIGHT_640;
        break;
    case Osd::ReferenceSize_E::REFERENCE_SIZE_576P:
        nWidth = PIXEL_WIDTH_1024;
        nHeight = PIXEL_HEIGHT_576;
        break;
    case Osd::ReferenceSize_E::REFERENCE_SIZE_720P:
        nWidth = PIXEL_WIDTH_1280;
        nHeight = PIXEL_HEIGHT_720;
        break;
    case Osd::ReferenceSize_E::REFERENCE_SIZE_1080P:
        nWidth = PIXEL_WIDTH_1920;
        nHeight = PIXEL_HEIGHT_1080;
        break;
    case Osd::ReferenceSize_E::REFERENCE_SIZE_2K:
        nWidth = PIXEL_WIDTH_2K;
        nHeight = PIXEL_HEIGHT_2K;
        break;
    case Osd::ReferenceSize_E::REFERENCE_SIZE_2_5K:
        nWidth = PIXEL_WIDTH_2_5K;
        nHeight = PIXEL_HEIGHT_2_5K;
        break;
    default:
        break;
    }
    return;
}

void CCoverDraw::calculate_template_size(std::vector<Osd::CoordinateInfo_S> stuCoordinate, uint32_t &unTemplateWidth, uint32_t &unTemplateHeight, int nActualWidth, int nActualHeight, int nReferenceWidth, int nReferenceHeight)
{
    /* 起始坐标和结束坐标 */
    if (Osd::Pos_E::POS_MAX != stuCoordinate.size())
    {
        dlog_error("坐标没有两个,无法计算宽高");
        return;
    }

    /* 模板长度 = (结束坐标 - 起始坐标) * 实际分辨率长度 / 模板分辨率长度 */
    unTemplateWidth = ALIGN_UP((stuCoordinate.at(Osd::Pos_E::POS_END).nX - stuCoordinate.at(Osd::Pos_E::POS_START).nX) * nActualWidth / nReferenceWidth, OT_RGN_ALIGN);
    unTemplateHeight = ALIGN_UP((stuCoordinate.at(Osd::Pos_E::POS_END).nY - stuCoordinate.at(Osd::Pos_E::POS_START).nY) * nActualHeight / nReferenceHeight, OT_RGN_ALIGN);
    if (0 == unTemplateWidth)
    {
        unTemplateWidth = 2; /* 最小宽度 */
    }
    if (0 == unTemplateHeight)
    {
        unTemplateHeight = 2; /* 最小高度 */
    }

    return;
}

void CCoverDraw::calculate_coordinate(std::vector<Osd::CoordinateInfo_S> &stuCoordinate, int nActualWidth, int nActualHeight, int nReferenceWidth, int nReferenceHeight)
{
    /* 矩形: 起始坐标和结束坐标 */
    if (Osd::Pos_E::POS_MAX == stuCoordinate.size())
    {
        /* 模板起始坐标 = 起始坐标 * 实际分辨率长度 / 模板分辨率长度 */
        stuCoordinate.at(Osd::Pos_E::POS_START).nX = ALIGN_UP(stuCoordinate.at(Osd::Pos_E::POS_START).nX * nActualWidth / nReferenceWidth, OT_RGN_ALIGN);
        stuCoordinate.at(Osd::Pos_E::POS_START).nY = ALIGN_UP(stuCoordinate.at(Osd::Pos_E::POS_START).nY * nActualHeight / nReferenceHeight, OT_RGN_ALIGN);
        return;
    }
    /* 四边形: 四个坐标 */
    else if (OT_QUAD_POINT_NUM == stuCoordinate.size())
    {
        for (int i = 0; i < OT_QUAD_POINT_NUM; i++)
        {
            /* 模板坐标 = 坐标 * 实际分辨率长度 / 模板分辨率长度 */
            stuCoordinate.at(i).nX = ALIGN_UP(stuCoordinate.at(i).nX * nActualWidth / nReferenceWidth, OT_RGN_ALIGN);
            stuCoordinate.at(i).nY = ALIGN_UP(stuCoordinate.at(i).nY * nActualHeight / nReferenceHeight, OT_RGN_ALIGN);
        }
    }

    return;
}
