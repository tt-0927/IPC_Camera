#include "UploadThread.h"
#include "MaintenanceData.h"
#include "dlog.h"

#include <iostream>
#include <fstream>
#include <cstring>

using namespace MaintenanceNS;

CUploadThread::CUploadThread() : CMaintenanceThread()
{
}

bool CUploadThread::init()
{
    std::string strUrl = CMaintenanceData::getInstance()->getRequeryUrl();
    if (strUrl.empty())
    {
        dlog_error("CUploadThread::init, upload url is empty! Please initialize the configuration!");
        return false;
    }

    std::unique_lock<std::mutex> locker(m_mutex);
    if (m_pPost == nullptr)
    {
        m_pPost = new CurlHttp::CCurlMultipartHttpPost(strUrl);
    }
    else
    {
        m_pPost->set_path(strUrl);
    }
    m_isInit = true;
    return true;
}

bool CUploadThread::isInit()
{
    return m_isInit;
}

void CUploadThread::run()
{
    int nMSleep = REQUPLOAD_EMPTY_MSLEEP;
    while (m_bIsRunFlag.load())
    {
        /* 是否初始化 */
        if (m_isInit)
        {
            /* 当前是否已经登录 */
            if (CMaintenanceData::getInstance()->getLoginStatus())
            {
                /* 获取一条需要上传的记录 */
                RecordInfo stRecordInfo = CMaintenanceData::getInstance()->getNeedUploadFile();
                if (stRecordInfo.stFileInfo.strIdentifier.empty())
                {
                    /* 如果文件唯一标识为空说明当前没有可上传的文件 */
                    nMSleep = REQUPLOAD_EMPTY_MSLEEP;
                }
                else
                {
                    nMSleep = REQUPLOAD_MSLEEP;
                    if (!checkRecordInfo(stRecordInfo))
                    {
                        /* 上传失败了，让线程休眠久一点 */
                        nMSleep = REQUPLOAD_EMPTY_MSLEEP;
                    }
                }
            }
        }
        else
        {
            dlog_info("upload thread is not init!");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(nMSleep));
    }
}

bool CUploadThread::checkRecordInfo(MaintenanceNS::RecordInfo &stRecordInfo)
{
    std::string strUploadUrl;
    std::string strToken = CMaintenanceData::getInstance()->getToken();
    int nProjectID = CMaintenanceData::getInstance()->getProjectID();
    std::string strIdentifier = stRecordInfo.stFileInfo.strIdentifier;
    std::size_t nFileSize = stRecordInfo.stFileInfo.nFileSize;

    if (strToken.empty())
    {
        /* token为空，那么等他重新登录再说 */
        dlog_info("error: token is empty!");
        CMaintenanceData::getInstance()->setLoginStatus(false);
        return false;
    }

    if (nProjectID <= 0)
    {
        /* 项目唯一ID 小于等于 0，说明还没有获取到，等待获取 */
        dlog_info("warring: project id is empty!");
        /* TODO: 这里是否应该通知他从新请求获取项目列表接口呢？待定 */
        return false;
    }

    if (strIdentifier.empty() || nFileSize <= 0)
    {
        dlog_error("error: upload record info is empty!");
        /* 直接将当前文件标志成已上传 */
        CMaintenanceData::getInstance()->changedRecordUploadStatus(strIdentifier, UPLOADED);
        return false;
    }

    switch (stRecordInfo.stFileInfo.enFileType)
    {
    case FILE_TYPE_LOG:
        strUploadUrl.assign(REQ_UPLOAD_LOG_PATH);
        break;
    case FILE_TYPE_CONFIGURE:
        strUploadUrl.assign(REQ_UPLOAD_CONFIG_PATH);
        break;
    default:
        dlog_error("error: upload record info file type is empty!");
        dlog_error("error file identifier: %s", strIdentifier.c_str());
        /* 直接将当前文件标志成已上传 */
        CMaintenanceData::getInstance()->changedRecordUploadStatus(strIdentifier, UPLOADED);
        return false;
        break;
    }

    /* 将当前文件标志成上传中状态 */
    CMaintenanceData::getInstance()->changedRecordUploadStatus(strIdentifier, UPLOADING);

    /* 清空 */
    m_vecUploadData.clear();

    dlog_info("upload file size: %ld", nFileSize);

    /* 判断文件需不需要进行分片上传 */
    if (nFileSize > SLICE_SIZE)
    {
        /* 分片 */
        sliceFile(stRecordInfo);
    }
    else
    {
        /* 不分片 */
        UploadData stData;
        stData.stRecordInfo = stRecordInfo;
        stData.nSliceCount = 1;
        stData.nCurSlice = 1;
        stData.nCurSliceSize = nFileSize;
        stData.bUploadSliceFlag = false;
        m_vecUploadData.push_back(stData);
    }

    if (m_vecUploadData.size() <= 0)
    {
        /* 将当前文件标志成未上传状态 */
        CMaintenanceData::getInstance()->changedRecordUploadStatus(strIdentifier, UPLOAD_NOT);
        return false;
    }

    return sendUploadReq(strUploadUrl, strToken, nProjectID);
}

