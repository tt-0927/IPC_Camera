#include "DataComm.hpp"

#include "ConvertInterface.h"
#include "ConvertJson.hpp"
#include "edukit_port.h"
#include "ini_disposed.h"
#include "Intern.hpp"
#include "ToolFunc.hpp"

using namespace Ai0630_NS;

#ifndef AI_SERVER_INI
    #define AI_SERVER_INI ("/opt/bl/.config/design_data/aiServer.ini")
#endif

DataComm::DataComm()
{
    std::string strIp;

    CIni ini(AI_SERVER_INI);
    ini.read("SERVER", "IP", strIp);
    /* 初始化TCP通讯 */
    init(strIp);
}

DataComm::~DataComm()
{
    disconnect(&sig_sendFeatureData);
    disconnect(&sig_sendData);

    unInit();
}

/* 接收任意数据并发送 */
void DataComm::recvData(
    char* pchData,
    int   nDataLen,
    int   nCode,
    int   nRecordTime,
    int   nClassId,
    bool  bSend)
{
    if (!m_pComm)
    {
        dlog(LOG_ERROR, "【数据通讯】通讯模块未初始化");
        return;
    }

    if (!pchData || nDataLen <= 0)
    {
        dlog(LOG_ERROR, "【数据通讯】输入数据为空");
        return;
    }

#if 0
    cv::Mat img = cv::imread("/root/test.png", cv::IMREAD_COLOR);
    if (img.empty())
    {
        dlog(LOG_ERROR, "【数据通讯】测试图片读取失败");
        return;
    }

    std::vector<uchar> jpegBuf;
    if (!encodeJpeg(img, jpegBuf))
    {
        dlog(LOG_ERROR, "【数据通讯】JPEG 编码失败");
        return;
    }

    pchData  = (char*)jpegBuf.data();
    nDataLen = jpegBuf.size();
#endif

    HeaderInfo_S stHeader;
    stHeader.clear();
    stHeader.nCode            = nCode;
    stHeader.bNeedPostProcess = false;

    std::string strHeader = to_string(stHeader);

    UserHeaderInfo_S stUserHeaderInfo;
    stUserHeaderInfo.nClassId   = nClassId;
    stUserHeaderInfo.nClassTime = nRecordTime;
    stUserHeaderInfo.lTimestamp = ToolFunc::getTimeStampUs();

    /* 保存图片逻辑 */
    /* 学生个人分析 */
    if (toInt(CommCode_E::AI_COM_ST_ANALYSE) == nCode)
    {
        /* 保存截图文件文件 */

        /* 创建目录 */
        ToolFunc::makeDirectory(GANCIAN_PICTURE_TEMP_PATH);

        /* 保存 */
        char achFilePath[1024] = { 0 };
        snprintf(achFilePath, sizeof(achFilePath), "%s/st_%lld.jpg", GANCIAN_PICTURE_TEMP_PATH, stUserHeaderInfo.lTimestamp);
        ToolFunc::writeDataToFile(achFilePath, pchData, nDataLen);
    }
    /* 老师个人分析 */
    else if (toInt(CommCode_E::AI_COM_TE_ANALYSE) == nCode)
    {
        /* 保存截图文件文件 */

        /* 创建目录 */
        ToolFunc::makeDirectory(GANCIAN_PICTURE_TEMP_PATH);

        /* 保存 */
        char achFilePath[1024] = { 0 };
        snprintf(achFilePath, sizeof(achFilePath), "%s/te_%lld.jpg", GANCIAN_PICTURE_TEMP_PATH, stUserHeaderInfo.lTimestamp);
        ToolFunc::writeDataToFile(achFilePath, pchData, nDataLen);
    }

    CommData_S* pstData = buildCommPacket(
        strHeader,
        pchData,
        nDataLen,
        sizeof(UserHeaderInfo_S),
        &stUserHeaderInfo);
    if (!pstData)
    {
        dlog(LOG_ERROR, "【数据通讯】构建数据包失败");
        return;
    }

    bool bOk = sendPacket(nCode, pstData);

    dlog(bOk ? LOG_TRACE : LOG_ERROR,
         "【数据通讯】发送%s nCode[%d] nDataSize[%d]",
         bOk ? "成功" : "失败",
         nCode, pstData->size());

    delete[] pstData;
    pstData = nullptr;
}

