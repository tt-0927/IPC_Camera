/**
 * @FilePath     : layout_define.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2024-10-11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-12 10:16:33
 * @Description  : 布局定义
 */

#pragma once

#include <string>

/* 初始状态显示屏幕大小 */
#define INIT_WIDTH 1920
#define INIT_HEIGHT 1080

namespace Layout
{
    /// @brief 布局类型枚举
    typedef enum class Type
    {
        SCREEN_1  = 1, /* 1画面 */
        SCREEN_4  = 4,
        SCREEN_8  = 8,
        SCREEN_9  = 9,
        SCREEN_16 = 16
    } Type_E;

     /* 最大布局数 */
    const int MAX_LAYOUT_NUM = 5;

    /// @brief 位置信息
    typedef struct Rect
    {
        int nX = 0;      /* 左上角 x 坐标 */
        int nY = 0;      /* 左上角 y 坐标 */
        int nWidth = 0;  /* 宽度 */
        int nHeight = 0; /* 高度 */
    } Rect_S;

    /// @brief 通道号与预览窗口的对应信息
    typedef struct Item
    {
        int nPos = -1;   /* 预览窗口号（预览窗口位置） */
        int nChnId = -1; /* 通道号 */
    } Item_S;

    /// @brief 通道信息
    typedef struct ChnInfo
    {
        Item_S stItem; /* 通道号与预览窗口的对应信息 */
        Rect_S stRect; /* 位置信息 */
    } ChnInfo_S;

}    // namespace Layout
