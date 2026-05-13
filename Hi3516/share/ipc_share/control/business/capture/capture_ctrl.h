/**
 * @FilePath     : capture_ctrl.h
 * @Author       : 梁浩尧 lianghaoyao@kfb.cn
 * @Date         : 2025-07-17 17:44:36
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-28 15:35:33
 * @Description  : 抓图计划管理
 */

#include <map>
#include <condition_variable>
#include <vector>
#include <atomic>
#include "Singleton.h"
#include "capture_define.h"
#include "event_define.h"
#include "IpcRet.h"

/* 获取jpeg编码通道参数回调定义 */
using GetjpegVencParamCallback = std::function<int(unsigned int &unWidth, unsigned int &unHeight, unsigned int &nUqFactor)>;

class CCaptureCtrl : public CSingleton<CCaptureCtrl>
{
    CCaptureCtrl();

public:
    /* 一天抓图计划的信息 */
    typedef struct Info
    {
        /* 星期几 */
        int nDayOfWeek = -1;
        /* 主图计划时间段 */
        std::vector<Capture_NS::CaptureTime_S> captureTimes;
    } Info_S;

    /* 单个事件抓图状态信息 */
    typedef struct EventCaptureState
    {
        Event::Type_E enType = Event::Type::MOTION_DETECT; /* 事件类型 */
        bool bCaptureFlag = false;                         /* 是否正在抓图 */
        unsigned int unCaptureCount = 0;                   /* 当前抓图数量 */
        unsigned long long ullLastCaptureTime = 0;         /* 上次抓图时间戳(ms) */
        Event::Info_S stEventInfo;                         /* 事件信息 */
    } EventCaptureState_S;

    typedef struct FrameInfo
    {
        unsigned char *pCaptrueData = NULL;
        int nDataLen = 0;
    } FrameInfo_S;

    ~CCaptureCtrl();
    friend class CSingleton<CCaptureCtrl>;

    /**
     * @brief 初始化抓图模块
     * @return IpcRet_E <0:失败, >=0:成功
     */
    IpcRet_E init();

    /**
     * @brief 去初始化抓图模块
     * @return IpcRet_E <0:失败, >=0:成功
     */
    IpcRet_E deinit();

    /**
     * @brief   : 获取jpeg编码通道参数回调
     * @param    {GetjpegVencParamCallback} &callback 配置回调函数
     * @return   {int} 0：成功，非0：失败
     */
    int get_jpegVencParamCallback(const GetjpegVencParamCallback &callback);

    /**
     * @brief   : 更新抓图计划
     */
    void update_capturePlan();

    /**
     * @brief   : 更新抓图参数
     */
    void update_captureParam();

    /**
     * @brief   : 获取抓图参数
     * @param    {Capture_NS::CaptureParam_S} &stCaptureParams 抓图参数
     */
    void get_captureParam(Capture_NS::CaptureParam_S &stCaptureParams);

    /**
     * @brief: 启动抓图
     * @return
     */
    void start_capture();

    /**
     * @brief: 停止抓图
     * @return
     */
    void stop_capture();

    /**
     * @brief   : 送jpeg数据
     * @param    {unsigned char} *pData 图片数据
     * @param    {int} nDataLen 数据长度
     * @return   {int} 0：成功，非0：失败
     */
    int send_frameData(unsigned char *pData, int nDataLen);

    /**
     * @brief   : 设置事件抓图信息
     * @param    {bool} bEventEnded 事件是否开始（true：开始，false：结束）
     * @param    {const Event::Info_S} &stEventInfo 事件信息
     * @return   {int} 0：成功，非0：失败
     */
    int set_event_capture(bool bEventEnded, const Event::Info_S &stEventInfo);

    /**
     * @brief   : 获取事件抓图状态
     * @return   {bool} true：有事件正在抓图，false：无事件抓图
     */
    bool get_event_capture_status();

    /**
     * @brief   : 获取指定事件类型的首张图片的抓图状态
     * @param    {Type_E} enType 指定的事件类型
     * @return   {bool} true：抓图完毕，false：未抓图完毕
     */
    bool get_event_first_capture_status(const Event::Type_E enType, std::string &strFirstPath);

    /**
     * @brief   : 获取定时抓图状态
     * @return   {bool} true：开启，false：关闭
     */
    bool get_timing_capture_status();

    /**
     * @brief   : 获取按日期分类的存储路径
     * @return   {std::string} 存储路径
     */
    std::string get_date_storage_path();

    /**
     * @brief   : 确保目录存在
     * @param    {std::string} &path 目录路径
     * @return   {bool} true：成功，false：失败
     */
    bool ensure_directory_exists(const std::string &path);

    /**
     * @brief   : 获取人脸抓拍当前全景图片文件名
     * @return   {string} 人脸抓拍当前全景图片文件名
     */
    std::string get_face_capture_file();

private:
    /**
     * @brief   : 线程函数 检测定时抓图计划
     */
    void run();

    /**
     * @brief   : 写入图片数据
     * @param    {std::string} filePath 文件路径
     * @param    {unsigned char} *pData 图片数据
     * @param    {int} nDataLen 数据长度
     * @return   {int} 文件大小（字节），<0 失败
     */
    int write_to_file(std::string filePath, unsigned char *pData, int nDataLen);

    /**
     * @brief   : 抓图
     * @param    {Capture_NS::CaptureType_E} eCaptureType 抓图类型
     * @param    {unsigned char} *pData 图片数据
     * @param    {int} nDataLen 数据长度
     * @param    {Event::Type_E} enEventType 事件类型（仅事件抓图需要）
     * @return   {string} 图片路径
     */
    std::string capture_image(Capture_NS::CaptureType_E eCaptureType,
                              unsigned char *pData,
                              int nDataLen,
                              Event::Type_E enEventType = Event::Type::SCREENSHOT);

    /**
     * @brief   : 循环抓图-删除旧图片
     * @return   {int} 0：成功，非0：失败
     */
    int delete_old_images();

private:
    /*存储一周抓图的计划*/
    std::vector<Info_S> m_infos;

    /* 定时和事件触发抓图参数 */
    Capture_NS::CaptureParam_S m_captureParams;

    /*是否停止抓图判断字段*/
    bool m_start = false;

    /*是否停止抓图判断字段*/
    bool m_stop = false;

    /*用于保护共享资源的互斥锁*/
    std::mutex m_mutex;

    /* 用于人脸抓拍文件名的锁 */
    std::mutex m_faceMutex;
    /* 用于人脸抓拍的条件变量 */
    std::condition_variable m_faceCv;

    /*是否停止检测抓图计划线程函数*/
    std::atomic_bool m_bRun = false;

    /* 定时触发抓图标志位 */
    bool m_TimingCaptureFlag = false;

    /* 各事件类型的抓图状态 key:事件类型 value:抓图状态 */
    std::map<Event::Type_E, EventCaptureState_S> m_mapEventCaptureStates;

    /* 上一次定时抓图的时间戳 */
    unsigned long long m_lastTimingCaptrueTime;

    /* 获取jpeg编码通道参数回调 */
    GetjpegVencParamCallback m_GetJpegVencParamCallback;

    /* 循环抓图删除计数（每次删除多少张） */
    static constexpr int BATCH_DELETE_COUNT = 50;

    /* 人脸抓拍当前图片文件名 */
    std::string m_strFaceCaptureFile;
};