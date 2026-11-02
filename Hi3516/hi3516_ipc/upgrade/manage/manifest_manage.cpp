/**
 * @FilePath     : manifest_manage.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-10-19 15:10:00
 * @Description  : 升级文件清单管理（解析与逐文件 sha256 校验）
 */

#include "manifest_manage.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>
#include <string>
#include <vector>

#include "dlog.h"
#include "sha256.h"
#include "share_define.h"
#include "upgrade_define.h"

/* 逐文件校验的豁免路径：这两个文件解压后会被网络配置恢复流程覆盖回
 * 板上用户版本，内容必然与包内清单不一致，跳过校验只记日志 */
static const char *const MANIFEST_EXEMPT_PATHS[] = {
    "etc/resolv.conf",
    "etc/init.d/S80network",
};

/* 差集清理允许触碰的目录（绝对路径）：清单条目落在这些目录之外时拒绝删除，
 * 防止异常清单误删用户数据 */
/**
 * @brief   : 拼接安装根与相对路径，避免出现 "//"
 * @return   {std::string} 拼接结果
 */
static std::string join_install_path(const std::string &strRoot, const std::string &strRel)
{
    if (strRoot.empty() || strRoot == "/")
    {
        return "/" + strRel;
    }
    if (*strRoot.rbegin() == '/')
    {
        return strRoot + strRel;
    }
    return strRoot + "/" + strRel;
}

/**
 * @brief   : 从清单文件名提取 scope（manifest_all.txt -> all）
 * @return   {bool} true：名字合法且 scope 已输出
 */
static bool extract_scope_from_affix(const std::string &strName, const char *pchPrefix, const char *pchSuffix, std::string &strScope)
{
    size_t nPrefix = strlen(pchPrefix);
    size_t nSuffix = strlen(pchSuffix);

    if (strName.size() <= nPrefix + nSuffix || 0 != strName.compare(0, nPrefix, pchPrefix) ||
        0 != strName.compare(strName.size() - nSuffix, nSuffix, pchSuffix))
    {
        return false;
    }

    strScope = strName.substr(nPrefix, strName.size() - nPrefix - nSuffix);
    if (strScope.empty() || strScope.size() > 16)
    {
        return false;
    }
    /* scope 只允许小写字母与数字（打包类型 all/app/model/lib/custom/script） */
    for (size_t i = 0; i < strScope.size(); i++)
    {
        if (!islower((unsigned char) strScope[i]) || !isalnum((unsigned char) strScope[i]))
        {
            return false;
        }
    }
    return true;
}

static bool extract_scope_from_name(const std::string &strName, std::string &strScope)
{
    return extract_scope_from_affix(strName, MANIFEST_NAME_PREFIX, MANIFEST_NAME_SUFFIX, strScope);
}

/**
 * @brief   : 读取文件内容（带大小上限，防御异常大文件）
 * @return   {IpcRet_E} OK：成功，其他：读取失败或超限
 */
static IpcRet_E read_file_limited(const std::string &strPath, size_t nMaxSize, std::string &strOut)
{
    FILE *fp = fopen(strPath.c_str(), "rb");
    if (NULL == fp)
    {
        return ERR_OPEN;
    }

    char achBuf[4096];
    size_t nRead = 0;
    strOut.clear();
    while ((nRead = fread(achBuf, 1, sizeof(achBuf), fp)) > 0)
    {
        if (strOut.size() + nRead > nMaxSize)
        {
            fclose(fp);
            return ERR_PARAM;
        }
        strOut.append(achBuf, nRead);
    }

    bool bError = (0 != ferror(fp));
    fclose(fp);
    return bError ? ERR_FREAD : OK;
}

/**
 * @brief   : 校验清单条目相对路径的安全性
 * @return   {bool} true：路径合法（非绝对、无 .. 、无 . 、无空段、无结尾 /）
 */
