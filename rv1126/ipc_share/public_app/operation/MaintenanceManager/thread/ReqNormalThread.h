/*
 * @FilePath     : ReqNormalThread.h
 * @Author       : xiezhh
 * @Date         : 2024-06-04 17:23
 * @LastEditors  : xiezhh
 * @LastEditTime : 2024-06-25 15:24
 * @Description  : 用于请求登录和请求查询项目列表的线程
 */
#ifndef REQNORMALTHREAD_H
#define REQNORMALTHREAD_H

#include "MaintenanceThread.h"
#include "common/MaintenanceJsonParse.h"
#include "CurlMultipartHttpPost.h"

class CReqNormalThread : public CMaintenanceThread
{

public:
    CReqNormalThread();

    /**
     * @brief 初始化
     * @return [bool] true：初始化成功，false：初始化失败
     */
    bool init();
    /**
     * @brief 是否初始化
     * @return [bool] true：初始化成功，false：未初始化
     */
    bool isInit();
    /**
     * @brief 将登录请求加入队列
     * @param [string] 登录用户
     * @param [string] 登录密码
     * @param [bool] 是否将当前请求插入队列头
     */
    void requeryLogin(const std::string &strUser, const std::string &strPwd,
                      const bool &isPushFront = false);

protected:
    /**
     * @brief 线程运行函数
     * @note 在这个函数中进行while循环
     */
    virtual void run() override;

private:
    /**
     * @brief 插入到队列尾
     * @param [HttpRequery] 一个请求的结构体
     */
    void pushBack(MaintenanceNS::HttpRequery &stRequery);
    /**
     * @brief 插入到队列头
     * @param [HttpRequery] 一个请求的结构体
     */
    void pushFront(MaintenanceNS::HttpRequery &stRequery);
    /**
     * @brief 从队列头弹出
     */
    void popFron();
    /**
     * @brief 清空队列
     */
    void clear();

private:
    /**
     * @brief 将一个查询项目列表请求插入队列
     * @param [string] 查询参数，用于过滤
     * @param [int] 查询参数，查询第几页
     */
    void requeryProject(const std::string &strFilter, const int &nPage);

    /**
     * @brief 发送“登录”请求
     * @return [bool] true: 成功，false：失败
     */
    bool sendLoginReq();
    /**
     * @brief 发送“查询项目列表”请求
     * @return [bool] true: 成功，false：失败
     */
    bool sendProjectReq();

private:
    /**
     * @brief 处理登录结果
     * @param [out][bool] 处理结果
     */
    void checkLoginResult(bool &bRet);
    /**
     * @brief 处理查询项目列表的结果
     * @param [out][bool] 处理结果
     */
    void checkProjectResult(bool &bRet);

private:
    /* 是否初始化的标志，true：已初始化 */
    bool m_isInit = false;
    /* Post请求的指针 */
    CurlHttp::CCurlMultipartHttpPost *m_pNormalPost = nullptr;
    /* Get请求的指针 */
    CurlHttp::Get *m_pNormalGet = nullptr;

    /* 双向队列 */
    std::deque<MaintenanceNS::HttpRequery> m_queue;
    /* 双向队列会使用到的读写锁 */
    std::shared_mutex m_mutex;

    /* 请求路径 */
    std::string m_strUrl;
    /* 当前项目唯一代码 */
    std::string m_strCode;
    /* 登录用户名 */
    std::string m_strUserName;
    /* 登录密码 */
    std::string m_strPwd;

    /* 解析Json的类 */
    CMaintenanceJsonParse m_cJson;
    /* 查询项目列表的过滤参数 */
    std::string m_strProjectFilter;
    /* 查询项目列表的当前页 */
    int m_nProjectCurPage = 1;
    /* 当前项目的唯一项目ID */
    int m_nCurProjectID = -1;

    /* 登录请求结果 */
    MaintenanceNS::LoginResult m_stLoginResult;
    /* 查询项目列表结果 */
    MaintenanceNS::ReqProjectResult m_stReqProjectResult;
};

#endif // REQNORMALTHREAD_H