/* 接受班级信息，用来发送给服务器进行特诊提取 */
void DataComm::recvClassData(ClassInfo_S stClassInfo)
{
    HeaderInfo_S stHeader;
    stHeader.clear();
    stHeader.nCode            = toInt(CommCode_E::AI_COM_FACE_FEATURE);
    stHeader.bNeedPostProcess = false;

    std::string strHeader = to_string(stHeader);
    int         nCode     = stHeader.nCode;

    auto processList = [&](const auto& list) {
        for (const auto& item : list)
        {
            UserFaceInfo_S stFaceInfo;
            FaceLibsInfoToUserFaceInfo(item, stFaceInfo);

            cv::Mat img = cv::imread(item.strLocalPicPath, cv::IMREAD_COLOR);
            if (img.empty())
            {
                continue;
            }

            sendFaceFeatureBoth(strHeader, stFaceInfo, img, nCode);
        }
    };

    processList(stClassInfo.listTeaInfo);
    processList(stClassInfo.listStuInfo);
}

/* 初始化TCP通讯 */
void DataComm::init(std::string strIp)
{
    if (m_stAiServerInfo.strIp == strIp)
    {

        dlog(LOG_INFO, "【数据通讯】 服务器IP地址没有变化 不用操作");
        return;
    }

    if (nullptr != m_pComm)
    {
        dlog(LOG_INFO, "【数据通讯】 服务器已被初始化, 进行反初始化");
        delete m_pComm;
        m_pComm = nullptr;
    }

    m_stAiServerInfo.strIp = strIp;
    CIni ini(AI_SERVER_INI);
    ini.write("SERVER", "IP", strIp);

    if (strIp.empty())
    {
        return;
    }
    m_stParam.stNeedParam.enType      = COMM_NS::COMM_SHARE_TCP;
    m_stParam.stNeedParam.strIP       = strIp;
    m_stParam.stNeedParam.nPort       = C_CONTROL_AI_STREAM;
    m_stParam.stNeedParam.bServerMode = false;

    m_stParam.stExParam.bAutoReconnect     = true;
    m_stParam.stExParam.nReconnectCount    = 0;
    m_stParam.stExParam.nReconnectInterval = 1000;

    m_stParam.stExParam.dataCallback = std::bind(
        &DataComm::dataCallback,
        this,
        std::placeholders::_1);
    m_stParam.stExParam.statusCallback = std::bind(
        &DataComm::statusCallback,
        this,
        std::placeholders::_1);
    m_stParam.stExParam.heartbeatCallback = std::bind(
        &DataComm::heartbeatCallback,
        this,
        std::placeholders::_1);

    m_pComm = new COMM_NS::CCommShareTcp(m_stParam);
    if (m_pComm)
    {
        dlog(LOG_INFO, "【数据通讯】 服务器初始化成功");
    }
    else
    {
        dlog(LOG_ERROR, "【数据通讯】 服务器初始化失败");
    }
}

/* 反初始化TCP通讯 */
BlError_E DataComm::unInit()
{
    if (m_pComm)
    {
        delete m_pComm;
        m_pComm = nullptr;
    }
    return OK;
}

/* 工具函数：JPEG 编码 */
bool DataComm::encodeJpeg(const cv::Mat& img, std::vector<uchar>& outJpeg)
{
    if (img.empty())
    {
        return false;
    }

    static std::vector<int> params = {
        cv::IMWRITE_JPEG_QUALITY, 100
    };

    return cv::imencode(".jpg", img, outJpeg, params);
}

