/**
 * @FilePath     : osd_configure.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-24 09:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-24 10:20:00
 * @Description  : OSD/Cover配置转换
 */

#include "osd_configure.h"

#include <algorithm>
#include "convert_interface.h"
#include "dlog.h"
#include "network_manage.h"
#include "path_define.h"
#include "system_manage.h"
#include "time_manage.h"

namespace
{
/* 中文提示语（与各业务仓库 share_data.h 中定义保持一致） */
const char *CN_PEOPLE_TIPS = "人数：%d";
const char *CN_MAC_TIPS = "物理地址：";
const char *CN_PRESET_TIPS = "预置位：";
/* 英文提示语 */
const char *EN_PEOPLE_TIPS = "People:%d";
const char *EN_MAC_TIPS = "MAC:";
const char *EN_PRESET_TIPS = "Preset:";
/* 时间OSD无文本内容 */
const std::string OSD_EMPTY_TEXT;
} // namespace

COsdConfigure::COsdConfigure() : m_strOsdConfigFile(OSD_CONFIG_FILE), m_strCoverConfigFile(COVER_CONFIG_FILE)
{
    m_pApplier = nullptr;
    m_bInit.store(false);
}

COsdConfigure::~COsdConfigure()
{
}

IpcRet_E COsdConfigure::setOsdConfigApplier(IOsdConfigApplier *pApplier)
{
    if (pApplier == nullptr)
    {
        dlog_error("无效的OSD配置应用接口");
        return ERR_PARAM_NULL;
    }

    m_pApplier = pApplier;
    return OK;
}

IpcRet_E COsdConfigure::clearOsdConfigApplier(IOsdConfigApplier *pApplier)
{
    if (pApplier == nullptr)
    {
        dlog_error("无效的OSD配置应用接口");
        return ERR_PARAM_NULL;
    }

    if (m_pApplier != pApplier)
    {
        dlog_warn("清理OSD配置应用接口失败，接口指针不匹配");
        return ERR_PARAM;
    }

    m_pApplier = nullptr;
    return OK;
}

IpcRet_E COsdConfigure::init()
{
    if (m_pApplier == nullptr)
    {
        dlog_error("OSD配置应用接口未注册，无法初始化");
        return ERR_UNINIT;
    }

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

    m_bInit.store(true);

    return OK;
}

bool COsdConfigure::is_initialized() const
{
    return m_bInit.load();
}

IpcRet_E COsdConfigure::get_osd_config(Osd::OsdConfig_S &stInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    stInfo = m_stOsdConfig;

    return OK;
}

IpcRet_E COsdConfigure::set_osd_config(Osd::OsdConfig_S stInfo)
{
    if (m_pApplier == nullptr)
    {
        dlog_error("OSD配置应用接口未注册，无法设置OSD配置");
        return ERR_UNINIT;
    }

    std::vector<Osd::OverplayInfo_S> vecInfo;
    if (OK != m_pApplier->get_overplay_info(vecInfo))
    {
        dlog_error("获取当前Overplay布局失败");
        return ERR;
    }

    stInfo.stOsdNameInfo.strName.reserve(OSD_NAME_LENGTH_LIMIT);

    /* 已消费的字符叠加配置索引：第N个CUSTOMIZE槽位对应vecOsdInfo[N-1]，槽位数多于配置项时跳过多余槽位 */
    size_t unOsdInfoIndex = 0;
    for (size_t i = 0; i < vecInfo.size(); i++)
    {
        if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_PEOPLE)
        {
            // note AI 动态分析专用 网页设置，避免影响 AI
            continue;
        }

        if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE)
        {
            if (unOsdInfoIndex >= stInfo.vecOsdInfo.size())
            {
                /* 该平台布局的字符叠加槽位数多于配置项，多余槽位保持原状 */
                continue;
            }

            if (stInfo.vecOsdInfo.at(unOsdInfoIndex).strName.size() > OSD_NAME_LENGTH_LIMIT)
            {
                dlog_error("字符叠加 ID:[%d] 字符串名称:[%s] 参数错误",
                           stInfo.vecOsdInfo[unOsdInfoIndex].nId,
                           stInfo.vecOsdInfo[unOsdInfoIndex].strName.c_str());
                return ERR_PARAM;
            }

            vecInfo.at(i).stuInfo.bEnable = stInfo.vecOsdInfo.at(unOsdInfoIndex).bEnable;
            vecInfo.at(i).stuOverplay.strCustomize = stInfo.vecOsdInfo.at(unOsdInfoIndex).strName;
            if (OK != m_pApplier->adapt_osd_attr(Osd::ElementType_E::ELEMENT_TYPE_CUSTOMIZE,
                                                 stInfo.vecOsdInfo.at(unOsdInfoIndex).strName,
                                                 stInfo.vecOsdInfo.at(unOsdInfoIndex).stOsdAttr,
                                                 vecInfo.at(i).stuOverplay))
            {
                return ERR;
            }
            unOsdInfoIndex++;
        }
        else if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_TIME)
        {
            /* 显示时间、日期 */
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
            if (OK != m_pApplier->adapt_osd_attr(Osd::ElementType_E::ELEMENT_TYPE_TIME,
                                                 OSD_EMPTY_TEXT,
                                                 stInfo.stOsdTimeInfo.stOsdAttr,
                                                 vecInfo.at(i).stuOverplay))
            {
                return ERR;
            }
        }
        else if (vecInfo.at(i).stuOverplay.enElementType == Osd::ElementType_E::ELEMENT_TYPE_NAME)
        {
            /* 通道名称 */
            if (stInfo.stOsdNameInfo.strName.size() > OSD_NAME_LENGTH_LIMIT)
            {
                dlog_error("通道名称:[%s] 参数错误", stInfo.stOsdNameInfo.strName.c_str());
                return ERR_PARAM;
            }
            vecInfo.at(i).stuInfo.bEnable = stInfo.stOsdNameInfo.bEnable;
            vecInfo.at(i).stuInfo.strName = stInfo.stOsdNameInfo.strName;
            if (OK != m_pApplier->adapt_osd_attr(Osd::ElementType_E::ELEMENT_TYPE_NAME,
                                                 stInfo.stOsdNameInfo.strName,
                                                 stInfo.stOsdNameInfo.stOsdAttr,
                                                 vecInfo.at(i).stuOverplay))
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

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stOsdConfig = stInfo;
    }

    if (OK != m_pApplier->apply_overplay_info(vecInfo))
    {
        dlog_error("应用Overplay布局失败");
    }

    return OK;
}

