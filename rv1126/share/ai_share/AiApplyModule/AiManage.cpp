/*
 * @FilePath     : AiManage.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-03-25 10:27:49
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2025-03-07 17:10:26
 * @Description  :
 */
#include "AiManage.hpp"

#include <net/if.h>

#include "dlog.h"
#include "edukit_network.h"
#include "PublicFunc.hpp"

/* 上传班级信息目标路径 */
#define UPLOAD_CLASSINFO_PATHBASE ("/opt/course/project/upload/")

BlError_E AiManage_NS::CAiManage::Enable_local_analysis()
{
    set_transcription(m_bTtranscription);
#ifdef USE_LOCAL_AI
    if (m_pAiLocal)
    {
        m_pAiLocal->initDiscipline();
        m_pAiLocal->initHeadCount();
        m_pAiLocal->initBehavior();
    }
#endif
    return OK;
}

BlError_E AiManage_NS::CAiManage::Disable_local_analysis()
{
    set_transcription(m_bTtranscription);
#ifdef USE_LOCAL_AI
    if (m_pAiLocal)
    {
        m_pAiLocal->uninitDiscipline();
        m_pAiLocal->uninitHeadCount();
        m_pAiLocal->uninitBehavior();
    }
#endif
    return OK;
}

/* 设置AI管理模块的参数 */
BlError_E AiManage_NS::CAiManage::set_params(Param_S stParam)
{

    m_stParam = stParam;

    /* 初始化通讯 */
    initComm_localServer();

    /* 初始化平台管理 */
    initPlatForm();

    /* 初始化通讯 */
    initComm_aiServer();

    /* 设置个算法的参数 */
    set_countStudentsParam(stParam.stCountStudents);

    set_teTelephoneCallsParam(stParam.stTeTelephoneCalls);
    set_stPlayPhoneParam(stParam.stStuPlayPhone);
    set_stDisciplineParam(stParam.stStuDiscipline);
    set_studentBehaviorParam(stParam.stStudentBehavior);
    set_studentBehaviorExParam(stParam.stBehaviorExParam);
    set_emoDetecrParam(stParam.stEmoDetecr);
    set_teFaceDetecrParam(stParam.stTeFaceDetecr);
    set_stFaceDetecrParam(stParam.stStFaceDetecr);
    set_trackTeacherParam(stParam.stTrackTeacher);
    set_boardDetecrParam(stParam.stBoardDetecr);

    /* 初始化结果处理类 */
    init_results();

    /* 初始化解析类 */
    init_parse();

    m_nAiServerStatus.store(0);

    return OK;
}

/*更新AI管理模块的参数*/
BlError_E AiManage_NS::CAiManage::update_params(Param_S stParam)
{
    m_stParam = stParam;

    set_countStudentsParam(stParam.stCountStudents);
    set_teTelephoneCallsParam(stParam.stTeTelephoneCalls);
    /* 学生玩手机识别 */
    AlgorithmParam_S stStuPlayPhone;
    stStuPlayPhone.bOpen      = true;
    stStuPlayPhone.enMode     = SERVER_MODE;
    stStuPlayPhone.nIntervals = 2 * 1000;

    /* 学生课堂纪律识别 */
    AlgorithmParam_S stStuDiscipline;
    stStuDiscipline.bOpen      = true;
    stStuDiscipline.enMode     = LOCAL_MODE;
    stStuDiscipline.nIntervals = 1 * 1000;

    set_stPlayPhoneParam(stStuPlayPhone);
    set_stDisciplineParam(stStuDiscipline);
    set_studentBehaviorParam(stParam.stStudentBehavior);
    set_studentBehaviorExParam(stParam.stBehaviorExParam);
    set_emoDetecrParam(stParam.stEmoDetecr);
    set_teFaceDetecrParam(stParam.stTeFaceDetecr);
    set_stFaceDetecrParam(stParam.stStFaceDetecr);
    set_trackTeacherParam(stParam.stTrackTeacher);
    set_boardDetecrParam(stParam.stBoardDetecr);

    return OK;
}

/*获取AI管理模块的参数*/
BlError_E AiManage_NS::CAiManage::get_params(Param_S& stParam)
{
    stParam = m_stParam;
    return OK;
}

/* 获取教室信息 */
BlError_E AiManage_NS::CAiManage::get_classInfo()
{
    BlError_E enRetCode = OK;

    if (nullptr == m_pUpdateFaceTimerKeep)
    {
        /* 创建计时器 */
        TimeKeepNeedParam_S stNeedParam;
        stNeedParam.nTimeMs    = 60 * 1000;
        m_pUpdateFaceTimerKeep = TimeKeep_alloc(stNeedParam);
        if (m_pUpdateFaceTimerKeep)
        {
            m_pUpdateFaceTimerKeep->stExParam.pUserFun = updateFace_taskFun;
            m_pUpdateFaceTimerKeep->stExParam.pParam   = this;
            TimeKeep_init(m_pUpdateFaceTimerKeep);
            TImeKeep_start(m_pUpdateFaceTimerKeep);
        }
    }
    else
    {
        return ERR_SYNC_ING;
    }
    if (m_pPlatformManage)
    {
        m_pPlatformManage->push_cmdQueue(PlatformManage_NS::AI_PLATFORM_CMD_HUMAN_INFO);
    }


    return OK_NO_RETURN;
}

BlError_E AiManage_NS::CAiManage::classInfoCallback(PlatformManage_NS::DataInfo_S stDataInfo, BlError_E enRetCode, void* pHandle)
{
    /* 获取班级信息 */
    if (m_stParam.getPlatformClassInfo && enRetCode >= OK)
    {
        m_stParam.getPlatformClassInfo(stDataInfo);
    }
    else
    {
        dlog(LOG_ERROR, "获取数据失败");
    }

    int         nRet = 0;
    char        chCmd[1000];
    std::string strOutJson;
    std::string strFilePath;
    std::string strTarPath;
    std::string strTarName;
    /* 存储班级信息数据 */
    ClassInfo_S stAiClassInfo = { 0 };

    CAiManage* pParam = (CAiManage*)pHandle;
    if (nullptr == pParam ||
        nullptr == pParam->m_stParam.setClassId ||
        nullptr == pParam->m_stParam.getAiServerIp)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 参数错误");
        pParam->m_stParam.notifyWsFaceState(ERR_GET_FAULT);
        close_updateFaceTimer(pHandle);
        return ERR_PARAM;
    }

    if (enRetCode < OK)
    {
        pParam->m_stParam.notifyWsFaceState(ERR_GET_FAULT);
        close_updateFaceTimer(pHandle);
        goto EXIT;
    }

    if (enRetCode == OK_EXIST)
    {
        dlog(LOG_INFO, "信息无变动，无需同步");
        pParam->m_stParam.notifyWsFaceState(OK);
        close_updateFaceTimer(pHandle);
        return enRetCode;
    }

    strTarName = std::to_string(stDataInfo.stClassInfo.nId) + std::string(".tar.gz");
    sprintf(chCmd,
            pParam->m_stParam.strUploadPath.c_str(),
            stDataInfo.strTarPath.c_str(),
            pParam->m_stParam.getAiServerIp().c_str());

    dlog(LOG_INFO, "[上传指令]-chCmd = %s", chCmd);


    /* 调用system函数执行命令 */
    nRet = system(chCmd);
    if (nRet != 0)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 资源上传失败");
        pParam->m_stParam.notifyWsFaceState(ERR_AI_SYNC_FAILED);
        close_updateFaceTimer(pHandle);
        pParam->m_pPlatformManage->clear_dataInfo();
        enRetCode = ERR_AI_SYNC_FAILED;
        goto EXIT;
    }

    strTarPath = UPLOAD_CLASSINFO_PATHBASE + strTarName;
    enRetCode  = pParam->m_pParseBase->convert(stDataInfo.stClassInfo.nId, strTarPath, strOutJson, (int)AI_UPDATE_FACE);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 通信转换失败 - 发送给AI服务器");
        pParam->m_stParam.notifyWsFaceState(ERR_PARAM);
        close_updateFaceTimer(pHandle);
        enRetCode = ERR_PARAM;
        goto EXIT;
    }

    pParam->sendAi_Data(strOutJson, (int)AI_UPDATE_FACE);

EXIT:
    int nCpySize = sizeof(stAiClassInfo.achClassName);
    if (strlen(stDataInfo.stClassInfo.strName.c_str()) > 0)
    {
        strncpy(stAiClassInfo.achClassName,
                stDataInfo.stClassInfo.strName.c_str(),
                nCpySize - 1);
        /* 确保以空字符终止 */
        stAiClassInfo.achClassName[nCpySize - 1] = '\0';
    }

    stAiClassInfo.nClassId = stDataInfo.stClassInfo.nId;

    stAiClassInfo.nTotal = stDataInfo.listTeaInfo.size() + stDataInfo.listStuInfo.size();

    if (stAiClassInfo.nTotal > 0)
    {
        /* 分配班级成员信息空间 */
        stAiClassInfo.pstMemberInfo = (ClassMemberInfo_S*)calloc(stAiClassInfo.nTotal, sizeof(ClassMemberInfo_S));
    }
    else
    {
        stAiClassInfo.pstMemberInfo = NULL;
    }

    if (stAiClassInfo.pstMemberInfo)
    {
        int nIndex = 0;
        for (auto teaInfo : stDataInfo.listTeaInfo)
        {
            stAiClassInfo.pstMemberInfo[nIndex].nCardId = 0;

            int nCpySize = sizeof(stAiClassInfo.pstMemberInfo[nIndex].achName);
            if (strlen(teaInfo.strName.c_str()) > 0)
            {
                strncpy(stAiClassInfo.pstMemberInfo[nIndex].achName,
                        teaInfo.strName.c_str(),
                        nCpySize - 1);
                /* 确保以空字符终止 */
                stAiClassInfo.pstMemberInfo[nIndex].achName[nCpySize - 1] = '\0';
            }

            nCpySize = sizeof(stAiClassInfo.pstMemberInfo[nIndex].achFilePath);
            if (strlen(teaInfo.strPath.c_str()) > 0)
            {
                strncpy(stAiClassInfo.pstMemberInfo[nIndex].achFilePath,
                        teaInfo.strPath.c_str(),
                        nCpySize - 1);
                /* 确保以空字符终止 */
                stAiClassInfo.pstMemberInfo[nIndex].achFilePath[nCpySize - 1] = '\0';

                /* 图片实际路径 */
                std::string strFileExten = getFileExtension(teaInfo.strPath);
                std::string strActPath   = std::to_string(stDataInfo.stClassInfo.nId) + std::string("/teacher/") + std::to_string(teaInfo.nId) + std::string("_") + teaInfo.strMd5 + strFileExten;
                nCpySize                 = sizeof(stAiClassInfo.pstMemberInfo[nIndex].achActualPath);
                strncpy(stAiClassInfo.pstMemberInfo[nIndex].achActualPath,
                        strActPath.c_str(),
                        nCpySize - 1);
                /* 确保以空字符终止 */
                stAiClassInfo.pstMemberInfo[nIndex].achActualPath[nCpySize - 1] = '\0';
            }

            stAiClassInfo.pstMemberInfo[nIndex].nUserId = teaInfo.nId;

            nIndex++;
        }

        for (auto stuInfo : stDataInfo.listStuInfo)
        {
            stAiClassInfo.pstMemberInfo[nIndex].nCardId = 1;

            int nCpySize = sizeof(stAiClassInfo.pstMemberInfo[nIndex].achName);
            strncpy(stAiClassInfo.pstMemberInfo[nIndex].achName,
                    stuInfo.strName.c_str(),
                    nCpySize - 1);
            /* 确保以空字符终止 */
            stAiClassInfo.pstMemberInfo[nIndex].achName[nCpySize - 1] = '\0';

            nCpySize = sizeof(stAiClassInfo.pstMemberInfo[nIndex].achFilePath);
            strncpy(stAiClassInfo.pstMemberInfo[nIndex].achFilePath,
                    stuInfo.strPath.c_str(),
                    nCpySize - 1);
            /* 确保以空字符终止 */
            stAiClassInfo.pstMemberInfo[nIndex].achFilePath[nCpySize - 1] = '\0';

            /* 图片实际路径 */
            std::string strFileExten = getFileExtension(stuInfo.strPath);
            std::string strActPath   = std::to_string(stDataInfo.stClassInfo.nId) + std::string("/student/") + std::to_string(stuInfo.nId) + std::string("_") + stuInfo.strMd5 + strFileExten;
            nCpySize                 = sizeof(stAiClassInfo.pstMemberInfo[nIndex].achActualPath);
            strncpy(stAiClassInfo.pstMemberInfo[nIndex].achActualPath,
                    strActPath.c_str(),
                    nCpySize - 1);
            /* 确保以空字符终止 */
            stAiClassInfo.pstMemberInfo[nIndex].achActualPath[nCpySize - 1] = '\0';

            stAiClassInfo.pstMemberInfo[nIndex].nUserId = stuInfo.nId;

            nIndex++;
        }
    }

    if (m_stParam.setClassInfo)
    {
        m_stParam.setClassInfo(stAiClassInfo);
    }

    /* 释放班级成员信息空间 */
    if (stAiClassInfo.pstMemberInfo)
    {
        free(stAiClassInfo.pstMemberInfo);
        stAiClassInfo.pstMemberInfo = NULL;
    }

    pParam->m_stParam.setClassId(stDataInfo.stClassInfo.nId);

    return enRetCode;
}

