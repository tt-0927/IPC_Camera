/***
 * @FilePath     : overplay_draw.h
 * @Author       : huangjunda
 * @Date         : 2025-05-27 14:12:50
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-05-30 10:59:49
 * @Description  :
 */
#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include "osd_define.h"
#include "common_define.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "dlog.h"
#include "stream_venc.h"
#include "alarm_define.h"
#include "sdl_utils.h"

#include "mpp_rgn.h"

class COverplayDraw : public CSingleton<COverplayDraw>
{
    COverplayDraw();

public:
    virtual ~COverplayDraw();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<COverplayDraw>;

    /***
     * @description : 初始化
     * @author      : huangjunda
     * @return       {IpcRet_E} 公共返回码
     */
     IpcRet_E init();

    /***
    * @description : 去初始化
    * @author      : huangjunda
    * @return       {IpcRet_E} 公共返回码
    */
    IpcRet_E deinit();

    /**
     * @brief   : 初始化抓拍叠加信息RGN
    * @return    {IpcRet_E} 公共返回码
     */
    IpcRet_E init_capture_overplay();

    /**
     * @brief   : 去初始化抓拍叠加信息RGN
    * @return    {IpcRet_E} 公共返回码
     */
    IpcRet_E deinit_capture_overplay();

    /*** 
     * @description : 设置更新标志
     * @author      : huangjunda
     * @param        {bool} bIsUpdate
     * @return       {*}
     */    
    void set_update_flag(bool bIsUpdate);

    /**
     * @brief   : 更新 AI 检测结果
     * @param    {int} nWidth：检测结果框对应的分辨率宽
     * @param    {int} nHeight：检测结果框对应的分辨率高
     * @param    {vector<Common::RectInfo_S>} &vRectInfo：矩形检测结果框
     * @param    {OverplayInfo_S} stOverplayInfo：ID为8的overplay 配置
     */
    void update_ai_result(int nWidth, int nHeight, const std::vector<Common::RectInfo_S> &vRectInfo, const Osd::OverplayInfo_S stOverplayInfo);

private:
    /***
     * @description : 开始启动osd
     * @author      : huangjunda
     * @return       {*}
     */
    void start();

    /***
     * @description : 停止osd
     * @author      : huangjunda
     * @return       {*}
     */
    void stop();

    /*** 
     * @description : 设置rgn参数
     * @author      : huangjunda
     * @param        {OverplayInfo_S} stuOverplayInfo
     * @param        {int} nChn
     * @param        {uint32_t} unHandle
     * @return       {HiRgnNeedParam_S} rgn参数结构体
     */     
    HiRgnNeedParam_S set_rgn(Osd::OverplayInfo_S stuOverplayInfo, int nChn, uint32_t unHandle, uint32_t unWidth = 0, uint32_t unHeight = 0);

    /*** 
     * @description : 销毁rgn
     * @author      : huangjunda
     * @return       {*}
     */    
    void destroy_rgn();

    /***
     * @description : osd显示时间信息线程
     * @author      : huangjunda
     * @param        {void} *args
     * @return       {void}
     */
    void osd_show_time();

    /***
     * @description : osd显示其他信息线程
     * @author      : huangjunda
     * @param        {void} *args
     * @return       {void}
     */
    void osd_show_others();
  
    /*** 
     * @description : osd闪烁线程
     * @author      : huangjunda
     * @return       {*}
     */    
    void osd_flicker();

    /**
     * @brief   : 抓拍叠加信息RGN线程
     */
    void osd_show_capture();

    /**
     * @brief   : 展会面板RGN线程
     * @return   {void}
     */
    void osd_show_exhibition_panel();

    /**
     * @brief   : 处理抓拍叠加信息RGN的辅助函数
     * @param    {vector<Osd::OverplayInfo_S>} &vecOverplayCaptureInfo 抓拍叠加信息
     * @param    {ShareInfo_S} &stShareInfo 其他信息
     */
    void process_capture_rgns(const std::vector<Osd::OverplayInfo_S> &vecOverplayCaptureInfo,
                              const Osd::ShareInfo_S &stShareInfo);

    /**
     * @brief   : 更新抓拍叠加信息
     * @param    {OverlayInfo_S} &stInfo rgn叠加参数
     * @param    {vector<Osd::OverplayInfo_S>} &vecOverplayCaptureInfo 抓拍叠加信息
     */
    void update_capture_overlay_info(const Alarm::OverlayInfo_S &stInfo, std::vector<Osd::OverplayInfo_S> &vecOverplayCaptureInfo);

    /***
     * @description : 获取模板文本信息
     * @author      : huangjunda
     * @param        {OverplayInfo_S} stuOverplayInfo 模板信息
     * @param        {ShareInfo_S} stuShareInfo 共用信息
     * @return       {std::string} 文本字符串
     */
    std::string get_template_text(Osd::OverplayInfo_S stuOverplayInfo, Osd::ShareInfo_S stuShareInfo);

    /***
     * @description : 获取参考尺寸宽高
     * @author      : huangjunda
     * @param        {ReferenceSize_E} enReferenceSize 参考分辨率大小
     * @param        {int} &nWidth 参考分辨率宽度
     * @param        {int} &nHeight 参考分辨率高度
     * @return       {void}
     */
    void get_reference_size(Osd::ReferenceSize_E enReferenceSize, int &nWidth, int &nHeight);

