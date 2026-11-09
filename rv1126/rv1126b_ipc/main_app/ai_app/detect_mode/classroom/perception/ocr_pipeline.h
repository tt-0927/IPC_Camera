/**
 * @FilePath     : ocr_pipeline.h
 * @Author       : leiyy
 * @Date         : 2026-11-04 11:07:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-04 11:07:00
 * @Description  : 板书 OCR 的编排层
 */

#pragma once

#include <cstdint>
#include <vector>

#include "board_watcher.h"
#include "ocr_decode.h"
#include "raw_model_handle.h"

namespace Classroom
{
/* 单行文本的字节上限 */
#define CLASSROOM_OCR_LINE_TEXT_MAX (256)

/* det 概率图与送帧的宽高比容差，**有向**（mapW/mapH 对 imgW/imgH 的相对误差） */
#define CLASSROOM_OCR_ASPECT_TOLERANCE (0.2f)

/**
 * @brief 解码出来的 det 概率图形状（送帧坐标系之外唯一的一套坐标）
 */
typedef struct _OcrDetShape_
{
    int32_t nMapW = 0;
    int32_t nMapH = 0;
} COcrDetShape_S;

/**
 * @brief 解码出来的 rec 输入/输出形状
 * @note  画布宽高不在这里解：那是**输入**侧事实，句柄已经从 .rknn 的输入属性读过并
 *        直接给整数（modelInputWidth/Height），没有任何排法可猜。本结构只装输出侧的两个数
 */
typedef struct _OcrRecShape_
{
    int32_t  nTimeStep = 0;         /* CTC 步数 */
    uint32_t u32ClassNum = 0;       /* 每步的类别数（含 blank） */
    uint32_t u32DictShift = 0;      /* 输出第 i 类 → 字典第 (i-shift) 条，由 ctcClassMap 定 */
} COcrRecShape_S;

/**
 * @brief   : 把 det 首路输出的自述形状折成概率图宽高
 * @param    {const CRawTensorShape_S&} rstShape：句柄给的原始 dims（可能全零=不可用）
 * @param    {int32_t} nImgW / nImgH：**送进 det 的那张图**的宽高（不是源帧、不是分析帧）
 * @param    {COcrDetShape_S&} rstOut：OUT=只在返回 true 时被写
 * @return   {bool} false=这一路不可用（零个或 ≥2 个候选自洽），调用方必须降级而不是挑一个
 * @note    : 候选 = 任取两维的**有序**组合（(d[a],d[b]) 与 (d[b],d[a]) 是两个候选，
 *            a≠b；取"任意两维"而不是"相邻两维"是因为 [1,T,1,C] 这种带 1 的形状真存在，
 *            相邻限制会把唯一正确的读法挡在外面）。一个候选自洽要同时过两道门：
 *            1) 两维之积 == u32Elems —— 这道门顺带要求**其余每一维都是 1**，
 *               于是"batch=1 且单通道"不是这里的假设而是被证出来的结论：
 *               2 通道的 DB 头（有些变体输出 二值图+阈值图）会所有候选全不过 ⇒ 判不可用，
 *               这正是要的行为（把 (2,H) 当成图会认出一张高减半的板，而它不会报错）；
 *            2) **有向**的宽高比与送帧一致（±容差，见 CLASSROOM_OCR_ASPECT_TOLERANCE 那条）。
 *            元素数那道门分不出谁是宽谁是高（(W,H) 与 (H,W) 之积相同），
 *            所以这一道是 det 侧唯一的取向判据，写成对称形式（长边比短边）就等于没有判据。
 *            代价也要写明：送帧是正方形而概率图不是（各轴降采样倍率不同的模型）时，
 *            两个候选都被这一道挡掉 ⇒ 判不可用。这一档宁可降级——把 240×120 当成 120×240
 *            认出来的东西是"每一行都错位半张板"，读数上仍然有文字、分数也不低
 */
bool resolveDetMapShape(const CRawTensorShape_S &rstShape, int32_t nImgW, int32_t nImgH,
                        COcrDetShape_S &rstOut);

/**
 * @brief   : 把 rec 首路输出的自述形状折成 (时间步, 类别数) 并定下字典偏移
 * @param    {const CRawTensorShape_S&} rstShape：rec 首路原始 dims
 * @param    {int32_t} nCanvasW：rec 画布宽（句柄的模型输入宽，不是这里解出来的）
 * @param    {const COcrTokenTable&} rstToken：字典（只用来数条数）
 * @param    {COcrRecShape_S&} rstOut：OUT=只在返回 true 时被写
 * @return   {bool} false=这一路不可用（零个或多个候选自洽）
 * @note    : 元素数那道门在 rec 这边**没有**区分能力（T×C 与 C×T 之积相同，batch=1 时
 *            两个读法都过），所以决定性的判据是字典条数：类别数只能等于条数（字典含 blank）
 *            或条数 +1（blank 恒在类别 0）——由 ctcClassMap 判，别在这里再抄一遍这条规则。
 *            再加一道"步数 ≤ 画布宽"（一步至少对应一个像素列，这是 §30.12 那条"与送帧自洽"
 *            在 rec 侧的对应形式）。**它的判例面很窄，这里不夸它**：只有当字典条数恰好
 *            等于画布宽时，一对转置读法才会一个过、一个不过（例：字典 160 条、画布 160、
 *            输出 [1,160,161]），再往两边都只有一道字典门在判。用例按这一档打，
 *            打不红的判据不该留（同理，字典与 T 都落在 {N,N+1} 里的方形输出会被判两个候选
 *            自洽 ⇒ 不可用，那是**对的**：那种形状确实认不出谁是步数）
 */
bool resolveRecShape(const CRawTensorShape_S &rstShape, int32_t nCanvasW,
                     const COcrTokenTable &rstToken, COcrRecShape_S &rstOut);

/**
 * @brief   : 一次板面识别走到了哪一步（**只有 OK 一档是"成功"**，其余全是失败）
 * @note    : 分档的理由是这几种的现场处置完全不同：DET_FAIL 是"这一拍没有"（模型被换下），
 *            DET_SIZE / REC_SIZE / DB_REJECT 是"形状读错了"（要改码，重启也不会好），
 *            REC_FAIL 是"有框但一行都不出"（多半是裁剪档没建对）。
 *            合成一档的后果是这三种现象在心跳里长得一样
 */
typedef enum class OcrPipeStage : int
{
    OCR_PIPE_OK = 0,       /* 成功：rvecOut 非空=有板书，为空=真·空板（两者都要报出去） */
    OCR_PIPE_BAD_PARAM,    /* 入参非法（图指针为空、形状没解出来、字典没装载） */
    OCR_PIPE_DET_FAIL,     /* det 这一次推理没出结果 */
    OCR_PIPE_DET_SIZE,     /* det 输出元素数 != nMapW*nMapH：形状与真实输出不一致 */
    OCR_PIPE_DB_REJECT,    /* dbDecodeTextBoxes 判不了这张图（值域不在 0~1） */
    OCR_PIPE_REC_SIZE,     /* rec 输出元素数 != T*C：**当场废整帧**，理由见 ocrRunBoard 第 4 步 */
    OCR_PIPE_REC_FAIL,     /* 有行框，但没有一行解出来（逐行原因见计数） */
    OCR_PIPE_STAGE_NUM
} OcrPipeStage_E;

/**
 * @brief 编排层观测计数（**一个字符都不含**，PII 红线同 COcrDbStat_S）
 */
typedef struct _OcrPipeStat_
{
    uint64_t u64FrameOk = 0;        /* 走到 OK 的次数（含空板） */
    uint64_t u64BoxNum = 0;         /* 累计行框数（进逐行循环的） */
    uint64_t u64CropFail = 0;       /* 裁不出这一行（RGA 拒 / 缓冲借空） */
    uint64_t u64RecFail = 0;        /* 这一行的 rec 推理失败 */
    uint64_t u64RecSize = 0;        /* rec 输出元素数 != T*C（形状读错的现场证据） */
    uint64_t u64RowEmpty = 0;       /* 解出来是空串（整行都是 blank，**含补裁后仍空**） */
    uint64_t u64RowOut = 0;         /* 交出去的行数 */
    uint64_t u64BoxClip = 0;        /* 换算回送帧后塌掉（不足 2 像素）的框 */
    /* 有界补推理：空串补一次"上下扩 1/4 高"的重裁、rec 失败补一次
       同一框重跑；预算按帧给（2 次），整板噪声时不会把 rec 开销翻倍。
       retry 涨而 retryok 不涨 = 补了也没救，该回头查框/画布而不是继续加预算 */
    uint64_t u64RowRetry = 0;       /* 已发生的补推理次数（扩框 + 重跑合计） */
    uint64_t u64RowRetryOk = 0;     /* 其中最终出了字、并且该行被交出去的次数 */
} COcrPipeStat_S;

/**
 * @brief 一行 rec 画布的出口（实现方持有的缓冲，有效期到下次 cropRow 之前）
 * @note  这里用「指针 + 字节数」而不是 std::vector：画布缓冲的来源在实现方那侧是
 *        CRgaPool 借出的一块（§4.2 规则 4 的内存账要能追溯到池子），
 *        要它在编排层再落一次 std::vector 就等于每行多一次 38KB 拷贝 + 一块不在账上的内存
 */
typedef struct _OcrRowCanvas_
{
    const uint8_t *pData = nullptr;
    size_t nBytes = 0;
} COcrRowCanvas_S;

/**
 * @brief   : 编排层向设备侧要的三件事
 * @note    : 刻意不叫 IOcrBackend（那是 board_watcher.h 给引擎的接口，签名是"图进、文字出"）；
 *            这一层是它的实现内部对设备的依赖，两个方向的调用者不同、线程也不同。
 *            实现方（CPPOcrBackend）必须保证这三个函数**只在调用它们的同一线程上**有效，
 *            且都不持编排层的任何锁——编排层在它们之外不做阻塞等待
 */
class IOcrStagePort
{
public:
    IOcrStagePort() = default;
    virtual ~IOcrStagePort();

