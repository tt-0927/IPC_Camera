/*** 
 * @FilePath     : visionText_analysis.hpp
 * @Author       : cyc
 * @Date         : 2025-09-22 13:43:33
 * @LastEditors  : cyc
 * @LastEditTime : 2025-10-15 16:27:48
 * @Description  : 画面分析跟文字输入模型
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>
#include <queue>
#include <algorithm>
#include <regex>
#include <sys/time.h>
#include <sys/resource.h>
#include "common_process.h"
#include "blocking_queue.hpp"
#include "common_process.h"
#include "LLMInferenceRK.hpp"
#include "stream_video.h"
#include "stream_vpss.h"
#include <opencv2/opencv.hpp>
#include "ImageEnoderInferenceRK.hpp"
#include "event_manager.hpp"
#include "stream_process_ext.hpp"
#include "task_publish.h"
#include "common_vlm.hpp"


#define IMAGE_TOKEN_NUM 256
#define EMBED_SIZE 896
//#define EMBED_SIZE 1536

#define TIMEOUT_0_MS 0
#define TIMEOUT_5_MS 5
#define TIMEOUT_1000_MS 1000

/* 默认检测频率 0.5秒*/
#define DETECT_FREENCY_TIME (500)
/*流式推理结果阈值为空次数上限*/
#define EMPTY_COUNT_MAX 25

/*画面分析定时分析，固定提问*/
#define  IMAGR_ANALYSIS_SCFEDULE_INPUTTEXT  "使用中文描述一下这张图片,控制在100字以内"

/* 最大允许同时存在的会话数*/
#define MAX_SESSION_SIZE 150
/* 单个对话的最大历史记录数*/
#define MAX_RECORD_INFO_SIZE 200

enum NewSessionType
{
   THIS_SESSION,   //此次新会话：对话前点击，或输出完成后点击
   NEXT_SESSION,    //下次新会话：推理/输出过程中点击
   POSTPONED_SESSION,  //其他推理任务关闭的下一次对话，新会话
   NULL_TYPE
};


// 异步IO任务包，包含所有业务结果数据
struct IoTask {
    enum Type 
    { 
        TEXT_PRESET,                   //文字预设任务
        TEXT_PRESET_RETURN_QUESTION,   //文字预设返回提问任务
        TEXT_PRESET_RESULT_UPDATE,     //文字预设结果更新任务
        TEXT_PRESET_DETAIL_UPDATE,     //文字预设详细分析结果更新任务
        ALERT_PUSH_RECORD_UPDATE,      //预警推送记录任务
        IMAGE_ANALYSIS,                //画面分析记录更新任务处理
        SCREENSHOT_ONLY,               //画面分析-截图任务处理
        MOVE_IMAGE                     //画面分析-移动图片文件任务处理
    }type;

    std::string timeKey;        // 时间戳
    std::string outputTimeKey;  // 输出时间戳
    std::string imagePath;      // 路径
    std::string prompt;         // 提问词
    std::string result;         // 推理结果
    bool isText;                // 是否纯文本
    bool bSkipDelete;           // 是否跳过删除记录文件(预设任务推理符合情况记录的文件由预警推送来最后来删除)
    bool bScheduleEnable;       // 画面分析实时分析或者定时分析-bScheduleEnable为true定时分析
    bool bDetailedAnalysis;     // 是否属于文字预设详细分析
    bool isUploadImage;         // 是否上传图片分析
    bool bImplicitImageIntent;  // 是否实时分析-意图分析（抓取画面分析）
    std::vector<unsigned char> jpegData;   // 压缩后的图片数据
};

class CVisionText
{
public:
    CVisionText();
    ~CVisionText();

    /**
    * @brief   : 更新画面分析参数
    * @param    {LLMImageAnalysis_S} &stAlgoCfg：画面分析
    */
    void setAlgoParamCfg(const Alarm::LLMImageAnalysis_S &stAlgoCfg);

    /**
    * @brief   : 更新文字预设任务参数
    * @param    {TextPreset_S} &stAlgoCfg：画面分析
    */
    void setAlgoParamCfg(const Alarm::TextPreset_S &stAlgoCfg);

    /**
    * @brief   : 更新算法配置参数
    * @param    {AlgorithmConfig} &stAlgoConfig：算法配置
    */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig);

    /**
    * @brief   : 接受媒体数据
    * @param    {MediaData_S} stMediaData：媒体数据
    */
    void recvMediaData(MediaData_S stMediaData);

    /**
     * @brief   : 输出结果回调函数
     */
    int onStreamOutput(const std::string& text, bool is_finished);

