/**
 * @file WsUpload.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-11-26
 * 
 * @brief websocket文件上传处理
 */

#pragma once
#include <string>
#include <libwebsockets.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

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
        int nChunk = 0;	            /* 当前第几分片 */
        int nChunks = 0;	        /* 总分片数 */
        std::string chunkFilename;	/* 文件分片名 */
        std::string chunkMd5;		/* 分片的名前缀=md5值 */
        int nChunkSize = 0;         /* 分片大小 */
        std::string filename;		/* 文件真实名称 */
        int nFileSize = 0;          /* 文件大小 */
        int nTotalFiles = 1;        /* 总文件数 */
        int nLen = 0;
        bool bUpload = false;
        bool bMerge = false;
        bool operator<(const UploadInfo& other) const
        {
            return filename < other.filename;  
        }
    } UploadInfo_S;

    void store_param(struct lws *wsi, std::string param);
    std::string get_param(struct lws *wsi);
    void del_param(struct lws *wsi);
    /**
     * @brief Query参数解析
     * @param wsi lws句柄
     * @return int 
     */
    int parse_param(struct lws *wsi);
    /**
     * @brief Query参数解析
     * @param wsi lws句柄
     * @return int 
     */
    int parse_param(struct lws *wsi, const char *data, size_t nLen);
    /**
     * @brief 写数据到文件
     * @param wsi lws句柄
     * @param data 数据
     * @param nLen 数据长度
     * @return int 
     */
    int write_data(struct lws *wsi, const char *data, size_t nLen);
    /**
     * @brief 判断是否结束
     * @param wsi 
     * @return true 
     * @return false 
     */
    bool is_eof(struct lws *wsi);
    /**
     * @brief 合并分片
     * @param wsi 
     * @return int 
     */
    int merge(struct lws *wsi);
    /**
     * @brief 删除上传信息
     * @param wsi 
     */
    void erase(struct lws *wsi);
    /**
     * @brief 设置上传文件路径
     */
    void set_file_path(const std::string &strFilePath);
    /**
     * @brief 获取上传文件名称
     * @param wsi ws客户端
     * @return std::string 
     */
    std::string get_upload_filename(struct lws *wsi);
    int get_progress(struct lws *wsi);
    std::string get_progressStr(struct lws *wsi);
private:
    /**
     * @brief 获取上传信息对象
     * @param wsi 
     * @param stUploadInfo 
     * @return int 
     */
    int get_info(struct lws *wsi, std::vector<UploadInfo_S> &uploadInfos);
private:
    std::map<struct lws *, std::vector<UploadInfo_S>> m_uploadInfoMap;
    std::map<struct lws *, std::string> m_storeParamMap;
    std::map<struct lws *, UpgradePackage_S> m_upgradePackageMap;
    /**
     * @brief 上传的文件路径
     */
    std::string m_filePath;  
};