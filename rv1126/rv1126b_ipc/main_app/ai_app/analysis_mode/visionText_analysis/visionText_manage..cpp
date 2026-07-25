/*** 
 * @FilePath     : visionText_manage..cpp
* @Author       : cyc
* @Date         : 2025-09-25 19:17:21
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-28 17:06:33
* @Description  : 视觉文本分析管理类 - 管理多个文字预设任务
*/

#include "visionText_manage.hpp"
#include "event_configure.h"
#include <algorithm>

CVisionTextManager::CVisionTextManager()
: m_pVisionTextEngine(nullptr),
m_bImageAnalysisEnabled(false),
m_bRunning(true)
{
    /* 创建底层分析引擎 */
    m_pVisionTextEngine = new CVisionText();
    if (!m_pVisionTextEngine)
    {
        dlog_error("创建视觉文本分析引擎失败");
    }
    else
    {
        dlog_info("视觉文本分析管理器初始化成功");
    }

    /* 从配置文件加载任务 */
    loadTasksFromConfig();
}

CVisionTextManager::~CVisionTextManager()
{
    m_bRunning.store(false);
    
    /* 清理底层引擎 */
    if (m_pVisionTextEngine)
    {
        delete m_pVisionTextEngine;
        m_pVisionTextEngine = nullptr;
    }
    
    /* 清理任务列表 */
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stTaskConfig.aTaskConfig.clear();
    }
    
    dlog_info("视觉文本分析管理器已销毁");
}

int CVisionTextManager::findTaskIndex(const std::string& strTaskId)
{
    auto it = std::find_if(m_stTaskConfig.aTaskConfig.begin(), m_stTaskConfig.aTaskConfig.end(),
        [&strTaskId](const Alarm::TextPreset_S& task) {
            return task.strTaskId == strTaskId;
        });
    
    if (it != m_stTaskConfig.aTaskConfig.end())
    {
        return std::distance(m_stTaskConfig.aTaskConfig.begin(), it);
    }
    
    return -1;
}

bool CVisionTextManager::loadTasksFromConfig()
{
    Alarm::TextPresetTaskManager_S stLoadedTasks;
    int nRet = CEventConfigure::instance()->get_configure(stLoadedTasks);
    
    if (nRet == 0)
    {
        // 不需要加锁，因为调用者已经加锁了
        m_stTaskConfig = stLoadedTasks;
        
        dlog_debug("从配置文件加载任务成功，共 %zu 个任务，当前激活: %s", 
                  m_stTaskConfig.aTaskConfig.size(),
                  m_stTaskConfig.strCurrentActiveTaskId.empty() ? "无" : m_stTaskConfig.strCurrentActiveTaskId.c_str());
        return true;
    }
    else
    {
        dlog_warn("从配置文件加载任务失败，错误码: %d，使用默认配置", nRet);
        /* 使用默认空配置 */
        m_stTaskConfig.aTaskConfig.clear();
        m_stTaskConfig.strCurrentActiveTaskId.clear();
        return false;
    }
    
}

std::string CVisionTextManager::getCurrentActiveTaskId()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stTaskConfig.strCurrentActiveTaskId;
}

void CVisionTextManager::setAlgoParamCfg(const Alarm::LLMImageAnalysis_S &stAlgoCfg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stImageAnalysisConfig = stAlgoCfg;
    
    /* 传递给底层引擎 */
    if (m_pVisionTextEngine)
    {
        m_pVisionTextEngine->setAlgoParamCfg(stAlgoCfg);
    }
    
    dlog_debug("画面分析参数已更新");
}

void CVisionTextManager::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    /* 场景智能分析总开关 */
    bool bAISceneAnalysis = stAlgoConfig.nEnAISceneAnalysis; 

    bool bImageAnalysisEnable = stAlgoConfig.nEnLLmInference;
    bool bTextPresetEnable = stAlgoConfig.nEnTextPreset;
    
    dlog_info("接收到算法配置更新 - 场景智能分析总开关: %s, 画面分析: %s, 文字预设: %s", 
            bAISceneAnalysis ? "启用" : "禁用",
            bImageAnalysisEnable ? "启用" : "禁用",
            bTextPresetEnable ? "启用" : "禁用");
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bImageAnalysisEnabled = bImageAnalysisEnable;

    /* 构造底层引擎需要的配置 */
    Event::AlgorithmConfig engineConfig = stAlgoConfig;

    /*场景智能分析总开关关闭时*/
    if(bAISceneAnalysis == false)
    {
        if (m_pVisionTextEngine)
        {
            /* 先调用 setAlgoEnCfg 设置算法启用状态 */ 
            m_pVisionTextEngine->setAlgoEnCfg(engineConfig);
        }
        return;    
    }
    
    // 重新从配置文件加载最新的任务配置
    loadTasksFromConfig();
    
    /* 如果文字预设启用且有激活任务，则启用文字预设 */
    bool bHasActiveTask = !m_stTaskConfig.strCurrentActiveTaskId.empty();
    bool bHasEnabledTask = std::any_of(m_stTaskConfig.aTaskConfig.begin(), m_stTaskConfig.aTaskConfig.end(),
        [](const Alarm::TextPreset_S& task) { return task.bEnable; });
    
    engineConfig.nEnTextPreset = (bTextPresetEnable && bHasActiveTask && bHasEnabledTask) ? 1 : 0;
    
    /* 传递给底层引擎 */
    if (m_pVisionTextEngine)
    {
       /* 先调用 setAlgoEnCfg 设置算法启用状态 */ 
       m_pVisionTextEngine->setAlgoEnCfg(engineConfig);
        
       /* 如果有激活任务，再设置任务参数 */ 
       if (bHasActiveTask)
       {
           int activeIndex = findTaskIndex(m_stTaskConfig.strCurrentActiveTaskId);
           if (activeIndex >= 0 && m_stTaskConfig.aTaskConfig[activeIndex].bEnable)
           {
               dlog_info("设置激活任务参数: %s (%s)", 
                        m_stTaskConfig.strCurrentActiveTaskId.c_str(),
                        m_stTaskConfig.aTaskConfig[activeIndex].strTaskName.c_str());
               
               m_pVisionTextEngine->setAlgoParamCfg(m_stTaskConfig.aTaskConfig[activeIndex]);
           }
       }
    }
    
    if (engineConfig.nEnTextPreset)
    {
        dlog_info("文字预设分析已启用，当前激活任务: %s", 
                 bHasActiveTask ? m_stTaskConfig.strCurrentActiveTaskId.c_str() : "无");
    }
    else
    {
        dlog_info("文字预设分析已禁用");
    }
}

void CVisionTextManager::recvMediaData(MediaData_S stMediaData)
{
    /* 直接传递给底层引擎 */
    if (m_pVisionTextEngine)
    {
        m_pVisionTextEngine->recvMediaData(stMediaData);
    }
}
