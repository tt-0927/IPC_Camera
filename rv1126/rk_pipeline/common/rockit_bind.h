/*
 * @Author: leiyy leiyy@kfb.cn
 * @Date: 2025-08-19 10:05:56
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-08-22 17:30:16
 * @FilePath: /rv1126/rk_pipeline/common/rockit_bind.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*** 
 * @FilePath     : rockit_bind.h
 * @Author       : cyc
 * @Date         : 2025-03-21 10:22:58
 * @LastEditors  : cyc
 * @LastEditTime : 2025-04-03 15:14:19
 * @Description  : 
 */
/*************************************************************************
	> File Name: rockit_bind.h
	> Author:luoyk 
	> Mail: 
    > description: 媒体模块绑定接口
	> Created Time: 2022年05月19日 星期四 14时29分46秒
 ************************************************************************/

#ifndef _ROCKIT_BIND_H
#define _ROCKIT_BIND_H

#include "rk_mpi_sys.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_cal.h"
#include "rk_comm_video.h"
#include "rk_mpi_vo.h"

/*vdec 绑定 vo*/
int rockitVdec_bind_vo(int nVdecChnId, int nVoLayer, int nVoChnId );
int rockitVdec_unbind_vo(int nVdecChnId, int nVoLayer, int nVoChnId );

/*vdec 绑定 venc*/
int rockitVdec_bind_venc(int nVdecChnId, int nVencChnId );
int rockitVdec_unbind_venc(int nVdecChnId, int nVencChnId );

/*vdec 绑定 vpss*/
int rockitVdec_bind_vpss( int nVdecChnId, int nVpssGrp, int nVpssChn );
int rockitVdec_unbind_vpss( int nVdecChnId, int nVpssGrp, int nVpssChn );

/*vpss 绑定 venc*/
int rockitVpss_bind_venc( int nVpssGrp, int nVpssChn, int nVencchn);
int rockitVpss_unbind_venc( int nVpssGrp, int nVpssChn, int nVencchn);

/*vpss 绑定 vpss*/
int rockitVpss_bind_vpss(int nVpssGrp, int nVpssChn, int nDstVpssGrp, int nDstVpssChn);
int rockitVpss_unbind_vpss(int nVpssGrp, int nVpssChn, int nDstVpssGrp, int nDstVpssChn);

/*vpss 绑定 vo*/
int rockitVpss_bind_vo( int nVpssGrp, int nVpssChn, int nVoLayer, int nVochn);
int rockitVpss_unbind_vo( int nVpssGrp, int nVpssChn, int nVoLayer, int nVochn);

/*venc 绑定 vdec*/
int rockitVenc_bind_vdec( int nVencChnId, int nVdecChnId);
int rockitVenc_unbind_vdec( int nVencChnId, int nVdecChnId);

/*vi 绑定 venc*/
int rockitVi_bind_venc(int nViDevId, int nViChnId, int nVencChnId );
int rockitVi_unbind_venc(int nViDevId, int nViChnId, int nVencChnId );

/*vi 绑定 vpss*/
int rockitVi_bind_vpss(int nViDevId, int nViChnId, int nVpssGrp, int nVpssChn );
int rockitVi_unbind_vpss(int nViDevId, int nViChnId, int nVpssGrp, int nVpssChn );

/*vi 绑定 vo*/
int rockitVi_bind_vo(int nViDevId, int nViChnId, int nVoLayer, int nVoChnId );
int rockitVi_unbind_vo(int nViDevId, int nViChnId, int nVoLayer, int nVoChnId );

/*ai 绑定 aenc*/
int rockitAi_bind_aenc(int nAiDevId, int nAiChnId, int nAencDevId, int nAencChnId);
int rockitAi_unbind_aenc(int nAiDevId, int nAiChnId, int nAencDevId, int nAencChnId);

/*adec bind ao*/
int rockitAdec_bind_ao(int nAdecDevId, int nAdecChnId, int nAoDevId, int nAoChnId);
int rockitAdec_unbind_ao(int nAdecDevId, int nAdecChnId, int nAoDevId, int nAoChnId);

/*vi bind gdc*/
int rockitVi_bind_gdc(int nViDevId, int nViChnId, int nGdcDevId, int nGdcChnId);
int rockitVi_unbind_gdc(int nViDevId, int nViChnId, int nGdcDevId, int nGdcChnId);

/*gdc bind vpss*/
int rockitGdc_bind_vpss(int nGdcDevId, int nGdcChnId, int nVpssGrp, int nVpssChn);
int rockitGdc_unbind_vpss(int nGdcDevId, int nGdcChnId, int nVpssGrp, int nVpssChn);

#endif
