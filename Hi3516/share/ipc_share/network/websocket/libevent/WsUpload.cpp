/**
 * @file WsUpload.cpp
 * @brief websocket文件上传处理（evws 后端版本，协议行为对齐 libwebsockets/WsUpload.cpp）
 */

#include "WsUpload.h"

#include "md5lib.h"
#include "dlog.h"
#include "Json.h"
#include "WsQuery.h"

#include <sstream>
#include <unistd.h>

void CWsUpload::store_param(void *pConn, std::string param)
{
    m_storeParamMap[pConn] += param;
}

std::string CWsUpload::get_param(void *pConn)
{
    return m_storeParamMap[pConn];
}

void CWsUpload::del_param(void *pConn)
{
    m_storeParamMap.erase(pConn);
}

int CWsUpload::parse_param(void *pConn, const std::map<std::string, std::string> &mapQuery)
{
    UploadInfo_S stUploadInfo;
    std::string strValue;
    /* Query参数：filename（必填）/ chunk / total_chunks / size */
    if (WsQuery::get(mapQuery, "filename", strValue))
    {
        stUploadInfo.filename = strValue;
        if (stUploadInfo.filename.find('/') == std::string::npos)
        {
            stUploadInfo.filename = m_filePath + stUploadInfo.filename;
        }
    }
    if (stUploadInfo.filename.empty())
    {
        return -1;
    }
    if (WsQuery::get(mapQuery, "chunk", strValue))
    {
        stUploadInfo.nChunk = std::stoul(strValue);
    }
    if (WsQuery::get(mapQuery, "total_chunks", strValue))
    {
        stUploadInfo.nChunks = std::stoul(strValue);
    }
    dlog_info("stUploadInfo.filename %s stUploadInfo.nChunks %d", stUploadInfo.filename.c_str(), stUploadInfo.nChunks);

    /* 组装分片文件名 */
    std::string tmpName = stUploadInfo.filename;
    char *md5 = MD5EncData(const_cast<char *>(tmpName.c_str()), tmpName.length());
    if (md5 != NULL)
    {
        stUploadInfo.chunkFilename = m_filePath + std::string(md5) + "_" + std::to_string(stUploadInfo.nChunk) + ".tmpart";
        stUploadInfo.chunkMd5 = md5;
        free(md5);
        md5 = NULL;
    }
    else
    {
        dlog_error("md5 is nullptr");
    }

    if (!WsQuery::get(mapQuery, "size", strValue))
    {
        dlog_warn("upload size 参数缺失 conn %p", pConn);
    }
    dlog_info("m_uploadInfoMap add %p stUploadInfo.chunkFilename %s", pConn, stUploadInfo.chunkFilename.c_str());
    m_uploadInfoMap[pConn].push_back(stUploadInfo);
    return 0;
}

