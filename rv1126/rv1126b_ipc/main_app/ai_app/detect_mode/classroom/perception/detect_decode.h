/**
 * @FilePath     : detect_decode.h
 * @Author       : leiyy
 * @Date         : 2026-10-13 15:12:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-12 15:12:00
 * @Description  : 检测输出解码与头身关联
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_define.h"
#include "crop_geom.h"
#include "feature_frame.h"
#include "frame_geometry.h"
#include "person_tracker.h"

namespace Classroom
{
/**
 * @brief 远带人脸的置信度折扣
 */
#define CLASSROOM_FACE_FAR_GATE_RATIO (0.4f)

/**
 * @brief   : 一次模型输入的坐标映射（模型输入像素 ↔ 分析帧）
 */
class CDetMap
{
public:
    CDetMap() = default;
    ~CDetMap() = default;

    /**
     * @brief   : 建立映射
     * @param    {const CFrameGeometry&} stSrcToCanvas：帧自带的源帧→分析帧变换（镜像/旋转在此）
     * @param    {int} nRectX / nRectY / nRectW / nRectH：本次模型输入**实际覆盖的源帧区域**
     *            整帧检测传 (0,0,srcW,srcH)；ROI 裁剪必须传 CCropUnit 钳制取偶后真正用到的矩形，
     *            传原始框会让关键点整体偏移半个框——偏移量随框大小变化，表现为「偶尔判错」
     * @param    {int} nModelW / nModelH：模型输入宽高
     * @param    {std::string&} rstrErr：OUT=失败原因
     * @return   {bool} false 时映射不可用，调用方必须丢帧而不是「按 1:1 凑合」
     */
    bool setup(const CFrameGeometry &stSrcToCanvas,
               int nRectX, int nRectY, int nRectW, int nRectH,
               int nModelW, int nModelH, std::string &rstrErr);

    /**
     * @brief   : 同上，另带等比缩放计划（#93-1 letterbox）
     * @param    {const CLetterboxPlan_S&} stLb：bEnable=false 时与八参版本完全同形；
     *            true 时"模型平面"按计划收成内容平面（左上锚定，pad 只在底部）——
     *            映射以内容子矩形为准，pad 区里即便出框也只会被钳到画面边，不产生假坐标
     * @return   {bool} 计划非法（内容越出模型档/尺寸退化）时 false，调用方丢帧
     */
    bool setup(const CFrameGeometry &stSrcToCanvas,
               int nRectX, int nRectY, int nRectW, int nRectH,
               int nModelW, int nModelH, const CLetterboxPlan_S &stLb, std::string &rstrErr);

    bool isReady() const { return m_bReady; }

    /* 模型输入框 → 分析帧框：越界由 CFrameGeometry 钳到画面内，不产生假坐标 */
    CRectf_S  modelToCanvas(const CRectf_S &stRect) const;
    CPointf_S modelToCanvas(const CPointf_S &stPoint) const;

    /* 分析帧框 → 模型输入框：给「把区域/结果画回模型图」用（调试绘制） */
    CRectf_S canvasToModel(const CRectf_S &stRect) const;

    /* 本映射覆盖的源帧区域（ROI 档要按它做二次裁剪时用） */
    CRectf_S srcRect() const
    {
        return CRectf_S{ static_cast<float>(m_nRectX), static_cast<float>(m_nRectY),
                         static_cast<float>(m_nRectX + m_nRectW), static_cast<float>(m_nRectY + m_nRectH) };
    }

    int modelWidth() const  { return m_nModelW; }
    int modelHeight() const { return m_nModelH; }

    /**
     * @brief   : 一个模型像素对应多少源帧像素
     * @return   {float} 映射未就绪返回 0
     */
    float srcPxPerModelPxX() const { return m_bReady ? m_fSrcPxPerModelX : 0.0f; }
    float srcPxPerModelPxY() const { return m_bReady ? m_fSrcPxPerModelY : 0.0f; }

private:
    /* 模型像素 → 源帧像素（逆变换：缩放回裁剪矩形 + 平移） */
    CPointf_S modelToSrc(const CPointf_S &stPoint) const;
    CPointf_S srcToModel(const CPointf_S &stPoint) const;

private:
    bool m_bReady = false;