IpcRet_E COsdConfigure::get_cover_config(Osd::CoverConfig_S &stInfo)
{
    if (m_pApplier == nullptr)
    {
        dlog_error("OSD配置应用接口未注册，无法获取Cover配置");
        return ERR_UNINIT;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    stInfo = m_stCoverConfig;
    normalize_cover_config(stInfo);
    return OK;
}

IpcRet_E COsdConfigure::set_cover_config(Osd::CoverConfig_S stInfo)
{
    if (m_pApplier == nullptr)
    {
        dlog_error("OSD配置应用接口未注册，无法设置Cover配置");
        return ERR_UNINIT;
    }

    if (stInfo.vecCoverAttr.size() > get_cover_max_area_count())
    {
        dlog_warn("隐私遮盖区域数超出平台能力, request:%zu, max:%zu", stInfo.vecCoverAttr.size(), get_cover_max_area_count());
        return ERR_PARAM;
    }

    /* 禁用时允许省略区域数组，内部补齐为固定的区域配置。 */
    normalize_cover_config(stInfo);

    std::vector<Osd::CoverInfo_S> vecInfo;
    if (OK != m_pApplier->get_cover_info(vecInfo))
    {
        dlog_error("获取当前Cover区域布局失败");
        return ERR;
    }

    /* 以布局与配置的较小者为界，避免布局文件区域数异常时越界访问配置数组 */
    const size_t unCount = std::min(vecInfo.size(), stInfo.vecCoverAttr.size());
    for (size_t i = 0; unCount > i; i++)
    {
        if (!stInfo.bEnable)
        {
            vecInfo.at(i).stuInfo.bEnable = stInfo.bEnable;
        }
        else
        {
            vecInfo.at(i).stuInfo.bEnable = stInfo.vecCoverAttr.at(i).bEnable;
        }

        vecInfo.at(i).stuInfo.strName = stInfo.vecCoverAttr.at(i).strName;

        if (OK != m_pApplier->cover_attr_to_info(stInfo.vecCoverAttr.at(i), vecInfo.at(i)))
        {
            return ERR;
        }
    }

    if (Convert::write_file(m_strCoverConfigFile, stInfo))
    {
        dlog_error("写入cover_config.json文件失败");
        return ERR;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stCoverConfig = stInfo;
    }

    if (OK != m_pApplier->apply_cover_info(vecInfo))
    {
        dlog_error("应用Cover区域布局失败");
    }

    return OK;
}

std::size_t COsdConfigure::get_cover_max_area_count() const
{
    if (m_pApplier == nullptr)
    {
        dlog_error("OSD配置应用接口未注册，无法获取隐私遮盖区域数量");
        return 0;
    }

    return m_pApplier->get_cover_max_area_count();
}

IpcRet_E COsdConfigure::get_overplay_info(std::vector<Osd::OverplayInfo_S> &vecInfo)
{
    if (m_pApplier == nullptr)
    {
        dlog_error("OSD配置应用接口未注册，无法获取Overplay布局");
        return ERR_UNINIT;
    }

    return m_pApplier->get_overplay_info(vecInfo);
}

IpcRet_E COsdConfigure::set_overplay_info(const std::vector<Osd::OverplayInfo_S> &vecInfo)
{
    if (m_pApplier == nullptr)
    {
        dlog_error("OSD配置应用接口未注册，无法设置Overplay布局");
        return ERR_UNINIT;
    }

    return m_pApplier->apply_overplay_info(vecInfo);
}

IpcRet_E COsdConfigure::get_osd_share_info(Osd::ShareInfo_S &stuShareInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    System::DeviceConfig_S stDeviceConfig;
    SystemManage::instance()->get_device_config(stDeviceConfig);

    m_stuShareInfo.stuTimeInfo.strZone = CTimeManage::instance()->get_current_zone(stDeviceConfig.enTimeZone);

    switch (m_stOsdConfig.stOsdTimeInfo.enDateFormat)
    {
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYY_MM_DD:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH,
                                                                                       System::DateFormat_E::YYYY_MM_DD);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH,
                                                                                           System::DateFormat_E::YYYY_MM_DD);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_MM_DD_YYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH,
                                                                                       System::DateFormat_E::MM_DD_YYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH,
                                                                                           System::DateFormat_E::MM_DD_YYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_DD_MM_YYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH,
                                                                                       System::DateFormat_E::DD_MM_YYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH,
                                                                                           System::DateFormat_E::DD_MM_YYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::CHINESE_YYYYMMDD:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::SIMP_CHINESE);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::SIMP_CHINESE,
                                                                                       System::DateFormat_E::YYYYMMDD);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::SIMP_CHINESE,
                                                                                           System::DateFormat_E::YYYYMMDD);
        break;
    case Osd::OSD_DATE_FORMAT_E::CHINESE_MMDDYYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::SIMP_CHINESE);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::SIMP_CHINESE,
                                                                                       System::DateFormat_E::MMDDYYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::SIMP_CHINESE,
                                                                                           System::DateFormat_E::MMDDYYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::CHINESE_DDMMYYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::SIMP_CHINESE);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::SIMP_CHINESE,
                                                                                       System::DateFormat_E::DDMMYYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::SIMP_CHINESE,
                                                                                           System::DateFormat_E::DDMMYYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_YYYYMMDD:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH,
                                                                                       System::DateFormat_E::YYYYMMDD);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH,
                                                                                           System::DateFormat_E::YYYYMMDD);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_MMDDYYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH,
                                                                                       System::DateFormat_E::MMDDYYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH,
                                                                                           System::DateFormat_E::MMDDYYYY);
        break;
    case Osd::OSD_DATE_FORMAT_E::ENGLISH_DDMMYYYY:
        m_stuShareInfo.stuTimeInfo.strWeek = CTimeManage::instance()->get_current_week(System::Language_E::ENGLISH);
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(System::Language_E::ENGLISH,
                                                                                       System::DateFormat_E::DDMMYYYY);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(System::Language_E::ENGLISH,
                                                                                           System::DateFormat_E::DDMMYYYY);
        break;

    default:
        break;
    }

    stuShareInfo = m_stuShareInfo;

    return OK;
}