    /***
     * @description : 获取真实字体大小
     * @author      : huangjunda
     * @param        {int} nFontSize 参考字体大小
     * @param        {int} nActualSize 真实分辨率大小
     * @param        {int} nReferenceSize 参考分辨率大小
     * @return       {int} 真实字体大小
     */
    int calculate_text_size(int nFontSize, int nActualSize, int nReferenceSize);

    /**
    * @brief   : 计算模板宽度
    * @note    : 通过参考字体计算
    * @param    {int} nWith 文本字符串长度
    * @param    {int} nFontSize 参考字体大小
    * @param    {int} nActualWidth 实际分辨率宽度
    * @param    {int} nReferenceWidth 参考分辨率宽度
    * @return   {int} 模板宽度
    */
    int calculate_template_width(int nWith, int nFontSize, int nActualWidth, int nReferenceWidth);

    /**
    * @brief   : 计算模板宽度
    * @note    : 通过参考分辨率宽度来计算
    * @param    {int} nWith 文本字符串长度
    * @param    {int} nActualWidth 实际分辨率宽度
    * @param    {int} nReferenceWidth 参考分辨率宽度
    * @return   {int} 模板宽度
    */
    int calculate_template_width(int nWith, int nActualWidth, int nReferenceWidth);

    /***
     * @description : 计算模板高度
     * @author      : huangjunda
     * @param        {int} nFontSize 参考字体大小
     * @param        {int} nActualHeight 实际分辨率高度
     * @param        {int} nReferenceHeight 参考分辨率高度
     * @return       {int} 模板高度
     */
    int calculate_template_height(int nFontSize, int nActualHeight, int nReferenceHeight);

    /*** 
     * @description : 
     * @author      : huangjunda
     * @param        {OverplayInfo_S} stuOverplayInfo
     * @param        {int} &nActualHorMargin
     * @param        {int} &nActualVerMargin
     * @param        {int} nActualWidth
     * @param        {int} nActualHeight
     * @param        {int} nReferenceWidth
     * @param        {int} nReferenceHeight
     * @return       {*}
     */
    void get_template_margin(Osd::OverplayInfo_S stuOverplayInfo, int &nActualHorMargin, int &nActualVerMargin, int nActualWidth, int nActualHeight, int nReferenceWidth, int nReferenceHeight);

    /***
     * @description : 获取模板起始坐标点
     * @author      : huangjunda
     * @param        {OverplayInfo_S} stuOverplayInfo 模板信息
     * @param        {HiRgnNeedParam_S} stuRgnNeedParam 模板需要参数
     * @param        {int} nActualWidth 实际分辨率宽度
     * @param        {int} nActualHeight 实际分辨率高度
     * @param        {int} nReferenceWidth 参考分辨率宽度
     * @param        {int} nReferenceHeight 参考分辨率高度
     * @return       {void}
     */
    void get_start_points(Osd::Align_E enAlign, int &nX, int &nY, int nActualHorMargin, int nActualVerMargin, int nTemplateWidth, int nTemplateHeight, int nActualWidth, int nActualHeight);

    /***
     * @description : 透明度由百分比转换到rgb
     * @author      : huangjunda
     * @param        {int} nAlpha
     * @return       {int} rgb范围(0~255)
     */
    int algha_percentage_to_rgb(uint32_t nAlpha);

    /***
     * @description : 画文本
     * @author      : huangjunda
     * @param        {HiRgn_S} *pHandle
     * @param        {char} *pText
     * @param        {int} nLength
     * @return       {void}
     */
    void draw_text(HiRgn_S *pHandle, Osd::OverplayInfo_S stuOverplayInfo, const char *pText);

    /**
     * @brief   : 画框
     * @param    {HiRgn_S} *pHandle：RGN句柄
     * @param    {OverplayInfo_S} stuOverplayInfo：overplay 配置
     * @param    {vector<Common::RectInfo_S>} &vRectInfo：框信息
     */
    void draw_rect(HiRgn_S *pHandle, Osd::OverplayInfo_S stuOverplayInfo, const std::vector<Common::RectInfo_S> &vRectInfo);

    /* 线程运行标志 */
    std::atomic<bool> m_bIsRunning;
    /* 互斥锁 */
    std::mutex m_mutex;
    /* rgn句柄（一个venc通道最多有8个叠加区域） */
    std::vector<HiRgn_S *> m_pVecRgns;
    /* OSD显示时间信息线程 */
    std::thread m_showTimeThread;
    /* OSD显示其他信息线程 */
    std::thread m_showOthersThread;
    /* OSD闪烁线程 */
    std::thread m_flickerThread;
    /* OSD抓拍叠加信息线程 */
    std::thread m_showCaptureThread;

    /* 展会版左上角面板刷新线程 */
    std::thread m_showExhibitionPanelThread;
    /* 时间信息rgn是否需要更新 */
    bool m_bIsTimeUpdate;
    /* 其他信息rgn是否需要更新 */
    bool m_bIsOthersUpdate;
    /* 用于绘制检测框覆盖图片的Surface。键: RGN 句柄 (unHandle), 值: 指向该 RGN 的 SDL_Surface 的指针 */
    std::map<int, SDL_Surface*> m_mapOverlaySurfaces;
    /* 时间线程 提前多少ms唤醒，进行绘制 */
    static constexpr long WAKEUP_ADVANCE_US = 200000; /* 200毫秒 */
};
