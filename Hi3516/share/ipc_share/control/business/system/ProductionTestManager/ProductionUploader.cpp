/*** 
 * @FilePath     : ProductionUploader.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-08 11:25:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-08 11:25:00
 * @Description  : 产测结果上传
 */

#include "ProductionUploader.h"
#include "ProductionTestManager.h"
#include "dlog.h"
#include "share_define.h"
#include "edukit_network.h"
#include "cJSON.h"

#include <algorithm>
#include <ctime>
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>

/* 运维平台地址 */
#define OPERATION_BASE_URL "https://oam.itc-pa.cn"

/**
 * @brief  curl写回调
 */
static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total = size * nmemb;
    std::string *pStr = static_cast<std::string *>(userp);
    pStr->append(static_cast<char *>(contents), total);
    return total;
}

std::string ProductionUploader::getMac()
{
    char achMac[32] = {0};
    if (0 == ReachMacAddrCapital(ETH0_INTERFACE, achMac))
    {
        return std::string(achMac);
    }
    if (0 == ReachMacAddrCapital(ETH1_INTERFACE, achMac))
    {
        return std::string(achMac);
    }
    return "";
}

int ProductionUploader::doLogin(std::string &strToken)
{
    std::string strUrl = std::string(OPERATION_BASE_URL) + "/api/v2/user/login";

    cJSON *pRoot = cJSON_CreateObject();
    cJSON_AddStringToObject(pRoot, "username", MQTT_LOGIN);
    cJSON_AddStringToObject(pRoot, "pwd", MQTT_PASSWORD);
    char *pJson = cJSON_Print(pRoot);
    if (!pJson)
    {
        cJSON_Delete(pRoot);
        return -1;
    }
    std::string strBody(pJson);
    free(pJson);
    cJSON_Delete(pRoot);

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        dlog_error("curl_easy_init failed");
        return -1;
    }

    std::string strResponse;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strBody.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strBody.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strResponse);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        dlog_error("Login request failed: %s", curl_easy_strerror(res));
        return -1;
    }

    dlog_info("Login response: %s", strResponse.c_str());

    cJSON *pResp = cJSON_Parse(strResponse.c_str());
    if (!pResp)
    {
        dlog_error("Failed to parse login response");
        return -1;
    }

    cJSON *pResult = cJSON_GetObjectItem(pResp, "result");
    if (!pResult || pResult->valueint != 200)
    {
        cJSON_Delete(pResp);
        dlog_error("Login failed: result=%d", pResult ? pResult->valueint : -1);
        return -1;
    }

    cJSON *pData = cJSON_GetObjectItem(pResp, "data");
    if (!pData)
    {
        cJSON_Delete(pResp);
        dlog_error("Login response missing data");
        return -1;
    }

    cJSON *pToken = cJSON_GetObjectItem(pData, "api_token");
    if (!pToken || !pToken->valuestring)
    {
        cJSON_Delete(pResp);
        dlog_error("Login response missing api_token");
        return -1;
    }

    strToken = pToken->valuestring;
    cJSON_Delete(pResp);
    return 0;
}