/* 获取文件后缀 */
std::string AiManage_NS::CAiManage::getFileExtension(
    const std::string& strFilePath)
{
    std::size_t nDotPos = strFilePath.find_last_of('.');
    if (nDotPos != std::string::npos && nDotPos != strFilePath.length() - 1)
    {
        return strFilePath.substr(nDotPos);
    }
    /* 返回空字符串表示没有后缀 */
    return "";
}

/* 设置学生人数统计的参数 */
BlError_E AiManage_NS::CAiManage::set_countStudentsParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pCountStudentsTimer, &m_pCountStudentsTimeKeep, stParam, countStudents_taskFun);
#else
    set_timerParam(&m_pCountStudentsTimeKeep, stParam, countStudents_taskFun);
#endif
    m_stParam.stCountStudents = stParam;

    return OK;
}

/* 设置教师接打电话识别的参数 */
BlError_E AiManage_NS::CAiManage::set_teTelephoneCallsParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pTeTelephoneCallsTimer, &m_pTeTelephoneCallsTimeKeep, stParam, teTelephoneCalls_taskFun);
#else
    set_timerParam(&m_pTeTelephoneCallsTimeKeep, stParam, teTelephoneCalls_taskFun);
#endif
    m_stParam.stTeTelephoneCalls = stParam;

    return OK;
}

/* 设置学生玩手机识别的参数 */
BlError_E AiManage_NS::CAiManage::set_stPlayPhoneParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pStPlayPhoneTimer, &m_pStuPlayPhoneTimeKeep, stParam, stPlayPhone_taskFun);
#else
    set_timerParam(&m_pStuPlayPhoneTimeKeep, stParam, stPlayPhone_taskFun);
#endif

    return OK;
}

/* 设置学生课堂纪律的参数 */
BlError_E AiManage_NS::CAiManage::set_stDisciplineParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pStDisciplineTimer, &m_pStuDisciplineTimeKeep, stParam, stDiscipline_taskFun);
#else
    set_timerParam(&m_pStuDisciplineTimeKeep, stParam, stDiscipline_taskFun);
#endif

    return OK;
}

/* 设置学生行为分析的参数 */
BlError_E AiManage_NS::CAiManage::set_studentBehaviorParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pStudentBehaviorTimer, &m_pStudentBehaviorTimeKeep, stParam, studentBehavior_taskFun);
#else
    set_timerParam(&m_pStudentBehaviorTimeKeep, stParam, studentBehavior_taskFun);
#endif

    m_stParam.stStudentBehavior = stParam;

    return OK;
}

/* 设置学生行为分析的额外参数 */
BlError_E AiManage_NS::CAiManage::set_studentBehaviorExParam(AiScenario_NS::BehaviorParam_S stExParam)
{
    m_stParam.stBehaviorExParam = stExParam;
    return OK;
}

/* 设置表情识别的参数 */
BlError_E AiManage_NS::CAiManage::set_emoDetecrParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pEmoDetecrTimer, &m_pEmoDetecrTimeKeep, stParam, emoDetecr_taskFun);
#else
    set_timerParam(&m_pEmoDetecrTimeKeep, stParam, emoDetecr_taskFun);
#endif

    m_stParam.stEmoDetecr = stParam;

    return OK;
}

/* 设置人脸识别的参数 */
BlError_E AiManage_NS::CAiManage::set_teFaceDetecrParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pTeFaceDetecrTimer, &m_pTeFaceDetecrTimeKeep, stParam, teFaceDetecr_taskFun);
#else
    set_timerParam(&m_pTeFaceDetecrTimeKeep, stParam, teFaceDetecr_taskFun);
#endif

    m_stParam.stTeFaceDetecr = stParam;

    return OK;
}

/* 设置学生人脸识别的参数 */
BlError_E AiManage_NS::CAiManage::set_stFaceDetecrParam(AlgorithmParam_S stParam)
{
    if (stParam.bOpen)
    {
        if (stParam.nIntervals < 1000)
        {
            /* 最低1分钟一次 */
            stParam.nIntervals = 60 * 1000;
        }

        /* 使用秒级计时器 */
        if (m_pStFaceDetecrTimeKeep)
        {
            /* 重新计时 */
            m_pStFaceDetecrTimeKeep->stNeedParam.nTimeMs = stParam.nIntervals;
            TImeKeep_resume(m_pStFaceDetecrTimeKeep);
        }
        else
        {
            /* 创建计时器 */
            TimeKeepNeedParam_S stNeedParam;
            stNeedParam.nTimeMs     = stParam.nIntervals;
            m_pStFaceDetecrTimeKeep = TimeKeep_alloc(stNeedParam);
            if (m_pStFaceDetecrTimeKeep)
            {
                m_pStFaceDetecrTimeKeep->stExParam.pUserFun = stFaceDetecr_taskFun;
                m_pStFaceDetecrTimeKeep->stExParam.pParam   = this;
                TimeKeep_init(m_pStFaceDetecrTimeKeep);
                TImeKeep_start(m_pStFaceDetecrTimeKeep);
            }
        }
    }
    else
    {
        if (m_pStFaceDetecrTimeKeep)
        {
            TimeKeep_uninit(m_pStFaceDetecrTimeKeep);
            TimeKeep_release(m_pStFaceDetecrTimeKeep);
            m_pStFaceDetecrTimeKeep = NULL;
        }
    }

    m_stParam.stStFaceDetecr = stParam;

    return OK;
}

/* 设置老师轨迹检测的参数 */
BlError_E AiManage_NS::CAiManage::set_trackTeacherParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pTrackTeacherTimer, &m_pTrackTeacherTimeKeep, stParam, trackTeacher_taskFun);
#else
    set_timerParam(&m_pTrackTeacherTimeKeep, stParam, trackTeacher_taskFun);
#endif

    m_stParam.stTrackTeacher = stParam;

    return OK;
}

/* 设置板书检测的参数 */
BlError_E AiManage_NS::CAiManage::set_boardDetecrParam(AlgorithmParam_S stParam)
{

#ifdef USE_SEM_TIMER
    set_timerParam(&m_pBoardDetecrTimer, &m_pBoardDetecrTimeKeep, stParam, boardDetecr_taskFun);
#else
    set_timerParam(&m_pBoardDetecrTimeKeep, stParam, boardDetecr_taskFun);
#endif

    m_stParam.stBoardDetecr = stParam;

    return OK;
}

/* 结束AI分析 */
BlError_E AiManage_NS::CAiManage::end_aiAnalysis(const void* pParam)
{
    if (m_pResultsBase)
    {
        m_pResultsBase->end_aiAnalysis(pParam);
        return OK;
    }
    return NOK;
}

/* 结束教育云平台AI分析 */
BlError_E AiManage_NS::CAiManage::end_platformAiAnalysis(const void* pParam)
{
    if (m_pResultsBase)
    {
        m_pResultsBase->end_platformAiAnalysis(pParam);
        return OK;
    }
    return NOK;
}

/* 结束教育云平台AI分析 */
BlError_E AiManage_NS::CAiManage::processClassExit(const void* pParam)
{
    if (m_pResultsBase)
    {
        m_pResultsBase->processClassExit(pParam);
        return OK;
    }
    return NOK;
}

/* 切换老师画面 */
BlError_E AiManage_NS::CAiManage::switchTeacherScreen()
{
    dlog(LOG_ERROR, "切换老师画面");
    m_bStudentScreen.store(false);

    if (m_pResultsBase)
    {
        m_pResultsBase->switchTeacherScreen();
        return OK;
    }
    return NOK;
}

/* 切换学生画面 */
BlError_E AiManage_NS::CAiManage::switchStudentScreen()
{
    dlog(LOG_ERROR, "切换学生画面");
    m_bStudentScreen.store(true);

    if (m_pResultsBase)
    {
        m_pResultsBase->switchStudentScreen();
        return OK;
    }
    return NOK;
}

/* 切换学生特写 */
BlError_E AiManage_NS::CAiManage::switchStudentCloseUp(bool bValue)
{
    m_bStudentCloseUp.store(bValue);

    if (m_pResultsBase)
    {
        m_pResultsBase->switchStudentCloseUp(bValue);
    }
    return OK;
}