/* 工具函数：构建 CommData_S 包 */
CommData_S* DataComm::buildCommPacket(
    const std::string& strHeader,
    const void*        pData,
    int                nDataLen,
    int                nUserSize,
    const void*        pUser)
{
    if (!pData || nDataLen <= 0)
    {
        return nullptr;
    }

    int nHeaderSize = strHeader.size();

    auto* pstData = (CommData_S*)new (std::nothrow) char[sizeof(CommData_S) +
                                                         nHeaderSize +
                                                         nUserSize +
                                                         nDataLen];

    if (!pstData)
    {
        return nullptr;
    }

    pstData->enHeaderFormat  = DataFormat_E::JSON;
    pstData->nHeaderSize     = nHeaderSize;
    pstData->enAiParamFormat = DataFormat_E::JSON;
    pstData->nAiParamSize    = 0;
    pstData->nUserParamSize  = nUserSize;
    pstData->enDataFormat    = DataFormat_E::JPEG;
    pstData->nDataSize       = nDataLen;

    char* p = pstData->data;

    memcpy(p, strHeader.c_str(), nHeaderSize);
    p += nHeaderSize;

    if (pUser && nUserSize > 0)
    {
        memcpy(p, pUser, nUserSize);
        p += nUserSize;
    }

    memcpy(p, pData, nDataLen);

    return pstData;
}

/* 工具函数：统一发送 */
bool DataComm::sendPacket(int nCode, CommData_S* pstData)
{
    if (!pstData || !m_pComm)
    {
        return false;
    }

    COMM_NS::SendDataInfo_S stSendInfo;
    stSendInfo.nCode     = nCode;
    stSendInfo.nDataSize = pstData->size();
    stSendInfo.pDate     = reinterpret_cast<char*>(pstData);

    return (m_pComm->send(stSendInfo) >= OK);
}

/* 发送一张（原图或镜像） */
bool DataComm::sendFaceFeatureOne(
    const std::string&    strHeader,
    const UserFaceInfo_S& stUserFaceInfo,
    const cv::Mat&        img,
    int                   nCode,
    const char*           logTag)
{
    if (img.empty())
    {
        dlog(LOG_ERROR, "【数据通讯】%s 图片为空 ID[%d]",
             logTag,
             stUserFaceInfo.nMemberId);
        return false;
    }

    std::vector<uchar> jpegBuf;
    if (!encodeJpeg(img, jpegBuf))
    {
        dlog(LOG_ERROR, "【数据通讯】%s JPEG 编码失败 ID[%d]",
             logTag,
             stUserFaceInfo.nMemberId);
        return false;
    }

    CommData_S* pstData = buildCommPacket(
        strHeader,
        jpegBuf.data(),
        jpegBuf.size(),
        sizeof(UserFaceInfo_S),
        &stUserFaceInfo);

    if (!pstData)
    {
        dlog(LOG_ERROR, "【数据通讯】构建数据包失败");
        return false;
    }

    bool bOk = sendPacket(nCode, pstData);

    if (bOk)
    {
        dlog(LOG_TRACE, "【数据通讯】%s 发送成功 ID[%d] nCode[%d]",
             logTag, stUserFaceInfo.nMemberId, nCode);
    }
    else
    {
        dlog(LOG_ERROR, "【数据通讯】%s 发送失败 ID[%d] nCode[%d]",
             logTag, stUserFaceInfo.nMemberId, nCode);
    }

    delete[] pstData;
    return bOk;
}

/* 发送原图 + 镜像 */
void DataComm::sendFaceFeatureBoth(
    const std::string&    strHeader,
    const UserFaceInfo_S& stUserFaceInfo,
    const cv::Mat&        img,
    int                   nCode)
{
    sendFaceFeatureOne(strHeader, stUserFaceInfo, img, nCode, "原图");

    // cv::Mat imgFlip;
    // cv::flip(img, imgFlip, 1);
    // sendFaceFeatureOne(strHeader, stUserFaceInfo, imgFlip, nCode, "镜像");
}