IpcRet_E COsdConfigure::set_osd_share_info(System::DeviceConfig_S stDeviceConfig)
{
    if (m_pApplier == nullptr)
    {
        dlog_error("OSD配置应用接口未注册，无法设置OSD共用信息");
        return ERR_UNINIT;
    }

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
        m_stuShareInfo.stuTimeInfo.strTime = CTimeManage::instance()->get_current_time(stDeviceConfig.enLanguage,
                                                                                       stDeviceConfig.enDateFormat);
        m_stuShareInfo.stuTimeInfo.strTime12 = CTimeManage::instance()->get_current_time12(stDeviceConfig.enLanguage,
                                                                                           stDeviceConfig.enDateFormat);
    }

    /* 渲染刷新在锁外执行，避免锁内调用业务侧回调 */
    m_pApplier->refresh_overplay();

    return OK;
}

bool COsdConfigure::normalize_cover_config(Osd::CoverConfig_S &stConfig) const
{
    const std::size_t unMaxCount = get_cover_max_area_count();

    bool bChanged = false;
    if (stConfig.vecCoverAttr.size() > unMaxCount)
    {
        stConfig.vecCoverAttr.resize(unMaxCount);
        bChanged = true;
    }

    while (stConfig.vecCoverAttr.size() < unMaxCount)
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
