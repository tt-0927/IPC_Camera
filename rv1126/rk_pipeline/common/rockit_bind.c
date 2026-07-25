/*************************************************************************
	> File Name: rockit_bind.c
	> Author:luoyk 
	> Mail: 
    > description: 媒体模块绑定接口
	> Created Time: Mon 09 May 2022 04:17:34 PM CST
 ************************************************************************/

#include<stdio.h>
#include <string.h>
#include"rockit_bind.h"

/*vdec 绑定 vo*/
int rockitVdec_bind_vo(int nVdecChnId, int nVoLayer, int nVoChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = nVdecChnId;

    stDstChn.enModId = RK_ID_VO;
    stDstChn.s32DevId = nVoLayer;
    stDstChn.s32ChnId = nVoChnId;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec and vo bind error %x", nRet);
    }   
    return nRet;
}
int rockitVdec_unbind_vo(int nVdecChnId, int nVoLayer, int nVoChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = nVdecChnId;

    stDstChn.enModId = RK_ID_VO;
    stDstChn.s32DevId = nVoLayer;
    stDstChn.s32ChnId = nVoChnId;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec and vo bind error %x", nRet);
    }   
    return nRet;
}

/*vdec 绑定 vpss*/
int rockitVdec_bind_vpss(int nVdecChnId, int nVpssGrp, int nVpssChn )
{

    int nRet=RK_SUCCESS;
    MPP_CHN_S stSrcChn, stDstChn;
    
    memset(&stSrcChn, 0, sizeof(MPP_CHN_S));
    memset(&stDstChn, 0, sizeof(MPP_CHN_S));

    stSrcChn.enModId = RK_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = nVdecChnId;

    stDstChn.enModId = RK_ID_VPSS;
    stDstChn.s32DevId = nVpssGrp;
    stDstChn.s32ChnId = nVpssChn; 
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec and vo bind error");
        return nRet;
    }   
    return 0;
}
int rockitVdec_unbind_vpss(int nVdecChnId, int nVpssGrp, int nVpssChn )
{

    int nRet=RK_SUCCESS;
    MPP_CHN_S stSrcChn, stDstChn;
    
    memset(&stSrcChn, 0, sizeof(MPP_CHN_S));
    memset(&stDstChn, 0, sizeof(MPP_CHN_S));

    stSrcChn.enModId = RK_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = nVdecChnId;

    stDstChn.enModId = RK_ID_VPSS;
    stDstChn.s32DevId = nVpssGrp;
    stDstChn.s32ChnId = nVpssChn; 
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec and vo bind error");
        return nRet;
    }   
    return 0;
}

/*vdec 绑定 venc*/
int rockitVdec_bind_venc(int nVdecChnId, int nVencChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = nVdecChnId;

    stDstChn.enModId = RK_ID_VENC;
    stDstChn.s32DevId = 0;
    stDstChn.s32ChnId = nVencChnId;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec and vo bind error %x", nRet);
    }   
    return nRet;
}
int rockitVdec_unbind_venc(int nVdecChnId, int nVencChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VDEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = nVdecChnId;

    stDstChn.enModId = RK_ID_VENC;
    stDstChn.s32DevId = 0;
    stDstChn.s32ChnId = nVencChnId;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec and vo bind error %x", nRet);
    }   
    return nRet;
}
/*vpss 绑定 vpss*/
int rockitVpss_bind_vpss(int nVpssGrp, int nVpssChn, int nDstVpssGrp, int nDstVpssChn)
{
    RK_S32 nRet = RK_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = RK_ID_VPSS;
    stSrcChn.s32DevId = nVpssGrp;
    stSrcChn.s32ChnId = nVpssChn;

    stDestChn.enModId = RK_ID_VPSS;
    stDestChn.s32DevId = nDstVpssGrp;
    stDestChn.s32ChnId = nDstVpssChn;

    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vpss grp=%d chn=%d bind vpss grp=%d chn=%d failed with %#x!",nVpssGrp, nVpssChn, nDstVpssGrp, nDstVpssChn, nRet);
        return RK_FAILURE;
    }   

    return nRet;
}
int rockitVpss_unbind_vpss(int nVpssGrp, int nVpssChn, int nDstVpssGrp, int nDstVpssChn)
{
    RK_S32 nRet = RK_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = RK_ID_VPSS;
    stSrcChn.s32DevId = nVpssGrp;
    stSrcChn.s32ChnId = nVpssChn;

    stDestChn.enModId = RK_ID_VPSS;
    stDestChn.s32DevId = nDstVpssGrp;
    stDestChn.s32ChnId = nDstVpssChn;

    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vpss grp=%d chn=%d unbind vpss grp=%d chn=%d failed with %#x!",nVpssGrp, nVpssChn, nDstVpssGrp, nDstVpssChn, nRet);
        return RK_FAILURE;
    }   

    return nRet;
}

