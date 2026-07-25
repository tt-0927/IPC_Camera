/*** 
 * @FilePath     : visionText_manage.hpp
 * @Author       : cyc
 * @Date         : 2025-09-25 19:17:29
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-26 17:33:11
 * @Description  : 视觉文本分析管理类 - 管理多个文字预设任务
 */

 #pragma once
 #include <atomic>
 #include <mutex>
 #include <vector>
 #include <string>
 #include "visionText_analysis.hpp"
 #include "algorithm.hpp"
 #include "alarm_define.h"

 // CVisionTextManager 作为管理层，继承 CAlgorithm
 class CVisionTextManager : public CAlgorithm, CSingleton<CVisionTextManager>
 {
 public:
    CVisionTextManager();
    ~CVisionTextManager();

    friend class CSingleton<CVisionTextManager>;

    /**
    * @brief 获取当前激活的任务ID
    * @return 当前激活的任务ID，如果没有则返回空字符串
    */
    std::string getCurrentActiveTaskId();

    /**
    * @brief 设置画面分析参数
    * @param stAlgoCfg 画面分析配置
    */
    void setAlgoParamCfg(const Alarm::LLMImageAnalysis_S &stAlgoCfg);

    /**
    * @brief 更新算法配置参数（重写CAlgorithm的虚函数）
    * @param stAlgoConfig 算法配置
    */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

private:

    /**
    * @brief 接受媒体数据（重写CAlgorithm的虚函数）
    * @param stMediaData 媒体数据
    */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief 从文件加载任务配置
     * @return 成功返回true，失败返回false
     */
    bool loadTasksFromConfig();

    /**
     * @brief 根据任务ID查找任务在数组中的索引
     * @param strTaskId 任务ID
     * @return 找到返回索引，未找到返回-1
     */
    int findTaskIndex(const std::string& strTaskId);
     
private:
    /* 底层分析引擎 - 使用普通指针 */
    CVisionText* m_pVisionTextEngine;
    
    /* 任务管理（从配置文件读取） */
    Alarm::TextPresetTaskManager_S m_stTaskConfig;
    
    /* 画面分析配置 */
    Alarm::LLMImageAnalysis_S m_stImageAnalysisConfig;
    bool m_bImageAnalysisEnabled;
    
    /* 线程安全 */
    std::mutex m_mutex;
    
    /* 运行状态 */
    std::atomic<bool> m_bRunning;
};
