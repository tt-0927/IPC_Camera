/**
 * @FilePath     : osd_manage.cpp
 * @Author       : huangjunda
 * @Date         : 2025-07-23 11:26:05
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-30 15:14:08
 * @Description  : OSD管理定义
 */

#include "osd_manage.h"

#include <cmath>
#include "time_manage.h"
#include "network_manage.h"
#include "path_define.h"
#include "share_data.h"
#include "overplay_draw.h"
#include "cover_draw.h"
#include "corner_rect_draw.h"
#include "convert_interface.h"
#include "dlog.h"
#include <chrono>

namespace
{
/**
 * @brief   : 估算文本渲染宽度（用于OSD宽度自适应）
 * @param    {const std::string&} text：文本内容
 * @param    {int} fontSize：字体大小（基准64）
 * @return   {int} 估算宽度（像素）
 * @note    : 按字符类别经验值累加，仅用于 nW<0 时的自适应兜底
 */
int GetCalibratedTextWidth(const std::string &text, int fontSize)
{
    float ratio = fontSize / 64.0f;
    float totalWidth = 15.0f;

    size_t len = text.length();

    for (size_t i = 0; i < len; ++i)
    {
        unsigned char c = (unsigned char) text[i];
        float charPx = 0.0f;

        if (c == ' ')
        {
            charPx = 6.0f; // 空格
        }
        else if (c >= '0' && c <= '9')
        {
            // 数字：由数据推导，严格等于 11
            charPx = 11.0f;
        }
        else if (c >= 'a' && c <= 'z')
        {
            // 小写字母
            if (c == 'i' || c == 'l')
            {
                charPx = 6.0f; // 极窄
            }
            else if (c == 'j' || c == 't')
            {
                charPx = 8.0f;
            }
            else if (c == 'm')
            {
                charPx = 15.0f; // 宽 (m)
            }
            else if (c == 'w')
            {
                charPx = 14.0f; // 宽 (w)
            }
            else
            {
                charPx = 10.5f; // 普通 (a, b, d, e, r...)
            }
        }
        else if (c >= 'A' && c <= 'Z')
        {
            // 大写字母
            if (c == 'I')
            {
                charPx = 6.0f; // 极窄
            }
            else if (c == 'M' || c == 'W')
            {
                charPx = 15.0f; // 宽
            }
            else
            {
                charPx = 11.5f; // 普通 (P, C, B...)
            }
        }
        else if (c > 127)
        {
            // 中文 (UTF-8)
            // 假设中文比数字宽一倍左右
            charPx = 22.0f;
            if (i + 2 < len)
            {
                i += 2; // 跳过后续字节
            }
        }
        else
        {
            // 标点符号
            if (c == '.' || c == ',' || c == ':' || c == ';')
            {
                charPx = 6.0f;
            }
            else
            {
                charPx = 10.0f;
            }
        }

        // 累加 (应用字号缩放)
        totalWidth += charPx * ratio;
    }

    return (int) std::ceil(totalWidth);
}

bool normalize_cover_info(std::vector<Osd::CoverInfo_S> &vecCoverInfo)
{
    bool bChanged = false;
    if (vecCoverInfo.size() > RGN_COVER_MAX_NUM)
    {
        vecCoverInfo.resize(RGN_COVER_MAX_NUM);
        bChanged = true;
    }

    while (vecCoverInfo.size() < RGN_COVER_MAX_NUM)
    {
        Osd::CoverInfo_S stCoverInfo;
        stCoverInfo.clear();
        stCoverInfo.stuInfo.nID = static_cast<int>(vecCoverInfo.size()) + 1;
        vecCoverInfo.push_back(stCoverInfo);
        bChanged = true;
    }
    return bChanged;
}

#if CAP_EXHIBITION_OSD_PANEL
/**
 * @brief   : 获取稳态时钟的毫秒时间戳
 * @return   {uint64_t} 当前毫秒时间戳
 * @note    : 使用 steady_clock 避免系统时间跳变影响面板超时判断
 */
uint64_t get_steady_time_ms()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}
#endif
} // namespace

COsdManage::COsdManage() : m_bInit(false), m_strOverplayFile(OSD_OVERPLAY_CONFIG_FILE), m_strCoverFile(OSD_COVER_CONFIG_FILE)
{
#if CAP_EXHIBITION_OSD_PANEL
    m_unPanelVersion = 0;
    m_unPanelUpdateTimeMs = 0;
#endif
}

COsdManage::~COsdManage()
{
}

