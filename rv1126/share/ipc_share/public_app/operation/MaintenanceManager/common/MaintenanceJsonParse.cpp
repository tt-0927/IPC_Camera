#include "MaintenanceJsonParse.h"
#include "dlog.h"
#include <iostream>
#include <cstring>

using namespace MaintenanceNS;

CMaintenanceJsonParse::CMaintenanceJsonParse() {}

MaintenanceManagerConf CMaintenanceJsonParse::parseConfig(std::string &strJson)
{
    MaintenanceManagerConf stConfig;

    stConfig.strCode = "";
    stConfig.vecUploadFile.clear();

    Json::Object *pRootItem = Json::init(strJson);
    if(pRootItem != NULL)
    {
        Json::Object * pDataItem = Json::get(pRootItem, "data");
        if(pDataItem != NULL)
        {
            Json::get(pDataItem, "project_code", stConfig.strCode);
            Json::get(pDataItem, "device_code", stConfig.strDeviceCode);
            Json::get(pDataItem, "url", stConfig.strUrl);
            Json::get(pDataItem, "record_path", stConfig.strRecordFilePath);

            addSplitChar(stConfig.strRecordFilePath);
            
            std::vector<std::string> vecPaths;
            Json::Object *pPathsItem = Json::get(pDataItem, std::string("paths"));
            if(pPathsItem != NULL)
            {
                int nIndex = 0;
                Json::Object *pArrAtItem = NULL;
                do
                {
                    std::string strPathTmp;
                    pArrAtItem = Json::Array::get(pPathsItem, nIndex++);
                    if(pArrAtItem != NULL)
                    {
                        Json::get(pArrAtItem, "path", strPathTmp);
                        if(!strPathTmp.empty())
                        {
                            addSplitChar(strPathTmp);
                            vecPaths.push_back(strPathTmp);
                        }
                        pArrAtItem = NULL;
                    }
                    else
                    {
                        break;
                    }
                }
                while (1);
            }
            stConfig.vecPaths = vecPaths;

            Json::Object *pArrItem = Json::get(pDataItem, std::string("uploadFileName"));
            if(pArrItem != NULL)
            {
                int nIndex = 0;
                Json::Object *pArrAtItem = NULL;
                bool bStop  = false;
                do
                {
                    pArrAtItem = Json::Array::get(pArrItem, nIndex++);
                    if(pArrAtItem != NULL)
                    {
                        UploadConfig stUploadFileFormat;
                        int nFileType = FILE_TYPE_NORMAL;
                        Json::get(pArrAtItem, "type", nFileType);
                        Json::get(pArrAtItem, "format", stUploadFileFormat.strFileNameFormat);
                        stUploadFileFormat.enFileType = static_cast<FileType>(nFileType);
                        stConfig.vecUploadFile.push_back(stUploadFileFormat);
                        pArrAtItem = NULL;
                    }
                    else
                    {
                        bStop = true;
                    }
                }
                while (!bStop);
            }
        }
        
        Json::deinit(pRootItem);
    }


    if(stConfig.strCode.empty())
    {
       dlog_error( "parse config, project code fial!");
    }
    if(stConfig.strDeviceCode.empty())
    {
       dlog_error( "parse config, device code fial!");
    }
    if(stConfig.strRecordFilePath.empty())
    {
       dlog_error( "parse config, record path fial!");
    }
    if(stConfig.vecPaths.size() <= 0)
    {
       dlog_error( "parse config, file paths fial!");
    }
    if(stConfig.vecUploadFile.size() <= 0)
    {
       dlog_error( "parse config, file format fial!");
    }
    return stConfig;
}

std::string CMaintenanceJsonParse::parseRegisterJson(std::string &strJson)
{
    std::string strDeviceCode;
    Json::Object *pRootItem = Json::init(strJson);
    if(pRootItem != NULL)
    {
        Json::get(pRootItem, "MachinSn", strDeviceCode);
        Json::deinit(pRootItem);
    }
    return strDeviceCode;
}