/* 切换老师是否在讲台 */
BlError_E AiManage_NS::CAiManage::switchTeacherPodium(bool bValue)
{
    m_bTeacherPodium.store(bValue);
    dlog(LOG_ERROR, "切换老师是否在讲台[%d]", bValue);

    if (m_pResultsBase)
    {
        m_pResultsBase->switchTeacherPodium(bValue);
    }
    return OK;
}

/* 开始互动行为 */
BlError_E AiManage_NS::CAiManage::startInteraction()
{
    if (m_pResultsBase)
    {
        m_pResultsBase->startInteraction();
        return OK;
    }
    return NOK;
}

/* 开始巡视行为 */
BlError_E AiManage_NS::CAiManage::startTour()
{
    if (m_pResultsBase)
    {
        m_pResultsBase->startTour();
        return OK;
    }
    return NOK;
}

/* 开始教授行为 */
BlError_E AiManage_NS::CAiManage::startTaught()
{
    if (m_pResultsBase)
    {
        m_pResultsBase->startTaught();
        return OK;
    }
    return NOK;
}

/* 教师板书 -- 摄像机*/
BlError_E AiManage_NS::CAiManage::teacherBoard(bool bValue)
{
    if (m_pResultsBase)
    {
        m_pResultsBase->teacherBoard(bValue);
        /* 督导巡课 */
        if (nullptr != m_stParam.isPlatformAiTime)
        {
            if (m_stParam.isPlatformAiTime())
            {
                m_pResultsBase->teacherPlatformBoard(bValue);
            }
        }

        return OK;
    }
    return NOK;
}

/* PPT切换 */
BlError_E AiManage_NS::CAiManage::pptSwitch()
{
    if (m_pResultsBase)
    {
        m_pResultsBase->pptSwitch();
        return OK;
    }
    return NOK;
}

/* 语音转写热词提取 */
BlError_E AiManage_NS::CAiManage::addAudioWordsResult(std::list<std::pair<std::string, int>> listWords)
{
    if (m_pResultsBase)
    {
        m_pResultsBase->addAudioWordsResult(listWords);
        return OK;
    }
    return NOK;
}

/* 设置本地老师人脸识别-结果 */
BlError_E AiManage_NS::CAiManage::setLocalTeFaceRecResult(std::list<std::pair<int, std::string>> listName)
{
    if (m_pResultsBase)
    {
        m_pResultsBase->setLocalTeFaceRecResult(listName);
        return OK;
    }
    return NOK;
}

/* 设置AI处理状态 */
BlError_E AiManage_NS::CAiManage::setAiProcessingStatus(int nStatus)
{
    if (nStatus == 0) /* 开始 */
    {
        if (nullptr == m_pCurRecordTimeKeep1s)
        {
            /* 创建计时器 */
            TimeKeepNeedParam_S stNeedParam;
            stNeedParam.nTimeMs    = 1 * 1000;
            m_pCurRecordTimeKeep1s = TimeKeep_alloc(stNeedParam);
            if (m_pCurRecordTimeKeep1s)
            {
                m_pCurRecordTimeKeep1s->stExParam.pUserFun = timer1s_taskFun;
                m_pCurRecordTimeKeep1s->stExParam.pParam   = this;
                TimeKeep_init(m_pCurRecordTimeKeep1s);
                TImeKeep_start(m_pCurRecordTimeKeep1s);
            }
        }
        else
        {
            TImeKeep_resume(m_pCurRecordTimeKeep1s);
        }

        /* 清空上一次考勤图片 */
        char achCmd[1024] = { 0 };
        snprintf(achCmd, sizeof(achCmd), "rm %s/* ; rm %s/*",
                 GANCIAN_PICTURE_TEMP_PATH,
                 KEY_SNAPS_TEMP_PATH);
        system(achCmd);

        /* 开始录制 */
        m_nCurRecordTime.store(0);
        /* 判断老师是否在讲台 */
        if (m_bTeacherPodium.load())
        {
            startTaught();
        }

        /* 开始一个教师考勤小循环定时器 */
        if (m_pTeFaceDetecrTimeKeep5s)
        {
            TImeKeep_resume(m_pTeFaceDetecrTimeKeep5s);
        }
        /* 停止教师考勤大循环定时器 */
        if (m_pTeFaceDetecrTimeKeep)
        {
            TimeKeep_stop(m_pTeFaceDetecrTimeKeep);
        }

        /* 开始一个学生考勤小循环定时器 */
        if (m_pStFaceDetecrTimeKeep5s)
        {
            TImeKeep_resume(m_pStFaceDetecrTimeKeep5s);
        }
        /* 停止考勤学生大循环定时器 */
        if (m_pStFaceDetecrTimeKeep)
        {
            TimeKeep_stop(m_pStFaceDetecrTimeKeep);
        }
    }
    else if (nStatus == 1) /* 暂停 */
    {
        if (m_pCurRecordTimeKeep1s)
        {
            TimeKeep_pause(m_pCurRecordTimeKeep1s);
        }
    }
    else if (nStatus == 2) /* 恢复 */
    {
        if (nullptr == m_pCurRecordTimeKeep1s)
        {
            /* 创建计时器 */
            TimeKeepNeedParam_S stNeedParam;
            stNeedParam.nTimeMs    = 1 * 1000;
            m_pCurRecordTimeKeep1s = TimeKeep_alloc(stNeedParam);
            if (m_pCurRecordTimeKeep1s)
            {
                m_pCurRecordTimeKeep1s->stExParam.pUserFun = timer1s_taskFun;
                m_pCurRecordTimeKeep1s->stExParam.pParam   = this;
                TimeKeep_init(m_pCurRecordTimeKeep1s);
                TImeKeep_start(m_pCurRecordTimeKeep1s);
            }
        }
        else
        {
            TImeKeep_start(m_pCurRecordTimeKeep1s);
            /* 同步录制时间 */
            if (m_stParam.getRecordTime)
            {
                int nRecordTime = m_stParam.getRecordTime();
                if (std::abs(m_nCurRecordTime.load() - nRecordTime) > 5)
                {
                    m_nCurRecordTime.store(nRecordTime);
                }
            }
        }
    }
    else /* 停止 */
    {
        if (m_pCurRecordTimeKeep1s)
        {
            TimeKeep_stop(m_pCurRecordTimeKeep1s);
        }
        m_nCurRecordTime.store(0);

#ifdef USE_LOCAL_AI
        if (m_pAiLocal)
        {
            m_pAiLocal->clearData();
        }
#endif
    }
    return OK;
}

/* 处理人数统计分析数据 */
BlError_E AiManage_NS::CAiManage::deal_countStudents(char* pchJson)
{
    if (nullptr == pchJson ||
        nullptr == m_pParseBase ||
        nullptr == m_pResultsBase)
    {
        return NOK;
    }

    BlError_E enRetCode = OK;

    AiManage_NS::NumberInfo_S stResultsInfo;
    enRetCode = m_pParseBase->parse(pchJson, stResultsInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "解析人数统计数据失败:\n[%s]", pchJson);
        return NOK;
    }

    /* 成功，处理 */
    m_pResultsBase->deal_countStudents(stResultsInfo);

    return OK;
}

/* 处理课堂纪律数据 */
BlError_E AiManage_NS::CAiManage::deal_discipline(char* pchJson)
{
    if (nullptr == pchJson ||
        nullptr == m_pParseBase ||
        nullptr == m_pResultsBase)
    {
        return NOK;
    }

    BlError_E enRetCode = OK;

    AiManage_NS::MoveProbability_S stMoveProbability;
    enRetCode = m_pParseBase->parse(pchJson, stMoveProbability);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "解析课堂纪律数据失败:\n[%s]", pchJson);
        return NOK;
    }

    /* 成功，处理 */
    m_pResultsBase->deal_discipline(stMoveProbability);

    return OK;
}

/* 处理学生行为分析数据 */
BlError_E AiManage_NS::CAiManage::deal_studentBehavior(char* pchJson)
{
    if (nullptr == pchJson ||
        nullptr == m_pParseBase ||
        nullptr == m_pResultsBase)
    {
        return NOK;
    }

    BlError_E enRetCode = OK;

    AiManage_NS::BehaviorInfo_S stResultsInfo;
    enRetCode = m_pParseBase->parse(pchJson, stResultsInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "解析学生行为数据失败:\n[%s]", pchJson);
        return NOK;
    }

    /* 成功，处理 */
    m_pResultsBase->deal_studentBehavior(stResultsInfo);

    return OK;
}

/* 设置平台IP地址 */
BlError_E AiManage_NS::CAiManage::set_platformIp(std::string strIp)
{
    if (m_pPlatformManage)
    {
        m_pPlatformManage->set_platformIp((char*)strIp.c_str());
    }
    return OK;
}

/* 获取AI服务器连接状态 */
int AiManage_NS::CAiManage::get_aiServerStatus()
{
    return m_nAiServerStatus.load();
}

/* 更新stream获取回来的数据 */
int AiManage_NS::CAiManage::updateStreamData(char* pchData, int nLength, int nCode)
{
    if (pchData == nullptr)
    {
        dlog(LOG_ERROR, "更新stream获取回来的数据失败 --- 数据为空");
        return -1;
    }
    ProcessMode_E enMode = LOCAL_MODE;
    bool          bLocal = true;

    switch (nCode)
    {
        /* 算法调用 */
        case AiManage_NS::AI_COM_BOARD:
        {
            enMode = m_stParam.stBoardDetecr.enMode;
            break;
        }
        case AiManage_NS::AI_COM_EMO:
        {
            enMode = m_stParam.stEmoDetecr.enMode;
            break;
        }
        case AiManage_NS::AI_COM_ST_FACE:
        case AiManage_NS::AI_COM_STAS_FACE:
        {
            enMode = m_stParam.stStFaceDetecr.enMode;
            break;
        }
        case AiManage_NS::AI_COM_TE_FACE:
        {
            enMode = m_stParam.stTeFaceDetecr.enMode;
            break;
        }
        case AiManage_NS::AI_COM_HEAD:
        {
            enMode = m_stParam.stTrackTeacher.enMode;
            break;
        }
        case AiManage_NS::AI_COM_NUM_COUNTER:
        {
            enMode = m_stParam.stCountStudents.enMode;
            if (enMode == SERVER_MODE)
            {
                /* 判断是否连接上服务器 */
                if (m_nAiServerStatus.load() == 0)
                {
                    enMode = LOCAL_MODE;
                }
            }
            break;
        }
        case AiManage_NS::AI_COM_ST_BEHAVIOR:
        {
            enMode = m_stParam.stStudentBehavior.enMode;
            if (enMode == SERVER_MODE)
            {
                /* 判断是否连接上服务器 */
                if (m_nAiServerStatus.load() == 0)
                {
                    enMode = LOCAL_MODE;
                }
            }
            break;
        }
        case AiManage_NS::AI_COM_TEA_CALLPHONE:
        {
            enMode = m_stParam.stTeTelephoneCalls.enMode;
            break;
        }
        case AiManage_NS::AI_COM_ST_PLAYPHONE:
        {
            enMode = AiManage_NS::SERVER_MODE;
            break;
        }
        case AiManage_NS::AI_COM_ST_DISCIPLINE:
        {
            enMode = AiManage_NS::LOCAL_MODE;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "[与stream通讯的服务器] 未定义命令处理[%d]", nCode);
            return NOK;
        }
    }

    dlog(LOG_TRACE, "接收到stream的JPEG数据返回长度[%d],命令码[%d],处理类型[%d]",
         nLength,
         nCode,
         enMode);
    sendAi_analyseData(pchData, nLength, nCode, enMode);
    return 0;
}