IpcRet_E COsdManage::init()
{
    /* 注册配置应用接口后初始化共享层OSD/Cover配置（含区域数量按平台能力收敛） */
    if (OK != COsdConfigure::instance()->setOsdConfigApplier(this))
    {
        dlog_error("注册OSD配置应用接口失败");
        return ERR;
    }

    if (OK != COsdConfigure::instance()->init())
    {
        dlog_error("初始化OSD配置失败");
        return ERR;
    }

    if (Convert::read_file(m_strOverplayFile, m_vecOverplayInfo))
    {
        dlog_error("没有找到overplay.json文件, 重新创建");
        m_vecOverplayInfo.clear();

        for (int i = 0; i < OT_RGN_VENC_MAX_OVERLAY_NUM; i++)
        {
            Osd::OverplayInfo_S stOverplayInfo;
            stOverplayInfo.clear();
            stOverplayInfo.stuInfo.nID = i + 1;

            /* 加入剩下三个给不可字符叠加的名称、时间和人数的overplay信息 */
            if (i <= Osd::ElementType_E::ELEMENT_TYPE_PEOPLE)
            {
                // note AI 动态分析专用 暂不使用，改为角框
                // stOverplayInfo.stuInfo.bEnable = true;
                stOverplayInfo.stuInfo.strName = "People";
                stOverplayInfo.stuOverplay.bEnableFlicker = true;
                stOverplayInfo.stuOverplay.enElementType = Osd::ElementType_E::ELEMENT_TYPE_PEOPLE;
                stOverplayInfo.stuOverplay.nFontSize = 3;             /* 边框大小 */
                stOverplayInfo.stuOverplay.strFontColor = "0x00ff00"; /* 绿色方框 */
            }
            else if (i == Osd::ElementType_E::ELEMENT_TYPE_TIME)
            {
                stOverplayInfo.stuInfo.strName = "Time";
                stOverplayInfo.stuOverplay.enElementType = Osd::ElementType_E::ELEMENT_TYPE_TIME;
            }
            else if (i == Osd::ElementType_E::ELEMENT_TYPE_NAME)
            {
                stOverplayInfo.stuInfo.strName = "Name";
                stOverplayInfo.stuOverplay.enElementType = Osd::ElementType_E::ELEMENT_TYPE_NAME;
            }
            m_vecOverplayInfo.push_back(stOverplayInfo);
        }
        Convert::write_file(m_strOverplayFile, m_vecOverplayInfo);
    }

    /* 旧版本配置无 FontColorType 字段，反序列化遗留未初始化值，按颜色串回落合法枚举 */
    for (auto &info : m_vecOverplayInfo)
    {
        if (info.stuOverplay.enFontColor < Osd::OSD_COLOR_BLACK || info.stuOverplay.enFontColor > Osd::OSD_COLOR_AUTO_BLACK_WHITE)
        {
            info.stuOverplay.enFontColor = ("0xFFFFFF" == info.stuOverplay.strFontColor) ? Osd::OSD_COLOR_WHITE : Osd::OSD_COLOR_BLACK;
        }
    }

    if (Convert::read_file(m_strCoverFile, m_vecCoverInfo))
    {
        dlog_error("没有找到cover.json文件, 重新创建");
        m_vecCoverInfo.clear();
        normalize_cover_info(m_vecCoverInfo);
        Convert::write_file(m_strCoverFile, m_vecCoverInfo);
    }
    else if (normalize_cover_info(m_vecCoverInfo))
    {
        /* 保证绘制模块与配置模块使用同一数量的区域。 */
        Convert::write_file(m_strCoverFile, m_vecCoverInfo);
    }

    System::DeviceConfig_S stDeviceConfig;
    SystemManage::instance()->get_device_config(stDeviceConfig);
    set_osd_share_info(stDeviceConfig);

    /* JPEG图片编码通道,用于人脸抓拍叠加信息 */
    for (int i = 0; i < OT_RGN_VENC_MAX_OVERLAY_NUM; i++)
    {
        Osd::OverplayInfo_S stOverplayInfo;
        stOverplayInfo.clear();
        /* 从9开始 */
        stOverplayInfo.stuInfo.nID = i + 1;
        stOverplayInfo.stuOverplay.bEnableFlicker = true;
        stOverplayInfo.stuOverplay.nVerMargin += RGN_INFO_VER_MARGIN * i;
        stOverplayInfo.stuOverplay.enElementType = Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE;
        /* 最后设置为时间 */
        if (i == RGN_CAPTURE_TIME_HANDLE)
        {
            /* 设置在左下角 */
            stOverplayInfo.stuOverplay.nVerMargin = RGN_CAPTURE_TIME_VER_MARGIN;
            stOverplayInfo.stuOverplay.enElementType = Osd::ElementType_E::ELEMENT_TYPE_TIME;
        }
        m_vecOverplayCaptureInfo.emplace_back(stOverplayInfo);
    }

    if (OK == COverplayDraw::instance()->init() && OK == CCornerRectDraw::instance()->init() && OK == CCoverDraw::instance()->init())
    {
        m_bInit.store(true);
    }

    return OK;
}

