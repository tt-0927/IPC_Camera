/**
 * @FilePath     : classroom_face_port.h
 * @Author       : leiyy
 * @Date         : 2026-10-28 11:23:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-28 11:23:00
 * @Description  : 人脸身份比对的契约
 */

#pragma once

#include <cstdint>
#include <string>

namespace Classroom
{
/**
 * @brief 一次人脸比对的结论（由 IFaceGallery 填，或由引擎在缺件时填一个空值）
 */
typedef struct _FaceMatch_
{
    bool        bHit = false;                                   /* false = 没比对上（不代表没这个人） */
    std::string strPersonId;                                    /* 平台侧稳定 ID，名单去重的唯一依据 */
    std::string strName;                                        /* 展示名，可空（ID 有而名没下下来时） */
    float       fScore = 0.0f;                                  /* 相似度 0~1，账本按 rstParam 再设门 */
} CFaceMatch_S;

/**
 * @brief 人脸底库的抽象缝（FaceModule / Faiss+sqlite 的实现落在 perception 层的适配器）
 */
class IFaceGallery
{
public:
    IFaceGallery() = default;
    virtual ~IFaceGallery();

    IFaceGallery(const IFaceGallery &) = delete;
    IFaceGallery &operator=(const IFaceGallery &) = delete;

    /**
     * @brief   : 512 维特征 → 身份。**只允许引擎在人脸节拍里调**（可达 ms 级、可能碰磁盘）
     * @return   {bool} false 表示没有可信命中（rstOut.bHit=false），调用方按「无身份」处理，
     *                  不得把"没对上"当成"不是本班学生"（访客、临时插班都会没对上）
     */
    virtual bool match(const float *pafFeat, int32_t nDim, int64_t llNowMs, CFaceMatch_S &rstOut) = 0;

    /* 底库是否可用（未交付 / 未加载 / 空库）。必须是常数时间、不碰磁盘 */
    virtual bool available() const = 0;

    /* 库内人数。<0 表示未知。这是"应到人数"的一个兜底来源（平台没下发时应到时用） */
    virtual int32_t enrollNum() const = 0;

    virtual const char *name() const = 0;

    /**
     * @brief   : 到点就把底库重读一遍
     * @param    {int64_t} llNowMs：单调毫秒，由引擎的考勤段传入
     * @return   {bool} true=本次真的重载了（调用方可据此打一条 info，不必每拍打）
     */
    virtual bool poll(int64_t llNowMs)
    {
        (void)llNowMs;
        return false;
    }
};
}
