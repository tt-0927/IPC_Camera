/*
 * @FilePath     : sdk_new/sdk_server/src/business/BG6_ZHSJ/BU_SJGZ/SjgzDomain.h
 * @Author       : chenchl
 * @Date         : 2026-08-20
 * @LastEditors  : chenchl
 * @LastEditTime : 2026-08-20
 * @Description  : SJGZ 配置域（视频/报警/AI/IPC，条件编译）
 *                 收口视频设备相关命令：视频流/OSD/图像参数、预览/对讲、报警输入输出、
 *                 移动侦测/越界/入侵/滞留/人员聚集等智能报警、人脸库/人脸抓拍、
 *                 AI 分析配置（安全帽/抽烟/打电话/摔倒/烟火/反光衣等）、4G/热点/WiFi/SD卡。
 *                 编译宏：未定义 BU_SJGZ_EXCLUDE 时编入，按 BG 群编译时可通过宏排除。
 *                 委托 CIpcBusiness 处理 IPC 专属命令（4G/热点/WiFi/SD卡）。
 */
#ifndef BU_SJGZ_EXCLUDE

#pragma once

#include "ConfigDomainBase.h"
#include "Singleton.h"

/**
 * SJGZ 配置域
 * @details 单例，注册视频/报警/AI 相关命令，查表分发。
 *          条件编译：未定义 BU_SJGZ_EXCLUDE 时编入。
 */
class CSjgzDomain : public CConfigDomainBase, public CSingleton<CSjgzDomain>
{
    friend class CSingleton<CSjgzDomain>;
    CSjgzDomain();
public:
    ~CSjgzDomain() {}
};

#endif /* BU_SJGZ_EXCLUDE */
