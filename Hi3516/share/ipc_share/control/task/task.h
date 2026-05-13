/*
 *  File Name: task.h
 *  Created on: 2022年11月01日
 *  Author: zjc
 *  description: 命令基类
 */
#pragma once
#include <functional>
#include <string>
#include <set>
#include <mutex>
#include <queue>

#include "Json.h"
#include "common_define.h"
/* 命令相关 */
namespace Task
{

    /**
     * @brief 任务结果回调指针
     */
    using ResultCallback = std::function<int(const void *, int, int, void *)>;

    // 自定义比较器
    struct ResultCallbackCompare
    {
        bool operator()(const ResultCallback &lhs, const ResultCallback &rhs) const
        {
            return lhs.target_type().before(rhs.target_type());
        }
    };
    /**
     * @brief 任务信息结构体
     */
    typedef struct Info_
    {
        void *pHandler = nullptr;
        std::string data;
        /**
         * @brief 任务请求者
         */
        Common::Requester_E enRequester = Common::REQUESTER_NONE;
        /**
         * @brief 任务结果回调
         */
        ResultCallback fnResultCallbacks;
        /**
         * @brief 任务唯一ID
         */
        int nUniqueId = 0;
        std::string user;
        /* 任务操作者Ip */
        std::string strIp;
        bool operator<(const Info_ &stInfo) const
        {
            return this->enRequester < stInfo.enRequester;
        }
    } Info_S;

    /* 命令基类 */
    class CTask
    {
    public:
        CTask(std::string taskData = std::string()) : m_taskData(taskData) {}
        virtual ~CTask() {}
        virtual void handle() = 0;

        /**
         * @brief 异步结果处理
         * @param fnDealFunc 结果处理函数
         */
        void deal_result(std::function<void(std::string)> fnDealFunc);
        /**
         * @brief 异步结果处理
         * @param data 异步返回的数据
         */
        void deal_result(std::string data);
        /**
         * @brief 获取data对象
         * @param jsonData
         * @return std::string
         */
        std::string get_data(std::string jsonData);
        /**
         * @brief 结果返回
         * @param res
         * @param nRet
         * @return int
         */
        int result(std::string res, int nRet = 0);
        /**
         * @brief 结果返回
         * @param nRet
         */
        void result(int nRet);

        /**
         * @brief 发布信息
         * @param res
         */
        void publish(std::string message);

        /**
         * @brief 发布信息
         * @param res
         */
        void publish(const void *pData, int nLen);

        /**
         * @brief 判断是否是返回消息
         * @param jsonData
         * @return true
         * @return false
         */
        bool is_result(std::string jsonData);
        /**
         * @brief 设置nActionCode stInfo对象
         * @param nActionCode 任务码
         * @param stInfo 任务信息
         */
        void set_info(int nActionCode, Info_S stInfo);

        /**
         * @brief 设置nActionCode
         * @param nActionCode 任务码
         */
        void set_actionCode(int nActionCode);

        /**
         * @brief 获取actionCode对象
         * @return int 任务码
         */
        int get_actionCode();
        /**
         * @brief 设置user对象
         * @param user
         */
        void set_user(std::string user);

        /**
         * @brief   : 设置任务操作者Ip
         * @param    {string} strUserIp
         */
        void set_userIp(std::string strUserIp);

        /**
         * @brief 填充json头数据
         * @param data
         * @param nRet
         */
        void fill_head(std::string &data, int nActionCode);
        void fill_returnHead(std::string &data, int nActionCode);
        bool verify_requester(Common::Requester_E enRequester);

        /**
         * @brief 注册订阅
         * @param fnResultCallback
         * @return * void
         */
        void register_subscribe(ResultCallback fnResultCallback);

    private:
        /**
         * @brief 填充json头数据
         * @param data
         * @param nRet
         */
        std::string fill_common(std::string data, int nRet, const Info_S &stInfo);

    protected:
        /**
         * @brief 任务数据
         */
        std::string m_data;
        std::string m_taskData;
        /**
         * @brief 任务码
         */
        int m_nActionCode = -1;
        /**
         * @brief 任务信息
         */
        std::set<Info_S> m_infos;
        /**
         * @brief 操作者
         */
        std::string m_user = std::string();
        /* 任务操作者Ip */
        std::string m_strUserIp = std::string();
        /**
         * @brief 订阅者
         */
        std::set<ResultCallback, Task::ResultCallbackCompare> m_subscribe;
        /**
         * @brief 任务结果
         */
        bool m_bResult = false;
        std::queue<std::function<void(std::string)>> m_fnDealResults;
        std::mutex m_mtx;
    };

}