/**
 * @FilePath     : llm_analysis.hpp
 * @Author       : leiyy
 * @Date         : 2025-09-10
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-09-10
 * @Description  : 多模态LLM推理任务(支持文本和图像输入)
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <sys/time.h>

#include "blocking_queue.hpp"
#include "LLMInferenceRK.hpp"
#include "stream_video.h"
#include "stream_vpss.h"
#include <opencv2/opencv.hpp>
#include "ImageEnoderInferenceRK.hpp"
#include "inference_record.hpp"
#include "algorithm.hpp"
#include "event_configure.h"

#define IMAGE_HEIGHT 448
#define IMAGE_WIDTH 448
#define IMAGE_TOKEN_NUM 256
#define EMBED_SIZE 896
#define VPSS_AI_IMAGE_WIDTH  PIXEL_WIDTH_AI
#define VPSS_AI_IMAGE_HEIGHT PIXEL_HEIGHT_AI

#define TIMEOUT_1000_MS 1000

/*推理记录最大数量*/
#define MAX_HISRY_NUM  10
/*推理记录存放路径*/
#define RES_HISRY_PATH  "/mnt/model/rkllm/llm_records/"

/*rkllm模型配置文件路径*/
#define RKLLM_MODEL_CONFIG_PATH     "/mnt/model/rkllm/aiRkllmConfig.json"
/*rknn视觉模型配置文件路径*/
#define RKNN_MODEL_CONFIG_PATH     "/mnt/model/rkllm/aiImageRknnConfig.json"


class CLLmInference : public CAlgorithm
{
public:
    CLLmInference();
    ~CLLmInference();

     /**
     * @brief   : 接受媒体数据
     * @param    {MediaData_S} stMediaData：媒体数据
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数
     * @param    {AlgorithmConfig} &stAlgoConfig：算法配置
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新模型推理画面分析参数 
     * @param    {LLMImageAnalysis_S} &stAlgoCfg：模型推理
     */
    void setAlgoParamCfg(const Alarm::LLMImageAnalysis_S &stAlgoCfg);

    /**
     * @brief   : 开始运行LLM推理 
     */
    void StartLLMInferenceTask();

     /**
     * @brief   : 获取推理历史记录
     * @return 推理历史管理器引用
     */
    InferenceHistory& getInferenceHistory();

    /**
     * @brief   : 获取所有推理记录
     * @return 所有推理记录的向量
     */
    std::vector<InferenceRecord> getAllRecords() const;

     /**
     * @brief   : 根据标签筛选记录
     * @param tag 标签
     * @return 匹配的记录列表
     */
    std::vector<InferenceRecord> getRecordsByTag(const std::string& tag) const;

    /**
     * @brief   : 导出记录到CSV文件
     * @param file_path 文件路径
     * @return 成功返回true，失败返回false
     */
    bool exportRecordsToCSV(const std::string& file_path) const;

    /**
     * @brief   : 清空所有记录
     */
    void clearAllRecords();

    /**
     * @brief   : 根据ID获取特定记录
     * @param id 记录ID
     * @return 特定记录
     */
    InferenceRecord getRecord(int id) const;

    /**
     * @brief   : 发送文本查询请求
     * @param    {std::string} text：查询文本
     * @param    {int} channel：业务通道号
     * @return   {bool} 成功返回true，失败返回false
     */
    bool sendTextQuery(const std::string& text, int channel = 0);

    /**
     * @brief   : 发送图像查询请求
     * @param    {cv::Mat} image：查询图像
     * @param    {std::string} text：查询文本
     * @param    {int} channel：业务通道号
     * @return   {bool} 成功返回true，失败返回false
     */
    bool sendImageQuery(const cv::Mat& image, const std::string& text, int channel = 0);

    /**
     * @brief   : 从VPSS通道发送查询请求
     * @param    {int} vpssChannel：VPSS通道号
     * @param    {std::string} text：查询文本
     * @param    {int} channel：业务通道号
     * @return   {bool} 成功返回true，失败返回false
     */
    bool sendVpssQuery(int vpssChannel, const std::string& text, int channel = 0);

    /**
     * @brief   : 从本地文件发送查询请求
     * @param    {std::string} imagePath：图像文件路径
     * @param    {std::string} text：查询文本
     * @param    {int} channel：业务通道号
     * @return   {bool} 成功返回true，失败返回false
     */
    bool sendLocalImageQuery(const std::string& imagePath, const std::string& text, int channel = 0);

private:

    /**
     * @brief 初始化
     * @return 成功返回true，失败返回false
     */
    bool init();

    /**
     * @brief 反初始化
     * @return 成功返回true，失败返回false
     */
    bool unInit();

    /**
     * @brief 线程函数
     */
    void run();

    /**
     * @brief 处理推理结果
     * @param result 推理结果文本
     * @param channel 业务通道号
     * @param input 输入推理文本
     * @param image 图片数据s
     */
    void handleResult(const std::string& result, int channel, const std::string& input, const cv::Mat& image);

    /**
     * @brief 从VPSS通道获取图像
     * @param channel vpss通道号
     * @param frame 输出图像帧
     * @param src_frame 输出原始大小图像数据
     * @return 成功返回true，失败返回false
     */
    bool getImageFromVpss(int channel, cv::Mat& frame,cv::Mat& src_frame);

    /**
     * @brief 从本地加载图像
     * @param path 图像路径
     * @param frame 输出图像帧
     * @return 成功返回true，失败返回false
     */
    bool loadImageFromLocal(const std::string& path, cv::Mat& frame);

    /**
     * @brief 图像预处理和编码
     * @param frame 输入图像
     * @param img_vec 输出图像向量
     * @return 成功返回true，失败返回false
     */
    bool encodeImage(const cv::Mat& frame, float* img_vec);

    /**
     * @brief 图像预处理和编码
     * @param path 图像路径
     * @param image 图片数据
     * @return 成功返回true，失败返回false
     */
    bool loadEncodeImage(const std::string& path, cv::Mat& image);

private:
    // LLM推理数据结构
    struct LlmQueryData {
        std::string text;
        cv::Mat image;
        int channel;
        int vpss_channel;
        std::string image_path;
        bool use_image;
    };

     /* 推理历史记录管理器 */
    InferenceHistory m_inferenceHistory;

    /* LLM推理引擎 */
    Inference_NS::CLLMInferenceRK* m_pLlmEngine;
    /* RKNN视觉模型 */
    std::unique_ptr<CImageEnoderInferenceRK> m_pImageEncoder;
    /* 队列 */
    BQ_NS::CBlockingQueue<LlmQueryData> m_dataQueue;
    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread             m_thread;
    /* 大模型使能标志 */
    bool m_bEnable;
    /* 原始图像，用于保存结果 */
    cv::Mat m_Image_Mat;
    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 配置参数，模型推理画面分析 */
    Alarm::LLMImageAnalysis_S m_stImageAnalysisCfg;

public:

};