int CWsUpload::parse_param(void *pConn, const char *data, size_t nLen)
{
    if (!data)
    {
        return -1;
    }

    std::string strData(data, nLen);
    std::string strType;
    Json::get(strData.c_str(), "type", strType);

    if (strType == "upgrade_start")
    {
        UpgradePackage_S stUpgradeInfo;
        Json::get(strData.c_str(), "filesize", stUpgradeInfo.totalSize);
        Json::get(strData.c_str(), "totalFiles", stUpgradeInfo.totalFiles);
        stUpgradeInfo.filesSent = 0;
        stUpgradeInfo.inProgress = true;
        m_upgradePackageMap[pConn] = stUpgradeInfo;
        dlog_info("Upgrade started, totalSize=%zu totalFiles=%d", stUpgradeInfo.totalSize, stUpgradeInfo.totalFiles);
        return 0;
    }
    else if (strType == "stream_start")
    {
        UploadInfo_S stUploadInfo;
        Json::get(strData.c_str(), "fileName", stUploadInfo.filename);
        Json::get(strData.c_str(), "chunkIndex", stUploadInfo.nChunk);
        Json::get(strData.c_str(), "chunkSize", stUploadInfo.nChunkSize);
        Json::get(strData.c_str(), "totalChunks", stUploadInfo.nChunks);
        Json::get(strData.c_str(), "fileSize", stUploadInfo.nFileSize);

        if (stUploadInfo.filename.find('/') == std::string::npos)
            stUploadInfo.filename = m_filePath + stUploadInfo.filename;

        std::string tmpName = stUploadInfo.filename;
        char *md5 = MD5EncData(const_cast<char *>(tmpName.c_str()), tmpName.length());
        if (md5)
        {
            stUploadInfo.chunkFilename = m_filePath + std::string(md5) + "_" + std::to_string(stUploadInfo.nChunk) + ".tmpart";
            stUploadInfo.chunkMd5 = md5;
            free(md5);
        }

        stUploadInfo.nLen = 0;
        stUploadInfo.bUpload = false;
        m_uploadInfoMap[pConn].push_back(stUploadInfo);
        return 0;
    }
    else if (strType == "stream_data")
    {
        return write_data(pConn, data, nLen);
    }
    else if (strType == "stream_end")
    {
        auto itUpload = m_uploadInfoMap.find(pConn);
        if (itUpload != m_uploadInfoMap.end())
        {
            for (auto &info : itUpload->second)
            {
                if (!info.bMerge)
                {
                    // 所有分片接收完后才 merge
                    if (info.nLen == info.nFileSize && info.nChunks > 0)
                    {
                        merge(pConn); // 调用 merge 函数处理所有分片
                        break;
                    }
                }
            }
        }

        if (m_upgradePackageMap.find(pConn) != m_upgradePackageMap.end())
        {
            m_upgradePackageMap[pConn].filesSent++;
        }
    }
    else if (strType == "upgrade_end")
    {
        if (m_upgradePackageMap.find(pConn) != m_upgradePackageMap.end())
            m_upgradePackageMap[pConn].inProgress = false;
        dlog_info("Upgrade completed for client %p", pConn);
        return 0;
    }
    else if (strType == "upload_complete")
    {
        merge(pConn);
    }
    else
    {
        UploadInfo_S stUploadInfo;
        Json::get(strData.c_str(), "fileName", stUploadInfo.filename);
        Json::get(strData.c_str(), "chunkIndex", stUploadInfo.nChunk);
        Json::get(strData.c_str(), "chunkSize", stUploadInfo.nChunkSize);
        Json::get(strData.c_str(), "totalChunks", stUploadInfo.nChunks);
        Json::get(strData.c_str(), "fileSize", stUploadInfo.nFileSize);
        Json::get(strData.c_str(), "totalFiles", stUploadInfo.nTotalFiles);
        if (stUploadInfo.filename.find('/') == std::string::npos)
        {
            stUploadInfo.filename = m_filePath + stUploadInfo.filename;
        }
        stUploadInfo.nLen = 0;
        stUploadInfo.bUpload = false;
        if (stUploadInfo.filename.empty())
        {
            return -1;
        }
        /* 组装分片文件名 */
        std::string tmpName = stUploadInfo.filename;
        char *md5 = MD5EncData(const_cast<char *>(tmpName.c_str()), tmpName.length());
        if (md5 != NULL)
        {
            stUploadInfo.chunkFilename = m_filePath + std::string(md5) + "_" + std::to_string(stUploadInfo.nChunk) + ".tmpart";
            stUploadInfo.chunkMd5 = md5;
            free(md5);
            md5 = NULL;
            /* 判断文件是否存在 */
            if (access(stUploadInfo.chunkFilename.c_str(), F_OK) == 0)
            {
                stUploadInfo.bUpload = true;
            }
        }
        else
        {
            dlog_error("md5 is nullptr");
        }

        dlog_info("conn %p json %s stUploadInfo.chunkFilename %s nLen %d",
                  pConn,
                  strData.c_str(),
                  stUploadInfo.chunkFilename.c_str(),
                  (int) nLen);
        if (m_uploadInfoMap[pConn].size() != 0 && m_uploadInfoMap[pConn].back().filename == stUploadInfo.filename)
        {
            stUploadInfo.nLen = m_uploadInfoMap[pConn].back().nLen;
            m_uploadInfoMap[pConn].back() = stUploadInfo;
        }
        else
        {
            m_uploadInfoMap[pConn].push_back(stUploadInfo);
        }
    }

    return 0;
}

