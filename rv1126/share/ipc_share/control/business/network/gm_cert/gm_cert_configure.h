/**
 * @FilePath     : gm_cert_configure.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-25 14:53:56
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-26 09:07:57
 * @Description  : 国密证书信息配置
 */

#pragma once

#include <functional>
#include <memory>
#include "Singleton.h"
#include "config_storage.h"
#include "network_define.h"
#include "IpcRet.h"

class CGmCertConfigure : public CSingleton<CGmCertConfigure>
{
    CGmCertConfigure();

public:
    ~CGmCertConfigure();
    friend class CSingleton<CGmCertConfigure>;

    /**
     * @brief   : 设置国密证书文件信息
     * @param    {GmCertFileInfo_S} &data 国密证书文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int set_configure(const Network::GmCertFileInfo_S &data);

    /**
     * @brief   : 获取国密证书文件信息
     * @param    {GmCertFileInfo_S} &data 国密证书文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int get_configure(Network::GmCertFileInfo_S &data) const;

    /**
     * @brief   : 获取国密证书文件信息
     * @param    {set<Network::GmCertFileInfo_S>} &data 国密证书文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int get_configure(std::set<Network::GmCertFileInfo_S> &data) const;

    /**
     * @brief   : 删除国密证书文件信息
     * @param    {GmCertFileInfo_S} &data 国密证书文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int del_configure(const Network::GmCertFileInfo_S &data);

    /**
     * @brief   : 清空国密证书文件信息
     * @param    {GmCertFileInfo_S} &data 国密证书文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int clear_configure(const Network::GmCertFileInfo_S &data);

    /**
     * @brief   : 排序国密证书文件信息
     * @param    {GmCertFileInfo_S} &data 国密证书文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int sort_configure(const Network::GmCertFileInfo_S &data);

    /**
     * @brief   : 设置国密CRL文件信息
     * @param    {GmCrlFileInfo_S} &data 国密CRL文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int set_configure(const Network::GmCrlFileInfo_S &data);

    /**
     * @brief   : 获取国密CRL文件信息
     * @param    {GmCrlFileInfo_S} &data 国密CRL文件信息
     * @return   {int} 0：成功，非0：失败
     */
    int get_configure(Network::GmCrlFileInfo_S &data) const;

private:
    /* 国密证书文件信息 */
    ConfigStorage<Network::GmCertFileInfo_S> m_gmCertFileInfo;
    /* 国密CRL文件信息 */
    ConfigStorage<Network::GmCrlFileInfo_S, StorageType_E::Single> m_gmCrlFileInfo;
};