/* 发送AI通讯数据-分析数据 */
BlError_E AiManage_NS::CAiManage::sendAi_analyseData(
    char*         pchData,
    int           nDataLen,
    int           nCode,
    ProcessMode_E enMode)
{

    if (nullptr == m_stParam.getClassId)
    {
        dlog(LOG_ERROR, "获取班级ID函数指针未设置");
        return ERR_UNINIT;
    }

    if (nullptr == m_stParam.getRecordTime)
    {
        dlog(LOG_ERROR, "获取当前录制时长函数指针未设置");
        return ERR_UNINIT;
    }

    BlError_E enRetCode = NOK;

    /* 通知stream，获取画面数据 */
    COMM_NS::SendDataInfo_S stSendInfo;

    int nDataSize = sizeof(CommDataInfo_S);
    int nExSize   = 0;

    if (nCode == AI_COM_ST_BEHAVIOR)
    {
        nExSize = sizeof(AiScenario_NS::BehaviorParam_S);
    }

    if (nCode == AI_COM_TEA_CALLPHONE || nCode == AI_COM_ST_PLAYPHONE || nCode == AI_COM_BOARD)
    {
        nExSize = sizeof(AiScenario_NS::SupClsInsParam_S);
    }

    CommDataInfo_S* pstDataInfo = (CommDataInfo_S*)new char[nDataSize + nExSize + nDataLen];

    if (pstDataInfo)
    {
        time_t current_time;
        current_time = time(NULL);

        if (current_time != -1)
        {
            if (nCode == AI_COM_ST_BEHAVIOR || nCode == AI_COM_TEA_CALLPHONE || nCode == AI_COM_ST_PLAYPHONE || nCode == AI_COM_BOARD)
            {
                if (nCode == AI_COM_ST_BEHAVIOR)
                {
                    memcpy(pstDataInfo->data, &m_stParam.stBehaviorExParam, nExSize);
                    memcpy(pstDataInfo->data + nExSize, pchData, nDataLen);
                }

                if (nCode == AI_COM_TEA_CALLPHONE || nCode == AI_COM_ST_PLAYPHONE || nCode == AI_COM_BOARD)
                {
                    AiScenario_NS::SupClsInsParam_S stSupClsInsParam;
                    stSupClsInsParam.stPlayPhone.dKeyScore = m_pResultsBase->m_stAiConfidenceTh.fStuPhoneCofid;
                    stSupClsInsParam.stPlayPhone.dRatio    = m_pResultsBase->m_stAiConfidenceTh.fRatio;
                    stSupClsInsParam.stCallPhone.dKeyScore = m_pResultsBase->m_stAiConfidenceTh.fTePhoneCofid;
                    stSupClsInsParam.stTeaBoard.dKeyScore  = m_pResultsBase->m_stAiConfidenceTh.fTeBoardCofid;
                    memcpy(pstDataInfo->data, &stSupClsInsParam, nExSize);
                    memcpy(pstDataInfo->data + nExSize, pchData, nDataLen);
                }
            }
            else
            {
                memcpy(pstDataInfo->data, pchData, nDataLen);
            }
            pstDataInfo->nCode          = nCode;
            pstDataInfo->nClassId       = m_stParam.getClassId();
            pstDataInfo->nCurRecordTime = m_nCurRecordTime.load();
            pstDataInfo->lTimestamp     = current_time;
            pstDataInfo->nDataSize      = nDataLen;
            pstDataInfo->nExSize        = nExSize;
            pstDataInfo->enCodec        = JPEG;

            if (nCode == AI_COM_ST_FACE ||
                nCode == AI_COM_STAS_FACE)
            {
                /* 保存截图文件文件 */
                /* 创建目录 */
                CPublicFunc::makeDirectory(GANCIAN_PICTURE_TEMP_PATH);

                /* 保存 */
                char achFilePath[1024] = { 0 };
                snprintf(achFilePath, sizeof(achFilePath), "%s/st_%lld.jpg", GANCIAN_PICTURE_TEMP_PATH, pstDataInfo->lTimestamp);
                CPublicFunc::writeDataToFile(achFilePath, pchData, nDataLen);
            }
            else if (nCode == AI_COM_TE_FACE)
            {
                /* 保存截图文件文件 */
                /* 创建目录 */
                CPublicFunc::makeDirectory(GANCIAN_PICTURE_TEMP_PATH);

                /* 保存 */
                char achFilePath[1024] = { 0 };
                snprintf(achFilePath, sizeof(achFilePath), "%s/te_%lld.jpg", GANCIAN_PICTURE_TEMP_PATH, pstDataInfo->lTimestamp);
                CPublicFunc::writeDataToFile(achFilePath, pchData, nDataLen);
            }

            if (enMode == LOCAL_MODE)
            {
#ifdef USE_LOCAL_AI
                if (m_pAiLocal)
                {
                    /* 不能释放pstDataInfo */
                    if (nCode == AI_COM_NUM_COUNTER)
                    {
                        enRetCode = m_pAiLocal->analyseCountStudent(pstDataInfo);
                        return enRetCode;
                    }
                    else if (nCode == AI_COM_ST_BEHAVIOR)
                    {
                        enRetCode = m_pAiLocal->analyseStudentBehavior(pstDataInfo);
                        return enRetCode;
                    }
                    else if (nCode == AI_COM_ST_DISCIPLINE)
                    {
                        enRetCode = m_pAiLocal->analyseDiscipline(pstDataInfo);
                        return enRetCode;
                    }
                }
#endif
            }
            else if (enMode == SERVER_MODE)
            {
                if (m_pAiServerComm)
                {
                    stSendInfo.nCode     = nCode;
                    stSendInfo.nDataSize = pstDataInfo->size();
                    stSendInfo.pDate     = reinterpret_cast<char*>(pstDataInfo);

                    enRetCode = m_pAiServerComm->send(stSendInfo);
                }
            }
        }
    }

    if (pstDataInfo)
    {
        delete[] pstDataInfo;
        pstDataInfo = nullptr;
    }

    return enRetCode;
}

/* 使能语音转写-为了关闭某些功能，规避性能不够的问题 */
BlError_E AiManage_NS::CAiManage::set_transcription(bool bEnabled)
{
    m_bTtranscription.store(bEnabled);
#ifdef USE_LOCAL_AI
    dlog(LOG_INFO, "使能语音转写-为了关闭某些功能，规避性能不够的问题, 1-关闭ai功能 0-开启ai功能 [%d]", bEnabled);
    if (bEnabled)
    {
        if (m_pAiLocal)
        {
            m_pAiLocal->uninitBehavior();
            m_pAiLocal->uninitHeadCount();
        }
    }
    else
    {
        if (m_pAiLocal)
        {
            m_pAiLocal->initBehavior();
            m_pAiLocal->initHeadCount();
        }
    }
#endif
    return OK;
}

AiManage_NS::CAiManage::CAiManage()
{
    m_stParam.clear();

    m_nCurRecordTime.store(0);
    m_bStudentCloseUp.store(false);
    m_bStudentScreen.store(false);
    /* 程序起来，默认老师在讲台 */
    m_bTeacherPodium.store(true);

    /* 创建计时器 */
    TimeKeepNeedParam_S stNeedParam;
    stNeedParam.nTimeMs       = 5 * 1000;
    m_pStFaceDetecrTimeKeep5s = TimeKeep_alloc(stNeedParam);
    if (m_pStFaceDetecrTimeKeep5s)
    {
        m_pStFaceDetecrTimeKeep5s->stExParam.pUserFun = stFaceDetecr_taskFun;
        m_pStFaceDetecrTimeKeep5s->stExParam.pParam   = this;
        TimeKeep_init(m_pStFaceDetecrTimeKeep5s);
        TimeKeep_pause(m_pStFaceDetecrTimeKeep5s);
    }

    m_pTeFaceDetecrTimeKeep5s = TimeKeep_alloc(stNeedParam);
    if (m_pTeFaceDetecrTimeKeep5s)
    {
        m_pTeFaceDetecrTimeKeep5s->stExParam.pUserFun = teFaceDetecr_taskFun;
        m_pTeFaceDetecrTimeKeep5s->stExParam.pParam   = this;
        TimeKeep_init(m_pTeFaceDetecrTimeKeep5s);
        TimeKeep_pause(m_pTeFaceDetecrTimeKeep5s);
    }

#ifdef USE_LOCAL_AI
    /* 初始化本地分析 */
    m_pAiLocal = new AiLocal_NS::CAiLocal(
        std::bind(&CAiManage::deal_countStudents, this, std::placeholders::_1),
        std::bind(&CAiManage::deal_studentBehavior, this, std::placeholders::_1),
        std::bind(&CAiManage::deal_discipline, this, std::placeholders::_1));

    if (m_pAiLocal)
    {
        m_pAiLocal->initDiscipline();
    }
#endif
}

AiManage_NS::CAiManage::~CAiManage()
{
    if (m_pAiServerComm)
    {
        COMM_NS::CCommModule::releaseComm(m_pAiServerComm);
    }

    if (m_pLocalServerComm)
    {
        COMM_NS::CCommModule::releaseComm(m_pLocalServerComm);
    }

    if (m_pResultsBase)
    {
        ResultsModule_NS::CResultsModule::releaseModule(m_pResultsBase);
    }

    if (m_pPlatformManage)
    {
        delete m_pPlatformManage;
        m_pPlatformManage = nullptr;
    }

#ifdef USE_LOCAL_AI
    if (m_pAiLocal)
    {
        delete m_pAiLocal;
        m_pAiLocal = nullptr;
    }
#endif
}

