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

bool normalize_cover_info(std::vector<Osd::CoverInfo_S> &vecCoverInfo)
{
    bool bChanged = false;
    if (vecCoverInfo.size() > MAX_OSD_COVER_NUM)
    {
        vecCoverInfo.resize(MAX_OSD_COVER_NUM);
        bChanged = true;
    }

    while (vecCoverInfo.size() < MAX_OSD_COVER_NUM)
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
/* 展会面板专用 overlay 槽位下标。 */
constexpr size_t EXHIBITION_PANEL_OVERPLAY_INDEX = Osd::ElementType_E::ELEMENT_TYPE_MAC; //(ELEMENT_TYPE_MAC暂未使用，借用)

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

COsdManage::COsdManage() : m_bInit(false), m_strOverplayFile(OSD_OVERPLAY_CONFIG_FILE), m_strCoverFile(OSD_COVER_CONFIG_FILE)
{
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
                // stOverplayInfo.stuOverplay.nFontSize = 4;   /* 边框大小 */
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
        m_vecCoverInfo.clear();
        normalize_cover_info(m_vecCoverInfo);
        Convert::write_file(m_strCoverFile, m_vecCoverInfo);
    }
    else if (normalize_cover_info(m_vecCoverInfo))
    {
        /* 配置项数量必须与 RK 的四个绘制区域保持一致。 */
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

    /* 清理共享层配置应用接口，避免悬挂引用 */
    COsdConfigure::instance()->clearOsdConfigApplier(this);

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

    m_vecOverplayInfo.clear();
    m_vecCoverInfo.clear();

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

IpcRet_E COsdManage::adapt_osd_attr(Osd::ElementType_E enType,
                                    const std::string &strText,
                                    Osd::OsdAttribute_S &stOsdAttr,
                                    Osd::Overplay_S &stOverplay)
{
    (void) enType;
    (void) strText;

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
    return MAX_OSD_COVER_NUM;
}

IpcRet_E COsdManage::set_cover_config(Osd::CoverConfig_S stInfo)
{
    return COsdConfigure::instance()->set_cover_config(stInfo);
}

IpcRet_E COsdManage::cover_attr_to_info(const Osd::CoverAttribute_S &stAttr, Osd::CoverInfo_S &stInfo)
{
    /* 坐标小于0，不进行生效 */
    if (stAttr.nX < 0 || stAttr.nY < 0)
    {
        stInfo.stuInfo.bEnable = false;
        return OK;
    }

    switch (stAttr.enColor)
    {
    case Osd::OSD_COLOR_E::OSD_COLOR_BLACK:
        stInfo.stuCover.strBackColor = "0xFF000000";
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_WHITE:
        stInfo.stuCover.strBackColor = "0xFFFFFFFF";
        break;
    case Osd::OSD_COLOR_E::OSD_COLOR_CUSTOMIZE:
        stInfo.stuCover.strBackColor = stAttr.strColor;
        if (stInfo.stuCover.strBackColor.size() > 0 && stInfo.stuCover.strBackColor[0] == '#')
        {
            stInfo.stuCover.strBackColor.replace(0, 1, "0xFF"); // 替换 '#' 为 '0xFF'
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
    COverlayDraw::instance()->set_update_flag(true);
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

IpcRet_E COsdManage::send_detection_result(const int nWidth, const int nHeight, const std::vector<Common::RectInfo_S> &vstRectInfo)
{
    if (!m_bInit)
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

IpcRet_E COsdManage::get_panel_result(OsdPanel::PanelFrame_S &stPanelFrame, uint64_t &unVersion, uint64_t &unUpdateTimeMs)
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

    if (OK == COverlayDraw::instance()->reinit(nChn))
    {

        dlog_info("osd-Overlay初始化成功");
    }

    return;
}
