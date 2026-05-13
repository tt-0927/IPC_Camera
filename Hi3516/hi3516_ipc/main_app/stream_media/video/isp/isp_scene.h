/*
 * @FilePath     : isp_scene,h
 * @Author       : cyc
 * @Date         : 2025-08-08 15:44:01
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-08 15:46:28
 * @Description  : isp场景模块
 */

#pragma once
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "Singleton.h"
#include "isp_define.h"

extern "C" {
    #include "scene_loadparam.h"
    #include "ot_scene.h"
}

typedef struct _SceneConfig_S_ 
{
    std::string strConfigPath;      /* isp配置路径 */
    ot_scene_param stSceneParam;    /* 场景参数结构体 */
    ot_scene_video_mode stSceneMode; /* 场景模式 */
}SceneConfig_S;

class CSceneParamManager :public CSingleton<CSceneParamManager>
{
    public:
        friend class CSingleton<CSceneParamManager>;
        CSceneParamManager();
        ~CSceneParamManager();

        /*** 
        * @description : 场景初始化
        * @author      : cyc
        * @return       {*}
        */            
        bool scene_init(const std::string stConfigDir);

        /*** 
        * @description : 设置场景模式
        * @author      : cyc
        * @param        {SceneType_E} enSceneType  场景类型
        * @return       {*}
        */            
        int scene_set_mode(ISP::SceneType_E enSceneType);

        /*** 
        * @description : 暂停场景设置
        * @author      : cyc
        * @param        {bool} bIsPause
        * @return       {*}
        */            
        bool scene_pause(bool bIsPause);

        /*** 
        * @description : 场景去初始化
        * @author      : cyc
        * @return       {*}
        */            
        bool scene_deinit();

        /*** 
         * @description : 获取当前场景
         * @author      : cyc
         * @return       SceneType_E
         */        
         ISP::SceneType_E scene_get_mode();
    
    private:
        SceneConfig_S m_stSceneConfig;
        /* 场景初始化标志 */
        bool m_bInit = false;
        /* 场景暂停标志 */
        bool m_bPaused = true;
        /* 当前场景模式 */
        ISP::SceneType_E enCurrentSceneMode = ISP::SCENE_NORMAL;
};
    