IpcRet_E COsdManage::deinit()
{
    m_bInit.store(false);

    /* 清理共享层配置应用接口，避免悬挂引用 */
    COsdConfigure::instance()->clearOsdConfigApplier(this);

    if (CCoverDraw::instance()->deinit())
    {
        return ERR;
    }

    if (CCornerRectDraw::instance()->deinit())
    {
        return ERR;
    }

    if (COverplayDraw::instance()->deinit())
    {
        return ERR;
    }

    return OK;
}

IpcRet_E COsdManage::get_osd_config(Osd::OsdConfig_S &stInfo)
{
    return COsdConfigure::instance()->get_osd_config(stInfo);
}

IpcRet_E COsdManage::set_osd_config(Osd::OsdConfig_S stInfo)
{
    return COsdConfigure::instance()->set_osd_config(stInfo);
}

IpcRet_E COsdManage::get_overplay_info(std::vector<Osd::OverplayInfo_S> &vecInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    vecInfo = m_vecOverplayInfo;

    return OK;
}

IpcRet_E COsdManage::set_overplay_info(std::vector<Osd::OverplayInfo_S> vecInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (Convert::write_file(m_strOverplayFile, vecInfo))
    {
        dlog_error("写入osd_overplay.json文件失败");
        return ERR;
    }
    m_vecOverplayInfo.clear();
    m_vecOverplayInfo = vecInfo;
    COverplayDraw::instance()->set_update_flag(true);

    return OK;
}

IpcRet_E COsdManage::apply_overplay_info(const std::vector<Osd::OverplayInfo_S> &vecInfo)
{
    return set_overplay_info(vecInfo);
}

IpcRet_E COsdManage::get_cover_config(Osd::CoverConfig_S &stInfo)
{
    return COsdConfigure::instance()->get_cover_config(stInfo);
}

std::size_t COsdManage::get_cover_max_area_count() const
{
    return RGN_COVER_MAX_NUM;
}

IpcRet_E COsdManage::set_cover_config(Osd::CoverConfig_S stInfo)
{
    return COsdConfigure::instance()->set_cover_config(stInfo);
}

IpcRet_E COsdManage::get_cover_info(std::vector<Osd::CoverInfo_S> &vecInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    vecInfo = m_vecCoverInfo;

    return OK;
}

IpcRet_E COsdManage::set_cover_info(std::vector<Osd::CoverInfo_S> vecInfo, bool bIsWriteFile)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (bIsWriteFile)
    {
        if (Convert::write_file(m_strCoverFile, vecInfo))
        {
            dlog_error("写入osd_cover.json文件失败");
            return ERR;
        }
    }
    m_vecCoverInfo.clear();
    m_vecCoverInfo = vecInfo;
    CCoverDraw::instance()->set_update_flag(true);

    return OK;
}

IpcRet_E COsdManage::apply_cover_info(const std::vector<Osd::CoverInfo_S> &vecInfo)
{
    return set_cover_info(vecInfo);
}

// IpcRet_E COsdManage::set_ai_cover_info(std::vector<Osd::CoverInfo_S> vecInfo)
// {
//     OS_mutexLock(&m_stuMutex);
//     m_vecCoverInfo.clear();
//     m_vecCoverInfo = vecInfo;
//     OS_mutexUnlock(&m_stuMutex);

//     CCoverDraw::instance()->set_update_flag(true);

//     return OK;
// }

IpcRet_E COsdManage::get_osd_share_info(Osd::ShareInfo_S &stuShareInfo)
{
    return COsdConfigure::instance()->get_osd_share_info(stuShareInfo);
}

IpcRet_E COsdManage::set_osd_share_info(System::DeviceConfig_S stDeviceConfig)
{
    return COsdConfigure::instance()->set_osd_share_info(stDeviceConfig);
}