    /* 源帧 → 分析帧：镜像、旋转、缩放全在这一步，与全链路其它地方同一实现 */
    CFrameGeometry m_stSrcToCanvas;
    /* 裁剪矩形 → 模型输入：纯缩放，故镜像/旋转参数恒为「无」 */
    CFrameGeometry m_stRectToModel;

    int m_nRectX = 0;
    int m_nRectY = 0;
    int m_nRectW = 0;
    int m_nRectH = 0;
    int m_nModelW = 0;
    int m_nModelH = 0;

    float m_fSrcPxPerModelX = 0.0f;
    float m_fSrcPxPerModelY = 0.0f;
};

/**
 * @brief 单路检测器的解码参数
 */
typedef struct _DetectParam_
{
    float     fScoreThreshold = CLASSROOM_DETECT_SCORE_THRESHOLD; /* 解析侧复检门 */
    int32_t   nMinBoxEdge = CLASSROOM_MIN_HEAD_PIXEL;             /* 分析帧短边下限 */
    int32_t   nMaxBoxNum = CLASSROOM_MAX_PERSONS;                 /* 容量上限，超出丢弃并计数 */
    /* 远带人头：画布短边 ∈ [nMinBoxEdge, nFarEdgePx) 用基门×fFarGateRatio */
    int32_t   nFarEdgePx = CLASSROOM_HEAD_FAR_EDGE_PX;
    float     fFarGateRatio = CLASSROOM_HEAD_FAR_GATE_RATIO;
    /* 画布长边上限，0=关（默认） */
    int32_t   nMaxBoxEdge = 0;
} CDetectParam_S;

/**
 * @brief 解码计数
 */
typedef struct _DetectDecodeStat_
{
    uint64_t u64Rows = 0;           /* 收到的框行数 */
    uint64_t u64Accepted = 0;
    uint64_t u64Malformed = 0;      /* 长度不整 / 非有限值 / 框倒挂：后端输出坏了 */
    uint64_t u64MapNotReady = 0;    /* 映射没建起来：这拍整批丢弃，与「屏上没人」是两回事 */
    uint64_t u64LowScore = 0;
    uint64_t u64TooSmall = 0;       /* 换算后短边不足 nMinBoxEdge */
    uint64_t u64TooLarge = 0;       /* 长边超 nMaxBoxEdge（幻影大头，深色板面） */
    uint64_t u64OffCanvas = 0;      /* 换算后退化成一个点（模型框整块在画面外） */
    uint64_t u64Truncated = 0;      /* 超容量被丢：>0 就该升 nMaxBoxNum 或查误检 */
} CDetDecodeStat_S;

/**
 * @brief 头身关联参数
 */
typedef struct _BodyAssembleParam_
{
    float   fWidthHead = CLASSROOM_BODY_EXPAND_W_HEAD;    /* 各向左右扩几个头宽 */
    float   fDownHead = CLASSROOM_BODY_EXPAND_DOWN_HEAD;  /* 向下扩几个头高 */
    float   fUpHead = CLASSROOM_BODY_EXPAND_UP_HEAD;      /* 向上扩几个头高 */
    float   fContain = CLASSROOM_BODY_HEAD_CONTAIN;       /* 头框落在体框内的面积占比下限 */
    int32_t nMinBodyEdge = CLASSROOM_MIN_BODY_PIXEL;      /* 人体框短边下限，不足则回落外扩 */
} CBodyAssembleParam_S;

/**
 * @brief 一个人本帧的框
 */
typedef struct _PersonBox_
{
    CRectf_S stHead;
    float    fHeadScore = 0.0f;
    CRectf_S stBody;
    int32_t  nHeadIdx = -1;           /* 回查 vecHead 用（OSD 配色、按 ID 取原框） */
    int32_t  nBodyIdx = -1;           /* 命中的真体框下标，-1=体框由外扩推得 */
} CPersonBox_S;

/**
 * @brief 头身关联计数
 */
typedef struct _BodyAssembleStat_
{
    uint64_t u64HeadNum = 0;
    uint64_t u64BodyNum = 0;
    uint64_t u64Matched = 0;       /* 用到人体检测框的行数 */
    uint64_t u64Expanded = 0;      /* 回落外扩的行数 */
    uint64_t u64BodyUnmatched = 0; /* 没配到人头的体框 */
} CBodyAssembleStat_S;

