/**
 * @FilePath     : osd_panel_result.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-19 13:44:04
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-19 13:52:30
 * @Description  : 展会 OSD 面板通用结果结构定义 
 */

#include "osd_panel_result.hpp"

#if CAP_EXHIBITION_OSD_PANEL

#include <algorithm>
#include <sstream>

namespace
{
/**
 * @brief   : 判断两个目标框是否完全相同
 * @param    {const Common::RectInfo_S &} stLhs：左侧目标框
 * @param    {const Common::RectInfo_S &} stRhs：右侧目标框
 * @return   {bool} true：相同 false：不同
 */
bool is_same_rect(const Common::RectInfo_S &stLhs, const Common::RectInfo_S &stRhs)
{
    return stLhs.nX1 == stRhs.nX1 &&
           stLhs.nY1 == stRhs.nY1 &&
           stLhs.nX2 == stRhs.nX2 &&
           stLhs.nY2 == stRhs.nY2;
}

/**
 * @brief   : 拼接单个字段的展示文本
 * @param    {const OsdPanel::PanelField_S &} stField：字段信息
 * @return   {std::string} 单行字段文本
 */
std::string build_field_line(const OsdPanel::PanelField_S &stField)
{
    if (stField.strLabel.empty())
    {
        return stField.strValue;
    }
    if (stField.strValue.empty())
    {
        return stField.strLabel;
    }
    return stField.strLabel + ": " + stField.strValue;
}
} // namespace

namespace OsdPanel
{
/**
 * @brief   : 清空字段内容
 * @return   {void}
 */
void PanelField_S::clear()
{
    strLabel.clear();
    strValue.clear();
}

/**
 * @brief   : 判断字段是否为空
 * @return   {bool} true：为空 false：非空
 */
bool PanelField_S::empty() const
{
    return strLabel.empty() && strValue.empty();
}

/**
 * @brief   : 比较两个字段是否相同
 * @param    {const PanelField_S &} stOther：待比较字段
 * @return   {bool} true：相同 false：不同
 */
bool PanelField_S::operator==(const PanelField_S &stOther) const
{
    return strLabel == stOther.strLabel && strValue == stOther.strValue;
}

/**
 * @brief   : 清空条目内容
 * @return   {void}
 */
void PanelItem_S::clear()
{
    strTitle.clear();
    bAlarm = false;
    bHasRect = false;
    stRect = Common::RectInfo_S();
    nSortKey = 0;
    nPriority = 0;
    vecFields.clear();
}

/**
 * @brief   : 判断条目是否为空
 * @return   {bool} true：为空 false：非空
 */
bool PanelItem_S::empty() const
{
    if (!strTitle.empty())
    {
        return false;
    }

    for (const auto &stField : vecFields)
    {
        if (!stField.empty())
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief   : 比较两个条目是否相同
 * @param    {const PanelItem_S &} stOther：待比较条目
 * @return   {bool} true：相同 false：不同
 */
bool PanelItem_S::operator==(const PanelItem_S &stOther) const
{
    return strTitle == stOther.strTitle &&
           bAlarm == stOther.bAlarm &&
           bHasRect == stOther.bHasRect &&
           is_same_rect(stRect, stOther.stRect) &&
           nSortKey == stOther.nSortKey &&
           vecFields == stOther.vecFields;
}

/**
 * @brief   : 清空整帧面板内容
 * @return   {void}
 */
void PanelFrame_S::clear()
{
    nWidth = 0;
    nHeight = 0;
    enEventType = Event::Type_E::UNKNOWN;
    vecItems.clear();
}

/**
 * @brief   : 判断面板帧是否为空
 * @return   {bool} true：为空 false：非空
 */
bool PanelFrame_S::empty() const
{
    return vecItems.empty();
}

/**
 * @brief   : 归一化面板条目
 * @param    {size_t} nMaxItems：最大显示条目数
 * @return   {void}
 * @note    : 会过滤空条目、按排序键排序，并限制最终输出数量
 */
void PanelFrame_S::normalize(size_t nMaxItems)
{
    vecItems.erase(
        std::remove_if(vecItems.begin(), vecItems.end(), [](const PanelItem_S &stItem)
                       { return stItem.empty(); }),
        vecItems.end());

    std::stable_sort(vecItems.begin(), vecItems.end(), [](const PanelItem_S &stLhs, const PanelItem_S &stRhs)
                     { return stLhs.nSortKey < stRhs.nSortKey; });

    if (vecItems.size() > nMaxItems)
    {
        vecItems.resize(nMaxItems);
    }
}

/**
 * @brief   : 比较两帧面板结果是否相同
 * @param    {const PanelFrame_S &} stOther：待比较面板帧
 * @return   {bool} true：相同 false：不同
 */
bool PanelFrame_S::operator==(const PanelFrame_S &stOther) const
{
    return nWidth == stOther.nWidth &&
           nHeight == stOther.nHeight &&
           enEventType == stOther.enEventType &&
           vecItems == stOther.vecItems;
}

/**
 * @brief   : 将结构化面板结果转换为最终展示文本
 * @param    {const PanelFrame_S &} stPanelFrame：面板结果
 * @param    {size_t} nMaxItems：最大显示条目数
 * @return   {std::string} 左上角面板展示文本
 */
std::string build_panel_text(const PanelFrame_S &stPanelFrame, size_t nMaxItems)
{
    /* 复制一份可修改的面板结果，避免影响调用方缓存 */
    PanelFrame_S stDisplayFrame = stPanelFrame;
    stDisplayFrame.normalize(nMaxItems);
    if (stDisplayFrame.vecItems.empty())
    {
        return "";
    }

    /* 按条目逐行拼接成最终显示字符串 */
    std::ostringstream oss;
    for (size_t i = 0; i < stDisplayFrame.vecItems.size(); ++i)
    {
        /* 当前需要输出的单条面板项 */
        const auto &stItem = stDisplayFrame.vecItems[i];
        if (!stItem.strTitle.empty())
        {
            oss << stItem.strTitle << "\n";
        }

        for (const auto &stField : stItem.vecFields)
        {
            if (stField.empty())
            {
                continue;
            }
            oss << build_field_line(stField) << "\n";
        }

        if (i + 1 < stDisplayFrame.vecItems.size())
        {
            oss << "\n";
        }
    }

    return oss.str();
}
} // namespace OsdPanel

#endif
