/**
 * @FilePath     : cover_draw.cpp
 * @Author       : huangjunda
 * @Date         : 2025-06-20 11:05:06
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-08-27 10:26:11
 * @Description  :
 */

#include "cover_draw.h"
#include "stream_video.h"


CCoverDraw::CCoverDraw()
{                                         /* rgn是否需要更新 */
}

CCoverDraw::~CCoverDraw()
{
}

IpcRet_E CCoverDraw::init()
{

    m_bIsRunning = false;                                          /* 运行标志 */
    m_pVecRgns.resize(MAX_OSD_COVER_NUM * VPSS_CHN_MAX); /* 初始化rgn指针向量 */
    m_bIsUpdate = true;                                            /* rgn是否需要更新 */

    std::vector<Osd::CoverInfo_S> vecCoverInfo;
    COsdManage::instance()->get_cover_info(vecCoverInfo);

    /* 判断是否存在osd信息 */
    if (!vecCoverInfo.size())
    {
        return ERR_PARAM_NULL;
    }

    /* 一个osd模板需要根据vpss通道数创建对应数量的rgn ,Cover最多8个，一个venc分配4个*/
     for (size_t i = 0; i < vecCoverInfo.size() * VPSS_CHN_MAX; i++)
     {
         int nRet = 0;
         int nIndex = i / VPSS_CHN_MAX; /* 下标 */
         int nChn = i % VPSS_CHN_MAX;   /* 码流通道 */

         /* 为rgn分配空间 */
         m_pVecRgns.at(i) = rockitRgn_alloc(set_rgn(vecCoverInfo.at(nIndex), nChn, i));
         if (NULL == m_pVecRgns.at(i))
         {
             dlog_error("HandleId:%d, rockitRgn_alloc error", i);
             return ERR;
         }
         /* 创建rgn */
         nRet = m_pVecRgns.at(i)->rockitRgn_create(m_pVecRgns.at(i));
         if (OK != nRet)
         {
             dlog_error("HandleId:%d, rockitRgn_create error: %d", i, nRet);
             return ERR;
         }
         if (vecCoverInfo.at(nIndex).stuInfo.bEnable)
         {
            /* rgn加入通道 */
            if (m_pVecRgns.at(i)->rockitRgn_attachToChn(m_pVecRgns.at(i)))
            {
                dlog_error("添加rgn到通道失败");
                return ERR;
            }
         } 

         m_bCoverThread[i] = true;

     }

    /* 开始启动rgn */
    start();

    dlog_info("osd_cover初始化成功");
    return OK;
}