IpcRet_E COsdManage::adapt_osd_attr(Osd::ElementType_E enType,
                                    const std::string &strText,
                                    Osd::OsdAttribute_S &stOsdAttr,
                                    Osd::Overplay_S &stOverplay)
{
    /* nW<0 表示宽度自适应：字符叠加/通道名称按文本估算宽度，时间由运行时自适应 */
    if (enType == Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE || enType == Osd::ElementType_E::ELEMENT_TYPE_NAME)
    {
        if (stOsdAttr.nW < 0)
        {
            int width = GetCalibratedTextWidth(strText, 64);
            dlog_debug("OsdType:%d str = %s,GetCalibratedTextWidth = %d", enType, strText.c_str(), width);
            stOsdAttr.nW = width;
        }
    }
    else if (enType == Osd::ElementType_E::ELEMENT_TYPE_TIME)
    {
        if (stOsdAttr.nW < 0)
        {
            /* 小于0表示自动宽度，由运行时根据真实文本长度自适应 */
            stOsdAttr.nW = -1;
        }
    }

    stOverplay.nHorMargin = stOsdAttr.nX;
    stOverplay.nVerMargin = stOsdAttr.nY;

    stOverplay.nWidth = stOsdAttr.nW;
    stOverplay.nHeight = stOsdAttr.nH;

    switch (stOsdAttr.enAttribute)
    {
    case Osd::OSD_ATTRIBUTE_E::OSD_ATTR_ALPHA_N_FLASH_N:
        stOverplay.nFontAlpha = 0;
        stOverplay.bEnableFlicker = false;
        break;
    case Osd::OSD_ATTRIBUTE_E::OSD_ATTR_ALPHA_N_FLASH_Y:
        stOverplay.nFontAlpha = 0;
        stOverplay.bEnableFlicker = true;
        break;
    case Osd::OSD_ATTRIBUTE_E::OSD_ATTR_ALPHA_Y_FLASH_N:
        stOverplay.nFontAlpha = 50; /* 半透明 */
        stOverplay.bEnableFlicker = false;
        break;
    case Osd::OSD_ATTRIBUTE_E::OSD_ATTR_ALPHA_Y_FLASH_Y:
        stOverplay.nFontAlpha = 50; /* 半透明 */
        stOverplay.bEnableFlicker = true;
        break;
    default:
        dlog_error("Osd属性设置错误");
        return ERR;
    }

    switch (stOsdAttr.enFontSize)
    {
    case Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_16:
        stOverplay.nFontSize = 16;
        break;
    case Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_32:
        stOverplay.nFontSize = 32;
        break;
    case Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_48:
        stOverplay.nFontSize = 48;
        break;
    case Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_ADAPTIVE:
    case Osd::OSD_FONT_SIZE_E::OSD_FONT_SIZE_64:
        stOverplay.nFontSize = 64;
        break;
    default:
        dlog_error("Osd字体大小设置错误");
        return ERR;
    }

    /* 颜色枚举透传给渲染层（"黑白自动"按枚举判断反色），颜色串按枚举映射（渲染/回退用） */
    stOverplay.enFontColor = stOsdAttr.enFontColor;
    switch (stOsdAttr.enFontColor)
    {
    case Osd::OSD_COLOR_E::OSD_COLOR_BLACK:
        stOverplay.strFontColor = "0x000000";
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_WHITE:
        stOverplay.strFontColor = "0xFFFFFF";
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_CUSTOMIZE:
        stOverplay.strFontColor = stOsdAttr.strFontColor;
        if (stOverplay.strFontColor.size() > 0 && stOverplay.strFontColor[0] == '#')
        {
            stOverplay.strFontColor.replace(0, 1, "0x"); // 替换 '#' 为 '0x'
        }
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_AUTO_BLACK_WHITE:
        /* 黑白自动：渲染层逐字符按背景亮度选黑/白，颜色串仅作反色不可用时的回退 */
        stOverplay.strFontColor = "0xFFFFFF";
        break;
    default:
        dlog_error("Osd字体颜色设置错误");
        return ERR;
    }

    return OK;
}

