/* GPL-2.0 WITH Linux-syscall-note OR Apache 2.0 */
/* Copyright (c) 2022 Fuzhou Rockchip Electronics Co., Ltd */

#ifndef INCLUDE_RT_MPI_RK_COMM_GDC_H_
#define INCLUDE_RT_MPI_RK_COMM_GDC_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include "rk_type.h"
#include "rk_common.h"
#include "rk_errno.h"
#include "rk_comm_video.h"

/* failure caused by malloc buffer */
#define RK_GDC_SUCCESS             RK_SUCCESS
#define RK_ERR_GDC_NOBUF           RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_NOBUF)
#define RK_ERR_GDC_BUF_EMPTY       RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_BUF_EMPTY)
#define RK_ERR_GDC_NULL_PTR        RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_NULL_PTR)
#define RK_ERR_GDC_ILLEGAL_PARAM   RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_ILLEGAL_PARAM)
#define RK_ERR_GDC_BUF_FULL        RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_BUF_FULL)
#define RK_ERR_GDC_SYS_NOTREADY    RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_NOTREADY)
#define RK_ERR_GDC_NOT_SUPPORT     RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_NOT_SUPPORT)
#define RK_ERR_GDC_NOT_PERMITTED   RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_NOT_PERM)
#define RK_ERR_GDC_BUSY            RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_BUSY)
#define RK_ERR_GDC_INVALID_CHNID   RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_INVALID_CHNID)
#define RK_ERR_GDC_CHN_UNEXIST     RK_DEF_ERR(RK_ID_GDC, RK_ERR_LEVEL_ERROR, RK_ERR_UNEXIST)

#define FISHEYE_MAX_REGION_NUM     9
#define FISHEYE_LMFCOEF_NUM        128
#define GDC_PMFCOEF_NUM            9
#define MAX_GDC_FILE_PATH_LEN      256

typedef RK_S32      GDC_HANDLE;

typedef struct rkGDC_TASK_ATTR_S {
    VIDEO_FRAME_INFO_S      stImgIn;             /* Input picture */
    VIDEO_FRAME_INFO_S      stImgOut;            /* Output picture */
    /* RW; Private data of task ; au64privateData[0]: stepx au64privateData[1]: stepy;
           advised to set this parameter to 0.*/
    RK_U64                  au64privateData[4];
    /* RW; Specify a task index, default 0 is not specify;[0,GDC_MAX_TASK_NUM);
           advised to set this parameter to 0*/
    RK_U64                  u64TaskId;
} GDC_TASK_ATTR_S;

/* Mount mode of device*/
typedef enum rkFISHEYE_MOUNT_MODE_E {
    FISHEYE_DESKTOP_MOUNT    = 0,        /* Desktop mount mode */
    FISHEYE_CEILING_MOUNT    = 1,        /* Ceiling mount mode */
    FISHEYE_WALL_MOUNT       = 2,        /* wall mount mode */

    FISHEYE_MOUNT_MODE_BUTT
} FISHEYE_MOUNT_MODE_E;

/* View mode of client*/
typedef enum rkFISHEYE_VIEW_MODE_E {
    FISHEYE_VIEW_360_PANORAMA   = 0,     /* 360 panorama mode of gdc correction */
    FISHEYE_VIEW_180_PANORAMA   = 1,     /* 180 panorama mode of gdc correction */
    FISHEYE_VIEW_NORMAL         = 2,     /* normal mode of gdc correction */
    FISHEYE_NO_TRANSFORMATION   = 3,     /* no gdc correction */

    FISHEYE_VIEW_MODE_BUTT
} FISHEYE_VIEW_MODE_E;

/*Fisheye region correction attribute */
typedef struct rkFISHEYE_REGION_ATTR_S {
    FISHEYE_VIEW_MODE_E     enViewMode;     /* RW; Range: [0, 3];gdc view mode */
    RK_U32                  u32InRadius;    /* RW; inner radius of gdc correction region*/
    RK_U32                  u32OutRadius;   /* RW; out radius of gdc correction region*/
    RK_U32                  u32Pan;         /* RW; Range: [0, 360] */
    RK_U32                  u32Tilt;        /* RW; Range: [0, 360] */
    RK_U32                  u32HorZoom;     /* RW; Range: [1, 4095] */
    RK_U32                  u32VerZoom;     /* RW; Range: [1, 4095] */
    RECT_S                  stOutRect;      /* RW; out Imge rectangle attribute */
} FISHEYE_REGION_ATTR_S;

