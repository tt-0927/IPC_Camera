/**
 * @FilePath     : gm_cert_configure.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-08-25 14:53:56
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-26 09:03:11
 * @Description  : 国密证书信息配置
 */

#include "gm_cert_configure.h"

CGmCertConfigure::CGmCertConfigure() : m_gmCertFileInfo(GM_CERT_INFO_FILE), m_gmCrlFileInfo(GM_CRL_INFO_FILE)
{
}

CGmCertConfigure::~CGmCertConfigure()
{
}

int CGmCertConfigure::set_configure(const Network::GmCertFileInfo_S &data)
{
    return m_gmCertFileInfo.set(data);
}

int CGmCertConfigure::get_configure(Network::GmCertFileInfo_S &data) const
{
    return m_gmCertFileInfo.get(data);
}

int CGmCertConfigure::get_configure(std::set<Network::GmCertFileInfo_S> &data) const
{
    return m_gmCertFileInfo.get(data);
}

int CGmCertConfigure::del_configure(const Network::GmCertFileInfo_S &data)
{
    return m_gmCertFileInfo.del(data);
}

int CGmCertConfigure::clear_configure(const Network::GmCertFileInfo_S &data)
{
    return m_gmCertFileInfo.clear();
}

int CGmCertConfigure::sort_configure(const Network::GmCertFileInfo_S &data)
{
    std::set<Network::GmCertFileInfo_S> astInfo;
    m_gmCertFileInfo.get(astInfo);

    if (!astInfo.empty())
    {
        /* 将set中的元素复制到vector中以便修改 */
        std::vector<Network::GmCertFileInfo_S> vecInfo(astInfo.begin(), astInfo.end());
        /* 重新编号，从1开始依次递增 */
        int nNewNum = 1;
        for (auto &stCertInfo : vecInfo)
        {
            /* 修改当前证书的序号 */
            stCertInfo.nNum = nNewNum++;
        }

        m_gmCertFileInfo.clear();

        /* 逐个插入重新编号后的证书信息 */
        for (const auto &stCertInfo : vecInfo)
        {
            m_gmCertFileInfo.set(stCertInfo);
        }
    }
    return 0;
}

int CGmCertConfigure::set_configure(const Network::GmCrlFileInfo_S &data)
{
    return m_gmCrlFileInfo.set(data);
}

int CGmCertConfigure::get_configure(Network::GmCrlFileInfo_S &data) const
{
    return m_gmCrlFileInfo.get(data);
}