/* 初始化平台管理 */
BlError_E AiManage_NS::CAiManage::initPlatForm()
{
    std::string strPlatformIp;
    if (m_stParam.getPlatformIp)
    {
        strPlatformIp = m_stParam.getPlatformIp();
    }

    PlatformManage_NS::AiPlatformInParam_S stPlatformInParam;

    int nCpySize = sizeof(stPlatformInParam.stNeedParam.achIpAddr);
    strncpy(stPlatformInParam.stNeedParam.achIpAddr,
            strPlatformIp.c_str(),
            nCpySize - 1);
    /* 确保以空字符终止 */
    stPlatformInParam.stNeedParam.achIpAddr[nCpySize - 1] = '\0';

    nCpySize = sizeof(stPlatformInParam.stNeedParam.achDownloadPath);
    strncpy(stPlatformInParam.stNeedParam.achDownloadPath,
            m_stParam.strDownloadPath.c_str(),
            nCpySize - 1);
    /* 确保以空字符终止 */
    stPlatformInParam.stNeedParam.achDownloadPath[nCpySize - 1] = '\0';

    stPlatformInParam.stNeedParam.enType             = PlatformManage_NS::AiPlatformType_E::ITC_PLATFORM;
    stPlatformInParam.stNeedParam.aiPlatformCallback = std::bind(&CAiManage::classInfoCallback,
                                                                 this,
                                                                 std::placeholders::_1,
                                                                 std::placeholders::_2,
                                                                 std::placeholders::_3);
    stPlatformInParam.stNeedParam.pHandle            = this;

    m_pPlatformManage = PlatformManage_NS::CAiPlatformFactory::createClient(stPlatformInParam);

    if (m_pPlatformManage)
    {
        m_pPlatformManage->start_thread();
    }

    return OK;
}

/* 初始化与AI服务器的连接 */
BlError_E AiManage_NS::CAiManage::initComm_aiServer()
{

    if (m_pAiServerComm)
    {
        dlog(LOG_ERROR, "AI服务器通讯客户端-已初始化");
        return ERR_INI_ERR;
    }

    if (nullptr == m_stParam.getAiServerIp)
    {
        dlog(LOG_ERROR, "AI服务器通讯客户端-获取AI服务器IP函数指针未设置");
        return ERR_INI_ERR;
    }

    COMM_NS::CommInParam_S stInfo;
    stInfo.clear();
    stInfo.stNeedParam.enType           = COMM_NS::COMM_SHARE_TCP;
    stInfo.stNeedParam.strIP            = m_stParam.getAiServerIp();
    stInfo.stNeedParam.nPort            = m_stParam.nAiClientPort;
    stInfo.stNeedParam.bServerMode      = false;
    stInfo.stExParam.bAutoReconnect     = true;
    stInfo.stExParam.nReconnectCount    = 0;
    stInfo.stExParam.nReconnectInterval = 1000;

    stInfo.stExParam.dataCallback   = std::bind(&CAiManage::aiServer_dataCallback, this, std::placeholders::_1);
    stInfo.stExParam.statusCallback = std::bind(&CAiManage::aiServer_statusCallback, this, std::placeholders::_1);

    m_pAiServerComm = COMM_NS::CCommModule::createComm(stInfo);
    if (m_pAiServerComm)
    {
        dlog(LOG_INFO, "AI服务器通讯客户端初始化成功");
    }

    return OK;
}

/* 初始化本机服务器 */
BlError_E AiManage_NS::CAiManage::initComm_localServer()
{
    if (m_pLocalServerComm)
    {
        dlog(LOG_ERROR, "ai逻辑的本地服务器-已初始化");
        return ERR_INI_ERR;
    }

    COMM_NS::CommInParam_S stInfo;
    stInfo.clear();
    stInfo.stNeedParam.enType      = COMM_NS::COMM_SHARE_TCP;
    stInfo.stNeedParam.strIP       = "127.0.0.1";
    stInfo.stNeedParam.nPort       = m_stParam.nAiServerPort;
    stInfo.stNeedParam.bServerMode = true;

    stInfo.stExParam.bAutoReconnect     = true;
    stInfo.stExParam.nReconnectCount    = 0;
    stInfo.stExParam.nReconnectInterval = 1000;

    stInfo.stExParam.dataCallback = std::bind(&CAiManage::localServer_dataCallback, this, std::placeholders::_1);

    m_pLocalServerComm = COMM_NS::CCommModule::createComm(stInfo);

    return OK;
}

/* 初始化处理结果类 */
BlError_E AiManage_NS::CAiManage::init_results()
{
    if (m_pResultsBase)
    {
        dlog(LOG_ERROR, "处理结果类-已初始化");
        return ERR_INI_ERR;
    }

    ResultsModule_NS::InParam_S stInfo;
    stInfo.stNeedParam.enType                = ResultsModule_NS::SAVE_JSON;
    stInfo.stNeedParam.getRecordState        = m_stParam.getRecordState;
    stInfo.stNeedParam.getRecordTime         = std::bind(&CAiManage::getRecordTimeFunc, this);
    stInfo.stNeedParam.isTeacherPodium       = std::bind(&CAiManage::getTeacherPodium, this);
    stInfo.stNeedParam.isStudentCloseUp      = std::bind(&CAiManage::getStudentCloseUp, this);
    stInfo.stNeedParam.sendStuPanoSS         = m_stParam.sendStuPanoSS;
    stInfo.stNeedParam.sendStuSpecSS         = m_stParam.sendStuSpecSS;
    stInfo.stNeedParam.sendStreamGetData     = m_stParam.notifyGetSteamData;
    stInfo.stNeedParam.isLocalMode           = std::bind(&CAiManage::isLocalMode, this);
    stInfo.stNeedParam.platformTeaEvent      = m_stParam.platformTeaEvent;
    stInfo.stNeedParam.platformBehaviorEvent = m_stParam.platformBehaviorEvent;
    stInfo.stNeedParam.platformAtStuAlert    = m_stParam.platformAtStuAlert;
    stInfo.stNeedParam.platformSdStuAlert    = m_stParam.platformSdStuAlert;
    stInfo.stNeedParam.platformPushAlertType = m_stParam.platformPushAlertType;
    stInfo.stNeedParam.platformPushStatus    = m_stParam.platformPushStatus;
    stInfo.stNeedParam.isPlatformClassTime   = m_stParam.isPlatformClassTime;
    stInfo.stNeedParam.getClassTime          = m_stParam.getClassTime;
    stInfo.stNeedParam.pfPushEmoticonType    = m_stParam.pfPushEmoticonType;
    stInfo.stNeedParam.isPlatformAiTime      = m_stParam.isPlatformAiTime;
    stInfo.stNeedParam.getPlatformSwitch     = m_stParam.getPlatformSwitch;

    m_pResultsBase = ResultsModule_NS::CResultsModule::createModule(stInfo);

    return OK;
}

/* 初始化解析类 */
BlError_E AiManage_NS::CAiManage::init_parse()
{
    if (m_pParseBase)
    {
        dlog(LOG_ERROR, "解析类-已初始化");
        return ERR_INI_ERR;
    }


    ParseData_NS::InParam_S stInfo;
    stInfo.stNeedParam.enType = ParseData_NS::PARSE_JSON;

    m_pParseBase = ParseData_NS::CParseData::create(stInfo);

    return OK;
}

/* 获取设备信息 */
BlError_E AiManage_NS::CAiManage::get_device_info(DevInfo_S& stDevInfo)
{
    int  nRet       = 0;
    char achIp[32]  = { 0 };
    char achMac[64] = { 0 };
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "获取设备型号失败");
        return NOK;
    }
    nRet = ReachGetIPaddrstring(ETH0_INTERFACE, achIp);
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "获取设备Ip失败");
        return NOK;
    }
    nRet = get_mac(achMac, sizeof(achMac));
    if (nRet < 0)
    {
        dlog(LOG_ERROR, "获取设备Mac失败");
        return NOK;
    }
    stDevInfo.strDevModel = m_stParam.strDevType;
    stDevInfo.strDevIp    = achIp;
    stDevInfo.strDevMac   = achMac;

    return OK;
}

/* 发送AI通讯数据 */
BlError_E AiManage_NS::CAiManage::sendAi_Data(std::string strMsg, int nCode)
{
    if (m_pAiServerComm == nullptr)
    {
        return NOK;
    }
    COMM_NS::SendDataInfo_S stSendInfo;
    stSendInfo.nDataSize = strMsg.size();
    stSendInfo.pDate     = const_cast<char*>(strMsg.data());
    stSendInfo.nCode     = nCode;
    dlog(LOG_INFO, "[Control --> AiServer] ========= %s", stSendInfo.pDate);
    m_pAiServerComm->send(stSendInfo);

    return OK;
}

#ifdef USE_SEM_TIMER

/* 设置定时器/计时器参数 */
BlError_E AiManage_NS::CAiManage::set_timerParam(
    Timer_S**        pTimer,
    TimeKeep_S**     pTimeKeep,
    AlgorithmParam_S stParam,
    int (*pUserFun)(void* pParam))
{
    if (nullptr == pTimer || nullptr == pTimeKeep || nullptr == pUserFun)
    {
        return ERR_IN_PARAM_NULL;
    }

    if (stParam.bOpen)
    {
        if (stParam.nIntervals < 1000)
        {
            /* 使用毫秒级定时器 */
            int nFrameRate = 1.0 / (stParam.nIntervals / 1000.0);
            if (*pTimer)
            {
                /* 重设定时器速度 */
                Timer_set_Rate(*pTimer, nFrameRate);
            }
            else
            {
                /* 创建定时器 */
                TimerNeedParam_S stNeedParam;
                stNeedParam.nFrameRate = nFrameRate;
                stNeedParam.pTaskFun   = pUserFun;
                stNeedParam.pParam     = this;
                *pTimer                = Timer_alloc(stNeedParam);
                if (*pTimer)
                {
                    Timer_init(*pTimer);
                }
            }

            /* 定时器和计时器同时只能使用一个 */
            if (*pTimeKeep)
            {
                TimeKeep_uninit(*pTimeKeep);
                TimeKeep_release(*pTimeKeep);
                *pTimeKeep = NULL;
            }
        }
        else
        {
            /* 使用秒级计时器 */
            if (*pTimeKeep)
            {
                /* 重新计时 */
                (*pTimeKeep)->stNeedParam.nTimeMs = stParam.nIntervals;
                TImeKeep_resume(*pTimeKeep);
            }
            else
            {
                /* 创建计时器 */
                TimeKeepNeedParam_S stNeedParam;
                stNeedParam.nTimeMs = stParam.nIntervals;
                *pTimeKeep          = TimeKeep_alloc(stNeedParam);
                if (*pTimeKeep)
                {
                    (*pTimeKeep)->stExParam.pUserFun = pUserFun;
                    (*pTimeKeep)->stExParam.pParam   = this;
                    TimeKeep_init(*pTimeKeep);
                    TImeKeep_start(*pTimeKeep);
                }
            }

            /* 定时器和计时器同时只能使用一个 */
            if (*pTimer)
            {
                Timer_uninit(*pTimer);
                Timer_release(*pTimer);
                *pTimer = NULL;
            }
        }
    }
    else
    {
        if (*pTimer)
        {
            Timer_uninit(*pTimer);
            Timer_release(*pTimer);
            *pTimer = NULL;
        }

        if (*pTimeKeep)
        {
            TimeKeep_uninit(*pTimeKeep);
            TimeKeep_release(*pTimeKeep);
            *pTimeKeep = NULL;
        }
    }

    return OK;
}

