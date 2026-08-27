/**
 * @FilePath     : capture_ctrl.h
 * @Author       : 梁浩尧 lianghaoyao@kfb.cn
 * @Date         : 2025-07-17 17:44:36
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-13 15:03:25
 * @Description  : 抓图计划管理
 */

#include <map>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <thread>
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
        unsigned long long ullLastCaptureTime = 0;         /* 上次实际抓图时间戳(ms)，用于抓图节拍控制 */
        unsigned long long ullLastTriggerTime = 0;         /* 上次事件触发时间戳(ms)，用于重复触发过滤 */
        Event::Info_S stEventInfo;                         /* 事件信息 */
    } EventCaptureState_S;

    /* 抓图落盘任务：JPEG 图片数据的独立拷贝，由工作线程消费 */
    typedef struct CaptureTask
    {
        std::vector<unsigned char> vecJpegData; /* JPEG 图片数据 */
    } CaptureTask_S;

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
     * @brief   : 送jpeg数据（异步投递到抓图工作线程，不执行文件I/O）
     * @param    {unsigned char} *pData 图片数据
     * @param    {int} nDataLen 数据长度
     * @return   {int} 0：成功，非0：失败
     * @note    : 本接口仅做内存拷贝与队列投递，耗时可忽略，
     *            可安全运行在流媒体取流线程等对延迟敏感的场景
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
    int write_to_file(std::string filePath, unsigned char *pData, int nDataLen);
private:
    /**
     * @brief   : 线程函数 检测定时抓图计划
     */
    void run();

    /**
     * @brief   : 抓图落盘工作线程：消费队列中的JPEG数据并执行抓图
     * @note    : 文件写入与数据库操作在此线程串行执行，避免阻塞流媒体取流线程
     */
    void capture_worker_run();

    /**
     * @brief   : 处理单次抓图任务（原 send_frameData 的三阶段逻辑）
     * @param    {unsigned char} *pData：JPEG图片数据
     * @param    {int} nDataLen：数据长度
     * @note    : 仅在抓图工作线程内调用，事件状态快照与文件写入串行化
     */
    void process_capture_task(unsigned char *pData, int nDataLen);

    /**
     * @brief   : 抓图任务队列（JPEG取流线程投递，工作线程消费）
     * @note    : 有界队列，满时丢弃最旧任务；pop 支持超时与退出唤醒
     */
    class CCaptureTaskQueue
    {
    public:
        /**
         * @brief   : 构造函数
         * @param    {int} nMaxSize：队列最大容量
         */
        explicit CCaptureTaskQueue(int nMaxSize);

        /**
         * @brief   : 投递任务
         * @param    {CaptureTask_S} &stTask：任务（包含图片数据拷贝）
         * @return   {bool} true：投递成功，false：队列已满或已退出
         */
        bool push(const CaptureTask_S &stTask);

        /**
         * @brief   : 阻塞取出任务
         * @param    {CaptureTask_S} &stTask：输出任务
         * @param    {int} nTimeoutMs：阻塞超时时间(ms)
         * @return   {bool} true：取出成功，false：超时或已退出
         */
        bool pop(CaptureTask_S &stTask, int nTimeoutMs);

        /**
         * @brief   : 唤醒阻塞线程并标记退出，退出后不可再投递
         */
        void exit();

        /**
         * @brief   : 清除退出标志，支持模块反初始化后重新初始化
         */
        void reset();

    private:
        std::mutex m_mutex;                    /* 保护队列的互斥锁 */
        std::condition_variable m_condition;   /* 队列非空通知 */
        std::queue<CaptureTask_S> m_queueData; /* 任务队列 */
        int m_nMaxSize;                        /* 队列最大容量 */
        bool m_bExit = false;                  /* 退出标志 */
    };

    /**
     * @brief   : 写入图片数据
     * @param    {std::string} filePath 文件路径
     * @param    {unsigned char} *pData 图片数据
     * @param    {int} nDataLen 数据长度
     * @return   {int} 文件大小（字节），<0 失败
     */
    

    /**
     * @brief   : 抓图
     * @param    {CaptureType_E} eCaptureType：抓图类型
     * @param    {unsigned char} *pData：图片数据
     * @param    {int} nDataLen：数据长度
     * @param    {Event::Info_S} &stEventInfo：事件信息快照（仅事件抓图使用）
     * @param    {unsigned int} unCaptureCount：当前抓图序号（仅事件抓图使用，用于文件名）
     * @param    {Event::Type_E} enEventType：事件类型（仅事件抓图使用）
     * @return   {string} 图片路径
     * @note    : 调用者须在无 m_mutex 保护下调用此函数；所有必要数据通过参数传入
     */
    std::string capture_image(Capture_NS::CaptureType_E eCaptureType,
                              unsigned char *pData,
                              int nDataLen,
                              const Event::Info_S &stEventInfo,
                              unsigned int unCaptureCount = 0,
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

      /* 串行保护抓图配额检查、旧图清理和新图写入 */
      std::mutex m_storageMutex;


    /* 用于人脸抓拍文件名的锁 */
    std::mutex m_faceMutex;
    /* 用于人脸抓拍的条件变量 */
    std::condition_variable m_faceCv;

    /*是否停止检测抓图计划线程函数*/
    std::atomic_bool m_bRun = false;

    /* 抓图落盘工作线程运行标志 */
    std::atomic_bool m_bWorkerRun = false;

    /* 抓图落盘工作线程 */
    std::thread m_captureWorkerThread;

    /* 抓图落盘任务队列（JPEG取流线程投递，工作线程消费） */
    CCaptureTaskQueue m_taskQueue;

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

    /* 抓图任务队列最大容量（按JPEG约1张/秒、单张约200KB估算，缓存4张足够吸收落盘延迟） */
    static constexpr int CAPTURE_QUEUE_MAX_SIZE = 4;

    /* 抓图工作线程队列取出超时时间（毫秒），用于周期性检查退出标志 */
    static constexpr int CAPTURE_QUEUE_POP_TIMEOUT_MS = 1000;

    /* 人脸抓拍当前图片文件名 */
    std::string m_strFaceCaptureFile;
};