/**
 * @FilePath     : email_manage.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-10-14 17:37:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-23 15:34:54
 * @Description  : 邮件类实现
 */

#include "email_manage.h"
#include "dlog.h"
#include "network_convert.h"
#include "convert_interface.h"
#include "path_define.h"
#include "time_utils.h"

#include <functional>
#include <vector>

namespace
{
    /* 同批事件邮件聚合窗口（秒），窗口内到达的邮件合并为一封发送 */
    const int MERGE_WINDOW_SEC = 3;
    /* 合并邮件附件数量上限，超出部分丢弃 */
    const size_t MAX_MERGE_ATTACHMENTS = 8;
    /* 事件邮件队列上限，超出时丢弃最旧邮件防止积压 */
    const size_t MAX_EVENT_QUEUE_SIZE = 50;
    /* 发送间隔节流的等待轮询粒度（毫秒），保证关停时可及时退出 */
    const int THROTTLE_POLL_MS = 500;
}

CEmailManage::CEmailManage() : m_configFile(EMAIL_CONFIG_FILE), m_bTesting(false)
{
    m_running = true;
    m_emailEventThread = std::thread(&CEmailManage::process_emailQueue, this);
}

IpcRet_E CEmailManage::init()
{
    Network::EmailInfo_S stNewEmInfo;
    Convert::read_file(m_configFile, stNewEmInfo);

    SetEmailInfo(stNewEmInfo);
    // StartSending(stNewEmInfo.nCaptureTimeInterval);
    return OK;
}

IpcRet_E CEmailManage::deinit()
{
    // StopSending();
    m_running = false;

    m_emailCond.notify_all();

    if (m_emailEventThread.joinable())
    {
        m_emailEventThread.join();
    }
    return OK;
}

void CEmailManage::process_emailQueue()
{
    while (m_running)
    {
        Network::EmailEventInfo_S stEmail;
        {
            std::unique_lock<std::mutex> lock(m_emailQueueMutex);
            m_emailCond.wait(lock,
                             [this]()
                             {
                                 return !m_emailEventQueue.empty() || !m_running;
                             });

            if (!m_running)
            {
                break;
            }
            stEmail = std::move(m_emailEventQueue.front());
            m_emailEventQueue.pop();

            /* 聚合窗口内等待同批事件邮件陆续入队，窗口结束后统一合并 */
            m_emailCond.wait_for(lock, std::chrono::seconds(MERGE_WINDOW_SEC),
                                 [this]()
                                 {
                                     return !m_running;
                                 });

            if (!m_running)
            {
                break;
            }

            /* 同类型事件只保留首条，其余合并主题/正文/附件 */
            std::vector<std::string> vecMergedEvents;
            vecMergedEvents.push_back(stEmail.strSubject);
            while (!m_emailEventQueue.empty())
            {
                Network::EmailEventInfo_S stNext = std::move(m_emailEventQueue.front());
                m_emailEventQueue.pop();

                bool bDuplicated = false;
                for (const auto &strEvent : vecMergedEvents)
                {
                    if (strEvent == stNext.strSubject)
                    {
                        bDuplicated = true;
                        break;
                    }
                }
                if (bDuplicated)
                {
                    continue;
                }
                vecMergedEvents.push_back(stNext.strSubject);

                stEmail.strSubject += "、";
                stEmail.strSubject += stNext.strSubject;
                stEmail.strMessage += "\n";
                stEmail.strMessage += stNext.strMessage;
                stEmail.vecImageFile.insert(stEmail.vecImageFile.end(),
                                            stNext.vecImageFile.begin(),
                                            stNext.vecImageFile.end());
            }

            if (stEmail.vecImageFile.size() > MAX_MERGE_ATTACHMENTS)
            {
                dlog_warn("合并邮件附件数%zu超上限，仅保留前%zu张",
                          stEmail.vecImageFile.size(), MAX_MERGE_ATTACHMENTS);
                stEmail.vecImageFile.resize(MAX_MERGE_ATTACHMENTS);
            }
        }

        /* 发送间隔节流：距上次发送不足配置间隔时顺延，等待中新到的邮件由下轮聚合窗口合并 */
        if (m_llLastSendTimeMs > 0 && m_EmInfo.nCaptureTimeInterval > 0)
        {
            long long llRemainMs = m_llLastSendTimeMs + m_EmInfo.nCaptureTimeInterval * 1000LL - TimeUtils_NS::get_monotonicTimestampMs();
            while (llRemainMs > 0 && m_running)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(THROTTLE_POLL_MS));
                llRemainMs -= THROTTLE_POLL_MS;
            }
        }

        SendEventEmail(stEmail);
        /* 无论成败都记录发送时间，发送失败(如535限流)同样需要降频 */
        m_llLastSendTimeMs = TimeUtils_NS::get_monotonicTimestampMs();
    }
}