typedef struct rkFISHEYE_REGION_ATTR_EX_S {
    FISHEYE_VIEW_MODE_E     enViewMode;     /* RW; Range: [0, 3];gdc view mode */
    RK_U32                  u32InRadius;    /* RW; inner radius of gdc correction region*/
    RK_U32                  u32OutRadius;   /* RW; out radius of gdc correction region*/
    RK_U32                  u32X;           /* RW; Range: [0, 4608] */
    RK_U32                  u32Y;           /* RW; Range: [0, 3456] */
    RK_U32                  u32HorZoom;     /* RW; Range: [1, 4095] */
    RK_U32                  u32VerZoom;     /* RW; Range: [1, 4095] */
    RECT_S                  stOutRect;      /* RW; out Imge rectangle attribute */
} FISHEYE_REGION_ATTR_EX_S;

/*Fisheye all regions correction attribute */
typedef struct rkFISHEYE_ATTR_S {
    RK_BOOL                 bEnable;    /* RW; Range: [0, 1];whether enable fisheye correction or not */
    /* RW; Range: [0, 1];whether gdc len's LMF coefficient is from user config or from default linear config */
    RK_BOOL                 bLMF;
    RK_BOOL                 bBgColor;   /* RW; Range: [0, 1];whether use background color or not */
    RK_U32                  u32BgColor; /* RW; Range: [0,0xffffff];the background color RGB888*/
    /* RW; Range: [-511, 511];the horizontal offset between image center and physical center of len*/
    RK_S32                  s32HorOffset;
    /* RW; Range: [-511, 511]; the vertical offset between image center and physical center of len*/
    RK_S32                  s32VerOffset;
    RK_U32                  u32TrapezoidCoef;  /* RW; Range: [0, 32];strength coefficient of trapezoid correction */
    RK_S32                  s32FanStrength;    /* RW; Range: [-760, 760];strength coefficient of fan correction */
    FISHEYE_MOUNT_MODE_E    enMountMode;       /* RW; Range: [0, 2];gdc mount mode */
    RK_U32                  u32RegionNum;      /* RW; Range: [1, 9]; gdc correction region number */
    /* RW; attribution of gdc correction region */
    FISHEYE_REGION_ATTR_S   astFishEyeRegionAttr[FISHEYE_MAX_REGION_NUM];
} FISHEYE_ATTR_S;

typedef struct rkFISHEYE_ATTR_EX_S {
    RK_BOOL                    bEnable;      /* RW; Range: [0, 1];whether enable fisheye correction or not */
    /* RW; Range: [0, 1];whether gdc len's LMF coefficient is from user config or from default linear config */
    RK_BOOL                    bLMF;
    RK_BOOL                    bBgColor;     /* RW; Range: [0, 1];whether use background color or not */
    RK_U32                     u32BgColor;   /* RW; Range: [0,0xffffff];the background color RGB888*/
    /* RW; Range: [-511, 511];the horizontal offset between image center and physical center of len*/
    RK_S32                     s32HorOffset;
    /* RW; Range: [-511, 511]; the vertical offset between image center and physical center of len*/
    RK_S32                     s32VerOffset;
    RK_U32                     u32TrapezoidCoef;  /* RW; Range: [0, 32];strength coefficient of trapezoid correction */
    RK_S32                     s32FanStrength;    /* RW; Range: [-760, 760];strength coefficient of fan correction */
    FISHEYE_MOUNT_MODE_E       enMountMode;       /* RW; Range: [0, 2];gdc mount mode */
    RK_U32                     u32RegionNum;      /* RW; Range: [1, 4]; gdc correction region number */
    /* RW; attribution of gdc correction region */
    FISHEYE_REGION_ATTR_EX_S   astFishEyeRegionAttr[FISHEYE_MAX_REGION_NUM];
} FISHEYE_ATTR_EX_S;