/**
 * @brief   : 打平框数组 → 分析帧坐标检测列表
 * @param    {const std::vector<float>&} vecRaw：CLASSROOM_BOX_FLOAT_NUM × 框数（适配器输出）
 * @param    {const CDetMap&} rstMap：本帧映射，未就绪时不产出任何框
 * @param    {const CDetectParam_S&} rstParam：门限与容量
 * @param    {DetectSource_E} enSource：这批框是谁给的
 *            由入参给出而不是让调用方事后遍历覆写：漏覆写一次的后果是「人体框被人头跟踪器当小头用」
 * @param    {std::vector<CDetection_S>&} rvecOut：OUT=有效框，函数内先 clear（调用方须 reserve 容量）
 * @param    {CDetDecodeStat_S&} rstStat：累加
 * @return   {size_t} 有效框数
 */
size_t decodeBoxes(const std::vector<float> &vecRaw, const CDetMap &rstMap,
                   const CDetectParam_S &rstParam, DetectSource_E enSource,
                   std::vector<CDetection_S> &rvecOut, CDetDecodeStat_S &rstStat);

/**
 * @brief 一个人脸：框 + 五点，全部在分析帧坐标系
 */
typedef struct _FaceDet_
{
    CRectf_S  stBox;                                       /* 人脸框（分析帧） */
    float     fScore = 0.0f;
    CPointf_S astPoints[CLASSROOM_FACE_LANDMARK_NUM] = { { 0.0f, 0.0f } };
    float     afPointScore[CLASSROOM_FACE_LANDMARK_NUM] = { 0.0f };
} CFaceDet_S;

/**
 * @brief 人脸解码参数
 */
typedef struct _FaceDecodeParam_
{
    float   fScoreThreshold = CLASSROOM_DETECT_SCORE_THRESHOLD; /* 解析侧复检门 */
    /* 检测侧短边下限 */
    int32_t nMinBoxEdge = CLASSROOM_MIN_FACE_DETECT_PIXEL;
    int32_t nMaxBoxNum = CLASSROOM_MAX_PERSONS;
    /* 近/远分带边界 */
    int32_t nNearEdge = CLASSROOM_MIN_FACE_PIXEL;
    float   fFarGateRatio = CLASSROOM_FACE_FAR_GATE_RATIO;
} CFaceDecodeParam_S;

/**
 * @brief 人脸解码计数
 */
typedef struct _FaceDecodeStat_
{
    uint64_t u64Rows = 0;
    uint64_t u64Accepted = 0;
    uint64_t u64Malformed = 0;      /* 长度不整 / 非有限值 / 框倒挂 */
    uint64_t u64MapNotReady = 0;
    uint64_t u64LowScore = 0;
    uint64_t u64TooSmall = 0;
    uint64_t u64OffCanvas = 0;
    uint64_t u64Truncated = 0;
    uint64_t u64PointOutOfBox = 0;
} CFaceDecodeStat_S;

/**
 * @brief   : 打平的人脸输出 → 分析帧坐标的人脸列表
 * @param    {const std::vector<float>&} vecRaw：CLASSROOM_FACE_BOX_FLOAT_NUM × 框数
 * @param    {const CDetMap&} rstMap：本帧映射，未就绪时不产出任何框
 * @param    {const CFaceDecodeParam_S&} rstParam：门限与容量
 * @param    {std::vector<CFaceDet_S>&} rvecOut：OUT=有效人脸，函数内先 clear（调用方须 reserve）
 * @param    {CFaceDecodeStat_S&} rstStat：累加
 * @return   {size_t} 有效人脸数
 */
size_t decodeFaces(const std::vector<float> &vecRaw, const CDetMap &rstMap,
                   const CFaceDecodeParam_S &rstParam,
                   std::vector<CFaceDet_S> &rvecOut, CFaceDecodeStat_S &rstStat);

/**
 * @brief   : 一张已换算到画布坐标的人脸框按远近带取用的置信度门
 * @param    {const CRectf_S&} stCanvas：画布坐标下的人脸框（decodeFaces 里 modelToCanvas 之后的那个）
 * @param    {const CFaceDecodeParam_S&} rstParam：基门 / 近带边界 / 远带折扣
 * @return   {float} 近带（短边 >= nNearEdge）= 基门；远带 = 基门×fFarGateRatio；
 *           折扣配错（<=0 或 >=1）或 nNearEdge 非法时一律回基门——
 *           这条门只允许"把后排的脸放进来"，绝不允许反过来把近带的门抬高
 */
