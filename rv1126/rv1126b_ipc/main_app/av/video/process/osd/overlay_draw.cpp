/***
 * @FilePath     : overplay_draw.cpp
 * @Author       : huangjunda
 * @Date         : 2025-05-27 14:12:40
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-08-26 14:19:49
 * @Description  :
 */

#include "overlay_draw.h"

#include "osd_manage.h"
#include "share_define.h"
#include "video_define.h"
#include "stream_video.h"

#include "freetype.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>

namespace
{
#if CAP_EXHIBITION_OSD_PANEL
/* 展会面板专用 overlay 槽位下标。 */
constexpr size_t EXHIBITION_PANEL_OVERPLAY_INDEX = Osd::ElementType_E::ELEMENT_TYPE_MAC; //(ELEMENT_TYPE_MAC暂未使用，借用)
/* 展会面板刷新周期，单位毫秒。 */
constexpr int EXHIBITION_PANEL_RENDER_INTERVAL_MS = 250;
/* 展会面板缓存有效期，单位毫秒。 */
constexpr int EXHIBITION_PANEL_VALID_TIME_MS = 1500;
/* 展会面板左上角水平和垂直边距。 */
constexpr int EXHIBITION_PANEL_MARGIN_PX = 24;
/* 展会面板参考字号。 */
constexpr int EXHIBITION_PANEL_FONT_SIZE = 24;

/**
 * @brief   : 判断当前 RGN 句柄是否属于展会面板
 * @param    {size_t} nHandle：RGN 句柄下标
 * @return   {bool} true：属于展会面板 false：不属于
 */
inline bool is_exhibition_panel_handle(size_t nHandle)
{
    return (nHandle / VPSS_CHN_MAX) == EXHIBITION_PANEL_OVERPLAY_INDEX;
}

/**
 * @brief   : 获取当前稳态时钟毫秒时间戳
 * @return   {uint64_t} 稳态时钟毫秒值
 */
uint64_t get_steady_time_ms()
{
    /* 当前稳态时钟时间点。 */
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

/**
 * @brief   : 构造展会面板专用 overlay 配置
 * @return   {Osd::OverplayInfo_S} 展会面板 overlay 配置
 */
Osd::OverplayInfo_S build_exhibition_panel_info()
{
    /* 组装后的展会面板 overlay 配置。 */
    Osd::OverplayInfo_S stOverplayInfo;
    stOverplayInfo.clear();
    stOverplayInfo.stuInfo.nID = static_cast<int>(EXHIBITION_PANEL_OVERPLAY_INDEX) + 1;
    stOverplayInfo.stuInfo.bEnable = false;
    stOverplayInfo.stuInfo.strName = "ExhibitionPanel";
    stOverplayInfo.stuInfo.enRefSize = Osd::REFERENCE_SIZE_1080P;

    stOverplayInfo.stuOverplay.enElementType = Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE;
    stOverplayInfo.stuOverplay.enAlign = Osd::Align_E::ALIGN_TOP_LEFT;
    stOverplayInfo.stuOverplay.nHorMargin = EXHIBITION_PANEL_MARGIN_PX;
    stOverplayInfo.stuOverplay.nVerMargin = EXHIBITION_PANEL_MARGIN_PX;
    stOverplayInfo.stuOverplay.nFontSize = EXHIBITION_PANEL_FONT_SIZE;
    stOverplayInfo.stuOverplay.strFontColor = "0xFFFFFF";
    stOverplayInfo.stuOverplay.strBackColor = "0x162028";
    stOverplayInfo.stuOverplay.nFontAlpha = 0;
    stOverplayInfo.stuOverplay.nBackAlpha = 55;
    stOverplayInfo.stuOverplay.strCustomize.clear();
    return stOverplayInfo;
}
/**
 * @brief   : 判断文本是否包含换行符
 * @param    {const char *} pText：原始文本
 * @return   {bool} true：包含换行 false：单行文本
 */
inline bool is_multi_line_text(const char *pText)
{
    return pText && std::strchr(pText, '\n');
}

/**
 * @brief   : 按换行符拆分多行文本
 * @param    {const std::string &} strText：原始文本
 * @return   {std::vector<std::string>} 拆分后的文本行列表
 */
std::vector<std::string> split_text_lines(const std::string &strText)
{
    /* 拆分后的文本行列表。 */
    std::vector<std::string> vecLines;
    /* 用于逐行读取的字符串流。 */
    std::stringstream ss(strText);
    /* 当前读取到的单行文本。 */
    std::string strLine;
    while (std::getline(ss, strLine))
    {
        vecLines.emplace_back(strLine);
    }

    if (!strText.empty() && strText.back() == '\n')
    {
        vecLines.emplace_back("");
    }
    if (vecLines.empty())
    {
        vecLines.emplace_back(strText);
    }
    return vecLines;
}

/**
 * @brief   : 根据字体大小计算多行文本行间距
 * @param    {int} nFontSize：当前字体大小
 * @return   {int} 行间距像素值
 */
int get_multi_line_spacing(int nFontSize)
{
    return std::max(2, nFontSize / 3);
}
#endif
} // namespace


COverlayDraw::COverlayDraw()
{                                
}

COverlayDraw::~COverlayDraw()
{
}

IpcRet_E COverlayDraw::init()
{
    m_bIsRunning = false;                                     /* 运行标志 */
    m_pVecRgns.resize(MAX_OSD_OVERLAY_NUM * VPSS_CHN_MAX); /* 初始化rgn指针向量 */
    OS_mutexCreate(&m_stuMutex);                                   /* 创建锁 */
    m_bIsTimeUpdate = true;                                        /* 时间信息rgn是否需要更新 */
    m_bIsOthersUpdate = true;                                      /* 其他信息rgn是否需要更新 */
            
    /* 获取osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
    COsdManage::instance()->get_overplay_info(vecOverplayInfo);

    /* 判断是否存在osd信息 */
    if (!vecOverplayInfo.size())
    {
        return ERR_PARAM_NULL;
    }

    /* 一个osd模板需要根据venc通道数创建对应数量的rgn */
    for (size_t i = 0; i < vecOverplayInfo.size() * VPSS_CHN_MAX; i++)
    {
        int nRet = 0;
        int nIndex = i / VPSS_CHN_MAX; /* 下标 */
        int nChn = i % VPSS_CHN_MAX;   /* 码流通道 */

        /* 为rgn分配空间 */
        m_pVecRgns.at(i) = rockitRgn_alloc(set_rgn(vecOverplayInfo.at(nIndex), nChn, i));
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
        
        if (vecOverplayInfo.at(nIndex).stuInfo.bEnable)
        {
            /* rgn加入通道 */
            if (m_pVecRgns.at(i)->rockitRgn_attachToChn(m_pVecRgns.at(i)))
            {
                dlog_error("添加rgn到通道失败");
                return ERR;
            }
        } 
        m_bOverlayThread[i] = true;
        
    }
    
    /* 初始化TTF */
    if (TTF_Init() < 0)
    {
        dlog_error("TTF初始化失败: %s", SDL_GetError());
        return ERR;
    }
    
    /* 初始化SDL (虽然不是必需，但最好初始化) */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        dlog_error("SDL初始化失败: %s", SDL_GetError());
        return ERR;
    }
    /* 开始rgn线程 */
    start();
    dlog_info("osd_overplay初始化成功");
    return OK;
}


IpcRet_E COverlayDraw::reinit(int nChn)
{
    /* 获取osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
    vecOverplayInfo.clear();
    COsdManage::instance()->get_overplay_info(vecOverplayInfo);

    /* 判断是否存在osd信息 */
    if (!vecOverplayInfo.size())
    {
        return ERR_PARAM_NULL;
    }

    /* 一个osd模板需要根据venc通道数创建对应数量的rgn */
    for (size_t i = 0; i < vecOverplayInfo.size() * VPSS_CHN_MAX; i++)
    {
        int nRet = 0;
        int nIndex = i / VPSS_CHN_MAX; /* 下标 */
        int nChannel = i % VPSS_CHN_MAX;   /* 码流通道 */

        if(nChannel != nChn)
            continue;

        /* 为rgn分配空间 */
        m_pVecRgns.at(i) = rockitRgn_alloc(set_rgn(vecOverplayInfo.at(nIndex), nChannel, i));
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

        if (vecOverplayInfo.at(nIndex).stuInfo.bEnable)
        {
                /* rgn加入通道 */
                if (m_pVecRgns.at(i)->rockitRgn_attachToChn(m_pVecRgns.at(i)))
                {
                    dlog_error("添加rgn到通道失败");
                    return ERR;
                }
        } 
        m_bOverlayThread[i] = true;
        
    }
    m_bIsTimeUpdate = true;                                       
    m_bIsOthersUpdate = true;  

    dlog_info("osd_overplay重新初始化成功");
    return OK;
}

