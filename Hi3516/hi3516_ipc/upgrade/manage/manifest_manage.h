/**
 * @FilePath     : manifest_manage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-10-19 15:10:00
 * @Description  : 升级文件清单管理（解析与逐文件 sha256 校验）
 */

#ifndef _UPGRADE_MANIFEST_MANAGE_H_
#define _UPGRADE_MANIFEST_MANAGE_H_

#include <map>
#include <string>
#include "IpcRet.h"

/* 一份清单的条目表：tar 内相对路径 -> sha256（64 位小写十六进制） */
typedef std::map<std::string, std::string> ManifestEntryMap_T;

/**
 * @brief   : 升级文件清单校验（解压后逐文件 sha256 与清单比对）
 * @param    {const char*} pchInstallRoot：安装根（APP 包为 "/"，AI 模型包为模型目录）
 * @param    {const char*} pchManifestDirRel：清单目录相对安装根路径
 * @return   {IpcRet_E} OK：全部一致或包内无清单（旧格式包），其他：校验失败
 * @note    : fail-closed：任一文件与清单不一致即整包失败，不重启。
 *            清理类机制（差集清理/delete-list/网页对账/冻结基线）暂缓启用，
 *            完整设计与实现存档于各仓 wip/upgrade-full 分支与任务档案。
 */
IpcRet_E manifest_verify_package(const char *pchInstallRoot, const char *pchManifestDirRel);

/**
 * @brief   : 网页目录全量对账（升级程序启动时调用，清理旧 html）
 * @note    : 白名单 = 板上全部清单条目并集；无清单时跳过（防误清空）；
 *            var/www/html 顶层 opt 软链豁免。安装根下无该目录时跳过。
 */
void manifest_reconcile_web_directory(const char *pchInstallRoot, const char *pchManifestDirRel);

#endif /* _UPGRADE_MANIFEST_MANAGE_H_ */