float faceGateScore(const CRectf_S &stCanvas, const CFaceDecodeParam_S &rstParam);

/**
 * @brief   : 一张已换算到画布坐标的头框按远近带取用的置信度门
 * @param    {const CRectf_S&} stCanvas：画布坐标下的头框（decodeBoxes 里 modelToCanvas 之后的那个）
 * @param    {const CDetectParam_S&} rstParam：基门 / 远带边界 nFarEdgePx / 远带折扣 fFarGateRatio
 * @return   {float} 近带（短边 >= nFarEdgePx）= 基门；远带 = 基门×fFarGateRatio；
 *           折扣配错（<=0 或 >=1）或 nFarEdgePx <= 0 时一律回基门——
 *           与 faceGateScore 同一条钳制：只允许"把后排的头放进来"，
 *           绝不允许反过来把门抬得比基门更严
 */
float headGateScore(const CRectf_S &stCanvas, const CDetectParam_S &rstParam);

/**
 * @brief   : 把本拍解出的人脸配到特征行上（一行只留一张脸）
 * @param    {const std::vector<CFaceDet_S>&} vecFace：decodeFaces 的输出（分析帧坐标）
 * @param    {CPersonFeature_S*} patRows / nRowNum：本拍已写好的特征行，只**读**头框
 * @param    {const std::vector<CTrackItem_S>&} vecTrack：本拍轨迹表（nDetIdx 是 ID 链的第一判据）
 * @param    {const std::vector<int32_t>&} vecRowOfHead：头框下标 → 行下标，-1=没配到行
 * @param    {std::vector<int32_t>&} rvecRowOfFace：OUT=长度等于 vecFace.size()，每张脸配到的行号，-1=没配上
 * @param    {int32_t&} rnDropped：OUT=因"这行已经有脸"而被放弃的脸数
 */
void assignFaceRows(const std::vector<CFaceDet_S> &vecFace, CPersonFeature_S *patRows,
                    int32_t nRowNum, const std::vector<CTrackItem_S> &vecTrack,
                    const std::vector<int32_t> &vecRowOfHead,
                    std::vector<int32_t> &rvecRowOfFace, int32_t &rnDropped);

/**
 * @brief   : 人头框 → 人体框外扩，并钳制到分析画布内
 * @param    {const CFrameGeometry&} stSrcToCanvas：用来取画布宽高做钳制
 * @param    {float} fWidthHead / fDownHead / fUpHead：倍数（以头宽/头高为单位，跨分辨率稳定）
 * @return   {CRectf_S} 保证落在画布内且右下不小于左上
 */
CRectf_S expandHeadToBody(const CFrameGeometry &stSrcToCanvas, const CRectf_S &stHead,
                          float fWidthHead, float fDownHead, float fUpHead);

/**
 * @brief   : 人头框 × 人体框 关联，产出每人一行
 * @param    {const std::vector<CDetection_S>&} vecHead / vecBody：均为分析帧坐标
 * @param    {const CFrameGeometry&} stSrcToCanvas：外扩回落时用于钳制
 * @param    {const CBodyAssembleParam_S&} rstParam：外扩倍数与归属判据
 * @param    {std::vector<CPersonBox_S>&} rvecPerson：OUT=每人一行，按 vecHead 顺序，函数内先 clear
 * @param    {CBodyAssembleStat_S&} rstStat：累加
 */
void assemblePersons(const std::vector<CDetection_S> &vecHead,
                     const std::vector<CDetection_S> &vecBody,
                     const CFrameGeometry &stSrcToCanvas,
                     const CBodyAssembleParam_S &rstParam,
                     std::vector<CPersonBox_S> &rvecPerson,
                     CBodyAssembleStat_S &rstStat);

/* 内框落在外框内的面积 / 内框面积，无交集为 0 */
float containRatioOf(const CRectf_S &stInner, const CRectf_S &stOuter);

/* 两框 IoU，任一框退化或无交集为 0 */
float iouOf(const CRectf_S &stA, const CRectf_S &stB);

/**
 * @brief   : 框是否贴到画面边缘（被切一半）
 * @param    {int} nEdgePx：算「贴边」的边缘带宽度
 */
bool touchesEdgeOf(const CRectf_S &stBox, int nCanvasW, int nCanvasH, int nEdgePx);
}
