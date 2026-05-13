/**
 * @FilePath     : overplay_draw.cpp
 * @Author       : huangjunda
 * @Date         : 2025-05-27 14:12:40
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-15 16:34:19
 * @Description  : OSD覆盖
 */

#include "overplay_draw.h"

#include "osd_manage.h"
#include "share_define.h"
#include "video_define.h"
#include "stream_video.h"
#include "event_configure.h"
#include "mpp_rgn.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>

namespace
{
#if CAP_EXHIBITION_OSD_PANEL
/* 展会面板复用的空闲 overlay 索引 */
constexpr int EXHIBITION_PANEL_OVERPLAY_INDEX = 0;
/* 展会面板刷新周期，单位毫秒 */
constexpr int EXHIBITION_PANEL_RENDER_INTERVAL_MS = 250;
/* 展会面板缓存有效期，单位毫秒 */
constexpr int EXHIBITION_PANEL_VALID_TIME_MS = 1500;
/* 展会面板左上角边距 */
constexpr int EXHIBITION_PANEL_MARGIN_PX = 24;
/* 展会面板参考宽度 */
constexpr int EXHIBITION_PANEL_BASE_WIDTH = 520;
/* 展会面板参考高度 */
constexpr int EXHIBITION_PANEL_BASE_HEIGHT = 620;
#endif
/* 展会面板参考字体大小 */
constexpr int EXHIBITION_PANEL_FONT_SIZE = 24;
/* 多行文本额外行间距 */
constexpr int MULTI_LINE_SPACING = 8;

/**
 * @brief   : 判断句柄是否属于展会面板专用 RGN
 * @param    {size_t} nHandle：RGN 句柄
 * @return   {bool} true：属于展会面板 false：不属于
 */
inline bool is_exhibition_panel_handle(size_t nHandle)
{
#if CAP_EXHIBITION_OSD_PANEL
    return nHandle < static_cast<size_t>(VENC_CHN_JPEG);
#else
    (void)nHandle;
    return false;
#endif
}

#if CAP_EXHIBITION_OSD_PANEL
/**
 * @brief   : 获取稳态时钟的毫秒时间戳
 * @return   {uint64_t} 当前毫秒时间戳
 * @note    : 使用 steady_clock 避免系统时间跳变影响面板超时判断
 */
inline uint64_t get_steady_time_ms()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}
#endif

/**
 * @brief   : 根据当前字体大小计算多行文本行间距
 * @param    {int} nFontSize：实际字体大小
 * @return   {int} 自适应后的行间距
 * @note    : 以 1080P 下 24 号字对应 8px 行间距为基准，随主子码流字体同比缩放
 */
int get_multi_line_spacing(int nFontSize)
{
    if (nFontSize <= 0)
    {
        return 0;
    }

    /* 以 1080P 基准字号和行间距为模板，按当前字体大小同比缩放 */
    int nLineSpacing = MULTI_LINE_SPACING * nFontSize / EXHIBITION_PANEL_FONT_SIZE;
    return std::max(2, nLineSpacing);
}

/**
 * @brief   : 按换行符拆分多行文本
 * @param    {const std::string &} strText：原始文本
 * @return   {std::vector<std::string>} 拆分后的文本行列表
 */
