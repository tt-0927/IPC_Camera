/*
 * @FilePath     : AiPlatformBase.hpp
 * @Author       : lih lih@kfb.cn
 * @Date         : 2023-05-10 15:37:59
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-11-29 17:47:37
 * @Description  :
 */
#pragma once

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include "BlError.h"
#include "dlog.h"
#include "PlatformExtern.hpp"

namespace PlatformManage_NS
{
    /*通讯命令码*/
    typedef enum _AiPlatformCmdCode_
    {
        AI_PLATFORM_CMD_HUMAN_INFO = 0, /* 获取班级信息 */
    } AiPlatformCmdCode_E;

    class CAiPlatformBase
    {
    public:

        CAiPlatformBase(AiPlatformInParam_S stInParam)
            : m_stInParam(stInParam)
        {
        }

        virtual ~CAiPlatformBase()
        {
            if (m_bRunning.load())
            {
                m_bRunning.store(false);
                m_threadObj.join();
            }
        }

    protected:

        /* =====================纯虚函数接口====================== */

        /**
         * @description: 初始化函数
         * @return [*] BlError_E::OK 成功  其他失败
         * @others:
         */
        virtual BlError_E init() = 0;

        /**
         * @description: 获取班级信息
         * @param [*]
         * @return [*] BlError_E::OK 成功  其他失败
         * @others:
         */
        virtual BlError_E get_classInfo(DataInfo_S& stDataInfo) = 0;

        /**
         * @description: 获取教育云平台班级信息
         * @param [*]
         * @return [*] BlError_E::OK 成功  其他失败
         * @others:
         */
        virtual BlError_E get_platformClassInfo(DataInfo_S& stDataInfo) = 0;

    public:

        /**
         * @brief 插入命令
         * @param [AiPlatformCmdCode_E] enCode: 命令类型
         * @return [*] 无
         * @note
         */
        void push_cmdQueue(AiPlatformCmdCode_E enCode)
        {
            /* 自动锁定互斥锁 */
            std::lock_guard<std::mutex> lock(m_mutex);

            static bool s_bPrint = false;

            if (m_setRequest.size() < 10)
            {
                m_setRequest.insert(enCode);
                s_bPrint = true;
            }
            else
            {
                if (s_bPrint)
                {
                    dlog(LOG_ERROR, "命令队列已满 [%ld]", m_setRequest.size());
                    s_bPrint = false;
                }
            }
        }

        /**
         * @brief 获取命令
         * @param [AiPlatformCmdCode_E&] enCode: 命令类型
         * @return [*]  BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E pop_cmdQueue(AiPlatformCmdCode_E& enCode)
        {
            /* 自动锁定互斥锁 */
            std::unique_lock<std::mutex> lock(m_mutex);

            if (!m_setRequest.empty())
            {
                /* 获取第一个元素 */
                enCode = *(m_setRequest.begin());
                /* 移除第一个元素 */
                m_setRequest.erase(m_setRequest.begin());

                return OK;
            }

            return NOK;
        }

        /**
         * @brief 判断队列是否为空
         * @return [*] 是否为空
         * @note
         */
        bool isEmpty_cmdQueue()
        {
            /* 自动锁定互斥锁 */
            std::unique_lock<std::mutex> lock(m_mutex);

            return m_setRequest.empty();
        }

        /**
         * @brief ping 服务器
         * @return [*]
         * @note
         */
        bool ping()
        {
            /* 主动ping一下 API。方便解析域名 */
            /* 需要执行的命令 */
            std::string strCommand = "ping -c 2 " + std::string(m_stInParam.stNeedParam.achIpAddr);
            /* 调用system函数执行命令 */
            int         nResult    = system(strCommand.c_str());
            /* 检查命令是否成功执行 */
            if (nResult == 0)
            {
                dlog(LOG_TRACE, "ping [%s] 成功", m_stInParam.stNeedParam.achIpAddr);
                return true;
            }
            else
            {
                dlog(LOG_ERROR, "ping [%s] 失败", m_stInParam.stNeedParam.achIpAddr);
                return false;
            }
        }

        BlError_E set_platformIp(char* pchIpAddr)
        {
            if (NULL == pchIpAddr)
            {
                return ERR_IN_PARAM_NULL;
            }

            m_paramMutex.lock();
            int nCpySize = sizeof(m_stInParam.stNeedParam.achIpAddr);
            strncpy(m_stInParam.stNeedParam.achIpAddr,
                    pchIpAddr,
                    nCpySize - 1);
            /* 确保以空字符终止 */
            m_stInParam.stNeedParam.achIpAddr[nCpySize - 1] = '\0';
            m_paramMutex.unlock();

            return OK;
        }

        /**
         * @brief 开始线程
         * @return [*]  BlError_E::OK 成功  其他失败
         * @note
         */
        BlError_E start_thread()
        {
            if (!m_bRunning.load())
            {
                m_bRunning.store(true);
                m_threadObj = std::thread(&CAiPlatformBase::run, this);
            }
            return OK;
        }

        void clear_dataInfo()
        {
            m_paramMutex.lock();
            m_stDataInfo.clear();
            m_paramMutex.unlock();
        }


    protected:

        /* 参数 */
        std::mutex                    m_paramMutex;
        AiPlatformInParam_S           m_stInParam;
        /* 教室信息结构体 */
        PlatformManage_NS::DataInfo_S m_stDataInfo;

    private:

        /**
         * @brief 线程函数
         * @return [*] 无
         * @note
         */
        void run()
        {

            BlError_E enRetCode = OK;

            std::chrono::milliseconds sleepDuration(200);

            while (m_bRunning.load())
            {
                if (!isEmpty_cmdQueue())
                {
                    AiPlatformCmdCode_E enCode;
                    enRetCode = pop_cmdQueue(enCode);
                    if (enRetCode >= OK)
                    {
                        switch (enCode)
                        {
                            /* 获取班级信息 */
                            case AI_PLATFORM_CMD_HUMAN_INFO:
                            {
                                PlatformManage_NS::DataInfo_S stDataInfo;
                                stDataInfo.clear();
                                enRetCode = get_classInfo(stDataInfo);
                                if(enRetCode < OK)
                                {
                                    stDataInfo.clear();
                                    enRetCode = get_platformClassInfo(stDataInfo);
                                }

                                /* 调用回调 */
                                if (m_stInParam.stNeedParam.aiPlatformCallback)
                                {
                                    m_stInParam.stNeedParam.aiPlatformCallback(stDataInfo, enRetCode, m_stInParam.stNeedParam.pHandle);
                                }

                                break;
                            }
                            default:
                                break;
                        }
                    }
                }

                /* 等待200ms */
                std::this_thread::sleep_for(sleepDuration);
            }
        }

        /* 请求容器 */
        std::set<AiPlatformCmdCode_E> m_setRequest;

        /* 线程相关 */
        std::thread       m_threadObj;
        std::atomic<bool> m_bRunning = { false }; /* 不能改成true */
        std::mutex        m_mutex;
    };
}    // namespace PlatformManage_NS
