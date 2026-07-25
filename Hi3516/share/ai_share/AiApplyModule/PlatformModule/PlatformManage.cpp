/*
 * @FilePath     : PlatformManage.cpp
 * @Author       : 李辉 lih@kfb.cn
 * @Date         : 2024-04-02 20:02:08
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-10-31 17:24:04
 * @Description  :
 */
#include "PlatformManage.hpp"

/* 检查C++标准的宏 */
#if __cplusplus >= 201703L
    #define SUPPORTS_CPP17
#endif

#include <chrono>

#ifdef SUPPORTS_CPP17
    #include <filesystem>
#endif

#include "CurlHttp.h"
#include "PublicFunc.hpp"

extern "C" {
#include "edukit_network.h"
}



using namespace PlatformManage_NS;

PlatformManage_NS::CPlatformManage::CPlatformManage(AiPlatformInParam_S stInParam)
    : CAiPlatformBase(stInParam)
{
    /* 初始化解析类 */
    m_stDataInfo.clear();
    ParseData_NS::InParam_S stInfo;
    stInfo.stNeedParam.enType = ParseData_NS::PARSE_JSON;

    m_pParseBase = ParseData_NS::CParseData::create(stInfo);
}

PlatformManage_NS::CPlatformManage::~CPlatformManage()
{
    dlog(LOG_TRACE, "释放ITC-平台获取信息类");
}

/*获取token值*/
BlError_E PlatformManage_NS::CPlatformManage::get_tokenInfo(void* pArgv)
{
    dlog(LOG_TRACE, "发送Token请求");
    BlError_E enRetCode = OK;

    m_paramMutex.lock();
    std::string strIP = std::string(m_stInParam.stNeedParam.achIpAddr);
    m_paramMutex.unlock();

    CurlHttp::Get httpCtrl(strIP);

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

        dlog(LOG_ERROR, "CPlatformManage-请求Token信息-失败[%s] header[%s]",
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
        dlog(LOG_ERROR, "CPlatformManage-请求Token信息-失败-接收到的数据为空");
        enRetCode = ERR_GET_FAULT;
        return enRetCode;
    }

    /* 判断返回是否异常 */
    int         nError = 0;
    std::string strError;
    enRetCode = m_pParseBase->parsePlatform(strResponse.c_str(), nError, strError);
    if (enRetCode != OK || nError != 200)
    {
        dlog(LOG_ERROR, "CPlatformManage-返回异常-[%d]:[%s]", nError, strError.c_str());
        return ERR_GET_FAULT;
    }

    dlog(LOG_TRACE, "CPlatformManage-请求Token信息-返回数据:\n%s", strResponse.c_str());

    /* 解析参数 */
    enRetCode = m_pParseBase->parseToken(strResponse.c_str(), m_strToken);
    if (enRetCode != OK)
    {
        dlog(LOG_ERROR, "CPlatformManage-请求Token信息-解析数据失败");
        return enRetCode;
    }

    dlog(LOG_TRACE, "CPlatformManage-组装后的token值[%s]", m_strToken.c_str());

    return OK;
}

BlError_E PlatformManage_NS::CPlatformManage::init()
{
    return get_tokenInfo();
}

bool PlatformManage_NS::CPlatformManage::Md5Exists(const std::list<HumanInfo_S>& list, const std::string& strMd5ToFind)
{
    for (const auto& stInfo : list)
    {
        if (stInfo.strMd5 == strMd5ToFind)
        {
            return true;
        }
    }

    return false;
}

std::string PlatformManage_NS::CPlatformManage::getFileExtension(const std::string& strFilePath)
{
    std::size_t nDotPos = strFilePath.find_last_of('.');
    if (nDotPos != std::string::npos && nDotPos != strFilePath.length() - 1)
    {
        return strFilePath.substr(nDotPos);
    }
    /* 返回空字符串表示没有后缀 */
    return "";
}