std::vector<std::string> split_text_lines(const std::string &strText)
{
    /* 按行保存的文本列表 */
    std::vector<std::string> vecLines;
    /* 用于逐行读取的字符串流 */
    std::stringstream ss(strText);
    /* 当前读取到的单行文本 */
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
 * @brief   : 前置声明单行文本尺寸测量函数
 * @param    {const char *} pText：待测文本
 * @param    {int} nFontSize：字体大小
 * @param    {int &} nTextWidth：输出文本宽度
 * @param    {int &} nTextHeight：输出文本高度
 * @return   {bool} true：测量成功 false：测量失败
 */
bool get_text_render_size(const char *pText, int nFontSize, int &nTextWidth, int &nTextHeight);

/**
 * @brief   : 计算多行文本的整体渲染尺寸
 * @param    {const char *} pText：待测文本
 * @param    {int} nFontSize：字体大小
 * @param    {int &} nTextWidth：输出文本宽度
 * @param    {int &} nTextHeight：输出文本高度
 * @return   {bool} true：测量成功 false：测量失败
 */
bool get_text_block_render_size(const char *pText, int nFontSize, int &nTextWidth, int &nTextHeight)
{
    nTextWidth = 0;
    nTextHeight = 0;
    if (!pText || !strlen(pText) || nFontSize <= 0)
    {
        return false;
    }

    /* 拆分后的文本行列表 */
    auto vecLines = split_text_lines(pText);
    if (vecLines.empty())
    {
        return false;
    }

    /* 是否至少成功测量过一行非空文本 */
    bool bHasMeasuredLine = false;
    /* 当前字号对应的自适应行间距 */
    const int nLineSpacing = get_multi_line_spacing(nFontSize);
    for (size_t i = 0; i < vecLines.size(); ++i)
    {
        /* 当前行文本宽度 */
        int nLineWidth = 0;
        /* 当前行文本高度 */
        int nLineHeight = nFontSize;
        if (!vecLines[i].empty() &&
            get_text_render_size(vecLines[i].c_str(), nFontSize, nLineWidth, nLineHeight))
        {
            bHasMeasuredLine = true;
        }

        nTextWidth = std::max(nTextWidth, nLineWidth);
        nTextHeight += std::max(nLineHeight, nFontSize);
        if (i + 1 < vecLines.size())
        {
            nTextHeight += nLineSpacing;
        }
    }

    return bHasMeasuredLine || !vecLines.empty();
}

/**
 * @brief   : 将多行文本逐行绘制到 SDL Surface
 * @param    {SDL_Surface *} pSurface：目标表面
 * @param    {const CSDLUtils::TextRenderConfig_S &} stTextConfig：文本绘制配置
 * @param    {const char *} pFontPath：字体文件路径
 * @return   {int} 0：成功 非0：失败
 */
int render_text_block(SDL_Surface *pSurface,
                      const CSDLUtils::TextRenderConfig_S &stTextConfig,
                      const char *pFontPath)
{
    /* 拆分后的文本行列表 */
    auto vecLines = split_text_lines(stTextConfig.strText);
    /* 当前绘制行的纵向偏移 */
    int nOffsetY = stTextConfig.nOffsetY;
    /* 当前字号对应的自适应行间距 */
    const int nLineSpacing = get_multi_line_spacing(stTextConfig.nFontSize);

    for (size_t i = 0; i < vecLines.size(); ++i)
    {
        /* 当前行文本高度 */
        int nLineHeight = stTextConfig.nFontSize;
        if (!vecLines[i].empty())
        {
            /* 当前行文本宽度，仅用于辅助计算实际行高 */
            int nUnusedWidth = 0;
            CSDLUtils::instance()->measureText(vecLines[i], stTextConfig.nFontSize, pFontPath, nUnusedWidth, nLineHeight);

            /* 当前行独立的绘制配置 */
            CSDLUtils::TextRenderConfig_S stLineConfig = stTextConfig;
            stLineConfig.strText = vecLines[i];
            stLineConfig.nOffsetY = nOffsetY;
            stLineConfig.stBackColor.a = 0;

            if (CSDLUtils::instance()->renderText(pSurface, stLineConfig, pFontPath) != 0)
            {
                return -1;
            }
        }

        nOffsetY += std::max(nLineHeight, stTextConfig.nFontSize);
        if (i + 1 < vecLines.size())
        {
            nOffsetY += nLineSpacing;
        }
    }

    return 0;
}

/**
 * @brief   : 构造展会面板专用 overlay 配置
 * @return   {Osd::OverplayInfo_S} 展会面板 overlay 配置
 */
#if CAP_EXHIBITION_OSD_PANEL
Osd::OverplayInfo_S build_exhibition_panel_info()
{
    /* 展会面板专用 overlay 配置 */
    Osd::OverplayInfo_S stOverplayInfo;
    stOverplayInfo.clear();
    stOverplayInfo.stuInfo.nID = EXHIBITION_PANEL_OVERPLAY_INDEX + 1;
    stOverplayInfo.stuInfo.bEnable = false;
    stOverplayInfo.stuInfo.strName = "ExhibitionPanel";
    stOverplayInfo.stuInfo.enRefSize = Osd::REFERENCE_SIZE_1080P;

    stOverplayInfo.stuOverplay.bEnableFlicker = false;
    stOverplayInfo.stuOverplay.enAlign = Osd::ALIGN_TOP_LEFT;
    stOverplayInfo.stuOverplay.nHorMargin = EXHIBITION_PANEL_MARGIN_PX;
    stOverplayInfo.stuOverplay.nVerMargin = EXHIBITION_PANEL_MARGIN_PX;
    stOverplayInfo.stuOverplay.nWidth = EXHIBITION_PANEL_BASE_WIDTH;
    stOverplayInfo.stuOverplay.nHeight = EXHIBITION_PANEL_BASE_HEIGHT;
    stOverplayInfo.stuOverplay.nFontSize = EXHIBITION_PANEL_FONT_SIZE;
    stOverplayInfo.stuOverplay.strFontColor = "0xFFFFFF";
    stOverplayInfo.stuOverplay.strBackColor = "0x162028";
    stOverplayInfo.stuOverplay.nFontAlpha = 0;
    stOverplayInfo.stuOverplay.nBackAlpha = 55;
    stOverplayInfo.stuOverplay.enElementType = Osd::ELEMENT_TYPE_CUSTOMIZE;
    stOverplayInfo.stuOverplay.strCustomize.clear();
    return stOverplayInfo;
}
#endif

/* 限制坐标范围，避免出现无符号坐标下溢 */
inline int clamp_value(int nValue, int nMin, int nMax)
{
    if (nValue < nMin)
    {
        return nMin;
    }
    if (nValue > nMax)
    {
        return nMax;
    }
    return nValue;
}

/* 限制RGN宽高不超过画面尺寸，同时保留对齐要求 */
inline int clamp_rgn_size(int nSize, int nActualSize, int nAlign)
{
    if (nActualSize <= 0)
    {
        return 0;
    }

    int nMaxSize = ALIGN_BACK(nActualSize, nAlign);
    if (nMaxSize <= 0)
    {
        nMaxSize = nActualSize;
    }

    return std::min(nSize, nMaxSize);
}

/**
 * @brief   : 获取指定码流通道下展会面板的自适应宽高
 * @param    {int} nChn：编码通道号
 * @param    {uint32_t &} unWidth：输出面板宽度
 * @param    {uint32_t &} unHeight：输出面板高度
 * @return   {void}
 * @note    : 以 1080P 下的展会面板尺寸为基准，按主子码流实际分辨率同比缩放
 */
#if CAP_EXHIBITION_OSD_PANEL
void get_exhibition_panel_size(int nChn, uint32_t &unWidth, uint32_t &unHeight)
{
    /* 默认回退到 1080P 基准尺寸 */
    unWidth = EXHIBITION_PANEL_BASE_WIDTH;
    unHeight = EXHIBITION_PANEL_BASE_HEIGHT;

    /* 当前所有码流配置列表 */
    std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
    CStreamVideo::instance()->getVideoConfig(vstVideoConfig);
    if (nChn < 0 || static_cast<size_t>(nChn) >= vstVideoConfig.size())
    {
        return;
    }

    /* 当前码流实际宽度 */
    int nActualWidth = vstVideoConfig.at(nChn).stVideoResolution.nWidth;
    /* 当前码流实际高度 */
    int nActualHeight = vstVideoConfig.at(nChn).stVideoResolution.nHeight;
    if (nActualWidth <= 0 || nActualHeight <= 0)
    {
        return;
    }

    /* 按 1080P 基准宽度同比缩放后的面板宽度 */
    int nScaledWidth = EXHIBITION_PANEL_BASE_WIDTH * nActualWidth / PIXEL_WIDTH_1920;
    /* 按 1080P 基准高度同比缩放后的面板高度 */
    int nScaledHeight = EXHIBITION_PANEL_BASE_HEIGHT * nActualHeight / PIXEL_HEIGHT_1080;

    /* 至少保留一个有效 RGN 尺寸，并满足底层对齐要求 */
    nScaledWidth = ALIGN_UP(std::max(16, nScaledWidth), 16);
    nScaledHeight = ALIGN_UP(std::max(OT_RGN_ALIGN, nScaledHeight), OT_RGN_ALIGN);

    unWidth = static_cast<uint32_t>(clamp_rgn_size(nScaledWidth, nActualWidth, 16));
    unHeight = static_cast<uint32_t>(clamp_rgn_size(nScaledHeight, nActualHeight, OT_RGN_ALIGN));
}
#endif

/* 计算当前文本实际渲染尺寸，用于动态增大RGN区域 */
bool get_text_render_size(const char *pText, int nFontSize, int &nTextWidth, int &nTextHeight)
{
    if (!pText || !strlen(pText) || nFontSize <= 0)
    {
        return false;
    }

    if (CSDLUtils::instance()->measureText(std::string(pText), nFontSize, ITC_FONT_FILE, nTextWidth, nTextHeight))
    {
        return false;
    }
    if (nTextWidth <= 0 || nTextHeight <= 0)
    {
        return false;
    }

    return true;
}
}

COverplayDraw::COverplayDraw()
{
    m_bIsRunning = false;                                          /* 运行标志 */
    m_pVecRgns.resize(OT_RGN_VENC_MAX_OVERLAY_NUM * VENC_CHN_MAX); /* 初始化rgn指针向量 */
    m_bIsTimeUpdate = true;                                        /* 时间信息rgn是否需要更新 */
    m_bIsOthersUpdate = true;                                      /* 其他信息rgn是否需要更新 */
}

COverplayDraw::~COverplayDraw()
{
}

IpcRet_E COverplayDraw::init()
{
    /* 获取osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
    COsdManage::instance()->get_overplay_info(vecOverplayInfo);

    /* 判断是否存在osd信息 */
    if (!vecOverplayInfo.size())
    {
        return ERR_PARAM_NULL;
    }

    /* 一个osd模板需要根据venc通道数 主码流、子码流 创建对应数量的rgn */
    for (size_t i = 0; i < vecOverplayInfo.size() * VENC_CHN_JPEG; i++)
    {
        int nRet = 0;
        int nIndex = i / VENC_CHN_JPEG; /* 下标 */
        int nChn = i % VENC_CHN_JPEG;   /* 码流通道 */

        if(nIndex <= Osd::ElementType_E::ELEMENT_TYPE_PEOPLE)
        {
            // note 暂未使用的自定义元素种类和AI 动态分析的人头 AI分析使用角框
            continue;
        }

        /* 为rgn分配空间 */
        m_pVecRgns.at(i) = mppRgn_alloc(set_rgn(vecOverplayInfo.at(nIndex), nChn, i));
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
    }

#if CAP_EXHIBITION_OSD_PANEL
    Osd::OverplayInfo_S stExhibitionPanelInfo = build_exhibition_panel_info();
    for (int nChn = 0; nChn < VENC_CHN_JPEG; ++nChn)
    {
        /* 当前码流通道下自适应后的展会面板宽度 */
        uint32_t unPanelWidth = 0;
        /* 当前码流通道下自适应后的展会面板高度 */
        uint32_t unPanelHeight = 0;
        get_exhibition_panel_size(nChn, unPanelWidth, unPanelHeight);

        uint32_t unHandle = EXHIBITION_PANEL_OVERPLAY_INDEX * VENC_CHN_JPEG + nChn;
        int nRet = 0;
        m_pVecRgns.at(unHandle) = mppRgn_alloc(set_rgn(stExhibitionPanelInfo,
                                                       nChn,
                                                       unHandle,
                                                       unPanelWidth,
                                                       unPanelHeight));
        if (NULL == m_pVecRgns.at(unHandle))
        {
            dlog_error("HandleId:%u, 展会面板区域分配失败", unHandle);
            return ERR;
        }

        nRet = m_pVecRgns.at(unHandle)->mppRgn_create(m_pVecRgns.at(unHandle));
        if (OK != nRet)
        {
            dlog_error("HandleId:%u, 展会面板区域创建失败: %d", unHandle, nRet);
            return ERR;
        }
    }
#endif

    /* 初始化抓拍叠加信息RGN */
    init_capture_overplay();

    /* 初始化SDL工具类 */
    if (CSDLUtils::instance()->init() != 0)
    {
        dlog_error("SDL工具类初始化失败");
        return ERR;
    }

    /* 开始rgn线程 */
    start();

    dlog_info("osd_overplay初始化成功");
    return OK;
}

