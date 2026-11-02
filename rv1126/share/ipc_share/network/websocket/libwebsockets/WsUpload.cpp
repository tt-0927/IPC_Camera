/**
 * @file WsUpload.cpp
 * @brief websocket文件上传处理（libwebsockets 后端，与 evws 版同构）
 */

#include "WsUpload.h"

#include "md5lib.h"
#include "dlog.h"
#include "Json.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <unistd.h>

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
    char args[512] = { 0 };
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
    dlog_info("stUploadInfo.filename %s stUploadInfo.nChunks %d", stUploadInfo.filename.c_str(), stUploadInfo.nChunks);

    size_t nFileSize = 0;
    if (lws_get_urlarg_by_name(wsi, "size=", args, sizeof(args)))
    {
        nFileSize = std::stoul(args);
    }
    else
    {
        dlog_warn("upload size 参数缺失 wsi %p", wsi);
    }
    /* 空间预检：不足直接拒绝本次上传，错误经进度通道下发网页 */
    if (nFileSize > 0 && !check_disk_space(nFileSize))
    {
        dlog_error("上传空间不足，拒绝接收文件 %s 需 %zu 字节", stUploadInfo.filename.c_str(), nFileSize);
        set_error(wsi, "no_space");
        return -2;
    }

    /* 组装暂存文件名：顺序分片直写 .part 单文件，收齐后原子 rename */
    std::string tmpName = stUploadInfo.filename;
    char *md5 = MD5EncData(const_cast<char *>(tmpName.c_str()), tmpName.length());
    if (md5 != NULL)
    {
        stUploadInfo.chunkFilename = m_filePath + std::string(md5) + ".part";
        stUploadInfo.chunkMd5 = md5;
        free(md5);
        md5 = NULL;
        /* 首片到达即清掉历史残留的 .part（前端重传也从首片开始） */
        if (stUploadInfo.nChunk == 0)
        {
            std::remove(stUploadInfo.chunkFilename.c_str());
        }
    }
    else
    {
        dlog_error("md5 is nullptr");
    }

    dlog_info("m_uploadInfoMap add %p stUploadInfo.chunkFilename %s", wsi, stUploadInfo.chunkFilename.c_str());
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

        /* 空间预检：不足直接拒绝本次上传 */
        if (stUploadInfo.nFileSize > 0 && !check_disk_space((size_t) stUploadInfo.nFileSize))
        {
            dlog_error("上传空间不足，拒绝接收文件 %s 需 %d 字节", stUploadInfo.filename.c_str(), stUploadInfo.nFileSize);
            set_error(wsi, "no_space");
            return -2;
        }

        std::string tmpName = stUploadInfo.filename;
        char *md5 = MD5EncData(const_cast<char *>(tmpName.c_str()), tmpName.length());
        if (md5)
        {
            stUploadInfo.chunkFilename = m_filePath + std::string(md5) + ".part";
            stUploadInfo.chunkMd5 = md5;
            free(md5);
            /* 首片到达即清掉历史残留的 .part */
            if (stUploadInfo.nChunk == 0)
            {
                std::remove(stUploadInfo.chunkFilename.c_str());
            }
        }

        stUploadInfo.nLen = 0;
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
                        merge(wsi); // 调用 merge 函数处理所有分片
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
        if (stUploadInfo.filename.empty())
        {
            return -1;
        }

        /* 空间预检：首片时检查一次（同文件后续分片继承条目，不重复检查） */
        const bool bFirstChunk = !(m_uploadInfoMap[wsi].size() != 0 && m_uploadInfoMap[wsi].back().filename == stUploadInfo.filename);
        if (bFirstChunk && stUploadInfo.nFileSize > 0 && !check_disk_space((size_t) stUploadInfo.nFileSize))
        {
            dlog_error("上传空间不足，拒绝接收文件 %s 需 %d 字节", stUploadInfo.filename.c_str(), stUploadInfo.nFileSize);
            set_error(wsi, "no_space");
            return -2;
        }

        /* 组装暂存文件名：顺序分片直写 .part 单文件 */
        std::string tmpName = stUploadInfo.filename;
        char *md5 = MD5EncData(const_cast<char *>(tmpName.c_str()), tmpName.length());
        if (md5 != NULL)
        {
            stUploadInfo.chunkFilename = m_filePath + std::string(md5) + ".part";
            stUploadInfo.chunkMd5 = md5;
            free(md5);
            md5 = NULL;
            /* 首片到达即清掉历史残留的 .part（前端重传也从首片开始） */
            if (stUploadInfo.nChunk == 0)
            {
                std::remove(stUploadInfo.chunkFilename.c_str());
            }
        }
        else
        {
            dlog_error("md5 is nullptr");
        }

        dlog_info("conn %p json %s stUploadInfo.chunkFilename %s nLen %d",
                  wsi,
                  strData.c_str(),
                  stUploadInfo.chunkFilename.c_str(),
                  (int) nLen);
        if (!bFirstChunk)
        {
            if (stUploadInfo.nChunk == 0)
            {
                /* 同文件从首片重传：.part 已清空，长度从零重计 */
                stUploadInfo.nLen = 0;
            }
            else
            {
                /* 同文件后续分片：继承已收长度 */
                stUploadInfo.nLen = m_uploadInfoMap[wsi].back().nLen;
            }
            m_uploadInfoMap[wsi].back() = stUploadInfo;
        }
        else
        {
            m_uploadInfoMap[wsi].push_back(stUploadInfo);
        }
    }

    return 0;
}

