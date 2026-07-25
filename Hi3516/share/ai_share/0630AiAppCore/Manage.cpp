#include "Manage.hpp"

#include "share_define.h"
#include "TimerManager.hpp"
#include "ToolFunc.hpp"

using namespace Ai0630_NS;
using namespace std::chrono_literals;
using namespace TimerManager_NS;

Manage::Manage()
{
    m_pDataComm     = std::make_shared<DataComm>(); /* 先设置默认值，后面有逻辑修改 */
    m_pCtrlComm     = std::make_shared<CtrlComm>();
    m_pDevComm      = std::make_shared<DevComm>();
    m_pFaceManage   = std::make_shared<FaceManage>();
    m_pResultModule = std::make_shared<ResultModule>();

    m_pGetClassInfo = std::make_shared<GetClassInfo>();

    m_pDevComm->bindSlot(m_pDataComm.get(), &DataComm::init);
    m_pDevComm->bindSlot(m_pCtrlComm.get(), &CtrlComm::init);
    m_pDataComm->bindSlot(m_pFaceManage.get(), &FaceManage::recvData);
    m_pDataComm->bindSlot(m_pResultModule.get(), &ResultModule::recvData);

    /* 第一步 */
    m_pGetClassInfo->bindSlot(this, &Manage::recvClassData); /* 这个必须在 &DataComm::recvClassData前面 */
    /* 第二布 */
    m_pGetClassInfo->bindSlot(m_pDataComm.get(), &DataComm::recvClassData); /* 这个必须在&Manage::recvClassData后面 */

    m_pFaceManage->bindSlot(m_pResultModule.get(), &ResultModule::recvFaceData);
    m_pDataComm->bindFeatureSlot(m_pFaceManage.get(), &FaceManage::recvFaceFeature);

    initTask();

    /* 测试 */
#if 0
    ClassInfo_S stClassInfo;
    stClassInfo.nClassId     = 1;
    stClassInfo.strClassName = "测试班级";
    FaceLibsInfo_S stFaceLibsInfo;
    stFaceLibsInfo.nIdentity        = 0;
    stFaceLibsInfo.nClassId         = 1;
    stFaceLibsInfo.nMemberId        = 1;
    stFaceLibsInfo.strName          = "严泽辉";
    stFaceLibsInfo.strRemotePicPath = "/root/1.png";
    stFaceLibsInfo.strLocalPicPath  = "/root/1.png";
    stFaceLibsInfo.strPicName       = "严泽辉";
    stClassInfo.listStuInfo.push_back(stFaceLibsInfo);
    stFaceLibsInfo.nIdentity  = 1;
    stFaceLibsInfo.nMemberId  = 3;
    stFaceLibsInfo.strPicName = "严老师";
    stFaceLibsInfo.strName    = "严老师";
    stClassInfo.listTeaInfo.push_back(stFaceLibsInfo);
    stFaceLibsInfo.nIdentity        = 0;
    stFaceLibsInfo.nClassId         = 1;
    stFaceLibsInfo.nMemberId        = 2;
    stFaceLibsInfo.strName          = "吴才朋";
    stFaceLibsInfo.strRemotePicPath = "/root/2.jpg";
    stFaceLibsInfo.strLocalPicPath  = "/root/2.jpg";
    stFaceLibsInfo.strPicName       = "吴才朋";
    stClassInfo.listStuInfo.push_back(stFaceLibsInfo);
    m_pGetClassInfo->setClassInfo(stClassInfo);
    recvClassData(stClassInfo);
    m_pDataComm->recvClassData(stClassInfo);
#endif
}

/* 设置获取流数据的回调函数 */
void Manage::setNotifyGetSteamData(notifyGetSteamDataFunc func)
{
    m_notifyGetSteamData = func;
}

/* 设置考勤控制的回调函数 */
void Ai0630_NS::Manage::setPTZControl(PTZControlFunc func)
{
    m_setPTZControl = func;
}

/* 发送流数据 */
void Manage::sendSteamData(char* pchData, int nDataLen, int nCode, int nRecordTime)
{
    if (m_pDataComm)
    {
        int nClassId = 0;
        if (m_pGetClassInfo)
        {
            nClassId = m_pGetClassInfo->getClassInfo().nClassId;
        }
        m_pDataComm->recvData(pchData, nDataLen, nCode, nRecordTime, nClassId);
    }
}

/* 整合 */
void Ai0630_NS::Manage::finalize(const void* pParam)
{
    if (m_pResultModule)
    {
        m_pResultModule->finalize(pParam);
    }

    m_bRecord.store(false);
    checkTask();
}