IpcRet_E COverplayDraw::deinit()
{
    /* 停止rgn */
    stop();
    
    /* 去初始化抓拍叠加信息RGN */
    deinit_capture_overplay();

    /* 销毁rgn */
    destroy_rgn();

    /* 释放绘制检测框覆盖图片的Surface */
    for (auto const& [handle, surface] : m_mapOverlaySurfaces)
    {
        CSDLUtils::instance()->releaseSurface(surface);
    }
    m_mapOverlaySurfaces.clear();

    return OK;
}

IpcRet_E COverplayDraw::init_capture_overplay()
{
    /* 获取osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayCaptureInfo;
    COsdManage::instance()->get_overplay_capture_info(vecOverplayCaptureInfo);

    /* 判断是否存在osd信息 */
    if (!vecOverplayCaptureInfo.size())
    {
        return ERR_PARAM_NULL;
    }

    /* 一个osd模板需要根据venc通道数 主码流、子码流 创建对应数量的rgn */
    for (size_t i = OT_RGN_VENC_MAX_OVERLAY_NUM * VENC_CHN_JPEG;
         i < OT_RGN_VENC_MAX_OVERLAY_NUM * VENC_CHN_JPEG + vecOverplayCaptureInfo.size();
         i++)
    {
        int nRet = 0;
        int nIndex = i - OT_RGN_VENC_MAX_OVERLAY_NUM * VENC_CHN_JPEG; /* 下标 */

        /* 为rgn分配空间 */
        m_pVecRgns.at(i) = mppRgn_alloc(set_rgn(vecOverplayCaptureInfo.at(nIndex), 2, i));
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
    }

    dlog_info("osd_overplay_jpeg初始化成功");
    return OK;
}

IpcRet_E COverplayDraw::deinit_capture_overplay()
{
    return OK;
}

void COverplayDraw::set_update_flag(bool bIsUpdate)
{
    m_bIsTimeUpdate = bIsUpdate;
    m_bIsOthersUpdate = bIsUpdate;
}

void COverplayDraw::update_ai_result(int nWidth, int nHeight, const std::vector<Common::RectInfo_S> &vRectInfo, const Osd::OverplayInfo_S stOverplayInfo)
{
    if(!m_bIsRunning)
    {
        return;
    }

    /* 静态变量 判断当前 OSD 是否为显示状态 */
    static bool s_bShow = false;
    for (size_t i = 0; i < m_pVecRgns.size(); i++)
    {
        if (!m_pVecRgns[i] || Osd::ELEMENT_TYPE_PEOPLE != m_pVecRgns[i]->unLayer)
        {
            continue;
        }

        if (vRectInfo.empty())
        {
            if(s_bShow)
            {
                m_pVecRgns[i]->mppRgn_showOrHide(m_pVecRgns[i], TD_FALSE);
            }
        }
        else
        {
            std::vector<Common::RectInfo_S> vRect = vRectInfo;
            
            float fW = static_cast<float>(m_pVecRgns[i]->unWidth) / nWidth;
            float fH = static_cast<float>(m_pVecRgns[i]->unHeight) / nHeight;
            for (size_t j = 0; j < vRect.size(); j++)
            {
                auto &rect = vRect[j];
                rect.nX1 *= fW;
                rect.nY1 *= fH;
                rect.nX2 *= fW;
                rect.nY2 *= fH;
                // dlog_debug("vRectInfo %d: [%d,%d] [%d,%d]", j, vRectInfo[j].nX1, vRectInfo[j].nY1, vRectInfo[j].nX2, vRectInfo[j].nY2);
                // dlog_debug("Rect      %d: [%d,%d] [%d,%d]", j, rect.nX1, rect.nY1, rect.nX2, rect.nY2);
            }

            /* 异步线程执行 */
            auto thrRun = [this](HiRgn_S *pHandle, Osd::OverplayInfo_S stuOverplayInfo, const std::vector<Common::RectInfo_S> &vRectInfo)
            {
                // double dTime = get_time_ms();
                draw_rect(pHandle, stuOverplayInfo, vRectInfo);
                // dlog_debug("draw_rect time: %f", get_time_ms() - dTime);
            };
            std::thread thr(thrRun, m_pVecRgns[i], stOverplayInfo, vRect);
            thr.detach();
            // draw_rect(m_pVecRgns[i], stOverplayInfo, vRect);
        }
    }

    /* 执行完，进行显示状态标志变换 */
    if (vRectInfo.size() < 1)
    {
        s_bShow = false;
    }
    else
    {
        s_bShow = true;
    }

    return;
}

void COverplayDraw::start()
{
    m_bIsRunning = true;

    m_showTimeThread = std::thread(
        [this]()
        {
            this->osd_show_time();
        });
    // m_showTimeThread.detach();

    m_showOthersThread = std::thread(
        [this]()
        {
            this->osd_show_others();
        });
    // m_showOthersThread.detach();

    m_flickerThread = std::thread(
        [this]()
        {
            this->osd_flicker();
        });
    // m_flickerThread.detach();

    m_showCaptureThread = std::thread(
        [this]()
        {
            this->osd_show_capture();
        });

#if CAP_EXHIBITION_OSD_PANEL
    m_showExhibitionPanelThread = std::thread(
        [this]()
        {
            this->osd_show_exhibition_panel();
        });
#endif
    return;
}

void COverplayDraw::stop()
{
    m_bIsRunning = false;

    /* 等待所有线程结束 */
    if (m_showCaptureThread.joinable())
    {
        m_showCaptureThread.join();
    }
#if CAP_EXHIBITION_OSD_PANEL
    if (m_showExhibitionPanelThread.joinable())
    {
        m_showExhibitionPanelThread.join();
    }
#endif
    if (m_flickerThread.joinable())
    {
        m_flickerThread.join();
    }
    if (m_showOthersThread.joinable())
    {
        m_showOthersThread.join();
    }
    if (m_showTimeThread.joinable())
    {
        m_showTimeThread.join();
    }

    return;
}