int CWsUpload::write_data(lws *wsi, const char *data, size_t nLen)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(wsi, uploadInfos) < 0 || uploadInfos.empty())
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

        std::ofstream outfile(stUploadInfo.chunkFilename, std::ios::binary | std::ios::app);
        if (!outfile.is_open())
        {
            dlog_warn("!outfile.is_open() %s", stUploadInfo.chunkFilename.c_str());
            set_error(wsi, "write_error");
            return -2;
        }
        outfile.write(data, nLen);
        outfile.flush();
        const bool bWriteOk = !outfile.bad() && !outfile.fail();
        outfile.close();
        if (!bWriteOk)
        {
            /* 写入被截断（典型为空间满）：复查剩余空间区分原因。
               长度不记账，is_eof 判定不会虚满足 */
            struct statvfs stVfs;
            if (statvfs(m_filePath.c_str(), &stVfs) == 0 && (uint64_t) stVfs.f_bavail * stVfs.f_frsize < (uint64_t) nLen)
            {
                set_error(wsi, "no_space");
            }
            else
            {
                set_error(wsi, "write_error");
            }
            dlog_error("上传数据写盘失败 %s 本次 %zu 字节", stUploadInfo.chunkFilename.c_str(), nLen);
            return -2;
        }
        stUploadInfo.nLen += nLen;
        break;
    }
    m_uploadInfoMap[wsi] = uploadInfos;
    return 0;
}
bool CWsUpload::is_eof(lws *wsi)
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
            dlog_info("已上传完成 stUploadInfo.nLen %d stUploadInfo.nChunks %d", stUploadInfo.nLen, stUploadInfo.nChunks);
            return true;
        }
    }
    return false;
}
int CWsUpload::merge(lws *wsi)
{
    std::vector<UploadInfo_S> uploadInfos;
    if (get_info(wsi, uploadInfos) < 0 || uploadInfos.empty())
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
        /* 分片已按序直写 .part 单文件，同目录 rename 原子落地：
           中途断电不会残留"名字像完整包"的半个目标文件 */
        if (std::rename(stUploadInfo.chunkFilename.c_str(), stUploadInfo.filename.c_str()) != 0)
        {
            dlog_error("上传文件落地失败 %s -> %s", stUploadInfo.chunkFilename.c_str(), stUploadInfo.filename.c_str());
            set_error(wsi, "write_error");
            m_uploadInfoMap[wsi] = uploadInfos;
            return -2;
        }
        dlog_info("File uploaded successfully: %s", stUploadInfo.filename.c_str());
        stUploadInfo.bMerge = true;
        /* 升级包落地后清理同目录其他待升级 .bin，避免多包堆积 */
        cleanup_other_bins(stUploadInfo.filename);
        break;
    }
    m_uploadInfoMap[wsi] = uploadInfos;
    return 0;
}
int CWsUpload::get_info(lws *wsi, std::vector<UploadInfo_S> &uploadInfos)
{
    if (m_uploadInfoMap.find(wsi) == m_uploadInfoMap.end())
    {
        dlog_error("get_info no find %p", wsi);
        return -1;
    }
    uploadInfos = m_uploadInfoMap.at(wsi);
    return 0;
}
void CWsUpload::erase(lws *wsi)
{
    auto iter = m_uploadInfoMap.find(wsi);

    if (iter == m_uploadInfoMap.end())
    {
        return;
    }
    if (!iter->second.empty())
    {
        auto &stUploadInfo = iter->second.back();
        /* 未完成上传（未 rename 落地）的暂存 .part 一并清理；
           已落地的目标文件保留给升级/导入流程消费 */
        if (!stUploadInfo.bMerge && !stUploadInfo.chunkFilename.empty())
        {
            std::remove(stUploadInfo.chunkFilename.c_str());
        }
    }
    m_uploadInfoMap.erase(iter);
    m_errorMap.erase(wsi);
}