int ProductionUploader::doUploadFile(const std::string &strToken,
                                     const std::string &strFilePath,
                                     const std::string &strFileName)
{
    std::string strUrl = std::string(OPERATION_BASE_URL) + "/api/v2/production/add_production_test_log";

    /* 获取MAC地址作为序列号 */
    std::string strMac = getMac();
    if (strMac.empty())
    {
        dlog_error("Failed to get MAC address");
        return -1;
    }

    std::string strSerial = strMac;
    strSerial.erase(std::remove(strSerial.begin(), strSerial.end(), ':'), strSerial.end());
    strSerial.erase(std::remove(strSerial.begin(), strSerial.end(), '-'), strSerial.end());

    /* 判断整体测试结果：有失败项则test_resource=2，同时提取test_user */
    std::string strResults = ProductionTestManager::instance()->getUploadData();
    int nTestResource = 1;
    std::string strTestUser;
    cJSON *pResultsJson = cJSON_Parse(strResults.c_str());
    if (pResultsJson)
    {
        cJSON *pFailed = cJSON_GetObjectItem(pResultsJson, "failed");
        if (pFailed && pFailed->valueint > 0)
        {
            nTestResource = 2;
        }
        cJSON *pTestUser = cJSON_GetObjectItem(pResultsJson, "tester");
        if (pTestUser && cJSON_IsString(pTestUser) && pTestUser->valuestring)
        {
            strTestUser = pTestUser->valuestring;
        }
        cJSON_Delete(pResultsJson);
    }

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        dlog_error("curl_easy_init failed");
        return -1;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, (std::string("ApiToken:") + strToken).c_str());
    headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
    headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.9,en;q=0.8");
    headers = curl_slist_append(headers, "Cache-Control: no-cache");
    headers = curl_slist_append(headers, "Connection: keep-alive");

    curl_httppost *postFirst = NULL;
    curl_httppost *postLast = NULL;

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "file",
                 CURLFORM_FILE, strFilePath.c_str(),
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "chunkNumber",
                 CURLFORM_COPYCONTENTS, "1",
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "currentChunkSize",
                 CURLFORM_COPYCONTENTS, std::to_string(strFileName.size()).c_str(),
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "identifier",
                 CURLFORM_COPYCONTENTS, strFileName.c_str(),
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "filename",
                 CURLFORM_COPYCONTENTS, strFileName.c_str(),
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "totalChunks",
                 CURLFORM_COPYCONTENTS, "1",
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "name",
                 CURLFORM_COPYCONTENTS, strFileName.c_str(),
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "product_id",
                 CURLFORM_COPYCONTENTS, MQTT_PRODUCT_ID,
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "model",
                 CURLFORM_COPYCONTENTS, MQTT_MODEL,
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "device_name",
                 CURLFORM_COPYCONTENTS, MQTT_DEVICE_NAME,
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "serial_number",
                 CURLFORM_COPYCONTENTS, strSerial.c_str(),
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "mac",
                 CURLFORM_COPYCONTENTS, strMac.c_str(),
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "stage",
                 CURLFORM_COPYCONTENTS, "4",
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "test_resource",
                 CURLFORM_COPYCONTENTS, std::to_string(nTestResource).c_str(),
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "test_type",
                 CURLFORM_COPYCONTENTS, "1",
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "content",
                 CURLFORM_COPYCONTENTS, "产测日志",
                 CURLFORM_END);

    curl_formadd(&postFirst, &postLast,
                 CURLFORM_COPYNAME, "test_user",
                 CURLFORM_COPYCONTENTS, strTestUser.c_str(),
                 CURLFORM_END);

    std::string strResponse;
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, postFirst);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strResponse);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);

    curl_formfree(postFirst);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        dlog_error("Upload request failed: %s", curl_easy_strerror(res));
        return -1;
    }

    dlog_info("Upload response: %s", strResponse.c_str());

    cJSON *pResp = cJSON_Parse(strResponse.c_str());
    if (!pResp)
    {
        dlog_error("Failed to parse upload response");
        return -1;
    }

    cJSON *pResult = cJSON_GetObjectItem(pResp, "result");
    int nRet = -1;
    if (pResult && pResult->valueint == 200)
    {
        nRet = 0;
    }
    else
    {
        dlog_error("Upload failed: result=%d", pResult ? pResult->valueint : -1);
    }

    cJSON_Delete(pResp);
    return nRet;
}

int ProductionUploader::upload(const std::string &strJsonData)
{
    if (strJsonData.empty() || strJsonData == "{}")
    {
        dlog_error("No data to upload");
        return -1;
    }

    std::string strToken;
    int nRet = doLogin(strToken);
    if (nRet != 0)
    {
        dlog_error("Login failed, cannot upload");
        return -1;
    }

    std::string strMac = getMac();
    if (strMac.empty())
    {
        dlog_error("Failed to get MAC address");
        return -1;
    }

    time_t nNow = time(nullptr);
    struct tm *pTM = localtime(&nNow);
    char achTime[32] = {0};
    strftime(achTime, sizeof(achTime), "%Y%m%d%H%M%S", pTM);

    std::string strFileName = strMac + "_production_test_" + achTime;
    std::string strTmpFile = std::string("/tmp/") + strFileName + ".json";
    dlog_info("Upload filename: %s", strFileName.c_str());
    {
        std::ofstream file(strTmpFile, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            dlog_error("Failed to create temp file: %s", strTmpFile.c_str());
            return -1;
        }
        file << strJsonData << std::endl;
        file.flush();
        file.close();
    }
    nRet = doUploadFile(strToken, strTmpFile, strFileName);

    ::unlink(strTmpFile.c_str());

    return nRet;
}
