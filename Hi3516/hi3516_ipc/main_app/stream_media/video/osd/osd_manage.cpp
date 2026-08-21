/**
 * @FilePath     : osd_manage.cpp
 * @Author       : huangjunda
 * @Date         : 2025-07-23 11:26:05
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-28 10:45:04
 * @Description  : OSD管理定义
 */

#include "osd_manage.h"

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
bool normalize_cover_config(Osd::CoverConfig_S &stConfig)
{
    bool bChanged = false;
    if (stConfig.vecCoverAttr.size() > RGN_COVER_MAX_NUM)
    {
        stConfig.vecCoverAttr.resize(RGN_COVER_MAX_NUM);
        bChanged = true;
    }

    while (stConfig.vecCoverAttr.size() < RGN_COVER_MAX_NUM)
    {
        Osd::CoverAttribute_S stAttr;
        stAttr.clear();
        stAttr.nId = static_cast<int>(stConfig.vecCoverAttr.size()) + 1;
        stAttr.strName += std::to_string(stAttr.nId);
        stConfig.vecCoverAttr.push_back(stAttr);
        bChanged = true;
    }
    return bChanged;
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

COsdManage::COsdManage() : m_bInit(false),
                           m_strOsdConfigFile(OSD_CONFIG_FILE),
                           m_strCoverConfigFile(COVER_CONFIG_FILE),
                           m_strOverplayFile(OSD_OVERPLAY_CONFIG_FILE),
                           m_strCoverFile(OSD_COVER_CONFIG_FILE)
{
#if CAP_EXHIBITION_OSD_PANEL
    m_unPanelVersion = 0;
    m_unPanelUpdateTimeMs = 0;
#endif
}

COsdManage::~COsdManage()
{
}

int GetCalibratedTextWidth(const std::string& text, int fontSize) 
{

    float ratio = fontSize / 64.0f;
    float totalWidth = 15.0f; 

    size_t len = text.length();

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)text[i];
        float charPx = 0.0f;

        
        if (c == ' ') {
            charPx = 6.0f; // 空格
        }
        else if (c >= '0' && c <= '9') {
            // 数字：由数据推导，严格等于 11
            charPx = 11.0f; 
        }
        else if (c >= 'a' && c <= 'z') {
            // 小写字母
            if (c == 'i' || c == 'l') charPx = 6.0f;       // 极窄
            else if (c == 'j' || c == 't') charPx = 8.0f;
            else if (c == 'm') charPx = 15.0f;             // 宽 (m)
            else if (c == 'w') charPx = 14.0f;             // 宽 (w)
            else charPx = 10.5f;                           // 普通 (a, b, d, e, r...)
        }
        else if (c >= 'A' && c <= 'Z') {
            // 大写字母
            if (c == 'I') charPx = 6.0f;                   // 极窄
            else if (c == 'M' || c == 'W') charPx = 15.0f; // 宽
            else charPx = 11.5f;                           // 普通 (P, C, B...)
        }
        else if (c > 127) {
            // 中文 (UTF-8)
            // 假设中文比数字宽一倍左右
            charPx = 22.0f; 
            if (i + 2 < len) i += 2; // 跳过后续字节
        }
        else {
            // 标点符号
            if (c == '.' || c == ',' || c == ':' || c == ';') charPx = 6.0f;
            else charPx = 10.0f;
        }

        // 累加 (应用字号缩放)
        totalWidth += charPx * ratio;
    }

    return (int)std::ceil(totalWidth);
}
IpcRet_E COsdManage::init()
{
    if (Convert::read_file(m_strOsdConfigFile, m_stOsdConfig))
    {
        dlog_error("没有找到osd_config.json文件, 重新创建");
        m_stOsdConfig.clear();
        Convert::write_file(m_strOsdConfigFile, m_stOsdConfig);
    }
    
    if (Convert::read_file(m_strCoverConfigFile, m_stCoverConfig))
    {
        dlog_error("没有找到cover_config.json文件, 重新创建");
        m_stCoverConfig.bEnable = false;
        m_stCoverConfig.vecCoverAttr.clear();
        normalize_cover_config(m_stCoverConfig);
        Convert::write_file(m_strCoverConfigFile, m_stCoverConfig);
    }
    else if (normalize_cover_config(m_stCoverConfig))
    {
        /* 旧版本可能保存多个区域，启动时按本平台能力裁剪并持久化。 */
        Convert::write_file(m_strCoverConfigFile, m_stCoverConfig);
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
                stOverplayInfo.stuOverplay.nFontSize = 3;   /* 边框大小 */
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
    std::lock_guard<std::mutex> lock(m_mutex);
    stInfo = m_stOsdConfig;

    return OK;
}

IpcRet_E COsdManage::set_osd_config(Osd::OsdConfig_S stInfo)
{
    std::vector<Osd::OverplayInfo_S> vecInfo;
    vecInfo = m_vecOverplayInfo;
    stInfo.stOsdNameInfo.strName.reserve(100);
    for (size_t i = 0; i < vecInfo.size(); i++)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(vecInfo[i].stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_PEOPLE)
        {
            // note AI 动态分析专用 网页设置，避免影响 AI
            continue;
        }

        if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE)
        {
            /* 字符叠加元素在Osd::OsdConfig_S stInfo中的索引 */
            int nOsdInfoIndex = i - 2;
            /* 四个字符叠加在 overplay 的[2,5] 元素种类 enElementType 为 ELEMENT_TYPE_CUSTOMIZE */
            if (stInfo.vecOsdInfo.at(nOsdInfoIndex).strName.size() > OSD_NAME_LENGTH_LIMIT)
            {
                dlog_error("字符叠加 ID:[%d] 字符串名称:[%s] 参数错误",
                           stInfo.vecOsdInfo[nOsdInfoIndex].nId,
                           stInfo.vecOsdInfo[nOsdInfoIndex].strName.c_str());
                return ERR_PARAM;
            }

            if(stInfo.vecOsdInfo.at(nOsdInfoIndex).stOsdAttr.nW < 0)
            {
                int width = GetCalibratedTextWidth(stInfo.vecOsdInfo.at(nOsdInfoIndex).strName,64);
                dlog_debug("Custom osdInfo: str = %s,GetCalibratedTextWidth = %d",stInfo.vecOsdInfo.at(nOsdInfoIndex).strName.c_str(),width);
                stInfo.vecOsdInfo.at(nOsdInfoIndex).stOsdAttr.nW  = width;
            }

            vecInfo.at(i).stuInfo.bEnable = stInfo.vecOsdInfo.at(nOsdInfoIndex).bEnable;
            vecInfo.at(i).stuOverplay.strCustomize = stInfo.vecOsdInfo.at(nOsdInfoIndex).strName;
            if (OK != set_osd_attr(stInfo.vecOsdInfo.at(nOsdInfoIndex).stOsdAttr, vecInfo.at(i).stuOverplay))
            {
                return ERR;
            }
        }
        else if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_TIME)
        {
            /* 显示时间、日期 ID 7 */
            vecInfo.at(i).stuInfo.bEnable = stInfo.stOsdTimeInfo.bEnable;
            vecInfo.at(i).stuOverplay.bEnableWeek = stInfo.stOsdTimeInfo.bEnableWeek;
            
            switch (stInfo.stOsdTimeInfo.enTimeFormat)
            {
            case Osd::OSD_TIME_FORMAT_E::OSD_TIME_FORMAT_24:
                vecInfo.at(i).stuOverplay.bEnablePeriod = false;
                break;
            case Osd::OSD_TIME_FORMAT_E::OSD_TIME_FORMAT_12:
                vecInfo.at(i).stuOverplay.bEnablePeriod = true;
                break;
            default:
                dlog_error("Osd时间制式设置错误");
                return ERR;
            }
            if(stInfo.stOsdTimeInfo.stOsdAttr.nW < 0)
            {
                /* 小于0表示自动宽度，由运行时根据真实文本长度自适应 */
                stInfo.stOsdTimeInfo.stOsdAttr.nW = -1;
            }
            if (OK != set_osd_attr(stInfo.stOsdTimeInfo.stOsdAttr, vecInfo.at(i).stuOverplay))
            {
                return ERR;
            }
        }
        else if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_NAME)
        {
            /* 通道名称 ID 8 */
            if(stInfo.stOsdNameInfo.strName.size() > OSD_NAME_LENGTH_LIMIT)
            {
                dlog_error("通道名称:[%s] 参数错误", stInfo.vecOsdInfo[i].strName.c_str());
                return ERR_PARAM;
            }
            vecInfo.at(i).stuInfo.bEnable = stInfo.stOsdNameInfo.bEnable;
            vecInfo.at(i).stuInfo.strName = stInfo.stOsdNameInfo.strName;
            if(stInfo.stOsdNameInfo.stOsdAttr.nW < 0)
            {
                int width = GetCalibratedTextWidth(stInfo.stOsdNameInfo.strName,64);
                dlog_debug("Name osdInfo: str = %s,GetCalibratedTextWidth = %d",stInfo.stOsdNameInfo.strName.c_str(),width);
                stInfo.stOsdNameInfo.stOsdAttr.nW = width;
            }
            if (OK != set_osd_attr(stInfo.stOsdNameInfo.stOsdAttr, vecInfo.at(i).stuOverplay))
            {
                return ERR;
            }
        }
    }
    
    if (Convert::write_file(m_strOsdConfigFile, stInfo))
    {
        dlog_error("写入osd_config.json文件失败");
        return ERR;
    }
    m_stOsdConfig.clear();
    m_stOsdConfig = stInfo;

    set_overplay_info(vecInfo);

    return OK;
}

