/*** 
 * @FilePath     : isp_sceneCtrl.h
 * @Author       : cyc
 * @Date         : 2025-06-20 16:19:22
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-01 11:29:41
 * @Description  : 图像场景定时控制
 */

#pragma once
#include "Singleton.h"
#include "isp_define.h"
#include <atomic>

class CSceneCtrl : public CSingleton<CSceneCtrl>
{
    CSceneCtrl();
public:

    /*当天录制时间的信息*/
    typedef struct Info
    {
        /* 当前月份 */
        int nMonthOfYear = -1;
        /* 录制时间 */
        std::vector<ISP::SceneTime_S> sceneTimes;
        
    } Info_S;

    ~CSceneCtrl();  
    friend class CSingleton<CSceneCtrl>;
    /* 更新控制 */
    void update();

private:
    /* 运行图像计划配置检测线程 */
    void run();

    ISP::SceneType_E determine_current_scene(int nMonth, int nCurTime);

private:
    /*存储一周七天录制时间的容器*/
    std::vector<Info_S> m_infos;
    /* 场景定时切换启动标志 */
    std::atomic_bool m_bStart = false;
    /* 用于保护共享资源的互斥锁 */ 
    std::mutex m_mutex;
    /*是否停止检测图像计划线程函数*/
    std::atomic_bool m_bRun = false;
    /*是否停止场景判断字段*/
    bool m_stop = false;
};