/* PPT切换 */
BlError_E Ai0630_NS::Manage::pptSwitch()
{
    dlog(LOG_INFO, "【AI分析】 PPT切换");
    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录PPT切换信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->pptSwitch();
    }
    return enRet;
}

/* 切换老师画面 */
BlError_E Ai0630_NS::Manage::switchTeacherScreen()
{
    dlog(LOG_INFO, "【AI分析】 切换老师画面");
    /* 是否学生画面 */
    m_bStudentScreen.store(false);
    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录切换老师画面信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->switchTeacherScreen();
    }
    return enRet;
}

/* 切换学生画面 */
BlError_E Ai0630_NS::Manage::switchStudentScreen()
{
    dlog(LOG_INFO, "【AI分析】 切换学生画面");
    /* 是否学生画面 */
    m_bStudentScreen.store(true);

    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录切换学生画面信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->switchStudentScreen();
    }
    return enRet;
}

/* 切换学生特写 */
BlError_E Ai0630_NS::Manage::switchStudentCloseUp(bool bValue)
{
    dlog(LOG_INFO, "【AI分析】 切换学生特写[%d]", bValue);
    m_bStudentCloseUp.store(bValue);

    int nClassId = 0;
    if (m_pGetClassInfo)
    {
        nClassId = m_pGetClassInfo->getClassInfo().nClassId;
    }

    UserHeaderInfo_S stUserHeaderInfo;
    stUserHeaderInfo.nClassId   = nClassId;
    stUserHeaderInfo.nClassTime = m_nRecordTime;
    stUserHeaderInfo.lTimestamp = ToolFunc::getTimeStampUs();
    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录切换学生特写信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->switchStudentCloseUp(stUserHeaderInfo, bValue);
    }
    return enRet;
}

/* 切换老师是否在讲台 */
BlError_E Ai0630_NS::Manage::switchTeacherPodium(bool bValue)
{
    dlog(LOG_INFO, "【AI分析】 切换老师是否在讲台[%d]", bValue);
    m_bTeacherPodium.store(bValue);

    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录老师是否在讲台信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->switchTeacherPodium(bValue);
    }
    return enRet;
}

/* 开始互动 */
BlError_E Ai0630_NS::Manage::startInteraction()
{
    dlog(LOG_INFO, "【AI分析】 开始互动");
    int nClassId = 0;
    if (m_pGetClassInfo)
    {
        nClassId = m_pGetClassInfo->getClassInfo().nClassId;
    }

    UserHeaderInfo_S stUserHeaderInfo;
    stUserHeaderInfo.nClassId   = nClassId;
    stUserHeaderInfo.nClassTime = m_nRecordTime;
    stUserHeaderInfo.lTimestamp = ToolFunc::getTimeStampUs();
    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录互动信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->startInteraction(stUserHeaderInfo);
    }

    return enRet;
}

/* 开始巡视行为 */
BlError_E Ai0630_NS::Manage::startTour()
{
    dlog(LOG_INFO, "【AI分析】 开始巡视行为");
    int nClassId = 0;
    if (m_pGetClassInfo)
    {
        nClassId = m_pGetClassInfo->getClassInfo().nClassId;
    }

    UserHeaderInfo_S stUserHeaderInfo;
    stUserHeaderInfo.nClassId   = nClassId;
    stUserHeaderInfo.nClassTime = m_nRecordTime;
    stUserHeaderInfo.lTimestamp = ToolFunc::getTimeStampUs();
    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录互动信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->startTour(stUserHeaderInfo);
    }
    return enRet;
}

/* 开始教授行为 */
BlError_E Ai0630_NS::Manage::startTaught()
{
    dlog(LOG_INFO, "【AI分析】 开始教授行为");
    int nClassId = 0;
    if (m_pGetClassInfo)
    {
        nClassId = m_pGetClassInfo->getClassInfo().nClassId;
    }

    UserHeaderInfo_S stUserHeaderInfo;
    stUserHeaderInfo.nClassId   = nClassId;
    stUserHeaderInfo.nClassTime = m_nRecordTime;
    stUserHeaderInfo.lTimestamp = ToolFunc::getTimeStampUs();
    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录互动信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->startTaught(stUserHeaderInfo);
    }
    return enRet;
}