IpcRet_E COverlayDraw::reDeinit(int nChn)
{
    /* 获取osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
    vecOverplayInfo.clear();
    COsdManage::instance()->get_overplay_info(vecOverplayInfo);

    /* 将相应的rgn从不同通道中撤出并释放空间 */
    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (NULL == m_pVecRgns.at(i))
        {
            continue;
        }
        
        int nChannel = i % VPSS_CHN_MAX;   /* 码流通道 */
        int  nIndex = i / VPSS_CHN_MAX;

        if(nChannel != nChn)
            continue;

        m_bOverlayThread[i] = false;

        // if(vecOverplayInfo.at(nIndex).stuInfo.bEnable)
        // {

        //     if (m_pVecRgns.at(i)->rockitRgn_detachFromChn(m_pVecRgns.at(i)))
        //     {
        //             dlog_error("从通道中撤出rgn失败");
        //     }
        // }

        /* 销毁rgn */
         if (m_pVecRgns.at(i)->rockitRgn_destroy(m_pVecRgns.at(i)))
         {
             dlog_error("销毁rgn失败");
         }
        /* 释放rgn句柄 */
        rockitRgn_release(m_pVecRgns.at(i));
        m_pVecRgns.at(i) = NULL;
    }

    return OK;
}

IpcRet_E COverlayDraw::deinit()
{
    /* 停止rgn */
    stop();
    /* 删除锁 */
    OS_mutexDelete(&m_stuMutex);
    /* 释放绘制检测框覆盖图片的Surface */
    for (auto const& [handle, surface] : m_mapOverlaySurfaces)
    {
        if (surface)
        {
            SDL_FreeSurface(surface);
        }
    }

    m_pVecRgns.clear();
    m_mapOverlaySurfaces.clear();
    /* 去初始化SDL */
    SDL_Quit();
    /* 去初始化TTF */
    TTF_Quit();
    return OK;
}

void COverlayDraw::set_update_flag(bool bIsUpdate)
{
    m_bIsTimeUpdate = bIsUpdate;
    m_bIsOthersUpdate = bIsUpdate;
}

double get_time_ms() 
{
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double, std::milli>(duration).count();
}


void COverlayDraw::update_ai_result(int nWidth, int nHeight, const std::vector<Common::RectInfo_S> &vRectInfo, const Osd::OverplayInfo_S stOverplayInfo)
{
    if(!m_bIsRunning) return;

    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (!m_pVecRgns[i] || Osd::ELEMENT_TYPE_PEOPLE != m_pVecRgns[i]->unLayer)
            continue;
        
        if (vRectInfo.empty()) {
            m_pVecRgns[i]->rockitRgn_clearPicture(m_pVecRgns[i]);
            continue;
        }

        uint32_t canvas_w = m_pVecRgns[i]->unWidth;
        uint32_t canvas_h = m_pVecRgns[i]->unHeight;

        float fW = static_cast<float>(canvas_w) / nWidth;
        float fH = static_cast<float>(canvas_h) / nHeight;

        std::vector<Common::RectInfo_S> vRect = vRectInfo;
        for (auto &rect : vRect) {
            //  16 像素对齐
            int x1 = ((int)(rect.nX1 * fW) / 16) * 16;
            int y1 = ((int)(rect.nY1 * fH) / 16) * 16;
            int x2 = ((int)(rect.nX2 * fW + 15) / 16) * 16;
            int y2 = ((int)(rect.nY2 * fH + 15) / 16) * 16;

            rect.nX1 = std::max(0, x1);
            rect.nY1 = std::max(0, y1);
            rect.nX2 = std::min((int)canvas_w - 16, x2);
            rect.nY2 = std::min((int)canvas_h - 16, y2);
        }

        if (m_drawMutex.try_lock()) {
            //double dTime = get_time_ms();
            draw_rect(m_pVecRgns[i], stOverplayInfo, vRect);
            //dlog_warn("执行画框now time:%f - draw_rect time: %f - i:%d",dTime,get_time_ms() - dTime,i);
            m_drawMutex.unlock();
        }
    }
}


void COverlayDraw::start()
{
    m_bIsRunning = true;

    m_showTimeThread = std::thread([this]()
                                   { this->osd_show_time(); });
    // m_showTimeThread.detach();

    m_showOthersThread = std::thread([this]()
                                     { this->osd_show_others(); });
    // m_showOthersThread.detach();

#if CAP_EXHIBITION_OSD_PANEL
    m_showExhibitionPanelThread = std::thread([this]()
                                              { this->osd_show_exhibition_panel(); });
#endif

    m_flickerThread = std::thread([this]()
                                  { this->osd_flicker(); });
    // m_flickerThread.detach();

    return;
}

void COverlayDraw::stop()
{
    m_bIsRunning = false;

    /* 等待所有线程结束 */
    if (m_flickerThread.joinable())
    {
        m_flickerThread.join();
    }
    if (m_showOthersThread.joinable())
    {
        m_showOthersThread.join();
    }
#if CAP_EXHIBITION_OSD_PANEL
    if (m_showExhibitionPanelThread.joinable())
    {
        m_showExhibitionPanelThread.join();
    }
#endif
    if (m_showTimeThread.joinable())
    {
        m_showTimeThread.join();
    }
    /* 销毁rgn */
    destroy_rgn();

    return;
}

RkRgnNeed_S COverlayDraw::set_rgn(Osd::OverplayInfo_S stuOverplayInfo, int nChn, uint32_t unHandle, uint32_t unWidth, uint32_t unHeight)
{
    /* 获取osd共用信息 */
    Osd::ShareInfo_S stuShareInfo;
    
    COsdManage::instance()->get_osd_share_info(stuShareInfo);

    /* 获取码流的分辨率大小 */
    std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
    CStreamVideo::instance()->getVideoConfig(vstVideoConfig);

    /* 配置rgn所需参数 */
   RkRgnNeed_S stuRgnNeedParam;
    memset(&stuRgnNeedParam, 0, sizeof(stuRgnNeedParam));
    stuRgnNeedParam.unOpFlag = 0;
    stuRgnNeedParam.unModId = RK_ID_VENC;
    stuRgnNeedParam.unDevId = RGN_OSD_VENC;
    //stuRgnNeedParam.unChnId = VENC_CHN_MAIN;
    stuRgnNeedParam.unType = OVERLAY_RGN;

    /* 实际分辨率宽高和参考分辨率宽高 */
    int nActualWidth = vstVideoConfig.at(nChn).stVideoResolution.nWidth;
    int nActualHeight = vstVideoConfig.at(nChn).stVideoResolution.nHeight;
    int nReferenceWidth = 0;
    int nReferenceHeight = 0;
    // int nFontSize = 0;
    int nHorMargin = 0;
    int nVerMargin = 0;
    int nX = 0;
    int nY = 0;

    /* 获取当前模板内容信息 */
    std::string strInfo = get_template_text(stuOverplayInfo, stuShareInfo);

    /* 获取模板参考分辨率宽高 */
    get_reference_size(stuOverplayInfo.stuInfo.enRefSize, nReferenceWidth, nReferenceHeight);

    stuRgnNeedParam.bIsShow = (RK_BOOL)stuOverplayInfo.stuInfo.bEnable;
    stuRgnNeedParam.unHandle = unHandle;

    /* 判断长宽是否已经由画图的大小确定 */
    if (unWidth && unHeight)
    {
        stuRgnNeedParam.unWidth = ALIGN_UP(unWidth, RGN_ALIGN);
        stuRgnNeedParam.unHeight = ALIGN_UP(unHeight,RGN_ALIGN);
    }
    else
    {
        if(stuOverplayInfo.stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_PEOPLE)
        {
            // note AI 动态分析专用
            stuRgnNeedParam.unWidth = ALIGN_BACK(nActualWidth, RGN_ALIGN);
            stuRgnNeedParam.unHeight = ALIGN_BACK(nActualHeight, RGN_ALIGN);
        }
        else
        {
            
            /* 模板尺寸需要根据模板内容等参数进行公式计算 */
            stuRgnNeedParam.unWidth = calculate_template_width(strInfo.length(), stuOverplayInfo.stuOverplay.nFontSize, nActualWidth, nReferenceWidth);
            stuRgnNeedParam.unHeight = calculate_template_height(stuOverplayInfo.stuOverplay.nFontSize, nActualHeight, nReferenceHeight);
            if (0 == stuRgnNeedParam.unWidth)
            {
                stuRgnNeedParam.unWidth = stuRgnNeedParam.unHeight;
            }
        }
    }

    stuRgnNeedParam.unChnId = nChn;

    /* 获取模板边距 */
    get_template_margin(stuOverplayInfo, nHorMargin, nVerMargin, nActualWidth, nActualHeight, nReferenceWidth, nReferenceHeight);
    stuRgnNeedParam.unHorMargin = nHorMargin;
    stuRgnNeedParam.unVerMargin = nVerMargin;

    /* 获取模板起始坐标点 */
    get_start_points(stuOverplayInfo.stuOverplay.enAlign, nX, nY, nHorMargin, nVerMargin, (int)stuRgnNeedParam.unWidth, (int)stuRgnNeedParam.unHeight, nActualWidth, nActualHeight);
    stuRgnNeedParam.unStartX = nX;
    stuRgnNeedParam.unStartY = nY;

    /* 转换透明度由百分比到rgb范围(0~255) */
    stuRgnNeedParam.unFgAlpha = algha_percentage_to_rgb(stuOverplayInfo.stuOverplay.nFontAlpha);
    stuRgnNeedParam.unBgAlpha = algha_percentage_to_rgb(stuOverplayInfo.stuOverplay.nBackAlpha);
    /* 转换颜色（BGR 格式） - RGN是RK_FMT_BGRA8888*/
    uint32_t fg_rgb = std::stoul(stuOverplayInfo.stuOverplay.strFontColor, NULL, 16);
    uint32_t bg_rgb = std::stoul(stuOverplayInfo.stuOverplay.strBackColor, NULL, 16);
    stuRgnNeedParam.unFgColor = RGB_TO_BGR(fg_rgb);
    stuRgnNeedParam.unBgColor = RGB_TO_BGR(bg_rgb);

    stuRgnNeedParam.unLayer = stuOverplayInfo.stuOverplay.enElementType;

    /* 获取模板字体大小 */
    stuRgnNeedParam.unFontSize = calculate_text_size(stuOverplayInfo.stuOverplay.nFontSize, nActualWidth, nReferenceWidth);

    /* 获取模板闪烁开关 */
    stuRgnNeedParam.bIsFlicker = (RK_BOOL)stuOverplayInfo.stuOverplay.bEnableFlicker;

    return stuRgnNeedParam;
}