IpcRet_E COsdManage::set_osd_attr(Osd::OsdAttribute_S stOsdAttr, Osd::Overplay_S &stOverplay)
{
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
    default:
        dlog_error("Osd字体颜色设置错误");
        return ERR;
    }

    return OK;
}

IpcRet_E COsdManage::get_overplay_info(std::vector<Osd::OverplayInfo_S> &vecInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    vecInfo = m_vecOverplayInfo;

    return OK;
}

IpcRet_E COsdManage::set_overplay_info(std::vector<Osd::OverplayInfo_S> &vecInfo)
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

IpcRet_E COsdManage::get_cover_config(Osd::CoverConfig_S &stInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    stInfo = m_stCoverConfig;
    normalize_cover_config(stInfo);
    return OK;
}

std::size_t COsdManage::get_cover_max_area_count() const
{
    return RGN_COVER_MAX_NUM;
}

IpcRet_E COsdManage::set_cover_config(Osd::CoverConfig_S stInfo)
{
    if (stInfo.vecCoverAttr.size() > get_cover_max_area_count())
    {
        dlog_warn("隐私遮盖区域数超出平台能力, request:%zu, max:%zu",
                  stInfo.vecCoverAttr.size(), get_cover_max_area_count());
        return ERR_PARAM;
    }

    /* 禁用时允许省略区域数组，内部补齐为固定的单区域配置。 */
    normalize_cover_config(stInfo);

    std::vector<Osd::CoverInfo_S> vecInfo;
    vecInfo = m_vecCoverInfo;
    for (size_t i = 0; vecInfo.size() > i; i++)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!stInfo.bEnable)
        {
            vecInfo.at(i).stuInfo.bEnable = stInfo.bEnable;
        }
        else
        {
            vecInfo.at(i).stuInfo.bEnable = stInfo.vecCoverAttr.at(i).bEnable;
        }

        vecInfo.at(i).stuInfo.strName = stInfo.vecCoverAttr.at(i).strName;

        switch (stInfo.vecCoverAttr.at(i).enColor)
        {
        case Osd::OSD_COLOR_E::OSD_COLOR_BLACK:
            vecInfo.at(i).stuCover.strBackColor = "0x000000";
            break;
        case Osd::OSD_COLOR_E::OSD_COLOR_WHITE:
            vecInfo.at(i).stuCover.strBackColor = "0xFFFFFF";
            break;
        case Osd::OSD_COLOR_E::OSD_COLOR_CUSTOMIZE:
            vecInfo.at(i).stuCover.strBackColor = stInfo.vecCoverAttr.at(i).strColor;
            if (vecInfo.at(i).stuCover.strBackColor.size() > 0 && vecInfo.at(i).stuCover.strBackColor[0] == '#')
            {
                vecInfo.at(i).stuCover.strBackColor.replace(0, 1, "0x"); // 替换 '#' 为 '0x'
            }
            break;
        default:
            dlog_error("Cover颜色设置错误");
            return ERR;
        }
        
        vecInfo.at(i).stuCover.stuCoordinate.at(Osd::POS_START).nX = stInfo.vecCoverAttr.at(i).nX;
        vecInfo.at(i).stuCover.stuCoordinate.at(Osd::POS_START).nY = stInfo.vecCoverAttr.at(i).nY;
        vecInfo.at(i).stuCover.stuCoordinate.at(Osd::POS_END).nX = stInfo.vecCoverAttr.at(i).nX + stInfo.vecCoverAttr.at(i).nWidth;
        vecInfo.at(i).stuCover.stuCoordinate.at(Osd::POS_END).nY = stInfo.vecCoverAttr.at(i).nY + stInfo.vecCoverAttr.at(i).nHeight;
    }
    
    if (Convert::write_file(m_strCoverConfigFile, stInfo))
    {
        dlog_error("写入cover_config.json文件失败");
        return ERR;
    }
    m_stCoverConfig.clear();
    m_stCoverConfig = stInfo;

    set_cover_info(vecInfo);

    return OK;
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
    std::lock_guard<std::mutex> lock(m_mutex);
    System::DeviceConfig_S stDeviceConfig;
    SystemManage::instance()->get_device_config(stDeviceConfig);
    
    m_stuShareInfo.stuTimeInfo.strZone = CTimeManage::instance()->get_current_zone(stDeviceConfig.enTimeZone);
    
    switch (m_stOsdConfig.stOsdTimeInfo.enDateFormat)
    {
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYY_MM_DD:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH, System::DateFormat_E::YYYY_MM_DD);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH, System::DateFormat_E::YYYY_MM_DD);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_MM_DD_YYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH, System::DateFormat_E::MM_DD_YYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH, System::DateFormat_E::MM_DD_YYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_DD_MM_YYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH, System::DateFormat_E::DD_MM_YYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH, System::DateFormat_E::DD_MM_YYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::CHINESE_YYYYMMDD:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::SIMP_CHINESE);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::SIMP_CHINESE, System::DateFormat_E::YYYYMMDD);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::SIMP_CHINESE, System::DateFormat_E::YYYYMMDD);
        break;
    case Osd::OSD_DATE_FORMAT_E::CHINESE_MMDDYYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::SIMP_CHINESE);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::SIMP_CHINESE, System::DateFormat_E::MMDDYYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::SIMP_CHINESE, System::DateFormat_E::MMDDYYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::CHINESE_DDMMYYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::SIMP_CHINESE);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::SIMP_CHINESE, System::DateFormat_E::DDMMYYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::SIMP_CHINESE, System::DateFormat_E::DDMMYYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYYMMDD:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH, System::DateFormat_E::YYYYMMDD);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH, System::DateFormat_E::YYYYMMDD);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_MMDDYYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH, System::DateFormat_E::MMDDYYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH, System::DateFormat_E::MMDDYYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_DDMMYYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH, System::DateFormat_E::DDMMYYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH, System::DateFormat_E::DDMMYYYY);
        break;
    
    default:
        break;
    }

    stuShareInfo = m_stuShareInfo;

    return OK;
}