/* 老师板书切换 */
BlError_E Ai0630_NS::Manage::teacherBoard(bool bValue)
{
    dlog(LOG_INFO, "【AI分析】 老师板书切换");
    int nClassId = 0;
    if (m_pGetClassInfo)
    {
        nClassId = m_pGetClassInfo->getClassInfo().nClassId;
    }

    UserHeaderInfo_S stUserHeaderInfo;
    stUserHeaderInfo.nClassId   = nClassId;
    stUserHeaderInfo.nClassTime = m_nRecordTime;
    stUserHeaderInfo.lTimestamp = ToolFunc::getTimeStampUs();
    if (!m_pResultModule)
    {
        return NOK;
    }

    /* 判断当前为录制状态则记录互动信息 */
    BlError_E enRet = OK;
    if (m_bRecord.load())
    {
        enRet = m_pResultModule->teacherBoard(stUserHeaderInfo, bValue);
    }
    return enRet;
}

/* 设置录制时间 */
BlError_E Ai0630_NS::Manage::setRecordTime(int nRecordTime)
{
    m_nRecordTime = nRecordTime;
    return OK;
}

/* 设置录制格式 */
BlError_E Ai0630_NS::Manage::setRecordStatus(int nStatus)
{
    dlog(LOG_INFO, "设置当前录制状态为【%s】", (RECORD_RESUME == nStatus) ? "录制" : "空闲");
    if (RECORD_RESUME == nStatus)
    {
        m_bRecord.store(true);
    }
    else
    {
        m_bRecord.store(false);
    }

    checkTask();

    /* 作用：防止刚开始录制的时候缺少一些数据 */
    if (m_bRecord.load())
    {
        /* 开始录制的时候判断老师是否在讲台 */
        if (m_bTeacherPodium.load())
        {
            /* 在讲台，则设置当前老师为授课行为 */
            startTaught();
        }
        else
        {
            /* 不在讲台，则设置当前老师为巡视行为 */
            startTour();
        }
    }

    return OK;
}

/* 设置服务器IP */
BlError_E Ai0630_NS::Manage::setServerIp(std::string strIp)
{
    if (m_pGetClassInfo)
    {
        m_pGetClassInfo->setPlatformIp(strIp);
    }
    return OK;
}

/* 添加课堂分贝值 */
void Ai0630_NS::Manage::addClassDb(int nDb)
{
    if (m_pResultModule && m_bRecord.load())
    {
        m_pResultModule->addClassDb(nDb);
    }
}

/* 获取班级信息 */
Ai0630_NS::ClassInfo_S Ai0630_NS::Manage::getClassInfo()
{
    Ai0630_NS::ClassInfo_S stClassInfo = m_pGetClassInfo->getClassInfo();

    dlog(LOG_INFO, "获取班级信息: 班级ID[%d], 班级名称[%s]",
         stClassInfo.nClassId,
         stClassInfo.strClassName.c_str());
    std::list<FaceLibsInfo_S> listOutInfo;
    listOutInfo.clear();
    m_pFaceManage->searchFaceInfoByTable(std::to_string(stClassInfo.nClassId), listOutInfo);

    stClassInfo.listTeaInfo.clear();
    stClassInfo.listStuInfo.clear();
    for (auto& item : listOutInfo)
    {
        if (item.nIdentity == 1)
        {
            stClassInfo.listTeaInfo.push_back(item);
        }
        else if (item.nIdentity == 0)
        {
            stClassInfo.listStuInfo.push_back(item);
        }
    }

    return stClassInfo;
}

/* 初始化定时任务 */
void Manage::initTask()
{
    m_nFaceTaskId = TimerManager::getInstance().addTask(
        "人脸识别",
        1s,
        6s,
        0,
        std::bind(&Manage::taskGetSteam, this, std::placeholders::_1),
        std::make_shared<int>(toInt(CommCode_E::AI_COM_FACE)));

    m_nHeadTaskId = TimerManager::getInstance().addTask(
        "人头识别",
        2s,
        6s,
        0,
        std::bind(&Manage::taskGetSteam, this, std::placeholders::_1),
        std::make_shared<int>(toInt(CommCode_E::AI_COM_HEAD)));

    m_nEmoTaskId = TimerManager::getInstance().addTask(
        "班级表情识别",
        3s,
        6s,
        0,
        std::bind(&Manage::taskGetSteam, this, std::placeholders::_1),
        std::make_shared<int>(toInt(CommCode_E::AI_COM_CLASS_EMO)));

    m_nBehaviorTaskId = TimerManager::getInstance().addTask(
        "班级行为分析",
        4s,
        6s,
        0,
        std::bind(&Manage::taskGetSteam, this, std::placeholders::_1),
        std::make_shared<int>(toInt(CommCode_E::AI_COM_CLASS_BEHAVIOR)));

    m_nStTaskId = TimerManager::getInstance().addTask(
        "学生个人分析",
        5s,
        6s,
        0,
        std::bind(&Manage::taskGetStudentSteam, this, std::placeholders::_1),
        std::make_shared<int>(toInt(CommCode_E::AI_COM_ST_ANALYSE)));

    m_nTeTaskId = TimerManager::getInstance().addTask(
        "老师个人分析",
        6s,
        6s,
        0,
        std::bind(&Manage::taskGetSteam, this, std::placeholders::_1),
        std::make_shared<int>(toInt(CommCode_E::AI_COM_TE_ANALYSE)));
}

