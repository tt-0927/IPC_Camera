/*** 
 * @FilePath     : production_test_task.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-08 11:25:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-08 11:25:00
 * @Description  : 产测任务
 */

#include "production_test_task.h"
#include "ProductionTestManager.h"
#include "ProductionUploader.h"
#include "action_code.h"
#include "dlog.h"

void Task::ProductionTest::GetItems::handle()
{
    /* 获取产测项目列表 */
    std::string strJson = ProductionTestManager::instance()->getTestResult();
    result(strJson, 0);
}

void Task::ProductionTest::GetResult::handle()
{
    /* 获取产测结果 */
    std::string strJson = ProductionTestManager::instance()->getTestResult();
    result(strJson, 0);
}

void Task::ProductionTest::SaveResult::handle()
{
    /* 保存产测结果 */
    int nRet = ProductionTestManager::instance()->saveTestResult(m_taskData);
    if (nRet != 0)
    {
        dlog_error("保存产测结果失败");
    }
    result(nRet);
}

void Task::ProductionTest::UploadResult::handle()
{
    /* 获取上传数据 */
    std::string strJson = ProductionTestManager::instance()->getUploadData();
    if (strJson.empty() || strJson == "{}")
    {
        dlog_error("没有产测数据可上传");
        result(-1);
        return;
    }

    /* 上传到运维平台 */
    int nRet = ProductionUploader::upload(strJson);
    if (nRet != 0)
    {
        dlog_error("上传产测结果到运维平台失败");
    }
    result(nRet);
}

void Task::ProductionTest::ResetResult::handle()
{
    /* 重置产测结果 */
    int nRet = ProductionTestManager::instance()->resetTestResult();
    result(nRet);
}
