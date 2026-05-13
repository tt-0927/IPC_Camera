#include "WsUpload.h"

#include "md5lib.h"
#include "dlog.h"
#include "Json.h"

void CWsUpload::store_param(lws *wsi, std::string param)
{
    m_storeParamMap[wsi] += param;
}

std::string CWsUpload::get_param(lws *wsi)
{
    return m_storeParamMap[wsi];
}

void CWsUpload::del_param(lws *wsi)
{
    m_storeParamMap.erase(wsi);
}

int CWsUpload::parse_param(struct lws *wsi)
{
    UploadInfo_S stUploadInfo;
    // 模拟接收Query参数
    char args[512] = {0};
    if (lws_get_urlarg_by_name(wsi, "filename=", args, sizeof(args)))
    {
        stUploadInfo.filename = std::string(args);
        if (stUploadInfo.filename.find('/') == std::string::npos) 
        {
            stUploadInfo.filename = m_filePath + stUploadInfo.filename;
        }
    }
    if (stUploadInfo.filename.empty())
    {
        return -1;
    }
    if (lws_get_urlarg_by_name(wsi, "chunk=", args, sizeof(args)))
    {
        stUploadInfo.nChunk = std::stoul(args);
    }
    if (lws_get_urlarg_by_name(wsi, "total_chunks=", args, sizeof(args)))
    {
        stUploadInfo.nChunks = std::stoul(args);
    }
    dlog_error("stUploadInfo.filename %s stUploadInfo.nChunks %d", stUploadInfo.filename.c_str(), stUploadInfo.nChunks);

    /* 组装分片文件名 */
    std::string tmpName = stUploadInfo.filename ;
    char *md5 = MD5EncData(const_cast<char *>(tmpName.c_str()), tmpName.length());
    if (md5 != NULL)
    {
        stUploadInfo.chunkFilename = m_filePath +  std::string(md5) + "_" + std::to_string(stUploadInfo.nChunk) + ".tmpart";
        stUploadInfo.chunkMd5 = md5;
        free(md5);
        md5 = NULL;
    }
    else
    {
        lwsl_err("md5 is nullptr\n");
    }

    if (!lws_get_urlarg_by_name(wsi, "size=", args, sizeof(args)))
    {
    }
    lwsl_warn("m_uploadInfoMap add %p stUploadInfo.chunkFilename %s\n", wsi, stUploadInfo.chunkFilename.c_str());
    m_uploadInfoMap[wsi].push_back(stUploadInfo);
    return 0;
}

int CWsUpload::parse_param(lws *wsi, const char *data, size_t nLen)
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
        m_upgradePackageMap[wsi] = stUpgradeInfo;
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
        char *md5 = MD5EncData(const_cast<char*>(tmpName.c_str()), tmpName.length());
        if (md5) {
            stUploadInfo.chunkFilename = m_filePath + std::string(md5) + "_" + std::to_string(stUploadInfo.nChunk) + ".tmpart";
            stUploadInfo.chunkMd5 = md5;
            free(md5);
        }

        stUploadInfo.nLen = 0;
        stUploadInfo.bUpload = false;
        m_uploadInfoMap[wsi].push_back(stUploadInfo);
        return 0;
    } 
    else if (strType == "stream_data")
    {
        return write_data(wsi, data, nLen);
    } 
    else if (strType == "stream_end")
    {
        auto itUpload = m_uploadInfoMap.find(wsi);
        if (itUpload != m_uploadInfoMap.end()) 
        {
            for (auto &info : itUpload->second) 
            {
                if (!info.bMerge) 
                {
                    // 所有分片接收完后才 merge
                    if (info.nLen == info.nFileSize && info.nChunks > 0)
                    {
                        merge(wsi);  // 调用 merge 函数处理所有分片
                        break;  
                    }
                }
            }
        }

        if (m_upgradePackageMap.find(wsi) != m_upgradePackageMap.end()) 
        {
            m_upgradePackageMap[wsi].filesSent++;
        }
    } 
    else if (strType == "upgrade_end")
    {
        if (m_upgradePackageMap.find(wsi) != m_upgradePackageMap.end())
            m_upgradePackageMap[wsi].inProgress = false;
        dlog_info("Upgrade completed for client %p", wsi);
        return 0;
    }
    else if (strType == "upload_complete")
    {
        merge(wsi);
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
        std::string tmpName = stUploadInfo.filename ;
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
            lwsl_err("md5 is nullptr\n");
        }
        
        lwsl_warn("wsi %p ; json %s stUploadInfo.chunkFilename  %s nLen %u\n", wsi, strData.c_str(), stUploadInfo.chunkFilename.c_str(), (int)nLen);
        if (m_uploadInfoMap[wsi].size() != 0 && m_uploadInfoMap[wsi].back().filename == stUploadInfo.filename)
        {
            stUploadInfo.nLen = m_uploadInfoMap[wsi].back().nLen;
            m_uploadInfoMap[wsi].back() = stUploadInfo;
        }
        else
        {
            m_uploadInfoMap[wsi].push_back(stUploadInfo);
        }
    }

    return 0;
}

