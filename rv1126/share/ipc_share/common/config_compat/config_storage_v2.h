/**
 * @FilePath     : config_storage_v2.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-13 10:44:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-13 16:42:04
 * @Description  : 带版本兼容的配置存储模块（对原有config_storage.h的增强）
 */

#pragma once

#include <set>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include "convert_interface.h"
#include "config_compat.h"

//*===========================================================================*/
//*                     版本化配置存储类型枚举                                   */
//*===========================================================================*/

typedef enum class VersionedStorageType
{
    Single,    /* 单个对象 */
    Collection /* 集合 */
} VersionedStorageType_E;

//*===========================================================================*/
//*                     主模板声明                                              */
//*===========================================================================*/

template <typename T,
          VersionedStorageType_E Type = VersionedStorageType_E::Collection,
          typename Migrator = ConfigCompat_NS::CConfigMigrator<T>,
          typename Validator = ConfigCompat_NS::CConfigValidator<T>>
class VersionedConfigStorage;

//*===========================================================================*/
//*                     集合存储特化                                            */
//*===========================================================================*/

template <typename T, typename Migrator, typename Validator>
class VersionedConfigStorage<T, VersionedStorageType_E::Collection, Migrator, Validator>
{
public:
    /**
     * @brief   : 构造函数
     * @param    {std::string} &filePath：配置文件路径
     */
    VersionedConfigStorage(const std::string &filePath) : m_filePath(filePath)
    {
        loadFromFile();
    }

    ~VersionedConfigStorage()
    {
        saveToFile();
    }

    /**
     * @brief   : 删除配置项
     * @param    {T} &config：配置项
     * @return   {int} 0：成功
     */
    int del(const T &config)
    {
        m_data.erase(config);
        saveToFile();
        return 0;
    }

    /**
     * @brief   : 设置配置项（更新或新增）
     * @param    {T} &config：配置项
     * @return   {int} 0：成功
     */
    int set(const T &config)
    {
        /* 验证配置有效性 */
        T validatedConfig = config;
        ConfigCompat_NS::ConfigValidateResult_S result = Validator::validate(validatedConfig);

        if (!result.isValid())
        {
            return -1;
        }

        m_data.erase(validatedConfig);
        m_data.insert(validatedConfig);
        saveToFile();
        return 0;
    }

    /**
     * @brief   : 获取单个配置项
     * @param    {T} &config：配置项（输入key，输出完整数据）
     * @return   {int} 0：成功，-1：未找到
     */
    int get(T &config) const
    {
        auto it = m_data.find(config);
        if (it != m_data.end())
        {
            config = *it;
            return 0;
        }
        return -1;
    }

    /**
     * @brief   : 获取所有配置项
     * @param    {std::set<T>} &config：配置集合
     * @return   {int} 0：成功
     */
    int get(std::set<T> &config) const
    {
        config = m_data;
        return 0;
    }

    /**
     * @brief   : 清空所有配置
     * @return   {int} 0：成功
     */
    int clear()
    {
        m_data.clear();
        saveToFile();
        return 0;
    }

private:
    std::set<T> m_data;
    std::string m_filePath;

    /**
     * @brief   : 从文件加载配置（带版本迁移）
     */
    void loadFromFile()
    {
        std::ifstream file(m_filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return;
        }

        std::ifstream::pos_type fileSize = file.tellg();
        if (fileSize == 0)
        {
            file.close();
            return;
        }

        file.seekg(0, std::ios::beg);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string jsonData = buffer.str();
        Json::Object *pRootJson = Json::init(jsonData);
        if (!pRootJson)
        {
            return;
        }

        /* 检查版本并迁移 */
        ConfigCompat_NS::ConfigVersion_S fileVersion;
        ConfigCompat_NS::readVersionFromJson(pRootJson, fileVersion);
        ConfigCompat_NS::ConfigVersion_S currentVersion = Migrator::getCurrentVersion();

        bool needRewrite = false;
        if (fileVersion < currentVersion)
        {
            /* 执行迁移 */
            ConfigCompat_NS::MigrateResult_E migrateResult =
                Migrator::migrate(pRootJson, fileVersion, currentVersion);
            if (migrateResult != ConfigCompat_NS::MigrateResult_E::FAILED)
            {
                needRewrite = true;
            }
        }

        /* 使用现有Convert框架解析数据 */
        Convert::CConvert convert(true);
        convert.structure(pRootJson, Migrator::getConfigType(), m_data);

        Json::deinit(pRootJson);

        /* 验证并修复每个配置项 */
        std::set<T> validatedData;
        for (auto item : m_data)
        {
            ConfigCompat_NS::ConfigValidateResult_S result = Validator::validate(item);
            if (result.isValid())
            {
                validatedData.insert(item);
                if (result.enOverallResult == ConfigCompat_NS::ValidateResult_E::FIXED)
                {
                    needRewrite = true;
                }
            }
        }
        m_data = validatedData;

        /* 如果有修改，重新保存 */
        if (needRewrite)
        {
            saveToFile();
        }
    }