void CUploadThread::sliceFile(RecordInfo &stRecordInfo)
{
    int nCount = stRecordInfo.stFileInfo.nFileSize / SLICE_SIZE;
    int nLastBufferSize = stRecordInfo.stFileInfo.nFileSize % SLICE_SIZE;
    if (nLastBufferSize > 0)
    {
        nCount++;
    }

    /* 打开文件 */
    std::string strFilePath = stRecordInfo.stFileInfo.strFilePath + stRecordInfo.stFileInfo.strFileName;
    std::fstream inStream(strFilePath.c_str(), std::ios::in | std::ios::binary |
                                                   std::ios::out | std::ios::app);
    if (inStream.is_open())
    {
        int nBufferSize = 0;
        for (int i = 0; i < nCount; i++)
        {
            nBufferSize = SLICE_SIZE;
            if (nLastBufferSize > 0 && i == nCount - 1)
            {
                nBufferSize = nLastBufferSize;
            }
            char *pBuffer = new char[nBufferSize];
            memset(pBuffer, 0, nBufferSize);

            UploadData stData;
            stData.stRecordInfo = stRecordInfo;
            stData.nSliceCount = nCount;
            stData.nCurSlice = i + 1;
            stData.pSliceBuffer = pBuffer;
            stData.nCurSliceSize = nBufferSize;

            dlog_info("slice No.%d size:%d", i + 1, nBufferSize);

            if (!inStream.eof())
            {
                inStream.read(pBuffer, nBufferSize);
            }

            m_vecUploadData.push_back(stData);
        }

        inStream.close();
    }
    else
    {
        /* 打开文件失败 */
    }
}

