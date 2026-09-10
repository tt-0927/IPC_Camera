/**
 * @FilePath     : isp_config_repository.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 11:53:59
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : ISP配置仓储CIspConfigure包装实现
 */

#include "isp_config_repository.h"

#include <cstddef>
#include <variant>

#include "IpcRet.h"
#include "isp_configure.h"

int CIspConfigRepository::load(ISP::IspConfigValue_T &stValue) const
{
    /* variant 当前备选类型就是读取分派标签，避免仓储暴露多组重复的 typed 接口。 */
    return std::visit(
        [](auto &stConfig) -> int
        {
            return CIspConfigure::instance()->get_configure(stConfig);
        },
        stValue);
}

int CIspConfigRepository::save(const ISP::IspConfigValue_T &stValue)
{
    /* 以 variant 类型选择对应持久化重载；不在此层做参数校验或硬件下发。 */
    return std::visit(
        [](const auto &stConfig) -> int
        {
            return CIspConfigure::instance()->set_configure(stConfig);
        },
        stValue);
}

int CIspConfigRepository::load_all_scene_params(ISP::AllSceneParams_S &stConfig) const
{
    return CIspConfigure::instance()->get_configure(stConfig);
}

int CIspConfigRepository::save_all_scene_params(const ISP::AllSceneParams_S &stConfig)
{
    return CIspConfigure::instance()->set_configure(stConfig);
}

int CIspConfigRepository::load_scene_params(ISP::SceneType_E enScene, ISP::SceneParams_S &stConfig) const
{
    if (enScene < ISP::SCENE_NORMAL || enScene >= ISP::SCENE_MAX)
    {
        return ERR_PARAM;
    }

    ISP::AllSceneParams_S stAllParams;
    int nRet = load_all_scene_params(stAllParams);
    if (nRet != OK)
    {
        return nRet;
    }

    const std::size_t nSceneIndex = static_cast<std::size_t>(enScene);
    if (nSceneIndex >= stAllParams.aSceneParams.size())
    {
        return ERR_PARAM;
    }

    stConfig = stAllParams.aSceneParams[nSceneIndex];
    stConfig.enSceneType = enScene;
    return OK;
}

int CIspConfigRepository::restore_defaults()
{
    return CIspConfigure::instance()->set_configure();
}
