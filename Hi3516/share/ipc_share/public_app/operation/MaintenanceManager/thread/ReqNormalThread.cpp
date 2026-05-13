#include "ReqNormalThread.h"
#include "MaintenanceData.h"
#include <iostream>
#include <fstream>

#include "dlog.h"

extern "C"
{
#include "http_communicate.h"
}

using namespace MaintenanceNS;

CReqNormalThread::CReqNormalThread()
    : CMaintenanceThread()
{
}

bool CReqNormalThread::init()
{
    std::string strUrl = CMaintenanceData::getInstance()->getRequeryUrl();
    std::string strCode = CMaintenanceData::getInstance()->getProjectCode();
    if (strUrl.empty() || strCode.empty())
    {
        dlog_error("CReqNormalThread::init, url or project code is empty! Please initialize the configuration!");
        return false;
    }

    m_strUrl = strUrl;
    m_strCode = strCode;

    dlog_info("当前code:%s", m_strCode.c_str());

    if (m_pNormalPost == nullptr && m_pNormalGet == nullptr)
    {
        m_pNormalPost = new CurlHttp::CCurlMultipartHttpPost(m_strUrl);
        m_pNormalGet = new CurlHttp::Get(m_strUrl);
    }
    else
    {
        m_pNormalPost->set_path(m_strUrl);
        m_pNormalGet->set_path(m_strUrl);
    }
    return true;
}

bool CReqNormalThread::isInit()
{
    if (!m_strUrl.empty() && !m_strCode.empty() &&
        m_pNormalPost != nullptr && m_pNormalGet != nullptr)
    {
        return true;
    }
    return false;
}

void CReqNormalThread::requeryLogin(const std::string &strUser, const std::string &strPwd, const bool &isPushFront)
{
    if (isInit())
    {
        if (strUser.empty() || strPwd.empty())
        {
            dlog_error("CReqNormalThread::requeryLogin, user or pwd is empty!");
            return;
        }

        m_strUserName = strUser;
        m_strPwd = strPwd;

        HttpRequery stRequery;
        stRequery.enInterface = REQ_LOGIN;
        if (isPushFront)
        {
            pushFront(stRequery);
        }
        else
        {
            pushBack(stRequery);
        }
    }
}

void CReqNormalThread::requeryProject(const std::string &strFilter, const int &nPage)
{
    if (isInit())
    {
        m_strProjectFilter = strFilter;
        m_nProjectCurPage = nPage;

        HttpRequery stRequery;
        stRequery.enInterface = REQ_PROJECT;
        stRequery.enRequeryFun = REQ_FUN_GET;
        dlog_debug("push项目列表请求");
        pushBack(stRequery);
    }
}

