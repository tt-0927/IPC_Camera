/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/fight_algo_detect/fight_algo_detect.hpp
 * @Date         : 2026-10-21
 * @Description  : 打架斗殴检测业务实现
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <sys/time.h>
#include "common_process.h"
#include "blocking_queue.hpp"
#include "stream_video.h"
#include "stream_vpss.h"
#include <opencv2/opencv.hpp>
#include "event_manager.hpp"
#include "stream_process_ext.hpp"
#include "algorithm.hpp"
#include "fight_detect.hpp" /* CFightDetector 纯检测组件 */

class CFightAlgoDetect : public CAlgorithm {
public:
    CFightAlgoDetect();
    ~CFightAlgoDetect();

    /**
     * @brief 接受媒体数据
     * @param [MediaData_S] stMediaData:
     * @return [*]
     * @note
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief 更新算法配置参数
     * @param stAlgoConfig
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;
    void setAlgoParamCfg(const Alarm::FightDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::FIGHT_DETECTION);

  private:
    /**
     * @brief 初始化
     * @return [*]
     * @note
     */
    bool init();

    /**
     * @brief 反初始化
     * @return [*]
     * @note
     */
    bool unInit();

    /**
     * @brief 灵敏度转置信度（反向映射：灵敏度越高，置信度越低）
     * @param sensitivity 输入灵敏度（范围：1~100，超出会被 clamp 到该范围）
     * @param minConfidence 最低置信度
     * @param maxConfidence 最高置信度
     * @return float 置信度（范围：minConfidence ~ maxConfidence）
     */
    float sensitivityToConfidence(int sensitivity, float minConfidence = 0.2f, float maxConfidence = 0.9f);

    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

    /**
     * @brief 结果分发：依次送打架斗殴判定，命中框统一送 OSD
     * @param vecResult 检测组件输出的原始目标框（原图坐标）
     * @return [*]
     * @note 
     */
    void processDetectResult(const std::vector<DetectedTarget_S> &vecResult);

    /**
     * @brief 打架斗殴判定：打架斗殴检出，连续帧判定后喂状态机
     * @param vecResult 检测组件输出的原始目标框（原图坐标）
     * @param vstRectInfo 输出：命中的打架斗殴框（原图坐标，供 OSD）
     * @return [*]
     */
    void processFight(const std::vector<DetectedTarget_S> &vecResult, std::vector<Common::RectInfo_S> &vstRectInfo);

  private:
    /* 检测组件句柄（纯检测，无业务） */
    FightDetect *m_pDetector = nullptr;

    /* 队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;

    /* 用于控制线程的运行 */
    std::atomic<bool> m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据获取线程 */
    std::thread m_thread;

    /* 打架斗殴相关配置 */
    Alarm::FightDetection_S m_stAlgoFightCfg;

    /* 打架斗殴条件持续计时起点（毫秒，0=未计时） */
    int64_t m_llFightStartMs = 0;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 报警状态管理 */
    CAlarmStateMachine m_FightMachine; /* 打架斗殴 */

    /* 通道号 */
    int m_nChannelId = -1;

    /* 缓存RGB原图（TVSDK全景图用） */
    cv::Mat m_fullRgbMat;
};


