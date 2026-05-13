#include "GetClassInfo.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ConvertInterface.h"
#include "ConvertJson.hpp"
#include "CurlHttp.h"
#include "dlog.h"
#include "JsonInterfase.h"
#include "ToolFunc.hpp"

extern "C" {
#include "edukit_network.h"
}

/* 下载平台ai人脸图片数据脚本路径 */
#define DOWNLOAD_PLATFORM_PIC ("/opt/bl/shell/download.sh %s %s %s %s %s")
#define CLASS_INFO_JSON       ("/opt/bl/.config/user_data/class_info.json")

using namespace Ai0630_NS;

Ai0630_NS::GetClassInfo::GetClassInfo()
{
    m_bRunning   = true;
    m_sendThread = std::thread(&GetClassInfo::senderLoop, this);

    /* 读取配置文件 */
    char* pchJsonData = ToolFunc::readJson_from_file(CLASS_INFO_JSON);
    if (pchJsonData)
    {
        to_struct(pchJsonData, m_stClassInfo);
        dlog(LOG_ERROR, "----------%d", m_stClassInfo.nClassId);
        free(pchJsonData);
        pchJsonData = NULL;
    }
}

Ai0630_NS::GetClassInfo::~GetClassInfo()
{
    disconnect(&sig_sendFaceData);
}

/* 设置平台IP地址 */
BlError_E Ai0630_NS::GetClassInfo::setPlatformIp(std::string strIp)
{
    m_strPlatformIp = strIp;
    return OK;
}

/* 获取并更新班级信息 */
BlError_E Ai0630_NS::GetClassInfo::update_classInfo()
{
    m_callQueue.pushUnique(&GetClassInfo::updateClassInfo, this);

    return OK;
}

/* 解析Json数据-获取错误 */
BlError_E GetClassInfo::parsePlatform(
    const char*  pchJson,
    int&         nError,
    std::string& strError)
{
    if (NULL == pchJson)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_IN_PARAM_NULL;
    }

    Json::Object* pJsonHandle = NULL;
    bool          bRet        = false;
    int           nReturn     = 0;

    pJsonHandle = Json::init(pchJson);

    bRet = Json::get(pJsonHandle, "result", nError);
    if (!bRet)
    {
        dlog(LOG_ERROR, "解析[result]字段失败");
        return ERR_PARSE;
    }

    bRet = Json::get(pJsonHandle, "return_message", strError);
    if (!bRet)
    {
        dlog(LOG_ERROR, "解析[return_message]字段失败");
        return ERR_PARSE;
    }

    return OK;
}

