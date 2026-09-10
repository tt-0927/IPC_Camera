/**
 * @file ca_file_database.h
 * @author tianl (tianl@kfb.cn)
 * @date 2024-11-08
 * 
 * @brief ca证书文件数据库
 */
#pragma once

#include "DbBase.h"
#include "network_define.h"
#include "Singleton.h"
namespace Db
{
    constexpr const char *TRUST_CA_FILE_TABLE_NAME = "trust_ca_file_manage";
    constexpr const char *TRUST_CA_FILE_DATABASE_PATH = "/opt/cam/db/trust_ca_file_manage.db";

    constexpr const char *DEVICE_CA_FILE_TABLE_NAME = "device_ca_file_manage";
    constexpr const char *DEVICE_CA_FILE_DATABASE_PATH = "/opt/cam/db/device_ca_file_manage.db";

    constexpr const char *CA_FILE_FIELD_NUM = "num";
    constexpr const char *CA_FILE_FIELD_SERIA_NUM = "seria_num";
    constexpr const char *CA_FILE_FIELD_EXPIRA_DATE = "expira_date";
    constexpr const char *CA_FILE_FIELD_USER = "user";
    constexpr const char *CA_FILE_FIELD_LICENSOR = "licensor";
    constexpr const char *CA_FILE_FIELD_PATH = "path";



    class CaFileDatabase : public CSingleton<CaFileDatabase>
    {
        CaFileDatabase();
    public:

        ~CaFileDatabase();
        friend class CSingleton<CaFileDatabase>;
        
        /**
         * @brief 向受信任证书数据库中添加证书信息
         * @param stInfo 
         * @return int 
         */
        int trust_add(Network::CertFileInfo_S &stInfo);

         /**
          * @brief 向设备证书数据库中添加证书信息
          * @param stInfo 
          * @return int 
          */
        int device_add(  Network::CertFileInfo_S &stInfo);


        /*
         * @description: 查找信息数据
         * @param[int]: elem 需要查找的内容
         * @param[out]: infos 输出信息数据
         * @return:  <0 失败
         */
        int trust_find(const Element &elem, std::vector<Network::CertFileInfo_S> &infos);
        int trust_find(const MatchMethods &methods, std::vector<Network::CertFileInfo_S> &infos);

    /*
         * @description: 查找信息数据
         * @param[int]: elem 需要查找的内容
         * @param[out]: infos 输出信息数据
         * @return:  <0 失败
         */
        int device_find(const Element &elem, std::vector<Network::CertFileInfo_S> &infos);
        int device_find(const MatchMethods &methods, std::vector<Network::CertFileInfo_S> &infos);

        /*
         * @description: 更新信息
         * @param[int]: item 需要更新的信息
         * @param[int]: methods 匹配方式
         * @return:  <0 失败
         */
        int trust_update(const Item &item, const MatchMethods &methods);
        int device_update(const Item &item, const MatchMethods &methods);

        /*
         * @description: 删除数据
         * @param[int]: item 需要删除条目的相关信息
         * @return:  <0 失败
         */
        int trust_del(const Network::CertFileInfo_S& stCertFileInfo);

        int device_del(const Network::CertFileInfo_S& stCertFileInfo);

        /**
         * @brief 获取信任证书数据库信息
         * @param stCertInfo 
         * @return int 
         */
        int trust_get_all_items(std::vector<Network::CertFileInfo_S> &stCertInfo);

        /**
         * @brief 获取设备证书数据库信息
         * @param stCertInfo 
         * @return int 
         */
        int device_get_all_items(std::vector<Network::CertFileInfo_S> &stCertInfo);
    private:
        int create();

    private:
        /**
         * @brief 受信任的证书数据库
         */
        CDbBase m_TrustDatabase;
        /**
         * @brief 设备证书数据库
         */
        CDbBase m_DeviceDatabase;
        /**
         * @brief 设备证书数组
         */
        std::vector<Network::CertFileInfo_S> vecDeviceCertInfo;

        /**
         * @brief 信任证书数组
         */
        std::vector<Network::CertFileInfo_S> vecTrustCertInfo;
    };

} /* namespace Db */