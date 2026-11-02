/**
 * @file WsUpload.h
 * @brief websocket文件上传处理（evws 后端版本）
 *
 * 与 libwebsockets/WsUpload.h 的差异：
 * 1. 连接句柄由 struct lws* 改为 void*（evws_connection*）；
 * 2. URL query 参数由 EvwsServer 在握手前解析，经 parse_param(conn, map) 传入
 *    （evws 建立会话后 request 即被释放，无法在会话期再取 URI）。
 * 协议行为与 lws 版本保持一致：JSON 参数 + BINARY 数据顺序分片直写
 * .part 单文件（追加写），收齐后同目录原子 rename 落地；空间预检与
 * 写失败经进度通道回错误 JSON（Progress:-1）。
 */

#pragma once
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class CWsUpload
{
public:
    typedef struct UpgradePackage
    {
        int totalFiles = 0;
        int filesSent = 0;
        int totalSize = 0;
        bool inProgress = false;
    } UpgradePackage_S;
    typedef struct UploadInfo
    {
        int nChunk = 0;            /* 当前第几分片 */
        int nChunks = 0;           /* 总分片数 */
        std::string chunkFilename; /* 文件分片名 */
        std::string chunkMd5;      /* 分片的名前缀=md5值 */
        int nChunkSize = 0;        /* 分片大小 */
        std::string filename;      /* 文件真实名称 */
        int nFileSize = 0;         /* 文件大小 */
        int nTotalFiles = 1;       /* 总文件数 */
        int nLen = 0;
        bool bUpload = false;
        bool bMerge = false;
        bool operator<(const UploadInfo &other) const
        {
            return filename < other.filename;
        }
    } UploadInfo_S;

    void store_param(void *pConn, std::string param);
    std::string get_param(void *pConn);
    void del_param(void *pConn);
    /**
     * @brief Query参数解析（evws：握手前已解析好的 query map）
     * @param pConn evws连接句柄
     * @param mapQuery URI query 的 key/value 映射
     * @return int
     */
    int parse_param(void *pConn, const std::map<std::string, std::string> &mapQuery);
    /**
     * @brief 首条 TEXT 消息的参数解析
     * @param pConn evws连接句柄
     * @param data 数据
     * @param nLen 数据长度
     * @return int
     */
    int parse_param(void *pConn, const char *data, size_t nLen);
    /**
     * @brief 写数据到文件
     * @param pConn evws连接句柄
     * @param data 数据
     * @param nLen 数据长度
     * @return int
     */
    int write_data(void *pConn, const char *data, size_t nLen);
    /**
     * @brief 判断是否结束
     * @param pConn evws连接句柄
     * @return true
     * @return false
     */
    bool is_eof(void *pConn);
    /**
     * @brief 合并分片
     * @param pConn evws连接句柄
     * @return int
     */
    int merge(void *pConn);
    /**
     * @brief 删除上传信息
     * @param pConn evws连接句柄
     */
    void erase(void *pConn);
    /**
     * @brief 设置上传文件路径
     */
    void set_file_path(const std::string &strFilePath);
    /**
     * @brief 获取上传文件名称
     * @param pConn evws连接句柄
     * @return std::string
     */
    std::string get_upload_filename(void *pConn);
    int get_progress(void *pConn);
    std::string get_progressStr(void *pConn);

private:
    /**
     * @brief 获取上传信息对象
     * @param pConn evws连接句柄
     * @param uploadInfos
     * @return int
     */
    int get_info(void *pConn, std::vector<UploadInfo_S> &uploadInfos);
    /**
     * @brief 上传目录剩余空间预检（statvfs）
     * @param nNeedBytes 本次上传的文件大小
     * @return bool true=空间足够（查询失败不拦截，写入失败兜底）
     */
    bool check_disk_space(size_t nNeedBytes);
    /**
     * @brief 记录上传错误（经 get_progressStr 以 Progress:-1 下发网页）
     */
    void set_error(void *pConn, const std::string &strReason);
    /**
     * @brief 删除同目录其他待升级 .bin（新包 rename 成功后调用）
     * @param strKeepPath 本次上传成功的目标文件全路径（保留）
     */
    void cleanup_other_bins(const std::string &strKeepPath);

private:
    std::map<void *, std::vector<UploadInfo_S>> m_uploadInfoMap;
    std::map<void *, std::string> m_storeParamMap;
    std::map<void *, UpgradePackage_S> m_upgradePackageMap;
    /* 上传失败原因（no_space/write_error），优先于进度下发 */
    std::map<void *, std::string> m_errorMap;
    /**
     * @brief 上传的文件路径
     */
    std::string m_filePath;
};
