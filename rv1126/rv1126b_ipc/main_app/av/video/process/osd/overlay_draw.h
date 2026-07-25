/***
 * @FilePath     : overlay_draw.h
 * @Author       : huangjunda
 * @Date         : 2025-05-27 14:12:50
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-08-26 14:19:49
 * @Description  :
 */
#pragma once

#include <atomic>
#include <thread>
#include <map>
#include "osd_define.h"
#include "common_define.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "dlog.h"
#include "os_mutex.h"
#include "stream_venc.h"
#include "stream_vpss.h"
#include "osd_manage.h"

#include "SDL.h"
#include "SDL_ttf.h"

// 颜色转换宏（24位 RGB → BGR）
#define RGB_TO_BGR(rgb) ( \
    (((uint32_t)(rgb) & 0x0000FF) << 16) | \
    (((uint32_t)(rgb) & 0x00FF00)) | \
    (((uint32_t)(rgb) & 0xFF0000) >> 16))

class COverlayDraw : public CSingleton<COverlayDraw>
{
    COverlayDraw();

public:
    virtual ~COverlayDraw();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<COverlayDraw>;

    /***
     * @description : 初始化
     * @author      : huangjunda
     * @return       {IpcRet_E} 公共返回码
     */
    IpcRet_E init();

    /***
     * @description : 重新初始化
     * @author      :
     * @return       {IpcRet_E} 公共返回码
     */
    IpcRet_E reinit(int nChn);


    /***
     * @description : 重新去初始化
     * @author      :
     * @return       {IpcRet_E} 公共返回码
     */
    
    IpcRet_E reDeinit(int nChn);

    /***
     * @description : 去初始化
     * @author      : huangjunda
     * @return       {IpcRet_E} 公共返回码
     */
    IpcRet_E deinit();

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
     * @return       {RkRgnNeed_S} rgn参数结构体
     */     
    RkRgnNeed_S set_rgn(Osd::OverplayInfo_S stuOverplayInfo, int nChn, uint32_t unHandle, uint32_t unWidth = 0, uint32_t unHeight = 0);

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

#if CAP_EXHIBITION_OSD_PANEL
    /**
     * @brief   : 展会面板显示线程
     * @return   {void}
     */
    void osd_show_exhibition_panel();
#endif
  
    /*** 
     * @description : osd闪烁线程
     * @author      : huangjunda
     * @return       {*}
     */    
    void osd_flicker();

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

    /***
     * @description : 计算模板宽度
     * @author      : huangjunda
     * @param        {int} nLen 文本字符串长度
     * @param        {int} nFontSize 参考字体大小
     * @param        {int} nActualWidth 实际分辨率宽度
     * @param        {int} nReferenceWidth 参考分辨率宽度
     * @return       {int} 模板宽度
     */
    int calculate_template_width(int nLen, int nFontSize, int nActualWidth, int nReferenceWidth);

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
    void draw_text(RkRgn_S *pHandle, Osd::OverplayInfo_S stuOverplayInfo, const char *pText);

    /**
     * @brief   : 画框
     * @param    {HiRgn_S} *pHandle：RGN句柄
     * @param    {OverplayInfo_S} stuOverplayInfo：overplay 配置
     * @param    {vector<Common::RectInfo_S>} &vRectInfo：框信息
     */
    void draw_rect(RkRgn_S *pHandle, Osd::OverplayInfo_S stuOverplayInfo, const std::vector<Common::RectInfo_S> &vRectInfo);

    /***
     * @description : 检查中文支持
     * @author      : huangjunda
     * @param        {char} *nFontPath
     * @return       {*}
     */
    void check_chinese_support(const char *nFontPath);

    /***
     * @description : 获取字体库支持的相近字体大小
     * @author      : huangjunda
     * @param        {char} *pFontPath
     * @return       {*}
     */
    int get_font_size(int nFontSize, const char *pFontPath);

    /**
     * @description : 创建文字渲染表面
     * @author      : huangjunda
     * @param pText 需要渲染的文本内容
     * @param nWidth 输出表面的宽度 (0表示自动适应)
     * @param nHeight 输出表面的高度 (0表示自动适应)
     * @param nFontSize 字体大小
     * @param stuFontColor 文字颜色 (SDL_Color格式)
     * @param stuBackColor 背景颜色 (SDL_Color格式)
     * @param pPixelFormat 目标颜色格式
     * @param pFontPath 字体文件路径
     * @return SDL_Surface* 渲染后的表面，使用完后需要调用SDL_FreeSurface
     */
    SDL_Surface *create_text_surface(const char *pText, int nWidth, int nHeight,
                                     int nFontSize, SDL_Color stuFontColor,
                                     SDL_Color stuBackColor, SDL_PixelFormat *pPixelFormat,
                                     const char *pFontPath);

    
     /**
     * @brief   : 在 surface 上画矩形框
     * @param    {SDL_Surface} *pSurface：目标surface
     * @param    {RectInfo_S} &stRect：矩形信息
     * @param    {Uint32} u32Color 框的颜色
     * @param    {int} nThickness：线条粗细，默认为2
     */
    void draw_rect_on_surface(SDL_Surface *pSurface,const Common::RectInfo_S &stRect,Uint32 u32Color,int nThickness = 2);


    /**
     * @brief   : 生成检测框覆盖图片
     * @param    {SDL_Surface} *pSurface 目标surface
     * @param    {vector<Common::RectInfo_S>} &vRectInfo 检测框信息向量
     * @param    {SDL_Color} stColor 框的颜色
     * @param    {int} nThickness 线条粗细
     */
    void generate_detection_overlay(SDL_Surface *pSurface,const std::vector<Common::RectInfo_S> &vRectInfo,SDL_Color stColor,int nThickness);

    /**
     * @description : 保存表面为BMP文件
     * @author      : huangjunda
     * @param pSurface 需要保存的表面
     * @param filename 输出文件名
     * @return int 0成功, 非0失败
     */
    int save_surface_to_bmp(SDL_Surface *pSurface, const char *pFileName);

    /* 线程运行标志 */
    std::atomic<bool> m_bIsRunning;
    /* 互斥锁 */
    OS_MutexHndl m_stuMutex;
    // AI画框互斥锁
    std::mutex m_drawMutex;
    /* rgn句柄（一个venc通道最多有8个叠加区域） */
    std::vector<RkRgn_S *> m_pVecRgns;
    /* OSD显示时间信息线程 */
    std::thread m_showTimeThread;
    /* OSD显示其他信息线程 */
    std::thread m_showOthersThread;
#if CAP_EXHIBITION_OSD_PANEL
    /* 展会面板显示线程。 */
    std::thread m_showExhibitionPanelThread;
#endif
    /* OSD闪烁线程 */
    std::thread m_flickerThread;
    /* 时间信息rgn是否需要更新 */
    bool m_bIsTimeUpdate;
    /* 其他信息rgn是否需要更新 */
    bool m_bIsOthersUpdate;
    /* 用于绘制检测框覆盖图片的Surface。键: RGN 句柄 (unHandle), 值: 指向该 RGN 的 SDL_Surface 的指针 */
    std::map<int, SDL_Surface*> m_mapOverlaySurfaces;
    /*Overlay线程可运行-标志*/
    std::atomic<bool> m_bOverlayThread[MAX_OSD_OVERLAY_NUM * VPSS_CHN_MAX];
};
