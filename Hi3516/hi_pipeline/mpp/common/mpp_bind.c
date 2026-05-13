/**
 * @FilePath     : mpp_bind.c
 * @Author       : zhouzirui
 * @Date         : 2025-03-21 16:24:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-18 20:22:52
 * @Description  : mpp模块绑定接口
 */

#include "mpp_bind.h"

int mppVi_bind_venc(int nViDevId, int nViChnId, int nVencChnId)
{
    int nRet = 0;
    ot_mpp_chn stSrcChn, stDstChn;
    stSrcChn.mod_id = OT_ID_VI;
    stSrcChn.dev_id = nViDevId;
    stSrcChn.chn_id = nViChnId;

    stDstChn.mod_id = OT_ID_VENC;
    stDstChn.dev_id = 0;
    stDstChn.chn_id = nVencChnId;
    nRet = ss_mpi_sys_bind(&stSrcChn, &stDstChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vi and venc bind error %x\n", nRet);
    }
    return nRet;
}

int mppVi_unbind_venc(int nViDevId, int nViChnId, int nVencChnId)
{
    int nRet = 0;
    ot_mpp_chn stSrcChn, stDstChn;
    stSrcChn.mod_id = OT_ID_VI;
    stSrcChn.dev_id = nViDevId;
    stSrcChn.chn_id = nViChnId;

    stDstChn.mod_id = OT_ID_VENC;
    stDstChn.dev_id = 0;
    stDstChn.chn_id = nVencChnId;
    nRet = ss_mpi_sys_unbind(&stSrcChn, &stDstChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vi and venc bind error %x\n", nRet);
    }
    return nRet;
}

int mppVi_bind_vpss(int nViDevId, int nViChnId, int nVpssGrp, int nVpssChn)
{
    int nRet = 0;
    ot_mpp_chn stSrcChn, stDstChn;
    stSrcChn.mod_id = OT_ID_VI;
    stSrcChn.dev_id = nViDevId;
    stSrcChn.chn_id = nViChnId;

    stDstChn.mod_id = OT_ID_VPSS;
    stDstChn.dev_id = nVpssGrp;
    stDstChn.chn_id = nVpssChn;
    nRet = ss_mpi_sys_bind(&stSrcChn, &stDstChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vi and vpss bind error %x\n", nRet);
    }
    return nRet;
}

int mppVi_unbind_vpss(int nViDevId, int nViChnId, int nVpssGrp, int nVpssChn)
{
    int nRet = 0;
    ot_mpp_chn stSrcChn, stDstChn;
    stSrcChn.mod_id = OT_ID_VI;
    stSrcChn.dev_id = nViDevId;
    stSrcChn.chn_id = nViChnId;

    stDstChn.mod_id = OT_ID_VPSS;
    stDstChn.dev_id = nVpssGrp;
    stDstChn.chn_id = nVpssChn;
    nRet = ss_mpi_sys_unbind(&stSrcChn, &stDstChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vi and vpss bind error %x\n", nRet);
    }
    return nRet;
}

int mppVi_bind_vo(int nViDevId, int nViChnId, int nVoLayer, int nVoChnId)
{
    int nRet = 0;
    ot_mpp_chn stSrcChn, stDstChn;
    stSrcChn.mod_id = OT_ID_VI;
    stSrcChn.dev_id = nViDevId;
    stSrcChn.chn_id = nViChnId;

    stDstChn.mod_id = OT_ID_VO;
    stDstChn.dev_id = nVoLayer;
    stDstChn.chn_id = nVoChnId;
    nRet = ss_mpi_sys_bind(&stSrcChn, &stDstChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vi and vo bind error %x\n", nRet);
    }
    return nRet;
}

int mppVi_unbind_vo(int nViDevId, int nViChnId, int nVoLayer, int nVoChnId)
{
    int nRet = 0;
    ot_mpp_chn stSrcChn, stDstChn;
    stSrcChn.mod_id = OT_ID_VI;
    stSrcChn.dev_id = nViDevId;
    stSrcChn.chn_id = nViChnId;

    stDstChn.mod_id = OT_ID_VO;
    stDstChn.dev_id = nVoLayer;
    stDstChn.chn_id = nVoChnId;
    nRet = ss_mpi_sys_bind(&stSrcChn, &stDstChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vi and vo bind error %x\n", nRet);
    }
    return nRet;
}

