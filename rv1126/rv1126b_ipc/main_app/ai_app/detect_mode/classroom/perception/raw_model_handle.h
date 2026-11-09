/**
 * @FilePath     : raw_model_handle.h
 * @Author       : leiyy
 * @Date         : 2026-10-19 13:57:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 13:57:00
 * @Description  : 裸 rknn 模型的 rknn 适配器
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model_pool.h"

namespace Classroom
{
/* 一个裸句柄最多记几路输出。FastPose 1 路、分类器 1~3 路、PP-OCR 的 det/rec 各 1 路。
   必须是定长数组而不是 vector<rknn_output>：那等于把 runtime 类型塞进堆上，
   而本层的内存图景要在 init 时一次算完（§4.2 规则 4 的记账靠它）。
   超出这个数的模型不是「张量进、浮点向量出」的形状，queryTensors() 当场拒加载 */
#define CLASSROOM_RAW_OUT_MAX (8)

/* 一路张量最多几维。与 rknn_api.h 的 RKNN_MAX_DIMS 同值，但**不在这里 include 那个头**
   （文件头的传染条款：rknn 类型出了 perception 层就收不住）。
   取 16 而不是「实际用得到 4」：取小了不会报错，只会在 n_dims 介于两者之间的模型上
   把一份合法自述判成未知——而未知在调用方那边是「这块板子今天没识别」 */
#define CLASSROOM_RAW_TENSOR_MAX_DIMS (16)

/**
 * @brief 一路输出张量的自述形状（模型报什么就记什么，一维都不解释）
 * @note  为什么非要把它交出去：OCR 的 rec 画布宽高、det 概率图宽高，交付件里
 *        配置 json 写一份、文件名写一份、真正烧进 .rknn 的是第三份，三份可以互不相同。
 *        只有 rknn_query 这一份是推理时实际生效的那份——读前两份的后果是「尺寸喂对了、
 *        布局解错了」，而它不产生任何错误日志（pose 那边同理，见 §1.4）
 */
typedef struct _RawTensorShape_
{
    uint32_t u32NumDims = 0;                        /* 0 = 这一路没有可信形状 */
    uint32_t u32Dim[CLASSROOM_RAW_TENSOR_MAX_DIMS] = {};
    uint32_t u32Elems = 0;

    /**
     * @brief   : 由 runtime 自报的 (n_dims, dims, n_elems) 填一份形状
     * @return   {bool} false=这份自述不可用，且本结构保持全零（调用方据此降级，
     *                  不会拿到「一半填了一半没填」的中间态）
     * @note    : 三道门，每一道都得有自己挡得住的那一档（挡不住的门就是噪音，删）：
     *            1) 维数为 0 或越界 —— 不是本类形状的模型；
     *            2) 某一维为 0 —— 动态 shape 没跑过时 runtime 报的就是全 0，此时
     *               ∏dims(0) 与 n_elems(0) **相等**，只有这一道门能挡住「0×0 的画布」；
     *            3) ∏dims != n_elems —— runtime 的自述自己不一致。
     *            n_elems==0 不单列一道门：dims 全非 0 时乘积 ≥1 与它不等（第 3 道挡），
     *            有 0 维时第 2 道挡，再加一道就是第三种说法。
     *            乘积按 64 位累**且超过 uint32 就当场停**：32 位会回绕，64 位也会——
     *            绕回来的数**可以**恰好等于 n_elems（用例里那两枪是 {0x80000001,2} vs 2
     *            与 65536^4==2^64→0 vs 0）。一份绕回来自洽的 dims 比一份不自洽的更坏：
     *            它带着"已通过校验"的章
     */
    bool fillFromRaw(uint32_t u32NumDims, const uint32_t *pu32Dim, uint32_t u32Elems)
    {
        *this = _RawTensorShape_();
        if (nullptr == pu32Dim || 0 == u32NumDims || u32NumDims > CLASSROOM_RAW_TENSOR_MAX_DIMS)
        {
            return false;
        }
        uint64_t ullProd = 1;
        for (uint32_t nIdx = 0; nIdx < u32NumDims; ++nIdx)
        {
            if (0 == pu32Dim[nIdx])
            {
                return false;
            }
            ullProd *= pu32Dim[nIdx];
            if (ullProd > UINT32_MAX)
            {
                return false;
            }
        }
        if (ullProd != u32Elems)
        {
            return false;
        }
        this->u32NumDims = u32NumDims;
        for (uint32_t nIdx = 0; nIdx < u32NumDims; ++nIdx)
        {
            u32Dim[nIdx] = pu32Dim[nIdx];
        }
        this->u32Elems = u32Elems;
        return true;
    }

} CRawTensorShape_S;

/**
 * @brief   : 「张量进、浮点向量出」的裸 rknn 句柄
 * @note    : 非线程安全，且刻意不做 —— 见文件头线程模型
 */
