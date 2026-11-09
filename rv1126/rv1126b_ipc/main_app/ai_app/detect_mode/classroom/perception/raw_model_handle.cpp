/**
 * @FilePath     : raw_model_handle.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-19 13:57:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 13:57:00
 * @Description  : 裸 rknn 适配器实现
 */

#include "raw_model_handle.h"

#include <cstring>

#include "rknn_api.h"

#include "classroom_mem.h"
#include "dlog.h"

namespace Classroom
{
namespace
{
/* 驻留实测的下限：与 CRkModelHandle 同一口径。小于这个量的增量基本都是 malloc 抖动 */
constexpr int64_t LL_RESIDENT_VALID_MIN_KB = 256;

/* 路数上限的真身在头文件（成员数组的维度得是编译期常量）。这里再引一次同名 constexpr
   是给 run() 里那个 rknn_output 栈数组和 %u 格式串用——用定长栈数组而不是
   vector<rknn_output>：那是把 rknn 类型塞进热路径的堆上，而本层的内存图景必须能在 init 时算完 */
constexpr uint32_t U32_RAW_OUT_MAX = CLASSROOM_RAW_OUT_MAX;

constexpr uint32_t U32_RAW_IN_MAX_DIMS = 4;
int64_t rssNowKb(void)
{
    const CMemInfo_S stMem = classroomQueryMem();
    return stMem.bValid ? stMem.llVmRssKb : 0;
}

const char *tensorTypeName(int nType)
{
    switch (nType)
    {
        case RKNN_TENSOR_FLOAT32: return "FP32";
        case RKNN_TENSOR_FLOAT16: return "FP16";
        case RKNN_TENSOR_INT8:    return "INT8";
        case RKNN_TENSOR_UINT8:   return "UINT8";
        default:                  return "OTHER";
    }
}

/* 平台句柄的两个转换点收在这里：成员按 uint64_t 存（头文件不许出现 rknn 类型），
   进出 runtime 各转一次，别在调用点散着写强转 */
inline rknn_context ctxOf(uint64_t ullCtx)
{
    return static_cast<rknn_context>(ullCtx);
}

/* 为什么把 fmt 打进加载日志（而不是打进 CRawTensorShape_S）：det 概率图那两维谁是 H、谁是 W，
   唯一的原料就是这一项（共享仓自己也是按它分支的，CVInferenceRK.cpp:401 那段 NCHW/NHWC 各取
   dims[1..3] 的两种次序），而它今天在全机没有任何读数。它又是 rknn 的枚举，
   放进头文件那份结构体就等于把枚举值抄第二遍——抄错的那一侧是静默的宽高对调。
   NCHW 恰好是 0：没填过的 attr 也会读成 NCHW，所以这里列全四种，不做 if/else 二分 */
const char *tensorFmtName(int nFmt)
{
    switch (nFmt)
    {
        case RKNN_TENSOR_NCHW:      return "NCHW";
        case RKNN_TENSOR_NHWC:      return "NHWC";
        case RKNN_TENSOR_NC1HWC2:   return "NC1HWC2";
        case RKNN_TENSOR_UNDEFINED: return "UNDEFINED";
        default:                    return "OTHER";
    }
}
}

CRawModelHandle::~CRawModelHandle()
{
    /* F1 后 runStart 是阻塞提交：m_bInflight 为真只意味着「推理已完成、输出还没收」，
       上下文必然空闲，不存在"销毁还在执行的上下文"的窗口——卸载竞态兜底收敛为清标记 */
    m_bInflight = false;
    /* 显式销毁：只靠进程退出会让 NPU arena 一直挂到内核回收，
       而课堂侧的「关掉功能=真的省内存」（需求 10）要的是卸载即归还 */
    if (0 != m_ullCtx)
    {
        rknn_destroy(ctxOf(m_ullCtx));
        m_ullCtx = 0;
    }
}

ModelPtr CRawModelHandle::create(const CModelDesc_S &stDesc)
{
    if (stDesc.strModelPath.empty())
    {
        dlog_error("raw_handle: %s 无 model_path，raw 后端无法推断加载哪个 .rknn",
                   stDesc.strKey.c_str());
        return ModelPtr();
    }

    const int64_t llBeforeKb = rssNowKb();

    std::shared_ptr<CRawModelHandle> spHandle(new CRawModelHandle());
    spHandle->m_strKey = stDesc.strKey;

    if (!spHandle->initFromPath(stDesc.strModelPath))
    {
        dlog_error("raw_handle: %s 加载失败,path=%s", stDesc.strKey.c_str(),
                   stDesc.strModelPath.c_str());
        return ModelPtr();
    }
    if (!spHandle->queryTensors())
    {
        return ModelPtr();
    }

    const int64_t llDeltaKb = rssNowKb() - llBeforeKb;
    if (llDeltaKb >= LL_RESIDENT_VALID_MIN_KB)
    {
        spHandle->m_llResidentKb = llDeltaKb;
    }

    if (0 != stDesc.u32InputW &&
        (stDesc.u32InputW != spHandle->m_u32InputW || stDesc.u32InputH != spHandle->m_u32InputH))
    {
        /* 不失败：裁剪档按配置建、张量按模型走，两者不等时 run() 会因字节数不符当场拒绝。
           但把原因说在这里，省得现场只看到一条「大小不一致」的数字对不上 */
        dlog_warn("raw_handle: %s 输入 %ux%u 与配置 %ux%u 不符, 按模型值",
                  stDesc.strKey.c_str(), spHandle->m_u32InputW, spHandle->m_u32InputH,
                  stDesc.u32InputW, stDesc.u32InputH);
    }

    dlog_info("raw_handle: %s 就绪 input=%ux%ux%u output=%ux%uelem resident=%lldKB%s path=%s",
              stDesc.strKey.c_str(), spHandle->m_u32InputW, spHandle->m_u32InputH,
              spHandle->m_u32Channel, spHandle->m_u32OutNum, spHandle->m_u32OutElems,
              static_cast<long long>(spHandle->m_llResidentKb),
              (spHandle->m_llResidentKb > 0) ? "" : "(not measurable, use declared)",
              stDesc.strModelPath.c_str());
    return spHandle;
}

bool CRawModelHandle::initFromPath(const std::string &strModelPath)
{
    rknn_context hCtx = 0;

    const int nRet = rknn_init(&hCtx, const_cast<char *>(strModelPath.c_str()), 0, 0, NULL);
    if (nRet < 0)
    {
        dlog_error("raw_handle: rknn_init 失败 ret=%d path=%s", nRet, strModelPath.c_str());
        return false;
    }
    m_ullCtx = static_cast<uint64_t>(hCtx);

    rknn_sdk_version stVer = {};
    if (0 == rknn_query(hCtx, RKNN_QUERY_SDK_VERSION, &stVer, sizeof(stVer)))
    {
        dlog_info("raw_handle: RKNN API 版本=%s 驱动版本=%s", stVer.api_version, stVer.drv_version);
    }
    return true;
}

bool CRawModelHandle::queryTensors()
{
    const rknn_context hCtx = ctxOf(m_ullCtx);

    rknn_input_output_num stIo = {};
    if (rknn_query(hCtx, RKNN_QUERY_IN_OUT_NUM, &stIo, sizeof(stIo)) < 0)
    {
        dlog_error("raw_handle: %s in/out num 查询失败", m_strKey.c_str());
        return false;
    }
    if (0 == stIo.n_input || stIo.n_output == 0)
    {
        dlog_error("raw_handle: %s 无张量，n_in=%u n_out=%u", m_strKey.c_str(),
                   stIo.n_input, stIo.n_output);
        return false;
    }
    if (stIo.n_input != 1 || stIo.n_output > U32_RAW_OUT_MAX)
    {
        /* 多输入模型要的是另一套喂法（每路一个尺寸档、还得配对），不是本类的范围。
           宁可拒绝加载并说清原因，也不要「按第一路凑合喂」——那出来的结果是能跑的垃圾 */
        dlog_error("raw_handle: %s 需 n_in==1 且 n_out<=%u, 实得n_in=%u n_out=%u",
                   m_strKey.c_str(), U32_RAW_OUT_MAX, stIo.n_input, stIo.n_output);
        return false;
    }
    m_u32OutNum = stIo.n_output;

    rknn_tensor_attr stIn = {};
    stIn.index = 0;
    if (rknn_query(hCtx, RKNN_QUERY_INPUT_ATTR, &stIn, sizeof(stIn)) < 0)
    {
        dlog_error("raw_handle: %s input attr 查询失败", m_strKey.c_str());
        return false;
    }
    /* dims 按 NHWC 读：FastPose 自报 [1,256,192,3]，即 H=256 / W=192。
       拿 NCHW 的次序去读会把宽高对调，而宽高对调的图不会报错——只会让 26 个点整体错位 */
    if (stIn.n_dims < 3 || stIn.n_dims > U32_RAW_IN_MAX_DIMS ||
        0 == stIn.dims[stIn.n_dims - 1] || 0 == stIn.dims[stIn.n_dims - 2])
    {
        dlog_error("raw_handle: %s 输入 dims 异常 n=%u [%u,%u,%u,%u]", m_strKey.c_str(),
                   stIn.n_dims, stIn.dims[0], stIn.dims[1], stIn.dims[2], stIn.dims[3]);
        return false;
    }
    m_u32Channel = stIn.dims[stIn.n_dims - 1];
    m_u32InputW  = (stIn.n_dims >= 4) ? stIn.dims[stIn.n_dims - 2] : 0;
    m_u32InputH  = (stIn.n_dims >= 4) ? stIn.dims[stIn.n_dims - 3] : 0;
    if (3 == stIn.n_dims)
    {
        /* 只有 [H,W,C] 的写法（批次维省略）：C 已知，剩下两维即 H、W */
        m_u32InputW = stIn.dims[1];
        m_u32InputH = stIn.dims[0];
    }
    if (0 == m_u32InputW || 0 == m_u32InputH)
    {
        dlog_error("raw_handle: %s 无法从 dims 解析输入 W/H", m_strKey.c_str());
        return false;
    }
    if (3 != m_u32Channel)
    {
        /* 本类只负责喂 RGA 直出的 3 通道图。灰度模型要的是另一条生成路径（RGA 出 Y 平面），
           在这里放行等于把它推给「配置申报 3 通道、实际张量不是」的静默错喂法 */
        dlog_error("raw_handle: %s 输入 channel=%u，本适配器只喂 3 通道 RGB/BGR",
                   m_strKey.c_str(), m_u32Channel);
        return false;
    }
    if (RKNN_TENSOR_NHWC != stIn.fmt)
    {
        /* pass_through=0 时 runtime 会按我们报的 fmt 转换，所以这不是错误；
           但内部格式不是 NHWC 值得留一行——真出问题时会先怀疑喂法 */
        dlog_warn("raw_handle: %s 输入 fmt=%d (非 NHWC)，runtime 会转换", m_strKey.c_str(),
                  static_cast<int>(stIn.fmt));
    }

    uint32_t u32ElemsSum = 0;
    int nFirstOutFmt = -1;
    for (uint32_t nIdx = 0; nIdx < m_u32OutNum; ++nIdx)
    {
        rknn_tensor_attr stOut = {};
        stOut.index = nIdx;
        if (rknn_query(hCtx, RKNN_QUERY_OUTPUT_ATTR, &stOut, sizeof(stOut)) < 0)
        {
            dlog_error("raw_handle: %s output attr %u 查询失败", m_strKey.c_str(), nIdx);
            return false;
        }
        u32ElemsSum += stOut.n_elems;
        if (!m_astOutShape[nIdx].fillFromRaw(stOut.n_dims, stOut.dims, stOut.n_elems))
        {
            /* **不因此拒绝加载**：run() 只需要 n_elems 就能打平缓冲，形状是「能不能解释」
               的前提而不是「能不能跑」的前提。姿态那一路今天已在机上跑着，为一条我们没见过
               的自述格式把它拒掉，是拿已验证的路去给新字段陪葬。
               但必须留一行——OCR 那一路拿不到形状会当场降级，现象是「板书一直不出字」，
               没有这行日志就会去查阈值 */
            dlog_warn("raw_handle: %s output %u 形状自述不可用, 按未知处理: n_dims=%u [%u,%u,%u,%u] "
                      "n_elems=%u",
                      m_strKey.c_str(), nIdx, stOut.n_dims, stOut.dims[0], stOut.dims[1],
                      stOut.dims[2], stOut.dims[3], stOut.n_elems);
        }
        if (0 == nIdx)
        {
            m_u32OutElems = stOut.n_elems;
            nFirstOutFmt = static_cast<int>(stOut.fmt);
        }
    }
    if (0 == u32ElemsSum)
    {
        dlog_error("raw_handle: %s 输出元素数为 0，无可解码内容", m_strKey.c_str());
        return false;
    }

    dlog_info("raw_handle: %s 输入 %ux%ux%u native=%s size=%uB stride_size=%uB, "
              "out=%u x %u elem, 首路 dims=[%u,%u,%u,%u] fmt=%s",
              m_strKey.c_str(), m_u32InputW, m_u32InputH, m_u32Channel,
              tensorTypeName(static_cast<int>(stIn.type)), stIn.size, stIn.size_with_stride,
              m_u32OutNum, m_u32OutElems, m_astOutShape[0].u32Dim[0], m_astOutShape[0].u32Dim[1],
              m_astOutShape[0].u32Dim[2], m_astOutShape[0].u32Dim[3], tensorFmtName(nFirstOutFmt));
    return true;
}

CRawTensorShape_S CRawModelHandle::outputShape(uint32_t nIdx) const
{
    /* 越界给「未知」（全零）而不是断言或引用到数组外：调用点在感知链上，
       拿一个不存在的输出去解释的正确结果是「这一路没有可信形状」 */
    return (nIdx < CLASSROOM_RAW_OUT_MAX) ? m_astOutShape[nIdx] : CRawTensorShape_S();
}

size_t CRawModelHandle::expectedInputBytes() const
{
    /* uint8 输入：一个像素一个通道一字节。模型自报的 size 是按 native 类型（FP16 时为两倍）
       算的，不能拿它来校验我们喂进去的字节数——这是本类唯一一处「期望值与模型自述值不同」，
       原因见头文件约定 2 */
    return static_cast<size_t>(m_u32InputW) * m_u32InputH * m_u32Channel;
}

/* run() / runStart() 共用的前半段：校验 + 提交输入。校验失败的两条日志只在这里打，
   两段式与同步路共用同一套「为什么这帧没有结果」的说法 */
bool CRawModelHandle::submitInput(const uint8_t *pucInput, size_t nInBytes)
{
    if (0 == m_ullCtx || nullptr == pucInput)
    {
        return false;
    }
    if (nInBytes != expectedInputBytes())
    {
        dlog_error("raw_handle: %s 输入 %zu 字节, 模型要 %u x %u x %u = %zu",
                   m_strKey.c_str(), nInBytes, m_u32InputW, m_u32InputH, m_u32Channel,
                   expectedInputBytes());
        return false;
    }

    const rknn_context hCtx = ctxOf(m_ullCtx);

    rknn_input stIn = {};
    stIn.index        = 0;
    stIn.buf          = const_cast<uint8_t *>(pucInput);
    stIn.size         = static_cast<uint32_t>(nInBytes);
    /* pass_through=0：把 uint8→模型输入类型的转换（含烧进图里的 mean/std）整个交给 runtime。
       这里若写 1，等于宣布「我给的已经是模型要的张量了」，runtime 便不再归一化，
       而 RGA 直出的图是 0~255 的原值——输出会退化成一片没有分数梯度的垃圾，且不报任何错 */
    stIn.pass_through = 0;
    stIn.type         = RKNN_TENSOR_UINT8;
    stIn.fmt          = RKNN_TENSOR_NHWC;

    if (rknn_inputs_set(hCtx, 1, &stIn) < 0)
    {
        dlog_warn("raw_handle: %s rknn_inputs_set 失败, size=%zu", m_strKey.c_str(), nInBytes);
        return false;
    }
    return true;
}

/* run() / runWait() 共用的收尾：outputs_get → 打平拷贝 → 释放。
   outputs_get 在同步语义下自己等推理完成；F1 后 runStart 阻塞返回时结果必然就绪 */
bool CRawModelHandle::collectOutputs(std::vector<float> &rvfOutput)
{
    const rknn_context hCtx = ctxOf(m_ullCtx);

    rvfOutput.clear();

    rknn_output astOut[U32_RAW_OUT_MAX] = {};
    for (uint32_t nIdx = 0; nIdx < m_u32OutNum; ++nIdx)
    {
        astOut[nIdx].index       = nIdx;
        astOut[nIdx].is_prealloc = 0;
        /* want_float=1：让 runtime 把 native FP16/INT8 统一转成 fp32 交出来。
           自解码 FP16 需要 __fp16：arm-gcc AArch32 默认 -mfp16-format=none，不开该选项
           就没有这个类型（opencv cvdef.h:384 同款守卫）——是缺编译选项，不是工具链缺能力；
           与其为自解码加选项，不如让 runtime 转：这里每次只有几十~几百个元素，
           转换成本相对几十 ms 的推理可以忽略 */
        astOut[nIdx].want_float  = 1;
    }
    if (rknn_outputs_get(hCtx, m_u32OutNum, astOut, NULL) < 0)
    {
        dlog_warn("raw_handle: %s rknn_outputs_get 失败", m_strKey.c_str());
        return false;
    }

    /* 一次推理的输出总量是固定的（几十~几百个 float），预留一次就够：
       调用方复用同一个 vector，容量在第一次之后不再变化 */
    const size_t nNeed = static_cast<size_t>(m_u32OutElems) * m_u32OutNum;
    if (rvfOutput.capacity() < nNeed)
    {
        rvfOutput.reserve(nNeed);
    }
    for (uint32_t nIdx = 0; nIdx < m_u32OutNum; ++nIdx)
    {
        const float *pcfBuf = static_cast<const float *>(astOut[nIdx].buf);
        const size_t nFloats = astOut[nIdx].size / sizeof(float);
        if (nullptr == pcfBuf || 0 == nFloats)
        {
            dlog_warn("raw_handle: %s 输出 %u 为空", m_strKey.c_str(), nIdx);
            continue;
        }
        rvfOutput.insert(rvfOutput.end(), pcfBuf, pcfBuf + nFloats);
    }

    /* 释放必须与 outputs_get 成对：is_prealloc=0 时 buf 是 runtime 分配的，
       漏掉这一次就是每次推理漏一块——姿态一路每秒要做好几次，一天就漏光 */
    rknn_outputs_release(hCtx, m_u32OutNum, astOut);

    if (rvfOutput.empty())
    {
        dlog_warn("raw_handle: %s 未产出浮点，模型声明 %u elem x %u tensor",
                  m_strKey.c_str(), m_u32OutElems, m_u32OutNum);
        return false;
    }
    if (1 == m_u32OutNum && rvfOutput.size() != nNeed)
    {
        /* 单路模型才敢逐元素核对：多路打平时的各路长度可以不等，
           拿首路长度乘路数当期望值会把正常结果判成截断 */
        dlog_warn("raw_handle: %s 得 %zu 浮点, 模型声明 %zu", m_strKey.c_str(),
                  rvfOutput.size(), nNeed);
        return false;
    }
    /* 本层只保证「拿到一份完整的浮点缓冲」，元素怎么解释（26 点 / 分类向量）一律留给调用方 */
    return true;
}

bool CRawModelHandle::run(const uint8_t *pucInput, size_t nInBytes, std::vector<float> &rvfOutput)
{
    if (!submitInput(pucInput, nInBytes))
    {
        return false;
    }
    if (rknn_run(ctxOf(m_ullCtx), NULL) < 0)
    {
        dlog_warn("raw_handle: %s rknn_run 失败", m_strKey.c_str());
        return false;
    }
    return collectOutputs(rvfOutput);
}

bool CRawModelHandle::runStart(const uint8_t *pucInput, size_t nInBytes)
{
    /* 同上下文一帧在飞是 rknn 的约束：上一帧没收就再喂，结果不可定义。
       调度器的同键串行闸是第一道，这里是第二道（防将来有人绕过调度器直接喂） */
    if (m_bInflight)
    {
        dlog_warn("raw_handle: %s runStart 重入（上帧未收）", m_strKey.c_str());
        return false;
    }
    if (!submitInput(pucInput, nInBytes))
    {
        return false;
    }

    if (rknn_run(ctxOf(m_ullCtx), NULL) < 0)
    {
        dlog_warn("raw_handle: %s rknn_run 失败", m_strKey.c_str());
        return false;
    }
    m_bInflight = true;
    return true;
}

bool CRawModelHandle::runWait(std::vector<float> &rvfOutput)
{
    if (!m_bInflight)
    {
        return false;    /* 没有在飞的帧：成对契约被破坏，拒绝凭空取一次 */
    }

    /* runStart 阻塞返回即本帧推理完成，这里直接收输出——本文件不许再出现任何
       异步等待调用（wedge 套源码守卫盯着） */
    m_bInflight = false;
    return collectOutputs(rvfOutput);
}
}