void COverlayDraw::destroy_rgn()
{
    int nIndex = 0;
    /* 获取osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
    vecOverplayInfo.clear();
    COsdManage::instance()->get_overplay_info(vecOverplayInfo);

    /* 将相应的rgn从不同通道中撤出并释放空间 */
    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (NULL == m_pVecRgns.at(i))
        {
            continue;
        }
        
        nIndex = i / VPSS_CHN_MAX;
        if(vecOverplayInfo.at(nIndex).stuInfo.bEnable)
        {
            if(m_pVecRgns[i])
                if (m_pVecRgns.at(i)->rockitRgn_detachFromChn(m_pVecRgns.at(i)))
                {
                    dlog_error("从通道中撤出rgn失败");
                }
        }
        /* 销毁rgn */
        if(m_pVecRgns[i])
            if (m_pVecRgns.at(i)->rockitRgn_destroy(m_pVecRgns.at(i)))
            {
                dlog_error("销毁rgn失败");
            }
        /* 释放rgn句柄 */
        if(m_pVecRgns[i])
            rockitRgn_release(m_pVecRgns.at(i));
        m_pVecRgns.at(i) = NULL;
    }
    return;
}

void COverlayDraw::osd_show_time()
{
    pthread_setname_np(pthread_self(), "OsdShowTime");

    int nIndex = 0;
    time_t stuLastTime = 0;
    time_t stuNowTime = time(NULL);

    /* 获取osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
    COsdManage::instance()->get_overplay_info(vecOverplayInfo);

    /* 获取osd共用信息 */
    Osd::ShareInfo_S stuShareInfo;

    /* 保持显示 */
    while (m_bIsRunning)
    {
        vecOverplayInfo.clear();
        
        COsdManage::instance()->get_overplay_info(vecOverplayInfo);
        if (!vecOverplayInfo.size() || (!vecOverplayInfo.at(Osd::ElementType_E::ELEMENT_TYPE_TIME).stuInfo.bEnable && !m_bIsTimeUpdate))
        {
            /* 睡1秒判断一次 */
            sleep(1);
            continue;
        }

        stuNowTime = time(NULL);
        if (stuNowTime != stuLastTime)
        {
            /* 对相应的rgn画出对应文本 */
            for (size_t i = 0; i < m_pVecRgns.size(); i++)
            {
                 if (m_bOverlayThread[i] == false || !m_pVecRgns.at(i) || Osd::ElementType_E::ELEMENT_TYPE_TIME != m_pVecRgns.at(i)->unLayer)
                 {
                     continue;
                 }

                nIndex = i / VPSS_CHN_MAX;
                if (vecOverplayInfo.at(nIndex).stuInfo.bEnable)
                {
                    if(m_pVecRgns[i])
                        if (!m_pVecRgns.at(i)->bIsShow)
                        {
                            
                             if(m_pVecRgns[i])
                                if (m_pVecRgns.at(i)->rockitRgn_attachToChn(m_pVecRgns.at(i)))
                                {
                                    dlog_error("添加rgn到通道失败");
                                    continue;
                                }
                        }
                        
                    /* 判断osd信息是否更新 */
                    if (m_bIsTimeUpdate)
                    {
                        /* 更新rgn */
                        if(m_pVecRgns[i])
                             m_pVecRgns.at(i)->RkRgn_update(m_pVecRgns.at(i), set_rgn(vecOverplayInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle, m_pVecRgns.at(i)->unWidth, m_pVecRgns.at(i)->unHeight));
                            
                    }

                    /* 获取osd共用信息 */
                    COsdManage::instance()->get_osd_share_info(stuShareInfo);
                    if(m_pVecRgns[i])
                        if (m_pVecRgns.at(i)->bIsFlicker && (stuNowTime % 2) && stuLastTime)
                        {
                            /* 奇数显示达到闪烁效果 */
                            if(m_pVecRgns[i])
                                if (m_pVecRgns.at(i)->rockitRgn_showOrHide(m_pVecRgns.at(i), (RK_BOOL)((stuNowTime % 2) ? true : false)))
                                { 
                                    dlog_error("设置rgn闪烁失败");
                                }
                        }

                    /* 对相应的rgn画出对应文本 */
                    OS_mutexLock(&m_stuMutex);
                    
                    if(m_pVecRgns[i])
                        draw_text(m_pVecRgns.at(i), vecOverplayInfo.at(nIndex), get_template_text(vecOverplayInfo.at(nIndex), stuShareInfo).c_str());
                    
                    OS_mutexUnlock(&m_stuMutex);
                    
                    if(m_pVecRgns[i])
                        if (m_pVecRgns.at(i)->bIsFlicker && !(stuNowTime % 2) && stuLastTime)
                        {
                            /* 偶数隐藏达到闪烁效果 */
                            if(m_pVecRgns[i])
                                if (m_pVecRgns.at(i)->rockitRgn_showOrHide(m_pVecRgns.at(i), (RK_BOOL)((stuNowTime % 2) ? true : false)))
                                {
                                    dlog_error("设置rgn闪烁失败");
                                }
                        }
                }
                else if (!vecOverplayInfo.at(nIndex).stuInfo.bEnable)
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
                            /* 判断osd信息是否更新 */
                            if (m_bIsTimeUpdate)
                            {
                                /* 更新rgn */
                                if(m_pVecRgns[i])
                                    m_pVecRgns.at(i)->RkRgn_update(m_pVecRgns.at(i), set_rgn(vecOverplayInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle, m_pVecRgns.at(i)->unWidth, m_pVecRgns.at(i)->unHeight));
                            }
                        }
                }
            }
            
            if (m_bIsTimeUpdate)
            {
                m_bIsTimeUpdate = !m_bIsTimeUpdate;
            }
            stuLastTime = stuNowTime;

            /* 计算到下一秒剩余的时间（毫秒级精度） */
            struct timeval tv;
            gettimeofday(&tv, NULL);
            long current_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
            long next_second_ms = (tv.tv_sec + 1) * 1000;
            long sleep_ms = next_second_ms - current_ms;

            /* 确保睡眠时间合理（0~1000ms） */
            if (sleep_ms > 0 && sleep_ms <= 1000)
            {
                usleep(sleep_ms * 1000);
            }
            else
            {
                sleep(1);
            }
        }
    }

    return;
}