HiRgnNeedParam_S COverplayDraw::set_rgn(Osd::OverplayInfo_S stuOverplayInfo, int nChn, uint32_t unHandle, uint32_t unWidth, uint32_t unHeight)
{
    /* 获取osd共用信息 */
    Osd::ShareInfo_S stuShareInfo;
    COsdManage::instance()->get_osd_share_info(stuShareInfo);

    /* 获取码流的分辨率大小 */
    std::vector<Video_NS::VideoConfig_S> vstVideoConfig;
    CStreamVideo::instance()->getVideoConfig(vstVideoConfig);

    /* 配置rgn所需参数 */
    HiRgnNeedParam_S stuRgnNeedParam;
    memset(&stuRgnNeedParam, 0, sizeof(stuRgnNeedParam));
    stuRgnNeedParam.unOpFlag = REGION_OP_CHN;
    stuRgnNeedParam.unModId = OT_ID_VENC;
    stuRgnNeedParam.unDevId = RGN_OSD_VENC;
    stuRgnNeedParam.unType = OT_RGN_OVERLAY;

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
    if (nReferenceWidth <= 0)
    {
        nReferenceWidth = nActualWidth;
    }
    if (nReferenceHeight <= 0)
    {
        nReferenceHeight = nActualHeight;
    }

    stuRgnNeedParam.bIsShow = (td_bool)stuOverplayInfo.stuInfo.bEnable;
    stuRgnNeedParam.unHandle = unHandle;

    /* 判断长宽是否已经由画图的大小确定 */
    if (unWidth && unHeight)
    {
        /*
         * 文本RGN宽度必须保持16字节对齐，否则贴图后可能出现斜向拉伸/错行。
         * 高度维持2字节对齐即可。
         */
        int nTemplateWidth = ALIGN_UP(static_cast<int>(unWidth), 16);
        int nTemplateHeight = ALIGN_UP(static_cast<int>(unHeight), OT_RGN_ALIGN);
        stuRgnNeedParam.unWidth = clamp_rgn_size(nTemplateWidth, nActualWidth, 16);
        stuRgnNeedParam.unHeight = clamp_rgn_size(nTemplateHeight, nActualHeight, OT_RGN_ALIGN);
    }
    else
    {
        if(stuOverplayInfo.stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_PEOPLE)
        {
            // note AI 动态分析专用
            stuRgnNeedParam.unWidth = ALIGN_BACK(nActualWidth, OT_RGN_ALIGN);
            stuRgnNeedParam.unHeight = ALIGN_BACK(nActualHeight, OT_RGN_ALIGN);
        }
        else
        {
            /* 模板尺寸需要根据模板内容等参数进行公式计算 */
            if (unHandle >= OT_RGN_VENC_MAX_OVERLAY_NUM * VENC_CHN_JPEG || stuOverplayInfo.stuOverplay.nWidth <= 0)
            {
                // note 人脸抓拍叠加信息与网页OSD功能计算宽度方式有区别，人脸抓拍使用此方式
                stuRgnNeedParam.unWidth = calculate_template_width(static_cast<int>(strInfo.length()), stuOverplayInfo.stuOverplay.nFontSize, nActualWidth,
                                                                   nReferenceWidth);
            }
            else
            {
                stuRgnNeedParam.unWidth = calculate_template_width(stuOverplayInfo.stuOverplay.nWidth, nActualWidth, nReferenceWidth);
            }
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
    nX = clamp_value(nX, 0, std::max(0, nActualWidth - static_cast<int>(stuRgnNeedParam.unWidth)));
    nY = clamp_value(nY, 0, std::max(0, nActualHeight - static_cast<int>(stuRgnNeedParam.unHeight)));
    stuRgnNeedParam.unStartX = nX;
    stuRgnNeedParam.unStartY = nY;

    /* 转换透明度由百分比到rgb范围(0~255) */
    stuRgnNeedParam.unFgAlpha = algha_percentage_to_rgb(stuOverplayInfo.stuOverplay.nFontAlpha);
    stuRgnNeedParam.unBgAlpha = algha_percentage_to_rgb(stuOverplayInfo.stuOverplay.nBackAlpha);
    stuRgnNeedParam.unFgColor = std::stoul(stuOverplayInfo.stuOverplay.strFontColor, NULL, 16); /* 16 表示十六进制 */
    stuRgnNeedParam.unBgColor = std::stoul(stuOverplayInfo.stuOverplay.strBackColor, NULL, 16); /* 16 表示十六进制 */

    stuRgnNeedParam.unLayer = stuOverplayInfo.stuOverplay.enElementType;

    /* 获取模板字体大小 */
    stuRgnNeedParam.unFontSize = calculate_text_size(stuOverplayInfo.stuOverplay.nFontSize, nActualWidth, nReferenceWidth);

    /* 获取模板闪烁开关 */
    stuRgnNeedParam.bIsFlicker = (td_bool)stuOverplayInfo.stuOverplay.bEnableFlicker;

    return stuRgnNeedParam;
}

void COverplayDraw::destroy_rgn()
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

void COverplayDraw::osd_show_time()
{
    pthread_setname_np(pthread_self(), "OsdShowTime");

    int nIndex = 0;
    time_t stuLastTime = 0;
    time_t stuNowTime = time(NULL);
    /* 线程绘制睡眠用 */
    struct timeval tv;
    gettimeofday(&tv, NULL);
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
            for (size_t i = 0; i < vecOverplayInfo.size() * VENC_CHN_JPEG; i++)
            {
                if (!m_pVecRgns.at(i) || Osd::ElementType_E::ELEMENT_TYPE_TIME != m_pVecRgns.at(i)->unLayer)
                {
                    continue;
                }

                nIndex = i / VENC_CHN_JPEG;
                if (vecOverplayInfo.at(nIndex).stuInfo.bEnable)
                {
                    if (!m_pVecRgns.at(i)->bIsAttached)
                    {
                        if (m_pVecRgns.at(i)->mppRgn_attachToChn(m_pVecRgns.at(i)))
                        {
                            dlog_error("添加rgn到通道失败");
                            continue;
                        }
                    }
                    /* 判断osd信息是否更新 */
                    if (m_bIsTimeUpdate)
                    {
                        /* 更新rgn */
                        m_pVecRgns.at(i)->mppRgn_update(m_pVecRgns.at(i), set_rgn(vecOverplayInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
                        /* 时间模板配置变化时，仅在这里同步区域尺寸与位置，避免秒级刷新重复摘挂 */
                        m_pVecRgns.at(i)->mppRgn_changeRect(m_pVecRgns.at(i), m_pVecRgns.at(i)->unWidth, m_pVecRgns.at(i)->unHeight);
                        m_pVecRgns.at(i)->mppRgn_changePos(m_pVecRgns.at(i), m_pVecRgns.at(i)->unStartX, m_pVecRgns.at(i)->unStartY);
                    }

                    /* 获取osd共用信息 */
                    COsdManage::instance()->get_osd_share_info(stuShareInfo);
                    // dlog_info("%s", get_template_text(vecOverplayInfo.at(nIndex), stuShareInfo).c_str());

                    if (m_pVecRgns.at(i)->bIsFlicker && (stuNowTime % 2) && stuLastTime)
                    {
                        /* 奇数显示达到闪烁效果 */
                        if (m_pVecRgns.at(i)->mppRgn_showOrHide(m_pVecRgns.at(i), (td_bool)((stuNowTime % 2) ? true : false)))
                        {
                            dlog_error("设置rgn闪烁失败");
                        }
                    }

                    /* 对相应的rgn画出对应文本 */
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        draw_text(m_pVecRgns.at(i), vecOverplayInfo.at(nIndex), get_template_text(vecOverplayInfo.at(nIndex), stuShareInfo).c_str());
                    }

                    if (m_pVecRgns.at(i)->bIsFlicker && !(stuNowTime % 2) && stuLastTime)
                    {
                        /* 偶数隐藏达到闪烁效果 */
                        if (m_pVecRgns.at(i)->mppRgn_showOrHide(m_pVecRgns.at(i), (td_bool)((stuNowTime % 2) ? true : false)))
                        {
                            dlog_error("设置rgn闪烁失败");
                        }
                    }
                }
                else if (!vecOverplayInfo.at(nIndex).stuInfo.bEnable)
                {
                    if (m_pVecRgns.at(i)->bIsAttached)
                    {
                        if (m_pVecRgns.at(i)->mppRgn_detachFromChn(m_pVecRgns.at(i)))
                        {
                            dlog_error("从通道中撤出rgn失败");
                            continue;
                        }
                        /* 判断osd信息是否更新 */
                        if (m_bIsTimeUpdate)
                        {
                            /* 更新rgn */
                            m_pVecRgns.at(i)->mppRgn_update(m_pVecRgns.at(i), set_rgn(vecOverplayInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
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
            gettimeofday(&tv, NULL);
            /* 获取当前时间的微秒部分 */
            long current_us = tv.tv_usec;
            /* 计算到下一秒还剩多少微秒 */
            long us_to_next_second = 1000000 - current_us;
            /* 计算实际睡眠时间 */
            long sleep_us = us_to_next_second - WAKEUP_ADVANCE_US;

            /* 如果计算结果为负数或过小,说明当前时间点已经很接近下一秒 */
            /* 此时应该立即绘制,或者睡眠一个很短的时间后绘制 */
            if (sleep_us < 80000) /* 小于80ms */
            {
                /* 时间太紧,跳到下一个周期 */
                /* 睡眠到再下一秒前的唤醒时间点 */
                sleep_us = us_to_next_second + 1000000 - WAKEUP_ADVANCE_US;
            }

            /* 执行睡眠 */
            if (sleep_us > 0 && sleep_us < 2000000) /* 确保在合理范围内(0~2秒) */
            {
                usleep(sleep_us);
            }
            else
            {
                /* 异常情况,使用默认睡眠 */
                usleep(800000); /* 睡眠800ms */
            }

        }
    }

    return;
}

void COverplayDraw::osd_show_others()
{
    pthread_setname_np(pthread_self(), "OsdShowOthers");

    int nIndex = 0;
    /* osd信息 */
    std::vector<Osd::OverplayInfo_S> vecOverplayInfo;
    /* osd共用信息 */
    Osd::ShareInfo_S stuShareInfo;

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
        for (size_t i = 0; i < vecOverplayInfo.size() * VENC_CHN_JPEG; i++)
        {
            if (!m_pVecRgns.at(i) || Osd::ElementType_E::ELEMENT_TYPE_TIME == m_pVecRgns.at(i)->unLayer ||
                Osd::ElementType_E::ELEMENT_TYPE_PEOPLE == m_pVecRgns.at(i)->unLayer ||
                is_exhibition_panel_handle(i))
            {
                continue;
            }

            nIndex = i / VENC_CHN_JPEG; /* 下标 */
            if (vecOverplayInfo.at(nIndex).stuInfo.bEnable)
            {
                if (!m_pVecRgns.at(i)->bIsAttached)
                {
                    if (m_pVecRgns.at(i)->mppRgn_attachToChn(m_pVecRgns.at(i)))
                    {
                        dlog_error("添加rgn到通道失败");
                        continue;
                    }
                }
                /* 更新rgn */
                m_pVecRgns.at(i)->mppRgn_update(m_pVecRgns.at(i), set_rgn(vecOverplayInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));

                /* 对相应的rgn画出对应文本 */
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    draw_text(m_pVecRgns.at(i), vecOverplayInfo.at(nIndex), get_template_text(vecOverplayInfo.at(nIndex), stuShareInfo).c_str());
                }
            }
            else if (!vecOverplayInfo.at(nIndex).stuInfo.bEnable)
            {
                if (m_pVecRgns.at(i)->bIsAttached)
                {
                    if (m_pVecRgns.at(i)->mppRgn_detachFromChn(m_pVecRgns.at(i)))
                    {
                        dlog_error("从通道中撤出rgn失败");
                        continue;
                    }
                }
                /* 更新rgn */
                m_pVecRgns.at(i)->mppRgn_update(m_pVecRgns.at(i), set_rgn(vecOverplayInfo.at(nIndex), m_pVecRgns.at(i)->unChnId, m_pVecRgns.at(i)->unHandle));
            }
        }
        if (m_bIsOthersUpdate)
        {
            m_bIsOthersUpdate = !m_bIsOthersUpdate;
        }
    }

    return;
}

void COverplayDraw::osd_flicker()
{
    pthread_setname_np(pthread_self(), "OsdFlicker");

    /* 睡1秒再进入循环（确保rgn已经添加到通道上） */
    sleep(1);
    while (m_bIsRunning)
    {
        time_t stuNowTime = time(NULL);
        for (size_t i = 0; i < OT_RGN_VENC_MAX_OVERLAY_NUM * VENC_CHN_JPEG; i++)
        {
            if (!m_pVecRgns.at(i) ||
                Osd::ELEMENT_TYPE_TIME == m_pVecRgns.at(i)->unLayer ||
                Osd::ElementType_E::ELEMENT_TYPE_PEOPLE == m_pVecRgns.at(i)->unLayer ||
                is_exhibition_panel_handle(i))
            {
                continue;
            }
            if (m_pVecRgns.at(i)->bIsShow && m_pVecRgns.at(i)->bIsFlicker)
            {
                /* 奇数显示, 偶数隐藏 */
                if (m_pVecRgns.at(i)->mppRgn_showOrHide(m_pVecRgns.at(i), (td_bool)((stuNowTime % 2) ? true : false)))
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

void COverplayDraw::osd_show_capture()
{
    pthread_setname_np(pthread_self(), "OsdShowCapture");

    /* 人脸抓拍叠加信息 */
    Alarm::OverlayInfo_S stOverlayinfo;
    /* 算法配置 */
    Event::AlgorithmConfig_S stAlgorithmConfig;
    /* 人脸抓拍上一次的状态 */
    int nLastFaceCaptureState = 0;
    /* 等待RGN初始化完成 */
    sleep(1);

    while (m_bIsRunning)
    {
        /* 是否启用人脸抓拍 */
        CEventConfigure::instance()->get_configure(stAlgorithmConfig);

        /* 检测人脸抓拍状态是否从开启变为关闭 */
        if (nLastFaceCaptureState != 0 && stAlgorithmConfig.nEnFaceCapture == 0)
        {
            /* 获取抓拍叠加信息 */
            std::vector<Osd::OverplayInfo_S> vecOverplayCaptureInfo;
            COsdManage::instance()->get_overplay_capture_info(vecOverplayCaptureInfo);

            /* 清空叠加信息内容 */
            stOverlayinfo = Alarm::OverlayInfo_S();
            update_capture_overlay_info(stOverlayinfo, vecOverplayCaptureInfo);
            COsdManage::instance()->set_overplay_capture_info(vecOverplayCaptureInfo);

            /* 获取共享信息并处理RGN，关闭所有抓拍叠加 */
            Osd::ShareInfo_S stShareInfo;
            COsdManage::instance()->get_osd_share_info(stShareInfo);
            process_capture_rgns(vecOverplayCaptureInfo, stShareInfo);
        }

        /* 更新状态 */
        nLastFaceCaptureState = stAlgorithmConfig.nEnFaceCapture;

        /* 如果人脸抓拍未启用，继续等待 */
        if (!stAlgorithmConfig.nEnFaceCapture)
        {
            sleep(1);
            continue;
        }

        /* 获取OSD信息 */
        std::vector<Osd::OverplayInfo_S> vecOverplayCaptureInfo;
        COsdManage::instance()->get_overplay_capture_info(vecOverplayCaptureInfo);
        if (vecOverplayCaptureInfo.empty())
        {
            sleep(1);
            continue;
        }

        /* 获取共用信息 */
        Osd::ShareInfo_S stShareInfo;
        COsdManage::instance()->get_osd_share_info(stShareInfo);

        /* 获取叠加信息 */
        Alarm::OverlayInfo_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);

        /* 是否遍历处理所有JPEG抓拍RGN */
        bool bProcess = stInfo.bOverlayCaptureTime;

        /* 叠加信息有更改 */
        if(stOverlayinfo != stInfo)
        {
            bProcess = true;
            stOverlayinfo = stInfo;
            /* 更新抓拍叠加信息 */
            update_capture_overlay_info(stOverlayinfo, vecOverplayCaptureInfo);
            COsdManage::instance()->set_overplay_capture_info(vecOverplayCaptureInfo);
        }

        if(bProcess)
        {
            /* 遍历处理所有抓拍叠加信息RGN */
            process_capture_rgns(vecOverplayCaptureInfo, stShareInfo);
        }

        /* 睡眠900ms，等待下次 */
        usleep(900 * 1000);
    }
}

void COverplayDraw::osd_show_exhibition_panel()
{
#if !CAP_EXHIBITION_OSD_PANEL
    return;
#else
    pthread_setname_np(pthread_self(), "OsdShowExpo");

    /* 展会面板的隐藏态 overlay 配置 */
    const Osd::OverplayInfo_S stHiddenPanelInfo = build_exhibition_panel_info();
    /* 展会面板的显示态 overlay 配置 */
    Osd::OverplayInfo_S stVisiblePanelInfo = stHiddenPanelInfo;
    stVisiblePanelInfo.stuInfo.bEnable = true;
    /* 上一次成功绘制的面板文本 */
    std::string strLastText;
    /* 上一次循环中面板是否处于可见状态 */
    bool bLastVisible = false;

    while (m_bIsRunning)
    {
        /* OSD 管理层缓存的结构化面板结果 */
        OsdPanel::PanelFrame_S stPanelFrame;
        /* 面板缓存版本号 */
        uint64_t unVersion = 0;
        /* 面板缓存更新时间 */
        uint64_t unUpdateTimeMs = 0;
        COsdManage::instance()->get_panel_result(stPanelFrame, unVersion, unUpdateTimeMs);

        /* 当前稳态时钟时间 */
        const uint64_t unNowTimeMs = get_steady_time_ms();
        /* 当前缓存是否仍在有效时间窗口内 */
        bool bPanelValid = unVersion > 0 &&
                           !stPanelFrame.vecItems.empty() &&
                           unNowTimeMs >= unUpdateTimeMs &&
                           (unNowTimeMs - unUpdateTimeMs) <= EXHIBITION_PANEL_VALID_TIME_MS;
        /* 当前帧最终需要绘制的面板文本 */
        std::string strPanelText;
        if (bPanelValid)
        {
            strPanelText = OsdPanel::build_panel_text(stPanelFrame);
            bPanelValid = !strPanelText.empty();
        }

        for (size_t i = 0; i < static_cast<size_t>(VENC_CHN_JPEG); ++i)
        {
            /* 当前编码通道对应的面板 RGN 句柄 */
            HiRgn_S *pHandle = m_pVecRgns.at(i);
            if (!pHandle || !is_exhibition_panel_handle(i))
            {
                continue;
            }

            /* 当前码流通道下自适应后的展会面板宽度 */
            uint32_t unPanelWidth = 0;
            /* 当前码流通道下自适应后的展会面板高度 */
            uint32_t unPanelHeight = 0;
            get_exhibition_panel_size(pHandle->unChnId, unPanelWidth, unPanelHeight);

            if (!bPanelValid)
            {
                if (pHandle->bIsAttached)
                {
                    pHandle->mppRgn_detachFromChn(pHandle);
                    pHandle->mppRgn_update(pHandle,
                                           set_rgn(stHiddenPanelInfo,
                                                   pHandle->unChnId,
                                                   pHandle->unHandle,
                                                   unPanelWidth,
                                                   unPanelHeight));
                }
                continue;
            }

            /* 面板当前未挂载到通道时，先按固定配置初始化并挂载 */
            if (!pHandle->bIsAttached)
            {
                pHandle->mppRgn_update(pHandle,
                                       set_rgn(stVisiblePanelInfo,
                                               pHandle->unChnId,
                                               pHandle->unHandle,
                                               unPanelWidth,
                                               unPanelHeight));
                if (pHandle->mppRgn_attachToChn(pHandle))
                {
                    dlog_error("展会面板添加rgn到通道失败");
                    continue;
                }
            }

            /* 文本内容、可见状态变化时才触发重绘，避免无效刷新 */
            if (strPanelText != strLastText || !bLastVisible || !pHandle->bIsAttached)
            {
                pHandle->mppRgn_update(pHandle,
                                       set_rgn(stVisiblePanelInfo,
                                               pHandle->unChnId,
                                               pHandle->unHandle,
                                               unPanelWidth,
                                               unPanelHeight));
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    draw_text(pHandle, stVisiblePanelInfo, strPanelText.c_str());
                }
            }
        }

        strLastText = bPanelValid ? strPanelText : "";
        bLastVisible = bPanelValid;
        usleep(EXHIBITION_PANEL_RENDER_INTERVAL_MS * 1000);
    }
#endif
}

void COverplayDraw::process_capture_rgns(const std::vector<Osd::OverplayInfo_S> &vecOverplayCaptureInfo,
                                         const Osd::ShareInfo_S &stShareInfo)
{
    size_t nStartIndex = OT_RGN_VENC_MAX_OVERLAY_NUM * VENC_CHN_JPEG;
    size_t nEndIndex = nStartIndex + vecOverplayCaptureInfo.size();

    for (size_t i = nStartIndex; i < nEndIndex; i++)
    {
        if (!m_pVecRgns[i])
        {
            continue;
        }

        size_t nInfoIndex = i - nStartIndex;
        const auto &overplayInfo = vecOverplayCaptureInfo[nInfoIndex];
        auto pRgn = m_pVecRgns[i];

        /* 根据元素类型处理 */
        if (overplayInfo.stuInfo.bEnable) /* 启用OSD */
        {
            /* 如果未显示,则添加到通道 */
            if (!pRgn->bIsAttached)
            {
                if (pRgn->mppRgn_attachToChn(pRgn))
                {
                    dlog_error("添加rgn到通道失败");
                    return;
                }
            }

            pRgn->mppRgn_update(pRgn, set_rgn(overplayInfo, pRgn->unChnId, pRgn->unHandle));

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                std::string strText = get_template_text(overplayInfo, stShareInfo);
                draw_text(pRgn, overplayInfo, strText.c_str());
            }
        }
        else /* 禁用OSD */
        {
            if (pRgn->bIsAttached)
            {
                if (pRgn->mppRgn_detachFromChn(pRgn))
                {
                    dlog_error("从通道中撤出rgn失败");
                    return;
                }

                pRgn->mppRgn_update(pRgn, set_rgn(overplayInfo, pRgn->unChnId, pRgn->unHandle));
            }
        }
    }
}

void COverplayDraw::update_capture_overlay_info(const Alarm::OverlayInfo_S &stInfo,
                                                std::vector<Osd::OverplayInfo_S> &vecOverplayCaptureInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string strFontColor;
    switch (stInfo.enFontColor)
    {
    case Osd::OSD_COLOR_E::OSD_COLOR_BLACK:
        strFontColor = "0x000000";
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_WHITE:
        strFontColor = "0xFFFFFF";
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_CUSTOMIZE:
        strFontColor = stInfo.strFontColor;
        if (stInfo.strFontColor.size() > 0 && stInfo.strFontColor[0] == '#')
        {
            strFontColor.replace(0, 1, "0x"); // 替换 '#' 为 '0x'
        }
        break;
    default:
        dlog_error("Osd字体颜色设置错误");
        return;
    }

    /* 更新RGN的叠加参数 */
    for (auto &info : vecOverplayCaptureInfo)
    {
        info.stuInfo.bEnable = false;
        info.stuOverplay.strCustomize.clear();
        info.stuOverplay.strFontColor = strFontColor;
    }

    /* 更新叠加信息 */
    if (stInfo.bOverlayDeviceID)
    {
        /* 叠加设备编号 */
        vecOverplayCaptureInfo[RGN_DEVICE_ID_HANDLE].stuInfo.bEnable = true;
        vecOverplayCaptureInfo[RGN_DEVICE_ID_HANDLE].stuOverplay.strCustomize = std::to_string(stInfo.nDeviceID);
    }
    if (stInfo.bOverlayMonitoryPointInfo)
    {
        /* 叠加监控点信息 */
        vecOverplayCaptureInfo[RGN_MONITORY_POINT_INFO_HANDLE].stuInfo.bEnable = true;
        vecOverplayCaptureInfo[RGN_MONITORY_POINT_INFO_HANDLE].stuOverplay.strCustomize = stInfo.strMonitoryPointInfo;
    }

    /* 叠加抓拍时间 */
    vecOverplayCaptureInfo[RGN_CAPTURE_TIME_HANDLE].stuInfo.bEnable = stInfo.bOverlayCaptureTime;
}

std::string COverplayDraw::get_template_text(Osd::OverplayInfo_S stuOverplayInfo, Osd::ShareInfo_S stuShareInfo)
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
        else
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

void COverplayDraw::get_reference_size(Osd::ReferenceSize_E enReferenceSize, int &nWidth, int &nHeight)
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

int COverplayDraw::calculate_text_size(int nFontSize, int nActualSize, int nReferenceSize)
{
    /* 实际字体大小 = 参考字体大小 * 实际分辨率大小 / 参考分辨率大小 */
    int nActualFontSize = nFontSize * nActualSize / nReferenceSize;
    /* 确保字体大小 2 字节对齐，不是则 +1 向上对齐 */
    return  ALIGN_UP(nActualFontSize, OT_RGN_ALIGN);
}

int COverplayDraw::calculate_template_width(int nWith, int nFontSize, int nActualWidth, int nReferenceWidth)
{
    /* 模板宽度 = （字符串长度 * 字体大小） * 实际分辨率宽度 / 参考分辨率宽度 */
    int nTemplateWidth = (nWith * nFontSize) * nActualWidth / nReferenceWidth;

    /* 确保模板宽度 2 字节对齐，不是则 +1 向上对齐 */
    nTemplateWidth = ALIGN_UP(nTemplateWidth, OT_RGN_ALIGN);

    /* 限制长度 */
    if (nActualWidth < nTemplateWidth)
    {
        nTemplateWidth = nActualWidth;
    }

    /* 宽度需16字节对齐，且不能超过画面宽度 */
    int nAlignedWidth = ALIGN_UP(nTemplateWidth, 16);
    return clamp_rgn_size(nAlignedWidth, nActualWidth, 16);
}

int COverplayDraw::calculate_template_width(int nWith, int nActualWidth, int nReferenceWidth)
{
    /* 模板宽度 = 参考宽度下配置值 * 实际分辨率宽度 / 参考分辨率宽度 */
    int nTemplateWidth = nWith * nActualWidth / nReferenceWidth;

    /* 确保模板宽度 2 字节对齐，不是则 +1 向上对齐 */
    nTemplateWidth = ALIGN_UP(nTemplateWidth, OT_RGN_ALIGN);

    /* 限制长度 */
    if (nActualWidth < nTemplateWidth)
    {
        nTemplateWidth = nActualWidth;
    }

    /* 宽度需16字节对齐，且不能超过画面宽度 */
    int nAlignedWidth = ALIGN_UP(nTemplateWidth, 16);
    return clamp_rgn_size(nAlignedWidth, nActualWidth, 16);
}

int COverplayDraw::calculate_template_height(int nFontSize, int nActualHeight, int nReferenceHeight)
{
    /* 模板高度 = 字体大小 * 实际分辨率高度 / 参考分辨率高度 */
    int nTemplateHeight = nFontSize * nActualHeight / nReferenceHeight;

    /* 确保模板高度 2 字节对齐，不是则 +1 向上对齐 */
    nTemplateHeight = ALIGN_UP(nTemplateHeight, OT_RGN_ALIGN);

    /* 限制高度 */
    if (nActualHeight < nTemplateHeight)
    {
        nTemplateHeight = nActualHeight;
    }

    return nTemplateHeight;
}

void COverplayDraw::get_template_margin(Osd::OverplayInfo_S stuOverplayInfo, int &nActualHorMargin, int &nActualVerMargin, int nActualWidth, int nActualHeight, int nReferenceWidth, int nReferenceHeight)
{
    /* 模板水平边距 = 水平边距 * 实际分辨率宽度 / 参考分辨率宽度 */
    nActualHorMargin = stuOverplayInfo.stuOverplay.nHorMargin * nActualWidth / nReferenceWidth;
    /* 模板垂直边距 = 垂直边距 * 实际分辨率高度 / 参考分辨率高度 */
    nActualVerMargin = stuOverplayInfo.stuOverplay.nVerMargin * nActualHeight / nReferenceHeight;
    /* 确保水平边距 2 字节对齐，不是则 +1 向上对齐 */
    nActualHorMargin = ALIGN_UP(nActualHorMargin, OT_RGN_ALIGN);
    /* 确保垂直边距 2 字节对齐，不是则 +1 向上对齐 */
    nActualVerMargin = ALIGN_UP(nActualVerMargin, OT_RGN_ALIGN);
}

void COverplayDraw::get_start_points(Osd::Align_E enAlign, int &nX, int &nY, int nActualHorMargin, int nActualVerMargin, int nTemplateWidth, int nTemplateHeight, int nActualWidth, int nActualHeight)
{
    switch (enAlign)
    {
    case Osd::Align_E::ALIGN_TOP_LEFT:
        /* 参考点在区域左上角(2字节对齐) */
        nX = ALIGN_UP(nActualHorMargin, OT_RGN_ALIGN);
        nY = ALIGN_UP(nActualVerMargin, OT_RGN_ALIGN);
        break;
    case Osd::Align_E::ALIGN_BOTTOM_LEFT:
        /* 参考点在区域左下角(2字节对齐) */
        nX = ALIGN_UP(nActualHorMargin, OT_RGN_ALIGN);
        nY = ALIGN_BACK(nActualHeight - nTemplateHeight - nActualVerMargin, OT_RGN_ALIGN);
        break;
    case Osd::Align_E::ALIGN_TOP_RIGHT:
        /* 参考点在区域右上角(2字节对齐) */
        nX = ALIGN_BACK(nActualWidth - nTemplateWidth - nActualHorMargin, OT_RGN_ALIGN);
        nY = ALIGN_UP(nActualVerMargin, OT_RGN_ALIGN);
        break;
    case Osd::Align_E::ALIGN_BOTTOM_RIGHT:
        /* 参考点在区域右下角(2字节对齐) */
        nX = ALIGN_BACK(nActualWidth - nTemplateWidth - nActualHorMargin, OT_RGN_ALIGN);
        nY = ALIGN_BACK(nActualHeight - nTemplateHeight - nActualVerMargin, OT_RGN_ALIGN);
        break;
    case Osd::Align_E::ALIGN_CENTER:
        /* 参考点在区域左上角(2字节对齐) */
        nX = ALIGN_UP((nActualWidth - nTemplateWidth) / 2 + nActualHorMargin, OT_RGN_ALIGN);
        nY = ALIGN_UP((nActualHeight - nTemplateHeight) / 2 + nActualVerMargin, OT_RGN_ALIGN);
        break;
    default:
        break;
    }

    return;
}

int COverplayDraw::algha_percentage_to_rgb(uint32_t nAlpha)
{
    /* 透明度由百分比变为0~255的范围 */
    return (100 - nAlpha) * 255 / 100;
}

#if 1
/* 用freetype + sdl + sdl_ttf库来画 */
void COverplayDraw::draw_text(HiRgn_S *pHandle, Osd::OverplayInfo_S stuOverplayInfo, const char *pText)
{
    if (NULL == pHandle || NULL == pText || 0 == strlen(pText))
    {
        if (pHandle && 0 == strlen(pText))
        {
            pHandle->mppRgn_clearPicture(pHandle);
        }
        return;
    }

    /* 确定像素格式 */
    Uint32 u32PixelFormat = SDL_PIXELFORMAT_ARGB4444;

    /* 准备颜色 */
    CSDLUtils::Color_S stFontColor = {
        static_cast<uint8_t>((pHandle->unFgColor >> 16) & 0xFF),
        static_cast<uint8_t>((pHandle->unFgColor >> 8) & 0xFF),
        static_cast<uint8_t>(pHandle->unFgColor & 0xFF),
        static_cast<uint8_t>(pHandle->unFgAlpha)
    };

    CSDLUtils::Color_S stBackColor = {
        static_cast<uint8_t>((pHandle->unBgColor >> 16) & 0xFF),
        static_cast<uint8_t>((pHandle->unBgColor >> 8) & 0xFF),
        static_cast<uint8_t>(pHandle->unBgColor & 0xFF),
        static_cast<uint8_t>(pHandle->unBgAlpha)
    };

    int nTextWidth = 0;
    int nTextHeight = 0;
    /* 是否因为文本越界而触发了区域扩容 */
    bool bNeedResize = false;
    if (get_text_block_render_size(pText, pHandle->unFontSize, nTextWidth, nTextHeight))
    {
        /* 预留少量边界，避免抗锯齿边缘被裁掉 */
        const int nPadding = 4;
        nTextWidth += nPadding;
        nTextHeight += nPadding;

        if (nTextWidth > static_cast<int>(pHandle->unWidth) || nTextHeight > static_cast<int>(pHandle->unHeight))
        {
            int nNeedWidth = std::max(nTextWidth, static_cast<int>(pHandle->unWidth));
            int nNeedHeight = std::max(nTextHeight, static_cast<int>(pHandle->unHeight));
            if (pHandle->mppRgn_update(pHandle, set_rgn(stuOverplayInfo, pHandle->unChnId, pHandle->unHandle, nNeedWidth, nNeedHeight)))
            {
                dlog_error("动态调整RGN宽高失败");
            }
            bNeedResize = true;
        }
    }

    /* 创建表面 */
    SDL_Surface *pSurface = CSDLUtils::instance()->createSurface(
        pHandle->unWidth,
        pHandle->unHeight,
        u32PixelFormat,
        &stBackColor
    );

    if (!pSurface)
    {
        dlog_error("创建表面失败");
        return;
    }

    /* 渲染文本 */
    CSDLUtils::TextRenderConfig_S stTextConfig;
    stTextConfig.strText = pText;
    stTextConfig.nFontSize = pHandle->unFontSize;
    stTextConfig.stFontColor = stFontColor;
    stTextConfig.stBackColor = stBackColor;
    stTextConfig.enPosition = CSDLUtils::TEXT_POS_TOP_LEFT;
    stTextConfig.nOffsetX = 0;
    stTextConfig.nOffsetY = 0;

    if (render_text_block(pSurface, stTextConfig, ITC_FONT_FILE) != 0)
    {
        dlog_error("渲染文本失败");
        CSDLUtils::instance()->releaseSurface(pSurface);
        return;
    }

    /*
     * 时间 OSD 每秒都会刷新文本。
     * 未发生配置变更时，如果仍然在这里重复 changeRect，会触发底层 overlay 属性重建，
     * 进而放大时间区域的偶发闪动。因此时间元素只在扩容时才同步尺寸与位置；
     * 其他 overlay 保持原有行为，避免影响现有业务逻辑。
     */
    if (bNeedResize || stuOverplayInfo.stuOverplay.enElementType != Osd::ElementType_E::ELEMENT_TYPE_TIME)
    {
        /* 改变rgn尺寸 */
        pHandle->mppRgn_changeRect(pHandle, pHandle->unWidth, pHandle->unHeight);
        /* 改变rgn起始位置 */
        pHandle->mppRgn_changePos(pHandle, pHandle->unStartX, pHandle->unStartY);
    }

    /* 保存为BMP文件(用于调试) */
    // CSDLUtils::instance()->saveSurfaceToBMP(pSurface, "/opt/cam/bin/osd.bmp");

    /* 贴图(pitch:表示每一行像素数据占用的字节数, pitch * h:整个图像数据大小) */
    pHandle->mppRgn_loadPicture(pHandle, pSurface->pixels, pSurface->pitch * pSurface->h);
    /* 释放表面 */
    CSDLUtils::instance()->releaseSurface(pSurface);
}
#else
/* 用opencv库来画 */
void COverplayDraw::draw_text(HiRgn_S *pHandle, char *pText, int nLength)
{
    std::lock_guard<std::mutex> lock(m_mutex);

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
        return;
    }

    pHandle->mppRgn_overlay_loadPic(pHandle, pchOutData, nOutDataSize);

    if (NULL != pchOutData)
    {
        delete[] pchOutData;
        pchOutData = NULL;
    }

    dlog_debug("加载图像成功");
    return;
}
#endif

void COverplayDraw::draw_rect(HiRgn_S *pHandle,
                              Osd::OverplayInfo_S stuOverplayInfo,
                              const std::vector<Common::RectInfo_S> &vRectInfo)
{
    if (NULL == pHandle)
    {
        return;
    }

    /* 准备颜色 */
    CSDLUtils::Color_S stRectColor = { static_cast<uint8_t>((pHandle->unFgColor >> 16) & 0xFF),
                                       static_cast<uint8_t>((pHandle->unFgColor >> 8) & 0xFF),
                                       static_cast<uint8_t>(pHandle->unFgColor & 0xFF),
                                       static_cast<uint8_t>(pHandle->unFgAlpha) };

    /* 获取或创建 Surface */
    SDL_Surface *pSurface = nullptr;
    auto it = m_mapOverlaySurfaces.find(pHandle->unHandle);

    if (it == m_mapOverlaySurfaces.end())
    {
        pSurface = CSDLUtils::instance()->createSurface(pHandle->unWidth,
                                                        pHandle->unHeight,
                                                        SDL_PIXELFORMAT_ARGB4444,
                                                        nullptr /* 透明背景 */
        );

        if (!pSurface)
        {
            dlog_error("创建表面失败");
            return;
        }
        m_mapOverlaySurfaces[pHandle->unHandle] = pSurface;
    }
    else
    {
        pSurface = it->second;

        /* 检查尺寸是否变化 */
        if (pSurface->w != static_cast<int>(pHandle->unWidth) || pSurface->h != static_cast<int>(pHandle->unHeight))
        {
            CSDLUtils::instance()->releaseSurface(pSurface);
            pSurface = CSDLUtils::instance()->createSurface(pHandle->unWidth,
                                                            pHandle->unHeight,
                                                            SDL_PIXELFORMAT_ARGB4444,
                                                            nullptr);

            if (!pSurface)
            {
                m_mapOverlaySurfaces.erase(it);
                return;
            }
            m_mapOverlaySurfaces[pHandle->unHandle] = pSurface;
        }
        else
        {
            /* 清空表面 */
            Uint32 u32Transparent = SDL_MapRGBA(pSurface->format, 0, 0, 0, 0);
            SDL_FillRect(pSurface, nullptr, u32Transparent);
        }
    }

    /* 转换矩形格式 */
    std::vector<CSDLUtils::Rect_S> vRects;
    for (const auto &rect : vRectInfo)
    {
        CSDLUtils::Rect_S sdlRect;
        sdlRect.nX1 = rect.nX1;
        sdlRect.nY1 = rect.nY1;
        sdlRect.nX2 = rect.nX2;
        sdlRect.nY2 = rect.nY2;
        vRects.push_back(sdlRect);
    }

    /* 绘制矩形 */
    CSDLUtils::instance()->drawRects(pSurface, vRects, stRectColor, pHandle->unFontSize);

    /* 检查尺寸是否需要更新 */
    if (pSurface->w != static_cast<int>(pHandle->unWidth) || pSurface->h != static_cast<int>(pHandle->unHeight))
    {
        pHandle->mppRgn_update(pHandle,
                               set_rgn(stuOverplayInfo, pHandle->unChnId, pHandle->unHandle, pSurface->w, pSurface->h));
        pHandle->mppRgn_changeRect(pHandle, pHandle->unWidth, pHandle->unHeight);
        pHandle->mppRgn_changePos(pHandle, pHandle->unStartX, pHandle->unStartY);
    }

    /* 贴图 */
    pHandle->mppRgn_loadPicture(pHandle, pSurface->pixels, pSurface->pitch * pSurface->h);
}