/*vpss 绑定 vo*/
int rockitVpss_bind_vo(int nVpssGrp, int nVpssChn, int nVoLayer, int nVoChn)
{
    RK_S32 nRet = RK_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = RK_ID_VPSS;
    stSrcChn.s32DevId = nVpssGrp;
    stSrcChn.s32ChnId = nVpssChn;

    stDestChn.enModId = RK_ID_VO;
    stDestChn.s32DevId = nVoLayer;
    stDestChn.s32ChnId = nVoChn;

    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vpss grp=%d chn=%d bind vo lay=%d chn=%d failed with %#x!",nVpssGrp, nVpssChn, nVoLayer, nVoChn, nRet);
        return RK_FAILURE;
    }   

    return nRet;
}
int rockitVpss_unbind_vo(int nVpssGrp, int nVpssChn, int nVoLayer, int nVoChn)
{
    RK_S32 nRet = RK_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = RK_ID_VPSS;
    stSrcChn.s32DevId = nVpssGrp;
    stSrcChn.s32ChnId = nVpssChn;

    stDestChn.enModId = RK_ID_VO;
    stDestChn.s32DevId = nVoLayer;
    stDestChn.s32ChnId = nVoChn;

    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vpss grp=%d chn=%d unbind vo lay=%d chn=%d failed with %#x!",nVpssGrp, nVpssChn, nVoLayer, nVoChn, nRet);
        return RK_FAILURE;
    }   

    return nRet;
}

/*vpss 绑定 venc*/
int rockitVpss_bind_venc(int nVpssGrp, int nVpssChn, int nVencChn)
{
	RK_S32 nRet = RK_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = RK_ID_VPSS;
    stSrcChn.s32DevId = nVpssGrp;
    stSrcChn.s32ChnId = nVpssChn;

    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = nVencChn;

    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if ( nRet != RK_SUCCESS) 
    {
        printf("vpss grp=%d chn=%d bind vencchn=%d failed with %#x!",nVpssGrp, nVpssChn, nVencChn, nRet);
        return RK_FAILURE;
    }

    return nRet;
}
int rockitVpss_unbind_venc(int nVpssGrp, int nVpssChn, int nVencChn)
{
	RK_S32 nRet = RK_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = RK_ID_VPSS;
    stSrcChn.s32DevId = nVpssGrp;
    stSrcChn.s32ChnId = nVpssChn;

    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = nVencChn;

    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if ( nRet != RK_SUCCESS) 
    {
        printf("vpss grp=%d chn=%d unbind vencchn=%d failed with %#x!",nVpssGrp, nVpssChn, nVencChn, nRet);
        return RK_FAILURE;
    }

    return nRet;
}


/*venc 绑定 vdec*/
int rockitVenc_bind_vdec(int nVencChnId, int nVdecChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stDstChn.enModId = RK_ID_VDEC;
    stDstChn.s32DevId = 0;
    stDstChn.s32ChnId = nVdecChnId;

    stSrcChn.enModId = RK_ID_VENC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = nVencChnId;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec and vo bind error %x", nRet);
    }   
    return nRet;
}
int rockitVenc_unbind_vdec(int nVencChnId, int nVdecChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stDstChn.enModId = RK_ID_VDEC;
    stDstChn.s32DevId = 0;
    stDstChn.s32ChnId = nVdecChnId;

    stSrcChn.enModId = RK_ID_VENC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = nVencChnId;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vdec and vo bind error %x", nRet);
    }   
    return nRet;
}


/*vi 绑定 venc*/
int rockitVi_bind_venc(int nViDevId, int nViChnId, int nVencChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = nViDevId;
    stSrcChn.s32ChnId = nViChnId;

    stDstChn.enModId = RK_ID_VENC;
    stDstChn.s32DevId = 0;
    stDstChn.s32ChnId = nVencChnId;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and venc bind error %x\n", nRet);
    }   
    return nRet;
}
int rockitVi_unbind_venc(int nViDevId, int nViChnId, int nVencChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = nViDevId;
    stSrcChn.s32ChnId = nViChnId;

    stDstChn.enModId = RK_ID_VENC;
    stDstChn.s32DevId = 0;
    stDstChn.s32ChnId = nVencChnId;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and venc bind error %x\n", nRet);
    }   
    return nRet;
}