std::vector<RecordInfo> CMaintenanceJsonParse::parseRecordFileData(std::string &strJson)
{
    std::vector<RecordInfo> vecInfos;
    Json::Object *pRootItem = Json::init(strJson);
    if(pRootItem != NULL)
    {
        Json::Object *pDataItem = Json::get(pRootItem, std::string("data"));
        if(pDataItem != NULL)
        {
            int nIndex = 0;
            Json::Object *pArrAtItem = NULL;
            bool bStop  = false;
            do
            {
                pArrAtItem = Json::Array::get(pDataItem, nIndex++);
                if(pArrAtItem != NULL)
                {
                    RecordInfo stRecord;
                    int nFileType = FILE_TYPE_NORMAL;
                    Json::get(pArrAtItem, "type", nFileType);
                    Json::get(pArrAtItem, "identifier", stRecord.stFileInfo.strIdentifier);
                    Json::get(pArrAtItem, "path", stRecord.stFileInfo.strFilePath);
                    Json::get(pArrAtItem, "name", stRecord.stFileInfo.strFileName);
                    Json::get(pArrAtItem, "date", stRecord.stFileInfo.strFileDate);
                    stRecord.enUploadStatus = UPLOADED;
                    stRecord.stFileInfo.enFileType = static_cast<FileType>(nFileType);
                    vecInfos.push_back(stRecord);
                    pArrAtItem = NULL;
                }
                else
                {
                    bStop = true;
                }
            }
            while (!bStop);
        }
        Json::deinit(pRootItem);
    }
    return vecInfos;
}

std::vector<std::string> CMaintenanceJsonParse::parseFilterFileData(std::string &strJson)
{
    std::vector<std::string> vecInfos;
    Json::Object *pRootItem = Json::init(strJson);
    if(pRootItem != NULL)
    {
        Json::Object *pDataItem = Json::get(pRootItem, std::string("data"));
        if(pDataItem != NULL)
        {
            int nIndex = 0;
            Json::Object *pArrAtItem = NULL;
            bool bStop  = false;
            do
            {
                pArrAtItem = Json::Array::get(pDataItem, nIndex++);
                if(pArrAtItem != NULL)
                {
                    std::string strTmp;
                    Json::get(pArrAtItem, "date", strTmp);
                    vecInfos.push_back(strTmp);
                    pArrAtItem = NULL;
                }
                else
                {
                    bStop = true;
                }
            }
            while (!bStop);
        }
        Json::deinit(pRootItem);
    }
    return vecInfos;
}

LoginResult CMaintenanceJsonParse::parseLoginResult(std::string &strRequeryResult, bool &bRet)
{
    m_stLoginResult.strToken.clear();
    m_stLoginResult.vecProduct.clear();

    bRet = false;
    const char *strJson = strRequeryResult.c_str();
    Json::Object *pRootItem = Json::init(strJson);
    if(pRootItem != NULL)
    {
        if(parseHeader(pRootItem, m_stLoginResult.stResult))
        {
            bRet = true;
            Json::Object *pObjItem = Json::get(pRootItem, "data");
            if(pObjItem != NULL)
            {
                Json::get(pObjItem, "api_token", m_stLoginResult.strToken);
                Json::get(pObjItem, "username", m_stLoginResult.strUsername);
                Json::get(pObjItem, "nick", m_stLoginResult.strNick);

                Json::Object *pArrItem = Json::get(pObjItem, "product_list");
                if(pArrItem != NULL)
                {
                    int nIndex = 0;
                    while(1)
                    {
                        Json::Object *pArrAtItem = Json::Array::get(pArrItem, nIndex++);
                        if(pArrAtItem == NULL)
                        {
                            break;
                        }
                        Product stProduct;
                        Json::get(pArrAtItem, "id", stProduct.nID);
                        Json::get(pArrAtItem, "name", stProduct.strName);
                        m_stLoginResult.vecProduct.push_back(stProduct);
                    }
                }
            }
        }
        Json::deinit(pRootItem);
    }
    return m_stLoginResult;
}

ReqProjectResult CMaintenanceJsonParse::parseProjectResult(std::string &strRequeryResult, bool &bRet)
{
    m_stReqProjectResult.vecProject.clear();

    bRet = false;
    const char *strJson = strRequeryResult.c_str();
    Json::Object *pRootItem = Json::init(strJson);
    if(pRootItem != NULL)
    {
        if(parseHeader(pRootItem, m_stReqProjectResult.stResult))
        {
            bRet = true;

            Json::Object *pObjItem = Json::get(pRootItem, "data");
            if(pObjItem != NULL)
            {
                Json::get(pObjItem, "totalpage", m_stReqProjectResult.nTotalPage);
                Json::get(pObjItem, "total", m_stReqProjectResult.nTotal);
                Json::get(pObjItem, "pageid", m_stReqProjectResult.nPageid);

                Json::Object *pArrItem = Json::get(pObjItem, "data");
                if(pArrItem != NULL)
                {
                    int nIndex = 0;
                    while(1)
                    {
                        Json::Object *pArrAtItem = Json::Array::get(pArrItem, nIndex++);
                        if(pArrAtItem == NULL)
                        {
                            break;
                        }
                        Project stProject;
                        Json::get(pArrAtItem, "products_name", stProject.strProductName);
                        Json::get(pArrAtItem, "name", stProject.strName);
                        Json::get(pArrAtItem, "id", stProject.nID);
                        Json::get(pArrAtItem, "code", stProject.strCode);
                        m_stReqProjectResult.vecProject.push_back(stProject);
                    }
                }
            }
        }
        Json::deinit(pRootItem);
    }
    return m_stReqProjectResult;
}