/*Spread correction attribute */
typedef struct rkSPREAD_ATTR_S {
    /* RW; Range: [0, 1];whether enable spread or not, When spread on, ldc DistortionRatio range should be [0, 500] */
    RK_BOOL                 bEnable;
    RK_U32                  u32SpreadCoef;      /* RW; Range: [0, 18];strength coefficient of spread correction */
    SIZE_S                  stDestSize;         /* RW; dest size of spread*/
} SPREAD_ATTR_S;

/*Fisheye Job Config */
typedef struct rkFISHEYE_JOB_CONFIG_S {
    RK_U64                  u64LenMapPhyAddr;   /* LMF coefficient Physic Addr*/
} FISHEYE_JOB_CONFIG_S;

/*Fisheye Config */
typedef struct rkFISHEYE_CONFIG_S {
    RK_U16                  au16LMFCoef[FISHEYE_LMFCOEF_NUM];     /*RW;  LMF coefficient of gdc len */
} FISHEYE_CONFIG_S;

/*Gdc PMF Attr */
typedef struct rkGDC_PMF_ATTR_S {
    RK_S64                  as64PMFCoef[GDC_PMFCOEF_NUM];         /*W;  PMF coefficient of gdc */
} GDC_PMF_ATTR_S;

typedef struct rkGDC_FISHEYE_POINT_QUERY_ATTR_S {
    RK_U32          u32RegionIndex;
    FISHEYE_ATTR_S *pstFishEyeAttr;
    RK_U16          au16LMF[FISHEYE_LMFCOEF_NUM];
} GDC_FISHEYE_POINT_QUERY_ATTR_S;

typedef enum rkGDC_CHN_MODE_E  {
    GDC_CHN_MODE_EIS,
    GDC_CHN_MODE_FEC,
} GDC_CHN_MODE_E;

typedef enum rkGDC_FEC_MODE_E  {
    GDC_FEC_UPDATE_MESH_ONLINE,                      	// generate lut online
    GDC_FEC_UPDATE_MESH_FROM_FILE,          			// external file import mesh
    GDC_FEC_UPDATE_MESH_FROM_BUFFER,
    GDC_FEC_UPDATE_MESH_ONLINE_FROM_INI,
} GDC_FEC_MODE_E;

typedef enum rkGDC_FEC_CORRECT_DIRECTION {
    GDC_FEC_CORRECT_DIRECTION_X = 0x1,
    GDC_FEC_CORRECT_DIRECTION_Y,
    GDC_FEC_CORRECT_DIRECTION_XY
} GDC_FEC_CORRECT_DIRECTION;

typedef enum rkGDC_FEC_CORRECT_STYLE {
    GDC_FEC_KEEP_ASPECT_RATIO_REDUCE_FOV = 0x1,
    GDC_FEC_COMPRES_IMAGE_KEEP_FOV,
} GDC_FEC_CORRECT_STYLE;

typedef struct rkGDC_FEC_BG_ATTR_S {
    RK_S32 s32BgY;
    RK_S32 s32BgU;
    RK_S32 s32BgV;
} GDC_FEC_BG_ATTR_S;

typedef struct rkGDC_FEC_ATTR_S {
    GDC_FEC_MODE_E enFecMode;
    RK_S32         s32InFourcc;
    RK_S32         s32OutFourcc;
    RK_DOUBLE      dLightCenter[2];
    RK_DOUBLE      dCoeff[4];
    GDC_FEC_CORRECT_DIRECTION   enDirection;
    GDC_FEC_CORRECT_STYLE       enStyle;
    RK_S32         s32CorrectLevel;
    RK_CHAR        mesh0[64];
    RK_CHAR        mesh1[64];
    RK_CHAR        mesh2[64];
    RK_CHAR        mesh3[64];
    RK_S32         s32BorderMode;
    RK_S32         s32CrossBufMode;
    GDC_FEC_BG_ATTR_S stBgVal;
    RK_CHAR        aIniFile[128];
} GDC_FEC_ATTR_S;

typedef struct rkGDC_EIS_ATTR_S {
    RK_S32            s32DevNo;
} GDC_EIS_ATTR_S;

