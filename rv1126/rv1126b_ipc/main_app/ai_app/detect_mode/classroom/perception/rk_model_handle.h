/**
 * @FilePath     : rk_model_handle.h
 * @Author       : leiyy
 * @Date         : 2026-10-19 16:34:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 15:34:00
 * @Description  : rknn 检测模型适配器
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "OutputDataEXT.hpp"    /* 只有 POD 框结构，不带 rknn 依赖 */

#include "model_pool.h"

/* 带 rknn/模型上下文依赖的封装类只进本适配器的 .cpp，这里用前向声明挡住传递 */
namespace Inference_NS
{
class CYolov5;
class CYoloUltralytics;
class CYoloUltralyticsPoint;
}

namespace Classroom
{
/* 打平框长的常量 CLASSROOM_BOX_FLOAT_NUM 定义在 classroom_define.h：
   本文件是生产者，detect_decode 是消费者，两处必须同值，故不放在本头。
   6 个 float = 24B/框，60 人上限也才 1.4KB，直接进 CInferResult_S 的 float 缓冲，
   不为它单开一条结果通路（调度器、回调、单测都只认 vector<float> 这一种载体） */

/**
 * @brief   : YOLO 系检测模型的 IModelHandle 实现
 * @note    : 非线程安全，且刻意不做线程安全 —— run() 只允许在 CInferScheduler 的 NPU worker
 *            线程上被调用（§4.3：串行化即线程安全）。create() 只在池子的加载路径上调用，
 *            那里已有 m_mtxLoad 串行保护
 */
class CRkModelHandle : public IModelHandle
{
public:
    /**
     * @brief   : 按描述建句柄并完成 init（加载即在此发生，百 ms 级）
     * @param    {const CModelDesc_S&} stDesc：模型描述，strJsonPath 必须是绝对路径
     * @return   {ModelPtr} 失败返回 nullptr，原因已写日志（池子据此刻 LOAD_FAILED）
     * @note    : 只支持 backend=yolov5/yolov8/yolov8_point；raw 类模型（FastPose、表情）
     *            走各自的适配器，不在这里塞一个「什么都能装」的分支——后端选错的表现是
     *            框乱飞而不是报错，宁可可观测地失败
     */
    static ModelPtr create(const CModelDesc_S &stDesc);

    ~CRkModelHandle() override;

    CRkModelHandle(const CRkModelHandle &) = delete;
    CRkModelHandle &operator=(const CRkModelHandle &) = delete;

    /**
     * @brief   : 推理一次并把结果打平进 rvfOutput
     * @param    {const uint8_t*} pucInput：RGA 直接吐出的连续 RGB/BGR 张量（无 stride 空隙）
     * @param    {size_t} nInBytes：输入字节数，与本句柄期望值不符直接失败（不猜）
     * @param    {std::vector<float>&} rvfOutput：OUT=打平结果，函数内先 clear。
     *            行长由 backend 定：只解框的是 CLASSROOM_BOX_FLOAT_NUM×框数，
     *            带关键点的是 CLASSROOM_FACE_BOX_FLOAT_NUM×框数
     * @return   {bool} false=本帧无结果，上层按「证据不足」处理，不得当成「屏上没人」
     */
    bool run(const uint8_t *pucInput, size_t nInBytes, std::vector<float> &rvfOutput) override;

    /* 加载期实测的驻留增量；测不出来返回 0，池子改用配置申报值 */
    int64_t residentKb() const override { return m_llResidentKb; }

    /* 门限下发（需求 11）：检测器每拍按配置调用，值没变则不碰后端 */
    void setThresholds(float fConfidence, float fNms) override;

    /* 模型自己声明的输入尺寸与期望字节数：建裁剪档后必须与之相等，否则喂进去的是错张量 */
    uint32_t modelInputWidth() const { return m_u32InputW; }
    uint32_t modelInputHeight() const { return m_u32InputH; }
    size_t expectedInputBytes() const;

    /* 解码后的框数（run 成功后有效），供观测与「框数与 float 数是否自洽」的自检 */
    uint32_t lastBoxNum() const { return m_u32LastBoxNum; }

private:
    /* 本句柄实际持有哪一家封装类。用枚举而不是 m_spV8 之类的是不是空指针：
       加第三家之后「空指针判身份」会退化成一串互斥判断的堆叠，
       而漏掉一条分支的表现是「推理照跑、输出按错的布局解析」 */
    enum class Kind : int
    {
        V5 = 0,
        V8,
        V8_POINT
    };

    CRkModelHandle();

    bool initByKind(const std::string &strJsonPath);
    /* 按 Kind 从模型自己的张量属性读输入尺寸（三家都同名同参，但没有共同基类指针可用） */
    bool inputSize(int &rnW, int &rnH, int &rnCh);
    bool applyThreshold(void);
    void packBoxes(std::vector<float> &rvfOutput);
    void packPoints(std::vector<float> &rvfOutput);

    std::unique_ptr<Inference_NS::CYolov5>               m_spV5;
    std::unique_ptr<Inference_NS::CYoloUltralytics>      m_spV8;
    std::unique_ptr<Inference_NS::CYoloUltralyticsPoint> m_spV8Point;
    Kind      m_enKind = Kind::V5;

    /* 复用容器：推理热路径上不重新分配（60 人 × 24B，一次预留就是永久预留） */
    std::vector<Inference_NS::BoxData_S>    m_vecBox;
    std::vector<Inference_NS::PointData_S>  m_vecPoint;

    std::string m_strKey;
    uint32_t    m_u32InputW = 0;
    uint32_t    m_u32InputH = 0;
    uint32_t    m_u32Channel = 3;
    int64_t     m_llResidentKb = 0;
    uint32_t    m_u32LastBoxNum = 0;

    float m_fConfidence = -1.0f;    /* <0 表示还没下发过，此时沿用模型 JSON 里的门限 */
    float m_fNms = -1.0f;
    float m_fAppliedConf = -1.0f;   /* 真正推进过后端的值，用于「变了才下发」 */
    float m_fAppliedNms = -1.0f;
};
}