ReqUploadResult CMaintenanceJsonParse::parseUploadResult(std::string &strRequeryResult, bool &bRet)
{
    bRet = false;
    Json::Object *pRootItem = Json::init(strRequeryResult);
    if(pRootItem != NULL)
    {
        /* 匹配正常的 */
        if(parseHeader(pRootItem, m_stReqUploadResult.stResult))
        {
            bRet = true;
        }
    }
    return m_stReqUploadResult;
}

char *CMaintenanceJsonParse::createRecordFileBuffer(std::string &strJson, const RecordInfo &stRecordInfo, std::size_t &nSize)
{
    char * pJsonBuffer = nullptr;
    nSize = 0;
    Json::Object *pRootItem = Json::init(strJson);
    if(pRootItem == NULL)
    {
        pRootItem = Json::init();
    }
    if(pRootItem != NULL)
    {
        Json::Object *pDataItem = Json::get(pRootItem, std::string("data"));
        bool bIsNew = false;
        if(pDataItem == NULL)
        {
            bIsNew = true;
            pDataItem = Json::Array::init();
        }
        if(pDataItem != NULL)
        {
            Json::Object *pArrAtItem = Json::init();
            Json::add(pArrAtItem, "type", stRecordInfo.stFileInfo.enFileType);
            Json::add(pArrAtItem, "identifier", stRecordInfo.stFileInfo.strIdentifier);
            Json::add(pArrAtItem, "path", stRecordInfo.stFileInfo.strFilePath);
            Json::add(pArrAtItem, "name", stRecordInfo.stFileInfo.strFileName);
            Json::add(pArrAtItem, "date", stRecordInfo.stFileInfo.strFileDate);
            Json::Array::add(pDataItem, pArrAtItem);

            if(bIsNew)
            {
                Json::add(pRootItem, "data", pDataItem);
            }

            pJsonBuffer = Json::print(pRootItem);
            if(pJsonBuffer != NULL)
            {
                nSize = strlen(pJsonBuffer);
            }
            strJson = Json::to_string(pRootItem);
        }
        Json::deinit(pRootItem);
    }
    return pJsonBuffer;
}

char *CMaintenanceJsonParse::createFilterFileBuffer(std::vector<std::string> &vecFilter, std::size_t &nSize)
{
    char * pJsonBuffer = nullptr;
    nSize = 0;

    Json::Object *pRootItem = Json::init();
    if(pRootItem != NULL)
    {
        Json::Object *pDataItem = Json::Array::init();
        if(pDataItem != NULL)
        {
            for(std::size_t i = 0; i < vecFilter.size(); i++)
            {
                Json::Object *pArrAtItem = Json::init();
                Json::add(pArrAtItem, "date", vecFilter.at(i));
                Json::Array::add(pDataItem, pArrAtItem);
            }

            Json::add(pRootItem, "data", pDataItem);

            pJsonBuffer = Json::print(pRootItem);
            if(pJsonBuffer != NULL)
            {
                nSize = strlen(pJsonBuffer);
            }
        }
        Json::deinit(pRootItem);
    }

    return pJsonBuffer;
}

bool CMaintenanceJsonParse::parseHeader(Json::Object *pObj, Result &header)
{
    bool bRet = false;
    bRet = Json::get(pObj, "result", header.nResult);
    Json::get(pObj, "company", header.strCompany);
    Json::get(pObj, "device_name", header.strDeviceName);
    Json::get(pObj, "return_message", header.strMsg);
    if(!bRet)
    {
        /* 说明Token失效了 */
        bRet = Json::get(pObj, "errcode", header.nResult);
        Json::get(pObj, "msg", header.strMsg);
    }
    return bRet;
}

void CMaintenanceJsonParse::addSplitChar(std::string &strData)
{
    if(!strData.empty())
    {
        int nLength = strData.length();
        if(strData
                .substr(nLength - 1, nLength)
                .compare("/") != 0)
        {
            strData.insert(nLength, "/");
        }
    }
}

std::string CMaintenanceJsonParse::createGetDeviceInfoJson(int nCode, int nOpt)
{
    std::string strJson;

    Json::Object *pRootItem = Json::init();
    if(pRootItem != NULL)
    {
        Json::Object *pDataItem = Json::init();
        if(pDataItem != NULL)
        {
            Json::add(pDataItem, "code", nCode);
            Json::add(pDataItem, "opt", nOpt);

            Json::add(pRootItem, "data", pDataItem);
        }
        strJson = Json::to_string(pRootItem);
        Json::deinit(pRootItem);
    }

    return strJson;
}