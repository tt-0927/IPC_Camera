/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/fire_person_algo_detect/fire_person_algo_detect.hpp
 * @Date         : 2026-10-14
 * @Description  : 烟火识别、明火检测业务（专用 FirePersonDetect 模型：烟雾/火焰/行人）
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
#include "fire_person_detect.hpp" /* CFirePersonDetector 纯检测组件 */

class CFirePersonAlgoDetect : public CAlgorithm {
  public:
    CFirePersonAlgoDetect();
    ~CFirePersonAlgoDetect();

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
    void setAlgoParamCfg(const Alarm::SmokeFireDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::SMOKE_FIRE);
    void setAlgoParamCfg(const Alarm::OpenFlameDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::OPEN_FLAME);
    void setAlgoParamCfg(const Alarm::OpenFlameLeaveDetection_S &stAlgoCfg, Event::Type_E enType = Event::Type_E::OPEN_FLAME_LEAVE);

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
     * @brief 将 1-100 的灵敏度转换为触发报警所需的连续帧数
     * @param sensitivity 灵敏度 (1-100)
     * @param minFrames 最高灵敏度 (100) 对应的帧数
     * @param maxFrames 最低灵敏度 (1) 对应的帧数
     * @return int 触发报警所需的连续帧数
     */
    int sensitivityToFrames(int sensitivity, int minFrames = 1, int maxFrames = 10);

    /**
     * @brief 线程函数
     * @return [*]
     * @note
     */
    void run();

    /**
     * @brief 结果分发：依次送烟火/明火/明火离人判定，命中框统一送 OSD
     * @param vecResult 检测组件输出的原始目标框（原图坐标）
     * @return [*]
     * @note 行人类(PERSON)暂不接业务，仅过滤使用
     */
    void processDetectResult(const std::vector<DetectedTarget_S> &vecResult);

    /**
     * @brief 烟火识别判定：烟雾或火焰任一检出，连续帧判定后喂状态机
     * @param vecResult 检测组件输出的原始目标框（原图坐标）
     * @param vstRectInfo 输出：命中的烟雾/火焰框（原图坐标，供 OSD）
     * @return [*]
     */
    void processSmokeFire(const std::vector<DetectedTarget_S> &vecResult, std::vector<Common::RectInfo_S> &vstRectInfo);

    /**
     * @brief 明火识别判定：仅火焰，连续帧判定后喂状态机
     * @param vecResult 检测组件输出的原始目标框（原图坐标）
     * @param vstRectInfo 输出：命中的火焰框（原图坐标，供 OSD）
     * @return [*]
     */
    void processOpenFlame(const std::vector<DetectedTarget_S> &vecResult, std::vector<Common::RectInfo_S> &vstRectInfo);

    /**
     * @brief 明火离人判定：区域内有明火且无人持续达时间阈值触发，明火消失或有人出现即清零
     * @param vecResult 检测组件输出的原始目标框（原图坐标）
     * @param vstRectInfo 输出：区域内命中的火焰框/人框（原图坐标，供 OSD）
     * @return [*]
     * @note 灵敏度映射火焰置信度，人员框直接使用检出结果
     */
    void processOpenFlameLeave(const std::vector<DetectedTarget_S> &vecResult,
                               std::vector<Common::RectInfo_S> &vstRectInfo);

  private:
    /* 检测组件句柄（纯检测，无业务） */
    CFirePersonDetector *m_pDetector = nullptr;

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

    /* 烟火 识别相关配置 */
    Alarm::SmokeFireDetection_S m_stAlgoSmokeFireCfg;
    /* 明火 识别相关配置 */
    Alarm::OpenFlameDetection_S m_stAlgoOpenFlameCfg;
    /* 明火离人 识别相关配置 */
    Alarm::OpenFlameLeaveDetection_S m_stAlgoOpenFlameLeaveCfg;

    /* 烟火(烟雾或火焰) 连续帧计数（漏检清零） */
    int m_nSmokeFireFrameCount = 0;
    /* 明火 连续帧计数（漏检清零） */
    int m_nFireFrameCount = 0;

    /* 明火离人 条件持续计时起点（毫秒，0=未计时） */
    int64_t m_llOpenFlameLeaveStartMs = 0;

    /* 检测频率控制 */
    EventManager m_RecvManager{500};

    /* 报警状态管理 */
    CAlarmStateMachine m_SmokeFireStateMachine; /* 烟火 */
    CAlarmStateMachine m_OpenFlameStateMachine; /* 明火 */
    CAlarmStateMachine m_OpenFlameLeaveStateMachine; /* 明火离人 */

    /* 通道号 */
    int m_nChannelId = -1;

    /* 缓存RGB原图（TVSDK全景图用） */
    cv::Mat m_fullRgbMat;
};
