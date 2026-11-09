/**
 * @FilePath     : face_align.h
 * @Author       : leiyy
 * @Date         : 2026-10-21 11:33:24
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-21 11:33:24
 * @Description  : 人脸五点对齐
 */
#pragma once

#include <cstdint>
#include <vector>

#include "classroom_define.h"

namespace Classroom
{
/* 人脸关键点数 CLASSROOM_FACE_LANDMARK_NUM 定义在 classroom_define.h：
   那里同时定了打平行长（后端输出的契约），两边必须用同一个数 */

/* 对齐输出缓冲的字节上限（160×160×3 + 余量）*/
#define CLASSROOM_FACE_ALIGN_MAX_BYTES (160 * 160 * 3)

/* 恒等置换表的下标。配置层的默认值引用它而不是写 0：表序在本文件里排，
   "默认不重排"这件事一旦改序就要跟着改，两处隔着一个模块就会各改各的 */
#define CLASSROOM_ALIGN_PERM_IDENTITY (0)

/* 对齐裁剪档的默认边长（正方形）。160 而不是模型输入的 112：见 classroom_config.h
   里 nAlignCropPx 那条「两趟重采样」的理由。必须是 4 的倍数（RGA 目标宽度对齐要求），
   且不超过 CLASSROOM_FACE_ALIGN_MAX_BYTES 那一档的边长口径 */
#define CLASSROOM_ALIGN_CROP_DEFAULT_PX (160)

/* 裁剪边长的合法区间，配置层钳制与这里同一口径（两处各写一个数就会出现"钳完仍不合法"）。
   下限取交付模型的输入边长 112：裁剪比模型输入还小，对齐那一趟重采样就变成放大，
   插值出来的假细节比真细节多（与 fMinEyeSpanPx 那条同一个理由）。
   上限 192 不是精度而是常驻内存 */
#define CLASSROOM_ALIGN_CROP_MIN_PX (112)
#define CLASSROOM_ALIGN_CROP_MAX_PX (192)

/* 对齐裁剪框相对人脸框的外扩倍数默认值  */
#define CLASSROOM_ALIGN_CROP_EXPAND (2.0f)

/**
 * @brief   : 五点的配对槽位（对齐的唯一权威口径）
 */
typedef enum class FaceLandmark : int
{
    FL_LEYE = 0,  /* 与模板[0] 配对的点：模板参考点里 x 较小（画面左）的那只眼 */
    FL_REYE = 1,  /* 与模板[1] 配对的点：x 较大（画面右）的那只眼 */
    FL_NOSE = 2,  /* 鼻尖 */
    FL_LMOUTH = 3,/* 与模板[3] 配对的嘴角：x 较小的一侧 */
    FL_RMOUTH = 4,/* 与模板[4] 配对的嘴角：x 较大的一侧 */
    FL_NUM = CLASSROOM_FACE_LANDMARK_NUM
} FaceLandmark_E;

/**
 * @brief   : 对齐解算的失败原因
 */
typedef enum class AlignFail : int
{
    AF_OK = 0,
    AF_NULL_INPUT,        /* 空指针 / 容器尺寸与点数不符 */
    AF_POINT_NOT_FINITE,  /* NaN / inf：喂进去的图或后端内存被踩 */
    AF_SCORE_TOO_LOW,     /* 过分数门的有效点不足 3 */
    AF_SPREAD_TOO_SMALL,  /* 五点近于共线或近于重合成一点：解自由度退化 */
    AF_EYE_SPAN_TOO_SMALL,/* 瞳距像素低于下限：对齐后脸不足模型要求的分辨率 */
    AF_EYE_LINE_BAD,      /* 双眼连线方向退化（两点重合），无法定旋转 */
    AF_SCALE_OUT_OF_RANGE,/* 解出的缩放离谱：坐标系或缩放基准搞错了 */
    /* 拟合残差超限。**这一道是挡镜像/点序错的唯一手段**：左右互换或点序错乱时，
       4 自由度相似度解仍然有闭式解、仍然"合法"、仍然能重采样出一张看着像脸的图，
       只是五点没有一个落准。没有这道闸门时它会被当成一次成功对齐送进识别，
       现象是"这个人今天认得出明天认不出" */
    AF_FIT_RESIDUAL,
} AlignFail_E;

/**
 * @brief 对齐解算参数
 */
typedef struct _FaceAlignParam_
{
    /**
     * 单点最低置信度 */
    float fPointScoreMin = 0.0f;
    /* 源图上瞳距（左右眼像素距离）下限  */
    float fMinEyeSpanPx = 16.0f;
    /* 缩放合法区间（模板瞳距 / 源瞳距） */
    float fScaleMin = 0.05f;
    float fScaleMax = 8.0f;
    /* 拟合残差上限 */
    float fMaxResidualRatio = 0.22f;
    /* 点噪声地板（源帧像素） */
    float fNoiseFloorSrcPx = 5.0f;
    /* 模板标准五点（同 FaceLandmark 顺序）。默认取 ArcFace 系 112×112 的公开参考点，
       可被配置覆盖：换模型时这是唯一需要动的量，算法侧给出什么就填什么 */
    CPointf_S astTemplate[CLASSROOM_FACE_LANDMARK_NUM] = {
        { 38.2946f, 51.6963f }, { 73.5318f, 51.5014f }, { 56.0252f, 68.4931f },
        { 41.5493f, 92.5475f }, { 70.9678f, 92.9713f }
    };
} CFaceAlignParam_S;

/**
 * @brief 一次对齐解算的结果：源图 → 模板的正反两个 2×3 矩阵
 */
typedef struct _FaceAlign_
{
    bool  bValid = false;
    /* [0 1 2; 3 4 5] 行主序 2×3，含平移 */
    float afSrcToTpl[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float afTplToSrc[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float fScale = 0.0f;      /* 源 → 模板的缩放，观测出口要用（判断"这张脸被放大了几倍"） */
    float fAngleRad = 0.0f;   /* 逆时针为正，单位弧度 */
    /* 五点拟合 RMS（模板像素） */
    float fResidualPx = 0.0f;
} CFaceAlign_S;

/**
 * @brief 对齐解算计数
 */
typedef struct _FaceAlignStat_
{
    uint64_t u64Tried = 0;            /* 进解算的次数 */
    uint64_t u64Ok = 0;               /* 解算成功 */
    uint64_t u64FailLowScore = 0;
    uint64_t u64FailSpread = 0;
    uint64_t u64FailEyeSpan = 0;
    uint64_t u64FailEyeLine = 0;
    uint64_t u64FailScale = 0;
    uint64_t u64FailResidual = 0;
    uint64_t u64FailNotFinite = 0;
    uint64_t u64WarpPixel = 0;        /* 真正取到样本的模板像素数（越界填背景的不计）：
                                        与「对齐次数 × 模板像素」之比明显偏低时，说明解算出的
                                        变换把大半张脸落在了裁剪框外，而不是脸真的出画 */
} CFaceAlignStat_S;

/**
 * @brief   : 五点 → 4 自由度相似度变换的最小二乘解
 * @param    {const CPointf_S*} pastSrc：源图坐标的五点，长度恒为 CLASSROOM_FACE_LANDMARK_NUM
 * @param    {const float*} pastScore：与 pastSrc 同序的单点置信度，可传 nullptr 表示"都信"
 * @param    {const CPointf_S*} pastTpl：模板标准点（一般传 rstParam.astTemplate）
 * @param    {const CFaceAlignParam_S&} rstParam：各道闸门与阈值
 * @param    {CFaceAlign_S&} rstOut：OUT=成功时两个矩阵都填好；失败时 bValid=false
 * @param    {CFaceAlignStat_S&} rstStat：累加（可传 nullptr，单测里不需要）
 * @param    {AlignFail_E&} renFail：OUT=失败原因，成功为 AF_OK
 * @param    {float} fInputPxPerSrcPx：本次解算输入空间每源帧像素折多少 px（画布解=画布缩放比，
 *             裁剪解=crop_px/裁剪源宽），噪声地板门靠它把「源帧常量噪声」折进本空间。
 *             缺省 1.0 = 不折算（旧行为）；≤0 同样按不折算处理
 * @return   {bool} false 表示本张脸没有可用的对齐变换
 */
bool solveFaceAlign(const CPointf_S *pastSrc, const float *pastScore,
                    const CPointf_S *pastTpl, const CFaceAlignParam_S &rstParam,
                    CFaceAlign_S &rstOut, CFaceAlignStat_S *pstStat, AlignFail_E &renFail,
                    float fInputPxPerSrcPx = 1.0f);

/**
 * @brief   : 按解算结果把源图里的人脸重采样成模板尺寸
 * @param    {const uint8_t*} pucSrcRgb：源图，连续 RGB888（RGA 输出的那张裁剪图，通道序与感知层一致）
 * @param    {int32_t} nSrcW / nSrcH：源图宽高（像素）
 * @param    {const CFaceAlign_S&} rstAlign：solveFaceAlign 的成功结果
 * @param    {int32_t} nTplW / nTplH：输出（模板）宽高，须与模型输入一致
 * @param    {std::vector<uint8_t>&} rvecDst：OUT=连续 RGB888 输出，函数内 resize 到 nTplW*nTplH*3；
 *             越界像素按 fPadValue 填充，**不做边缘钳制**（理由见下）
 * @param    {uint8_t} ucPadValue：越界填充值，默认 0
 * @param    {CFaceAlignStat_S&} rstStat：可空，累加像素数
 * @return   {bool} false 表示入参非法（对齐无效 / 尺寸非法 / 空指针 / 超出缓冲上限）
 */
bool warpFaceRgb(const uint8_t *pucSrcRgb, int32_t nSrcW, int32_t nSrcH,
                 const CFaceAlign_S &rstAlign, int32_t nTplW, int32_t nTplH,
                 std::vector<uint8_t> &rvecDst, uint8_t ucPadValue = 0,
                 bool bClampEdge = false, CFaceAlignStat_S *pstStat = nullptr);

/**
 * @brief   : 变换的逆（相似度闭式求逆，供需要"模板 → 源"方向的调用方复用）
 * @return   {bool} 缩放为 0 时返回 false，且不改动 rstOut
 */
bool invertSimilarity(const float afM[6], float afMInv[6]);

/**
 * @brief   : 点过一个 2×3 矩阵（唯一的乘法出口）
 */
CPointf_S applySimilarity(const float afM[6], const CPointf_S &stP);

/**
 * @brief   : 源图坐标的瞳距（观测与门限判定共用一处实现，避免两处算法分叉）
 * @return   {float} 像素距离；输入含 NaN 时返回 NaN（调用方的比较会因此判失败，不会误判通过）
 */
float eyeSpanPx(const CPointf_S *pastSrc);

/**
 * @brief   : 失败原因的一行文本（进 warn 日志与 statusJson）
 * @return   {const char*} 静态串，永不为空
 */
const char *alignFailName(AlignFail_E enFail);

/**
 * @brief   : 对齐缓冲字节数上限校验（配置层与调用方共用同一个口径）
 */
bool faceAlignBufOk(int32_t nW, int32_t nH);


/**
 * @brief 候选点序置换的张数
 */
#define CLASSROOM_ALIGN_PERM_NUM (6)

/**
 * @brief   : 取第 nPerm 张置换表
 * @return   {const int32_t*} 长度 CLASSROOM_FACE_LANDMARK_NUM；**语义是"模板槽 i 用模型输出的第 p[i] 个点"**，
 *            即 pastOut[i] = pastIn[p[i]]。写成"槽→源下标"而不是"源→槽下标"，
 *            是因为解算与残差都按模板槽遍历，这样取值方向与使用方向一致，不需要求逆
 */
const int32_t *alignPermTable(int32_t nPerm);

/* 置换表的名字（配置校验、心跳与单测断言共用一处出口，避免三处拼写各写一遍） */
const char *alignPermName(int32_t nPerm);
/**
 * @brief   : 名字 → 置换表下标
 * @return   {int32_t} 未知名字返回 -1（配置层据此报"值非法"，不静默退回恒等）
 */
int32_t alignPermOf(const char *pName);

/**
 * @brief   : 按置换表重排五点（以及可选的分数）
 * @param    {const CPointf_S*} pastIn：模型原始输出序，长度恒为 CLASSROOM_FACE_LANDMARK_NUM
 * @param    {const float*} pastScoreIn：与 pastIn 同序的分数，可空
 * @param    {CPointf_S*} pastOut：OUT=按模板槽排好的点，**不得与 pastIn 别名**（非原地算法）
 * @param    {float*} pastScoreOut：可空
 * @return   {bool} false=入参为空或表下标非法
 */
bool applyAlignPerm(const CPointf_S *pastIn, const float *pastScoreIn,
                    CPointf_S *pastOut, float *pastScoreOut, int32_t nPerm);

/**
 * @brief   : 只算拟合残差的最小二乘解（**不带任何闸门**，与 solveFaceAlign 的区别就是这点）
 * @param    {const CPointf_S*} pastSrc / pastTpl：长度恒为五点的对应点，已过有限性检查
 * @param    {float*} pfScale：可空，OUT=源→模板缩放
 * @return   {float} 残差 RMS（模板像素）；五点退化或含 NaN 时返回 NaN
 */
float faceAlignFitRms(const CPointf_S *pastSrc, const CPointf_S *pastTpl, float *pfScale);

/**
 * @brief   : 由五点估计人脸横摆角（yaw，度；正 = 鼻尖偏向画面右侧）
 * @param    {const CPointf_S*} pastSlot：**已按模板槽位排好**的五点（即 applyAlignPerm 的输出，
 *            下标见 FaceLandmark_E；传模型原始点序在 point_order≠0 的机位上会把左右眼接反）
 * @param    {int32_t} nNum：点数，必须 ≥ CLASSROOM_FACE_LANDMARK_NUM
 * @return   {float} 估计角，范围 (-90, 90]；瞳距退化（< 1 px）或点位含 NaN 时返回 NaN
 */
float faceYawDeg(const CPointf_S *pastSlot, int32_t nNum);

/**
 * @brief 点序探针的累计量（回答"这套模型到底按什么顺序出点"，一台设备一次读数就够）
 */
typedef struct _AlignProbe_
{
    uint64_t u64Face = 0;                            /* 喂进探针的脸数（分母的总数，用于判"探针有没有吃到数"） */
    uint64_t u64Num[CLASSROOM_ALIGN_PERM_NUM] = { 0 }; /* 该表算出有效残差的次数，均值的分母 */
    uint64_t u64Ok[CLASSROOM_ALIGN_PERM_NUM] = { 0 };/* 该表下残差过门的次数 */
    double   adSumRms[CLASSROOM_ALIGN_PERM_NUM] = { 0.0 };
    float    afMinRms[CLASSROOM_ALIGN_PERM_NUM] = { 0.0f };
    int32_t  nBad = 0;                               /* 一张脸都没算出来的次数（点位含 NaN） */
} CFaceAlignProbe_S;

/**
 * @brief   : 喂一张脸进探针（按模型原始点序传，函数内部逐表重排）
 * @return   {int32_t} 残差最小的那张表的下标；无有效结果返回 -1
 */
int32_t alignProbeFeed(const CPointf_S *pastSrc, const CPointf_S *pastTpl,
                       const CFaceAlignParam_S &rstParam, CFaceAlignProbe_S &rstProbe);

/**
 * @brief   : 探针读数的一行文本（进心跳）。均数除以次数即平均残差，
 *            六张表并排给、谁最小谁就是答案，不做自动判定——判定要人看图
 */
const char *alignProbeText(const CFaceAlignProbe_S &rstProbe, char *pacBuf, int32_t nBufSize);

/* ============================ 对齐用的裁剪几何 ============================ */

/**
 * @brief   : 由人脸框算出「送去重采样的源区域」
 * @param    {const CRectf_S&} stBox：人脸框（任意坐标系，与 pastPoints 同一坐标系）
 * @param    {float} fExpand：外扩倍数（1.0=不外扩）。取 2.0 量级的理由写在调用方注释里
 * @param    {const CRectf_S&} stLimit：可写区域边界（一般是整个画布），越界在此钳掉
 * @param    {CRectf_S&} rstOut：OUT=**正方形**且已钳进 stLimit 的源区域
 * @return   {bool} false=框非法或钳完退化（零宽），调用方本张脸放弃
 */
bool faceAlignCropRect(const CRectf_S &stBox, float fExpand, const CRectf_S &stLimit,
                       CRectf_S &rstOut);

/**
 * @brief   : 把一组点从「源区域所在的坐标系」映到「裁剪缓冲内部的像素坐标」
 * @param    {const CRectf_S&} stUsed：RGA 钳制取偶后**真正用到**的那块源矩形（不是请求的那块）
 * @param    {int32_t} nDstW / nDstH：裁剪缓冲的实际像素尺寸
 */
void mapPointsToRect(const CPointf_S *pastSrc, int32_t nNum, const CRectf_S &stUsed,
                     int32_t nDstW, int32_t nDstH, CPointf_S *pastOut);

}