/* 回调函数-数据返回 */
BlError_E DataComm::dataCallback(COMM_NS::DataParam_S stInfo)
{
    if (nullptr == stInfo.pchMessege ||
        nullptr == stInfo.pHandle)
    {
        dlog(LOG_ERROR, "传入参数异常");
        return ERR_PARAM;
    }

    if (stInfo.nCode == toInt(CommCode_E::HEARTBEAT_STATUS))
    {
        /*心跳跳过*/
        return OK;
    }

    dlog(LOG_TRACE, "【数据通讯】 接收到的数据[%d]", stInfo.nCode);

    /* 校验数据 */
    CommData_S* pstRecvDataInfo = reinterpret_cast<CommData_S*>(stInfo.pchMessege);
    if (pstRecvDataInfo && pstRecvDataInfo->size() == stInfo.nMessegeLen)
    {
        if (!parse(stInfo.nCode, pstRecvDataInfo))
        {
            dlog(LOG_ERROR, "【数据通讯】 接收到的数据异常");
        }
    }
    else
    {
        dlog(LOG_ERROR, "【数据通讯】 接收到的数据异常");
    }

    return OK;
}

/* 回调函数-链接状态 */
BlError_E DataComm::statusCallback(COMM_NS::StatusParam_S stInfo)
{
    if (stInfo.enType == COMM_NS::SUCCESS)
    {
        m_stAiServerInfo.nNetStatus = 1;
        dlog(LOG_INFO, "回调函数-链接状态 [连接成功]");
    }
    else
    {
        m_stAiServerInfo.nNetStatus = 0;
        dlog(LOG_ERROR, "回调函数-链接状态 [连接失败]");
    }
    return OK;
}

/* 回调函数-心跳回调 */
BlError_E DataComm::heartbeatCallback(COMM_NS::HeartbeatParam_S stInfo)
{
    return OK;
}