void COverlayDraw::osd_show_others()
{
    pthread_setname_np(pthread_self(), "OsdShowOthers");

    int nIndex = 0;
    /* osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;

    /* osd共用信息 */
    Osd::ShareInfo_S stuShareInfo;
    COsdManage::instance()->get_osd_share_info(stuShareInfo);

    /* 保持显示 */
    while (m_bIsRunning)
    {
        /* 判断是否需要更新 */
        if (!m_bIsOthersUpdate)
        {
            /* 睡1秒判断一次 */
            sleep(1);
            continue;
        }

        vecOverplayInfo.clear();
        COsdManage::instance()->get_overplay_info(vecOverplayInfo);
        COsdManage::instance()->get_osd_share_info(stuShareInfo);
        if (!vecOverplayInfo.size())
        {
            /* 睡1秒判断一次 */
            sleep(1);
            continue;
        }

        /* 对相应的rgn画出对应文本 */
        for (size_t i = 0; i < m_pVecRgns.size(); i++)
        {
            bool bSkipOverlay = m_bOverlayThread[i] == false || !m_pVecRgns.at(i) || Osd::ElementType_E::ELEMENT_TYPE_TIME == m_pVecRgns.at(i)->unLayer || Osd::ElementType_E::ELEMENT_TYPE_PEOPLE == m_pVecRgns.at(i)->unLayer;
#if CAP_EXHIBITION_OSD_PANEL
            bSkipOverlay = bSkipOverlay || is_exhibition_panel_handle(i);
#endif
            if (bSkipOverlay)
            {
                continue;
            }

            nIndex = i / VPSS_CHN_MAX; /* 下标 */
            if (vecOverplayInfo.at(nIndex).stuInfo.bEnable)
            {
                if(m_pVecRgns[i])
                    if (!m_pVecRgns.at(i)->bIsShow)
                    {
                        if(m_pVecRgns[i])
                            if (m_pVecRgns.at(i)->rockitRgn_attachToChn(m_pVecRgns.at(i)))
                            {
                                dlog_error("添加rgn到通道失败");
                                continue;
                            }
                    }
                /* 更新rgn */
                if(m_pVecRgns[i])
                    m_pVecRgns.at(i)->RkRgn_update(m_pVecRgns.at(i), set_rgn(vecOverplayInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));

                /* 对相应的rgn画出对应文本 */
                OS_mutexLock(&m_stuMutex);
                if(m_pVecRgns[i])
                    draw_text(m_pVecRgns.at(i), vecOverplayInfo.at(nIndex), get_template_text(vecOverplayInfo.at(nIndex), stuShareInfo).c_str());
                OS_mutexUnlock(&m_stuMutex);
            }
            else if (!vecOverplayInfo.at(nIndex).stuInfo.bEnable)
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
                /* 更新rgn */
                if(m_pVecRgns[i])
                    m_pVecRgns.at(i)->RkRgn_update(m_pVecRgns.at(i), set_rgn(vecOverplayInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
            }
        }
        if (m_bIsOthersUpdate)
        {
            m_bIsOthersUpdate = !m_bIsOthersUpdate;
        }
    }

    return;
}

#if CAP_EXHIBITION_OSD_PANEL
void COverlayDraw::osd_show_exhibition_panel()
{
    pthread_setname_np(pthread_self(), "OsdShowExpo");

    /* 展会面板隐藏态 overlay 配置。 */
    const Osd::OverplayInfo_S stHiddenPanelInfo = build_exhibition_panel_info();
    /* 展会面板显示态 overlay 配置。 */
    Osd::OverplayInfo_S stVisiblePanelInfo = stHiddenPanelInfo;
    stVisiblePanelInfo.stuInfo.bEnable = true;

    /* 上一次成功绘制的面板文本。 */
    std::string strLastText;
    /* 上一次循环中面板是否处于可见状态。 */
    bool bLastVisible = false;

    while (m_bIsRunning)
    {
        /* OSD 管理层缓存的结构化面板结果。 */
        OsdPanel::PanelFrame_S stPanelFrame;
        /* 面板缓存版本号。 */
        uint64_t unVersion = 0;
        /* 面板缓存更新时间。 */
        uint64_t unUpdateTimeMs = 0;
        COsdManage::instance()->get_panel_result(stPanelFrame, unVersion, unUpdateTimeMs);

        /* 当前稳态时钟时间。 */
        const uint64_t unNowTimeMs = get_steady_time_ms();
        /* 当前缓存是否仍处于有效展示窗口内。 */
        bool bPanelValid = unVersion > 0 &&
                           !stPanelFrame.vecItems.empty() &&
                           unNowTimeMs >= unUpdateTimeMs &&
                           (unNowTimeMs - unUpdateTimeMs) <= EXHIBITION_PANEL_VALID_TIME_MS;
        /* 当前帧最终需要绘制的面板文本。 */
        std::string strPanelText;
        if (bPanelValid)
        {
            strPanelText = OsdPanel::build_panel_text(stPanelFrame);
            bPanelValid = !strPanelText.empty();
        }

        for (size_t i = 0; i < m_pVecRgns.size(); ++i)
        {
            /* 当前 handle 对应的 RGN 句柄。 */
            RkRgn_S *pHandle = m_pVecRgns.at(i);
            if (!pHandle || !is_exhibition_panel_handle(i))
            {
                continue;
            }

            if (!bPanelValid)
            {
                if (pHandle->bIsShow)
                {
                    pHandle->rockitRgn_detachFromChn(pHandle);
                    pHandle->RkRgn_update(pHandle, set_rgn(stHiddenPanelInfo, pHandle->unChnId, pHandle->unHandle));
                }
                continue;
            }

            if (!pHandle->bIsShow)
            {
                pHandle->RkRgn_update(pHandle, set_rgn(stVisiblePanelInfo, pHandle->unChnId, pHandle->unHandle));
                if (pHandle->rockitRgn_attachToChn(pHandle))
                {
                    dlog_error("展会面板添加rgn到通道失败");
                    continue;
                }
            }

            if (strPanelText != strLastText || !bLastVisible || !pHandle->bIsShow)
            {
                pHandle->RkRgn_update(pHandle, set_rgn(stVisiblePanelInfo, pHandle->unChnId, pHandle->unHandle));
                OS_mutexLock(&m_stuMutex);
                draw_text(pHandle, stVisiblePanelInfo, strPanelText.c_str());
                OS_mutexUnlock(&m_stuMutex);
            }
        }

        strLastText = bPanelValid ? strPanelText : "";
        bLastVisible = bPanelValid;
        usleep(EXHIBITION_PANEL_RENDER_INTERVAL_MS * 1000);
    }
}
#endif

void COverlayDraw::osd_flicker()
{
    pthread_setname_np(pthread_self(), "OsdFlicker");

    /* 睡1秒再进入循环（确保rgn已经添加到通道上） */
    sleep(1);
    while (m_bIsRunning)
    {
        time_t stuNowTime = time(NULL);
        for (size_t i = 0; i < m_pVecRgns.size(); i++)
        {
            bool bSkipOverlay = m_bOverlayThread[i] == false ||  !m_pVecRgns.at(i) || Osd::ELEMENT_TYPE_TIME == m_pVecRgns.at(i)->unLayer || Osd::ElementType_E::ELEMENT_TYPE_PEOPLE == m_pVecRgns.at(i)->unLayer;
#if CAP_EXHIBITION_OSD_PANEL
            bSkipOverlay = bSkipOverlay || is_exhibition_panel_handle(i);
#endif
            if (bSkipOverlay)
            {
                continue;
            }
            if(m_pVecRgns[i])
                if (m_pVecRgns.at(i)->bIsShow && m_pVecRgns.at(i)->bIsFlicker)
                {
                    /* 奇数显示, 偶数隐藏 */
                    if(m_pVecRgns[i])
                        if (m_pVecRgns.at(i)->rockitRgn_showOrHide(m_pVecRgns.at(i), (RK_BOOL)((stuNowTime % 2) ? true : false)))
                        {
                            dlog_error("设置rgn闪烁失败");
                        }
                }
        }

        /* 计算到下一秒剩余的时间（毫秒级精度） */
        struct timeval tv;
        gettimeofday(&tv, NULL);
        long current_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        long next_second_ms = (tv.tv_sec + 1) * 1000;
        long sleep_ms = next_second_ms - current_ms;

        /* 确保睡眠时间合理（0~1000ms） */
        if (sleep_ms > 0 && sleep_ms <= 1000)
        {
            usleep(sleep_ms * 1000);
        }
        else
        {
            sleep(1);
        }
    }

    return;
}

std::string COverlayDraw::get_template_text(Osd::OverplayInfo_S stuOverplayInfo, Osd::ShareInfo_S stuShareInfo)
{
    std::string strInfo;
    switch (stuOverplayInfo.stuOverplay.enElementType)
    {
        /* 自定义字符串 */
    case Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE:
        return stuOverplayInfo.stuOverplay.strCustomize;
        /* 时间字符串 */
    case Osd::ElementType_E::ELEMENT_TYPE_TIME:
        /* 拼接字符串 */
        /* 时间 */
        if (stuOverplayInfo.stuOverplay.bEnablePeriod)
        {
            strInfo = stuShareInfo.stuTimeInfo.strTime12;
        }
        else if (!stuOverplayInfo.stuOverplay.bEnablePeriod)
        {
            strInfo = stuShareInfo.stuTimeInfo.strTime;
        }
        /* 星期 */
        if (stuOverplayInfo.stuOverplay.bEnableWeek)
        {
            /* 查找第一个空格位置（分隔日期和时间） */
            size_t nSpacePos = strInfo.find(' ');

            if(nSpacePos == std::string::npos)
            {
                /* 若没有空格，直接拼接 */
                strInfo += " " + stuShareInfo.stuTimeInfo.strWeek;
            }
            else
            {
                /* 分割并插入 */
                std::string strDatePart = strInfo.substr(0, nSpacePos);
                std::string strTimePart = strInfo.substr(nSpacePos);
                
                /* 新格式：日期 + 空格 + Friday + 时间(包含空格和AM/PM) */
                strInfo = strDatePart + " " + stuShareInfo.stuTimeInfo.strWeek + strTimePart;
            }
        }
        /* 时区 */
        if (stuOverplayInfo.stuOverplay.bEnableTimeZone)
        {
            strInfo += " " + stuShareInfo.stuTimeInfo.strZone;
        }
        return strInfo;
        /* 人头字符串 */
    case Osd::ElementType_E::ELEMENT_TYPE_PEOPLE:
        return stuShareInfo.strPeople;
        /* 名称字符串 */
    case Osd::ElementType_E::ELEMENT_TYPE_NAME:
        return stuOverplayInfo.stuInfo.strName;
        /* Mac地址注册信息字符串 */
    case Osd::ElementType_E::ELEMENT_TYPE_MAC:
        return stuShareInfo.strMac;
        /* IP地址字符串 */
    case Osd::ElementType_E::ELEMENT_TYPE_IP:
        return stuShareInfo.strIp;
        /* 预置位字符串 */
    case Osd::ElementType_E::ELEMENT_TYPE_PRESET:
        return stuShareInfo.strPreset;
    default:
        return NULL;
    }
}

void COverlayDraw::get_reference_size(Osd::ReferenceSize_E enReferenceSize, int &nWidth, int &nHeight)
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

int COverlayDraw::calculate_text_size(int nFontSize, int nActualSize, int nReferenceSize)
{
    /* 实际字体大小 = 参考字体大小 * 实际分辨率大小 / 参考分辨率大小 */
    int nActualFontSize = nFontSize * nActualSize / nReferenceSize;
    /* 确保字体大小 2 字节对齐，不是则 +1 向上对齐 */
    return  ALIGN_UP(nActualFontSize, RGN_ALIGN_2);
}

int COverlayDraw::calculate_template_width(int nLen, int nFontSize, int nActualWidth, int nReferenceWidth)
{
    /* 模板宽度 = （字符串长度 * 字体大小） * 实际分辨率宽度 / 参考分辨率宽度 */
    int nTemplateWidth = (nLen * nFontSize) * nActualWidth / nReferenceWidth;

    /* 确保模板宽度 16 字节对齐，不是则 +1 向上对齐 */
    nTemplateWidth = ALIGN_UP(nTemplateWidth, RGN_ALIGN);

    /* 限制长度 */
    if (nActualWidth < nTemplateWidth)
    {
        nTemplateWidth = nActualWidth;
    }

    return nTemplateWidth;
}

int COverlayDraw::calculate_template_height(int nFontSize, int nActualHeight, int nReferenceHeight)
{
    /* 模板高度 = 字体大小 * 实际分辨率高度 / 参考分辨率高度 */
    int nTemplateHeight = nFontSize * nActualHeight / nReferenceHeight;

    /* 确保模板高度 16 字节对齐，不是则 +1 向上对齐 */
    nTemplateHeight = ALIGN_UP(nTemplateHeight, RGN_ALIGN);

    /* 限制高度 */
    if (nActualHeight < nTemplateHeight)
    {
        nTemplateHeight = nActualHeight;
    }

    return nTemplateHeight;
}

void COverlayDraw::get_template_margin(Osd::OverplayInfo_S stuOverplayInfo, int &nActualHorMargin, int &nActualVerMargin, int nActualWidth, int nActualHeight, int nReferenceWidth, int nReferenceHeight)
{
    /* 模板水平边距 = 水平边距 * 实际分辨率宽度 / 参考分辨率宽度 */
    nActualHorMargin = stuOverplayInfo.stuOverplay.nHorMargin * nActualWidth / nReferenceWidth;
    /* 模板垂直边距 = 垂直边距 * 实际分辨率高度 / 参考分辨率高度 */
    nActualVerMargin = stuOverplayInfo.stuOverplay.nVerMargin * nActualHeight / nReferenceHeight;
    /* 确保水平边距 16 字节对齐，不是则 +1 向上对齐 */
    nActualHorMargin = ALIGN_UP(nActualHorMargin, RGN_ALIGN);
    /* 确保垂直边距 16 字节对齐，不是则 +1 向上对齐 */
    nActualVerMargin = ALIGN_UP(nActualVerMargin, RGN_ALIGN);
}

void COverlayDraw::get_start_points(Osd::Align_E enAlign, int &nX, int &nY, int nActualHorMargin, int nActualVerMargin, int nTemplateWidth, int nTemplateHeight, int nActualWidth, int nActualHeight)
{
    switch (enAlign)
    {
    case Osd::Align_E::ALIGN_TOP_LEFT:
        /* 参考点在区域左上角(2字节对齐) */
        nX = ALIGN_UP(nActualHorMargin, RGN_ALIGN_2);
        nY = ALIGN_UP(nActualVerMargin, RGN_ALIGN_2);
        break;
    case Osd::Align_E::ALIGN_BOTTOM_LEFT:
        /* 参考点在区域左下角(2字节对齐) */
        nX = ALIGN_UP(nActualHorMargin, RGN_ALIGN_2);
        nY = ALIGN_BACK(nActualHeight - nTemplateHeight - nActualVerMargin, RGN_ALIGN_2);
        break;
    case Osd::Align_E::ALIGN_TOP_RIGHT:
        /* 参考点在区域右上角(2字节对齐) */
        nX = ALIGN_BACK(nActualWidth - nTemplateWidth - nActualHorMargin, RGN_ALIGN_2);
        nY = ALIGN_UP(nActualVerMargin, RGN_ALIGN_2);
        break;
    case Osd::Align_E::ALIGN_BOTTOM_RIGHT:
        /* 参考点在区域右下角(2字节对齐) */
        nX = ALIGN_BACK(nActualWidth - nTemplateWidth - nActualHorMargin, RGN_ALIGN_2);
        nY = ALIGN_BACK(nActualHeight - nTemplateHeight - nActualVerMargin, RGN_ALIGN_2);
        break;
    case Osd::Align_E::ALIGN_CENTER:
        /* 参考点在区域左上角(2字节对齐) */
        nX = ALIGN_UP((nActualWidth - nTemplateWidth) / 2 + nActualHorMargin, RGN_ALIGN_2);
        nY = ALIGN_UP((nActualHeight - nTemplateHeight) / 2 + nActualVerMargin, RGN_ALIGN_2);
        break;
    default:
        break;
    }
    
    /*防止起始点+尺寸超过实际分辨率，导致osd显示不全问题*/
    if(nX + nTemplateWidth > nActualWidth)
    {
        /* 2字节对齐 */
        nX = ALIGN_BACK(nActualWidth - nTemplateWidth, RGN_ALIGN_2); 
    }
    if(nY + nTemplateHeight > nActualHeight)
    {
        /* 2字节对齐 */
        nY = ALIGN_BACK(nActualHeight - nTemplateHeight, RGN_ALIGN_2);
    }

    return;
}

int COverlayDraw::algha_percentage_to_rgb(uint32_t nAlpha)
{
    /* 透明度由百分比变为0~255的范围 */
    return (100 - nAlpha) * 255 / 100;
}

#if 1
/* 用freetype + sdl + sdl_ttf库来画 */
void COverlayDraw::draw_text(RkRgn_S *pHandle, Osd::OverplayInfo_S stuOverplayInfo, const char *pText)
{
    
    if (NULL == pHandle || NULL == pText)
    {
        dlog_error("指针为空，不进行绘制图片");
        return;
    }
    if (0 == strlen(pText))
    {
        dlog_debug("没有文本需要绘制，清空对应区域上的图片");
        pHandle->rockitRgn_clearPicture(pHandle);
        return;
    }

     static unsigned int OldStartX[MAX_OSD_OVERLAY_NUM * VPSS_CHN_MAX]={0}, OldStartY[MAX_OSD_OVERLAY_NUM * VPSS_CHN_MAX]={0};

    /* 定义当前时间 */
    // time_t stuNowTime = time(NULL);

    /* 定义字体类型 */
    // int nFixedSizes = 0;

    /* 定义像素格式 */
    SDL_PixelFormat stuFormat;
    memset(&stuFormat, 0, sizeof(SDL_PixelFormat));

    /* 定义字体颜色 */
    SDL_Color stuFontColor;
    memset(&stuFontColor, 0, sizeof(SDL_Color));

    /* 定义背景颜色 */
    SDL_Color stuBackColor;
    memset(&stuBackColor, 0, sizeof(SDL_Color));

    /* 设置像素格式 */
    switch (pHandle->enFormat)
    { 
    case RK_FMT_ARGB4444:
        stuFormat.format = SDL_PIXELFORMAT_ARGB4444; /* 颜色格式 */
        stuFormat.BitsPerPixel = 16;                 /* 16位色 */
        stuFormat.BytesPerPixel = 2;                 /* 2字节/像素 */
        /* 设置颜色掩码 */
        stuFormat.Amask = 0xF000; /* Alpha掩码 */
        stuFormat.Ashift = 12;    /* Alpha偏移 */
        stuFormat.Rmask = 0x0F00; /* 红色掩码 */
        stuFormat.Rshift = 8;     /* 红色偏移 */
        stuFormat.Gmask = 0x00F0; /* 绿色掩码 */
        stuFormat.Gshift = 4;     /* 绿色偏移 */
        stuFormat.Bmask = 0x000F; /* 蓝色掩码 */
        stuFormat.Bshift = 0;     /* 蓝色偏移 */
        break;
    case RK_FMT_BGRA8888:
        stuFormat.format = SDL_PIXELFORMAT_BGRA8888; /* 颜色格式 */
        stuFormat.BitsPerPixel = 32; /* 32位色 */
        stuFormat.BytesPerPixel = 4; /* 4字节/像素 */
        /* 设置颜色掩码 */
        stuFormat.Bmask = 0x00FF0000; /* 蓝色掩码 */
        stuFormat.Bshift = 16; /* 蓝色偏移 */
        stuFormat.Gmask = 0x0000FF00; /* 绿色掩码 */
        stuFormat.Gshift = 8; /* 绿色偏移 */
        stuFormat.Rmask = 0x000000FF; /* 红色掩码 */
        stuFormat.Rshift = 0; /* 红色偏移 */
        stuFormat.Amask = 0xFF000000; /* Alpha掩码 */
        stuFormat.Ashift = 24; /* Alpha偏移 */
        break;
    default:
        break;
    }
    
    /* 设置字体颜色分量 */
    stuFontColor.r = (pHandle->unFgColor >> 16) & 0xFF;
    stuFontColor.g = (pHandle->unFgColor >> 8) & 0xFF;
    stuFontColor.b = pHandle->unFgColor & 0xFF;
    stuFontColor.a = pHandle->unFgAlpha;
    /* 设置背景颜色分量 */
    stuBackColor.r = (pHandle->unBgColor >> 16) & 0xFF;
    stuBackColor.g = (pHandle->unBgColor >> 8) & 0xFF;
    stuBackColor.b = pHandle->unBgColor & 0xFF;
    stuBackColor.a = pHandle->unBgAlpha;

    /* 判断字体是否支持中文 */
    // check_chinese_support(ITC_FONT_FILE);

    /* 创建文字表面(绘制操作) */
    SDL_Surface *pSurface = create_text_surface(
        pText,               /* OSD文本 */
        pHandle->unWidth,    /* 宽度 */
        pHandle->unHeight,   /* 高度 */
        pHandle->unFontSize, /* 字体大小 */
        stuFontColor,        /* 文字颜色 */
        stuBackColor,        /* 背景颜色 */
        &stuFormat,          /* 颜色格式 */
        ITC_FONT_FILE        /* 字体路径 */
    );

    if (!pSurface)
    {
        dlog_error("OSD创建文字表面失败");
        SDL_Quit();
        return;
    }
    //dlog_debug("成功创建OSD文字表面: %d×%d-%s", pSurface->w, pSurface->h,pText);

    /* 修改rgn尺寸 */
    if (pSurface->w != (int)pHandle->unWidth || pSurface->h != (int)pHandle->unHeight \
         ||  OldStartX[pHandle->unHandle] != pHandle->unStartX || OldStartY[pHandle->unHandle] != pHandle->unStartY )
    {
        OldStartX[pHandle->unHandle] = pHandle->unStartX;
        OldStartY[pHandle->unHandle] = pHandle->unStartY;
        /* 更新rgn */
        pHandle->RkRgn_update(pHandle, set_rgn(stuOverplayInfo, pHandle->unChnId, pHandle->unHandle, pSurface->w, pSurface->h));
        /* 改变rgn尺寸 */
        pHandle->rockitrgn_change_rect(pHandle, pHandle->unWidth, pHandle->unHeight);
        /* 改变rgn起始位置 */
        pHandle->rockitRgn_changePos(pHandle, pHandle->unStartX, pHandle->unStartY);
    }

#if 0
    /* 保存为BMP文件(用于调试) */
    if (0 != save_surface_to_bmp(pSurface, "/opt/cam/osd.bmp"))
    {
        dlog_error("保存失败");
        SDL_FreeSurface(pSurface);
        SDL_Quit();
        return;
    }
#endif
    
    /* 贴图(pitch:表示每一行像素数据占用的字节数, pitch * h:整个图像数据大小) */
    pHandle->rockitRgn_overlay_loadPic(pHandle, pSurface->pixels, pSurface->pitch * pSurface->h);

    /* 释放指针 */
    if (NULL != pSurface)
    {
        SDL_FreeSurface(pSurface);
        pSurface = NULL;
    }

    // dlog_debug("加载图像成功");
    return;
}
#else
/* 用opencv库来画 */
void COverlayDraw::draw_text(RkRgn_S *pHandle, char *pText, int nLength)
{
    OS_mutexLock(&m_stuMutex);

    int nRet = 0;

    /* 创建绘图参数 */
    CImageProcessor::DrawParam_S stDrawParam;
    /* 设置输出格式 */
    if (OT_PIXEL_FORMAT_ARGB_4444 == pHandle->enFormat)
    {
        stDrawParam.enOutType = CImageProcessor::DataFormatType_E::ARGB4444;
    }
    else
    {
        stDrawParam.enOutType = CImageProcessor::DataFormatType_E::RGB888;
    }

    stDrawParam.nOutWidth = pHandle->unWidth;
    stDrawParam.nOutHeight = pHandle->unHeight;
    stDrawParam.nA = pHandle->unBgAlpha; /* 背景透明度（0=完全透明, 255=不透明） */
    stDrawParam.nR = 0;                  /* 背景色红色分量 （0-255） */
    stDrawParam.nG = 0;                  /* 背景色绿色分量 （0-255） */
    stDrawParam.nB = 0;                  /* 背景色蓝色分量 （0-255） */

    /* 要绘制的标签列表 */
    std::list<CImageProcessor::LabelInfo_S> listLabelInfo;
    CImageProcessor::LabelInfo_S label;
    label.strLabel = pText;                            /* 标签文本 */
    label.nX = 0;                                      /* X坐标 */
    label.nY = 0;                                      /* Y坐标 */
    label.nFontFace = cv::FONT_HERSHEY_SIMPLEX;        /* 字体类型 */
    label.dFontScale = double(pHandle->unHeight) / 32; /* 字体缩放 */
    label.nThickness = 2;                              /* 字体厚度 */
    label.nLabelA = pHandle->unFgAlpha;                /* 字体透明度（0=完全透明, 255=不透明）*/
    label.nLabelR = 0;                                 /* 红色分量 */
    label.nLabelG = 0;                                 /* 绿色分量 */
    label.nLabelB = 0;                                 /* 蓝色分量 */
    listLabelInfo.push_back(label);

    /* 绘制操作 */
    char *pchOutData = NULL;
    int nOutDataSize = 0;
    if (m_imageProcessor.draw(stDrawParam, listLabelInfo, &pchOutData, nOutDataSize))
    {
        dlog_error("生成图像失败");
        delete[] pchOutData;
        pchOutData = NULL;
        OS_mutexUnlock(&m_stuMutex);
        return;
    }

    pHandle->mppRgn_overlay_loadPic(pHandle, pchOutData, nOutDataSize);

    if (NULL != pchOutData)
    {
        delete[] pchOutData;
        pchOutData = NULL;
    }

    dlog_debug("加载图像成功");
    OS_mutexUnlock(&m_stuMutex);
    return;
}
#endif

void COverlayDraw::draw_rect(RkRgn_S *pHandle, Osd::OverplayInfo_S stuOverplayInfo, const std::vector<Common::RectInfo_S> &vRectInfo)
{
    if (!pHandle || vRectInfo.empty()) return;

    RGN_CANVAS_INFO_S stCanvasInfo;
    if (pHandle->rockitRgn_getCanvasInfo(pHandle, &stCanvasInfo) != 0) return;

    uint32_t uWidth  = stCanvasInfo.u32VirWidth;
    uint32_t uHeight = stCanvasInfo.u32VirHeight;
    unsigned char *buffer = (unsigned char *)stCanvasInfo.u64VirAddr;
    uint32_t stride = uWidth >> 2; 

    // 清空画布
    memset(buffer, 0, stride * uHeight);

    // 线宽自适应
    int line_p = (int)(uWidth * 4 / 1920); 
    if (line_p < 2) line_p = 2; 

    unsigned char color_byte = 0xAA; 
    bool isRed = (stuOverplayInfo.stuOverplay.strFontColor.find("ff0000") != std::string::npos);
    if (isRed) color_byte = 0xFF;

    unsigned char mask_L, mask_R;
    if (!isRed) {
        // 针对 0xAA (索引0b10) 的边缘掩码 
        unsigned char m_l[] = {0x00, 0x80, 0xA0, 0xA8, 0xAA}; // 1-4px 绿
        unsigned char m_r[] = {0x00, 0x02, 0x0A, 0x2A, 0xAA};
        mask_L = m_l[line_p > 4 ? 4 : line_p];
        mask_R = m_r[line_p > 4 ? 4 : line_p];
    } else {
        // 针对 0xFF (索引0b11) 的边缘掩码 
        unsigned char m_l[] = {0x00, 0xC0, 0xF0, 0xFC, 0xFF}; // 1-4px 红
        unsigned char m_r[] = {0x00, 0x03, 0x0F, 0x3F, 0xFF};
        mask_L = m_l[line_p > 4 ? 4 : line_p];
        mask_R = m_r[line_p > 4 ? 4 : line_p];
    }

    for (const auto &rect : vRectInfo) {
        int x1 = rect.nX1; 
        int y1 = rect.nY1; 
        int x2 = rect.nX2;
        int y2 = rect.nY2;

        if (x2 <= x1 || y2 <= y1) continue;

        // 绘制横线 (顶边和底边)
        int line_width_bytes = (x2 - x1) >> 2; 
        for (int i = 0; i < line_p; i++) {
            if (y1 + i < (int)uHeight)
                memset(buffer + (y1 + i) * stride + (x1 >> 2), color_byte, line_width_bytes);
            if (y2 - 1 - i >= 0)
                memset(buffer + (y2 - 1 - i) * stride + (x1 >> 2), color_byte, line_width_bytes);
        }

        // 绘制竖线 (左边和右边)
        for (int j = y1; j < y2; j++) {
            unsigned char *row_ptr = buffer + (j * stride);
            
            if (line_p >= 4) {
                // 高分辨率：直接填充完整字节
                int lb = line_p >> 2;
                memset(row_ptr + (x1 >> 2), color_byte, lb);
                memset(row_ptr + (x2 >> 2) - lb, color_byte, lb);
            } else {
                // 低分辨率：使用位或 |= 叠加，防止擦除拐角横线
                // 左竖线字节位置
                row_ptr[x1 >> 2] |= mask_L; 
                // 右竖线字节位置 (x2是16对齐的，x2 >> 2 指向框外第一个字节，需-1)
                row_ptr[(x2 >> 2) - 1] |= mask_R; 
            }
        }
    }
    pHandle->rockitRgn_updateCanvas(pHandle);
}


void COverlayDraw::check_chinese_support(const char *pFontPath)
{
    TTF_Font *pFont = TTF_OpenFont(pFontPath, 0);
    if (!pFont)
    {
        dlog_error("字体加载失败: %s", TTF_GetError());
        TTF_Quit();
        return;
    }

    /* 检查中文字符支持 */
    if (TTF_GlyphIsProvided(pFont, 0x4E2D)) /* "中" 字 */
    {
        dlog_info("字体支持中文");
    }
    else
    {
        dlog_info("字体不支持中文");
    }

    TTF_CloseFont(pFont);

    return;
}

int COverlayDraw::get_font_size(int nFontSize, const char *pFontPath)
{
    /* 打开字体文件，使用0大小来获取默认face（主要是为了获取face数量）*/
    TTF_Font *pFont = TTF_OpenFont(pFontPath, 0);
    if (!pFont)
    {
        dlog_error("字体加载失败: %s", TTF_GetError());
        TTF_Quit();
        return -1;
    }

    /* 获取字体文件中包含的face数量 */
    long lnFaces = TTF_FontFaces(pFont);
    // dlog_info("Total faces in font: %ld", lnFaces);

    /* 关闭字体文件 */
    TTF_CloseFont(pFont);

    int nBestSize = nFontSize;
    int minDiff = nFontSize;

    /* 位图字体 - 查找最接近的可用大小 */
    if (0 < lnFaces)
    {
        /* 常见字体大小列表（可根据需要扩展） */
        const int achCommonSizes[] = {
            16, 20, 24, 28, 32,                                          /* 小标题/正文 (16-32) */
            36, 40, 44, 48,                                              /* 中等标题 (36-48) */
            56, 60, 64, 72, 76,                                          /* 大标题 (56-72) */
            80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128}; /* 超大字/海报尺寸 (80-128) */

        const int nSizes = sizeof(achCommonSizes) / sizeof(achCommonSizes[0]);

        // dlog_info("检测到位图字体，查找最接近 %d 的可用大小", nFontSize);

        for (int i = 0; i < nSizes; i++)
        {
            const int nFaceSize = achCommonSizes[i];
            TTF_Font *pTestFont = TTF_OpenFont(pFontPath, nFaceSize);

            if (pTestFont)
            {
                /* 计算当前大小与请求大小的差异 */
                const int nDiff = abs(nFaceSize - nFontSize);

                /* 如果找到更接近的大小 */
                if (nDiff < minDiff)
                {
                    minDiff = nDiff;
                    nBestSize = nFaceSize;
                }

                // dlog_info("可用大小: %d (差异: %d)", nFaceSize, nDiff);
                TTF_CloseFont(pTestFont);
            }
        }
        // dlog_info("选择最接近的大小: %d", nBestSize);
    }
    else
    {
        dlog_info("此字体为矢量字体，支持任意大小");
    }

    return nBestSize;
}

SDL_Surface *COverlayDraw::create_text_surface(const char *pText, int nWidth, int nHeight,
                                           int nFontSize, SDL_Color stuFontColor,
                                           SDL_Color stuBackColor, SDL_PixelFormat *pPixelFormat,
                                           const char *pFontPath)
{  
    (void)nWidth;
    (void)nHeight;

    /* 打开字体文件 */
    TTF_Font *pFont = TTF_OpenFont(pFontPath, nFontSize);
    if (!pFont)
    {
        dlog_error("字体加载失败: %s", TTF_GetError());
        TTF_Quit();
        return NULL;
    }

    /* 创建文字表面 */
    SDL_Surface *pTextSurface = NULL;

    /* 当传入字体透明度为全透明时,用下面这两个函数的文字透明度不生效,故设置透明度为1 */
    if (0x00 == stuFontColor.a)
    {
        stuFontColor.r = 0x00;
        stuFontColor.g = 0x00;
        stuFontColor.b = 0x00;
        stuFontColor.a = 0x01;
    }

#if CAP_EXHIBITION_OSD_PANEL
    if (is_multi_line_text(pText))
    {
        /* 拆分后的多行文本列表。 */
        const std::vector<std::string> vecLines = split_text_lines(pText);
        /* 多行文本的额外行间距。 */
        const int nLineSpacing = get_multi_line_spacing(nFontSize);
        /* 当前字体对应的单行基础高度。 */
        const int nFontHeight = std::max(1, TTF_FontHeight(pFont));
        /* 每一行文本单独渲染后的 surface 列表。 */
        std::vector<SDL_Surface *> vecLineSurfaces;
        vecLineSurfaces.reserve(vecLines.size());

        /* 所有行中的最大宽度。 */
        int nSurfaceWidth = 0;
        /* 所有行叠加后的总高度。 */
        int nSurfaceHeight = 0;
        for (size_t i = 0; i < vecLines.size(); ++i)
        {
            /* 当前行单独渲染后的 surface。 */
            SDL_Surface *pLineSurface = nullptr;
            if (!vecLines[i].empty())
            {
                pLineSurface = TTF_RenderUTF8_Blended(pFont, vecLines[i].c_str(), stuFontColor);
                if (!pLineSurface)
                {
                    for (auto *pSurface : vecLineSurfaces)
                    {
                        if (pSurface)
                        {
                            SDL_FreeSurface(pSurface);
                        }
                    }
                    dlog_error("多行文字渲染失败: %s", TTF_GetError());
                    TTF_CloseFont(pFont);
                    return NULL;
                }
                nSurfaceWidth = std::max(nSurfaceWidth, pLineSurface->w);
                nSurfaceHeight += pLineSurface->h;
            }
            else
            {
                nSurfaceHeight += nFontHeight;
            }

            if (i + 1 < vecLines.size())
            {
                nSurfaceHeight += nLineSpacing;
            }
            vecLineSurfaces.emplace_back(pLineSurface);
        }

        /* 用于承载多行文本的合成 surface。 */
        SDL_Surface *pComposeSurface = SDL_CreateRGBSurfaceWithFormat(0,
                                                                      ALIGN_UP(std::max(1, nSurfaceWidth), RGN_ALIGN),
                                                                      ALIGN_UP(std::max(1, nSurfaceHeight), RGN_ALIGN),
                                                                      32,
                                                                      SDL_PIXELFORMAT_RGBA32);
        if (!pComposeSurface)
        {
            for (auto *pSurface : vecLineSurfaces)
            {
                if (pSurface)
                {
                    SDL_FreeSurface(pSurface);
                }
            }
            dlog_error("创建多行文字Surface失败: %s", SDL_GetError());
            TTF_CloseFont(pFont);
            return NULL;
        }

        SDL_FillRect(pComposeSurface,
                     NULL,
                     SDL_MapRGBA(pComposeSurface->format, stuBackColor.r, stuBackColor.g, stuBackColor.b, stuBackColor.a));

        /* 当前绘制到合成 surface 的纵向偏移。 */
        int nCurrentY = 0;
        for (size_t i = 0; i < vecLineSurfaces.size(); ++i)
        {
            /* 当前准备贴到合成 surface 的行 surface。 */
            SDL_Surface *pLineSurface = vecLineSurfaces[i];
            if (pLineSurface)
            {
                /* 当前行在合成 surface 上的目标区域。 */
                SDL_Rect stDstRect = {0, nCurrentY, pLineSurface->w, pLineSurface->h};
                SDL_BlitSurface(pLineSurface, NULL, pComposeSurface, &stDstRect);
                nCurrentY += pLineSurface->h;
            }
            else
            {
                nCurrentY += nFontHeight;
            }

            if (i + 1 < vecLineSurfaces.size())
            {
                nCurrentY += nLineSpacing;
            }
        }

        for (auto *pSurface : vecLineSurfaces)
        {
            if (pSurface)
            {
                SDL_FreeSurface(pSurface);
            }
        }
        pTextSurface = pComposeSurface;
    }
    /* 检查是否需要透明背景 */
    else if (0 == stuBackColor.a)
#else
    if (0 == stuBackColor.a)
#endif
    {
        /* 透明背景 */
        pTextSurface = TTF_RenderUTF8_Blended(pFont, pText, stuFontColor);
    }
    else
    {   
        /* 实心背景 */
        pTextSurface = TTF_RenderUTF8_Shaded(pFont, pText, stuFontColor, stuBackColor);
    }

    if (!pTextSurface)
    {
        dlog_error("文字渲染失败: %s", TTF_GetError());
        TTF_CloseFont(pFont);
        TTF_Quit();
        return NULL;
    }

    /* 创建目标尺寸的表面 */
    SDL_Surface *pTargetSurface = NULL;

    // /* 图片宽度16字节对齐, 否则可能显示异常 */
    // if (pTextSurface->w % 16 < RGN_ALIGN)
    // {
    //     pTextSurface->w = ALIGN_BACK(pTextSurface->w, RGN_ALIGN);
    // }
    // else
    // {
    //     pTextSurface->w = ALIGN_UP(pTextSurface->w, RGN_ALIGN);
        
    // }
    /* 图片宽度,高度16字节对齐, 否则可能显示异常 */
    if (pTextSurface->w % RGN_ALIGN != 0 || pTextSurface->h % RGN_ALIGN != 0) {
        int aligned_w = ALIGN_UP(pTextSurface->w, RGN_ALIGN);
        int aligned_h = ALIGN_UP(pTextSurface->h, RGN_ALIGN);
        /* 图片宽高对齐后的表面 */
        SDL_Surface* pAlignedSurface = SDL_CreateRGBSurfaceWithFormat(
            0, // 标志位
            aligned_w, // 新的、对齐后的宽度
            aligned_h, // 新的、对齐后的高度
            pTextSurface->format->BitsPerPixel, // 位深
            pTextSurface->format->format // 像素格式
        );
        if (!pAlignedSurface) {
            dlog_error("创建对齐Surface失败: %s", SDL_GetError());
            SDL_FreeSurface(pTextSurface);
            TTF_CloseFont(pFont);
            return nullptr;
        }
        /* 填充背景色 */
        if (SDL_FillRect(
            pAlignedSurface,
            nullptr,
            SDL_MapRGBA(pAlignedSurface->format, stuBackColor.r, stuBackColor.g, stuBackColor.b, stuBackColor.a))) {
                dlog_error("填充对齐Surface失败: %s", SDL_GetError());
                SDL_FreeSurface(pTextSurface);
                SDL_FreeSurface(pAlignedSurface);
                TTF_CloseFont(pFont);
                return nullptr;
            }
        SDL_Rect src_rect = {0, 0, pTextSurface->w, pTextSurface->h};
        SDL_Rect dst_rect = {0, 0, pTextSurface->w, pTextSurface->h};
        /* 将未对齐画面绘制到对齐后表面上，填充部分有背景色填充 */
        SDL_BlitSurface(pTextSurface, &src_rect, pAlignedSurface, &dst_rect);
        pTargetSurface = SDL_ConvertSurface(pAlignedSurface, pPixelFormat, 0);
        SDL_FreeSurface(pAlignedSurface);
    } else {
        pTargetSurface = SDL_ConvertSurface(pTextSurface, pPixelFormat, 0);
    }

    /* 释放临时资源 */
    SDL_FreeSurface(pTextSurface);
    TTF_CloseFont(pFont);

    return pTargetSurface;
}

void COverlayDraw::draw_rect_on_surface(SDL_Surface *pSurface, const Common::RectInfo_S &stRect, Uint32 u32Color, int nThickness)
{
    if (!pSurface || nThickness <= 0)
        return;

    /* 为防止越界，限制矩形坐标在Surface范围内 */
    int nX1 = std::max(0, stRect.nX1);
    int nY1 = std::max(0, stRect.nY1);
    int nX2 = std::min(pSurface->w - 1, stRect.nX2);
    int nY2 = std::min(pSurface->h - 1, stRect.nY2);
    
    /* 如果厚度导致负值或坐标无效，则提前返回 */
    if (nX1 >= nX2 || nY1 >= nY2) return;

    int nW = nX2 - nX1 + 1;
    int nH = nY2 - nY1 + 1;

    /* 确保厚度不会超过矩形本身的大小 */
    int safeThickness = std::min(nThickness, std::min(nW / 2, nH / 2));
    if (safeThickness <= 0) safeThickness = 1;

    SDL_Rect rectToFill;

    /* 上边线 */
    rectToFill = { nX1, nY1, nW, safeThickness };
    SDL_FillRect(pSurface, &rectToFill, u32Color);

    /* 下边线 */
    rectToFill = { nX1, nY2 - safeThickness + 1, nW, safeThickness };
    SDL_FillRect(pSurface, &rectToFill, u32Color);

    /* 左边线 */
    rectToFill = { nX1, nY1, safeThickness, nH };
    SDL_FillRect(pSurface, &rectToFill, u32Color);

    /* 右边线 */
    rectToFill = { nX2 - safeThickness + 1, nY1, safeThickness, nH };
    SDL_FillRect(pSurface, &rectToFill, u32Color);
}

void COverlayDraw::generate_detection_overlay(SDL_Surface* pSurface, const std::vector<Common::RectInfo_S> &vRectInfo, SDL_Color stColor, int nThickness)
{
    if (!pSurface) return;

    /* 高效清空画布为全透明 */
    Uint32 u32Transparent = SDL_MapRGBA(pSurface->format, 0, 0, 0, 0);
    SDL_FillRect(pSurface, NULL, u32Transparent);

    /* 预计算矩形颜色 */
    Uint32 u32RectColor = SDL_MapRGBA(pSurface->format, stColor.r, stColor.g, stColor.b, stColor.a);

    /* 遍历所有检测框，高效画出框线 */
    for (const auto &rectInfo : vRectInfo)
    {
        if (rectInfo.nX1 < 0 || rectInfo.nY1 < 0 || rectInfo.nX2 < 0 || rectInfo.nY2 < 0 || rectInfo.nX1 >= rectInfo.nX2 || rectInfo.nY1 >= rectInfo.nY2)
        {
            continue; /* 跳过无效的框 */
        }
        draw_rect_on_surface(pSurface, rectInfo, u32RectColor, nThickness);
    }
}

int COverlayDraw::save_surface_to_bmp(SDL_Surface *pSurface, const char *pFileName)
{
    if (!pSurface || !pFileName)
        return -1;

    if (0 != SDL_SaveBMP(pSurface, pFileName))
    {
        dlog_error("BMP保存失败: %s", SDL_GetError());
        return -1;
    }

    return 0;
}
