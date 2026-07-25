/*** 
 * @FilePath     : isp_scene.cpp
 * @Author       : cyc
 * @Date         : 2025-08-08 15:43:47
 * @LastEditors  : cyc
 * @LastEditTime : 2026-01-28 15:33:33
 * @Description  : isp场景模块
 */

 #include "isp_scene.h"
 #include <iostream>
 #include <filesystem>
 #include <fstream>
 #include <cstring>
 #include "dlog.h"
 #include "IpcRet.h"
 #include "ss_mpi_isp.h"
  

  
CSceneParamManager::CSceneParamManager() 
{
    memset(&m_stSceneConfig.stSceneParam, 0, sizeof(ot_scene_param));
    memset(&m_stSceneConfig.stSceneMode, 0, sizeof(ot_scene_video_mode));
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
    if (stConfigDir.empty()) 
    {
        dlog_error("Config directory path is empty");
        return false;
    }
  
    m_stSceneConfig.strConfigPath = stConfigDir;
     
     /* 加载isp参数 */ 
    int nRet = ot_scene_create_param(stConfigDir.c_str(), 
                                    &m_stSceneConfig.stSceneParam, 
                                    &m_stSceneConfig.stSceneMode);
     
    if(nRet != OK)
    {
        dlog_error("Failed to load scene parameters:%u",nRet);
        return false;
    }
    
    nRet = ot_scene_init(&m_stSceneConfig.stSceneParam);
    if (nRet != OK) 
    {
        dlog_error("Scene initialization failed %u",nRet);
        return false;
    }

    dlog_info("scene_init successfully");
    m_bInit = true;

    return true;
}
  
int CSceneParamManager::scene_set_mode(ISP::SceneType_E enSceneType) 
{
    if (!m_bInit) 
    {
        dlog_error("Scene not initialized");
        return ERR;
    }

    int nIndex;

    if(enSceneType == ISP::SceneType_E::SCENE_NIGHT)
    {
        nIndex = 1;
    }
#if CAP_ISP_SCENE_LIGHT_PARAM
    else if(enSceneType == ISP::SceneType_E::SCENE_NIGHT_LIGHT)
    {
        /* TV-3852H* 系列夜间白光模式使用独立 light 场景参数，避免复用白天参数。 */
        nIndex = 2;
    }
#endif
    else
    {
        nIndex = 0;
    }

    int nRet = ot_scene_set_scene_mode(&m_stSceneConfig.stSceneMode.video_mode[nIndex]);
    if (nRet != OK) 
    {
        dlog_error("Failed to set scene mode %u",nRet);
        return ERR;
    }
    #ifdef SENSOR_SC500AI
    /* 暂时强制关闭，后续继续优化好 */
    if(nIndex == 0)
    {
        ot_isp_drc_attr drc_attr;
        if(ss_mpi_isp_get_drc_attr(0,&drc_attr) != OK)
        {
            dlog_error("ss_mpi_isp_get_drc_attr error");
            return ERR;
        }
        drc_attr.enable = TD_FALSE;

        if(ss_mpi_isp_set_drc_attr(0,&drc_attr) != OK)
        {
            dlog_error("ss_mpi_isp_set_drc_attr error");
            return ERR;
        }
    }
    #else 
#if  DEVICE_TV_3852TL4G || DEVICE_TV_3852TLW
    if(nIndex == 0)
    {
        ot_isp_drc_attr drc_attr;
        if(ss_mpi_isp_get_drc_attr(0,&drc_attr) != OK)
        {
            dlog_error("ss_mpi_isp_get_drc_attr error");
            return ERR;
        }
        drc_attr.enable = TD_FALSE;

        if(ss_mpi_isp_set_drc_attr(0,&drc_attr) != OK)
        {
            dlog_error("ss_mpi_isp_set_drc_attr error");
            return ERR;
        }
    }
    else if (nIndex == 1 ) {
        ot_isp_drc_attr drc_attr;
        if(ss_mpi_isp_get_drc_attr(0,&drc_attr) != OK)
        {
            dlog_error("ss_mpi_isp_get_drc_attr error");
            return ERR;
        }
        drc_attr.enable = TD_TRUE;

        if(ss_mpi_isp_set_drc_attr(0,&drc_attr) != OK)
        {
            dlog_error("ss_mpi_isp_set_drc_attr error");
            return ERR;
        }
    }
#else    
    if(nIndex == 0)
    {
        ot_isp_drc_attr drc_attr;
        if(ss_mpi_isp_get_drc_attr(0,&drc_attr) != OK)
        {
            dlog_error("ss_mpi_isp_get_drc_attr error");
            return ERR;
        }
        drc_attr.enable = TD_TRUE;

        if(ss_mpi_isp_set_drc_attr(0,&drc_attr) != OK)
        {
            dlog_error("ss_mpi_isp_set_drc_attr error");
            return ERR;
        }
    }
#endif
#endif

    // scene_pause(false);

    enCurrentSceneMode = enSceneType;
    return nRet;
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
    if (!m_bInit) 
    {
        dlog_error("Scene not initialized");
        return false;
    }

    int nRet = ot_scene_pause((td_bool)bIsPause);
    if (nRet != OK) 
    {
        dlog_error("Failed to %s",(bIsPause ? "pause" : "resume"));
        return false;
    }
    m_bPaused = bIsPause;
    return true;
}
  
bool CSceneParamManager::scene_deinit() 
{
    if (!m_bInit) 
    {
        return true; 
    }

    int nRet = ot_scene_deinit();
    if (nRet != OK) 
    {
        dlog_error("Scene deinitialization failed: %u",nRet);
        return false;
    }

    m_bInit = false;
    m_bPaused = true;
    return true;
}
  
  

  