int CWsUpload::write_data(struct lws *wsi, const char *data, size_t nLen)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(wsi, uploadInfos) < 0 || uploadInfos.empty())
    {
        lwsl_err("get_info err");

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
            lwsl_warn("stUploadInfo.chunkFilename.empty()\n");
            return -1;
        }

        if (stUploadInfo.bUpload)
        {
            lwsl_warn("stUploadInfo.bUpload %s\n", stUploadInfo.chunkFilename.c_str());
            return 0;
        }

        std::ofstream outfile(stUploadInfo.chunkFilename, std::ios::binary | std::ios::app);
        if (!outfile.is_open())
        {
            
            lwsl_warn("!outfile.is_open() %s\n", stUploadInfo.chunkFilename.c_str());
            return -1;
        }
        outfile.write(data, nLen);
        stUploadInfo.nLen += nLen;
        outfile.flush();
        outfile.close();
        break;
    }
    m_uploadInfoMap[wsi] = uploadInfos;
    return 0;
}
bool CWsUpload::is_eof(struct lws *wsi)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(wsi, uploadInfos) < 0 || uploadInfos.empty())
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
            lwsl_warn("已上传完成 stUploadInfo.nLen %d stUploadInfo.nChunks %d\n" , stUploadInfo.nLen, stUploadInfo.nChunks);
            return true;
        }
    }
    return false;
}
int CWsUpload::merge(struct lws *wsi)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(wsi, uploadInfos) < 0 || uploadInfos.empty())
    {
        lwsl_err("get_info err");

        return -1;
    }
    for (auto &stUploadInfo : uploadInfos)
    {
        if (stUploadInfo.bMerge)
        {
            continue;
        }
        lwsl_warn("开始合并文件");
        // 所有片段接收完毕，合并文件
        std::ofstream outFile(stUploadInfo.filename, std::ios::binary | std::ios::trunc);
        if (outFile.is_open())
        {
            for (size_t i = 0; i < stUploadInfo.nChunks; ++i)
            {
                std::stringstream chunkFilename;
                chunkFilename << m_filePath << stUploadInfo.chunkMd5 << "_" << i << ".tmpart";
                std::ifstream chunk_file(chunkFilename.str(), std::ios::binary);

                lwsl_warn("合并文件:%s", chunkFilename.str().c_str());
                if (chunk_file.is_open())
                {
                    outFile << chunk_file.rdbuf();
                    chunk_file.close();
                    std::remove(chunkFilename.str().c_str());
                }
                else
                {
                    lwsl_err("Failed to open chunk file: %s\n", chunkFilename.str().c_str());
                }
            }
            outFile.flush();
            outFile.close();
            lwsl_warn("File merged successfully: %s\n", stUploadInfo.filename.c_str());
        }
        else
        {
            lwsl_err("Failed to create merged file %s", stUploadInfo.filename.c_str());
        }
        stUploadInfo.bMerge = true;
        break;
    }
    m_uploadInfoMap[wsi] = uploadInfos;
    return 0;
}
int CWsUpload::get_info(struct lws *wsi, std::vector<UploadInfo_S> &uploadInfos)
{
    if (m_uploadInfoMap.find(wsi) == m_uploadInfoMap.end())
    {
        lwsl_err("get_info no find %p\n", wsi);
        return -1;
    }
    uploadInfos = m_uploadInfoMap.at(wsi);
    return 0;
}
void CWsUpload::erase(struct lws *wsi)
{
    auto iter =  m_uploadInfoMap.find(wsi);

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

std::string CWsUpload::get_upload_filename(struct lws *wsi)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(wsi, uploadInfos) < 0 || uploadInfos.empty())
    {
        return "";
    }
    return uploadInfos.back().filename;
}

int CWsUpload::get_progress(lws *wsi)
{
    // 优先处理升级包多文件情况
    // if (m_upgradePackageMap.find(wsi) != m_upgradePackageMap.end())
    // {
    //     auto &info = m_upgradePackageMap[wsi];
    //     if (info.totalFiles == 0) return 0;

    //     // 计算所有文件已接收字节总数
    //     std::vector<UploadInfo_S> uploadInfos;
    //     if (get_info(wsi, uploadInfos) < 0 || uploadInfos.empty())
    //         return 0;

    //     size_t totalReceived = 0;
    //     size_t totalSize = 0;
    //     for (auto &file : uploadInfos)
    //     {
    //         totalReceived += file.nLen;
    //         totalSize += file.nFileSize;
    //     }

    //     int progress = 0;
    //     if (totalSize > 0)
    //         progress = (int)(totalReceived * 100 / totalSize);

    //     if (progress > 99 && info.inProgress) progress = 99;
    //     return progress;
    // }

    // 兼容原有单文件上传逻辑
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(wsi, uploadInfos) < 0 || uploadInfos.empty())
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
        nProgress = (int)(stUploadInfo.nChunk + 1) * 100 / stUploadInfo.nChunks * nRecvFileNum / stUploadInfo.nTotalFiles;
    }
    else
    {
        nProgress = (int)(stUploadInfo.nChunk + 1) * 100 / stUploadInfo.nChunks;
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

std::string CWsUpload::get_progressStr(lws *wsi)
{
    /* 先检查wsi是否存在 */
    if (m_uploadInfoMap.find(wsi) == m_uploadInfoMap.end())
    {
        /* 如果不存在，返回空JSON或默认值 */
        Json::Object *pRootJson = Json::init();
        Json::add(pRootJson, "Progress", 0);
        std::string jsonString = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return jsonString;
    }

    int nProgress = get_progress(wsi);
    Json::Object *pRootJson = Json::init();
    Json::add(pRootJson, "Progress", nProgress);
    std::string jsonString = Json::to_string(pRootJson);
    Json::deinit(pRootJson);
    if (nProgress == 100)
    {
        erase(wsi);
    }
    return jsonString;
}
