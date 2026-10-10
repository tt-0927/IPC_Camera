/**
 * @FilePath     : posix_fs.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-23 15:07:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-23 15:19:49
 * @Description  : POSIX 文件系统操作兼容层（C++11 基线，替代 std::filesystem）
 */

#pragma once

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief 以 POSIX 接口提供 std::filesystem 的常用能力
 * @note  C++ 标准基线为 C++11（部分平台工具链不支持 C++17），
 *        本模块只做薄封装，错误通过返回值表达（不抛异常），
 *        路径一律使用 std::string，目录分隔符固定按 '/' 处理。
 */
namespace PosixFs_NS
{

/** 路径是否存在（符号链接目标存在即视为存在） */
inline bool exists(const std::string &strPath)
{
    return access(strPath.c_str(), F_OK) == 0;
}

/** 是否为目录 */
inline bool is_directory(const std::string &strPath)
{
    struct stat stPath;
    return (stat(strPath.c_str(), &stPath) == 0) && S_ISDIR(stPath.st_mode);
}

/** 是否为常规文件 */
inline bool is_regular_file(const std::string &strPath)
{
    struct stat stPath;
    return (stat(strPath.c_str(), &stPath) == 0) && S_ISREG(stPath.st_mode);
}

/** 删除单个文件或空目录，失败返回 false */
inline bool remove(const std::string &strPath)
{
    if (is_directory(strPath))
    {
        return rmdir(strPath.c_str()) == 0;
    }
    return unlink(strPath.c_str()) == 0;
}

/** 递归删除文件或整棵目录树（rm -rf 语义），任一节点失败返回 false 并尽量继续 */
inline bool remove_all(const std::string &strPath)
{
    struct stat stPath;
    if (lstat(strPath.c_str(), &stPath) != 0)
    {
        return false;
    }
    if (!S_ISDIR(stPath.st_mode))
    {
        return unlink(strPath.c_str()) == 0;
    }

    bool bAllRemoved = true;
    DIR *pDir = opendir(strPath.c_str());
    if (pDir == nullptr)
    {
        return false;
    }
    struct dirent *pEntry = nullptr;
    while ((pEntry = readdir(pDir)) != nullptr)
    {
        const std::string strName = pEntry->d_name;
        if (strName == "." || strName == "..")
        {
            continue;
        }
        if (!remove_all(strPath + "/" + strName))
        {
            bAllRemoved = false;
        }
    }
    closedir(pDir);
    if (rmdir(strPath.c_str()) != 0)
    {
        bAllRemoved = false;
    }
    return bAllRemoved;
}

/** 逐级创建目录（mkdir -p 语义），目录已存在视为成功 */
inline bool make_directories(const std::string &strPath)
{
    if (strPath.empty())
    {
        return false;
    }
    std::string strCur;
    for (size_t i = 0; i < strPath.size(); i++)
    {
        strCur.push_back(strPath[i]);
        if (strPath[i] == '/' && i > 0)
        {
            mkdir(strCur.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }
    }
    return mkdir(strPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0 || is_directory(strPath);
}

/** 取父目录路径（去掉最后一级），无 '/' 时返回空串 */
inline std::string parent_path(const std::string &strPath)
{
    const size_t unPos = strPath.rfind('/');
    return (unPos == std::string::npos) ? std::string() : strPath.substr(0, unPos);
}

/** 取最后一级文件名（不含目录部分） */
inline std::string filename(const std::string &strPath)
{
    const size_t unPos = strPath.rfind('/');
    return (unPos == std::string::npos) ? strPath : strPath.substr(unPos + 1);
}

/** 取后缀（含 '.'，基于文件名部分），无后缀返回空串 */
inline std::string extension(const std::string &strPath)
{
    const std::string strName = filename(strPath);
    const size_t unPos = strName.rfind('.');
    return (unPos == std::string::npos) ? std::string() : strName.substr(unPos);
}

/** 常规文件大小（字节），失败返回 -1 */
inline int64_t file_size(const std::string &strPath)
{
    struct stat stPath;
    if (stat(strPath.c_str(), &stPath) != 0 || !S_ISREG(stPath.st_mode))
    {
        return -1;
    }
    return static_cast<int64_t>(stPath.st_size);
}

/** 重命名/移动路径，失败返回 false（跨文件系统移动不在本接口职责内） */
inline bool rename_path(const std::string &strFrom, const std::string &strTo)
{
    return rename(strFrom.c_str(), strTo.c_str()) == 0;
}

/**
 * 列出目录下的一级条目名（不含 "." 和 ".."），失败返回 false
 * @note  返回的是文件名而非完整路径，调用方按需拼接
 */
inline bool list_dir(const std::string &strPath, std::vector<std::string> &vecNames)
{
    DIR *pDir = opendir(strPath.c_str());
    if (pDir == nullptr)
    {
        return false;
    }
    struct dirent *pEntry = nullptr;
    while ((pEntry = readdir(pDir)) != nullptr)
    {
        const std::string strName = pEntry->d_name;
        if (strName != "." && strName != "..")
        {
            vecNames.push_back(strName);
        }
    }
    closedir(pDir);
    return true;
}

/**
 * 递归收集目录树下所有条目的完整路径（含目录本身和文件，不含 "." 和 ".."）
 * @note  条目顺序不保证排序，路径形如 strPath + "/" + name 逐级拼接
 */
inline void recursive_list(const std::string &strPath, std::vector<std::string> &vecPaths)
{
    vecPaths.push_back(strPath);
    if (!is_directory(strPath))
    {
        return;
    }
    std::vector<std::string> vecNames;
    if (!list_dir(strPath, vecNames))
    {
        return;
    }
    for (size_t i = 0; i < vecNames.size(); i++)
    {
        recursive_list(strPath + "/" + vecNames[i], vecPaths);
    }
}

} // namespace PosixFs_NS