void CReqNormalThread::run()
{
    /* 请求结果 */
    bool bRet = false;

    while (m_bIsRunFlag.load())
    {
        /* 判断是否需要重新登录，检查登录状态和Token */
        if (!CMaintenanceData::getInstance()->getLoginStatus())
        {
            requeryLogin(m_strUserName, m_strPwd, true);
        }

        /* 缩小锁的颗粒度，防止线程休眠时一直锁着 */
        {
            std::shared_lock<std::shared_mutex> locker(m_mutex);
            if (m_queue.size() > 0)
            {
                HttpRequery stRequery = m_queue.front();
                // m_queue.pop_front();
                locker.unlock();

                switch (stRequery.enInterface)
                {
                case REQ_LOGIN:
                    bRet = sendLoginReq();
                    checkLoginResult(bRet);
                    break;
                case REQ_PROJECT:
                    bRet = sendProjectReq();
                    checkProjectResult(bRet);
                    break;
                default:
                    break;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(REQNORMAL_MSLEEP));
    }
}

void CReqNormalThread::pushBack(MaintenanceNS::HttpRequery &stRequery)
{
    std::unique_lock<std::shared_mutex> locker(m_mutex);
    m_queue.push_back(stRequery);
}

void CReqNormalThread::pushFront(HttpRequery &stRequery)
{
    std::unique_lock<std::shared_mutex> locker(m_mutex);
    /* 如果当前队列头中不是登录请求则添加登录请求进入队列 */
    if (m_queue.size() <= 0 || m_queue.front().enInterface != REQ_LOGIN)
    {
        m_queue.push_front(stRequery);
    }
    /* TODO：这里需不需要修改登录请求的账号和密码呢？ */
}

void CReqNormalThread::popFron()
{
    std::unique_lock<std::shared_mutex> locker(m_mutex);
    if (m_queue.size() > 0)
    {
        m_queue.pop_front();
    }
}

void CReqNormalThread::clear()
{
    std::unique_lock<std::shared_mutex> locker(m_mutex);
    m_queue.clear();
}

bool CReqNormalThread::sendLoginReq()
{
    dlog_info("添加一个登录请求 用户名:%s 密码:%s", m_strUserName.c_str(), m_strPwd.c_str());
    bool bRet = false;
    m_pNormalPost->clear_form();
    m_pNormalPost->set_path(REQ_LOGIN_PATH);
    m_pNormalPost->add_formData("username", m_strUserName);
    m_pNormalPost->add_formData("pwd", m_strPwd);

    std::string strResult;
    int nReqRet = m_pNormalPost->send_request();
    if (nReqRet == CURLE_OK)
    {
        bRet = true;
        m_pNormalPost->get_recvData(strResult);
        m_stLoginResult = m_cJson.parseLoginResult(strResult, bRet);

        /* 打印结果 */
        dlog_debug("req login recv:");
        dlog_debug("%s", strResult.c_str());

        if (!bRet)
        {
            dlog_error("CReqNormalThread::sendLoginReq error: parse json faile!");
        }
    }
    else
    {
        bRet = false;
        strResult = m_pNormalPost->get_error(nReqRet);
        dlog_error("CReqNormalThread::sendLoginReq error: %s", strResult.c_str());
    }

    return bRet;
}

bool CReqNormalThread::sendProjectReq()
{
    bool bRet = false;
    std::string strToken = m_stLoginResult.strToken;
    if (!strToken.empty())
    {
        dlog_info("添加一个请求项目列表请求");
        /* 添加token */
        {
            std::string strTmp;
            std::list<std::string> arrHeader;
            strTmp.assign("ApiToken:").append(strToken);
            arrHeader.push_back(strTmp);
            m_pNormalGet->set_header(arrHeader);
        }

        /* 添加参数 */
        {
            std::string strTmp;
            std::list<std::string> arrPar;
            strTmp.assign("keyword=").append(m_strProjectFilter);
            arrPar.push_back(strTmp);
            strTmp.assign("pageid=").append(std::to_string(m_nProjectCurPage));
            arrPar.push_back(strTmp);

            m_pNormalGet->set_params(arrPar);
        }

        /* 设置请求路径 */
        m_pNormalGet->set_path(REQ_PROJECT_PATH);

        std::string strResult;
        int nReqRet = m_pNormalGet->send_request();
        if (nReqRet == CURLE_OK)
        {
            bRet = true;
            m_pNormalGet->get_recvData(strResult);
            m_stReqProjectResult = m_cJson.parseProjectResult(strResult, bRet);

            /* 打印结果 */
            dlog_debug("req project recv:");
            dlog_debug("%s", strResult.c_str());

            if (!bRet)
            {
                dlog_error("CReqNormalThread::sendProjectReq error: parse json faile!");
            }
        }
        else
        {
            bRet = false;
            strResult = m_pNormalGet->get_error(nReqRet);
            dlog_error("CReqNormalThread::sendProjectReq error: %s", strResult.c_str());
        }
    }
    return bRet;
}

void CReqNormalThread::checkLoginResult(bool &bRet)
{
    if (bRet && m_stLoginResult.stResult.nResult == SUCCESS)
    {
        popFron();

        /* 设置登录状态和Token */
        CMaintenanceData::getInstance()->setToken(m_stLoginResult.strToken);
        // Token传入到接口
        dlog_info("======================当前Tocken为：%s", m_stLoginResult.strToken.c_str());
        set_LoginTocken(m_stLoginResult.strToken.c_str());
        CMaintenanceData::getInstance()->setLoginStatus(true);

        /* 直接请求项目信息，无过滤字符，请求第一页 */
        // requeryProject("", 1);
    }
    else
    {
        dlog_error("login fail! msg: %s", m_stLoginResult.stResult.strMsg.c_str());
    }
}

void CReqNormalThread::checkProjectResult(bool &bRet)
{
    if (bRet && m_stReqProjectResult.stResult.nResult == SUCCESS)
    {
        /* 如果当前查询的结果，页数为0或者数据条数为0条 */
        if (m_stReqProjectResult.nTotalPage <= 0 || m_stReqProjectResult.vecProject.size() <= 0)
        {
            /* 将查询的页数置为1，之后不做任何操作 */
            m_nProjectCurPage = 1;
            dlog_info("req project result: data is empty!");
            return;
        }
        /* 找到自己对应的项目 */
        for (std::size_t i = 0; i < m_stReqProjectResult.vecProject.size(); i++)
        {
            /* 如果项目列表结果中，与当前项目的唯一code相同 */
            if (m_stReqProjectResult.vecProject.at(i).strCode.compare(m_strCode) == 0)
            {
                /* 找到该项目获取项目ID */
                m_nCurProjectID = m_stReqProjectResult.vecProject.at(i).nID;

                /* 设置项目ID */
                CMaintenanceData::getInstance()->setProjectID(m_nCurProjectID);

                /* 移除当前请求 */
                popFron();

                dlog_info("找到对应的项目，项目ID：%d", m_nCurProjectID);

                return;
            }
        }

        /* 如果当前列表中找不到当前项目唯一code，并且当前请求的页数小于总页数，当前页数++ */
        if (m_nProjectCurPage < m_stReqProjectResult.nTotalPage)
        {
            m_nProjectCurPage++;
        }
        else
        {
            /* 页数超了，回到第一页 */
            m_nProjectCurPage = 1;
        }
    }
    else if ((bRet && m_stReqProjectResult.stResult.nResult == UNAUTHENTICATION) ||
             (bRet && m_stReqProjectResult.stResult.nResult == UNAUTHENTICATION_V2))
    {
        /* 移除当前请求 */
        popFron();

        /* 设置登录状态 */
        CMaintenanceData::getInstance()->setLoginStatus(false);

        /* 从新请求登录 */
        requeryLogin(m_strUserName, m_strPwd, true);
    }
    else
    {
        dlog_error("req project fail! msg: %s", m_stReqProjectResult.stResult.strMsg.c_str());
    }
}
