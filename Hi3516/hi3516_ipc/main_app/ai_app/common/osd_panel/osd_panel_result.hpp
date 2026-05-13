/**
 * @FilePath     : osd_panel_result.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-03-19 13:44:04
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-19 13:52:38
 * @Description  : 展会 OSD 面板通用结果结构 
 */

#pragma once

#if CAP_EXHIBITION_OSD_PANEL

#include <string>
#include <vector>

#include "common_define.h"
#include "event_define.h"

namespace OsdPanel
{
    /**
     * @brief   : 面板单个字段结构
     * @return   {struct} 展会面板字段信息
     */
    typedef struct _PanelField_S_
    {
        /* 字段标签，例如“目标”“状态” */
        std::string strLabel;
        /* 字段值，例如“人”“已触发报警” */
        std::string strValue;

        _PanelField_S_() = default;

        /**
         * @brief   : 使用标签和值初始化面板字段
         * @param    {const std::string &} strFieldLabel：字段标签
         * @param    {const std::string &} strFieldValue：字段值
         * @return   {void}
         */
        _PanelField_S_(const std::string &strFieldLabel, const std::string &strFieldValue)
            : strLabel(strFieldLabel), strValue(strFieldValue)
        {
        }

        /**
         * @brief   : 清空字段内容
         * @return   {void}
         */
        void clear();

        /**
         * @brief   : 判断字段是否为空
         * @return   {bool} true：为空 false：非空
         */
        bool empty() const;

        /**
         * @brief   : 比较两个字段是否相同
         * @param    {const _PanelField_S_ &} stOther：待比较字段
         * @return   {bool} true：相同 false：不同
         */
        bool operator==(const _PanelField_S_ &stOther) const;
    } PanelField_S;

    /**
     * @brief   : 面板单条展示项结构
     * @return   {struct} 展会面板条目信息
     */
    typedef struct _PanelItem_S_
    {
        /* 条目标题，例如“区域 1” */
        std::string strTitle;
        /* 当前条目是否已经触发报警 */
        bool bAlarm = false;
        /* 当前条目是否携带目标框 */
        bool bHasRect = false;
        /* 当前条目对应的目标框 */
        Common::RectInfo_S stRect;
        /* 面板显示排序键，通常为区域号 */
        int nSortKey = 0;
        /* 条目筛选优先级，内部用于选出每个区域最终展示项 */
        int nPriority = 0;
        /* 条目字段列表 */
        std::vector<PanelField_S> vecFields;

        /**
         * @brief   : 清空条目内容
         * @return   {void}
         */
        void clear();

        /**
         * @brief   : 判断条目是否为空
         * @return   {bool} true：为空 false：非空
         */
        bool empty() const;

        /**
         * @brief   : 比较两个条目是否相同
         * @param    {const _PanelItem_S_ &} stOther：待比较条目
         * @return   {bool} true：相同 false：不同
         */
        bool operator==(const _PanelItem_S_ &stOther) const;
    } PanelItem_S;

    /**
     * @brief   : 面板整帧结果结构
     * @return   {struct} 展会面板帧信息
     */
    typedef struct _PanelFrame_S_
    {
        /* 检测结果对应的源宽度 */
        int nWidth = 0;
        /* 检测结果对应的源高度 */
        int nHeight = 0;
        /* 当前面板对应的事件类型 */
        Event::Type_E enEventType = Event::Type_E::UNKNOWN;
        /* 当前面板需要显示的条目列表 */
        std::vector<PanelItem_S> vecItems;

        /**
         * @brief   : 清空整帧面板内容
         * @return   {void}
         */
        void clear();

        /**
         * @brief   : 判断面板帧是否为空
         * @return   {bool} true：为空 false：非空
         */
        bool empty() const;

        /**
         * @brief   : 归一化面板条目
         * @param    {size_t} nMaxItems：最大显示条目数
         * @return   {void}
         * @note    : 会移除空条目、按排序键排序，并裁剪到最大条目数
         */
        void normalize(size_t nMaxItems = 4);

        /**
         * @brief   : 比较两帧面板结果是否相同
         * @param    {const _PanelFrame_S_ &} stOther：待比较面板帧
         * @return   {bool} true：相同 false：不同
         */
        bool operator==(const _PanelFrame_S_ &stOther) const;
    } PanelFrame_S;

    /**
     * @brief   : 将结构化面板结果转换为最终展示文本
     * @param    {const PanelFrame_S &} stPanelFrame：面板结果
     * @param    {size_t} nMaxItems：最大显示条目数
     * @return   {std::string} 左上角面板展示文本
     */
    std::string build_panel_text(const PanelFrame_S &stPanelFrame, size_t nMaxItems = 4);
} // namespace OsdPanel

#endif
