/*** 
 * @FilePath     : cover_draw.h
 * @Author       : huangjunda
 * @Date         : 2025-06-20 11:21:31
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-08-27 10:26:11
 * @Description  : 
 */

#pragma once

#include <atomic>
#include <thread>

#include "osd_define.h"
#include "Singleton.h"
#include "IpcRet.h"
#include "dlog.h"
#include "osd_manage.h"

class CCoverDraw : public CSingleton<CCoverDraw>
{
    CCoverDraw();

public:
    virtual ~CCoverDraw();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<CCoverDraw>;

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
 
    /**
     * @brief   : 设置更新标志
     * @param    {bool} bIsUpdate
     */
    void set_update_flag(bool bIsUpdate);

private:
    /***
     * @description : 启动
     * @author      : huangjunda
     * @return       {*}
     */
    void start();

    /***
     * @description : 停止
     * @author      : huangjunda
     * @return       {*}
     */
    void stop();
   
    /**
     * @brief   : osd遮挡函数-线程函数
     */
    void osd_cover();

    /*** 
     * @description : 设置rgn参数
     * @author      : huangjunda
     * @param        {CoverInfo_S} stuCoverInfo
     * @param        {int} nChn
     * @param        {uint32_t} unHandle
     * @return       {HiRgnNeedParam_S} rgn参数结构体
     */     
   RkRgnNeed_S set_rgn(Osd::CoverInfo_S stuCoverInfo, int nChn, uint32_t unHandle);

    /*** 
     * @description : 销毁rgn
     * @author      : huangjunda
     * @return       {*}
     */    
    void destroy_rgn();

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
     * @description : 计算模板宽高
     * @author      : huangjunda
     * @param        {vector<Osd::CoordinateInfo_S>} stuCoordinate
     * @param        {int} &nTemplateWidth
     * @param        {int} &nTemplateHeight
     * @param        {int} nActualWidth 实际分辨率宽度
     * @param        {int} nActualHeight 实际分辨率高度
     * @param        {int} nReferenceWidth 参考分辨率宽度
     * @param        {int} nReferenceHeight 参考分辨率高度
     * @return       {*}
     */    
    void calculate_template_size(std::vector<Osd::CoordinateInfo_S> stuCoordinate, uint32_t &unTemplateWidth, uint32_t &unTemplateHeight, int nActualWidth, int nActualHeight, int nReferenceWidth, int nReferenceHeight);

    /*** 
     * @description : 计算坐标
     * @author      : huangjunda
     * @param        {uint32_t} &unStartX
     * @param        {uint32_t} &unStartY
     * @param        {int} nActualWidth
     * @param        {int} nActualHeight
     * @param        {int} nReferenceWidth
     * @param        {int} nReferenceHeight
     * @return       {*}
     */    
    void calculate_coordinate(std::vector<Osd::CoordinateInfo_S> &stuCoordinate, int nActualWidth, int nActualHeight, int nReferenceWidth, int nReferenceHeight);

    /* 线程运行标志 */
    std::atomic<bool> m_bIsRunning;
    /* rgn句柄（一个vpss通道最多有8个遮挡区域） */
    std::vector<RkRgn_S *> m_pVecRgns;
    /* 遮挡线程 */
    std::thread m_thread;
    /* rgn是否需要更新 */
    bool m_bIsUpdate;
    /*Cover线程可运行-标志*/
    std::atomic<bool> m_bCoverThread[MAX_OSD_OVERLAY_NUM ];
};
