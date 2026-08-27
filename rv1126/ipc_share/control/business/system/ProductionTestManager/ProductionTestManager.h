/*** 
 * @FilePath     : ProductionTestManager.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-08 11:25:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-08 11:25:00
 * @Description  : 产测管理
 */

#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "Singleton.h"
#include "Json.h"
#include "path_define.h"

class ProductionTestManager : public CSingleton<ProductionTestManager>
{
    ProductionTestManager() = default;
public:
    ~ProductionTestManager() = default;
    friend class CSingleton<ProductionTestManager>;

    /**
     * @brief  获取测试结果
     * @return std::string JSON格式的测试结果
     */
    std::string getTestResult();

    /**
     * @brief  保存测试结果
     * @param  strJsonData JSON格式的测试数据
     * @return int 0成功，非0失败
     */
    int saveTestResult(const std::string &strJsonData);

    /**
     * @brief  重置测试结果
     * @return int 0成功，非0失败
     */
    int resetTestResult();

    /**
     * @brief  获取上传数据
     * @return std::string JSON格式的上传数据
     */
    std::string getUploadData();

private:
    /**
     * @brief  从文件加载测试结果
     * @return bool true成功，false失败
     */
    bool loadResults();

    /**
     * @brief  保存测试结果到文件
     * @return bool true成功，false失败
     */
    bool saveResults();

    std::vector<Json::Object *> m_vecResults;
    std::string m_strTester;
};
