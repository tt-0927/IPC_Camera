/*** 
 * @FilePath     : isp_scene.cpp
 * @Author       : cyc
 * @Date         : 2025-08-08 15:43:47
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-06 11:04:39
 * @Description  : isp场景模块
 */

 #include "isp_scene.h"
 #include <iostream>
 #include <filesystem>
 #include <fstream>
 #include <cstring>
 #include "dlog.h"
 #include "IpcRet.h"
  

  
CSceneParamManager::CSceneParamManager() 
{

}
  
CSceneParamManager::~CSceneParamManager() 
{
    if (m_bInit) 
    {
        scene_deinit();
    }
}
  
bool CSceneParamManager::scene_init(const std::string stConfigDir) 
{
    dlog_info("scene_init successfully");
    m_bInit = true;

    return true;
}
  
int CSceneParamManager::scene_set_mode(ISP::SceneType_E enSceneType) 
{
    return OK;
}

ISP::SceneType_E CSceneParamManager::scene_get_mode() 
{
    if (!m_bInit) 
    {
        dlog_error("Scene not initialized");
        return enCurrentSceneMode;
    }

    return enCurrentSceneMode;
}
  
bool CSceneParamManager::scene_pause(bool bIsPause) 
{
    return true;
}
  
bool CSceneParamManager::scene_deinit() 
{
    return true;
}
  
  

  