/* 数据校验 */
BlError_E PlatformManage_NS::CPlatformManage::check_dataChanged(PlatformManage_NS::DataInfo_S stDataInfo)
{
    if (m_stDataInfo.listTeaInfo.size() == 0 || m_stDataInfo.listStuInfo.size() == 0)
    {
        dlog(LOG_TRACE, "[数据校验] -------- 信息列表为空，需同步");
        return OK;
    }

    if (m_stDataInfo.listTeaInfo.size() != stDataInfo.listTeaInfo.size() ||
        m_stDataInfo.listStuInfo.size() != stDataInfo.listStuInfo.size())
    {
        dlog(LOG_TRACE, "[数据校验] -------- 信息列表为空，需同步");
        return OK;
    }

    PlatformManage_NS::DataInfo_S stTempInfo = m_stDataInfo;
    /* 班级信息不同时同步数据 */
    if (stTempInfo.stClassInfo.nId != stDataInfo.stClassInfo.nId || stTempInfo.stClassInfo.strName != stDataInfo.stClassInfo.strName || stTempInfo.listTeaInfo.size() != stDataInfo.listTeaInfo.size() || stTempInfo.listStuInfo.size() != stDataInfo.listStuInfo.size())
    {
        dlog(LOG_TRACE, "[数据校验] -------- 班级信息或信息列表大小改变，需同步");
        return OK;
    }

    for (const auto& info : stDataInfo.listTeaInfo)
    {
        if (!Md5Exists(stTempInfo.listTeaInfo, info.strMd5))
        {
            dlog(LOG_TRACE, "[数据校验] -------- 班级信息教师成员改变，需同步");
            return OK;
        }
    }

    for (const auto& info : stDataInfo.listStuInfo)
    {
        if (!Md5Exists(stTempInfo.listStuInfo, info.strMd5))
        {
            dlog(LOG_TRACE, "[数据校验] -------- 班级信息学生成员改变，需同步");
            return OK;
        }
    }

    return NOK;
}

BlError_E PlatformManage_NS::CPlatformManage::get_classInfo(DataInfo_S& stDataInfo)
{
    BlError_E enRetCode = OK;

    /* 获取班级信息 */
    enRetCode = get_humanInfo(stDataInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 获取班级信息失败");
        return ERR_GET_FAULT;
    }

    if(stDataInfo.listTeaInfo.size() == 0
        && stDataInfo.listStuInfo.size() == 0)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 获取班级信息为空");
        return ERR_AI_CLASS_EMPTY;
    }

    /* 数据校验 */
    enRetCode = check_dataChanged(stDataInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_INFO, "[获取教室信息] - 班级信息没有变化");
        return OK_EXIST;
    }

    /* 获取拖信息 */
    enRetCode = get_downloadPicInfo(stDataInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 获取图片信息失败");
        m_stDataInfo.clear();
        return ERR_GET_FAULT;
    }

    m_stDataInfo = stDataInfo;

    return enRetCode;
}

/* 获取教育云平台班级信息接口 */
BlError_E PlatformManage_NS::CPlatformManage::get_platformClassInfo(DataInfo_S& stDataInfo)
{
    BlError_E enRetCode = OK;

    /* 获取班级信息 */
    enRetCode = get_humanInfo(stDataInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 获取班级信息失败");
        return ERR_GET_FAULT;
    }

    /* 获取拖信息 */
    enRetCode = get_downloadPicInfo(stDataInfo);
    if (enRetCode < OK)
    {
        dlog(LOG_ERROR, "[获取教室信息] - 获取图片信息失败");
        m_stDataInfo.clear();
        return ERR_GET_FAULT;
    }

    m_stDataInfo = stDataInfo;

    return enRetCode;
}

/*获取班级成员信息*/
BlError_E PlatformManage_NS::CPlatformManage::get_humanInfo(PlatformManage_NS::DataInfo_S& stDataInfo)
{
    BlError_E enRetCode = OK;
    static int s_nNumber = 0;

    m_paramMutex.lock();
    std::string strIP = std::string(m_stInParam.stNeedParam.achIpAddr);
    m_paramMutex.unlock();

    CurlHttp::Get httpCtrl(strIP);

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
    enRetCode = m_pParseBase->parsePlatform(strResponse.c_str(), nError, strError);
    if (enRetCode != OK || nError != 200)
    {
        if (nError == 401 && s_nNumber <= 10)
        {
            s_nNumber++;
            /* 立即获取一下Token值 */
            enRetCode = PlatformManage_NS::CPlatformManage::get_tokenInfo();
            if (enRetCode < OK)
            {
                return ERR_GET_FAULT;
            }
            enRetCode = PlatformManage_NS::CPlatformManage::get_humanInfo(stDataInfo);
            return enRetCode;
        }
        dlog(LOG_ERROR, "返回异常-[%d]:[%s]", nError, strError.c_str());
        return ERR_GET_FAULT;
    }
    s_nNumber = 0;

    /* 解析参数 */
    enRetCode = m_pParseBase->parseClassInfo(strResponse.c_str(), stDataInfo);
    if (enRetCode != OK)
    {
        dlog(LOG_ERROR, "获取班级成员信息-解析数据失败");
        return enRetCode;
    }

    return enRetCode;
}

