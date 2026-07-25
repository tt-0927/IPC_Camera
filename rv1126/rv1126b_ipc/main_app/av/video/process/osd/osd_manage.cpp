/*** 
 * @FilePath     : osd_manage.cpp
 * @Author       : huangjunda
 * @Date         : 2025-07-23 11:26:05
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-08-27 09:21:50
 * @Description  : OSD管理定义
 */

#include "osd_manage.h"

#include "time_manage.h"
#include "network_manage.h"
#include "path_define.h"
#include "share_define.h"
#include "overlay_draw.h"
#include "cover_draw.h"
#include "convert_interface.h"
#include "dlog.h"

#include <chrono>

namespace
{
#if CAP_EXHIBITION_OSD_PANEL
/* 展会面板专用 overlay 槽位下标。 */
constexpr size_t EXHIBITION_PANEL_OVERPLAY_INDEX =  Osd::ElementType_E::ELEMENT_TYPE_MAC; //(ELEMENT_TYPE_MAC暂未使用，借用)

/**
 * @brief   : 预留展会面板专用 overlay 槽位
 * @param    {std::vector<Osd::OverplayInfo_S> &} vecInfo：overlay 配置列表
 * @return   {void}
 */
void reserve_exhibition_panel_overlay(std::vector<Osd::OverplayInfo_S> &vecInfo)
{
    if (vecInfo.size() > MAX_OSD_OVERLAY_NUM)
    {
        vecInfo.resize(MAX_OSD_OVERLAY_NUM);
    }
    if (vecInfo.size() < MAX_OSD_OVERLAY_NUM)
    {
        vecInfo.resize(MAX_OSD_OVERLAY_NUM);
        for (size_t i = 0; i < vecInfo.size(); ++i)
        {
            if (vecInfo[i].stuInfo.nID <= 0)
            {
                vecInfo[i].clear();
                vecInfo[i].stuInfo.nID = static_cast<int>(i) + 1;
            }
        }
    }

    /* 当前固定保留给展会面板的 overlay 配置项。 */
    Osd::OverplayInfo_S &stPanelInfo = vecInfo.at(EXHIBITION_PANEL_OVERPLAY_INDEX);
    stPanelInfo.clear();
    stPanelInfo.stuInfo.nID = static_cast<int>(EXHIBITION_PANEL_OVERPLAY_INDEX) + 1;
    stPanelInfo.stuInfo.bEnable = false;
    stPanelInfo.stuInfo.strName = "ExhibitionPanel";
    stPanelInfo.stuOverplay.enElementType = Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE;
    stPanelInfo.stuOverplay.strCustomize.clear();
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
#endif
} // namespace

COsdManage::COsdManage() : m_bInit(false),
                           m_strOsdConfigFile(OSD_CONFIG_FILE),
                           m_strCoverConfigFile(COVER_CONFIG_FILE),
                           m_strOverplayFile(OSD_OVERPLAY_CONFIG_FILE),
                           m_strCoverFile(OSD_COVER_CONFIG_FILE)
{
}

COsdManage::~COsdManage()
{
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
        m_stCoverConfig.clear();
        Convert::write_file(m_strCoverConfigFile, m_stCoverConfig);
    }