typedef struct rkGDC_CHN_ATTR_S {
    RK_U32            u32MaxInQueue;
    RK_U32            u32MaxOutQueue;
    RK_S32            s32DstWidth;
    RK_S32            s32DstHeight;
    RK_CHAR           cfgFile[MAX_GDC_FILE_PATH_LEN];
    RK_S32            s32Depth;
    GDC_CHN_MODE_E    enMode;
    PIXEL_FORMAT_E    enDstPixelFormat;
    COMPRESS_MODE_E   enDstCompMode;
    union {
        GDC_FEC_ATTR_S  stFecAttr;
        GDC_EIS_ATTR_S  stEisAttr;
    };
    VIDEO_FORMAT_E    enDstVideoFormat;
} GDC_CHN_ATTR_S;

typedef struct rkGDC_SENSOR_INFO_S {
    RK_S64      s64Timestamp;
    RK_DOUBLE   dTemp;
    RK_DOUBLE   dGyroData[3];
    RK_DOUBLE   dAccData[3];
} GDC_SENSOR_INFO_S;

typedef struct rkGDC_VFAME_INFO_S {
    RK_U64              u64ExtraPts;
    RK_U32              u32RsSkew;
    RK_U32              u32ExpTime;
    RK_U32              u32Again;
    RK_U32              u32Dgain;
    RK_U32              u32Ispgain;
    RK_DOUBLE           dIso;
    MB_BLK              pMbBlk;
    RK_U32              u32Width;
    RK_U32              u32Height;
    RK_U32              u32VirWidth;
    RK_U32              u32VirHeight;
    PIXEL_FORMAT_E      enPixelFormat;
    COMPRESS_MODE_E     enCompressMode;
    RK_U32              u32Seq;
    RK_U64              u64PTS;
} GDC_VFAME_INFO_S;

typedef enum rkGDC_INFO_CB_RESULT {
    GDC_INFO_CB_SUCCESS = RK_SUCCESS,
    GDC_INFO_CB_CANCEL,
    GDC_INFO_CB_ERROR,
} GDC_INFO_CB_RESULT;

typedef struct rkGDC_FEC_UPDATE_MESH_FILE_INFO_S {
    RK_CHAR        aMeshPath[256];
} GDC_FEC_UPDATE_MESH_FILE_INFO_S;

typedef struct rkGDC_FEC_UPDATE_MESH_BUFFER_INFO_S {
    RK_S32         s32DmaFd;
    RK_VOID       *pVirAddr;
    RK_U64         u64Size;
    RK_BOOL        bNeedCopy;
} GDC_FEC_UPDATE_MESH_BUFFER_INFO_S;

typedef struct rkGDC_FEC_UPDATE_MESH_ONLINE_INFO_S {
    RK_DOUBLE      dLightCenter[2];
    RK_DOUBLE      dCoeff[4];
    GDC_FEC_CORRECT_DIRECTION   enDirection;
    GDC_FEC_CORRECT_STYLE       enStyle;
    RK_S32         s32CorrectLevel;
    RK_S32         s32CalibWidth;
    RK_S32         s32CalibHeight;
    RK_S32         s32CalibLevelMaxLimit;
} GDC_FEC_UPDATE_MESH_ONLINE_INFO_S;

typedef struct rkGDC_FEC_UPDATE_ATTR_S {
    GDC_FEC_MODE_E enFecMode;
    union {
        GDC_FEC_UPDATE_MESH_ONLINE_INFO_S stOnlineCfg;
        GDC_FEC_UPDATE_MESH_FILE_INFO_S   stFileCfg;
        GDC_FEC_UPDATE_MESH_BUFFER_INFO_S stBufCfg;
    };
} GDC_FEC_UPDATE_ATTR_S;

typedef struct rkGDC_UPDATE_ATTR_S {
    GDC_CHN_MODE_E  enMode;
    union {
        GDC_FEC_UPDATE_ATTR_S  stFecAttr;
    };
} GDC_UPDATE_ATTR_S;

typedef struct rkGDC_INFO_CB_S {
    RK_S32 (*pfnSensorCB)(RK_VOID *pUsr, GDC_SENSOR_INFO_S *pInfo);
    RK_S32 (*pfnVframeCB)(RK_VOID *pUsr, GDC_VFAME_INFO_S  *pInfo);
} GDC_INFO_CB_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __RK_COMM_GDC_H__ */