    IOcrStagePort(const IOcrStagePort &) = delete;
    IOcrStagePort &operator=(const IOcrStagePort &) = delete;

    /**
     * @brief   : 整张板面图 → det 概率图
     * @param    {std::vector<float>&} rvecMap：OUT=行主序 nMapW*nMapH 项，实现方 resize
     * @return   {bool} false=这一次推理没有结果（含模型未驻留）
     */
    virtual bool runDet(const uint8_t *pucRgb, int32_t nW, int32_t nH,
                        std::vector<float> &rvecMap) = 0;

    /**
     * @brief   : det 图坐标系里的一个行框 → rec 画布（nCanvasW×nCanvasH 连续 RGB888）
     * @param    {int32_t} nFilledW：等比缩放后真实写入的宽度（由本层按 recCanvasWidth 算好，
     *                               **不是**实现方自己算——算错它不报错，只会掉字）
     * @return   {bool} false=这一行裁不出来，实现方自己计数并归类
     */
    virtual bool cropRow(const uint8_t *pucSrc, int32_t nSrcW, int32_t nSrcH,
                         const COcrBox_S &stBox, int32_t nCanvasW, int32_t nCanvasH,
                         int32_t nFilledW, COcrRowCanvas_S &rstOut) = 0;

    /**
     * @brief   : 画布 → rec 原始输出（实现方 resize 成 T*C 项）
     */
    virtual bool runRec(const COcrRowCanvas_S &stCanvas, std::vector<float> &rvecOut) = 0;
};

/**
 * @brief   : 一张板面图 → 若干行文字（recognize 的正文，不含任何线程与设备动作）
 * @param    {const COcrDetShape_S&} rstDet：resolveDetMapShape 的结果
 * @param    {const COcrRecShape_S&} rstRec：resolveRecShape 的结果
 * @param    {int32_t} nCanvasW / nCanvasH：rec 画布尺寸（来自句柄的模型输入尺寸）
 * @param    {IOcrStagePort&} rstPort：设备侧三件事
 * @param    {const uint8_t*} pucRgb：det 输入图，连续 RGB888，宽高必须与解形状时送的那对一致
 * @param    {std::vector<CBoardText_S>&} rvecOut：OUT=函数内先 clear 再填，行框在**本图坐标系**
 * @param    {COcrPipeStat_S*} pstStat：可空，累加不覆盖
 * @param    {COcrDbStat_S*} pstDbStat：可空，**透传给 dbDecodeTextBoxes**（那张表的门是阈值调坏
 *             时的唯一现场证据；本层不重复计数，所以拿它当出参而不是把它并进上面的结构）
 * @return   {OcrPipeStage_E} OK 以外一律按失败处理；**空板是 OK + rvecOut 为空**
 * @note    : 五步，顺序不可换：
 *            1) runDet 拿概率图，并校验元素数 == nMapW*nMapH；
 *            2) dbDecodeTextBoxes 出图坐标系（概率图坐标系）的行框，返回值 -1 与 0 在这里
 *               是两种结局（DB_REJECT / 空板 OK），这一分岔是本函数存在的理由；
 *            3) 逐框 scaleBoxToImage 换算回送帧坐标系 + recCanvasWidth 定该行宽度；
 *            4) cropRow → runRec → ctcGreedyDecode，**一行失败不废整帧**（跳过并计数；
 *                起每一种"这一行没出字"都有一次有界补救：空串补上下扩框重裁、
 *                rec 失败补同一框重跑，合计每帧最多 2 次，见 COcrPipeStat_S.u64RowRetry），
 *               但任何一行撞上"输出元素数 != T*C"就当场废整帧：那一档说明形状读错了，
 *               剩下每一行的结果都是垃圾，继续跑只是白烧 NPU 并交出一份看起来正常的乱码；
 *            5) 有框却一行都没交出去 ⇒ REC_FAIL
 */
OcrPipeStage_E ocrRunBoard(const COcrDetShape_S &rstDet, const COcrRecShape_S &rstRec,
                           int32_t nCanvasW, int32_t nCanvasH,
                           const COcrDbParam_S &stDbParam, const COcrTokenTable &rstToken,
                           IOcrStagePort &rstPort,
                           const uint8_t *pucRgb, int32_t nImgW, int32_t nImgH,
                           std::vector<CBoardText_S> &rvecOut,
                           COcrPipeStat_S *pstStat = nullptr,
                           COcrDbStat_S *pstDbStat = nullptr);
}