IpcRet_E CCoverDraw::reinit(int nChn)
{

    std::vector<Osd::CoverInfo_S> vecCoverInfo;
    COsdManage::instance()->get_cover_info(vecCoverInfo);

    /* 判断是否存在osd信息 */
    if (!vecCoverInfo.size())
    {
        return ERR_PARAM_NULL;
    }

    /* 一个osd模板需要根据vpss通道数创建对应数量的rgn ,Cover最多8个，一个venc分配4个*/
     for (size_t i = 0; i < vecCoverInfo.size() ; i++)
     {
         int nRet = 0;
         int nIndex = i / VPSS_CHN_MAX; /* 下标 */
         int nChannel = i % VPSS_CHN_MAX;   /* 码流通道 */

         if(nChannel != nChn)
            continue;


        if(m_pVecRgns[i])
        m_pVecRgns.at(i)->RkRgn_update(m_pVecRgns.at(i), set_rgn(vecCoverInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
         /* 创建rgn */


          if (vecCoverInfo.at(nIndex).stuInfo.bEnable)
            m_pVecRgns.at(i)->bIsShow =(RK_BOOL)false;

         m_bCoverThread[i] = true;

     }

    m_bIsUpdate = true;                                            /* rgn是否需要更新 */

    dlog_info("osd_cover重新初始化成功");

    return OK;
}


IpcRet_E CCoverDraw::reDeinit(int nChn)
{

    /* 获取osd信息 */
    std::vector<Osd::CoverInfo_S> vecCoverInfo;
    vecCoverInfo.clear();
    COsdManage::instance()->get_cover_info(vecCoverInfo);

    /* 将相应的rgn从不同通道中撤出并释放空间 */
    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (NULL == m_pVecRgns.at(i))
        {
            continue;
        }

         int nChannel = i % VPSS_CHN_MAX;   /* 码流通道 */
         int nIndex = i / VPSS_CHN_MAX; /* 下标 */

         if(nChannel != nChn)
            continue;

        m_bCoverThread[i] = false;

        if (vecCoverInfo.at(nIndex).stuInfo.bEnable)
        {
             if (m_pVecRgns.at(i)->bIsShow)
            {  
                if (m_pVecRgns.at(i)->rockitRgn_detachFromChn(m_pVecRgns.at(i)))
                {
                    dlog_error("从通道中撤出rgn失败");
                }
            }
        }

        m_pVecRgns.at(i)->bIsShow =(RK_BOOL)false;

    }

    dlog_info("osd_cover重新去初始化成功");

    return OK;
}

IpcRet_E CCoverDraw::deinit()
{
    /* 停止rgn */
    stop();

    m_pVecRgns.clear();

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

RkRgnNeed_S CCoverDraw::set_rgn(Osd::CoverInfo_S stuCoverInfo, int nChn, uint32_t unHandle)
{
    /* 获取码流的分辨率大小 */
    std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
    CStreamVideo::instance()->getVideoConfig(vstVideoConfig);

    /* 配置rgn所需参数 */
    RkRgnNeed_S stuRgnNeedParam;
    memset(&stuRgnNeedParam, 0, sizeof(stuRgnNeedParam));
    stuRgnNeedParam.unOpFlag = 0;
    stuRgnNeedParam.unModId = RK_ID_VPSS;
    stuRgnNeedParam.unDevId = RGN_OSD_VPSS_DEVID;
    stuRgnNeedParam.unType = COVER_RGN;

    /* 实际分辨率宽高和参考分辨率宽高 */
    int nActualWidth = vstVideoConfig.at(nChn).stVideoResolution.nWidth;
    int nActualHeight = vstVideoConfig.at(nChn).stVideoResolution.nHeight;
    int nReferenceWidth = 0;
    int nReferenceHeight = 0;

    /* 获取模板参考分辨率宽高 */
    get_reference_size(stuCoverInfo.stuInfo.enRefSize, nReferenceWidth, nReferenceHeight);

    /* 句柄号 */
    stuRgnNeedParam.unHandle = unHandle;

    /* 通道号 */
    stuRgnNeedParam.unChnId = nChn;

    /* 区域层次 [0-7] */
    stuRgnNeedParam.unLayer = unHandle ;

    /* 是否显示 */
    stuRgnNeedParam.bIsShow = (RK_BOOL)stuCoverInfo.stuInfo.bEnable;

    /* 是否实心 */
    stuRgnNeedParam.bIsSolid = (RK_BOOL)stuCoverInfo.stuCover.bEnableSolid;

    /* 是否为矩形 */
    stuRgnNeedParam.bIsRectangle = (RK_BOOL)stuCoverInfo.stuCover.bEnableRectangle;

    /* 背景颜色   0xffffffff  */
    stuRgnNeedParam.unBgColor = std::stoul(stuCoverInfo.stuCover.strBackColor, NULL, 0); /* 16 表示十六进制 最后一位参数写0，自动检测字符串使用的进制*/

    /* 模板尺寸需要根据模板坐标等参数进行公式计算 */
    calculate_template_size(stuCoverInfo.stuCover.stuCoordinate, stuRgnNeedParam.unWidth, stuRgnNeedParam.unHeight, nActualWidth, nActualHeight, nReferenceWidth, nReferenceHeight);

    /* 计算模板坐标 */
    calculate_coordinate(stuCoverInfo.stuCover.stuCoordinate, nActualWidth, nActualHeight, nReferenceWidth, nReferenceHeight);

    if (stuCoverInfo.stuCover.bEnableRectangle)
    {
        stuRgnNeedParam.unStartX = stuCoverInfo.stuCover.stuCoordinate.at(Osd::Pos_E::POS_START).nX;
        stuRgnNeedParam.unStartY = stuCoverInfo.stuCover.stuCoordinate.at(Osd::Pos_E::POS_START).nY;
    }
    else
    {
        for (int i = 0; i < QUAD_POINT_NUM; i++)
        {
            stuRgnNeedParam.stuPoints[i].s32X = stuCoverInfo.stuCover.stuCoordinate.at(i).nX;
            stuRgnNeedParam.stuPoints[i].s32Y = stuCoverInfo.stuCover.stuCoordinate.at(i).nY;
        }
    }

    return stuRgnNeedParam;
}

void CCoverDraw::destroy_rgn()
{
    int nIndex = 0;
    /* 获取osd信息 */
    std::vector<Osd::CoverInfo_S> vecCoverInfo;
    vecCoverInfo.clear();
    COsdManage::instance()->get_cover_info(vecCoverInfo);

    /* 将相应的rgn从不同通道中撤出并释放空间 */
    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (NULL == m_pVecRgns.at(i))
        {
            continue;
        }

        nIndex = i / VPSS_CHN_MAX; /* 下标 */
        if (vecCoverInfo.at(nIndex).stuInfo.bEnable)
        {
            if (m_pVecRgns.at(i)->rockitRgn_detachFromChn(m_pVecRgns.at(i)))
            {
                dlog_error("从通道中撤出rgn失败");
            }
        }
        /* 销毁rgn */
        if (m_pVecRgns.at(i)->rockitRgn_destroy(m_pVecRgns.at(i)))
        {
            dlog_error("销毁rgn失败");
        }
        /* 释放rgn句柄 */
        rockitRgn_release(m_pVecRgns.at(i));
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
        if (m_bIsUpdate)
        {
            vecCoverInfo.clear();
            COsdManage::instance()->get_cover_info(vecCoverInfo);
            if (!vecCoverInfo.size())
            {
                /* 睡1秒判断一次 */
                sleep(1);
                continue;
            }
        }

        for (size_t i = 0; i < m_pVecRgns.size(); i++)
        {
            if (m_bCoverThread[i] == false || !m_pVecRgns.at(i))
            {
                continue;
            }

            int nIndex = i / VPSS_CHN_MAX; /* 下标 */

            if (vecCoverInfo.at(nIndex).stuInfo.bEnable)
            {
                  if(m_pVecRgns[i])
                    if (!m_pVecRgns.at(i)->bIsShow)
                    {
                        if(m_pVecRgns[i])
                            m_pVecRgns.at(i)->RkRgn_update(m_pVecRgns.at(i), set_rgn(vecCoverInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
                    
                        if(m_pVecRgns[i])
                            if (m_pVecRgns.at(i)->rockitRgn_attachToChn(m_pVecRgns.at(i)))
                            {
                                dlog_error("添加rgn到通道失败");
                                continue;
                            }
                    }
                /* 判断osd信息是否更新 */
                if (m_bIsUpdate)
                {
                    /* 更新rgn */
                    if(m_pVecRgns[i])
                    {
                        m_pVecRgns.at(i)->RkRgn_update(m_pVecRgns.at(i), set_rgn(vecCoverInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
                        m_pVecRgns.at(i)->rockitrgn_change_rect(m_pVecRgns.at(i), m_pVecRgns.at(i)->unWidth, m_pVecRgns.at(i)->unHeight);
                        m_pVecRgns.at(i)->rockitRgn_changePos(m_pVecRgns.at(i), m_pVecRgns.at(i)->unStartX, m_pVecRgns.at(i)->unStartY);
                        m_pVecRgns.at(i)->rockitRgn_detachFromChn(m_pVecRgns.at(i));
                        m_pVecRgns.at(i)->rockitRgn_attachToChn(m_pVecRgns.at(i));
                    }
                }

            }
            else if (!vecCoverInfo.at(nIndex).stuInfo.bEnable)
            {
                if(m_pVecRgns[i])
                    if (m_pVecRgns.at(i)->bIsShow)
                    {
                        if(m_pVecRgns[i])
                            if (m_pVecRgns.at(i)->rockitRgn_detachFromChn(m_pVecRgns.at(i)))
                            {
                                dlog_error("从通道中撤出rgn失败");
                                continue;
                            }
                    }

                /* 判断osd信息是否更新 */
                if (m_bIsUpdate)
                {
                    /* 更新rgn */
                    if(m_pVecRgns[i])
                        m_pVecRgns.at(i)->RkRgn_update(m_pVecRgns.at(i), set_rgn(vecCoverInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
                }
            }
        }

        if (m_bIsUpdate)
        {
            m_bIsUpdate = !m_bIsUpdate;
        }

        /* 休眠200ms */
        usleep(200);
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
    case Osd::ReferenceSize_E::REFERENCE_SIZE_4K:
        nWidth = PIXEL_WIDTH_4K;
        nHeight = PIXEL_HEIGHT_4K;
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
    unTemplateWidth = ALIGN_UP((stuCoordinate.at(Osd::Pos_E::POS_END).nX - stuCoordinate.at(Osd::Pos_E::POS_START).nX) * nActualWidth / nReferenceWidth, RGN_ALIGN);
    unTemplateHeight = ALIGN_UP((stuCoordinate.at(Osd::Pos_E::POS_END).nY - stuCoordinate.at(Osd::Pos_E::POS_START).nY) * nActualHeight / nReferenceHeight, RGN_ALIGN);
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

        // int x = stuCoordinate.at(Osd::Pos_E::POS_START).nX;
        // int y = stuCoordinate.at(Osd::Pos_E::POS_START).nY;
        // int w = stuCoordinate.at(Osd::Pos_E::POS_END).nX - stuCoordinate.at(Osd::Pos_E::POS_START).nX;
        // int h = stuCoordinate.at(Osd::Pos_E::POS_END).nY - stuCoordinate.at(Osd::Pos_E::POS_START).nY;

        // /* 180° 旋转（模板坐标系）*/
        // stuCoordinate.at(Osd::POS_START).nX = nReferenceWidth - (x + w);
        // stuCoordinate.at(Osd::POS_START).nY = nReferenceHeight - (y + h);
        // stuCoordinate.at(Osd::POS_END).nX   = nReferenceWidth - (x + w) + w;
        // stuCoordinate.at(Osd::POS_END).nY   = nReferenceHeight - (y + h)  + h;


        /* 模板起始坐标 = 起始坐标 * 实际分辨率长度 / 模板分辨率长度 */
        stuCoordinate.at(Osd::Pos_E::POS_START).nX = ALIGN_UP(stuCoordinate.at(Osd::Pos_E::POS_START).nX * nActualWidth / nReferenceWidth, RGN_ALIGN_2);
        stuCoordinate.at(Osd::Pos_E::POS_START).nY = ALIGN_UP(stuCoordinate.at(Osd::Pos_E::POS_START).nY * nActualHeight / nReferenceHeight, RGN_ALIGN_2);

        return;
    }
    /* 四边形: 四个坐标 */
    else if (QUAD_POINT_NUM == stuCoordinate.size())
    {
        for (int i = 0; i < QUAD_POINT_NUM; i++)
        {
            /* 模板坐标 = 坐标 * 实际分辨率长度 / 模板分辨率长度 */
            stuCoordinate.at(i).nX = ALIGN_UP(stuCoordinate.at(i).nX * nActualWidth / nReferenceWidth, RGN_ALIGN_2);
            stuCoordinate.at(i).nY = ALIGN_UP(stuCoordinate.at(i).nY * nActualHeight / nReferenceHeight, RGN_ALIGN_2);
        }
    }

    return;
}