/* 根据传入的参数处理邮件 */
int CEmailManage::HandleEmail(const EmailVariant &emailRequest)
{
    int nRet;
    /* 设置邮件相关信息 */
    if (mpark::holds_alternative<Network::EmailInfo_S>(emailRequest))
    {
        dlog_info("设置邮件信息");
        nRet = SetEmailInfo(mpark::get<Network::EmailInfo_S>(emailRequest));
    }
    /* 事件触发邮件发送 */
    else if (mpark::holds_alternative<Network::EmailEventInfo_S>(emailRequest))
    {
        dlog_info("处理事件触发邮件");
        {
            std::lock_guard<std::mutex> lock(m_emailQueueMutex);
            /* 队列满时丢弃最旧邮件，防止发送阻塞期间无限积压 */
            if (m_emailEventQueue.size() >= MAX_EVENT_QUEUE_SIZE)
            {
                m_emailEventQueue.pop();
                dlog_warn("事件邮件队列已满，丢弃最旧邮件");
            }
            m_emailEventQueue.push(mpark::get<Network::EmailEventInfo_S>(emailRequest));
        }
        m_emailCond.notify_one();
        nRet = 0;
    }
    else
    {
        dlog_error("未知的邮件处理类型");
        nRet = -1;
    }
    return nRet;
}

void CEmailManage::GetEmailInfo(Network::EmailInfo_S &stEmInfo)
{
    Convert::read_file(m_configFile, stEmInfo);
}

/* 设置邮件信息 */
int CEmailManage::SetEmailInfo(const Network::EmailInfo_S &stNewEmInfo)
{
    /* 获取发件人信息 */
    if (!stNewEmInfo.stSender.strName.empty() && !stNewEmInfo.stSender.strAddress.empty())
    {
        m_EmInfo.stSender = stNewEmInfo.stSender;
        dlog_info("设置发件人名字：%s，发件人地址：%s", stNewEmInfo.stSender.strName.c_str(), stNewEmInfo.stSender.strAddress.c_str());
    }
    else
    {
        dlog_info("清空发件人信息");
    }

    /* 获取SMTP服务器信息 */
    if ((stNewEmInfo.stServer.nPort > 0) && !stNewEmInfo.stServer.strAddress.empty())
    {
        m_EmInfo.stServer = stNewEmInfo.stServer;
        dlog_info("设置SMTP服务器端口号：%d，服务器地址：%s", stNewEmInfo.stServer.nPort, stNewEmInfo.stServer.strAddress.c_str());
    }
    else
    {
        dlog_info("清空SMTP服务器信息");
    }

    /* 是否开启加密 */
    m_EmInfo.bTlsEnable = stNewEmInfo.bTlsEnable;

    /* 是否开启TLS认证 */
    if (m_EmInfo.bTlsEnable)
    {
        dlog_info("设置加密");
    }
    else
    {
        dlog_info("不设置加密");
    }
    /* 是否开启图片附件 */
    m_EmInfo.bImageAttachment = stNewEmInfo.bImageAttachment;
    if (stNewEmInfo.nCaptureTimeInterval > 0)
    {
        m_EmInfo.nCaptureTimeInterval = stNewEmInfo.nCaptureTimeInterval;
        dlog_info("抓图间隔设置为：%d", m_EmInfo.nCaptureTimeInterval);
    }

    /* 服务器认证信息 */
    m_EmInfo.bEnServerAuthentication = stNewEmInfo.bEnServerAuthentication;
    /* 设置服务器用户名和密码 */
    if (m_EmInfo.bEnServerAuthentication)
    {
        dlog_info("开启SMTP服务器认证");
        if (!stNewEmInfo.strUserName.empty() && !stNewEmInfo.strPassword.empty())
        {
            m_EmInfo.strUserName = stNewEmInfo.strUserName;
            m_EmInfo.strPassword = stNewEmInfo.strPassword;
            dlog_info("设置SMTP服务器认证用户名：%s，密码：%s", m_EmInfo.strUserName.c_str(), m_EmInfo.strPassword.c_str());
        }
        else
        {
            dlog_info("清空SMTP服务器认证信息");
        }
    }
    else
    {
        dlog_info("不开启SMTP服务器认证");
    }

    /* 收件人信息初始化 */
    m_EmInfo.stRecipient.clear();
    m_EmInfo.stRecipient = stNewEmInfo.stRecipient;
    Convert::write_file(m_configFile, m_EmInfo);
    return 0;
}

