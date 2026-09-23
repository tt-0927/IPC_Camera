/**
 * @FilePath     : cover_draw.cpp
 * @Author       : huangjunda
 * @Date         : 2025-06-20 11:05:06
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 17:07:55
 * @Description  : 遮挡绘制
 */

#include "cover_draw.h"

#include "osd_manage.h"
#include "stream_video.h"
#include "mpp_rgn.h"

CCoverDraw::CCoverDraw()
{
    m_bIsRunning = false;                                /* 运行标志 */
    m_pVecRgns.resize(RGN_COVER_MAX_NUM * VPSS_CHN_MAX); /* 初始化rgn指针向量 */
    m_bIsUpdate = true;                                  /* rgn是否需要更新 */
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

    if (m_thread.joinable())
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
    if (OK == CStreamVideo::instance()->get_stream_geometry(nChn, stGeometry) && stGeometry.nOutputWidth > 0 &&
        stGeometry.nOutputHeight > 0)
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
    stuRgnNeedParam.bIsShow = (td_bool) stuCoverInfo.stuInfo.bEnable;

    /* 是否实心 */
    stuRgnNeedParam.bIsSolid = (td_bool) stuCoverInfo.stuCover.bEnableSolid;

    /* 是否为矩形 */
    stuRgnNeedParam.bIsRectangle = (td_bool) stuCoverInfo.stuCover.bEnableRectangle;

    /* 背景颜色 */
    stuRgnNeedParam.unBgColor = std::stoul(stuCoverInfo.stuCover.strBackColor, NULL, 16); /* 16 表示十六进制 */

    /* 模板尺寸需要根据模板坐标等参数进行公式计算 */
    calculate_template_size(stuCoverInfo.stuCover.stuCoordinate,
                            stuRgnNeedParam.unWidth,
                            stuRgnNeedParam.unHeight,
                            nActualWidth,
                            nActualHeight,
                            nReferenceWidth,
                            nReferenceHeight);

    /* 计算模板坐标 */
    calculate_coordinate(stuCoverInfo.stuCover.stuCoordinate, nActualWidth, nActualHeight, nReferenceWidth, nReferenceHeight);
    // dlog_debug("nActualWidth:%d*%d,%d*%d", nActualWidth, nActualHeight, nReferenceWidth, nReferenceHeight);
    // dlog_debug("nActualWidth:%d*%d,%d*%d", stuCoverInfo.stuCover.stuCoordinate[Osd::POS_START].nX,
    // stuCoverInfo.stuCover.stuCoordinate[Osd::POS_START].nY, stuCoverInfo.stuCover.stuCoordinate[Osd::POS_END].nX,
    // stuCoverInfo.stuCover.stuCoordinate[Osd::POS_END].nY);
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

        bool bMainChanged = false; /* 本轮主码流通道(chn0)的遮挡是否发生实际变更 */

        for (size_t i = 0; i < m_pVecRgns.size(); i++)
        {
            if (!m_pVecRgns.at(i))
            {
                continue;
            }

            int nIndex = i / VPSS_CHN_MAX; /* 下标 */
            if (vecCoverInfo.at(nIndex).stuInfo.bEnable)
            {
                /* 判断osd信息是否更新 */
                if (m_bIsUpdate)
                {
                    /* 先计算新参数：与当前一致且处于显示状态时跳过，
                     * 重复设置也会触发卷绕通道与RGN的冲突，必须拦截 */
                    const HiRgnNeedParam_S stParam = set_rgn(vecCoverInfo.at(nIndex),
                                                             m_pVecRgns.at(i)->unChnId,
                                                             m_pVecRgns.at(i)->unHandle);
                    if (m_pVecRgns.at(i)->bIsAttached && m_pVecRgns.at(i)->bIsShow && m_pVecRgns.at(i)->unStartX == stParam.unStartX &&
                        m_pVecRgns.at(i)->unStartY == stParam.unStartY && m_pVecRgns.at(i)->unWidth == stParam.unWidth &&
                        m_pVecRgns.at(i)->unHeight == stParam.unHeight)
                    {
                        continue;
                    }

                    /* update会覆盖内存中的显隐状态，先保存驱动当前真实显隐 */
                    const td_bool bWasShow = m_pVecRgns.at(i)->bIsShow;
                    if (m_pVecRgns.at(i)->unChnId == 0)
                    {
                        bMainChanged = true; /* 主码流为卷绕通道，RGN变更后需重建编码通道 */
                    }
                    /* 参数变更统一走 隐藏→改参数→显示：
                     * 实测卷绕通道上遮挡在叠加运行中直接收缩参数（大面积改小）会让
                     * VENC数秒后停止编码且不可恢复；隐藏期间硬件不叠加该RGN，
                     * 参数就位后再按新参数从零开始叠加，规避参数突变窗口 */
                    if (bWasShow && m_pVecRgns.at(i)->mppRgn_showOrHide(m_pVecRgns.at(i), TD_FALSE))
                    {
                        dlog_error("隐藏rgn失败");
                    }
                    /* 更新rgn内存参数 */
                    m_pVecRgns.at(i)->mppRgn_update(m_pVecRgns.at(i), stParam);
                    if (!m_pVecRgns.at(i)->bIsAttached)
                    {
                        /* 首次挂载：attach按句柄当前参数一次性写入通道显示属性 */
                        if (m_pVecRgns.at(i)->mppRgn_attachToChn(m_pVecRgns.at(i)))
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
                        if (m_pVecRgns.at(i)->mppRgn_changeAttr(m_pVecRgns.at(i),
                                                                m_pVecRgns.at(i)->unStartX,
                                                                m_pVecRgns.at(i)->unStartY,
                                                                m_pVecRgns.at(i)->unWidth,
                                                                m_pVecRgns.at(i)->unHeight))
                        {
                            dlog_error("更新rgn位置尺寸失败");
                        }
                        /* 此时驱动侧必为隐藏态：参数变更路径是刚才主动隐藏，
                         * 禁用恢复路径是禁用时隐藏的，统一恢复显示 */
                        if (m_pVecRgns.at(i)->mppRgn_showOrHide(m_pVecRgns.at(i), TD_TRUE))
                        {
                            dlog_error("恢复rgn显示失败");
                        }
                    }
                }
            }
            else if (!vecCoverInfo.at(nIndex).stuInfo.bEnable)
            {
                /* 禁用只隐藏不摘挂，保持通道RGN列表结构稳定 */
                if (m_pVecRgns.at(i)->bIsShow)
                {
                    if (m_pVecRgns.at(i)->mppRgn_showOrHide(m_pVecRgns.at(i), TD_FALSE))
                    {
                        dlog_error("隐藏rgn失败");
                        continue;
                    }
                    if (m_pVecRgns.at(i)->unChnId == 0)
                    {
                        bMainChanged = true; /* 主码流为卷绕通道，RGN变更后需重建编码通道 */
                    }
                }
                /* 判断osd信息是否更新 */
                if (m_bIsUpdate)
                {
                    /* 更新rgn */
                    m_pVecRgns.at(i)->mppRgn_update(
                        m_pVecRgns.at(i),
                        set_rgn(vecCoverInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
                }
            }
        }

        if (m_bIsUpdate)
        {
            m_bIsUpdate = !m_bIsUpdate;
        }

        if (bMainChanged)
        {
            /* 实测：主码流为卷绕(wrap online)通道时，对VPSS chn0的遮挡RGN做任何
             * 动态变更（含隐藏-改参数-显示）都会概率性打坏VENC编码状态——
             * 卷绕send-done出现差帧、码流停止产出且不可自愈（proc表现为
             * pic queue busy堆积、stream buffer full反复打印）。
             * RGN参数已就位（挂在VPSS上，不受VENC重建影响），此处按当前配置
             * 走既有路径重建主码流编码通道恢复出图，断流约1~2秒 */
            dlog_info("遮挡参数已变更, 重建主码流编码通道");
            if (OK != CStreamVideo::instance()->restart_encode_channel(0 /* VENC_CHN_MAIN */))
            {
                dlog_error("重建主码流编码通道失败");
            }
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

void CCoverDraw::calculate_template_size(std::vector<Osd::CoordinateInfo_S> stuCoordinate,
                                         uint32_t &unTemplateWidth,
                                         uint32_t &unTemplateHeight,
                                         int nActualWidth,
                                         int nActualHeight,
                                         int nReferenceWidth,
                                         int nReferenceHeight)
{
    /* 起始坐标和结束坐标 */
    if (Osd::Pos_E::POS_MAX != stuCoordinate.size())
    {
        dlog_error("坐标没有两个,无法计算宽高");
        return;
    }

    /* 模板长度 = (结束坐标 - 起始坐标) * 实际分辨率长度 / 模板分辨率长度 */
    unTemplateWidth = ALIGN_UP(
        (stuCoordinate.at(Osd::Pos_E::POS_END).nX - stuCoordinate.at(Osd::Pos_E::POS_START).nX) * nActualWidth / nReferenceWidth,
        OT_RGN_ALIGN);
    unTemplateHeight = ALIGN_UP(
        (stuCoordinate.at(Osd::Pos_E::POS_END).nY - stuCoordinate.at(Osd::Pos_E::POS_START).nY) * nActualHeight / nReferenceHeight,
        OT_RGN_ALIGN);
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

void CCoverDraw::calculate_coordinate(std::vector<Osd::CoordinateInfo_S> &stuCoordinate,
                                      int nActualWidth,
                                      int nActualHeight,
                                      int nReferenceWidth,
                                      int nReferenceHeight)
{
    /* 矩形: 起始坐标和结束坐标 */
    if (Osd::Pos_E::POS_MAX == stuCoordinate.size())
    {
        /* 模板起始坐标 = 起始坐标 * 实际分辨率长度 / 模板分辨率长度 */
        stuCoordinate.at(Osd::Pos_E::POS_START).nX = ALIGN_UP(stuCoordinate.at(Osd::Pos_E::POS_START).nX * nActualWidth / nReferenceWidth,
                                                              OT_RGN_ALIGN);
        stuCoordinate.at(Osd::Pos_E::POS_START).nY = ALIGN_UP(stuCoordinate.at(Osd::Pos_E::POS_START).nY * nActualHeight / nReferenceHeight,
                                                              OT_RGN_ALIGN);
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
