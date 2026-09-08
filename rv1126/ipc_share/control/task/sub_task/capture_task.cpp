/**
 * @FilePath     : capture_task.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-15 17:30:46
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-07 09:53:23
 * @Description  : 抓图任务
 */

#include "capture_task.h"
#include "convert_interface.h"
#include "action_code.h"
#include "capture_configure.h"
#include "capture_ctrl.h"
#include "capture_param_validation.h"
#include <memory>

/* 修改记录：2026-09-08，保存前严格校验两组抓图参数，拒绝非法类型及越界值。 */

/*获取抓图计划*/
void Task::Capture::GetCapturePlanInfo::handle()
{
    Capture_NS::CapturePlan_S stCapturePlan;
    CCaptureConfigure::instance()->get_configure(stCapturePlan);
    result(Convert::to_string(stCapturePlan));
}

/*设置抓图计划*/
void Task::Capture::SetCapturePlanInfo::handle()
{
    Capture_NS::CapturePlan_S stCapturePlan;
    Convert::to_struct(m_taskData, stCapturePlan);
    int nRet = CCaptureConfigure::instance()->set_configure(stCapturePlan);
    CCaptureCtrl::instance()->update_capturePlan();
    result(nRet);
}

/*获取抓图参数*/
void Task::Capture::GetCaptureParamInfo::handle()
{
    Capture_NS::CaptureParam_S stCaptureParam;
    CCaptureCtrl::instance()->get_captureParam(stCaptureParam);
    result(Convert::to_string(stCaptureParam));
}

/*
 * @brief 校验并保存抓图参数；校验失败时不保存配置、不刷新抓图状态。
 * @author Codex
 * @param [in] 无，读取当前任务的 m_taskData。
 * @param [out] 无，通过 result 返回任务结果。
 * @return 无返回值；参数不合法时任务返回 ERR_WEB_PARAM。
 */
void Task::Capture::SetCaptureParamInfo::handle()
{
    std::unique_ptr<Json::Object, decltype(&cJSON_Delete)> pRoot(Json::init(m_taskData), &cJSON_Delete);
    if (!CaptureValidation::capture_validateParams(pRoot.get()))
    {
        dlog_error("设置抓图参数失败：字段缺失、类型错误或数值超出允许范围");
        result(ERR_WEB_PARAM);
        return;
    }
    Capture_NS::CaptureParam_S stCaptureParam = {};
    Convert::to_struct(m_taskData, stCaptureParam);
    const int nRet = CCaptureConfigure::instance()->set_configure(stCaptureParam);
    if (nRet == 0)
    {
        CCaptureCtrl::instance()->update_captureParam();
    }
    result(nRet);
}