/* 发送测试邮件 */
int CEmailManage::SendTestEmail(const Network::EmailUser_S &stTestRecipient, std::function<void(int)> result)
{
    /* 检查是否已经在测试中 */
    bool bExpected = false;
    if (!m_bTesting.compare_exchange_strong(bExpected, true))
    {
        result(-1);
        dlog_error("正在测试，等待测试完成再测试");
        return -1;
    }

    auto thrRun = [this](Network::EmailUser_S stTestRecipient, std::function<void(int)> result)
    {
        CSmtp stSmtp;
        int nRet;
        /* 是否开启服务器认证 */
        if (m_EmInfo.bEnServerAuthentication)
        {

            if (!m_EmInfo.strUserName.empty() && !m_EmInfo.strPassword.empty())
            {
                stSmtp.SetLogin(m_EmInfo.strUserName.c_str());
                stSmtp.SetPassword(m_EmInfo.strPassword.c_str());
                dlog_info("认证的SMTP服务器用户名：%s，密码：%s", m_EmInfo.strUserName.c_str(), m_EmInfo.strPassword.c_str());
            }
            else
            {
                dlog_error("未设置SMTP服务器认证信息！");
                nRet = -1;
            }
            if ((m_EmInfo.stServer.nPort > 0) && !m_EmInfo.stServer.strAddress.empty())
            {
                stSmtp.SetSMTPServer(m_EmInfo.stServer.strAddress.c_str(), m_EmInfo.stServer.nPort, m_EmInfo.bEnServerAuthentication);
                dlog_info("发送的SMTP服务器端口号：%d，服务器地址：%s", m_EmInfo.stServer.nPort, m_EmInfo.stServer.strAddress.c_str());
            }
            else
            {
                dlog_error("未设置发送的SMTP服务器！");
                nRet = -1;
            }
        }
        else
        {
            if ((m_EmInfo.stServer.nPort > 0) && !m_EmInfo.stServer.strAddress.empty())
            {
                stSmtp.SetSMTPServer(m_EmInfo.stServer.strAddress.c_str(), m_EmInfo.stServer.nPort, m_EmInfo.bEnServerAuthentication);
                dlog_info("发送的SMTP服务器端口号：%d，服务器地址：%s", m_EmInfo.stServer.nPort, m_EmInfo.stServer.strAddress.c_str());
            }
            else
            {
                dlog_error("未设置发送的SMTP服务器！");
                nRet = -1;
            }
        }

        /* 是否开启TLS认证 */
        if (m_EmInfo.bTlsEnable)
        {
            dlog_info("使用加密");
            stSmtp.SetSecurityType(USE_SSL);
        }
        else
        {
            dlog_info("不使用加密");
            stSmtp.SetSecurityType(NO_SECURITY);
        }

        /* 设置发件人信息 */
        if (!m_EmInfo.stSender.strName.empty() && !m_EmInfo.stSender.strAddress.empty())
        {
            stSmtp.SetSenderName(m_EmInfo.stSender.strName.c_str());
            stSmtp.SetSenderMail(m_EmInfo.stSender.strAddress.c_str());
            dlog_info("测试邮件设置发件人名字：%s，发件人地址：%s",
                      m_EmInfo.stSender.strName.c_str(),
                      m_EmInfo.stSender.strAddress.c_str());
        }
        else
        {
            dlog_error("测试邮件设置发件人失败");
            nRet = -1;
        }

        stSmtp.DelRecipients();
        /* 设置收件人信息 */
        if (!stTestRecipient.strAddress.empty() && !stTestRecipient.strName.empty())
        {
            stSmtp.AddRecipient(stTestRecipient.strAddress.c_str(), stTestRecipient.strName.c_str());
            dlog_info("测试邮件设置收件人名字：%s，收件人地址：%s", stTestRecipient.strName.c_str(), stTestRecipient.strAddress.c_str());
        }
        else
        {
            dlog_error("测试邮件设置收件人失败");
            nRet = -1;
        }

        /* 设置邮件标题 */
        stSmtp.SetSubject(TEST_SUBJECT);
        /* 设置邮件优先级 */
        stSmtp.SetXPriority(XPRIORITY_NORMAL);
        stSmtp.SetXMailer(XMAILER);

        /* 邮件消息 */
        stSmtp.AddMsgLine(TEST_MESSAGE);

        dlog_info("开始发送测试邮件");
        /* 发送邮件 */
        if (!stSmtp.Send())
        {
            dlog_error("测试邮件发送失败");
            stSmtp.ClearMessage();
            nRet = -1;
        }
        else
        {
            nRet = 0;
            dlog_info("测试邮件发送成功");
        }

        stSmtp.ClearMessage();
        stSmtp.DisconnectRemoteServer();
        m_bTesting.store(false);
        result(nRet);
    };
    std::thread thr(thrRun, stTestRecipient, result);
    thr.detach();

    return 0;
}

