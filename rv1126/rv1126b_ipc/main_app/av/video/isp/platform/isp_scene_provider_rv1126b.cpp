/**
 * @FilePath     : isp_scene_provider_rv1126b.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:51:17
 * @Description  : RV1126B ISP场景适配端口实现
 */

#include "isp_scene_provider_rv1126b.h"

#include "IpcRet.h"
#include "isp_scene.h"
#include "path_define.h"

int CIspSceneProviderRv1126b::init()
{
    /* step: 共享service初始化前确认RK AIQ上下文已由CIspControl建立。 */
    return CSceneParamManager::instance()->scene_init(ISP_CONFIG_PATH) ? OK : ERR;
}

int CIspSceneProviderRv1126b::deinit()
{
    /* memory: 只释放场景管理器状态，不释放RK AIQ；AIQ由CIspControl统一管理。 */
    return CSceneParamManager::instance()->scene_deinit() ? OK : ERR;
}

int CIspSceneProviderRv1126b::apply_scene(ISP::IspRuntimeScene_E enRuntimeScene)
{
    /* note: 场景管理器负责调用normal/day，适配器不直接操作IQ或补光。 */
    return CSceneParamManager::instance()->scene_set_mode(enRuntimeScene);
}