/*vi 绑定 vpss*/
int rockitVi_bind_vpss(int nViDevId, int nViChnId, int nVpssGrp, int nVpssChn )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = nViDevId;
    stSrcChn.s32ChnId = nViChnId;

    stDstChn.enModId = RK_ID_VPSS;
    stDstChn.s32DevId = nVpssGrp;
    stDstChn.s32ChnId = nVpssChn;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and vpss bind error %x\n", nRet);
    }   
    return nRet;
}
int rockitVi_unbind_vpss(int nViDevId, int nViChnId, int nVpssGrp, int nVpssChn )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = nViDevId;
    stSrcChn.s32ChnId = nViChnId;

    stDstChn.enModId = RK_ID_VPSS;
    stDstChn.s32DevId = nVpssGrp;
    stDstChn.s32ChnId = nVpssChn;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and vpss bind error %x\n", nRet);
    }   
    return nRet;
}

/*vi 绑定 vo*/
int rockitVi_bind_vo(int nViDevId, int nViChnId, int nVoLayer, int nVoChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = nViDevId;
    stSrcChn.s32ChnId = nViChnId;

    stDstChn.enModId = RK_ID_VO;
    stDstChn.s32DevId = nVoLayer;
    stDstChn.s32ChnId = nVoChnId;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and vo bind error %x\n", nRet);
    }   
    return nRet;
}
int rockitVi_unbind_vo(int nViDevId, int nViChnId, int nVoLayer, int nVoChnId)
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = nViDevId;
    stSrcChn.s32ChnId = nViChnId;

    stDstChn.enModId = RK_ID_VO;
    stDstChn.s32DevId = nVoLayer;
    stDstChn.s32ChnId = nVoChnId;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and vo bind error %x\n", nRet);
    }   
    return nRet;
}

/*ai 绑定 aenc*/
int rockitAi_bind_aenc(int nAiDevId, int nAiChnId, int nAencDevId, int nAencChnId)
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_AI;
    stSrcChn.s32DevId = nAiDevId;
    stSrcChn.s32ChnId = nAiChnId;

    stDstChn.enModId = RK_ID_AENC;
    stDstChn.s32DevId = nAencDevId;
    stDstChn.s32ChnId = nAencChnId;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and vo bind error %x\n", nRet);
    } 

    return nRet;
}

int rockitAi_unbind_aenc(int nAiDevId, int nAiChnId, int nAencDevId, int nAencChnId )
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_AI;
    stSrcChn.s32DevId = nAiDevId;
    stSrcChn.s32ChnId = nAiChnId;

    stDstChn.enModId = RK_ID_AENC;
    stDstChn.s32DevId = nAencDevId;
    stDstChn.s32ChnId = nAencChnId;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and vo bind error %x\n", nRet);
    } 

    return nRet;
}

int rockitAdec_bind_ao(int nAdecDevId, int nAdecChnId, int nAoDevId, int nAoChnId)
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_ADEC;
    stSrcChn.s32DevId = nAdecDevId;
    stSrcChn.s32ChnId = nAdecChnId;

    stDstChn.enModId = RK_ID_AO;
    stDstChn.s32DevId = nAoDevId;
    stDstChn.s32ChnId = nAoChnId;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("adec and ao bind error %x\n", nRet);
    } 

    return nRet;
}

int rockitAdec_unbind_ao(int nAdecDevId, int nAdecChnId, int nAoDevId, int nAoChnId)
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_ADEC;
    stSrcChn.s32DevId = nAdecDevId;
    stSrcChn.s32ChnId = nAdecChnId;

    stDstChn.enModId = RK_ID_AO;
    stDstChn.s32DevId = nAoDevId;
    stDstChn.s32ChnId = nAoChnId;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("adec and ao unbind error %x\n", nRet);
    } 

    return nRet;
}

// /*vi bind gdc*/
int rockitVi_bind_gdc(int nViDevId, int nViChnId, int nGdcDevId, int nGdcChnId)
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = nViDevId;
    stSrcChn.s32ChnId = nViChnId;

    stDstChn.enModId = RK_ID_GDC;
    stDstChn.s32DevId = nGdcDevId;
    stDstChn.s32ChnId = nGdcChnId;
    nRet = RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and gdc bind error %x\n", nRet);
    } 

    return nRet;
}

int rockitVi_unbind_gdc(int nViDevId, int nViChnId, int nGdcDevId, int nGdcChnId)
{
    int nRet = 0;
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_GDC;
    stSrcChn.s32DevId = nViDevId;
    stSrcChn.s32ChnId = nViChnId;

    stDstChn.enModId = RK_ID_VPSS;
    stDstChn.s32DevId = nGdcDevId;
    stDstChn.s32ChnId = nGdcChnId;
    nRet = RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
    if (nRet != RK_SUCCESS) 
    {
        printf("vi and gdc unbind error %x\n", nRet);
    } 

    return nRet;
}