private:

    /**
    * @brief  停止大模型所有推理任务
    * @return 成功返回true，失败返回false
    */
    bool stop();
    /**
    * @brief  开启大模型所有推理任务
    * @return 成功返回true，失败返回false
    */
    bool start();

    /**
    * @brief 初始化大模型
    * @return 成功返回true，失败返回false
    */
    bool init();

    /**
    * @brief 去初始化大模型
    * @return 成功返回true，失败返回false
    */
    bool uninit();

    /**
    * @brief 初始化LLM模型
    * @return 成功返回true，失败返回false
    */
    bool init_Llm();

    /**
    * @brief 反初始化LLM模型
    * @return 成功返回true，失败返回false
    */
    bool unInit_Llm();

    /**
    * @brief 初始化RKNN模型
    * @return 成功返回true，失败返回false
    */
    bool init_Rknn();

    /**
    * @brief 反初始化rknn模型
    * @return 成功返回true，失败返回false
    */
    bool unInit_Rknn();

    /**
    * @brief 推理记录更新
    * @param RecordInfo 推理结果数据
    * @param bNewSession 是否开启新对话 (true: 创建新会话插在最前, false: 追加到当前最新会话)
    * @return 成功返回0，失败返回非0
    */
    int UpdateImageAnalysisRecord(Alarm::AnalysisRecords_S& RecordInfo, bool bNewSession);

    /**
    * @brief 执行画面分析任务
    * @param isText 是否纯文本推理
    * @param cfg 画面分析配置
    * @param timeStr 时间
    * @param imagePath 图片路径
    * @param isUpload 是否上传图片分析
    * @param pImgVec 图像向量
    */
    void execute_ImageAnalysis_Task(bool isText, Alarm::LLMImageAnalysis_S cfg, const std::string& timeStr, 
                                    const std::string& imagePath, bool isUpload, float* pImgVec);

    /**
    * @brief   : 执行文字预设任务
    * @param cfg 文字预设配置
    * @param timeStr 时间
    * @param {float*} pImgVec：图像向量
    * @param {cv::Mat&} Frame：原始BGR图像数据
    * @return   {*}
    */
    void execute_TextPreset_Task(Alarm::TextPreset_S cfg, const std::string& timeStr,float* pImgVec, cv::Mat& Frame);

    /**
    * @brief 执行详细的图片分析
    * @param cfg 文字预设配置
    * @param timeStr 时间
    * @param pImgVec 图像向量
    * @return 详细的图片描述文本
    */
    std::string perform_DetailedImage_Analysis(const Alarm::TextPreset_S& cfg, const std::string& timeStr, float* pImgVec);

    /**
    * @brief 获取当前模式对应的LLM配置文件路径
    * @return 配置文件路径
    */
    std::string getLlmConfigPath();

    /**
    * @brief 线程函数
    */
    void run();

    /**
    * @brief 处理非推理任务，保存图片，更新记录任务线程函数
    */
    void ioWorker();

    /**
    * @brief 检查是否可以执行文字预设任务（基于时间间隔）
    * @return true可以执行，false需要等待
    */
    bool check_TextPresetTask();

    /**
    * @brief 保存实时预警推送记录
    * @param taskConfig 任务配置
    * @param strResult 检测结果
    * @param timeStr 时间戳
    * @param imagePath 推理图片
    */
    void save_realAlarmPush_record(const Alarm::TextPreset_S& taskConfig, const std::string& strResult, 
                                   const std::string& timeStr, const std::string& imagePath);


    /**
    * @brief 获取指定格式时间
    * @return 指定格式时间
    */
    std::string updateCreateTime();

private:

    /* LLM推理引擎 */
    std::unique_ptr<Inference_NS::CLLMInferenceRK> m_pLlmHandle; 

    /* RKNN模型实例 */
    std::unique_ptr<CImageEnoderInferenceRK> m_pRknnHandle;

    /*推理所需图片向量数据 */
    std::vector<float> m_imgVec;
    /*异步IO任务处理线程 */
    std::thread m_ioThread;
    /*IO任务队列，存储待处理的IO任务 */
    std::queue<IoTask> m_ioQueue;
    /*保护IO任务队列的互斥锁 */
    std::mutex m_ioMutex;
    /*线程同步条件变量，用于唤醒IO线程 */
    std::condition_variable m_ioCondition;

    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dataQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning = false;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread             m_thread;
    /* 检测频率控制 */
    EventManager m_RecvManager;

    /* 配置参数 */
    /* 场景智能分析总控制 */
    Alarm::LLMAISceneAnalysis_S m_stAISceneAnalysisCfg;
    /* 画面分析 */
    Alarm::LLMImageAnalysis_S m_stLLmImageCfg;
    /* 文字预设任务 */
    Alarm::TextPreset_S m_stTextPreseCfg;
  
    /*是否上传图片文件*/
    std::atomic<bool> isUploadImage = false;
    /* 是否新对话 */
    std::atomic<bool> m_bNewDialogue = false;
    /* 是否中断推理 */
    std::atomic<bool> m_bAnalysisStop = false;
    /* 记录新会话状态（用于实时分析） */
    std::atomic<NewSessionType> m_newSessionType = THIS_SESSION;
    /* 回显网页输入分析的文本内容 */
    std::string strAnalysisInputText;
    /* 回显网页图片路径 */
    std::string m_currentStreamImagePath;
    /* 输出结果的时间 */
    std::string m_strOutputCreateTime = "";
    /* 文字预设任务是否裁剪画面 */
    bool m_bIsCrop = false;

    /* 模型初始化状态 */
    std::atomic<bool> m_bAiInitialized = false;

    /*流式输出回调*/
    Inference_NS::StreamCallback m_callback;

    /* 文字预设任务时间控制 */
    std::chrono::steady_clock::time_point m_lastTextPresetTime;  /* 上次执行文字预设任务的时间 */
    std::atomic<long long> m_textPresetIntervalMs = 30000;       /* 文字预设任务执行间隔（毫秒），默认30秒 */
};