/* 发送事件触发邮件 */
int CEmailManage::SendEventEmail(const Network::EmailEventInfo_S &stEventInfo)
{
    CSmtp stSmtp;
    /* 是否开启服务器认证 */
    if (m_EmInfo.bEnServerAuthentication)
    {
        if (!m_EmInfo.strUserName.empty() && !m_EmInfo.strPassword.empty())
        {
            stSmtp.SetLogin(m_EmInfo.strUserName.c_str());
            stSmtp.SetPassword(m_EmInfo.strPassword.c_str());
            dlog_info("认证的SMTP服务器用户名：%s，密码：%s", m_EmInfo.strUserName.c_str(), m_EmInfo.strPassword.c_str());
        }
        else
        {
            dlog_error("未设置SMTP服务器认证信息！");
            return -1;
        }
        if ((m_EmInfo.stServer.nPort > 0) && !m_EmInfo.stServer.strAddress.empty())
        {
            stSmtp.SetSMTPServer(m_EmInfo.stServer.strAddress.c_str(), m_EmInfo.stServer.nPort, m_EmInfo.bEnServerAuthentication);
            dlog_info("发送的SMTP服务器端口号：%d，服务器地址：%s", m_EmInfo.stServer.nPort, m_EmInfo.stServer.strAddress.c_str());
        }
        else
        {
            dlog_error("未设置发送的SMTP服务器！");
            return -1;
        }
    }
    else
    {
        if ((m_EmInfo.stServer.nPort > 0) && !m_EmInfo.stServer.strAddress.empty())
        {
            stSmtp.SetSMTPServer(m_EmInfo.stServer.strAddress.c_str(), m_EmInfo.stServer.nPort, m_EmInfo.bEnServerAuthentication);
            dlog_info("发送的SMTP服务器端口号：%d，服务器地址：%s", m_EmInfo.stServer.nPort, m_EmInfo.stServer.strAddress.c_str());
        }
        else
        {
            dlog_error("未设置发送的SMTP服务器！");
            return -1;
        }
    }

    /* 是否开启TLS认证 */
    if (m_EmInfo.bTlsEnable)
    {
        dlog_info("使用加密");
        stSmtp.SetSecurityType(USE_SSL);
    }
    else
    {
        dlog_info("不使用加密");
        stSmtp.SetSecurityType(NO_SECURITY);
    }

    /* 设置发件人信息 */
    if (!m_EmInfo.stSender.strName.empty() && !m_EmInfo.stSender.strAddress.empty())
    {
        stSmtp.SetSenderName(m_EmInfo.stSender.strName.c_str());
        stSmtp.SetSenderMail(m_EmInfo.stSender.strAddress.c_str());
        dlog_info("事件触发邮件设置发件人名字：%s，发件人地址：%s",
                  m_EmInfo.stSender.strName.c_str(),
                  m_EmInfo.stSender.strAddress.c_str());
    }
    else
    {
        dlog_error("事件触发邮件设置发件人失败");
        return -1;
    }

    /* 设置收件人信息 */
    int nAddedRecipients = 0;
    for (const auto &recipient : m_EmInfo.stRecipient)
    {
        if (!recipient.strAddress.empty() && !recipient.strName.empty())
        {
            stSmtp.AddRecipient(recipient.strAddress.c_str(), recipient.strName.c_str());
            dlog_info("事件触发邮件添加收件人名字：%s，收件人地址：%s", recipient.strName.c_str(), recipient.strAddress.c_str());
            ++nAddedRecipients;
        }
    }

    if (nAddedRecipients == 0)
    {
        dlog_error("事件触发邮件收件人列表为空");
        return -1;
    }

    /* 设置邮件标题和内容 */
    if (!stEventInfo.strSubject.empty() && !stEventInfo.strMessage.empty())
    {
        stSmtp.SetSubject(stEventInfo.strSubject.c_str());
        stSmtp.AddMsgLine(stEventInfo.strMessage.c_str());
        dlog_info("事件触发邮件标题：%s，内容：%s", stEventInfo.strSubject.c_str(), stEventInfo.strMessage.c_str());

        /* 是否使用图片附件 */
        if (m_EmInfo.bImageAttachment)
        {
            /* 图片附件 */
            for (auto &filename : stEventInfo.vecImageFile)
            {
                stSmtp.AddAttachment(filename.c_str());
                dlog_info("图片附件：%s", filename.c_str());
            }
        }
    }
    else
    {
        dlog_error("事件触发邮件内容为空");
        return -1;
    }

    /* 设置邮件优先级 */
    stSmtp.SetXPriority(XPRIORITY_NORMAL);
    stSmtp.SetXMailer(XMAILER);

    stSmtp.SetCharSet("UTF-8");

    dlog_info("开始发送事件邮件");
    /* 发送邮件 */
    if (!stSmtp.Send())
    {
        dlog_error("事件邮件发送失败");
        stSmtp.ClearMessage();
        return -1;
    }
    dlog_info("事件邮件发送成功");
    stSmtp.ClearMessage();
    stSmtp.DisconnectRemoteServer();

    return 0;
}

void CEmailManage::SendEmailPeriodically(int nCaptureTimeInterval)
{
    while (!bEnStopThread)
    {
        /* 发送邮件 */
        // SendEventEmail();

        /* 线程睡眠，等待指定的时间间隔 */
        std::this_thread::sleep_for(std::chrono::seconds(nCaptureTimeInterval));
    }
}

/* 启动线程，定时发送邮件 */
void CEmailManage::StartSending(int nCaptureTimeInterval)
{
    SendThread = std::unique_ptr<std::thread>(new std::thread(&CEmailManage::SendEmailPeriodically, this, nCaptureTimeInterval));
}

/* 停止发送邮件 */
void CEmailManage::StopSending()
{
    bEnStopThread = true;
    if (SendThread->joinable())
    {
        SendThread->join();
    }
}