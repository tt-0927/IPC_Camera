/**
 * @FilePath     : mpp_bind.h
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 16:24:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-18 20:02:06
 * @Description  : mpp模块绑定接口
 */

#ifndef _MPP_BIND_H_
#define _MPP_BIND_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "ot_common_sys_bind.h"
#include "ss_mpi_sys_bind.h"

#include "mpi_common.h"

/**
 * @brief       : vi 绑定 venc
 * @author      : zhouzirui
 * @param        {int} nViDevId
 * @param        {int} nViChnId
 * @param        {int} nVencChnId
 * @return       {*}成功返回0,失败返回-1
 */
int mppVi_bind_venc(int nViDevId, int nViChnId, int nVencChnId);

/**
 * @brief       : vi 解绑 venc
 * @author      : zhouzirui
 * @param        {int} nViDevId
 * @param        {int} nViChnId
 * @param        {int} nVencChnId
 * @return       {*}成功返回0,失败返回-1
 */
int mppVi_unbind_venc(int nViDevId, int nViChnId, int nVencChnId);

/**
 * @brief       : vi 绑定 vpss
 * @author      : zhouzirui
 * @param        {int} nViDevId
 * @param        {int} nViChnId
 * @param        {int} nVpssGrp
 * @param        {int} nVpssChn
 * @return       {*}成功返回0,失败返回-1
 */
int mppVi_bind_vpss(int nViDevId, int nViChnId, int nVpssGrp, int nVpssChn);

/**
 * @brief       : vi 解绑 vpss
 * @author      : zhouzirui
 * @param        {int} nViDevId
 * @param        {int} nViChnId
 * @param        {int} nVpssGrp
 * @param        {int} nVpssChn
 * @return       {*}成功返回0,失败返回-1
 */
int mppVi_unbind_vpss(int nViDevId, int nViChnId, int nVpssGrp, int nVpssChn);

/**
 * @brief       : vi 绑定 vo
 * @author      : zhouzirui
 * @param        {int} nViDevId
 * @param        {int} nViChnId
 * @param        {int} nVoLayer
 * @param        {int} nVoChnId
 * @return       {*}成功返回0,失败返回-1
 */
int mppVi_bind_vo(int nViDevId, int nViChnId, int nVoLayer, int nVoChnId);

/**
 * @brief       : vi 解绑 vo
 * @author      : zhouzirui
 * @param        {int} nViDevId
 * @param        {int} nViChnId
 * @param        {int} nVoLayer
 * @param        {int} nVoChnId
 * @return       {*}成功返回0,失败返回-1
 */
int mppVi_unbind_vo(int nViDevId, int nViChnId, int nVoLayer, int nVoChnId);

/**
 * @brief       : vpss 绑定 vpss
 * @author      : zhouzirui
 * @param        {int} nVpssGrp
 * @param        {int} nVpssChn
 * @param        {int} nDstVpssGrp
 * @param        {int} nDstVpssChn
 * @return       {*}成功返回0,失败返回-1
 */
int mppVpss_bind_vpss(int nVpssGrp, int nVpssChn, int nDstVpssGrp, int nDstVpssChn);

/**
 * @brief       : vpss 解绑 vpssc
 * @author      : zhouzirui
 * @param        {int} nVpssGrp
 * @param        {int} nVpssChn
 * @param        {int} nDstVpssGrp
 * @param        {int} nDstVpssChn
 * @return       {*}成功返回0,失败返回-1
 */
int mppVpss_unbind_vpss(int nVpssGrp, int nVpssChn, int nDstVpssGrp, int nDstVpssChn);

/**
 * @brief       : vpss 绑定 vo
 * @author      : zhouzirui
 * @param        {int} nVpssGrp
 * @param        {int} nVpssChn
 * @param        {int} nVoLayer
 * @param        {int} nVoChn
 * @return       {*}成功返回0,失败返回-1
 */
int mppVpss_bind_vo(int nVpssGrp, int nVpssChn, int nVoLayer, int nVoChn);

/**
 * @brief       : vpss 解绑 vo
 * @author      : zhouzirui
 * @param        {int} nVpssGrp
 * @param        {int} nVpssChn
 * @param        {int} nVoLayer
 * @param        {int} nVoChn
 * @return       {*}成功返回0,失败返回-1
 */
int mppVpss_unbind_vo(int nVpssGrp, int nVpssChn, int nVoLayer, int nVoChn);

/**
 * @brief       : vpss 绑定 venc
 * @author      : zhouzirui
 * @param        {int} nVpssGrp
 * @param        {int} nVpssChn
 * @param        {int} nVencChn
 * @return       {*}成功返回0,失败返回-1
 */
int mppVpss_bind_venc(int nVpssGrp, int nVpssChn, int nVencChn);

/**
 * @brief       : vpss 解绑 venc
 * @author      : zhouzirui
 * @param        {int} nVpssGrp
 * @param        {int} nVpssChn
 * @param        {int} nVencChn
 * @return       {*}成功返回0,失败返回-1
 */
int mppVpss_unbind_venc(int nVpssGrp, int nVpssChn, int nVencChn);

/**
 * @brief   : venc 绑定 venc
 * @note    : 内部缩放功能
 * @param    {int} nSrcVencChn
 * @param    {int} nDstVencChn
 * @return   {int} 成功返回0,失败返回-1
 */
int mppVenc_bind_venc(int nSrcVencChn, int nDstVencChn);

/**
 * @brief   : venc 解绑 venc
 * @note    : 内部缩放功能
 * @param    {int} nSrcVencChn
 * @param    {int} nDstVencChn
 * @return   {int} 成功返回0,失败返回-1
 */
int mppVenc_unbind_venc(int nSrcVencChn, int nDstVencChn);

/**
 * @brief       : ai 绑定 aenc
 * @author      : zhouzirui
 * @param        {int} nAiDev
 * @param        {int} nAiChn
 * @param        {int} nAencChn
 * @return       {*}
 */
int mppAi_bind_aenc(int nAiDev, int nAiChn, int nAencChn);

/**
 * @brief       : ai 解绑 aenc
 * @author      : zhouzirui
 * @param        {int} nAiDev
 * @param        {int} nAiChn
 * @param        {int} nAencChn
 * @return       {*}
 */
int mppAi_unbind_aenc(int nAiDev, int nAiChn, int nAencChn);

/**
 * @brief       : adec 绑定 ao
 * @author      : zhouzirui
 * @param        {int} nAdecChn
 * @param        {int} nAoDev
 * @param        {int} nAoChn
 * @return       {*}
 */
int mppAdec_bind_ao(int nAdecChn, int nAoDev, int nAoChn);

/**
 * @brief       : adec 解绑 ao
 * @author      : zhouzirui
 * @param        {int} nAdecChn
 * @param        {int} nAoDev
 * @param        {int} nAoChn
 * @return       {*}
 */
int mppAdec_unbind_ao(int nAdecChn, int nAoDev, int nAoChn);

#ifdef __cplusplus
}
#endif
#endif //_MPP_BIND_H_