int CWsUpload::write_data(void *pConn, const char *data, size_t nLen)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(pConn, uploadInfos) < 0 || uploadInfos.empty())
    {
        dlog_error("get_info err");

        return -1;
    }
    for (auto &stUploadInfo : uploadInfos)
    {
        if (stUploadInfo.bMerge)
        {
            continue;
        }
        if (stUploadInfo.chunkFilename.empty())
        {
            dlog_warn("stUploadInfo.chunkFilename.empty()");
            return -1;
        }

        if (stUploadInfo.bUpload)
        {
            dlog_warn("stUploadInfo.bUpload %s", stUploadInfo.chunkFilename.c_str());
            return 0;
        }

        std::ofstream outfile(stUploadInfo.chunkFilename, std::ios::binary | std::ios::app);
        if (!outfile.is_open())
        {

            dlog_warn("!outfile.is_open() %s", stUploadInfo.chunkFilename.c_str());
            return -1;
        }
        outfile.write(data, nLen);
        stUploadInfo.nLen += nLen;
        outfile.flush();
        outfile.close();
        break;
    }
    m_uploadInfoMap[pConn] = uploadInfos;
    return 0;
}
bool CWsUpload::is_eof(void *pConn)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(pConn, uploadInfos) < 0 || uploadInfos.empty())
    {
        return -1;
    }
    for (auto &stUploadInfo : uploadInfos)
    {
        if (stUploadInfo.bMerge)
        {
            continue;
        }
        if (stUploadInfo.nChunk + 1 >= stUploadInfo.nChunks && stUploadInfo.nLen == stUploadInfo.nFileSize && stUploadInfo.nFileSize != 0)
        {
            dlog_info("已上传完成 stUploadInfo.nLen %d stUploadInfo.nChunks %d", stUploadInfo.nLen, stUploadInfo.nChunks);
            return true;
        }
    }
    return false;
}
int CWsUpload::merge(void *pConn)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(pConn, uploadInfos) < 0 || uploadInfos.empty())
    {
        dlog_error("get_info err");

        return -1;
    }
    for (auto &stUploadInfo : uploadInfos)
    {
        if (stUploadInfo.bMerge)
        {
            continue;
        }
        dlog_info("开始合并文件");
        // 所有片段接收完毕，合并文件
        std::ofstream outFile(stUploadInfo.filename, std::ios::binary | std::ios::trunc);
        if (outFile.is_open())
        {
            for (size_t i = 0; i < (size_t) stUploadInfo.nChunks; ++i)
            {
                std::stringstream chunkFilename;
                chunkFilename << m_filePath << stUploadInfo.chunkMd5 << "_" << i << ".tmpart";
                std::ifstream chunk_file(chunkFilename.str(), std::ios::binary);

                dlog_info("合并文件:%s", chunkFilename.str().c_str());
                if (chunk_file.is_open())
                {
                    outFile << chunk_file.rdbuf();
                    chunk_file.close();
                    std::remove(chunkFilename.str().c_str());
                }
                else
                {
                    dlog_error("Failed to open chunk file: %s", chunkFilename.str().c_str());
                }
            }
            outFile.flush();
            outFile.close();
            dlog_info("File merged successfully: %s", stUploadInfo.filename.c_str());
        }
        else
        {
            dlog_error("Failed to create merged file %s", stUploadInfo.filename.c_str());
        }
        stUploadInfo.bMerge = true;
        break;
    }
    m_uploadInfoMap[pConn] = uploadInfos;
    return 0;
}
int CWsUpload::get_info(void *pConn, std::vector<UploadInfo_S> &uploadInfos)
{
    if (m_uploadInfoMap.find(pConn) == m_uploadInfoMap.end())
    {
        dlog_error("get_info no find %p", pConn);
        return -1;
    }
    uploadInfos = m_uploadInfoMap.at(pConn);
    return 0;
}
void CWsUpload::erase(void *pConn)
{
    auto iter = m_uploadInfoMap.find(pConn);

    if (iter == m_uploadInfoMap.end())
    {
        return;
    }
    if (iter->second.empty())
    {
        return;
    }
    auto &stUploadInfo = iter->second.back();
    if (stUploadInfo.nChunk + 1 < stUploadInfo.nChunks)
    {
        std::remove(stUploadInfo.chunkFilename.c_str());
    }
    m_uploadInfoMap.erase(iter);
}

void CWsUpload::set_file_path(const std::string &strFilePath)
{
    m_filePath = strFilePath;
}

std::string CWsUpload::get_upload_filename(void *pConn)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(pConn, uploadInfos) < 0 || uploadInfos.empty())
    {
        return "";
    }
    return uploadInfos.back().filename;
}

int CWsUpload::get_progress(void *pConn)
{
    // 兼容原有单文件上传逻辑
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(pConn, uploadInfos) < 0 || uploadInfos.empty())
    {
        return 0;
    }
    int nRecvFileNum = 0;
    for (auto &stUploadInfo : uploadInfos)
    {
        if (stUploadInfo.bMerge)
        {
            nRecvFileNum++;
        }
    }
    auto &stUploadInfo = uploadInfos.back();
    int nProgress = 0;
    if (stUploadInfo.nTotalFiles > 1)
    {
        nProgress = (int) (stUploadInfo.nChunk + 1) * 100 / stUploadInfo.nChunks * nRecvFileNum / stUploadInfo.nTotalFiles;
    }
    else
    {
        nProgress = (int) (stUploadInfo.nChunk + 1) * 100 / stUploadInfo.nChunks;
    }
    if (nRecvFileNum == stUploadInfo.nTotalFiles)
    {
        nProgress = 100;
    }
    else if (nProgress > 99)
    {
        nProgress = 99;
    }
    return nProgress;
}

std::string CWsUpload::get_progressStr(void *pConn)
{
    /* 先检查连接是否存在 */
    if (m_uploadInfoMap.find(pConn) == m_uploadInfoMap.end())
    {
        /* 如果不存在，返回空JSON或默认值 */
        Json::Object *pRootJson = Json::init();
        Json::add(pRootJson, "Progress", 0);
        std::string jsonString = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return jsonString;
    }

    int nProgress = get_progress(pConn);
    Json::Object *pRootJson = Json::init();
    Json::add(pRootJson, "Progress", nProgress);
    std::string jsonString = Json::to_string(pRootJson);
    Json::deinit(pRootJson);
    if (nProgress == 100)
    {
        erase(pConn);
    }
    return jsonString;
}