int mppVpss_bind_vpss(int nVpssGrp, int nVpssChn, int nDstVpssGrp, int nDstVpssChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_VPSS;
    stSrcChn.dev_id = nVpssGrp;
    stSrcChn.chn_id = nVpssChn;

    stDestChn.mod_id = OT_ID_VPSS;
    stDestChn.dev_id = nDstVpssGrp;
    stDestChn.chn_id = nDstVpssChn;

    nRet = ss_mpi_sys_bind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vpss grp=%d chn=%d bind vpss grp=%d chn=%d failed with %#x!", nVpssGrp, nVpssChn, nDstVpssGrp, nDstVpssChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppVpss_unbind_vpss(int nVpssGrp, int nVpssChn, int nDstVpssGrp, int nDstVpssChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_VPSS;
    stSrcChn.dev_id = nVpssGrp;
    stSrcChn.chn_id = nVpssChn;

    stDestChn.mod_id = OT_ID_VPSS;
    stDestChn.dev_id = nDstVpssGrp;
    stDestChn.chn_id = nDstVpssChn;

    nRet = ss_mpi_sys_unbind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vpss grp=%d chn=%d unbind vpss grp=%d chn=%d failed with %#x!", nVpssGrp, nVpssChn, nDstVpssGrp, nDstVpssChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppVpss_bind_vo(int nVpssGrp, int nVpssChn, int nVoLayer, int nVoChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_VPSS;
    stSrcChn.dev_id = nVpssGrp;
    stSrcChn.chn_id = nVpssChn;

    stDestChn.mod_id = OT_ID_VO;
    stDestChn.dev_id = nVoLayer;
    stDestChn.chn_id = nVoChn;

    nRet = ss_mpi_sys_bind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vpss grp=%d chn=%d bind vo lay=%d chn=%d failed with %#x!", nVpssGrp, nVpssChn, nVoLayer, nVoChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppVpss_unbind_vo(int nVpssGrp, int nVpssChn, int nVoLayer, int nVoChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_VPSS;
    stSrcChn.dev_id = nVpssGrp;
    stSrcChn.chn_id = nVpssChn;

    stDestChn.mod_id = OT_ID_VO;
    stDestChn.dev_id = nVoLayer;
    stDestChn.chn_id = nVoChn;

    nRet = ss_mpi_sys_unbind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vpss grp=%d chn=%d unbind vo lay=%d chn=%d failed with %#x!", nVpssGrp, nVpssChn, nVoLayer, nVoChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppVpss_bind_venc(int nVpssGrp, int nVpssChn, int nVencChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_VPSS;
    stSrcChn.dev_id = nVpssGrp;
    stSrcChn.chn_id = nVpssChn;

    stDestChn.mod_id = OT_ID_VENC;
    stDestChn.dev_id = 0;
    stDestChn.chn_id = nVencChn;

    nRet = ss_mpi_sys_bind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vpss grp=%d chn=%d bind vencchn=%d failed with %#x!", nVpssGrp, nVpssChn, nVencChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppVpss_unbind_venc(int nVpssGrp, int nVpssChn, int nVencChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_VPSS;
    stSrcChn.dev_id = nVpssGrp;
    stSrcChn.chn_id = nVpssChn;

    stDestChn.mod_id = OT_ID_VENC;
    stDestChn.dev_id = 0;
    stDestChn.chn_id = nVencChn;

    nRet = ss_mpi_sys_unbind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("vpss grp=%d chn=%d unbind vencchn=%d failed with %#x!", nVpssGrp, nVpssChn, nVencChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppVenc_bind_venc(int nSrcVencChn, int nDstVencChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_VENC;
    stSrcChn.dev_id = 1;
    stSrcChn.chn_id = nSrcVencChn;

    stDestChn.mod_id = OT_ID_VENC;
    stDestChn.dev_id = 0;
    stDestChn.chn_id = nDstVencChn;

    nRet = ss_mpi_sys_bind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("venc dev:1 chn:%d bind venc dev:0 chn:%d failed with %#x!", nSrcVencChn, nDstVencChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppVenc_unbind_venc(int nSrcVencChn, int nDstVencChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_VENC;
    stSrcChn.dev_id = 1;
    stSrcChn.chn_id = nSrcVencChn;

    stDestChn.mod_id = OT_ID_VENC;
    stDestChn.dev_id = 0;
    stDestChn.chn_id = nDstVencChn;

    nRet = ss_mpi_sys_unbind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("venc dev:1 chn:%d unbind venc dev:0 chn:%d failed with %#x!", nSrcVencChn, nDstVencChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppAi_bind_aenc(int nAiDev, int nAiChn, int nAencChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_AI;
    stSrcChn.dev_id = nAiDev;
    stSrcChn.chn_id = nAiChn;

    stDestChn.mod_id = OT_ID_AENC;
    stDestChn.dev_id = 0;
    stDestChn.chn_id = nAencChn;

    nRet = ss_mpi_sys_bind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("ai dev=%d chn=%d bind aenc chn=%d failed with %#x!", nAiDev, nAiChn, nAencChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppAi_unbind_aenc(int nAiDev, int nAiChn, int nAencChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_AI;
    stSrcChn.dev_id = nAiDev;
    stSrcChn.chn_id = nAiChn;

    stDestChn.mod_id = OT_ID_AENC;
    stDestChn.dev_id = 0;
    stDestChn.chn_id = nAencChn;

    nRet = ss_mpi_sys_unbind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("ai dev=%d chn=%d unbind aenc chn=%d failed with %#x!", nAiDev, nAiChn, nAencChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppAdec_bind_ao(int nAdecChn, int nAoDev, int nAoChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_ADEC;
    stSrcChn.dev_id = 0;
    stSrcChn.chn_id = nAdecChn;

    stDestChn.mod_id = OT_ID_AO;
    stDestChn.dev_id = nAoDev;
    stDestChn.chn_id = nAoChn;

    nRet = ss_mpi_sys_bind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("adec chn=%d bind ao dev=%d chn=%d failed with %#x!", nAdecChn, nAoDev, nAoChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}

int mppAdec_unbind_ao(int nAdecChn, int nAoDev, int nAoChn)
{
    td_s32 nRet = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    stSrcChn.mod_id = OT_ID_ADEC;
    stSrcChn.dev_id = 0;
    stSrcChn.chn_id = nAdecChn;

    stDestChn.mod_id = OT_ID_AO;
    stDestChn.dev_id = nAoDev;
    stDestChn.chn_id = nAoChn;

    nRet = ss_mpi_sys_unbind(&stSrcChn, &stDestChn);
    if (nRet != TD_SUCCESS)
    {
        mpi_log("adec chn=%d unbind ao dev=%d chn=%d failed with %#x!", nAdecChn, nAoDev, nAoChn, nRet);
        return TD_FAILURE;
    }

    return nRet;
}