/* 平台-下载人脸图片信息 */
BlError_E PlatformManage_NS::CPlatformManage::get_downloadPicInfo(PlatformManage_NS::DataInfo_S& stDataInfo)
{
    BlError_E enRetCode = OK;
    static int s_nNumber = 0;

    m_paramMutex.lock();
    std::string strIP = std::string(m_stInParam.stNeedParam.achIpAddr);
    m_paramMutex.unlock();

    CurlHttp::Get httpCtrl(strIP);

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
    m_pParseBase->parsePlatform(strResponse.c_str(), nError, strError);
    if (nError != 200)
    {
        dlog(LOG_ERROR, "返回异常-[%d]:[%s]- strResponse = %s", nError, strError.c_str(), strResponse.c_str());
        if (nError == 401 && s_nNumber <= 10)
        {
            s_nNumber++;
            /* 立即获取一下Token值 */
            enRetCode = PlatformManage_NS::CPlatformManage::get_tokenInfo();
            if (enRetCode < OK)
            {
                return ERR_GET_FAULT;
            }
            enRetCode = PlatformManage_NS::CPlatformManage::get_downloadPicInfo(stDataInfo);
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
            m_stInParam.stNeedParam.achDownloadPath,
            m_strToken.c_str(),
            strDesPath.c_str(),
            strIP.c_str(),
            strLocalIp.c_str(),
            std::to_string(stDataInfo.stClassInfo.nId).c_str());
    m_paramMutex.unlock();
    dlog(LOG_TRACE, "[脚本指令]-chCmd = %s", chCmd);

    /* 调用system函数执行命令 */
    nRet = system(chCmd);

    if (nRet == 0)
    {
        std::string unzipPath = m_strPathBase + std::to_string(stDataInfo.stClassInfo.nId);

#ifdef SUPPORTS_CPP17
        if (!std::filesystem::exists(unzipPath) || !std::filesystem::is_directory(unzipPath))
        {
            dlog(LOG_ERROR, "文件不存在，或不是文件夹 [%s]", unzipPath.c_str());
            return ERR_NOT_EXIST;
        }

        /*文件遍历及其匹配*/
        for (const auto& entry : std::filesystem::directory_iterator(unzipPath))
        {
            if (entry.is_regular_file())
            {
                std::string strBaseFolder = m_strPathBase + std::to_string(stDataInfo.stClassInfo.nId).c_str() + std::string("/");
                std::cout << "[遍历文件名] : " << entry.path().filename() << std::endl;
                /* 获取文件拓展名 */
                std::filesystem::path filePath     = entry.path().filename();
                std::string           strFileExten = filePath.extension();
                for (const auto& stuInfo : stDataInfo.listStuInfo)
                {
                    /* 文件匹配 */
                    if (stuInfo.strFileName == entry.path().filename())
                    {
                        dlog(LOG_TRACE, "[班级信息]-stuInfo.strFileName = %s", stuInfo.strFileName.c_str());
                        /* 源文件 */
                        std::string strSourceFilePath = strBaseFolder + std::string(entry.path().filename());
                        /* 目标文件名 */
                        std::string strNewFile        = strBaseFolder + std::to_string(stuInfo.nId) + std::string("_") + stuInfo.strMd5 + strFileExten;

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
                        std::string strTargetFile   = strTargetFolder + std::string("/") + std::to_string(stuInfo.nId) + std::string("_") + stuInfo.strMd5 + strFileExten;
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

                        // dlog(LOG_TRACE, "%s->%s", strNewFile.c_str(), strTargetFile.c_str());
                        if (!CPublicFunc::resizeImage(strNewFile, strTargetFile, 1920, 1024))
                        {
                            dlog(LOG_ERROR, "修改图片大小并移入指定文件夹-失败");
                        }
                        /* 删除原始文件 */
                        std::filesystem::remove(strNewFile);
                    }
                }

                for (const auto& teaInfo : stDataInfo.listTeaInfo)
                {
                    /* 文件匹配 */
                    if (teaInfo.strFileName == entry.path().filename())
                    {
                        dlog(LOG_TRACE, "[班级信息]-teaInfo.strFileName = %s", teaInfo.strFileName.c_str());
                        /* 源文件 */
                        std::string strSourceFilePath = strBaseFolder + std::string(entry.path().filename());
                        /* 目标文件名 */
                        std::string strNewFile        = strBaseFolder + std::to_string(teaInfo.nId) + std::string("_") + teaInfo.strMd5 + strFileExten;

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
                        std::string strTargetFile   = strTargetFolder + std::string("/") + std::to_string(teaInfo.nId) + std::string("_") + teaInfo.strMd5 + strFileExten;
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
                        // dlog(LOG_TRACE, "%s->%s", strNewFile.c_str(), strTargetFile.c_str());
                        if (!CPublicFunc::resizeImage(strNewFile, strTargetFile, 1920, 1024))
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
            to_string(stDataInfo.stClassInfo.nId).c_str() +
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
                    for (const auto& stuInfo : stDataInfo.listStuInfo)
                    {
                        /* 文件匹配 */
                        if (stuInfo.strFileName == strFilename)
                        {
                            /* 源文件 */
                            std::string strSourceFilePath = strBaseFolder +
                                strFilename;
                            /* 目标文件名 */
                            std::string strNewFile = strBaseFolder +
                                std::string("student/") +
                                to_string(stuInfo.nId) +
                                std::string("_") +
                                stuInfo.strMd5 + strFileExten;

                            /* 修改图片大小并移入指定文件夹 */
                            if (!CPublicFunc::resizeImage(strSourceFilePath, strNewFile, 1920, 1024))
                            {
                                dlog(LOG_ERROR, "修改图片大小并移入指定文件夹-失败");
                            }
                            /* 删除原始文件 */
                            std::system(("rm " + std::string(strSourceFilePath)).c_str());

                            break;
                        }
                    }

                    /* 处理学生信息 */
                    for (const auto& stuInfo : stDataInfo.listTeaInfo)
                    {
                        /* 文件匹配 */
                        if (stuInfo.strFileName == strFilename)
                        {
                            /* 源文件 */
                            std::string strSourceFilePath = strBaseFolder +
                                strFilename;
                            /* 目标文件名 */
                            std::string strNewFile = strBaseFolder +
                                std::string("teacher/") +
                                to_string(stuInfo.nId) +
                                std::string("_") +
                                stuInfo.strMd5 + strFileExten;


                            /* 修改图片大小并移入指定文件夹 */
                            if (!CPublicFunc::resizeImage(strSourceFilePath, strNewFile, 1920, 1024))
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

        /* 压缩文件至class_id.tar.gz */
        std::string pathBase  = m_strPathBase + std::to_string(stDataInfo.stClassInfo.nId);
        stDataInfo.strTarPath = m_strPathBase + std::to_string(stDataInfo.stClassInfo.nId) + std::string(".tar.gz");

        sprintf(chTarCmd, "tar -cvf %s -C %s %d",
                stDataInfo.strTarPath.c_str(),
                m_strPathBase.c_str(),
                stDataInfo.stClassInfo.nId);

        dlog(LOG_TRACE, "[压缩指令]-chTarCmd = %s", chTarCmd);

        /* 调用system函数执行命令 */
        nRet = system(chTarCmd);
    }
    else
    {
        dlog(LOG_ERROR, "脚本执行失败- [脚本指令]-chCmd = %s", chCmd);
    }

    return enRetCode;
}

std::string PlatformManage_NS::CPlatformManage::get_pathBase()
{
    return m_strPathBase;
}
