#include "CurlMultipartHttpPost.h"
#include <string>
#include <iostream>

using namespace CurlHttp;

CCurlMultipartHttpPost::CCurlMultipartHttpPost(const std::string& url)
    :Base(url)
{
    std::string strFullUri = m_url + m_strPaht;
    curl_easy_setopt(m_curl.get(), CURLOPT_URL, strFullUri.c_str());
    curl_easy_setopt(m_curl.get(), CURLOPT_POST, 1L);
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const int &nData)
{
    const char *itemName = strItem.c_str();
    std::string strData = std::to_string(nData);
    const char *itemData = strData.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_COPYCONTENTS, itemData,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const float &fData)
{
    const char *itemName = strItem.c_str();
    std::string strData = std::to_string(fData);
    const char *itemData = strData.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_COPYCONTENTS, itemData,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const double &dData)
{
    const char *itemName = strItem.c_str();
    std::string strData = std::to_string(dData);
    const char *itemData = strData.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_COPYCONTENTS, itemData,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const std::size_t &nData)
{
    const char *itemName = strItem.c_str();
    std::string strData = std::to_string(nData);
    const char *itemData = strData.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_COPYCONTENTS, itemData,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const char *pData)
{
    const char *itemName = strItem.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_COPYCONTENTS, pData,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const std::string &strData)
{
    const char *itemName = strItem.c_str();
    const char *itemData = strData.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_COPYCONTENTS, itemData,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const int arrData[])
{
    const char *itemName = strItem.c_str();
    int nSize = sizeof(arrData) / sizeof(int);
    if(nSize > 0)
    {
        struct curl_forms arrForms[nSize + 1];
        std::string strTempData;
        for(int i = 0; i < nSize; i++)
        {
            strTempData = std::to_string(arrData[i]);
            arrForms[i].option = CURLFORM_COPYCONTENTS;
            arrForms[i].value  = strTempData.c_str();
        }
        arrForms[nSize].option  = CURLFORM_END;

        curl_formadd(&m_pFirst, &m_pLast,
                     CURLFORM_COPYNAME, itemName,
                     CURLFORM_ARRAY, arrForms,
                     CURLFORM_END);
    }
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, int nArrCount, const char *arrData[])
{
    const char *itemName = strItem.c_str();
    if(nArrCount > 0)
    {
        struct curl_forms arrForms[nArrCount + 1];
        for(int i = 0; i < nArrCount; i++)
        {
            arrForms[i].option = CURLFORM_COPYCONTENTS;
            arrForms[i].value  = arrData[i];
        }
        arrForms[nArrCount].option  = CURLFORM_END;

        curl_formadd(&m_pFirst, &m_pLast,
                     CURLFORM_COPYNAME, itemName,
                     CURLFORM_ARRAY, arrForms,
                     CURLFORM_END);
    }
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const std::string arrData[])
{
    const char *itemName = strItem.c_str();
    int nSize = sizeof(arrData) / sizeof(std::string);
    if(nSize > 0)
    {
        struct curl_forms arrForms[nSize + 1];
        for(int i = 0; i < nSize; i++)
        {
            arrForms[i].option = CURLFORM_COPYCONTENTS;
            arrForms[i].value  = arrData[i].c_str();
        }
        arrForms[nSize].option  = CURLFORM_END;

        curl_formadd(&m_pFirst, &m_pLast,
                     CURLFORM_COPYNAME, itemName,
                     CURLFORM_ARRAY, arrForms,
                     CURLFORM_END);
    }
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const std::string &strFileName, void *buffer, int size)
{
    const char *itemName = strItem.c_str();
    const char *fileName = strFileName.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_BUFFER, fileName,
                 CURLFORM_BUFFERPTR, buffer,
                 CURLFORM_BUFFERLENGTH, size,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::add_formData(const std::string &strItem, const std::string &strFileName, void *buffer, std::size_t size)
{
    const char *itemName = strItem.c_str();
    const char *fileName = strFileName.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_BUFFER, fileName,
                 CURLFORM_BUFFERPTR, buffer,
                 CURLFORM_BUFFERLENGTH, size,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::add_file_formData(const std::string &strItem, const std::string strFileName)
{
    const char *itemName = strItem.c_str();
    const char *itemData = strFileName.c_str();
    curl_formadd(&m_pFirst, &m_pLast,
                 CURLFORM_COPYNAME, itemName,
                 CURLFORM_FILE, itemData,
                 CURLFORM_END);
}

void CCurlMultipartHttpPost::clear_form()
{
    curl_formfree(m_pFirst);
    m_pFirst = m_pLast = NULL;
}

void CCurlMultipartHttpPost::update_data()
{
    std::string strFullUri = m_url + m_strPaht;
    curl_easy_setopt(m_curl.get(), CURLOPT_URL, strFullUri.c_str());
}

int CCurlMultipartHttpPost::send_request()
{
    if (!m_curl)
    {
        return CURLE_URL_MALFORMAT;
    }

    /*清空接收缓冲区*/
    m_sstream.str("");

    /*更新一下请求路径*/
    update_data();

    /*设置表单*/
    curl_easy_setopt(m_curl.get(), CURLOPT_HTTPPOST, m_pFirst);

    int nRet = -1;
    for (int i = 0; (i < 5) && (nRet != 0); i++)
    {
        /* 发送请求 */
        nRet = curl_easy_perform(m_curl.get());
        if (CURLE_OK == nRet)
        {
            break;
        }
        else
        {
            /*
            dlog(LOG_ERROR, "http请求失败 错误码[%d] URL[http://%s%s?%s]",
            nRet, m_url.c_str(), m_strPaht.c_str(), m_strParams.c_str());
            */
        }
    }

    return nRet;
}
