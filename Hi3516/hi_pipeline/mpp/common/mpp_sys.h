/**
 * @FilePath     : mpp_sys.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 16:30:25
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-17 14:41:23
 * @Description  : mpp系统控制模块接口
 */

#ifndef _MPP_SYS_H_
#define _MPP_SYS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "ot_common_sys.h"
#include "ot_common_vb.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_vb.h"
#include "ot_common_sys_mem.h"
#include "ss_mpi_sys_mem.h"
#include "ss_mpi_audio.h"

#include "mpi_common.h"

/*DDR名字，从哪个MMZ区域分配内存*/
#define MMZ_NAME "anonymous"

/**
 * @brief       : 初始化MPP系统
 * @author      : zhouzirui
 * @return       {int}成功返回0,失败返回-1
 */
int mppSys_init();

/**
 * @brief       : 去初始化MPP系统
 * @author      : zhouzirui
 * @return       {int}成功返回0,失败返回-1
 */
int mppSys_uninit();

/**
 * @brief       : 设置MPP视频缓存池属性
 * @author      : zhouzirui
 * @param        {ot_vb_cfg} stVbCfg：视频缓存池属性
 * @return       {int}成功返回0,失败返回-1
 */
int mppVb_set_cfg(ot_vb_cfg stVbCfg);

/**
 * @brief       : 获取MPP视频缓存池属性
 * @author      : zhouzirui
 * @param        {ot_vb_cfg} *pVbCfg：视频缓存池属性指针
 * @return       {int}成功返回0,失败返回-1
 */
int mppVb_get_cfg(ot_vb_cfg *pVbCfg);

/**
 * @brief       : 初始化MPP视频缓存池
 * @author      : zhouzirui
 * @return       {int}成功返回0,失败返回-1
 */
int mppVb_init();

/**
 * @brief       : 去初始化MPP视频缓存池
 * @author      : zhouzirui
 * @return       {int}成功返回0,失败返回-1
 */
int mppVb_uninit();

/**
 * @brief   : 初始化Audio模块
 * @return   {int}成功返回0,失败返回-1
 */
int mppAudio_init();

/**
 * @brief   : 去初始化Audio模块
 * @return   {int}成功返回0,失败返回-1
 */
int mppAudio_uninit();

#ifdef __cplusplus
}
#endif
#endif //_MPP_SYS_H_