IpcRet_E COsdManage::set_osd_share_info(System::DeviceConfig_S stDeviceConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    Network::Info_S stNetInfo;
    CNetworkManage::instance()->get_system_networkInfo(stNetInfo);
    m_stuShareInfo.strIp = stNetInfo.stIp.ipv4Ip;

    if (System::Language_E::SIMP_CHINESE == stDeviceConfig.enLanguage)
    {
        m_stuShareInfo.strPeople = CN_PEOPLE_TIPS;
        m_stuShareInfo.strMac = CN_MAC_TIPS;
        m_stuShareInfo.strPreset = CN_PRESET_TIPS;
    }
    else if (System::Language_E::ENGLISH == stDeviceConfig.enLanguage)
    {
        m_stuShareInfo.strPeople = EN_PEOPLE_TIPS;
        m_stuShareInfo.strMac = EN_MAC_TIPS;
        m_stuShareInfo.strPreset = EN_PRESET_TIPS;
    }
    m_stuShareInfo.stuTimeInfo.strZone = CTimeManage::instance()->get_current_zone(stDeviceConfig.enTimeZone);
    m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(stDeviceConfig.enLanguage);
    m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(stDeviceConfig.enLanguage, stDeviceConfig.enDateFormat);
    m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(stDeviceConfig.enLanguage, stDeviceConfig.enDateFormat);
    COverplayDraw::instance()->set_update_flag(true);

    return OK;
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
    if(!m_bInit)
    {
        return ERR_UNINIT;
    }

    /* 更新 */
    // COverplayDraw::instance()->update_ai_result(nWidth, nHeight, vstRectInfo, m_vecOverplayInfo[Osd::ElementType_E::ELEMENT_TYPE_PEOPLE]);
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
IpcRet_E COsdManage::get_panel_result(OsdPanel::PanelFrame_S &stPanelFrame,
                                      uint64_t &unVersion,
                                      uint64_t &unUpdateTimeMs)
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
