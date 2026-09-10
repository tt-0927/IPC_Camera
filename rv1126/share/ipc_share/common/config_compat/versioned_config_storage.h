/**
 * @FilePath     : versioned_config_storage.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-13 11:37:28
 * @Description  : 版本化配置存储模块
 */

#pragma once

#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include "Json.h"
#include "config_version.h"
#include "config_validator.h"
#include "config_migrator.h"

namespace ConfigCompat_NS
{

/* JSON中版本信息的键名 */
constexpr const char *VERSION_KEY = "_configVersion";
constexpr const char *CONFIG_TYPE_KEY = "_configType";

/**
 * @brief   : 从JSON中读取版本信息
 * @param    {Json::Object*} pRootJson：JSON对象
 * @param    {ConfigVersion_S} &version：输出版本
 * @return   {bool} 是否成功读取
 */
inline bool readVersionFromJson(Json::Object *pRootJson, ConfigVersion_S &version)
{
    std::string strVersion;
    if (!Json::get(pRootJson, VERSION_KEY, strVersion))
    {
        /* 无版本信息，视为最初版本 0.0.0 */
        version = ConfigVersion_S(0, 0, 0);
        return false;
    }
    return version.fromString(strVersion);
}

/**
 * @brief   : 向JSON中写入版本信息
 * @param    {Json::Object*} pRootJson：JSON对象
 * @param    {ConfigVersion_S} &version：版本
 * @param    {std::string} &configType：配置类型
 */
inline void writeVersionToJson(Json::Object* pRootJson,
                               const ConfigVersion_S& version,
                               const std::string& configType)
{
    Json::add(pRootJson, VERSION_KEY, version.toString());
    Json::add(pRootJson, CONFIG_TYPE_KEY, configType);
}

/**
 * @brief   : 版本化配置存储基类
 * @note    : 提供版本检测、迁移、验证的通用逻辑
 */
template <typename T,
          typename Migrator = CConfigMigrator<T>,
          typename Validator = CConfigValidator<T>>
class CVersionedStorageBase
{
protected:
    /**
     * @brief   : 读取并处理配置文件
     * @param    {std::string} &filePath：文件路径
     * @param    {T} &data：输出数据
     * @return   {int} 0：成功，-1：文件不存在或读取失败，1：迁移后成功
     */
    int readAndMigrate(const std::string &filePath, T &data)
    {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return -1;
        }

        std::ifstream::pos_type fileSize = file.tellg();
        if (fileSize == 0)
        {
            file.close();
            return -1;
        }

        file.seekg(0, std::ios::beg);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string jsonData = buffer.str();
        Json::Object *pRootJson = Json::init(jsonData);
        if (!pRootJson)
        {
            return -1;
        }

        /* 读取文件版本 */
        ConfigVersion_S fileVersion;
        readVersionFromJson(pRootJson, fileVersion);

        ConfigVersion_S currentVersion = Migrator::getCurrentVersion();
        CompatResult_E compatResult = checkCompatibility(fileVersion, currentVersion);

        bool needRewrite = false;
        int result = 0;

        switch (compatResult)
        {
        case CompatResult_E::COMPATIBLE:
            /* 完全兼容，直接解析 */
            break;

        case CompatResult_E::NEED_MIGRATION:
        case CompatResult_E::NEED_VALIDATION:
            {
                /* 执行迁移 */
                MigrateResult_E migrateResult = Migrator::migrate(pRootJson, fileVersion, currentVersion);
                if (migrateResult == MigrateResult_E::FAILED)
                {
                    /* 迁移失败，重置为默认值 */
                    data = getDefaultData();
                    Json::deinit(pRootJson);
                    return 1;
                }
                needRewrite = true;
                result = 1;
            }
            break;

        case CompatResult_E::INCOMPATIBLE:
        case CompatResult_E::VERSION_TOO_NEW:
            /* 不兼容或版本过新，重置为默认值 */
            data = getDefaultData();
            Json::deinit(pRootJson);
            return 1;
        }

        /* 解析数据（使用现有的Convert框架） */
        parseFromJson(pRootJson, data);
        Json::deinit(pRootJson);

        /* 验证并修复数据 */
        ConfigValidateResult_S validateResult = Validator::validate(data);
        if (validateResult.enOverallResult == ValidateResult_E::FIXED)
        {
            needRewrite = true;
            result = 1;
        }
        else if (validateResult.enOverallResult == ValidateResult_E::INVALID)
        {
            /* 无法修复，重置为默认值 */
            data = getDefaultData();
            return 1;
        }

        /* 如果有修改，重新写入文件 */
        if (needRewrite)
        {
            writeToFile(filePath, data);
        }

        return result;
    }

    /**
     * @brief   : 写入配置文件（带版本信息）
     * @param    {std::string} &filePath：文件路径
     * @param    {T} &data：数据
     * @return   {int} 0：成功，非0：失败
     */
    int writeToFile(const std::string &filePath, const T &data)
    {
        Json::Object *pRootJson = Json::init();
        if (!pRootJson)
        {
            return -1;
        }

        /* 写入版本信息 */
        writeVersionToJson(pRootJson, Migrator::getCurrentVersion(), Migrator::getConfigType());

        /* 序列化数据 */
        serializeToJson(pRootJson, data);

        std::string jsonData = Json::to_string(pRootJson);
        Json::deinit(pRootJson);

        /* 原子写入 */
        std::string tmpPath = filePath + ".tmp";
        {
            std::ofstream file(tmpPath, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open())
            {
                return -1;
            }
            file << jsonData << std::endl;
            file.flush();
            file.close();

            int fd = ::open(tmpPath.c_str(), O_WRONLY);
            if (fd != -1)
            {
                ::fsync(fd);
                ::close(fd);
            }
        }

        ::rename(tmpPath.c_str(), filePath.c_str());

        /* 同步目录 */
        std::string::size_type pos = filePath.rfind('/');
        if (pos != std::string::npos)
        {
            std::string dir = filePath.substr(0, pos);
            int dirFd = ::open(dir.c_str(), O_DIRECTORY | O_RDONLY);
            if (dirFd != -1)
            {
                ::fsync(dirFd);
                ::close(dirFd);
            }
        }

        return 0;
    }

    /**
     * @brief   : 获取默认数据（由子类实现或使用SFINAE检测）
     * @return   {T} 默认数据
     */
    virtual T getDefaultData()
    {
        return getDefaultDataImpl(hasCreateDefaultRule<T>{});
    }

    /**
     * @brief   : 从JSON解析数据（需要包含convert框架后实现）
     */
    virtual void parseFromJson(Json::Object *pRootJson, T &data) = 0;

    /**
     * @brief   : 将数据序列化到JSON（需要包含convert框架后实现）
     */
    virtual void serializeToJson(Json::Object *pRootJson, const T &data) = 0;

private:
    /* SFINAE检测CreateWithDefaultRule方法 */
    template <typename, typename = std::void_t<>>
    struct hasCreateDefaultRule : std::false_type {};

    template <typename U>
    struct hasCreateDefaultRule<U, std::void_t<decltype(U::CreateWithDefaultRule())>>
        : std::true_type {};

    T getDefaultDataImpl(std::true_type)
    {
        return T::CreateWithDefaultRule();
    }

    T getDefaultDataImpl(std::false_type)
    {
        return T{};
    }
};

} /* namespace ConfigCompat_NS */