/* 解析Json数据-获取token值 */
BlError_E GetClassInfo::parseToken(
    const char*  pchJson,
    std::string& strToken)
{
    if (NULL == pchJson)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_IN_PARAM_NULL;
    }
    BlError_E enRetCode = OK;
    bool      bRet      = false;
    int       nReturn   = 0;

    Json::Object* pJsonHandle = NULL;
    Json::Object* pJsonData   = NULL;
    int           nExpires;

    pJsonHandle = Json::init(pchJson);
    if (!pJsonHandle)
    {
        dlog(LOG_ERROR, "pJsonHandle传入参数错误, %s", pchJson);
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    pJsonData = Json::get(pJsonHandle, "data");
    if (!pJsonData)
    {
        dlog(LOG_ERROR, "解析[data]字段失败, pchJson = %s", pchJson);
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    bRet = Json::get(pJsonData, "access_token", strToken);
    if (!bRet)
    {
        dlog(LOG_ERROR, "解析[access_token]字段失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    bRet = Json::get(pJsonData, "expires", nExpires);
    if (!bRet)
    {
        dlog(LOG_ERROR, "解析[expires]字段失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

EXIT:
    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* 解析Json数据-获取班级成员信息 */
BlError_E GetClassInfo::parseClassInfo(
    const char*  pchJson,
    ClassInfo_S& stClassInfo)
{
    if (NULL == pchJson)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_IN_PARAM_NULL;
    }
    BlError_E     enRetCode        = OK;
    int           nSize            = 0;
    bool          bRet             = false;
    Json::Object* pJsonHandle      = NULL;
    Json::Object* pDataObject      = NULL;
    Json::Object* pChildDataObject = NULL;
    Json::Object* pClassObject     = NULL;
    Json::Object* pTeaObject       = NULL;
    Json::Object* pStuObject       = NULL;
    Json::Object* pItemObject      = NULL;

    /*创建操作句柄*/
    pJsonHandle = Json::init(pchJson);
    if (NULL == pJsonHandle)
    {
        dlog(LOG_ERROR, "传入的Json字符串有问题, 无法创建句柄");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    pDataObject = Json::get(pJsonHandle, "data");
    if (NULL == pDataObject)
    {
        dlog(LOG_ERROR, "获取[data]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    pChildDataObject = Json::get(pDataObject, "data");
    if (NULL == pChildDataObject)
    {
        dlog(LOG_ERROR, "获取[data]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /*解析班级信息*/
    pClassObject = Json::get(pChildDataObject, "class");
    if (NULL == pClassObject)
    {
        dlog(LOG_ERROR, "获取[class]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    bRet = Json::get(pClassObject, "class_id", stClassInfo.nClassId);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[class_id]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }
    bRet = Json::get(pClassObject, "class_name", stClassInfo.strClassName);
    if (!bRet)
    {
        dlog(LOG_ERROR, "获取[class_name]数据节点-失败");
        enRetCode = ERR_PARSE;
        goto EXIT;
    }

    /*解析教师信息*/
    pTeaObject = Json::get(pChildDataObject, "teacher");
    if (NULL != pTeaObject)
    {
        /* 获取数组大小 */
        nSize = Json::Array::size(pTeaObject);
        if (nSize <= 0)
        {
            dlog(LOG_ERROR, "数组大小异常[%d]", nSize);
        }
        for (int i = 0; i < nSize; i++)
        {
            /* 获取数组的节点 */
            pItemObject = Json::Array::get(pTeaObject, i);
            if (NULL == pItemObject)
            {
                dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            FaceLibsInfo_S stFaceLibsInfo;
            stFaceLibsInfo.nIdentity = 1;
            stFaceLibsInfo.nClassId  = stClassInfo.nClassId;

            bRet = Json::get(pItemObject, "user_id", stFaceLibsInfo.nMemberId);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[user_id]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "name", stFaceLibsInfo.strName);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[name]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "face_path", stFaceLibsInfo.strRemotePicPath);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[face_path]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "face_md5", stFaceLibsInfo.strPicMd5);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[face_md5]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "file_name", stFaceLibsInfo.strPicName);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[file_name]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }
            stClassInfo.listTeaInfo.push_back(stFaceLibsInfo);
        }
    }
    else
    {
        dlog(LOG_TRACE, "获取[teacher]数据节点-为空");
    }


    /*解析学生信息*/
    pStuObject = Json::get(pChildDataObject, "student");
    if (NULL == pStuObject)
    {
        dlog(LOG_ERROR, "获取[student]数据节点-为空");
    }
    else
    {
        /* 获取数组大小 */
        nSize = Json::Array::size(pStuObject);
        if (nSize <= 0)
        {
            dlog(LOG_ERROR, "数组大小异常[%d]", nSize);
        }
        for (int i = 0; i < nSize; i++)
        {
            /* 获取数组的节点 */
            pItemObject = Json::Array::get(pStuObject, i);
            if (NULL == pItemObject)
            {
                dlog(LOG_ERROR, "获取数组节点失败 [%d]", i);
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            FaceLibsInfo_S stFaceLibsInfo;
            stFaceLibsInfo.nIdentity = 0;
            stFaceLibsInfo.nClassId  = stClassInfo.nClassId;

            bRet = Json::get(pItemObject, "user_id", stFaceLibsInfo.nMemberId);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[user_id]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "name", stFaceLibsInfo.strName);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[name]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "face_path", stFaceLibsInfo.strRemotePicPath);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[face_path]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "face_md5", stFaceLibsInfo.strPicMd5);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[face_md5]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }

            bRet = Json::get(pItemObject, "file_name", stFaceLibsInfo.strPicName);
            if (!bRet)
            {
                dlog(LOG_ERROR, "获取[file_name]数据节点-失败");
                enRetCode = ERR_PARSE;
                goto EXIT;
            }
            stClassInfo.listStuInfo.push_back(stFaceLibsInfo);
        }
    }


EXIT:

    if (pJsonHandle)
    {
        Json::deinit(pJsonHandle);
        pJsonHandle = NULL;
    }

    return enRetCode;
}

/* 获取文件后缀 */
std::string GetClassInfo::getFileExtension(const std::string& strFilePath)
{
    std::size_t nDotPos = strFilePath.find_last_of('.');
    if (nDotPos != std::string::npos && nDotPos != strFilePath.length() - 1)
    {
        return strFilePath.substr(nDotPos);
    }
    /* 返回空字符串表示没有后缀 */
    return "";
}

/* 判断md5值是否存在 */
bool GetClassInfo::Md5Exists(
    const std::list<FaceLibsInfo_S>& list,
    const std::string&               strMd5ToFind)
{
    for (const auto& stInfo : list)
    {
        if (stInfo.strPicMd5 == strMd5ToFind)
        {
            return true;
        }
    }

    return false;
}

/* 校验是否需要更新 */
bool GetClassInfo::needsUpdate(
    const ClassInfo_S& stNewInfo,
    const ClassInfo_S& stOldInfo)
{
    if (stOldInfo.listTeaInfo.size() == 0 ||
        stOldInfo.listStuInfo.size() == 0)
    {
        return true;
    }

    if (stOldInfo.listTeaInfo.size() != stNewInfo.listTeaInfo.size() ||
        stOldInfo.listStuInfo.size() != stNewInfo.listStuInfo.size())
    {
        return true;
    }

    /* 班级信息不同时同步数据 */
    if (stOldInfo.nClassId != stNewInfo.nClassId ||
        stOldInfo.strClassName != stNewInfo.strClassName)
    {
        return true;
    }

    for (const auto& info : stNewInfo.listTeaInfo)
    {
        if (!Md5Exists(stOldInfo.listTeaInfo, info.strPicMd5))
        {
            return true;
        }
    }

    for (const auto& info : stNewInfo.listStuInfo)
    {
        if (!Md5Exists(stOldInfo.listStuInfo, info.strPicMd5))
        {
            return true;
        }
    }

    return false;
}

/* 平台-获取Token值 */
BlError_E Ai0630_NS::GetClassInfo::getToken(void* pArgv)
{
    dlog(LOG_TRACE, "发送Token请求");
    BlError_E enRetCode = OK;

    CurlHttp::Get httpCtrl(m_strPlatformIp);

    /* 路径 */
    std::string strPath = "/accessToken";
    httpCtrl.set_path(strPath);

    /* 参数 */
    std::string            strItem;
    std::list<std::string> listParams;

    strItem = "company=BL";
    listParams.push_back(strItem);
    strItem = "device_name=TE-0600R";
    listParams.push_back(strItem);
    std::string strOutJson;
    strOutJson          = "{\"client_id\":\"20882088\",\"secret\":\"nGk5R2wrnZqQ02bed29rjzax1QWRIu1O\"}";
    std::string strData = "data=" + strOutJson;
    listParams.push_back(strData);

    httpCtrl.set_params(listParams);

    /* 发送请求 */
    int nRet = httpCtrl.send_request();
    if (nRet != 0)
    {
        ping();

        dlog(LOG_ERROR, "【获取班级信息类】 请求Token信息-失败[%s] header[%s]",
             httpCtrl.get_error(nRet).c_str(),
             strItem.c_str());
        enRetCode = ERR_GET_FAULT;
        return enRetCode;
    }

    /* 获取接收到的信息 */
    std::string strResponse;
    nRet = httpCtrl.get_recvData(strResponse);
    if (nRet <= 0)
    {
        dlog(LOG_ERROR, "【获取班级信息类】 请求Token信息-失败-接收到的数据为空");
        enRetCode = ERR_GET_FAULT;
        return enRetCode;
    }

    /* 判断返回是否异常 */
    int         nError = 0;
    std::string strError;
    enRetCode = parsePlatform(strResponse.c_str(), nError, strError);
    if (enRetCode != OK || nError != 200)
    {
        dlog(LOG_ERROR, "【获取班级信息类】 返回异常-[%d]:[%s]", nError, strError.c_str());
        return ERR_GET_FAULT;
    }

    dlog(LOG_TRACE, "【获取班级信息类】 请求Token信息-返回数据:\n%s", strResponse.c_str());

    /* 解析参数 */
    enRetCode = parseToken(strResponse.c_str(), m_strToken);
    if (enRetCode != OK)
    {
        dlog(LOG_ERROR, "【获取班级信息类】 请求Token信息-解析数据失败");
        return enRetCode;
    }

    dlog(LOG_TRACE, "【获取班级信息类】 组装后的token值[%s]", m_strToken.c_str());

    return OK;
}

/* 获取并更新班级信息 */
BlError_E Ai0630_NS::GetClassInfo::updateClassInfo()
{
    BlError_E enRetCode = OK;

    ClassInfo_S stClassInfo;
    stClassInfo.clear();

    /* 获取班级信息 */
    enRetCode = getClassInfo(stClassInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "【获取班级信息类】 获取班级信息失败");
        return ERR_GET_FAULT;
    }

    if (stClassInfo.listTeaInfo.size() == 0 &&
        stClassInfo.listStuInfo.size() == 0)
    {
        dlog(LOG_ERROR, "【获取班级信息类】 获取班级信息为空");
        return ERR_AI_CLASS_EMPTY;
    }

    /* 校验是否需要更新 */
    if (!needsUpdate(stClassInfo, m_stClassInfo))
    {
        dlog(LOG_INFO, "【获取班级信息类】 班级信息没有变化");
        return OK_EXIST;
    }

    /* 获取拖信息 */
    enRetCode = downloadPicInfo(stClassInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 获取图片信息失败");
        m_stClassInfo.clear();
    }
    else
    {
        m_stClassInfo = stClassInfo;
    }

    std::string strJson = to_string(m_stClassInfo);
    ToolFunc::writeJson_to_file(CLASS_INFO_JSON, strJson.c_str());

    return enRetCode;
}

/* 获取班级信息 */
BlError_E Ai0630_NS::GetClassInfo::getClassInfo(ClassInfo_S& stClassInfo)
{
    BlError_E  enRetCode = OK;
    static int s_nNumber = 0;

    CurlHttp::Get httpCtrl(m_strPlatformIp);

    /* 路径 */
    std::string strPath = "/api/ainew/getClassUser";
    httpCtrl.set_path(strPath);

    /* 参数 */
    std::string            strOutJson;
    char                   achIp[32] = { 0 };
    std::string            strLocalIp;
    int                    nRet   = 0;
    int                    nError = 0;
    std::string            strError;
    std::string            strItem;
    std::string            strData;
    std::string            strResponse;
    std::list<std::string> listParams;
    std::list<std::string> listHeaders;

    nRet       = ReachGetIPaddrstring(ETH0_INTERFACE, achIp);
    strLocalIp = achIp;
    // strLocalIp = "172.16.19.203";
    strOutJson = "{\"ip\":\"" + strLocalIp + "\"}";
    strData    = "data=" + strOutJson;

    listParams.push_back("company=BL");
    listParams.push_back("device_name=TE-0600R");
    listParams.push_back(strData);
    httpCtrl.set_params(listParams);

    /* 头信息 */
    strItem = "Authorization: Bearer " + m_strToken;

    listHeaders.push_back(strItem);
    httpCtrl.set_header(listHeaders);

    /* 发送请求 */
    nRet = httpCtrl.send_request();
    if (nRet != 0)
    {
        ping();

        dlog(LOG_ERROR, "获取班级成员信息-失败[%s] header[%s]",
             httpCtrl.get_error(nRet).c_str(),
             strItem.c_str());
        enRetCode = ERR_GET_FAULT;
        return enRetCode;
    }

    /* 获取接收到的信息 */
    nRet = httpCtrl.get_recvData(strResponse);
    if (nRet <= 0)
    {
        dlog(LOG_ERROR, "获取班级成员信息-失败-接收到的数据为空");
        enRetCode = ERR_GET_FAULT;
        return enRetCode;
    }
    dlog(LOG_TRACE, "strResponse.c_str() = %s", strResponse.c_str());

    /* 判断返回是否异常 */
    enRetCode = parsePlatform(strResponse.c_str(), nError, strError);
    if (enRetCode != OK || nError != 200)
    {
        if (nError == 401 && s_nNumber <= 10)
        {
            s_nNumber++;
            /* 立即获取一下Token值 */
            enRetCode = getToken();
            if (enRetCode < OK)
            {
                return ERR_GET_FAULT;
            }
            enRetCode = getClassInfo(stClassInfo);
            return enRetCode;
        }
        dlog(LOG_ERROR, "返回异常-[%d]:[%s]", nError, strError.c_str());
        return ERR_GET_FAULT;
    }
    s_nNumber = 0;

    /* 解析参数 */
    enRetCode = parseClassInfo(strResponse.c_str(), stClassInfo);
    if (enRetCode != OK)
    {
        dlog(LOG_ERROR, "获取班级成员信息-解析数据失败");
        return enRetCode;
    }

    return enRetCode;
}

/* 下载人脸图片信息 */
BlError_E Ai0630_NS::GetClassInfo::downloadPicInfo(ClassInfo_S& stClassInfo)
{
    BlError_E  enRetCode = OK;
    static int s_nNumber = 0;

    CurlHttp::Get httpCtrl(m_strPlatformIp);

    /* 参数 */
    std::string            strPath;
    std::string            strOutJson;
    char                   achIp[32] = { 0 };
    std::string            strLocalIp;
    int                    nRet = 0;
    std::string            strDesPath;
    std::string            strData;
    char                   chCmd[1000];
    char                   chTarCmd[1000];
    std::list<std::string> listParams;
    std::list<std::string> listHeaders;
    std::string            strItem;
    std::string            strResponse;
    int                    nError = 200;
    std::string            strError;

    /*获取本机IP*/
    nRet       = ReachGetIPaddrstring(ETH0_INTERFACE, achIp);
    strLocalIp = achIp;
    // strLocalIp = "172.16.19.203";

    /* 路径 */
    strPath = "/api/ainew/getFaceZip";
    httpCtrl.set_path(strPath);

    strOutJson = "{\"ip\":\"" + strLocalIp + "\"}";
    strData    = "data=" + strOutJson;
    listParams.push_back("company=BL");
    listParams.push_back("device_name=TE-0600R");
    listParams.push_back(strData);
    httpCtrl.set_params(listParams);

    /* 头信息 */
    strItem = "Authorization: Bearer " + m_strToken;
    listHeaders.push_back(strItem);
    httpCtrl.set_header(listHeaders);

    /* 发送请求 */
    nRet = httpCtrl.send_request();
    if (nRet != 0)
    {
        ping();

        dlog(LOG_ERROR, "下载人脸图片信息-失败[%s] header[%s]",
             httpCtrl.get_error(nRet).c_str(),
             strItem.c_str());
        enRetCode = ERR_GET_FAULT;
        return enRetCode;
    }

    /* 获取接收到的信息 */
    nRet = httpCtrl.get_recvData(strResponse);
    if (nRet <= 0)
    {
        dlog(LOG_ERROR, "下载人脸图片信息-失败-接收到的数据为空");
        enRetCode = ERR_GET_FAULT;
        return enRetCode;
    }

    /* 判断返回是否异常 */
    parsePlatform(strResponse.c_str(), nError, strError);
    if (nError != 200)
    {
        dlog(LOG_ERROR, "返回异常-[%d]:[%s]- strResponse = %s", nError, strError.c_str(), strResponse.c_str());
        if (nError == 401 && s_nNumber <= 10)
        {
            s_nNumber++;
            /* 立即获取一下Token值 */
            enRetCode = getToken();
            if (enRetCode < OK)
            {
                return ERR_GET_FAULT;
            }
            enRetCode = downloadPicInfo(stClassInfo);
            return enRetCode;
        }
        else
        {
            return ERR_GET_FAULT;
        }
    }
    s_nNumber = 0;

    /* 下载文件目标路径 */
    strDesPath = m_strPathBase + std::string("platformPic.zip");
    /* 调用脚本下载并传给AI服务器 DOWNLOAD_PLATFORM_PIC*/
    m_paramMutex.lock();
    sprintf(chCmd,
            DOWNLOAD_PLATFORM_PIC,
            m_strToken.c_str(),
            strDesPath.c_str(),
            m_strPlatformIp.c_str(),
            strLocalIp.c_str(),
            std::to_string(stClassInfo.nClassId).c_str());
    m_paramMutex.unlock();
    dlog(LOG_TRACE, "[脚本指令]-chCmd = %s", chCmd);

    /* 调用system函数执行命令 */
    nRet = system(chCmd);

    if (nRet == 0)
    {
        std::string strUnzipPath = m_strPathBase + std::to_string(stClassInfo.nClassId);

#ifdef SUPPORTS_CPP17
        if (!std::filesystem::exists(strUnzipPath) || !std::filesystem::is_directory(strUnzipPath))
        {
            dlog(LOG_ERROR, "文件不存在，或不是文件夹 [%s]", strUnzipPath.c_str());
            return ERR_NOT_EXIST;
        }

        /*文件遍历及其匹配*/
        for (const auto& entry : std::filesystem::directory_iterator(strUnzipPath))
        {
            if (entry.is_regular_file())
            {
                std::string strBaseFolder = m_strPathBase + std::to_string(stClassInfo.nClassId).c_str() + std::string("/");
                std::cout << "[遍历文件名] : " << entry.path().filename() << std::endl;
                /* 获取文件拓展名 */
                std::filesystem::path filePath     = entry.path().filename();
                std::string           strFileExten = filePath.extension();
                for (auto& stuInfo : stClassInfo.listStuInfo)
                {
                    /* 文件匹配 */
                    if (stuInfo.strPicName == entry.path().filename())
                    {
                        dlog(LOG_TRACE, "[班级信息]-stuInfo.strPicName = %s", stuInfo.strPicName.c_str());
                        /* 源文件 */
                        std::string strSourceFilePath = strBaseFolder + std::string(entry.path().filename());
                        /* 目标文件名 */
                        std::string strNewFile        = strBaseFolder + std::to_string(stuInfo.nMemberId) + std::string("_") + stuInfo.strPicMd5 + strFileExten;

                        /* 判断是否存在源文件 */
                        if (std::filesystem::exists(strSourceFilePath))
                        {
                            /* 判断是否存在目标文件 */
                            if (std::filesystem::exists(strNewFile))
                            {
                                /* 删除存在的目标文件 */
                                std::filesystem::remove(strNewFile);
                            }

                            try
                            {
                                std::filesystem::rename(strSourceFilePath, strNewFile);
                            }
                            catch (const std::filesystem::filesystem_error& e)
                            {
                                dlog(LOG_ERROR, "重命名文件名失败：%s", e.what());
                                continue;
                            }
                        }
                        else
                        {
                            continue;
                        }

                        /* 目标文件夹 */
                        std::string strTargetFolder = strBaseFolder + std::string("student");
                        stuInfo.strLocalPicPath     = strTargetFolder + std::string("/") + std::to_string(stuInfo.nMemberId) + std::string("_") + stuInfo.strPicMd5 + strFileExten;
                        dlog(LOG_TRACE, "strTargetFolder = %s", strTargetFolder.c_str());
                        if (!std::filesystem::exists(strTargetFolder))
                        {
                            /* 创建目标文件夹 */
                            std::filesystem::create_directory(strTargetFolder);
                            std::cout << "目标文件夹已创建" << std::endl;
                        }
                        else
                        {
                            std::cout << "目标文件夹已存在" << std::endl;
                        }

                        // dlog(LOG_TRACE, "%s->%s", strNewFile.c_str(), stuInfo.strLocalPicPath.c_str());
                        if (!ToolFunc::resizeImage(strNewFile, stuInfo.strLocalPicPath, 1920, 1024))
                        {
                            dlog(LOG_ERROR, "修改图片大小并移入指定文件夹-失败");
                        }
                        /* 删除原始文件 */
                        std::filesystem::remove(strNewFile);
                    }
                }

                for (auto& teaInfo : stClassInfo.listTeaInfo)
                {
                    /* 文件匹配 */
                    if (teaInfo.strPicName == entry.path().filename())
                    {
                        dlog(LOG_TRACE, "[班级信息]-teaInfo.strPicName = %s", teaInfo.strPicName.c_str());
                        /* 源文件 */
                        std::string strSourceFilePath = strBaseFolder + std::string(entry.path().filename());
                        /* 目标文件名 */
                        std::string strNewFile        = strBaseFolder + std::to_string(teaInfo.nMemberId) + std::string("_") + teaInfo.strPicMd5 + strFileExten;

                        /* 判断是否存在源文件 */
                        if (std::filesystem::exists(strSourceFilePath))
                        {
                            /* 判断是否存在目标文件 */
                            if (std::filesystem::exists(strNewFile))
                            {
                                /* 删除存在的目标文件 */
                                std::filesystem::remove(strNewFile);
                            }

                            try
                            {
                                std::filesystem::rename(strSourceFilePath, strNewFile);
                            }
                            catch (const std::filesystem::filesystem_error& e)
                            {
                                dlog(LOG_ERROR, "重命名文件名失败：%s", e.what());
                                continue;
                            }
                        }
                        else
                        {
                            continue;
                        }

                        /* 目标文件夹 */
                        std::string strTargetFolder = strBaseFolder + std::string("teacher");
                        teaInfo.strLocalPicPath     = strTargetFolder + std::string("/") + std::to_string(teaInfo.nMemberId) + std::string("_") + teaInfo.strPicMd5 + strFileExten;
                        dlog(LOG_TRACE, "strTargetFolder = %s", strTargetFolder.c_str());
                        if (!std::filesystem::exists(strTargetFolder))
                        {
                            /* 创建目标文件夹 */
                            std::filesystem::create_directory(strTargetFolder);
                            std::cout << "目标文件夹已创建" << std::endl;
                        }
                        else
                        {
                            std::cout << "目标文件夹已存在" << std::endl;
                        }
                        /* 修改图片大小并移入指定文件夹 */
                        // dlog(LOG_TRACE, "%s->%s", strNewFile.c_str(), teaInfo.strLocalPicPath.c_str());
                        if (!ToolFunc::resizeImage(strNewFile, teaInfo.strLocalPicPath, 1920, 1024))
                        {
                            dlog(LOG_ERROR, "修改图片大小并移入指定文件夹-失败");
                        }
                        /* 删除原始文件 */
                        std::filesystem::remove(strNewFile);
                    }
                }
            }
        }
#else
        /* 检查文件或目录是否存在 */
        struct stat stBuffer;
        if (stat(strUnzipPath.c_str(), &stBuffer) != 0 || !S_ISDIR(stBuffer.st_mode))
        {
            dlog(LOG_ERROR, "文件不存在，或不是文件夹 [%s]\n", strUnzipPath.c_str());
            return ERR_NOT_EXIST;
        }

        /* 使用 dirent.h 遍历目录 */
        DIR*           pstDir   = NULL;
        struct dirent* pstEntry = NULL;

        std::string strBaseFolder = m_strPathBase +
            std::to_string(stClassInfo.nClassId).c_str() +
            std::string("/");


        if ((pstDir = opendir(strUnzipPath.c_str())) != NULL)
        {
            while ((pstEntry = readdir(pstDir)) != NULL)
            {
                std::string strFilename = pstEntry->d_name;
                if (strFilename == "." || strFilename == "..")
                {
                    continue;
                }

                std::string strFullFilePath = strUnzipPath + "/" + strFilename;

                /* 判断是否为普通文件 */
                struct stat fileInfo;
                if (stat(strFullFilePath.c_str(), &fileInfo) == 0 && S_ISREG(fileInfo.st_mode))
                {
                    std::string strFileExten = getFileExtension(strFilename);

                    /* 处理学生信息 */
                    for (auto& stuInfo : stClassInfo.listStuInfo)
                    {
                        /* 文件匹配 */
                        if (stuInfo.strPicName == strFilename)
                        {
                            /* 源文件 */
                            std::string strSourceFilePath = strBaseFolder +
                                strFilename;
                            /* 目标文件名 */
                            stuInfo.strLocalPicPath = strBaseFolder +
                                std::string("student/") +
                                std::to_string(stuInfo.nMemberId) +
                                std::string("_") +
                                stuInfo.strPicMd5 + strFileExten;

                            /* 修改图片大小并移入指定文件夹 */
                            if (!ToolFunc::resizeImage(strSourceFilePath, stuInfo.strLocalPicPath, 1920, 1024))
                            {
                                dlog(LOG_ERROR, "修改图片大小并移入指定文件夹-失败");
                            }
                            /* 删除原始文件 */
                            std::system(("rm " + std::string(strSourceFilePath)).c_str());
                            break;
                        }
                    }

                    /* 处理老师信息 */
                    for (auto& stuInfo : stClassInfo.listTeaInfo)
                    {
                        /* 文件匹配 */
                        if (stuInfo.strPicName == strFilename)
                        {
                            /* 源文件 */
                            std::string strSourceFilePath = strBaseFolder +
                                strFilename;
                            /* 目标文件名 */
                            stuInfo.strLocalPicPath = strBaseFolder +
                                std::string("teacher/") +
                                std::to_string(stuInfo.nMemberId) +
                                std::string("_") +
                                stuInfo.strPicMd5 + strFileExten;


                            /* 修改图片大小并移入指定文件夹 */
                            if (!ToolFunc::resizeImage(strSourceFilePath, stuInfo.strLocalPicPath, 1920, 1024))
                            {
                                dlog(LOG_ERROR, "修改图片大小并移入指定文件夹-失败");
                            }
                            /* 删除原始文件 */
                            std::system(("rm " + std::string(strSourceFilePath)).c_str());
                            break;
                        }
                    }
                }
            }
            closedir(pstDir);
        }
        else
        {
            dlog(LOG_ERROR, "无法打开目录 [%s]\n", strUnzipPath.c_str());
            return ERR_OPEN;
        }

#endif

        std::string pathBase = m_strPathBase + std::to_string(stClassInfo.nClassId);
        stClassInfo.print();
        sig_sendFaceData.emit(stClassInfo);
    }
    else
    {
        dlog(LOG_ERROR, "脚本执行失败- [脚本指令]-chCmd = %s", chCmd);
    }

    return enRetCode;
}

/* ping 服务器 */
bool Ai0630_NS::GetClassInfo::ping()
{
    /* 主动ping一下 API。方便解析域名 */
    /* 需要执行的命令 */
    std::string strCommand = "ping -c 2 " + m_strPlatformIp;
    /* 调用system函数执行命令 */
    int         nResult    = system(strCommand.c_str());
    /* 检查命令是否成功执行 */
    if (nResult == 0)
    {
        dlog(LOG_TRACE, "ping [%s] 成功", m_strPlatformIp.c_str());
        return true;
    }
    else
    {
        dlog(LOG_ERROR, "ping [%s] 失败", m_strPlatformIp.c_str());
        return false;
    }
}

/* 发送请求线程 */
void Ai0630_NS::GetClassInfo::senderLoop()
{
    int nSize = 0;
    getToken();
    while (m_bRunning)
    {
        nSize = m_callQueue.size();
        if (nSize != 0 && nSize % 10 == 0)
        {
            dlog(LOG_ERROR, "【获取班级信息类】 发送队列数据堆积[%ld]", m_callQueue.size());
        }
        m_callQueue.tryPopAndRun();


        std::this_thread::sleep_for(std::chrono::milliseconds(200));    // 睡眠 200ms
    }
}