bool CUploadThread::sendUploadReq(std::string strUploadUrl,
                                  std::string &strToken,
                                  int &nProjectID)
{
    if (m_vecUploadData.size() <= 0)
    {
        return false;
    }

    bool bRet = true;
    std::string strIdentifier;
    std::string strFullFilePath;

    for (std::size_t i = 0; i < m_vecUploadData.size(); i++)
    {
        /* 请求头列表 */
        std::list<std::string> arrHeader;

        /* 清空表单 */
        m_pPost->clear_form();
        /* 从新设置请求路径 */
        m_pPost->set_path(strUploadUrl);
        /* 添加请求头 */
        {
            /* Token */
            std::string strTmp;
            strTmp.assign("ApiToken:").append(strToken);
            arrHeader.push_back(strTmp);

            arrHeader.push_back("Accept-Encoding: gzip, deflate");
            arrHeader.push_back("Accept-Language: zh-CN,zh;q=0.9,en;q=0.8");
            arrHeader.push_back("Cache-Control: no-cache");
            arrHeader.push_back("Connection: keep-alive");

            /* 设置请求头 */
            m_pPost->set_header(arrHeader);
        }

        UploadData stData = m_vecUploadData.at(i);
        strIdentifier = stData.stRecordInfo.stFileInfo.strIdentifier;
        strFullFilePath = stData.stRecordInfo.stFileInfo.strFilePath + stData.stRecordInfo.stFileInfo.strFileName;

        if (m_vecUploadData.size() == 1)
        {
            /* 说明是不分片上传 */
            m_pPost->add_file_formData("file", strFullFilePath);
        }
        else
        {
            /* 分片上传 */
            if (stData.pSliceBuffer != nullptr)
            {
                /* 设置分片数据 */
                m_pPost->add_formData("file", stData.stRecordInfo.stFileInfo.strFileName,
                                      (void *)stData.pSliceBuffer, stData.nCurSliceSize);
            }
            else
            {
                dlog_error("error: file buffer is null!");
            }
        }
        m_pPost->add_formData("identifier", strIdentifier);
        m_pPost->add_formData("totalChunks", stData.nSliceCount);
        m_pPost->add_formData("chunkNumber", stData.nCurSlice);
        m_pPost->add_formData("currentChunkSize", stData.nCurSliceSize);
        m_pPost->add_formData("filename", stData.stRecordInfo.stFileInfo.strFileName);
        m_pPost->add_formData("name", strIdentifier);
        m_pPost->add_formData("project_id", nProjectID);

        dlog_info("\nupload file:%s\ntotal slice:%d\nslice:%d\n", strFullFilePath.c_str(),
                 stData.nSliceCount, stData.nCurSlice);

        std::string strResult;
        std::unique_lock<std::mutex> locker(m_mutex);
        int nRet = m_pPost->send_request();
        m_mutex.unlock();

        if (nRet == CURLE_OK)
        {
            m_pPost->get_recvData(strResult);
            ReqUploadResult result = m_cJson.parseUploadResult(strResult, bRet);

            dlog_info("upload result:%s\nresult code:%d", strResult.c_str(), result.stResult.nResult);

            if (bRet && result.stResult.nResult == SUCCESS)
            {
                /* 因为现在是单线程上传并且是按照顺序上传的，这里就不再检查列表中的上传状态 */
                /* 是否是最后一条数据上传，是最后一条上传说明上传已经成功 */
                if (i == m_vecUploadData.size() - 1)
                {
                    dlog_info("upload file:%s  --> success!", strFullFilePath.c_str());
                    bRet = true;
                    /* 修改上传文件的状态 */
                    CMaintenanceData::getInstance()->changedRecordUploadStatus(strIdentifier, UPLOADED);
                }
            }
            else if ((bRet && result.stResult.nResult == UNAUTHENTICATION) ||
                     (bRet && result.stResult.nResult == UNAUTHENTICATION_V2))
            {
                dlog_info("The token is invalid! Need ReLogin");
                bRet = false;
                /* 需要从新登录 */
                CMaintenanceData::getInstance()->setLoginStatus(false);
                break;
            }
            else
            {
                dlog_error("parse upload result fail! json:\n%s", strResult.c_str());
                /* 需要从新登录 */
                CMaintenanceData::getInstance()->setLoginStatus(false);
                bRet = false;
                break;
            }
        }
        else
        {
            bRet = false;
            strResult = m_pPost->get_error(nRet);
            dlog_error("upload error:%s", strResult.c_str());
            break;
        }

        if (bRet)
        {
            /* 循环体中休眠一下 */
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /* 只有分片上传才有分片Buffer，释放分片Buffer */
    if (m_vecUploadData.size() > 1)
    {
        dlog_info("free slice buffer...total:%ld", m_vecUploadData.size());
        /* 只有分片上传才有分片Buffer，释放分片Buffer */
        for (std::size_t i = 0; i < m_vecUploadData.size(); i++)
        {
            if (m_vecUploadData.at(i).pSliceBuffer != nullptr)
            {
                dlog_info("free [%ld] slice buffer.", i);
                delete[] m_vecUploadData[i].pSliceBuffer;
                m_vecUploadData[i].pSliceBuffer = nullptr;
            }
        }
        dlog_info("free slice buffer ok.");
    }

    /* 上传失败了 */
    if (!bRet)
    {
        /* 上传过程中出现错误，将当前文件修改上传状态为未上传 */
        CMaintenanceData::getInstance()->changedRecordUploadStatus(strIdentifier, UPLOAD_NOT);
    }

    /* 清空 */
    m_vecUploadData.clear();

    return bRet;
}