void CWsUpload::set_file_path(const std::string &strFilePath)
{
    m_filePath = strFilePath;
}

std::string CWsUpload::get_upload_filename(lws *wsi)
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

bool CWsUpload::check_disk_space(size_t nNeedBytes)
{
    struct statvfs stVfs;
    if (statvfs(m_filePath.c_str(), &stVfs) != 0)
    {
        /* 查询失败不拦截上传，由写入失败检测兜底 */
        dlog_warn("statvfs 查询失败 %s", m_filePath.c_str());
        return true;
    }
    const uint64_t nFree = (uint64_t) stVfs.f_bavail * stVfs.f_frsize;
    /* 预留 512KB 余量，避免贴着临界值放行 */
    return nFree >= (uint64_t) nNeedBytes + 512 * 1024;
}

void CWsUpload::set_error(lws *wsi, const std::string &strReason)
{
    m_errorMap[wsi] = strReason;
}

void CWsUpload::cleanup_other_bins(const std::string &strKeepPath)
{
    /* 只处理 .bin 升级包；参数文件/人脸图片等其他上传不受影响 */
    const std::string strKeepName = strKeepPath.substr(strKeepPath.find_last_of('/') + 1);
    if (strKeepName.size() <= 4 || strKeepName.compare(strKeepName.size() - 4, 4, ".bin") != 0)
    {
        return;
    }
    DIR *pDir = opendir(m_filePath.c_str());
    if (nullptr == pDir)
    {
        return;
    }
    struct dirent *pEntry;
    while ((pEntry = readdir(pDir)) != nullptr)
    {
        const std::string strName = pEntry->d_name;
        if (strName.size() <= 4 || strName.compare(strName.size() - 4, 4, ".bin") != 0)
        {
            continue;
        }
        if (strName == strKeepName || strName == "onvif_firmware.bin")
        {
            continue;
        }
        const std::string strFull = m_filePath + strName;
        struct stat stFile;
        if (lstat(strFull.c_str(), &stFile) != 0 || !S_ISREG(stFile.st_mode))
        {
            continue;
        }
        std::remove(strFull.c_str());
        dlog_info("删除遗留的待升级包 %s", strFull.c_str());
    }
    closedir(pDir);
}

std::string CWsUpload::get_progressStr(lws *wsi)
{
    /* 上传失败时优先下发错误（Progress:-1），前端不识别 Reason 也不影响
       既有进度渲染；错误在连接断开（erase）时清除 */
    auto itError = m_errorMap.find(wsi);
    if (itError != m_errorMap.end())
    {
        Json::Object *pRootJson = Json::init();
        Json::add(pRootJson, "Progress", -1);
        Json::add(pRootJson, "Reason", itError->second);
        std::string jsonString = Json::to_string(pRootJson);
        Json::deinit(pRootJson);
        return jsonString;
    }
    /* 先检查连接是否存在 */
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