class CRawModelHandle : public IModelHandle
{
public:
    /**
     * @brief   : 按描述建句柄并完成加载
     * @param    {const CModelDesc_S&} stDesc：strModelPath 必须是 .rknn 绝对路径
     * @return   {ModelPtr} 失败返回 nullptr，原因已写日志（池子据此刻 LOAD_FAILED）
     * @note    : 与 YOLO 路不同，本句柄**不需要** aiConfig*.json：尺寸与类型都从 .rknn
     *            自己的张量属性读（配置里那份 json 的 mean/std 与文件内嵌值本来就同源）
     */
    static ModelPtr create(const CModelDesc_S &stDesc);

    ~CRawModelHandle() override;

    CRawModelHandle(const CRawModelHandle &) = delete;
    CRawModelHandle &operator=(const CRawModelHandle &) = delete;

    /**
     * @brief   : 推理一次并把全部输出打平进 rvfOutput
     * @param    {const uint8_t*} pucInput：RGA 直出的连续 RGB/BGR888 张量（无 stride 空隙）
     * @param    {size_t} nInBytes：输入字节数，与本句柄期望值不符直接失败（不猜）
     * @param    {std::vector<float>&} rvfOutput：OUT=模型全部输出元素，函数内先 clear
     * @return   {bool} false=本次无结果，上层按「证据不足」处理，不得当成「没这个人」
     */
    bool run(const uint8_t *pucInput, size_t nInBytes, std::vector<float> &rvfOutput) override;

    /* ---- 两段式接口----
       runStart = inputs_set + **阻塞** rknn_run（返回即本帧推理完成）；runWait = 只收输出
       （同一套 outputs_get/拷贝/释放），不再有等待调用。同键串行约束不变：runStart 时
       已在飞 ⇒ 拒（调度器同键串行闸下面的第二道，防新调用方绕过调度器直接喂）。*/
    bool asyncCapable() const override { return true; }
    bool runStart(const uint8_t *pucInput, size_t nInBytes) override;
    bool runWait(std::vector<float> &rvfOutput) override;

    /* 加载期实测的驻留增量；测不出来返回 0，池子改用配置申报值 */
    int64_t residentKb() const override { return m_llResidentKb; }

    /* 模型自己声明的输入尺寸与期望字节数：建裁剪档后必须与之相等，否则喂进去的是错张量 */
    uint32_t modelInputWidth() const { return m_u32InputW; }
    uint32_t modelInputHeight() const { return m_u32InputH; }
    size_t expectedInputBytes() const;

    /* 单路输出元素数（FastPose=78）。多输出模型返回首路，调用方按 rvfOutput.size() 为准 */
    uint32_t outputElemsPerTensor() const { return m_u32OutElems; }
    uint32_t outputTensorNum() const { return m_u32OutNum; }

    /**
     * @brief   : 第 nIdx 路输出的自述形状
     * @return   {CRawTensorShape_S} 越界、或该路自述不可用时返回全零（u32NumDims==0）。
     *             调用方必须判 0，不要拿 u32Dim[0] 当尺寸用
     */
    CRawTensorShape_S outputShape(uint32_t nIdx) const;

private:
    CRawModelHandle() = default;

    bool initFromPath(const std::string &strModelPath);
    /* 查询输入/输出张量属性并定死本次运行期要用的常量；任何一项不合理都返回 false */
    bool queryTensors();
    /* run() 与 runWait() 共用的收尾：outputs_get → 打平拷贝 → 释放。
       拆两份写就会有一份漏掉 outputs_release（每次推理漏一块，一天漏光） */
    bool collectOutputs(std::vector<float> &rvfOutput);
    /* run() 与 runStart() 共用的前半段：校验 + 提交输入 */
    bool submitInput(const uint8_t *pucInput, size_t nInBytes);

    /* rknn_context 在 32 位机上是 uint32_t、64 位上是 uint64_t（平台 typedef）。
       头文件里不许出现 rknn_api.h（否则顺着 model_pool.h 传染到 bus/rules 层），
       所以这里按最宽的那档存，进 .cpp 再转回平台类型——自己抄一份 typedef 的话，
       抄错的一侧是静默截断句柄，表现为「偶尔推理失败」而不是编译错误 */
    uint64_t m_ullCtx = 0;
    std::string  m_strKey;

    uint32_t m_u32InputW = 0;
    uint32_t m_u32InputH = 0;
    uint32_t m_u32Channel = 3;
    uint32_t m_u32OutNum = 0;
    uint32_t m_u32OutElems = 0;
    /* 逐路的原始 dims（加载期一次写好，之后只读）。与 m_u32OutElems 并存是有意的：
       run() 只要元素数就能打平缓冲，形状是「能不能解释」的前提，不是「能不能跑」的前提 */
    CRawTensorShape_S m_astOutShape[CLASSROOM_RAW_OUT_MAX];

    int64_t m_llResidentKb = 0;

    /* 在飞标记。F1 后 runStart 阻塞返回即帧已完成，它只剩「输出已产出、尚未收」一个含义
       （旧两段式的 frame_id 成员已随等待调用一起移除）。
       bool 是普通成员：本类所有调用点都在 worker 线程上，串行化即无需原子 */
    bool m_bInflight = false;
};
}