    /**
     * @brief   : 保存配置到文件（带版本信息）
     */
    void saveToFile()
    {
        Json::Object *pRootJson = Json::init();
        if (!pRootJson)
        {
            return;
        }

        /* 写入版本信息 */
        ConfigCompat_NS::writeVersionToJson(pRootJson,
                                         Migrator::getCurrentVersion(),
                                         Migrator::getConfigType());

        /* 使用现有Convert框架序列化数据 */
        Convert::CConvert convert(false);
        std::set<T> dataCopy = m_data;
        convert.structure(pRootJson, Migrator::getConfigType(), dataCopy);

        std::string jsonData = Json::to_string(pRootJson);
        Json::deinit(pRootJson);

        /* 原子写入 */
        std::string tmpPath = m_filePath + ".tmp";
        {
            std::ofstream file(tmpPath, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open())
            {
                return;
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

        ::rename(tmpPath.c_str(), m_filePath.c_str());

        std::string::size_type pos = m_filePath.rfind('/');
        if (pos != std::string::npos)
        {
            std::string dir = m_filePath.substr(0, pos);
            int dirFd = ::open(dir.c_str(), O_DIRECTORY | O_RDONLY);
            if (dirFd != -1)
            {
                ::fsync(dirFd);
                ::close(dirFd);
            }
        }
    }
};

//*===========================================================================*/
//*                     单对象存储特化                                          */
//*===========================================================================*/

template <typename T, typename Migrator, typename Validator>
class VersionedConfigStorage<T, VersionedStorageType_E::Single, Migrator, Validator>
{
public:
    /**
     * @brief   : 构造函数
     * @param    {std::string} &filePath：配置文件路径
     */
    VersionedConfigStorage(const std::string &filePath) : m_filePath(filePath)
    {
        if (!loadFromFile())
        {
            initializeDefault();
            saveToFile();
        }
    }

    ~VersionedConfigStorage()
    {
        saveToFile();
    }

    /**
     * @brief   : 设置配置
     * @param    {T} &config：配置数据
     * @return   {int} 0：成功，-1：验证失败
     */
    int set(const T &config)
    {
        T validatedConfig = config;
        ConfigCompat_NS::ConfigValidateResult_S result = Validator::validate(validatedConfig);

        if (!result.isValid())
        {
            return -1;
        }

        m_data = validatedConfig;
        saveToFile();
        return 0;
    }

    /**
     * @brief   : 获取配置
     * @param    {T} &config：输出配置数据
     * @return   {int} 0：成功
     */
    int get(T &config) const
    {
        config = m_data;
        return 0;
    }

    /**
     * @brief   : 获取配置的常量引用
     * @return   {const T&} 配置数据
     */
    const T &getData() const
    {
        return m_data;
    }

    /**
     * @brief   : 获取配置的引用
     * @return   {T&} 配置数据
     */
    T &getData()
    {
        return m_data;
    }

private:
    T m_data;
    std::string m_filePath;

    /* SFINAE检测CreateWithDefaultRule方法 */
    template <typename, typename = std::void_t<>>
    struct hasCreateDefaultRule : std::false_type {};

    template <typename U>
    struct hasCreateDefaultRule<U, std::void_t<decltype(U::CreateWithDefaultRule())>>
        : std::true_type {};

    void initializeDefault()
    {
        initializeDefaultImpl(hasCreateDefaultRule<T>{});
    }

    void initializeDefaultImpl(std::true_type)
    {
        m_data = T::CreateWithDefaultRule();
    }

    void initializeDefaultImpl(std::false_type)
    {
        m_data = T{};
    }

    /**
     * @brief   : 从文件加载配置
     * @return   {bool} true：加载成功，false：加载失败
     */
    bool loadFromFile()
    {
        std::ifstream file(m_filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return false;
        }

        std::ifstream::pos_type fileSize = file.tellg();
        if (fileSize == 0)
        {
            file.close();
            return false;
        }

        file.seekg(0, std::ios::beg);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string jsonData = buffer.str();
        Json::Object *pRootJson = Json::init(jsonData);
        if (!pRootJson)
        {
            return false;
        }

        /* 检查版本并迁移 */
        ConfigCompat_NS::ConfigVersion_S fileVersion;
        ConfigCompat_NS::readVersionFromJson(pRootJson, fileVersion);
        ConfigCompat_NS::ConfigVersion_S currentVersion = Migrator::getCurrentVersion();

        bool needRewrite = false;
        if (fileVersion < currentVersion)
        {
            ConfigCompat_NS::MigrateResult_E migrateResult =
                Migrator::migrate(pRootJson, fileVersion, currentVersion);
            if (migrateResult == ConfigCompat_NS::MigrateResult_E::FAILED)
            {
                Json::deinit(pRootJson);
                return false;
            }
            needRewrite = true;
        }

        /* 解析数据 */
        Convert::to_struct(Json::to_string(pRootJson), m_data);
        Json::deinit(pRootJson);

        /* 验证并修复 */
        ConfigCompat_NS::ConfigValidateResult_S result = Validator::validate(m_data);
        if (!result.isValid())
        {
            return false;
        }
        if (result.enOverallResult == ConfigCompat_NS::ValidateResult_E::FIXED)
        {
            needRewrite = true;
        }

        if (needRewrite)
        {
            saveToFile();
        }

        return true;
    }

    /**
     * @brief   : 保存配置到文件
     */
    void saveToFile()
    {
        Json::Object *pRootJson = Json::init();
        if (!pRootJson)
        {
            return;
        }

        ConfigCompat_NS::writeVersionToJson(pRootJson,
                                         Migrator::getCurrentVersion(),
                                         Migrator::getConfigType());

        /* 序列化数据 */
        Convert::CConvert convert(false);
        T dataCopy = m_data;
        convert.structure(pRootJson, dataCopy);

        std::string jsonData = Json::to_string(pRootJson);
        Json::deinit(pRootJson);

        std::string tmpPath = m_filePath + ".tmp";
        {
            std::ofstream file(tmpPath, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open())
            {
                return;
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

        ::rename(tmpPath.c_str(), m_filePath.c_str());

        std::string::size_type pos = m_filePath.rfind('/');
        if (pos != std::string::npos)
        {
            std::string dir = m_filePath.substr(0, pos);
            int dirFd = ::open(dir.c_str(), O_DIRECTORY | O_RDONLY);
            if (dirFd != -1)
            {
                ::fsync(dirFd);
                ::close(dirFd);
            }
        }
    }
};
