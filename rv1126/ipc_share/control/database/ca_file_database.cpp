/**
 * @file ca_file_database.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2024-11-08
 * 
 * @brief 
 */

#include "ca_file_database.h"
#include "ca_manage.h"
#include <iostream>

using namespace Db;
CaFileDatabase::CaFileDatabase()
    : m_TrustDatabase(TRUST_CA_FILE_DATABASE_PATH, TRUST_CA_FILE_TABLE_NAME)
    , m_DeviceDatabase(DEVICE_CA_FILE_DATABASE_PATH, DEVICE_CA_FILE_TABLE_NAME)  
{
    create();
}

CaFileDatabase::~CaFileDatabase()
{
}

int CaFileDatabase::create()
{
    m_TrustDatabase.add_tableKey(TableKey(CA_FILE_FIELD_NUM, CDbBase::type_int()));
    m_TrustDatabase.add_tableKey(TableKey(CA_FILE_FIELD_SERIA_NUM, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_TrustDatabase.add_tableKey(TableKey(CA_FILE_FIELD_EXPIRA_DATE, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_TrustDatabase.add_tableKey(TableKey(CA_FILE_FIELD_USER, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_TrustDatabase.add_tableKey(TableKey(CA_FILE_FIELD_LICENSOR, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_TrustDatabase.add_tableKey(TableKey(CA_FILE_FIELD_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));

    m_DeviceDatabase.add_tableKey(TableKey(CA_FILE_FIELD_NUM, CDbBase::type_int()));
    m_DeviceDatabase.add_tableKey(TableKey(CA_FILE_FIELD_SERIA_NUM, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_DeviceDatabase.add_tableKey(TableKey(CA_FILE_FIELD_EXPIRA_DATE, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_DeviceDatabase.add_tableKey(TableKey(CA_FILE_FIELD_USER, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_DeviceDatabase.add_tableKey(TableKey(CA_FILE_FIELD_LICENSOR, CDbBase::type_string(MAX_DB_STRING_SIZE)));
    m_DeviceDatabase.add_tableKey(TableKey(CA_FILE_FIELD_PATH, CDbBase::type_string(MAX_DB_STRING_SIZE)));

    m_TrustDatabase.init();
    m_DeviceDatabase.init();


    Network::CertFileInfo_S stRootInfo;
    Network::CertFileInfo_S stMiddleInfo;
    stRootInfo.nNum = 1;
    stMiddleInfo.nNum = 2;

    std::vector<Network::CertFileInfo_S> stInfos;

    /* 添加根证书信息 */
    trust_find(Element(CA_FILE_FIELD_NUM, stRootInfo.nNum), stInfos);
    if (stInfos.size() == 0)
    {
        CCaManage::instance()->getCertificateInfo(CA_ROOT_CERT,stRootInfo);
        stRootInfo.strPath = CA_ROOT_CERT;
        trust_add(stRootInfo);
    }

    stInfos.clear();

    /* 添加中间证书信息 */
    trust_find(Element(CA_FILE_FIELD_NUM, stMiddleInfo.nNum), stInfos);
    if (stInfos.size() == 0)
    {
        CCaManage::instance()->getCertificateInfo(CA_MIDDLE_CERT,stMiddleInfo);
        stMiddleInfo.strPath = CA_MIDDLE_CERT;
        trust_add(stMiddleInfo);
    }

    return 0;
}

int CaFileDatabase::trust_add( Network::CertFileInfo_S &stInfo)
{
    /* 获取编号 */
    int nMaxNum = 0;
    std::vector<Network::CertFileInfo_S> stCertInfo;
    MatchMethods emptyMethods;
    trust_find(emptyMethods, stCertInfo);
    nMaxNum = static_cast<int>(stCertInfo.size());
  
    stInfo.nNum = nMaxNum + 1;
    Item item;
    item.push_back(Element(CA_FILE_FIELD_NUM, stInfo.nNum));
    item.push_back(Element(CA_FILE_FIELD_SERIA_NUM, stInfo.strSerialNum));
    item.push_back(Element(CA_FILE_FIELD_EXPIRA_DATE, stInfo.strExpiraDate));
    item.push_back(Element(CA_FILE_FIELD_USER, stInfo.strUser));
    item.push_back(Element(CA_FILE_FIELD_LICENSOR, stInfo.strLicensor));
    item.push_back(Element(CA_FILE_FIELD_PATH, stInfo.strPath));

    item.push_back(Element(DB_COMMON_FIELD_RESERVE1, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE2, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE3, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE4, std::string()));
    return m_TrustDatabase.add(item);
}


int CaFileDatabase::device_add( Network::CertFileInfo_S &stInfo)
{
    /* 获取编号 */
    int nMaxNum = 0;
    std::vector<Network::CertFileInfo_S> stCertInfo;
    MatchMethods emptyMethods;
    device_find(emptyMethods, stCertInfo);
    nMaxNum = static_cast<int>(stCertInfo.size());
  
    stInfo.nNum = nMaxNum + 1;

    Item item;
    item.push_back(Element(CA_FILE_FIELD_NUM, stInfo.nNum));
    item.push_back(Element(CA_FILE_FIELD_SERIA_NUM, stInfo.strSerialNum));
    item.push_back(Element(CA_FILE_FIELD_EXPIRA_DATE, stInfo.strExpiraDate));
    item.push_back(Element(CA_FILE_FIELD_USER, stInfo.strUser));
    item.push_back(Element(CA_FILE_FIELD_LICENSOR, stInfo.strLicensor));
    item.push_back(Element(CA_FILE_FIELD_PATH, stInfo.strPath));

    item.push_back(Element(DB_COMMON_FIELD_RESERVE1, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE2, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE3, std::string()));
    item.push_back(Element(DB_COMMON_FIELD_RESERVE4, std::string()));
    return m_DeviceDatabase.add(item);
}

int CaFileDatabase::trust_find(const Element &elem, std::vector<Network::CertFileInfo_S> &infos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    trust_find(methods, infos);

    return 0;
}

int CaFileDatabase::trust_find(const MatchMethods &methods, std::vector<Network::CertFileInfo_S> &infos)
{
    std::vector<Item> items;
    m_TrustDatabase.find(methods, items);

    for (Item &item : items)
    {
        Network::CertFileInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(CA_FILE_FIELD_NUM):
                stInfo.nNum = mpark::get<int>(value);
                break;
            case str2tag(CA_FILE_FIELD_SERIA_NUM):
                stInfo.strSerialNum = mpark::get<std::string>(value);
                break;
            case str2tag(CA_FILE_FIELD_EXPIRA_DATE):
                stInfo.strExpiraDate = mpark::get<std::string>(value);
                break;
            case str2tag(CA_FILE_FIELD_USER):
                stInfo.strUser = mpark::get<std::string>(value);
                break;
            case str2tag(CA_FILE_FIELD_LICENSOR):
                stInfo.strLicensor = mpark::get<std::string>(value);
                break;
            case str2tag(CA_FILE_FIELD_PATH):
                stInfo.strPath = mpark::get<std::string>(value);
                break;
            default:
                break;
            }

        }
        infos.push_back(stInfo);

    }
   
    return 0;
}


int CaFileDatabase::device_find(const Element &elem, std::vector<Network::CertFileInfo_S> &infos)
{
    MatchMethods methods;
    methods.push_back(MatchMethod(elem, FIND_CRITERION_EQ));
    device_find(methods, infos);

    return 0;
}

int CaFileDatabase::device_find(const MatchMethods &methods, std::vector<Network::CertFileInfo_S> &infos)
{
    std::vector<Item> items;
    m_DeviceDatabase.find(methods, items);

    for (Item &item : items)
    {
        Network::CertFileInfo_S stInfo;
        for (Element &pair : item)
        {
            std::string &key = pair.first;
            FieldValue &value = pair.second;
            switch (str2tag(key.c_str()))
            {
            case str2tag(CA_FILE_FIELD_NUM):
                stInfo.nNum = mpark::get<int>(value);
                break;
            case str2tag(CA_FILE_FIELD_SERIA_NUM):
                stInfo.strSerialNum = mpark::get<std::string>(value);
                break;
            case str2tag(CA_FILE_FIELD_EXPIRA_DATE):
                stInfo.strExpiraDate = mpark::get<std::string>(value);
                break;
            case str2tag(CA_FILE_FIELD_USER):
                stInfo.strUser = mpark::get<std::string>(value);
                break;
            case str2tag(CA_FILE_FIELD_LICENSOR):
                stInfo.strLicensor = mpark::get<std::string>(value);
                break;
            case str2tag(CA_FILE_FIELD_PATH):
                stInfo.strPath = mpark::get<std::string>(value);
                break;
            default:
                break;
            }

        }
        infos.push_back(stInfo);

    }
   
    return 0;
}

int CaFileDatabase::trust_get_all_items(std::vector<Network::CertFileInfo_S> &stCertInfo)
{
    MatchMethods emptyMethods;
    trust_find(emptyMethods, stCertInfo);
 
    return 0;
}


int CaFileDatabase::device_get_all_items(std::vector<Network::CertFileInfo_S> &stCertInfo)
{
    MatchMethods emptyMethods;
    device_find(emptyMethods, stCertInfo);
 
    return 0;
}

int CaFileDatabase::trust_update(const Item &item, const MatchMethods &methods)
{
    return m_TrustDatabase.update(item, methods);
}

int CaFileDatabase::device_update(const Item &item, const MatchMethods &methods)
{
    return m_DeviceDatabase.update(item, methods);
}

int CaFileDatabase::trust_del(const Network::CertFileInfo_S& stCertFileInfo)
{
    /* 获取所有证书 */
    std::vector<Network::CertFileInfo_S>vecCertInfo;
    MatchMethods emptyMethods;
    trust_find(emptyMethods, vecCertInfo);

    /* 根证书和中间证书不能删除 */
    if (stCertFileInfo.nNum == 1 || stCertFileInfo.nNum == 2)
    {
        dlog_error("不能删除编号为 %d 的证书", stCertFileInfo.nNum);
        return -1;
    }
    

    int nMaxNum = static_cast<int>(vecCertInfo.size());
    if (stCertFileInfo.nNum > nMaxNum || stCertFileInfo.nNum < 1)
    {
        dlog_error("删除失败，编号 %d 超出范围 [1, %d]", stCertFileInfo.nNum, nMaxNum);
        return -1; 
    }

    Item item;
    item.push_back(Element(CA_FILE_FIELD_SERIA_NUM, stCertFileInfo.strSerialNum));

    m_TrustDatabase.del(item);

     /* 如果删除的是最后一个编号，直接返回 */ 
    if (stCertFileInfo.nNum == nMaxNum)
    {
        dlog_info("无需更新编号");
        return 0;
    }

     /* 更新后续证书编号 */ 
    for (int i = stCertFileInfo.nNum; i < nMaxNum; ++i)
    {
        auto updateIt = std::find_if(vecCertInfo.begin(), vecCertInfo.end(),
                                     [i](const Network::CertFileInfo_S &info)
                                     {
                                         return info.nNum == i + 1; 
                                     });

        dlog_info("更新编号 %d", i);
        if (updateIt != vecCertInfo.end())
        {
            /* 更新编号（减 1） */ 
            updateIt->nNum = i;
            dlog_info("更新编号 %d", i);

            /* 更新到数据库 */ 
            Item updateItem;
            MatchMethods methods;
            updateItem.push_back(Element(CA_FILE_FIELD_NUM, updateIt->nNum));
            methods.push_back(MatchMethod(Element(CA_FILE_FIELD_NUM,i+1), FIND_CRITERION_EQ));
            if (trust_update(updateItem, methods) != 0)
            {
                dlog_error("更新编号 %d 失败", i);
                return -1;
            }
        }
    }


    return 0;
}

int CaFileDatabase::device_del(const Network::CertFileInfo_S& stCertFileInfo)
{
    /* 获取所有证书 */
    std::vector<Network::CertFileInfo_S>vecCertInfo;
    MatchMethods emptyMethods;
    device_find(emptyMethods, vecCertInfo);

    int nMaxNum = static_cast<int>(vecCertInfo.size());
    if (stCertFileInfo.nNum > nMaxNum || stCertFileInfo.nNum < 1)
    {
        dlog_error("删除失败，编号 %d 超出范围 [1, %d]", stCertFileInfo.nNum, nMaxNum);
        return -1; 
    }

    Item item;
    item.push_back(Element(CA_FILE_FIELD_SERIA_NUM, stCertFileInfo.strSerialNum));

    m_DeviceDatabase.del(item);

     /* 如果删除的是最后一个编号，直接返回 */ 
    if (stCertFileInfo.nNum == nMaxNum)
    {
        dlog_info("无需更新编号");
        return 0;
    }

     /* 更新后续证书编号 */ 
    for (int i = stCertFileInfo.nNum; i < nMaxNum; ++i)
    {
        auto updateIt = std::find_if(vecCertInfo.begin(), vecCertInfo.end(),
                                     [i](const Network::CertFileInfo_S &info)
                                     {
                                         return info.nNum == i + 1; 
                                     });

        dlog_info("更新编号 %d", i);
        if (updateIt != vecCertInfo.end())
        {
            /* 更新编号（减 1） */ 
            updateIt->nNum = i;
            dlog_info("更新编号 %d", i);

            /* 更新到数据库 */ 
            Item updateItem;
            MatchMethods methods;
            updateItem.push_back(Element(CA_FILE_FIELD_NUM, updateIt->nNum));
            methods.push_back(MatchMethod(Element(CA_FILE_FIELD_NUM,i+1), FIND_CRITERION_EQ));
            if (device_update(updateItem, methods) != 0)
            {
                dlog_error("更新编号 %d 失败", i);
                return -1;
            }
        }
    }


    return 0;
}