/* 解析数据 */
bool DataComm::parse(int nCode, CommData_S* pCommData)
{
    if (!pCommData)
    {
        return false;
    }

    HeaderInfo_S     stHeaderInfo;
    UserHeaderInfo_S stUserHeaderInfo;
    FaceLibsInfo_S   stFaceLibsInfo;
    UserFaceInfo_S   stUserFaceInfo;

    /* 头数据 */
    if (pCommData->nHeaderSize > 0)
    {
        switch (pCommData->enHeaderFormat)
        {
            case DataFormat_E::JSON:
            {
                std::string strHeader(pCommData->data, pCommData->nHeaderSize);
                to_struct(strHeader, stHeaderInfo);
                break;
            }
            default:
            {
                dlog(LOG_ERROR, "【数据通讯】 待处理数据的头数据格式异常");
                return false;
            }
        }
    }

    /* 算法参数 */
    if (pCommData->nAiParamSize > 0)
    {
        size_t nOffset    = (size_t)pCommData->nHeaderSize;
        /* 必须的越界保护 */
        size_t nTotalNeed = nOffset + pCommData->nAiParamSize;
        if (nTotalNeed > (pCommData->size() - sizeof(CommData_S)))
        {
            dlog(LOG_ERROR, "【数据通讯】 待处理数据的算法参数越界，数据包异常");
            return false;
        }

        switch (pCommData->enAiParamFormat)
        {
            case DataFormat_E::JSON:
            {
                break;
            }
            default:
            {
                dlog(LOG_ERROR, "【数据通讯】 待处理数据的算法参数格式异常");
                return false;
            }
        }
    }

    /* 用户参数，不需要处理返回的时候透传 */
    if (pCommData->nUserParamSize > 0)
    {
        size_t nOffset    = (size_t)pCommData->nHeaderSize + pCommData->nAiParamSize;
        /* 必须的越界保护 */
        size_t nTotalNeed = nOffset + pCommData->nUserParamSize;
        if (nTotalNeed > (pCommData->size() - sizeof(CommData_S)))
        {
            dlog(LOG_ERROR, "【数据通讯】 待处理数据的用户参数越界，数据包异常");
            return false;
        }

        switch (nCode)
        {
            /* 人脸识别 */
            case toInt(CommCode_E::AI_COM_FACE):
            /* 人头识别 */
            case toInt(CommCode_E::AI_COM_HEAD):
            /* 班级表情识别 */
            case toInt(CommCode_E::AI_COM_CLASS_EMO):
            /* 班级行为分析 */
            case toInt(CommCode_E::AI_COM_CLASS_BEHAVIOR):
            /* 学生个人分析 */
            case toInt(CommCode_E::AI_COM_ST_ANALYSE):
            /* 老师个人分析 */
            case toInt(CommCode_E::AI_COM_TE_ANALYSE):
            /* 课堂纪律 */
            case toInt(CommCode_E::AI_COM_DISCIPLINE):
            {
                std::memcpy(&stUserHeaderInfo,
                            pCommData->data + nOffset,
                            pCommData->nUserParamSize);
                break;
            }
            /* 人脸特征提取 */
            case toInt(CommCode_E::AI_COM_FACE_FEATURE):
            {
                std::memcpy(&stUserFaceInfo,
                            pCommData->data + nOffset,
                            pCommData->nUserParamSize);
                UserFaceInfoToFaceLibsInfo(stUserFaceInfo, stFaceLibsInfo);
                break;
            }
            default:
            {
                dlog(LOG_ERROR, "【数据通讯】 待处理数据的命令码异常");
                return false;
            }
        }
    }

    /* 数据内容 */
    if (pCommData->nDataSize > 0)
    {
        size_t nOffset = (size_t)pCommData->nHeaderSize +
            pCommData->nAiParamSize +
            pCommData->nUserParamSize;
        /* 必须的越界保护 */
        size_t nTotalNeed = nOffset + pCommData->nDataSize;
        if (nTotalNeed > (pCommData->size() - sizeof(CommData_S)))
        {
            dlog(LOG_ERROR, "【数据通讯】 待处理数据的图片数据越界，数据包异常");
            return false;
        }

        switch (nCode)
        {
            /* 人脸识别 */
            case toInt(CommCode_E::AI_COM_FACE):
            /* 人头识别 */
            case toInt(CommCode_E::AI_COM_HEAD):
            /* 班级表情识别 */
            case toInt(CommCode_E::AI_COM_CLASS_EMO):
            /* 班级行为分析 */
            case toInt(CommCode_E::AI_COM_CLASS_BEHAVIOR):
            /* 学生个人分析 */
            case toInt(CommCode_E::AI_COM_ST_ANALYSE):
            /* 老师个人分析 */
            case toInt(CommCode_E::AI_COM_TE_ANALYSE):
            /* 课堂纪律 */
            case toInt(CommCode_E::AI_COM_DISCIPLINE):
            {
                FaceResult_S stFaceResult;
                switch (pCommData->enDataFormat)
                {
                    case DataFormat_E::JSON:
                    {
                        std::string strData(pCommData->data + nOffset, pCommData->nHeaderSize + nTotalNeed);
                        to_struct(strData, stFaceResult);
                        sig_sendData.emit(stHeaderInfo, stUserHeaderInfo, stFaceResult);
                        break;
                    }
                    default:
                    {
                        dlog(LOG_ERROR, "【数据通讯】 待处理数据的数据内容格式异常");
                        return false;
                    }
                }
                break;
            }
            /* 人脸特征提取 */
            case toInt(CommCode_E::AI_COM_FACE_FEATURE):
            {
                FaceResult_S stFaceResult;
                switch (pCommData->enDataFormat)
                {
                    case DataFormat_E::JSON:
                    {
                        std::string strData(pCommData->data + nOffset, pCommData->nHeaderSize + nTotalNeed);
                        to_struct(strData, stFaceResult);
                        sig_sendFeatureData.emit(stHeaderInfo, stFaceLibsInfo, stFaceResult);
                        break;
                    }
                    default:
                    {
                        dlog(LOG_ERROR, "【数据通讯】 待处理数据的数据内容格式异常");
                        return false;
                    }
                }
                break;
            }
            default:
            {
                dlog(LOG_ERROR, "【数据通讯】 待处理数据的命令码异常");
                return false;
            }
        }
    }

    return true;
}