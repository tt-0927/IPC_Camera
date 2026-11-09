/**
 * @FilePath     : ocr_decode.h
 * @Author       : leiyy
 * @Date         : 2026-11-03 14:37:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 14:37:00
 * @Description  : PP-OCR 前后处理的纯函数层
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "classroom_define.h"

namespace Classroom
{
/* ---- 阈值 ----
   mask 0.6 / box 0.25 是这颗 .rknn 出厂带的值，配置可覆盖；unclip 2.0 是
   参照实现里写死的成员默认值（CPPOcrPoint.hpp:52） */
#define CLASSROOM_OCR_MASK_THRESHOLD (0.6f)
#define CLASSROOM_OCR_BOX_THRESHOLD  (0.25f)
#define CLASSROOM_OCR_UNCLIP_RATIO   (2.0f)

/* 一次板面检测最多交出去多少行。黑板上真写的字数在 10~30 行量级，
   超出这个数基本都是噪声（阈值调坏、板面反光成一整片），截断比"每行都识别一次"便宜 */
#define CLASSROOM_OCR_BOX_MAX (32)

/* 字典条数上限。当前 rec 模型（RV1126B_PPOcr_Rec48x320_V6.0.rknn）的字典是 18709 条
   ，留一倍余量取 32768。
   定死上限的理由与所有有界容器相同：这个数会被拿去乘 rec 的输出类别数判一致性 */
#define CLASSROOM_OCR_TOKEN_MAX (32768)

/* 字典的文件名 */
#define CLASSROOM_OCR_DICT_NAME ("token.txt")

/* 一个连通域的 BFS 上限 */
#define CLASSROOM_OCR_CCL_STACK_MAX (65536)

/* rec 画布的补边值 */
#define CLASSROOM_OCR_REC_PAD (127)

/**
 * @brief DB（Differentiable Binarization）检测后处理参数
 */
typedef struct _OcrDbParam_
{
    float   fMaskThreshold = CLASSROOM_OCR_MASK_THRESHOLD;  /* 概率图二值化门 */
    float   fBoxThreshold  = CLASSROOM_OCR_BOX_THRESHOLD;   /* 框内平均概率门 */
    float   fUnclipRatio   = CLASSROOM_OCR_UNCLIP_RATIO;    /* 行框外扩系数 */
    int32_t nBoxMax        = CLASSROOM_OCR_BOX_MAX;
    /* 同一行碎片的合并判据：竖直重叠比例与水平间隙倍数。
       中文板书的字与字之间本来就有空隙，二值化后一行常常碎成几个域，
       不合并的话一行变三四行、每行一个字，报表读起来像坏掉了。
       这两个数是上游 PaddleOCR 的 sorted_boxes 取值（0.7 / 1.5），不另起一套 */
    float   fMergeVOverlap = 0.7f;
    float   fMergeHGap     = 1.5f;
} COcrDbParam_S;

/**
 * @brief 一个文本行框（图坐标，[x1,x2) 半开，已钳在图内）
 */
typedef struct _OcrBox_
{
    int32_t nX1 = 0;
    int32_t nY1 = 0;
    int32_t nX2 = 0;
    int32_t nY2 = 0;
    float   fScore = 0.0f;      /* 框内平均概率，进 CBoardText_S.fScore */

    int32_t width() const  { return nX2 - nX1; }
    int32_t height() const { return nY2 - nY1; }
} COcrBox_S;

/**
 * @brief DB 解码观测（只计数，不含任何文字内容）
 * @note  这一族计数要进心跳，但**板书内容一个字都不能进**（PII 红线：板书中出现的
 *        学生姓名会随内容上行），所以这里全是整数
 */
typedef struct _OcrDbStat_
{
    uint64_t u64CompNum = 0;        /* 连通域总数（含被各道门挡掉的） */
    uint64_t u64DropSmall = 0;      /* 域太小（<3 像素），不是字 */
    uint64_t u64DropScore = 0;      /* 框内平均概率低于 box 门 */
    uint64_t u64DropGrow = 0;       /* 外扩后仍不足 4×4 */
    uint64_t u64Merged = 0;         /* 被并进上一行的碎片数 */
    uint64_t u64Truncated = 0;      /* 超过 nBoxMax 被截掉的行数 */
    uint64_t u64StackFull = 0;      /* BFS 栈触顶：这块区域不是文字，当场弃 */
} COcrDbStat_S;

/**
 * @brief   : OCR 字典（装机件名 `token.txt`，路径由 `ocrDictPathOf` 从 rec 那一行推）
 * @note    : 词条按「一整块字节 + 偏移表」存而不是 vector<string>：当前字典 18709 条，
 *            每条一个 std::string 的代价是 32B 的头部再加一次独立 malloc，
 *            加起来比词条本身还大一个量级。这里一次读入、一次分配、只查偏移
 */