    if (Convert::read_file(m_strOverplayFile, m_vecOverplayInfo))
    {
        dlog_error("没有找到overplay.json文件, 重新创建");
        m_vecOverplayInfo.clear();
        
        for (int i = 0; i < MAX_OSD_OVERLAY_NUM; i++)
        {
            Osd::OverplayInfo_S stOverplayInfo;
            stOverplayInfo.clear();
            stOverplayInfo.stuInfo.nID = i + 1;

            /* 加入剩下三个给不可字符叠加的名称、时间和人数的overplay信息 */
            if (i == Osd::ElementType_E::ELEMENT_TYPE_PEOPLE)
            {
                // note AI 动态分析专用
                stOverplayInfo.stuInfo.bEnable = true;
                stOverplayInfo.stuInfo.strName = "People";
                stOverplayInfo.stuOverplay.bEnableFlicker = true;
                stOverplayInfo.stuOverplay.enElementType = Osd::ElementType_E::ELEMENT_TYPE_PEOPLE;
                /*边框大小修改无效，已自适应分辨率分配边框大小*/
                //stOverplayInfo.stuOverplay.nFontSize = 4;   /* 边框大小 */
                /*仅支持 0x00ff00 绿色和 0xff0000 红色二种颜色*/
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

#if CAP_EXHIBITION_OSD_PANEL
    reserve_exhibition_panel_overlay(m_vecOverplayInfo);
#endif

    if (Convert::read_file(m_strCoverFile, m_vecCoverInfo))
    {
        dlog_error("没有找到cover.json文件, 重新创建");
        Osd::CoverInfo_S stCoverInfo;
        stCoverInfo.clear();
        for (size_t i = 0; i < MAX_OSD_COVER_NUM; i++)
        {
            stCoverInfo.stuInfo.nID = i + 1;
            m_vecCoverInfo.push_back(stCoverInfo);
        }
        Convert::write_file(m_strCoverFile, m_vecCoverInfo);
    }

    /* 创建锁 */
    OS_mutexCreate(&m_stuMutex);

    System::DeviceConfig_S stDeviceConfig;
    SystemManage::instance()->get_device_config(stDeviceConfig);
    set_osd_share_info(stDeviceConfig);

     if (OK == COverlayDraw::instance()->init() && OK == CCoverDraw::instance()->init())
     {
         m_bInit.store(true);
     }

    return OK;
}

IpcRet_E COsdManage::deinit()
{
    m_bInit.store(false);

    /* 删除锁 */
    OS_mutexDelete(&m_stuMutex);

    if (COverlayDraw::instance()->deinit())
    {
        return ERR;
    }

    if (CCoverDraw::instance()->deinit())
    {
        return ERR;
    }

    m_stOsdConfig.clear();
    m_stOsdConfig.vecOsdInfo.clear();
    m_stCoverConfig.clear();
    m_stCoverConfig.vecCoverAttr.clear();
    m_vecOverplayInfo.clear();
    m_vecCoverInfo.clear();

    return OK;
}

IpcRet_E COsdManage::get_osd_config(Osd::OsdConfig_S &stInfo)
{
    OS_mutexLock(&m_stuMutex);
    stInfo = m_stOsdConfig;
    OS_mutexUnlock(&m_stuMutex);

    return OK;
}

IpcRet_E COsdManage::set_osd_config(Osd::OsdConfig_S stInfo)
{
    OS_mutexLock(&m_stuMutex);

    std::vector<Osd::OverplayInfo_S> vecInfo;
    vecInfo = m_vecOverplayInfo;
    for (size_t i = 0; i < vecInfo.size(); i++)
    {
        if(vecInfo[i].stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_PEOPLE)
        {
            // note AI 动态分析专用 网页设置，避免影响 AI
            continue;
        }
        if (vecInfo.at(i).stuOverplay.enElementType== Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE)
        {
            if(FIELD_ZERO == i)
            {
                vecInfo.at(i).stuInfo.bEnable = stInfo.vecOsdInfo.at(i).bEnable;
                vecInfo.at(i).stuOverplay.strCustomize = stInfo.vecOsdInfo.at(i).strName;
                if (OK != set_osd_attr(stInfo.vecOsdInfo.at(i).stOsdAttr, vecInfo.at(i).stuOverplay))
                {
                    return ERR;
                }
            }
            else if(i >= FIELD_TWO && i <= FIELD_FOUR)
            {
                vecInfo.at(i).stuInfo.bEnable = stInfo.vecOsdInfo.at(i-1).bEnable;
                vecInfo.at(i).stuOverplay.strCustomize = stInfo.vecOsdInfo.at(i-1).strName;
                if (OK != set_osd_attr(stInfo.vecOsdInfo.at(i-1).stOsdAttr, vecInfo.at(i).stuOverplay))
                {
                    return ERR;
                }
            }
        }
        else if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_TIME)
        {
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
            if (OK != set_osd_attr(stInfo.stOsdTimeInfo.stOsdAttr, vecInfo.at(i).stuOverplay))
            {
                return ERR;
            }
        }
        else if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_NAME)
        {
            vecInfo.at(i).stuInfo.bEnable = stInfo.stOsdNameInfo.bEnable;
            vecInfo.at(i).stuInfo.strName = stInfo.stOsdNameInfo.strName;
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

    OS_mutexUnlock(&m_stuMutex);

    set_overplay_info(vecInfo);

    return OK;
}

IpcRet_E COsdManage::set_osd_attr(Osd::OsdAttribute_S stOsdAttr, Osd::Overplay_S &stOverplay)
{
    stOverplay.nHorMargin = stOsdAttr.nX;
    stOverplay.nVerMargin = stOsdAttr.nY;

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
    OS_mutexLock(&m_stuMutex);
    vecInfo = m_vecOverplayInfo;
    OS_mutexUnlock(&m_stuMutex);

    return OK;
}

IpcRet_E COsdManage::set_overplay_info(std::vector<Osd::OverplayInfo_S> vecInfo)
{
    OS_mutexLock(&m_stuMutex);
#if CAP_EXHIBITION_OSD_PANEL
    reserve_exhibition_panel_overlay(vecInfo);
#endif
    if (Convert::write_file(m_strOverplayFile, vecInfo))
    {
        dlog_error("写入osd_overplay.json文件失败");
        return ERR;
    }
    m_vecOverplayInfo.clear();
    m_vecOverplayInfo = vecInfo;
    COverlayDraw::instance()->set_update_flag(true);
    OS_mutexUnlock(&m_stuMutex);

    return OK;
}

IpcRet_E COsdManage::get_cover_config(Osd::CoverConfig_S &stInfo)
{
    OS_mutexLock(&m_stuMutex);
    stInfo = m_stCoverConfig;
    OS_mutexUnlock(&m_stuMutex);
    return OK;
}

IpcRet_E COsdManage::set_cover_config(Osd::CoverConfig_S stInfo)
{
    OS_mutexLock(&m_stuMutex);

    std::vector<Osd::CoverInfo_S> vecInfo;
    vecInfo = m_vecCoverInfo;
    for (int i = 0; vecInfo.size() > i; i++)
    {
        if (!stInfo.bEnable)
        {
            vecInfo.at(i).stuInfo.bEnable = stInfo.bEnable;
        }
        else
        {
            /*坐标小于0，不进行生效*/
            if(stInfo.vecCoverAttr.at(i).nX < 0 || stInfo.vecCoverAttr.at(i).nY < 0)
            {
                vecInfo.at(i).stuInfo.bEnable = false;
                continue;
            }
            
            vecInfo.at(i).stuInfo.bEnable = stInfo.vecCoverAttr.at(i).bEnable;
        }

        vecInfo.at(i).stuInfo.strName = stInfo.vecCoverAttr.at(i).strName;

        switch (stInfo.vecCoverAttr.at(i).enColor)
        {
        case Osd::OSD_COLOR_E::OSD_COLOR_BLACK:
            vecInfo.at(i).stuCover.strBackColor = "0xFF000000";
            break;
        case Osd::OSD_COLOR_E::OSD_COLOR_WHITE:
            vecInfo.at(i).stuCover.strBackColor = "0xFFFFFFFF";
            break;
        case Osd::OSD_COLOR_E::OSD_COLOR_CUSTOMIZE:
            vecInfo.at(i).stuCover.strBackColor = stInfo.vecCoverAttr.at(i).strColor;
            if (vecInfo.at(i).stuCover.strBackColor.size() > 0 && vecInfo.at(i).stuCover.strBackColor[0] == '#')
            {
                vecInfo.at(i).stuCover.strBackColor.replace(0, 1, "0xFF"); // 替换 '#' 为 '0xFF'
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

    OS_mutexUnlock(&m_stuMutex);

    set_cover_info(vecInfo);

    return OK;
}

IpcRet_E COsdManage::get_cover_info(std::vector<Osd::CoverInfo_S> &vecInfo)
{
    OS_mutexLock(&m_stuMutex);
    vecInfo = m_vecCoverInfo;
    OS_mutexUnlock(&m_stuMutex);

    return OK;
}

IpcRet_E COsdManage::set_cover_info(std::vector<Osd::CoverInfo_S> vecInfo, bool bIsWriteFile)
{
    OS_mutexLock(&m_stuMutex);
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
    OS_mutexUnlock(&m_stuMutex);
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
    OS_mutexLock(&m_stuMutex);
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
    OS_mutexUnlock(&m_stuMutex);

    return OK;
}

IpcRet_E COsdManage::set_osd_share_info(System::DeviceConfig_S stDeviceConfig)
{
    OS_mutexLock(&m_stuMutex);

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
    COverlayDraw::instance()->set_update_flag(true);
    OS_mutexUnlock(&m_stuMutex);

    return OK;
}

IpcRet_E COsdManage::send_detection_result(const int nWidth, const int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo)
{
    if(!m_bInit)
    {
        return ERR_UNINIT;
    }

    /* 更新 */
    COverlayDraw::instance()->update_ai_result(nWidth, nHeight, vstRectInfo, m_vecOverplayInfo[Osd::ElementType_E::ELEMENT_TYPE_PEOPLE]);
    return OK;
}

#if CAP_EXHIBITION_OSD_PANEL
IpcRet_E COsdManage::send_panel_result(const OsdPanel::PanelFrame_S &stPanelFrame)
{
    if (!m_bInit)
    {
        return ERR_UNINIT;
    }

    OsdPanel::PanelFrame_S stNormalizedFrame = stPanelFrame;
    stNormalizedFrame.normalize();

    std::lock_guard<std::mutex> lock(m_panelMutex);
    if (stNormalizedFrame.empty())
    {
        return OK;
    }

    if (m_stPanelFrame == stNormalizedFrame)
    {
        m_unPanelUpdateTimeMs = get_steady_time_ms();
        return OK;
    }

    m_stPanelFrame = stNormalizedFrame;
    ++m_unPanelVersion;
    m_unPanelUpdateTimeMs = get_steady_time_ms();
    return OK;
}

IpcRet_E COsdManage::get_panel_result(OsdPanel::PanelFrame_S &stPanelFrame,
                                      uint64_t &unVersion,
                                      uint64_t &unUpdateTimeMs)
{
    std::lock_guard<std::mutex> lock(m_panelMutex);
    stPanelFrame = m_stPanelFrame;
    unVersion = m_unPanelVersion;
    unUpdateTimeMs = m_unPanelUpdateTimeMs;
    return OK;
}
#endif

void COsdManage::reset_osd_status(int nChn)
{

    System::DeviceConfig_S stDeviceConfig;
    SystemManage::instance()->get_device_config(stDeviceConfig);
    set_osd_share_info(stDeviceConfig);

    if (OK == COverlayDraw::instance()->reDeinit(nChn))
    {
             dlog_info("osd-Overlay去初始化成功");
    }
        
    CCoverDraw::instance()->set_update_flag(true);
 

    if ( OK == COverlayDraw::instance()->reinit(nChn))
    {

             dlog_info("osd-Overlay初始化成功");
    }


    return;


}