#else

/* 设置定时器/计时器参数 */
BlError_E AiManage_NS::CAiManage::set_timerParam(
    TimeKeep_S**     pTimeKeep,
    AlgorithmParam_S stParam,
    int (*pUserFun)(void* pParam))
{
    if (nullptr == pTimeKeep || nullptr == pUserFun)
    {
        return ERR_IN_PARAM_NULL;
    }

    if (stParam.bOpen)
    {
        if (stParam.nIntervals < 1000)
        {
            stParam.nIntervals = 1000;
        }

        /* 使用秒级计时器 */
        if (*pTimeKeep)
        {
            /* 重新计时 */
            (*pTimeKeep)->stNeedParam.nTimeMs = stParam.nIntervals;
            TImeKeep_resume(*pTimeKeep);
        }
        else
        {
            /* 创建计时器 */
            TimeKeepNeedParam_S stNeedParam;
            stNeedParam.nTimeMs = stParam.nIntervals;
            *pTimeKeep          = TimeKeep_alloc(stNeedParam);
            if (*pTimeKeep)
            {
                (*pTimeKeep)->stExParam.pUserFun = pUserFun;
                (*pTimeKeep)->stExParam.pParam   = this;
                TimeKeep_init(*pTimeKeep);
                TImeKeep_start(*pTimeKeep);
            }
        }
    }
    else
    {
        if (*pTimeKeep)
        {
            TimeKeep_uninit(*pTimeKeep);
            TimeKeep_release(*pTimeKeep);
            *pTimeKeep = NULL;
        }
    }

    return OK;
}
#endif


/* 与stream连接的数据回调函数 */
BlError_E AiManage_NS::CAiManage::send_streamData(const char* pchMsg, int nMsgLength, int nCode)
{

    ProcessMode_E enMode = LOCAL_MODE;
    bool          bLocal = true;

    switch (nCode)
    {
        /* 算法调用 */
        case AI_COM_BOARD:
        {
            enMode = m_stParam.stBoardDetecr.enMode;
            break;
        }
        case AI_COM_EMO:
        {
            enMode = m_stParam.stEmoDetecr.enMode;
            break;
        }
        case AI_COM_ST_FACE:
        case AI_COM_STAS_FACE:
        {
            enMode = m_stParam.stStFaceDetecr.enMode;
            break;
        }
        case AI_COM_TE_FACE:
        {
            enMode = m_stParam.stTeFaceDetecr.enMode;
            break;
        }
        case AI_COM_HEAD:
        {
            enMode = m_stParam.stTrackTeacher.enMode;
            break;
        }
        case AI_COM_NUM_COUNTER:
        {
            enMode = m_stParam.stCountStudents.enMode;
            if (enMode == SERVER_MODE)
            {
                /* 判断是否连接上服务器 */
                if (m_nAiServerStatus.load() == 0)
                {
                    enMode = LOCAL_MODE;
                }
            }
            break;
        }
        case AI_COM_ST_BEHAVIOR:
        {
            enMode = m_stParam.stStudentBehavior.enMode;
            if (enMode == SERVER_MODE)
            {
                /* 判断是否连接上服务器 */
                if (m_nAiServerStatus.load() == 0)
                {
                    enMode = LOCAL_MODE;
                }
            }
            break;
        }
        case AI_COM_TEA_CALLPHONE:
        {
            enMode = m_stParam.stTeTelephoneCalls.enMode;
            break;
        }
        case AI_COM_ST_PLAYPHONE:
        {
            enMode = AiManage_NS::SERVER_MODE;
            break;
        }
        case AI_COM_ST_DISCIPLINE:
        {
            enMode = AiManage_NS::LOCAL_MODE;
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "[与stream通讯的服务器] 未定义命令处理[%d]", nCode);
            return NOK;
        }
    }

    dlog(LOG_TRACE, "接收到stream的JPEG数据返回长度[%d],命令码[%d],处理类型[%d]",
         nMsgLength,
         nCode,
         enMode);
    sendAi_analyseData((char*)pchMsg, nMsgLength, nCode, enMode);

    return OK;
}

void AiManage_NS::CAiManage::close_updateFaceTimer(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_pUpdateFaceTimerKeep)
        {
            TimeKeep_uninit(pThis->m_pUpdateFaceTimerKeep);
            TimeKeep_release(pThis->m_pUpdateFaceTimerKeep);
            pThis->m_pUpdateFaceTimerKeep = nullptr;
        }
    }
}

BlError_E AiManage_NS::CAiManage::aiServer_statusCallback(COMM_NS::StatusParam_S stInfo)
{
    if (stInfo.enType == COMM_NS::StatusType_E::SUCCESS)
    {
        dlog(LOG_TRACE, "[主业务服务器] 客户端连接成功[%p]", stInfo.pHandle);
        m_nAiServerStatus.store(1);
    }
    else
    {
        dlog(LOG_TRACE, "[主业务服务器] 客户端连接失败[%p]", stInfo.pHandle);
        m_nAiServerStatus.store(0);
    }

    if (m_stParam.aiServerStatusChange)
    {
        m_stParam.aiServerStatusChange();
    }
    return OK;
}

BlError_E AiManage_NS::CAiManage::deal_aiServer_heartData(char* pchMsg)
{
    if (pchMsg == nullptr)
    {
        dlog(LOG_ERROR, "[AI心跳] ------- 数据为空");
    }

    // printf("[与aiServer通讯的客户端] 接收到:\n%s\n", pchMsg);
    if (nullptr == m_pParseBase ||
        nullptr == m_pResultsBase)
    {
        return OK;
    }
    BlError_E enRetCode = OK;

    AiManage_NS::VodHeartInfo_S stVodHeartInfo;
    stVodHeartInfo.clear();
    /* 转换 */
    enRetCode = m_pParseBase->parse(pchMsg, stVodHeartInfo);
    if (m_nAiDevCount != stVodHeartInfo.nDevCount && stVodHeartInfo.nDevCount >= 0)
    {
        m_nAiDevCount = stVodHeartInfo.nDevCount;
        /* 学生玩手机识别 */
        AlgorithmParam_S stStuPlayPhone;
        stStuPlayPhone.bOpen      = true;
        stStuPlayPhone.enMode     = SERVER_MODE;
        stStuPlayPhone.nIntervals = 2 * 1000;

        /* 学生课堂纪律识别 */
        AlgorithmParam_S stStuDiscipline;
        stStuDiscipline.bOpen      = true;
        stStuDiscipline.enMode     = LOCAL_MODE;
        stStuDiscipline.nIntervals = 1 * 1000;
        if (stVodHeartInfo.nDevCount >= DEV_LIMIT_NUM)
        {
            m_stParam.stCountStudents.nIntervals    = 2 * 1000;
            m_stParam.stTeTelephoneCalls.nIntervals = 2 * 1000;
            m_stParam.stStudentBehavior.nIntervals  = 2 * 1000;
            m_stParam.stEmoDetecr.nIntervals        = 2 * 1000;
            m_stParam.stTeFaceDetecr.nIntervals     = 15 * 1000;
            m_stParam.stStFaceDetecr.nIntervals     = 20 * 1000;
            m_stParam.stTrackTeacher.nIntervals     = 2 * 1000;
            m_stParam.stBoardDetecr.nIntervals      = 5 * 1000;
        }
        else
        {
            m_stParam.stCountStudents.nIntervals    = 1 * 1000;
            m_stParam.stTeTelephoneCalls.nIntervals = 1 * 1000;
            m_stParam.stStudentBehavior.nIntervals  = 1 * 1000;
            m_stParam.stEmoDetecr.nIntervals        = 1 * 1000;
            m_stParam.stTeFaceDetecr.nIntervals     = 5 * 1000;
            m_stParam.stStFaceDetecr.nIntervals     = 5 * 1000;
            m_stParam.stTrackTeacher.nIntervals     = 1 * 1000;
            m_stParam.stBoardDetecr.nIntervals      = 1 * 1000;
        }

        set_countStudentsParam(m_stParam.stCountStudents);
        set_teTelephoneCallsParam(m_stParam.stTeTelephoneCalls);
        set_stPlayPhoneParam(stStuPlayPhone);
        set_stDisciplineParam(stStuDiscipline);
        set_studentBehaviorParam(m_stParam.stStudentBehavior);
        set_emoDetecrParam(m_stParam.stEmoDetecr);
        set_teFaceDetecrParam(m_stParam.stTeFaceDetecr);
        set_stFaceDetecrParam(m_stParam.stStFaceDetecr);
        set_trackTeacherParam(m_stParam.stTrackTeacher);
        set_boardDetecrParam(m_stParam.stBoardDetecr);
    }
    return OK;
}