class COcrTokenTable
{
public:
    /**
     * @brief   : 从文件装载（每行一个词条，行尾 \r 丢掉、行内空格**保留**）
     * @return   {bool} false 时 rstrErr 说明原因，表保持为空
     * @note    : 容忍 **UTF-8 BOM**：交付的 rec 配置带 BOM，而字典文件同样可能是
     *            在 Windows 上存的——第一个词条丢了不影响任何读数（它是 blank），
     *            但把"空格"当成一个词条会认出一个不存在的字符
     */
    bool loadFromFile(const std::string &strPath, std::string &rstrErr);

    /**
     * @brief   : 从文本装载（主机用例与「配置里直接给字典」的通道共用这一条实现）
     * @return   {bool} false=空表、一条有效词条都没有（全是空行也算，这种表 ready 但永远取不出字）、
     *                  条数超上限
     */
    bool loadFromText(const std::string &strText, std::string &rstrErr);

    void clear();

    uint32_t numTokens() const { return m_bReady ? static_cast<uint32_t>(m_vecOff.size() - 1) : 0u; }
    bool ready() const { return m_bReady; }

    /**
     * @brief   : 把第 nIndex 个词条追加到 rstrOut
     * @param    {int32_t} nLimitBytes：rstrOut 的字节上限（含本次），超出则**整条不追加**；
     *                                  **0 与负数都是「一个字节都装不下」**，不是「不设限」
     * @return   {bool} false=越界、空词条、或会超上限
     * @note    : 「超上限就整条放弃」而不是截断：半个 UTF-8 汉字是乱码，
     *            而一条完整但被丢掉的词条只是少一个字——前者会污染整份报表
     */
    bool appendToken(uint32_t nIndex, std::string &rstrOut, int32_t nLimitBytes) const;

private:
    std::string m_strBlob;          /* 全部词条首尾相接 */
    std::vector<uint32_t> m_vecOff; /* numTokens+1 项：第 i 条 = [off[i], off[i+1]) */
    bool m_bReady = false;
};

/**
 * @brief   : 从 **rec** 那颗模型的路径推出字典路径（装配期唯一的字典定位口径）
 * @param    {const std::string&} strRecModelPath：models[] 里 OcrRec 那一行的 model_path
 * @return   {std::string} "<该模型所在目录>/CLASSROOM_OCR_DICT_NAME"；
 *                        推不出目录（传入为空、或没有路径分隔符）时返回**空串**
 */
std::string ocrDictPathOf(const std::string &strRecModelPath);

/**
 * @brief   : DB 概率图 → 文本行框（本文件的主体）
 * @param    {const float*} pfProbMap：det 首路输出，nMapW×nMapH 行主序，取值 0~1
 * @param    {int32_t} nMapW / nMapH：概率图宽高（由模型输出 dims 给出，不假定等于输入尺寸）
 * @param    {std::vector<COcrBox_S>&} rvecBox：OUT=行框，函数内先 clear，按阅读序排列
 * @param    {COcrDbStat_S*} pstStat：OUT=可空，累加而非覆盖
 * @return   {int32_t} 框数；**0 表示「这块板面上没有可信文字」而不是失败**
 *             （空板是真实存在的现象，与"识别失败"必须分开，见 board_watcher.h 的 recognize 约定）；
 *             **-1 表示本函数没能判**（参数非法、申请工作缓冲失败、或输入根本不是一张
 *             取值在 0~1 的概率图），调用方必须按失败处理，不得当成空板
 * @note    : 六步，顺序不可换：
 *            1) 按 mask 门二值化；2) 8 邻域连通域（栈有界，触顶弃该域）；
 *            3) 每域算轴向外接框 + 域内平均概率，两道门各挡一种情况
 *            （小挡噪声、平均概率挡「框里有字但整片是背景」）；
 *            4) 外扩（相当于把参照实现里 Clipper 的等距外扩按矩形算：
 *               d = 面积×ratio / 周长，矩形即 w×h×ratio / (2(w+h))，四边各放 d）；
 *            5) 同行碎片合并；6) 阅读序排序后截到 nBoxMax。
 *            与参照实现的已知偏离是第 3、4 步用轴对齐框而不是旋转框：
 *            板面文字基本水平，代价是**斜着写的一行会得到偏大的框**（把邻行的像素圈进来），
 *            偏大的框只影响那一行的识别准确率，不影响它被记下来
 */
int32_t dbDecodeTextBoxes(const float *pfProbMap, int32_t nMapW, int32_t nMapH,
                          const COcrDbParam_S &stParam, std::vector<COcrBox_S> &rvecBox,
                          COcrDbStat_S *pstStat = nullptr);