static bool is_safe_relative_path(const std::string &strRel)
{
    if (strRel.empty() || '/' == strRel[0] || '/' == *strRel.rbegin())
    {
        return false;
    }

    size_t nStart = 0;
    while (nStart <= strRel.size())
    {
        size_t nPos = strRel.find('/', nStart);
        std::string strSeg = strRel.substr(nStart, (std::string::npos == nPos) ? std::string::npos : nPos - nStart);
        if (strSeg.empty() || strSeg == ".." || strSeg == ".")
        {
            return false;
        }
        if (std::string::npos == nPos)
        {
            break;
        }
        nStart = nPos + 1;
    }
    return true;
}
/**
 * @brief   : 判断相对路径是否属于逐文件校验豁免清单
 */
static bool is_exempt_verify_path(const std::string &strRel)
{
    for (size_t i = 0; i < sizeof(MANIFEST_EXEMPT_PATHS) / sizeof(MANIFEST_EXEMPT_PATHS[0]); i++)
    {
        if (strRel == MANIFEST_EXEMPT_PATHS[i])
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief   : 解析 META 行，校验格式版本、scope 与设备型号
 * @param    {const std::vector<std::string>&} vecTokens：META 行按空格切分后的键值对
 * @param    {const std::string&} strFileScope：从文件名提取的 scope
 * @return   {IpcRet_E} OK：通过，其他：不通过（fail-closed）
 */
static IpcRet_E check_manifest_meta(const std::vector<std::string> &vecTokens, const std::string &strFileScope)
{
    std::string strFormat;
    std::string strScope;
    std::string strDevice;
    std::string strVersion;

    for (size_t i = 0; i < vecTokens.size(); i++)
    {
        const std::string &strTok = vecTokens[i];
        size_t nEq = strTok.find('=');
        if (std::string::npos == nEq)
        {
            continue;
        }
        std::string strKey = strTok.substr(0, nEq);
        std::string strVal = strTok.substr(nEq + 1);
        if ("format" == strKey)
        {
            strFormat = strVal;
        }
        else if ("scope" == strKey)
        {
            strScope = strVal;
        }
        else if ("device" == strKey)
        {
            strDevice = strVal;
        }
        else if ("version" == strKey)
        {
            strVersion = strVal;
        }
    }

    /* 未知格式版本一律拒绝：解析不了的内容不允许安装 */
    if (strFormat != "1")
    {
        dlog_error("升级文件清单格式版本不支持 format[%s]", strFormat.c_str());
        return ERR_SYSTEM_UPGRADE;
    }
    if (strScope != strFileScope)
    {
        dlog_error("升级文件清单类型不匹配 文件名[%s] 清单内[%s]", strFileScope.c_str(), strScope.c_str());
        return ERR_SYSTEM_UPGRADE;
    }
    /* 型号二次核对（包头型号匹配检查在解压前已拦截一次，此处防手工构造的包） */
    if (!strDevice.empty() && strDevice != "*" && strDevice != DEVICE_CODE)
    {
        dlog_error("升级文件清单型号不匹配 包内[%s] 本机[%s]", strDevice.c_str(), DEVICE_CODE);
        return ERR_SYSTEM_UPGRADE;
    }

    dlog_info("升级文件清单：类型[%s] 版本[%s] 型号[%s]",
              strScope.c_str(),
              strVersion.c_str(),
              strDevice.empty() ? "未标注" : strDevice.c_str());
    return OK;
}

/**
 * @brief   : 解析清单内容为条目表
 * @return   {IpcRet_E} OK：成功，其他：任何一行不合法即失败（fail-closed）
 */
static IpcRet_E parse_manifest_content(const std::string &strRaw, const std::string &strFileScope, ManifestEntryMap_T &mapEntries)
{
    std::istringstream issInput(strRaw);
    std::string strLine;
    bool bMetaSeen = false;
    int nLine = 0;

    mapEntries.clear();

    while (std::getline(issInput, strLine))
    {
        nLine++;

        /* 兼容手工编辑带入的 \r */
        if (!strLine.empty() && '\r' == *strLine.rbegin())
        {
            strLine.erase(strLine.size() - 1);
        }
        if (strLine.empty())
        {
            continue;
        }

        if ('#' == strLine[0])
        {
            if (!bMetaSeen && 0 == strLine.compare(0, 6, "# META"))
            {
                std::istringstream issMeta(strLine.substr(6));
                std::vector<std::string> vecTokens;
                std::string strTok;
                while (issMeta >> strTok)
                {
                    vecTokens.push_back(strTok);
                }
                IpcRet_E enRet = check_manifest_meta(vecTokens, strFileScope);
                if (enRet != OK)
                {
                    return enRet;
                }
                bMetaSeen = true;
            }
            /* 其他注释行跳过 */
            continue;
        }

        if (!bMetaSeen)
        {
            dlog_error("升级文件清单缺少 META 首行 类型[%s]", strFileScope.c_str());
            return ERR_SYSTEM_UPGRADE;
        }

        /* 数据行：64 位十六进制 + 两个空格 + 相对路径 */
        if (strLine.size() < 66 || strLine[64] != ' ' || strLine[65] != ' ')
        {
            dlog_error("升级文件清单行格式错误 类型[%s] 行号[%d]", strFileScope.c_str(), nLine);
            return ERR_SYSTEM_UPGRADE;
        }

        std::string strHash = strLine.substr(0, 64);
        for (size_t i = 0; i < strHash.size(); i++)
        {
            if (!isxdigit((unsigned char) strHash[i]))
            {
                dlog_error("升级文件清单哈希非法 类型[%s] 行号[%d]", strFileScope.c_str(), nLine);
                return ERR_SYSTEM_UPGRADE;
            }
        }
        /* 统一小写后与计算值比对 */
        for (size_t i = 0; i < strHash.size(); i++)
        {
            strHash[i] = (char) tolower((unsigned char) strHash[i]);
        }

        std::string strPath = strLine.substr(66);
        if (!is_safe_relative_path(strPath))
        {
            dlog_error("升级文件清单路径非法 类型[%s] 路径[%s]", strFileScope.c_str(), strPath.c_str());
            return ERR_SYSTEM_UPGRADE;
        }
        if (mapEntries.find(strPath) != mapEntries.end())
        {
            dlog_error("升级文件清单路径重复 类型[%s] 路径[%s]", strFileScope.c_str(), strPath.c_str());
            return ERR_SYSTEM_UPGRADE;
        }

        mapEntries[strPath] = strHash;

        if (mapEntries.size() > MANIFEST_ENTRY_MAX)
        {
            dlog_error("升级文件清单条目超限 类型[%s]", strFileScope.c_str());
            return ERR_SYSTEM_UPGRADE;
        }
    }

    if (!bMetaSeen)
    {
        dlog_error("升级文件清单缺少 META 首行 类型[%s]", strFileScope.c_str());
        return ERR_SYSTEM_UPGRADE;
    }
    return OK;
}

/**
 * @brief   : 逐文件计算 sha256 与清单比对
 * @return   {IpcRet_E} OK：全部一致（豁免文件跳过），其他：任一文件不一致
 */
static IpcRet_E verify_manifest_entries(const std::string &strInstallRoot, const ManifestEntryMap_T &mapEntries, int *pnExemptCount)
{
    char achHex[65];
    int nTotal = 0;
    int nExempt = 0;

    dlog_info("升级文件清单校验开始：文件[%d]个", (int) mapEntries.size());

    for (ManifestEntryMap_T::const_iterator it = mapEntries.begin(); it != mapEntries.end(); ++it)
    {
        nTotal++;

        if (is_exempt_verify_path(it->first))
        {
            nExempt++;
            dlog_info("升级文件清单：文件[%s] 属于恢复类配置，跳过校验", it->first.c_str());
            continue;
        }

        std::string strAbs = join_install_path(strInstallRoot, it->first);
        if (0 != sha256_file_hex(strAbs.c_str(), achHex))
        {
            dlog_error("升级文件清单校验失败：文件[%s] 读取失败", it->first.c_str());
            return ERR_SYSTEM_UPGRADE;
        }
        if (achHex != it->second)
        {
            dlog_error("升级文件清单校验失败：文件[%s] 内容与清单记录不一致", it->first.c_str());
            return ERR_SYSTEM_UPGRADE;
        }
    }

    if (NULL != pnExemptCount)
    {
        *pnExemptCount = nExempt;
    }
    dlog_info("升级文件清单校验通过：文件[%d]个 跳过恢复类配置[%d]个", nTotal, nExempt);
    return OK;
}
/**
 * @brief   : 列出清单目录下的全部清单文件名
 */
static bool list_manifest_files(const std::string &strDir, std::vector<std::string> &vecNames)
{
    DIR *pDir = opendir(strDir.c_str());
    if (NULL == pDir)
    {
        return false;
    }

    struct dirent *pstEntry = NULL;
    while (NULL != (pstEntry = readdir(pDir)))
    {
        std::string strName = pstEntry->d_name;
        if (strName == "." || strName == "..")
        {
            continue;
        }
        std::string strScope;
        if (extract_scope_from_name(strName, strScope))
        {
            vecNames.push_back(strName);
        }
    }
    closedir(pDir);
    return true;
}
IpcRet_E manifest_verify_package(const char *pchInstallRoot, const char *pchManifestDirRel)
{
    std::string strRoot = (pchInstallRoot == NULL) ? "/" : pchInstallRoot;
    std::string strDir = join_install_path(strRoot, pchManifestDirRel == NULL ? "" : pchManifestDirRel);

    std::vector<std::string> vecNames;
    if (!list_manifest_files(strDir, vecNames))
    {
        /* 解压后清单目录不存在：包内没有清单，走旧格式包行为 */
        dlog_info("升级包内无升级文件清单（旧格式包），跳过校验");
        return OK;
    }

    for (size_t i = 0; i < vecNames.size(); i++)
    {
        std::string strScope;
        extract_scope_from_name(vecNames[i], strScope);

        std::string strRaw;
        IpcRet_E enRet = read_file_limited(strDir + "/" + vecNames[i], MANIFEST_FILE_MAX, strRaw);
        if (enRet != OK)
        {
            dlog_error("读取升级文件清单失败 类型[%s]", strScope.c_str());
            return ERR_SYSTEM_UPGRADE;
        }

        ManifestEntryMap_T mapNew;
        enRet = parse_manifest_content(strRaw, strScope, mapNew);
        if (enRet != OK)
        {
            return enRet;
        }

        enRet = verify_manifest_entries(strRoot, mapNew, NULL);
        if (enRet != OK)
        {
            return enRet;
        }
    }

    return OK;
}

/* ===== 删除执行原语（网页目录对账使用，全套防御：受控目录、路径合法性、常规文件/软链、豁免清单、父目录软链穿透检查）===== */

/* 差集清理允许触碰的目录（绝对路径）：清单条目落在这些目录之外时拒绝删除，
 * 防止异常清单误删用户数据 */
static const char *const MANIFEST_CLEANUP_ROOTS[] = {
    "/opt/cam", "/bin", "/sbin", "/etc", "/var/www", "/home/admin", "/root",
};

/**
 * @brief   : 判断绝对路径是否落在清理允许范围内
 * @note    : 安装根为 "/" 时比对受控目录集（前缀比对校验边界字符，
 *            防止 /etc2 骗过 /etc 前缀）；安装根为子目录（如 AI 模型
 *            目录）时只允许该安装根内部
 */
static bool is_under_controlled_root(const std::string &strAbs, const std::string &strInstallRoot)
{
    if (strInstallRoot != "/")
    {
        return strAbs == strInstallRoot || (0 == strAbs.compare(0, strInstallRoot.size(), strInstallRoot) &&
                                            strAbs.size() > strInstallRoot.size() && '/' == strAbs[strInstallRoot.size()]);
    }

    for (size_t i = 0; i < sizeof(MANIFEST_CLEANUP_ROOTS) / sizeof(MANIFEST_CLEANUP_ROOTS[0]); i++)
    {
        std::string strRoot = MANIFEST_CLEANUP_ROOTS[i];
        if (strAbs == strRoot)
        {
            return true;
        }
        if (0 == strAbs.compare(0, strRoot.size(), strRoot) && strAbs.size() > strRoot.size() && '/' == strAbs[strRoot.size()])
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief   : 删除一个差集路径，并回收变空的父目录（限受控目录内）
 * @return   {bool} true：已删除或本就不存在
 */
static bool remove_diff_entry(const std::string &strInstallRoot, const std::string &strRel)
{
    if (!is_safe_relative_path(strRel))
    {
        dlog_warn("清理旧文件：路径非法，跳过[%s]", strRel.c_str());
        return false;
    }

    std::string strAbs = join_install_path(strInstallRoot, strRel);
    if (!is_under_controlled_root(strAbs, strInstallRoot))
    {
        dlog_warn("清理旧文件：路径在受控目录之外，跳过[%s]", strAbs.c_str());
        return false;
    }

    /* lstat 不跟随软链：软链本身可安全 unlink，目录与设备文件不动 */
    struct stat stFileInfo = {};
    if (lstat(strAbs.c_str(), &stFileInfo) != 0)
    {
        if (ENOENT == errno)
        {
            return true;
        }
        dlog_warn("清理旧文件：检查失败[%s] errno[%d]", strAbs.c_str(), errno);
        return false;
    }
    if (!S_ISREG(stFileInfo.st_mode) && !S_ISLNK(stFileInfo.st_mode))
    {
        dlog_warn("清理旧文件：不是普通文件，跳过[%s]", strAbs.c_str());
        return false;
    }

    /* 父目录软链穿透防御：路径中间某级目录若是软链（如 /usr/lib → /lib），
       unlink 实际作用于软链指向的真实位置，可能删掉受控目录之外、甚至
       本版刚解压落盘的文件。对目标所在目录做 realpath 解析，真实位置
       必须仍在受控目录内才允许删除 */
    {
        std::string strDir = strAbs;
        size_t nSlash = strDir.rfind('/');
        if (std::string::npos != nSlash && nSlash > 0)
        {
            strDir.resize(nSlash);
        }
        char achRealDir[PATH_MAX] = { 0 };
        if (realpath(strDir.c_str(), achRealDir) == NULL)
        {
            dlog_warn("清理旧文件：所在目录解析失败，跳过[%s] errno[%d]", strDir.c_str(), errno);
            return false;
        }
        if (!is_under_controlled_root(std::string(achRealDir), strInstallRoot))
        {
            dlog_warn("清理旧文件：所在目录经软链指向受控目录之外，跳过[%s → %s]", strDir.c_str(), achRealDir);
            return false;
        }
    }

    if (unlink(strAbs.c_str()) != 0 && ENOENT != errno)
    {
        dlog_warn("清理旧文件：删除失败[%s] errno[%d]", strAbs.c_str(), errno);
        return false;
    }

    /* 回收变空的父目录，逐级上溯，出到受控目录边界为止 */
    std::string strParent = strAbs;
    size_t nPos = strParent.rfind('/');
    if (std::string::npos != nPos && nPos > 0)
    {
        strParent.resize(nPos);
    }
    while (strParent.size() > 1 && is_under_controlled_root(strParent, strInstallRoot))
    {
        if (rmdir(strParent.c_str()) != 0)
        {
            /* 非空（ENOTEMPTY/EEXIST）属正常，其余记录后停止 */
            if (errno != ENOTEMPTY && errno != EEXIST && errno != ENOENT)
            {
                dlog_warn("清理旧文件：回收空目录失败[%s] errno[%d]", strParent.c_str(), errno);
            }
            break;
        }
        nPos = strParent.rfind('/');
        if (std::string::npos == nPos || 0 == nPos)
        {
            break;
        }
        strParent.resize(nPos);
    }
    return true;
}

/* ===== 网页目录全量对账 ===== */

/* var/www/html 顶层豁免：S81appinit 创建的 /opt 软链（网页经它访问
   /opt 下的抓图与录像），对账时保留 */
static bool is_web_exempt_entry(const std::string &strRelFromWebRoot)
{
    return ("opt" == strRelFromWebRoot);
}

static void cleanup_web_dir_recursive(const std::string &strInstallRoot,
                                      const std::string &strAbsDir,
                                      const std::string &strRelDir,
                                      const ManifestEntryMap_T &mapCurrent,
                                      int *pnRemoved,
                                      int *pnKept)
{
    DIR *pDir = opendir(strAbsDir.c_str());
    if (NULL == pDir)
    {
        return;
    }

    struct dirent *pstEntry = NULL;
    while (NULL != (pstEntry = readdir(pDir)))
    {
        std::string strName = pstEntry->d_name;
        if (strName == "." || strName == "..")
        {
            continue;
        }
        std::string strRel = strRelDir.empty() ? strName : strRelDir + "/" + strName;
        std::string strAbs = strAbsDir + "/" + strName;

        struct stat stEntry = {};
        if (lstat(strAbs.c_str(), &stEntry) != 0)
        {
            continue;
        }

        if (S_ISDIR(stEntry.st_mode))
        {
            cleanup_web_dir_recursive(strInstallRoot, strAbs, strRel, mapCurrent, pnRemoved, pnKept);
            /* 清单只记文件不记目录：内部文件对账后，变空的目录顺手回收
               （rmdir 对非空目录自然失败，本版仍在交付其中文件的目录不受影响） */
            rmdir(strAbs.c_str());
            continue;
        }

        if (is_web_exempt_entry(strRel))
        {
            (*pnKept)++;
            continue;
        }

        std::string strManifestRel = "var/www/html/" + strRel;
        if (mapCurrent.count(strManifestRel) > 0)
        {
            (*pnKept)++;
            continue;
        }

        /* 不在本版任何清单：删除（走差集清理同款全套防御） */
        if (remove_diff_entry(strInstallRoot, strManifestRel))
        {
            (*pnRemoved)++;
        }
        else
        {
            (*pnKept)++;
        }
    }
    closedir(pDir);
}

/**
 * @brief   : 网页目录全量对账（var/www/html 内不在任何清单中的文件删除）
 * @param    {const char*} pchInstallRoot：安装根
 * @param    {const char*} pchManifestDirRel：清单目录相对安装根路径
 * @return   {void} 删除失败仅记日志（少删方向安全）
 * @note    : 升级程序启动时调用（升级成功重启后即清理旧 html）。
 *            白名单 = 板上现存全部清单条目并集（设备当前交付全集）；
 *            板上无任何清单时跳过对账（防误清空）；安装根下无
 *            var/www/html 时自然跳过（AI 模型安装根不受影响）
 */
void manifest_reconcile_web_directory(const char *pchInstallRoot, const char *pchManifestDirRel)
{
    std::string strInstallRoot = (pchInstallRoot == NULL) ? "/" : pchInstallRoot;
    std::string strManifestDir = join_install_path(strInstallRoot, pchManifestDirRel == NULL ? "" : pchManifestDirRel);
    std::string strWebRoot = join_install_path(strInstallRoot, "var/www/html");
    struct stat stWeb = {};
    if (lstat(strWebRoot.c_str(), &stWeb) != 0 || !S_ISDIR(stWeb.st_mode))
    {
        return;
    }

    ManifestEntryMap_T mapCurrent;
    std::vector<std::string> vecNames;
    if (list_manifest_files(strManifestDir, vecNames))
    {
        for (size_t i = 0; i < vecNames.size(); i++)
        {
            std::string strScope;
            extract_scope_from_name(vecNames[i], strScope);
            std::string strRaw;
            if (read_file_limited(strManifestDir + "/" + vecNames[i], MANIFEST_FILE_MAX, strRaw) == OK)
            {
                ManifestEntryMap_T mapOne;
                if (parse_manifest_content(strRaw, strScope, mapOne) == OK)
                {
                    mapCurrent.insert(mapOne.begin(), mapOne.end());
                }
            }
        }
    }

    /* 启动时机的关键防御：板上无任何清单（尚未经历过带清单的升级）时，
       白名单为空会把整个 html 目录清空——必须跳过 */
    if (mapCurrent.empty())
    {
        dlog_info("板上无升级文件清单，跳过网页目录对账");
        return;
    }

    int nRemoved = 0;
    int nKept = 0;
    cleanup_web_dir_recursive(strInstallRoot, strWebRoot, "", mapCurrent, &nRemoved, &nKept);
    dlog_info("网页目录对账完成：保留[%d]个 删除[%d]个（不在清单中的一律删除，opt 软链豁免）", nKept, nRemoved);
}

