/**
 * @FilePath     : corner_rect_draw.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-11-17 09:18:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-30 15:14:08
 * @Description  : 角框类型绘制：AI动态分析用
 */

#pragma once

#include <atomic>
#include <thread>

#include "osd_define.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "dlog.h"

#include "mpp_rgn.h"

class CCornerRectDraw : public CSingleton<CCornerRectDraw>
{
    CCornerRectDraw();

public:
    virtual ~CCornerRectDraw();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<CCornerRectDraw>;

    /**
     * @brief   : 初始化
     * @return   {IpcRet_E} 公共返回码
     */
    IpcRet_E init();

    /**
     * @brief   : 去初始化
     * @return   {IpcRet_E} 公共返回码
     */
    IpcRet_E deinit();

    /**
     * @brief   : 更新 AI 检测结果
     * @param    {int} nWidth：检测结果框对应的分辨率宽
     * @param    {int} nHeight：检测结果框对应的分辨率高
     * @param    {vector<Common::RectInfo_S>} &vRectInfo：矩形检测结果框
     */
    void update_ai_result(int nWidth, int nHeight, const std::vector<Common::RectInfo_S> &vRectInfo);

    /**
     * @brief   : 隐藏指定 VPSS 通道的全部 AI 角框
     * @param    {int} nChn：VPSS 通道号
     * @return   {void}
     * @note    : 裁剪几何切换后，旧坐标的角框不得继续显示，等待下一帧 AI 结果重绘。
     */
    void clear_channel(int nChn);

private:

    /**
     * @brief   : 设置rgn参数
     * @param    {int} nChn VPSS通道号，主码流/子码流
     * @param    {uint32_t} unHandle 句柄号，第几个区域
     * @param    {RectInfo_S} &stRectInfo 矩形检测结果框
     * @return   {HiRgnNeedParam_S} rgn参数结构体
     */
    HiRgnNeedParam_S set_rgn(int nChn, uint32_t unHandle, const Common::RectInfo_S &stRectInfo);

private:
    /* rgn句柄（一个vpss通道最多有4个角框区域） */
    std::vector<HiRgn_S *> m_pVecRgns;
};