/**
 * @brief   : 把概率图坐标的框换算到「送进 det 的那张图」的坐标
 * @note    : det 输入是黑板区域等比拉伸直出的 RGB888，所以这张图与概率图之间只有一个
 *            线性缩放（没有 letterbox 的 pad，也就没有要减的偏移）。
 *            单独一个函数是因为这一步错了不报错：缩放写反就是「每行都识别成邻行的像素」，
 *            而读数看起来一切正常（有文字、分数也不低）
 */
void scaleBoxToImage(const COcrBox_S &stMapBox, int32_t nMapW, int32_t nMapH,
                     int32_t nImgW, int32_t nImgH, COcrBox_S &rstImgBox);

/**
 * @brief   : 等比缩放到 rec 画布时该画多宽（4 对齐）
 * @return   {int32_t} [4, nCanvasW] 内的 4 倍数；框尺寸非法时返回 nCanvasW（整幅拉伸）
 * @note    : 与参照实现 preprocessImage 同一算术（先按高等比、宽超了才夹到画布宽），
 *            多出来的「向下取 4 倍数」是 RGA 的硬要求：dst_format 为 RGB888 时 dw 必须
 *            4 对齐（common_process.cpp:1038），不取整的结果是**这一行当场裁不出来**
 */
int32_t recCanvasWidth(int32_t nBoxW, int32_t nBoxH, int32_t nCanvasW, int32_t nCanvasH);

/**
 * @brief   : 把 RGA 直出的密集 nFilledW×nCanvasH 图在**同一块缓冲**里摊成 nCanvasW 步长并右补边
 * @param    {uint8_t*} pucCanvas：RGA 已写入前 nFilledW×nCanvasH×3 字节
 * @note    : 为什么不在 RGA 里一次做到：RGA 只认「目标图就是 dw×dh、步长等于 dw」，
 *            它没法把一行像素写到带空隙的位置上。所以要在这里逐行搬一次
 *            （自下而上，保证不踩到还没读的源行）。这是整条 OCR 路上唯一一次逐像素 CPU 搬运，
 *            量级是 画布高×画布宽×3 ≤ 48×320×3 = 46KB 每行，几十微秒
 */
void padRecCanvasRight(uint8_t *pucCanvas, int32_t nCanvasW, int32_t nCanvasH,
                       int32_t nFilledW, uint8_t ucPad);

/**
 * @brief   : 定「输出第 i 类 → 字典第几条」的偏移
 * @param    {uint32_t} u32ClassNum：rec 输出的类别维长度
 * @param    {const COcrTokenTable&} stToken：字典
 * @param    {uint32_t&} ru32Shift：OUT=偏移（0 或 1）
 * @return   {bool} false=两边对不上，本次识别结果不可信（调用方按失败处理，不要硬套）
 * @note    : 上游有两种出字典的习惯（字典含 blank 那条 / 不含而 blank 恒在类别 0），
 *            交付件属于哪一种**只能靠数出来**：条数相等即含，多一类即不含。
 *            猜错的后果是整体错一位——每一个字都变成隔壁那个字，
 *            而这是最难发现的一类"能跑、读起来像乱码"
 */
bool ctcClassMap(uint32_t u32ClassNum, const COcrTokenTable &stToken, uint32_t &ru32Shift);

/**
 * @brief   : CTC 贪心解码（每步 argmax → 折叠相邻重复 → 丢 blank → 拼字典）
 * @param    {const float*} pfOut：rec 输出，[nTimeStep][nClassNum] 行主序
 * @param    {int32_t} nTimeStep：时间步数（rec 画布宽的 1/4，但**以模型 dims 为准**）
 * @param    {uint32_t} u32ClassNum：类别数（含 blank）
 * @param    {uint32_t} u32Shift：ctcClassMap 给的字典偏移
 * @param    {int32_t} nMaxBytes：文本字节上限，超出即整行判不可信（返回 0 且串清空）
 * @return   {float} 0~1 的行置信度（被采纳步的 softmax 概率均值），nAccepted=0 时为 0
 * @param    {std::string&} rstrText：OUT=UTF-8 文本
 * @note    : argmax 与是否已 softmax 无关，所以模型尾部带不带 softmax 都不影响识别结果；
 *            但**置信度**要 softmax 才有意义，这里自己按步做一次（数值稳定：先减该步最大值）
 */
float ctcGreedyDecode(const float *pfOut, int32_t nTimeStep, uint32_t u32ClassNum,
                      uint32_t u32Shift, const COcrTokenTable &stToken,
                      int32_t nMaxBytes, std::string &rstrText);
}