/* 与aiServer连接的数据回调函数 */
BlError_E AiManage_NS::CAiManage::aiServer_dataCallback(COMM_NS::DataParam_S stInfo)
{
    if (stInfo.nCode == AiManage_NS::PC_CMD_HEARTBEAT_STATUS)
    {
        deal_aiServer_heartData(stInfo.pchMessege);
        /*心跳跳过*/
        return OK;
    }

    // dlog(LOG_TRACE, "[与aiServer通讯的客户端] 接收到:\n%s", stInfo.pchMessege);
    if (nullptr == m_pParseBase ||
        nullptr == m_pResultsBase)
    {
        return OK;
    }

    BlError_E enRetCode = OK;

    switch (stInfo.nCode)
    {
        /* 获取设备信息 */
        case AiManage_NS::AI_GET_DEV_INFO:
        {
            DevInfo_S   stDevInfo;
            std::string strOutJson;
            enRetCode = get_device_info(stDevInfo);
            if (enRetCode < 0)
            {
                dlog(LOG_ERROR, "[AI_GET_DEV_INFO] 设备信息读取失败");
                break;
            }
            /* 转换 */
            enRetCode = m_pParseBase->convert(stDevInfo, strOutJson, (int)AI_GET_DEV_INFO);
            if (enRetCode < 0)
            {
                dlog(LOG_ERROR, "[AI_GET_DEV_INFO] Json数据转换失败");
                break;
            }
            /* 发送 */
            COMM_NS::SendDataInfo_S stSendInfo;
            stSendInfo.nDataSize = strOutJson.length();
            stSendInfo.pDate     = const_cast<char*>(strOutJson.data());
            stSendInfo.nCode     = stInfo.nCode;
            dlog(LOG_INFO, "[AI_GET_DEV_INFO] stSendInfo.pDate = %s", stSendInfo.pDate);
            m_pAiServerComm->send(stSendInfo);
            break;
        }
        /* 板书识别 */
        case AiManage_NS::AI_COM_BOARD:
        {
            AiManage_NS::BehaviorInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析板书识别数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_boardDetecr(stResultsInfo);

            break;
        }
        /* 表情识别 */
        case AiManage_NS::AI_COM_EMO:
        {
            AiManage_NS::EmoInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析表情识别数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_emoDetecr(stResultsInfo);


            break;
        }
        /* 学生人脸识别 */
        case AiManage_NS::AI_COM_ST_FACE:
        {
            AiManage_NS::FaceInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析学生人脸识别数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_stFaceDetecr(stResultsInfo);


            break;
        }
        /* 老师人脸识别 */
        case AiManage_NS::AI_COM_TE_FACE:
        {
            AiManage_NS::FaceInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析老师人脸识别数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_teFaceDetecr(stResultsInfo);


            break;
        }
        /* 轨迹/人头检测 */
        case AiManage_NS::AI_COM_HEAD:
        {
            AiManage_NS::TrackInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析轨迹/人头检测数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_trackTeacher(stResultsInfo);


            break;
        }
        /* 人数统计 */
        case AiManage_NS::AI_COM_NUM_COUNTER:
        {
            AiManage_NS::NumberInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析人数统计数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_countStudents(stResultsInfo);

            break;
        }
        /* 行为分析 */
        case AiManage_NS::AI_COM_ST_BEHAVIOR:
        {
            AiManage_NS::BehaviorInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析学生行为分析数据失败");
                break;
            }
            /* 成功，处理 */
            m_pResultsBase->deal_studentBehavior(stResultsInfo);

            break;
        }
        /* 教师接打电话识别 */
        case AiManage_NS::AI_COM_TEA_CALLPHONE:
        {
            // dlog(LOG_TRACE, "[与aiServer通讯的客户端] 接收到:\n%s", stInfo.pchMessege);
            AiManage_NS::BehaviorInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析接打电话识别数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_CallPhone(stResultsInfo);


            break;
        }
        /* 学生玩手机识别 */
        case AiManage_NS::AI_COM_ST_PLAYPHONE:
        {
            // dlog(LOG_TRACE, "[与aiServer通讯的客户端] 接收到:\n%s", stInfo.pchMessege);
            AiManage_NS::BehaviorInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析学生玩手机识别数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_PlayPhone(stResultsInfo);


            break;
        }
        /* 更新班级信息 */
        case AiManage_NS::AI_UPDATE_FACE:
        {
            int nReturn = -1;
            enRetCode   = m_pParseBase->parse(stInfo.pchMessege, nReturn);
            if (m_stParam.notifyWsFaceState)
            {
                m_stParam.notifyWsFaceState((BlError_E)nReturn);
            }

            close_updateFaceTimer((void*)this);

            break;
        }
        /* 学生回答问题人脸识别 */
        case AiManage_NS::AI_COM_STAS_FACE:
        {
            AiManage_NS::FaceInfo_S stResultsInfo;
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, stResultsInfo);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "解析学生回答问题人脸识别数据失败");
                break;
            }

            /* 成功，处理 */
            m_pResultsBase->deal_stAsFaceDetecr(stResultsInfo);

            break;
        }
        /* AI删除设备 */
        case AiManage_NS::AI_DELETE_DEV:
        {
            /*清空信息*/
            m_nAiServerStatus.store(0);
            if (m_stParam.setAiServerIp)
            {
                m_stParam.setAiServerIp("");
                dlog(LOG_INFO, "[清空信息] =======");
            }

            if (m_stParam.aiServerStatusChange)
            {
                m_stParam.aiServerStatusChange();
            }

            break;
        }
        default:
        {
            dlog(LOG_ERROR, "[与aiServer通讯的客户端] 未定义命令处理[%d]", stInfo.nCode);
            break;
        }
    }

    return OK;
}

/* 与本地连接的数据回调函数 */
BlError_E AiManage_NS::CAiManage::localServer_dataCallback(COMM_NS::DataParam_S stInfo)
{
    if (stInfo.nCode == AiManage_NS::PC_CMD_HEARTBEAT_STATUS)
    {
        /*心跳跳过*/
        return OK;
    }

    dlog(LOG_TRACE, "[ai逻辑的本地服务器] 接收到[%s]", stInfo.pchMessege);
    if (nullptr == m_pParseBase)
    {
        return OK;
    }

    BlError_E enRetCode = OK;

    switch (stInfo.nCode)
    {
        /* 获取设备信息 */
        case AiManage_NS::AI_GET_DEV_INFO:
        {
            DevInfo_S   stDevInfo;
            std::string strOutJson;

            /* 获取本机IP信息 */
            enRetCode = get_device_info(stDevInfo);
            if (enRetCode < 0)
            {
                dlog(LOG_ERROR, "[AI_GET_DEV_INFO] 设备信息读取失败");
                break;
            }
            /* 转换 */
            enRetCode = m_pParseBase->convert(stDevInfo, strOutJson, AI_GET_DEV_INFO);
            if (enRetCode < 0)
            {
                dlog(LOG_ERROR, "[AI_GET_DEV_INFO] Json数据转换失败");
                break;
            }
            /* 发送本机IP信息 */
            COMM_NS::SendDataInfo_S stSendInfo;
            stSendInfo.nDataSize = strOutJson.length();
            stSendInfo.pDate     = const_cast<char*>(strOutJson.data());
            stSendInfo.nCode     = stInfo.nCode;
            dlog(LOG_INFO, "[AI_GET_DEV_INFO] stSendInfo.pDate = %s", stSendInfo.pDate);
            m_pLocalServerComm->send(stSendInfo);
            break;
        }
        /* 设置AI服务器IP */
        case AiManage_NS::AI_SET_IP_INFO:
        {
            std::string strAiIp;
            std::string strAiServerIp;
            std::string strOutJson;
            /*解析*/
            enRetCode = m_pParseBase->parse(stInfo.pchMessege, strAiIp);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "[AI_SET_IP_INFO] 解析失败");
                break;
            }

            if (nullptr == m_stParam.getAiServerIp)
            {
                enRetCode = ERR_PARAM;
                dlog(LOG_ERROR, "[AI_SET_IP_INFO] 解析失败");
                break;
            }

            /* 读取AI服务器信息 */
            strAiServerIp = m_stParam.getAiServerIp();

            /* AI服务器IP校验 */
            if (strAiServerIp != strAiIp)
            {
                /* 断开旧的，连接新的 */
                if (m_pAiServerComm)
                {
                    delete m_pAiServerComm;
                    m_pAiServerComm = nullptr;
                    dlog(LOG_INFO, "[AI_SET_IP_INFO] 断开AI服务器连接，更换AI服务器");
                }

                /* 存储AI服务器信息 */
                if (m_stParam.setAiServerIp)
                {
                    m_stParam.setAiServerIp(strAiIp);
                    dlog(LOG_INFO, "[存储AI服务器信息] %s", strAiIp.c_str());
                }
            }

            /* 转换 */
            enRetCode = m_pParseBase->convert(enRetCode, strOutJson, AI_SET_IP_INFO);
            if (enRetCode < OK)
            {
                dlog(LOG_ERROR, "[AI_SET_IP_INFO] Json数据转换失败");
                break;
            }
            /* 发送本机IP信息 */
            COMM_NS::SendDataInfo_S stSendInfo;
            stSendInfo.nDataSize = strOutJson.length();
            stSendInfo.pDate     = const_cast<char*>(strOutJson.data());
            stSendInfo.nCode     = stInfo.nCode;
            dlog(LOG_INFO, "[AI_SET_IP_INFO] stSendInfo.pDate = %s", stSendInfo.pDate);
            m_pLocalServerComm->send(stSendInfo);

            /* 连接AI服务器 */
            if (!m_pAiServerComm)
            {
                initComm_aiServer();
                dlog(LOG_INFO, "[AI_SET_IP_INFO] 连接AI服务器");
            }
            break;
        }
        default:
        {
            dlog(LOG_ERROR, "[ai逻辑的本地服务器] 未定义命令处理[%d]", stInfo.nCode);
            break;
        }
    }


    return OK;
}

/* 学生人数统计定时回调函数 */
int AiManage_NS::CAiManage::countStudents_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_stParam.stCountStudents.bOpen == false)
        {
            if (pThis->m_pCountStudentsTimeKeep)
            {
                TimeKeep_stop(pThis->m_pCountStudentsTimeKeep);
            }
            return OK;
        }

        if (pThis->m_pCountStudentsTimeKeep)
        {
            TImeKeep_resume(pThis->m_pCountStudentsTimeKeep);
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_NUM_COUNTER);
}

/* 教师接打电话识别定时回调函数 */
int AiManage_NS::CAiManage::teTelephoneCalls_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_pTeTelephoneCallsTimeKeep)
        {
            TImeKeep_resume(pThis->m_pTeTelephoneCallsTimeKeep);
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_TEA_CALLPHONE);
}

/* 学生玩手机识别定时回调函数 */
int AiManage_NS::CAiManage::stPlayPhone_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_pStuPlayPhoneTimeKeep)
        {
            TImeKeep_resume(pThis->m_pStuPlayPhoneTimeKeep);
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_ST_PLAYPHONE);
}

/* 学生课堂纪律定时回调函数 */
int AiManage_NS::CAiManage::stDiscipline_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_pStuDisciplineTimeKeep)
        {
            TImeKeep_resume(pThis->m_pStuDisciplineTimeKeep);
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_ST_DISCIPLINE);
}

/* 学生行为分析定时回调函数 */
int AiManage_NS::CAiManage::studentBehavior_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_stParam.stStudentBehavior.bOpen == false)
        {
            if (pThis->m_pStudentBehaviorTimeKeep)
            {
                TimeKeep_stop(pThis->m_pStudentBehaviorTimeKeep);
            }
            return OK;
        }

        if (pThis->m_pStudentBehaviorTimeKeep)
        {
            TImeKeep_resume(pThis->m_pStudentBehaviorTimeKeep);
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_ST_BEHAVIOR);
}

/* 表情识别定时回调函数 */
int AiManage_NS::CAiManage::emoDetecr_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_stParam.stEmoDetecr.bOpen == false)
        {
            if (pThis->m_pEmoDetecrTimeKeep)
            {
                TimeKeep_stop(pThis->m_pEmoDetecrTimeKeep);
            }
            return OK;
        }

        if (pThis->m_pEmoDetecrTimeKeep)
        {
            TImeKeep_resume(pThis->m_pEmoDetecrTimeKeep);
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_EMO);
}

