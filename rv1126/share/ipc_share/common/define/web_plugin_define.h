/**
 * @FilePath     : web_plugin_define.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-11 15:25:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-11 16:45:26
 * @Description  : PC端的插件配置参数定义
 */

#pragma once

#include <string>

namespace WebPlugin
{
    typedef struct
    {
        int nProtoType = 0;           /* 协议类型 */
        int nStreamType = 0;          /* 码流类型 */
        int nPerformance = 0;         /* 播放性能 */
        int nFrameRate = 25;          /* 自定义播放性能（帧率fps 1~25） */
        bool bEnableRuleInfo = false; /* 规则信息 */
        int nImageScale = 0;          /* 图像比例 */
        bool bEnableAutoPreview = 0;  /* 自动开启预览 */
        int nImageRecordType = 0;     /* 抓图格式 */
        std::string streamKey;        /* 码流密钥 */
    } StreamParam_S;

    typedef struct
    {
        std::string recordFile;       /* 录制文件路径 */
        std::string playbackDown;     /* 回放下载路径 */
        std::string previewSnapshot;  /* 预览抓图保存路径 */
        std::string playbackSnapshot; /* 回放抓图保存路径 */
        std::string playbackClip;     /* 回放剪辑保存路径 */
        std::string facialSnapshot;   /* 人脸抓拍保存路径 */
    } FileSavePath_S;

    typedef struct
    {
        int nRecordFileSize = 0; /* 录制文件打包大小 */
        FileSavePath_S stFileSavePath;
    } RecordParam_S;

    typedef struct
    {
        StreamParam_S stStreamParam;
        RecordParam_S stRecordParam;
    } Param_S;
}