/* 检查状态，判断是否开启定时器 */
void Ai0630_NS::Manage::checkTask()
{
    if (m_bRecord.load())
    {
        /* 人脸识别 */
        if (m_stAiSwitchInfo.nStuAttendSwitch == 1 ||
            m_stAiSwitchInfo.nTecAttendSwitch == 1)
        {
            TimerManager::getInstance().resumeTask(m_nFaceTaskId);
        }
        else
        {
            TimerManager::getInstance().pauseTask(m_nFaceTaskId);
        }

        /* 人数统计 */
        if (m_stAiSwitchInfo.nStuCountSwitch == 1)
        {
            TimerManager::getInstance().resumeTask(m_nHeadTaskId);
        }
        else
        {
            TimerManager::getInstance().pauseTask(m_nHeadTaskId);
        }

        /* 表情识别 */
        if (m_stAiSwitchInfo.nEmoSwitch == 1)
        {
            TimerManager::getInstance().resumeTask(m_nEmoTaskId);
        }
        else
        {
            TimerManager::getInstance().pauseTask(m_nEmoTaskId);
        }

        /* 行为分析 */
        if (m_stAiSwitchInfo.nStuBehavSwitch == 1 ||
            m_stAiSwitchInfo.nTecBehavSwitch == 1)
        {
            TimerManager::getInstance().resumeTask(m_nBehaviorTaskId);
            TimerManager::getInstance().resumeTask(m_nStTaskId);
            TimerManager::getInstance().resumeTask(m_nTeTaskId);
        }
        else
        {
            TimerManager::getInstance().pauseTask(m_nBehaviorTaskId);
            TimerManager::getInstance().pauseTask(m_nStTaskId);
            TimerManager::getInstance().pauseTask(m_nTeTaskId);
        }
    }
    else
    {
        TimerManager::getInstance().pauseTask(m_nFaceTaskId);
        TimerManager::getInstance().pauseTask(m_nHeadTaskId);
        TimerManager::getInstance().pauseTask(m_nEmoTaskId);
        TimerManager::getInstance().pauseTask(m_nBehaviorTaskId);
        TimerManager::getInstance().pauseTask(m_nStTaskId);
        TimerManager::getInstance().pauseTask(m_nTeTaskId);
    }
}

/* 获取stream数据定时函数 */
void Manage::taskGetSteam(std::shared_ptr<void> param)
{
    if (!m_notifyGetSteamData)
    {
        return;
    }

    auto* pArgs = static_cast<int*>(param.get());
    if (!pArgs)
    {
        return;
    }

    m_notifyGetSteamData(*pArgs);
}

/* 获取stream数据定时函数 */
void Ai0630_NS::Manage::taskGetStudentSteam(std::shared_ptr<void> param)
{
    if (!m_notifyGetSteamData ||
        !m_setPTZControl)
    {
        return;
    }

    auto* pArgs = static_cast<int*>(param.get());
    if (!pArgs)
    {
        return;
    }


    if (!m_bStudentCloseUp.load() &&
        (!m_bStudentScreen.load() ||
         (!m_bTeacherPodium.load() && m_bStudentScreen.load())))
    {
        if (m_setPTZControl())
        {
            /* 调用成功 */
            m_notifyGetSteamData(*pArgs);
        }
    }
    else
    {
        dlog(LOG_ERROR, "学生特写抢夺考勤摄像头权限, 暂停学生考勤 m_bStudentScreen[%d] m_bTeacherPodium[%d] m_bStudentCloseUp[%d]",
             m_bStudentScreen.load(),
             m_bTeacherPodium.load(),
             m_bStudentCloseUp.load());
    }
}

/* 接受班级信息 */
void Ai0630_NS::Manage::recvClassData(ClassInfo_S stClassInfo)
{
    m_pFaceManage->deleteFaceTable();
    m_pFaceManage->deleteHumanTable();
}