IpcRet_E COsdManage::cover_attr_to_info(const Osd::CoverAttribute_S &stAttr, Osd::CoverInfo_S &stInfo)
{
    switch (stAttr.enColor)
    {
    case Osd::OSD_COLOR_E::OSD_COLOR_BLACK:
        stInfo.stuCover.strBackColor = "0x000000";
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_WHITE:
        stInfo.stuCover.strBackColor = "0xFFFFFF";
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_CUSTOMIZE:
        stInfo.stuCover.strBackColor = stAttr.strColor;
        if (stInfo.stuCover.strBackColor.size() > 0 && stInfo.stuCover.strBackColor[0] == '#')
        {
            stInfo.stuCover.strBackColor.replace(0, 1, "0x"); // 替换 '#' 为 '0x'
        }
        break;
    default:
        dlog_error("Cover颜色设置错误");
        return ERR;
    }

    stInfo.stuCover.stuCoordinate.at(Osd::POS_START).nX = stAttr.nX;
    stInfo.stuCover.stuCoordinate.at(Osd::POS_START).nY = stAttr.nY;
    stInfo.stuCover.stuCoordinate.at(Osd::POS_END).nX = stAttr.nX + stAttr.nWidth;
    stInfo.stuCover.stuCoordinate.at(Osd::POS_END).nY = stAttr.nY + stAttr.nHeight;

    return OK;
}

void COsdManage::refresh_overplay()
{
    COverplayDraw::instance()->set_update_flag(true);
}

IpcRet_E COsdManage::get_overplay_capture_info(std::vector<Osd::OverplayInfo_S> &vecInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    vecInfo = m_vecOverplayCaptureInfo;

    return OK;
}

IpcRet_E COsdManage::set_overplay_capture_info(std::vector<Osd::OverplayInfo_S> &vecInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_vecOverplayCaptureInfo.clear();
    m_vecOverplayCaptureInfo = vecInfo;

    return OK;
}

IpcRet_E COsdManage::send_detection_result(const int nWidth, const int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo)
{
    if (!m_bInit)
    {
        return ERR_UNINIT;
    }

    /* 更新 */
    // COverplayDraw::instance()->update_ai_result(nWidth, nHeight, vstRectInfo,
    // m_vecOverplayInfo[Osd::ElementType_E::ELEMENT_TYPE_PEOPLE]);
    CCornerRectDraw::instance()->update_ai_result(nWidth, nHeight, vstRectInfo);
    return OK;
}

#if CAP_EXHIBITION_OSD_PANEL
IpcRet_E COsdManage::send_panel_result(const OsdPanel::PanelFrame_S &stPanelFrame)
{
    if (!m_bInit)
    {
        return ERR_UNINIT;
    }

    /* 先在锁外归一化，缩短临界区占用时间 */
    OsdPanel::PanelFrame_S stNormalizedFrame = stPanelFrame;
    stNormalizedFrame.normalize();

    /* 加锁保护展会面板缓存，避免算法线程和渲染线程并发读写 */
    std::lock_guard<std::mutex> lock(m_mutex);
    /* 当前发送的是空面板帧时，不立即清缓存，交给超时机制自然熄灭 */
    if (stNormalizedFrame.empty())
    {
        return OK;
    }

    /* 内容一致时只刷新存活时间，避免无意义版本抖动 */
    if (m_stPanelFrame == stNormalizedFrame)
    {
        m_unPanelUpdateTimeMs = get_steady_time_ms();
        return OK;
    }

    m_stPanelFrame = stNormalizedFrame;
    m_unPanelUpdateTimeMs = get_steady_time_ms();
    ++m_unPanelVersion;
    return OK;
}

/**
 * @brief   : 获取展会面板缓存
 * @param    {OsdPanel::PanelFrame_S &} stPanelFrame：输出的面板结果
 * @param    {uint64_t &} unVersion：输出的缓存版本号
 * @param    {uint64_t &} unUpdateTimeMs：输出的更新时间戳
 * @return   {IpcRet_E} 0：成功 小于零：失败
 */
IpcRet_E COsdManage::get_panel_result(OsdPanel::PanelFrame_S &stPanelFrame, uint64_t &unVersion, uint64_t &unUpdateTimeMs)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    stPanelFrame = m_stPanelFrame;
    unVersion = m_unPanelVersion;
    unUpdateTimeMs = m_unPanelUpdateTimeMs;
    return OK;
}
#endif

void COsdManage::update_osd_flag()
{
    COverplayDraw::instance()->set_update_flag(true);
    CCoverDraw::instance()->set_update_flag(true);
}

void COsdManage::before_venc_channel_reset(int nChn)
{
    if (!m_bInit)
    {
        return;
    }

    COverplayDraw::instance()->detach_venc_channel(nChn);
}

void COsdManage::after_venc_channel_reset(int nChn)
{
    if (!m_bInit)
    {
        return;
    }

    COverplayDraw::instance()->resume_venc_channel(nChn);
    /* 旧 AI 框属于切换前几何，必须在下一帧检测结果到达前隐藏。 */
    CCornerRectDraw::instance()->clear_channel(nChn);
    update_osd_flag();
}