/* 老师人脸识别定时回调函数 */
int AiManage_NS::CAiManage::teFaceDetecr_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (nullptr == pThis)
    {
        dlog(LOG_ERROR, "参数为空");
        return ERR_PARAM;
    }

    if (pThis->m_stParam.stTeFaceDetecr.bOpen == false)
    {
        if (pThis->m_pTeFaceDetecrTimeKeep)
        {
            TimeKeep_stop(pThis->m_pTeFaceDetecrTimeKeep);
        }

        return OK;
    }

    if (nullptr == pThis->m_stParam.getRecordState ||
        nullptr == pThis->m_pResultsBase)
    {
        dlog(LOG_ERROR, "参数为空");
        return ERR_PARAM;
    }


    // if (pThis->m_stParam.getRecordState() == 0)
    // {
    //     /* 不用进行分析 */
    //     /* 重新开始一个考勤大循环定时器 */
    //     if (pThis->m_pTeFaceDetecrTimeKeep)
    //     {
    //         TImeKeep_resume(pThis->m_pTeFaceDetecrTimeKeep);
    //     }
    //     /* 停止考勤小循环定时器 */
    //     if (pThis->m_pTeFaceDetecrTimeKeep5s)
    //     {
    //         TimeKeep_stop(pThis->m_pTeFaceDetecrTimeKeep5s);
    //     }
    //     return NOK;
    // }

    if (pThis)
    {
        if (pThis->m_pResultsBase->getTeAttendanceSize() >= 1 ||
            pThis->m_nCurRecordTime.load() > 6 * 60)
        {
            /* 开始写一个考勤大循环定时器 */
            if (pThis->m_pTeFaceDetecrTimeKeep)
            {
                TImeKeep_resume(pThis->m_pTeFaceDetecrTimeKeep);
            }

            /* 停止考勤小循环定时器 */
            if (pThis->m_pTeFaceDetecrTimeKeep5s)
            {
                TimeKeep_stop(pThis->m_pTeFaceDetecrTimeKeep5s);
            }
        }
        else
        {
            /* 开始一个考勤小循环定时器 */
            if (pThis->m_pTeFaceDetecrTimeKeep5s)
            {
                TImeKeep_resume(pThis->m_pTeFaceDetecrTimeKeep5s);
            }
            /* 停止考勤大循环定时器 */
            if (pThis->m_pTeFaceDetecrTimeKeep)
            {
                TimeKeep_stop(pThis->m_pTeFaceDetecrTimeKeep);
            }
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_TE_FACE);
}

/* 学生人脸识别定时回调函数 */
int AiManage_NS::CAiManage::stFaceDetecr_taskFun(void* pParam)
{
    BlError_E enRetCode = OK;

    CAiManage* pThis = (CAiManage*)pParam;

    if (nullptr == pThis)
    {
        dlog(LOG_ERROR, "参数为空");
        return ERR_PARAM;
    }

    if (pThis->m_stParam.stStFaceDetecr.bOpen == false)
    {
        if (pThis->m_pStFaceDetecrTimeKeep)
        {
            TimeKeep_stop(pThis->m_pStFaceDetecrTimeKeep);
        }

        return OK;
    }

    if (nullptr == pThis->m_stParam.getRecordState ||
        nullptr == pThis->m_pResultsBase)
    {
        dlog(LOG_ERROR, "参数为空");
        return ERR_PARAM;
    }


    if (pThis->m_stParam.getRecordState() == 0)
    {
        /* 不用进行分析 */
        /* 重新开始一个考勤大循环定时器 */
        if (pThis->m_pStFaceDetecrTimeKeep)
        {
            TImeKeep_resume(pThis->m_pStFaceDetecrTimeKeep);
        }
        /* 停止考勤小循环定时器 */
        if (pThis->m_pStFaceDetecrTimeKeep5s)
        {
            TimeKeep_stop(pThis->m_pStFaceDetecrTimeKeep5s);
        }
        return NOK;
    }

    if (pThis && pThis->m_stParam.PTZControl)
    {
        if (!pThis->m_bStudentCloseUp.load() &&
            (!pThis->m_bStudentScreen.load() ||
             (!pThis->m_bTeacherPodium.load() && pThis->m_bStudentScreen.load())))
        {
            if (pThis->m_stParam.PTZControl())
            {
                /* 调用成功 */

                /* 发送学生考勤命令 */
                enRetCode = pThis->m_stParam.notifyGetSteamData(AI_COM_ST_FACE);

                /* 开始一个考勤小循环定时器 */
                if (pThis->m_pStFaceDetecrTimeKeep5s)
                {
                    TImeKeep_resume(pThis->m_pStFaceDetecrTimeKeep5s);
                }
                /* 停止考勤大循环定时器 */
                if (pThis->m_pStFaceDetecrTimeKeep)
                {
                    TimeKeep_stop(pThis->m_pStFaceDetecrTimeKeep);
                }
            }
            else
            {
                /* 调用失败-表示已经循环考勤了一遍 */

                if (pThis->m_pResultsBase->getStAttendanceSize() >= pThis->m_pResultsBase->getCurPeopleSize() ||
                    pThis->m_nCurRecordTime.load() > 6 * 60)
                {
                    dlog(LOG_ERROR, "结束学生考勤");
                    /* 开始写一个考勤大循环定时器 */
                    if (pThis->m_pStFaceDetecrTimeKeep)
                    {
                        TImeKeep_resume(pThis->m_pStFaceDetecrTimeKeep);
                    }
                    /* 停止考勤小循环定时器 */
                    if (pThis->m_pStFaceDetecrTimeKeep5s)
                    {
                        TimeKeep_stop(pThis->m_pStFaceDetecrTimeKeep5s);
                    }
                }
                else
                {
                    /* 开始一个考勤小循环定时器 */
                    if (pThis->m_pStFaceDetecrTimeKeep5s)
                    {
                        TImeKeep_resume(pThis->m_pStFaceDetecrTimeKeep5s);
                    }
                    /* 停止考勤大循环定时器 */
                    if (pThis->m_pStFaceDetecrTimeKeep)
                    {
                        TimeKeep_stop(pThis->m_pStFaceDetecrTimeKeep);
                    }
                }
            }
        }
        else
        {
            dlog(LOG_ERROR, "学生特写抢夺考勤摄像头权限, 暂停学生考勤 m_bStudentScreen[%d] m_bTeacherPodium[%d] m_bStudentCloseUp[%d]",
                 pThis->m_bStudentScreen.load(),
                 pThis->m_bTeacherPodium.load(),
                 pThis->m_bStudentCloseUp.load());
            /* 开始一个考勤小循环定时器 */
            if (pThis->m_pStFaceDetecrTimeKeep5s)
            {
                TImeKeep_resume(pThis->m_pStFaceDetecrTimeKeep5s);
            }
            /* 停止考勤大循环定时器 */
            if (pThis->m_pStFaceDetecrTimeKeep)
            {
                TimeKeep_stop(pThis->m_pStFaceDetecrTimeKeep);
            }
        }
    }

    return enRetCode;
}

/* 老师轨迹识别定时回调函数 */
int AiManage_NS::CAiManage::trackTeacher_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_stParam.stTrackTeacher.bOpen == false)
        {
            if (pThis->m_pTrackTeacherTimeKeep)
            {
                TimeKeep_stop(pThis->m_pTrackTeacherTimeKeep);
            }
            return OK;
        }

        if (pThis->m_pTrackTeacherTimeKeep)
        {
            TImeKeep_resume(pThis->m_pTrackTeacherTimeKeep);
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_HEAD);
}

/* 板书识别定时回调函数 */
int AiManage_NS::CAiManage::boardDetecr_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        /* 判断是否是晨安跟踪，晨安跟踪不需要AI分析板书，直接调用teacherBoard */
        if (nullptr != pThis->m_stParam.getTrackTypeInfo)
        {
            if (3 == pThis->m_stParam.getTrackTypeInfo())
            {
                if (pThis->m_pBoardDetecrTimeKeep)
                {
                    TimeKeep_stop(pThis->m_pBoardDetecrTimeKeep);
                }

                return OK;
            }
        }

        if (pThis->m_stParam.stBoardDetecr.bOpen == false)
        {
            if (pThis->m_pBoardDetecrTimeKeep)
            {
                TimeKeep_stop(pThis->m_pBoardDetecrTimeKeep);
            }

            return OK;
        }

        if (pThis->m_pBoardDetecrTimeKeep)
        {
            TImeKeep_resume(pThis->m_pBoardDetecrTimeKeep);
        }
    }

    return pThis->m_stParam.notifyGetSteamData(AI_COM_BOARD);
}

/* 人脸更新定时回调函数 */
int AiManage_NS::CAiManage::updateFace_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    close_updateFaceTimer(pParam);

    if (pThis->m_stParam.notifyWsFaceState)
    {
        pThis->m_stParam.notifyWsFaceState(ERR_GET_FAULT);
    }
    return 0;
}

/* 1s定时回调函数 */
int AiManage_NS::CAiManage::timer1s_taskFun(void* pParam)
{
    CAiManage* pThis = (CAiManage*)pParam;

    if (pThis)
    {
        if (pThis->m_pCurRecordTimeKeep1s)
        {
            TImeKeep_resume(pThis->m_pCurRecordTimeKeep1s);
            pThis->m_nCurRecordTime.fetch_add(1);

            dlog(LOG_TRACE, "AI分析的录制时长计时器，当前录制时长：%d", pThis->m_nCurRecordTime.load());
            /* 同步录制时间 */
            if (pThis->m_stParam.getRecordTime)
            {
                if (std::abs(pThis->m_nCurRecordTime.load() - pThis->m_stParam.getRecordTime()) > 5)
                {
                    pThis->m_nCurRecordTime.store(pThis->m_stParam.getRecordTime());
                }
            }

            /* 确保刚开始老师位置状态正常 */
            if (pThis->m_nCurRecordTime.load() < 3)
            {
                /* 判断老师是否在讲台 */
                if (pThis->m_bTeacherPodium.load())
                {
                    pThis->startTaught();
                }
            }
        }
    }

    return 0;
}

/* 获取当前录制时长 */
int AiManage_NS::CAiManage::getRecordTimeFunc()
{
    return m_nCurRecordTime.load();
}

/* 获取是否在学生特写 */
bool AiManage_NS::CAiManage::getStudentCloseUp()
{
    return m_bStudentCloseUp.load();
}

/* 获取是否老师上下讲台 */
bool AiManage_NS::CAiManage::getTeacherPodium()
{
    return m_bTeacherPodium.load();
}

/* 判断是否为本地分析模式 */
bool AiManage_NS::CAiManage::isLocalMode()
{
    if (m_stParam.stStudentBehavior.enMode == LOCAL_MODE ||
        m_nAiServerStatus.load() == 0)
    {
        return true;
    }
    return false;
}

/* 获取本机mac地址 */
int AiManage_NS::CAiManage::get_mac(char* pMac, int nLen)
{
    struct ifreq ifreq;    // ifreq结构体常用来配置和获取ip地址
    int          nSocket;

    if ((nSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }
    strcpy(ifreq.ifr_name, "eth0");    // Currently, only get eth0

    if (ioctl(nSocket, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror("ioctl");
        return -1;
    }
    close(nSocket);
    return snprintf(pMac, nLen, "%02X:%02X:%02X:%02X:%02X:%02X", (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
                    (unsigned char)ifreq.ifr_hwaddr.sa_data[1], (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
                    (unsigned char)ifreq.ifr_hwaddr.sa_data[3], (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
                    (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
}