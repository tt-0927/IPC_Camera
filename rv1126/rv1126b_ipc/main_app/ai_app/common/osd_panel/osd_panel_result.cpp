/**
 * @FilePath     : osd_panel_result.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-23 14:44:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-25 09:34:11
 * @Description  : 展会 OSD 面板通用结果结构定义
 */

#include "osd_panel_result.hpp"

#include <algorithm>
#include <sstream>

namespace
{
/**
 * @brief   : 判断两个目标框是否完全一致
 * @param    {const Common::RectInfo_S &} stLhs：左侧目标框
 * @param    {const Common::RectInfo_S &} stRhs：右侧目标框
 * @return   {bool} true：一致 false：不一致
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
 * @return   {std::string} 单行展示文本
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
void PanelField_S::clear()
{
    strLabel.clear();
    strValue.clear();
}

bool PanelField_S::empty() const
{
    return strLabel.empty() && strValue.empty();
}

bool PanelField_S::operator==(const PanelField_S &stOther) const
{
    return strLabel == stOther.strLabel && strValue == stOther.strValue;
}

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

bool PanelItem_S::operator==(const PanelItem_S &stOther) const
{
    return strTitle == stOther.strTitle &&
           bAlarm == stOther.bAlarm &&
           bHasRect == stOther.bHasRect &&
           is_same_rect(stRect, stOther.stRect) &&
           nSortKey == stOther.nSortKey &&
           vecFields == stOther.vecFields;
}

void PanelFrame_S::clear()
{
    nWidth = 0;
    nHeight = 0;
    enEventType = Event::Type_E::UNKNOWN;
    vecItems.clear();
}

bool PanelFrame_S::empty() const
{
    return vecItems.empty();
}

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

bool PanelFrame_S::operator==(const PanelFrame_S &stOther) const
{
    return nWidth == stOther.nWidth &&
           nHeight == stOther.nHeight &&
           enEventType == stOther.enEventType &&
           vecItems == stOther.vecItems;
}

std::string build_panel_text(const PanelFrame_S &stPanelFrame, size_t nMaxItems)
{
    /* 用于输出的可修改面板副本。 */
    PanelFrame_S stDisplayFrame = stPanelFrame;
    stDisplayFrame.normalize(nMaxItems);
    if (stDisplayFrame.vecItems.empty())
    {
        return "";
    }

    /* 最终拼接出的多行面板文本。 */
    std::ostringstream oss;
    for (size_t i = 0; i < stDisplayFrame.vecItems.size(); ++i)
    {
        /* 当前需要输出的单条面板项。 */
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
