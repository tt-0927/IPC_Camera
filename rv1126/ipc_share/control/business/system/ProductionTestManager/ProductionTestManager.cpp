/*** 
 * @FilePath     : ProductionTestManager.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-08 11:25:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-08 11:25:00
 * @Description  : 产测管理
 */

#include "ProductionTestManager.h"
#include "dlog.h"
#include <fstream>
#include <sstream>
#include <ctime>

static std::string getCurrentTimeStr()
{
    time_t nNow = time(nullptr);
    struct tm stTM;
    localtime_r(&nNow, &stTM);
    char achBuf[32] = {0};
    strftime(achBuf, sizeof(achBuf), "%Y-%m-%d %H:%M:%S", &stTM);
    return std::string(achBuf);
}

bool ProductionTestManager::loadResults()
{
    std::ifstream file(PRODUCTION_TEST_CONFIG_FILE, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    Json::Object *pRoot = Json::init(buffer.str());
    if (!pRoot)
    {
        return false;
    }

    Json::Object *pResults = Json::get(pRoot, "results");
    if (pResults)
    {
        int nSize = Json::Array::size(pResults);
        for (int i = 0; i < nSize; i++)
        {
            Json::Object *pItem = Json::Array::get(pResults, i);
            if (pItem)
            {
                std::string strItem = Json::to_string(pItem);
                Json::Object *pClone = Json::init(strItem);
                if (pClone)
                {
                    m_vecResults.push_back(pClone);
                }
            }
        }
    }

    Json::get(pRoot, "tester", m_strTester);

    Json::deinit(pRoot);
    return true;
}

bool ProductionTestManager::saveResults()
{
    Json::Object *pRoot = Json::init();
    if (!pRoot)
    {
        return false;
    }

    Json::Object *pArr = Json::Array::init();
    for (auto &pItem : m_vecResults)
    {
        if (pItem)
        {
            std::string strItem = Json::to_string(pItem);
            Json::Object *pClone = Json::init(strItem);
            if (pClone)
            {
                Json::Array::add(pArr, pClone);
            }
        }
    }
    Json::add(pRoot, "results", pArr);
    if (!m_strTester.empty())
    {
        Json::add(pRoot, "tester", m_strTester);
    }

    std::string strJson = Json::to_string(pRoot);
    Json::deinit(pRoot);

    std::string tmpPath = std::string(PRODUCTION_TEST_CONFIG_FILE) + ".tmp";
    {
        std::ofstream file(tmpPath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            return false;
        }
        file << strJson << std::endl;
        file.flush();
        file.close();
    }
    ::rename(tmpPath.c_str(), PRODUCTION_TEST_CONFIG_FILE);
    return true;
}

std::string ProductionTestManager::getTestResult()
{
    m_vecResults.clear();
    loadResults();

    Json::Object *pRoot = Json::init();
    if (!pRoot)
    {
        return "{}";
    }

    Json::Object *pArr = Json::Array::init();
    int nPassed = 0;
    int nFailed = 0;
    int nUnfilled = 0;
    for (auto &pItem : m_vecResults)
    {
        if (pItem)
        {
            std::string strItem = Json::to_string(pItem);
            Json::Object *pClone = Json::init(strItem);
            if (pClone)
            {
                int nPass = -1;
                Json::get(pClone, "passed", nPass);
                if (nPass > 0)
                {
                    nPassed++;
                }
                else if (nPass == 0)
                {
                    nFailed++;
                }
                else
                {
                    nUnfilled++;
                }
                Json::Array::add(pArr, pClone);
            }
        }
    }
    Json::add(pRoot, "results", pArr);
    Json::add(pRoot, "total", (int)m_vecResults.size());
    Json::add(pRoot, "passed", nPassed);
    Json::add(pRoot, "failed", nFailed);
    Json::add(pRoot, "unfilled", nUnfilled);
    Json::add(pRoot, "finished", (int)m_vecResults.size() > 0 ? 1 : 0);
    Json::add(pRoot, "tester", m_strTester);
    Json::add(pRoot, "lastUpdateTime", m_vecResults.empty() ? "" : getCurrentTimeStr());

    std::string result = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return result;
}

static void saveSingleResult(Json::Object *pItem, std::vector<Json::Object *> &vecResults)
{
    std::string strCategoryId, strItemId;
    int nPassed = 0;
    std::string strNote;

    Json::get(pItem, "categoryId", strCategoryId);
    Json::get(pItem, "itemId", strItemId);
    Json::get(pItem, "passed", nPassed);
    Json::get(pItem, "note", strNote);

    if (strItemId.empty())
    {
        return;
    }

    /* 查找并更新已存在的测试项 */
    for (auto it = vecResults.begin(); it != vecResults.end(); ++it)
    {
        if (*it == nullptr)
        {
            continue;
        }
        std::string strCatId, strItId;
        Json::get(*it, "categoryId", strCatId);
        Json::get(*it, "itemId", strItId);
        if (strCatId == strCategoryId && strItId == strItemId)
        {
            Json::update(*it, "passed", nPassed);
            Json::update(*it, "note", strNote);
            Json::update(*it, "timestamp", getCurrentTimeStr());
            return;
        }
    }

    /* 未找到则新增 */
    Json::Object *pNew = Json::init();
    Json::add(pNew, "categoryId", strCategoryId);
    Json::add(pNew, "itemId", strItemId);
    Json::add(pNew, "passed", nPassed);
    Json::add(pNew, "note", strNote);
    Json::add(pNew, "timestamp", getCurrentTimeStr());
    vecResults.push_back(pNew);
}

int ProductionTestManager::saveTestResult(const std::string &strJsonData)
{
    m_vecResults.clear();
    loadResults();

    Json::Object *pInput = Json::init(strJsonData);
    if (!pInput)
    {
        return -1;
    }

    /* 从Data中提取产测人员 */
    std::string strTester;
    Json::get(pInput, "tester", strTester);
    if (!strTester.empty())
    {
        m_strTester = strTester;
    }

    /* 从 results 数组中提取数据 */
    Json::Object *pResults = Json::get(pInput, "results");
    if (!pResults)
    {
        Json::deinit(pInput);
        return -1;
    }

    int nSize = Json::Array::size(pResults);
    for (int i = 0; i < nSize; i++)
    {
        Json::Object *pItem = Json::Array::get(pResults, i);
        if (pItem)
        {
            saveSingleResult(pItem, m_vecResults);
        }
    }

    Json::deinit(pInput);

    if (!saveResults())
    {
        return -1;
    }

    return 0;
}

int ProductionTestManager::resetTestResult()
{
    m_vecResults.clear();
    m_strTester.clear();
    if (saveResults())
    {
        return 0;
    }
    return -1;
}

std::string ProductionTestManager::getUploadData()
{
    m_vecResults.clear();
    loadResults();

    Json::Object *pRoot = Json::init();
    if (!pRoot)
    {
        return "{}";
    }

    Json::Object *pResults = Json::Array::init();
    int nPassed = 0;
    int nFailed = 0;
    int nUnfilled = 0;
    for (auto &pItem : m_vecResults)
    {
        if (!pItem)
        {
            continue;
        }
        std::string strItemJson = Json::to_string(pItem);
        Json::Object *pClone = Json::init(strItemJson);
        if (pClone)
        {
            int nPass = -1;
            Json::get(pClone, "passed", nPass);
            if (nPass > 0)
            {
                nPassed++;
            }
            else if (nPass == 0)
            {
                nFailed++;
            }
            else
            {
                nUnfilled++;
            }
            Json::Array::add(pResults, pClone);
        }
    }

    Json::add(pRoot, "results", pResults);
    Json::add(pRoot, "total", (int)m_vecResults.size());
    Json::add(pRoot, "passed", nPassed);
    Json::add(pRoot, "failed", nFailed);
    Json::add(pRoot, "unfilled", nUnfilled);
    Json::add(pRoot, "tester", m_strTester);
    Json::add(pRoot, "timestamp", getCurrentTimeStr());

    std::string result = Json::to_string(pRoot);
    Json::deinit(pRoot);
    return result;
}
