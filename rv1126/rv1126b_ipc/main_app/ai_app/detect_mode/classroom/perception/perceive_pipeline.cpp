/**
 * @FilePath     : perceive_pipeline.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-20 09:20:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-20 09:20:00
 * @Description  : 感知管线实现
 */

#include "perceive_pipeline.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>

#include "classroom_clock.h"
#include "classroom_osd.h"
#include "classroom_warm_policy.h"
#include "dlog.h"
#include "geometry.h"
#include "log_gate.h"

namespace Classroom
{
namespace
{
/* 人头是所有行为的主语，被挤出队列的代价是「整拍作废」，故排最高优先级 */
const int32_t nPRIO_HEAD = 0;
/* 姿态排最后：队列被挤时先挤它。理由是它**有跨帧缓存**——骨骼晚一拍下来，下游读到的
   还是上一拍那副（在新鲜度窗口内），判定不受影响；而人头晚一拍就是人数少一个，
   人数是所有指标的基数。同样一次的代价，一个只是延后，一个是出错 */
const int32_t nPRIO_POSE = 2;
/* 人脸排最后（P5）：考勤链上没有任何一条秒级判据——到课时长是按增量累加的，
   一次比对晚几拍只是把它推后，而这里晚一拍是"全教室的脸都没解出来"。
   另外这颗是 640×640 全帧推理，比人头/人体那两张重，让路给它们是同一句权衡 */
const int32_t nPRIO_FACE = 3;

/* 表情排在人脸之后（P5e）：它与取特征同批提交、同吃一张对齐图，被挤出队列的后果
   是这节课少一个采样点，而同一拍里的人头少一拍就是人数少一个。
   这一档不是"最低优先级随便挤"——它挤在 FaceFeature 之后，
   而 FaceFeature 已经挤在姿态之后，中间那两级都是有人的判据在等的 */
const int32_t nPRIO_EXPR = 4;

/* 管线里所有档位的最大值必须落在 submit 的合法区间内。这条断言存在的理由是越界既不编译失败、
   也不在启动时报错：只在那一路每次 submit 时被当场判成 bad request，
   日志里一条 warning，心跳里只表现为 try 涨而 fed 恒为 0（表情这一路踩过） */
constexpr int32_t nPRIO_MAX = std::max({nPRIO_HEAD, nPRIO_POSE, nPRIO_FACE,
                                        nPRIO_EXPR});
static_assert(nPRIO_MAX < CLASSROOM_INFER_PRIORITY_LEVEL,
              "priority out of range: bump CLASSROOM_INFER_PRIORITY_LEVEL in infer_scheduler.h too");
/* 第二条管的是**同档**：上面那条只挡越界，挡不住"新加一路感知直接写 6"。
   那种写法不报错也不越界，只是在队列满时与一次一分钟才来的板书 OCR 互相挤，
   被挤掉的这一拍人脸当场没有结果（第 8 刀把 LEVEL 抬到 7 就是为了给 OCR 留一格独占） */
static_assert(nPRIO_MAX < CLASSROOM_INFER_PRIORITY_BOARD_OCR,
              "perception must not share the board-OCR priority slot");

/* warp 输出池的块数。**不许退化成 1**：rknn 按「指针 + 尺寸」缓存输入（rga_pool.h 约束 2 的
   实测），同指针同尺寸时第二次输出与第一次逐位相同——现象是"每个人的特征都一样"，
   而比对侧看到的是恒 100% 命中，比报错危险得多。与裁剪档 CROP_BUF_NUM 同一口径，
   但不共用那个宏：它定义在引擎的 .cpp 里，为这一个数去把两个编译单元缝起来不值 */
const uint32_t nWARP_BUF_NUM = 2;

/* 主体标注：P1 一律记学生。教师身份要等 P4 的讲台区域与教师跟踪落地才有依据，
   在那之前把讲台上那个人记成学生会污染所有学生指标，但把整行留 UNKNOWN
   又会让 P2 的行为判定无行可跑（规则层按主体分流）——取前者，并在 P4 换掉这一行 */
const Subject_E enSUBJECT_DEFAULT = Subject_E::SUBJECT_STUDENT;
}

CPerceivePipeline::CPerceivePipeline(IPerceiveHost *pHost) : m_pHost(pHost)
{
    /* 运行期容器一次撑到上限，此后 run() 内不允许再有任何分配（§1.2 红线）。
       总量按 60 人计：4 张框/人员表 + 1 张下标表 + OSD 表 ≈ 14KB，
       换来的是「跑 7×24 之后堆上不会有碎片」这条可验证的性质 */
    m_vecHead.reserve(CLASSROOM_MAX_PERSONS);
    m_vecPersonBox.reserve(CLASSROOM_MAX_PERSONS);
    m_vecBoxOfDet.reserve(CLASSROOM_MAX_PERSONS);
    m_vecOsdBox.reserve(static_cast<size_t>(CLASSROOM_MAX_PERSONS) * 2);
    /* 姿态三张表同样在构造期定容。缓存表最贵（每行一份 26 点骨骼 ≈328B，满表 ≈20KB），
       但它是「轮不到的那几拍也有证据」的唯一载体，运行期不再增删 */
    m_vecPoseCache.resize(CLASSROOM_MAX_PERSONS);
    m_vecPoseCand.reserve(CLASSROOM_MAX_PERSONS);
    m_vecPoseSel.reserve(CLASSROOM_MAX_PERSONS);
    /* 人脸表同样构造期定容：每行 ≈64B，满表不到 4KB，换的是「解码到配行」这一段零分配 */
    m_vecFace.reserve(CLASSROOM_MAX_PERSONS);
    m_vecFaceRow.reserve(CLASSROOM_MAX_PERSONS);
    /* 特征表要 resize 而不是 reserve：faceFeatIndexOf 是按行下标扫的，空槽也得在表里
       （nTrackId<0 就是空槽这个约定要求行存在）。满表 ≈132KB，是这段链路里最大的一块常驻，
       记在 §1.2 的预算表上。三张临时表只 reserve——它们每拍 clear 后按实际检出数增长 */
    m_vecFeat.resize(CLASSROOM_MAX_PERSONS);
    m_vecFeatTrack.reserve(CLASSROOM_MAX_PERSONS);
    m_vecFeatCand.reserve(CLASSROOM_MAX_PERSONS);
    m_vecFeatSel.reserve(CLASSROOM_MAX_PERSONS);
    /* 复核表同样 resize（空槽必须在表里，nTrackId<0 才是「这一格没人」的口径），
       每行 24B、满表 ≈1.5KB——它是整条链上最便宜的一张跨帧表，
       却没有它就让 106 一条事件都攒不出来（理由见 phone_recheck.h 文件头） */
    m_vecPhoneCache.resize(CLASSROOM_MAX_PERSONS);
    m_vecPhoneCand.reserve(CLASSROOM_MAX_PERSONS);
    m_vecPhoneSel.reserve(CLASSROOM_MAX_PERSONS);
    m_vecPhoneBox.reserve(CLASSROOM_MAX_PERSONS);

    /* 等待槽在此处不建：槽与模型键一一对应，第一次用到才建，
       免得为「配了但没开」的模型白留 1.4KB×N 的框缓冲 */
}

bool CPerceivePipeline::run(const FramePtr &spPixel, const ConfigPtr &spConf, const FeatureFramePtr &spOut)
{
    if (!m_pHost || !spPixel || !spConf || !spOut)
    {
        return false;
    }

    syncConfig(spConf);

    const int64_t llNowMs = classroomNowMs();
    const uint64_t u64BeginUs = classroomNowUs();

    if (!spPixel->stGeom.isReady() || nullptr == spPixel->pucNv12)
    {
        /* 帧自带的几何没建起来 = 整条坐标链无定义。此时框画在哪都是错的，不猜 */
        return false;
    }

    /* ---------- 1) 人头检测：本拍的主体，跑不出来整拍作废 ----------
       （原优化2 的头/体交错随人体检测一起摘除：那个开关存在的唯一理由是
       让人头的 NPU 等待窗口去备体检测的输入，没有体检测就没有可交错的活） */
    ++m_stStat.u64HeadRounds;
    m_vecHead.clear();
    bool bHeadOk = false;
    if (inferWhole(CLASSROOM_MODEL_KEY_HEAD, *spPixel, nPRIO_HEAD,
                   DetectSource_E::DET_SRC_HEAD, m_stHeadParam, m_stDecodeStat, m_vecHead))
    {
        bHeadOk = true;
        ++m_stStat.u64HeadOk;
        m_stStat.nLastHeadDet = static_cast<int32_t>(m_vecHead.size());
    }

    if (!bHeadOk)
    {
        return false;
    }

    /* ---------- 2) 跟踪：每拍都跑，CPU 近 0 成本（§4.4）---------- */
    const std::vector<CTrackItem_S> &vecTrack = m_stTrk.update(m_vecHead, llNowMs);
    m_stStat.nLastTrackNum = static_cast<int32_t>(vecTrack.size());

    /* ---------- 3) 头身关联：体框全部由头框外扩推得----------
       人体检测模型已摘除（#96）：关联层固定收空体表，每个头都走 expandHeadToBody
       （下 7 头高、左右各 3 头宽、上 0.5，clamp 到画布），系数由 body_expand.* 配。
       空表让关联层统一按「无体框」走外扩，比传一个判空指针少一处分支 */
    const std::vector<CDetection_S> vecNoBody;
    assemblePersons(m_vecHead, vecNoBody, spPixel->stGeom, m_stAssembleParam,
                    m_vecPersonBox, m_stAssembleStat);

    /* 头框下标 → 人员行下标。跟踪器给回的是「本帧命中的头框下标」（nDetIdx），
       而头框与人员行不是一一对应（关联层会让体框优先服务面积大的头），
       少了这张中间表就只能拿坐标反查——平滑框反查会偶发错配，表现为「体框套在邻座身上」 */
    m_vecBoxOfDet.assign(m_vecHead.size(), -1);
    for (size_t nBox = 0; nBox < m_vecPersonBox.size(); ++nBox)
    {
        const int32_t nHead = m_vecPersonBox[nBox].nHeadIdx;
        if (nHead >= 0 && nHead < static_cast<int32_t>(m_vecBoxOfDet.size()))
        {
            m_vecBoxOfDet[nHead] = static_cast<int32_t>(nBox);
        }
    }

    /* ---------- 4) 写特征行：只写已确认轨迹 ----------
       未确认轨迹（命中数不足 min_hits）不进特征帧是刻意的：它们会在下一拍消失，
       让下游的人数在 N 与 N+1 之间抖，而人数是所有指标的基数（§4.4 的稳定性口径）。
       代价是新出现的人晚 1 拍被计入，5fps 下约 200ms。 */
    const int32_t nCanvasW = spPixel->stGeom.dstWidth();
    const int32_t nCanvasH = spPixel->stGeom.dstHeight();

    int32_t nWant = 0;
    for (size_t t = 0; t < vecTrack.size(); ++t)
    {
        if (vecTrack[t].bConfirmed)
        {
            ++nWant;
        }
    }
    if (nWant > spConf->nMaxPersons)
    {
        /* 配置侧上限（比编译期容量小）也要置截断标志：人数指标少一截时必须能分清
           是「屏上就这些人」还是「被限流了」，否则平台侧拿到的百分比无从解释 */
        nWant = spConf->nMaxPersons;
        spOut->bPersonTruncated = true;
    }

    int32_t nRowNum = nWant;
    CPersonFeature_S *pstRows = spOut->reservePersons(nRowNum);
    if (nullptr == pstRows)
    {
        /* 总线没建容量：这是装配期缺陷，不是运行期抖动 */
        dlog_error("classroom perceive: 人体行表满，写不进");
        return false;
    }

    int32_t nWritten = 0;
    for (size_t t = 0; t < vecTrack.size() && nWritten < nRowNum; ++t)
    {
        const CTrackItem_S &stTrk = vecTrack[t];
        if (!stTrk.bConfirmed)
        {
            continue;
        }

        CPersonFeature_S &stRow = pstRows[nWritten];
        stRow.nPersonId = stTrk.nTrackId;
        stRow.enSubject = enSUBJECT_DEFAULT;
        stRow.nTrackAgeFrames = stTrk.nHits;
        stRow.stHeadBox = stTrk.stBox;
        stRow.fHeadScore = stTrk.fScore;
        stRow.bOnBoundary = touchesEdgeOf(stTrk.stBox, nCanvasW, nCanvasH, spConf->nEdgeBandPx);

        const int32_t nDet = stTrk.nDetIdx;
        const int32_t nBox = (nDet >= 0 && nDet < static_cast<int32_t>(m_vecBoxOfDet.size()))
                                 ? m_vecBoxOfDet[nDet] : -1;
        if (nBox >= 0)
        {
            stRow.stBodyBox = m_vecPersonBox[nBox].stBody;
        }
        else
        {
            /* 走到这里的两种情况都要外扩兜底：
               ①本拍未命中检测（丢失期靠轨迹外推续命，最长 keep_ms）——没有同帧头框可关联；
               ②关联层没为这个头出行——理论上不该发生，发生了也不该让这一行没有体框 */
            stRow.stBodyBox = expandHeadToBody(spPixel->stGeom, stTrk.stBox,
                                               m_stAssembleParam.fWidthHead,
                                               m_stAssembleParam.fDownHead,
                                               m_stAssembleParam.fUpHead);
        }

        stRow.fBlurScore = blurScoreOf(*spPixel, stRow.stHeadBox);
        ++nWritten;
    }

    /* 遮挡判定要在行数定下之后做：它是行与行之间的关系，不是单行属性。
       两两比一遍（60 人 ≈ 1800 次 IoU，纯整数比较级别），不做空间索引——
       为 60 个框上一棵 R 树，代价全在维护上 */
    for (int32_t i = 0; i < nWritten; ++i)
    {
        for (int32_t j = i + 1; j < nWritten; ++j)
        {
            if (iouOf(pstRows[i].stHeadBox, pstRows[j].stHeadBox) > spConf->fOccludeIou)
            {
                pstRows[i].bOccluded = true;
                pstRows[j].bOccluded = true;
            }
        }
    }

    /* ---------- 4.9) 人脸检测提前提交（优化3，bFaceEarlySubmit 开时）----------
       人脸检测只依赖原始帧，不依赖姿态：先把它投进 NPU 队列，
       NPU 跑人脸的这几十毫秒里 CPU 正去做姿态的挑选与裁剪。
       等待+解码+配行仍在 5b（姿态段之后）——配行只吃 trackId，与拍内位次无关；
       提交失败（在途拒绝/模型没驻留）不 book，下一拍重试（与原段同一句话） */
    if (m_stOpt.bSubmitAsyncEnable && m_stOpt.bFaceEarlySubmit)
    {
        (void)runFaceDetectOnly(*spPixel, nWritten, llNowMs);
    }

    /* ---------- 5) 姿态轮询：按人限流的一段，跑不动就整段跳过 ----------
       排在遮挡判定之后：挑选要用 bOccluded 与 fBlurScore 两道证据闸，
       而这两列一个在上一轮刚算出来、一个在写行时算出来。
       这一段的失败**不影响本拍产出**：没有骨骼只是这一行判不出姿态，
       而人头已经跑出来了，特征帧照发（下游按 SR_NO_POSE 记「没证据」，不是「没发生」）
       交错开（优化5）时由统一驱动接管本段与 5c：姿态与手机交替提交，
       NPU 队列里始终压着两路任务，CPU 的解码与裁剪不再串行空等。
       人脸段（5b）随后照跑——人脸配行只吃 trackId，不读姿态/手机列，
       姿态/手机也互不读人脸列，段序对调不换任何口径 */
    if (m_stOpt.bSubmitAsyncEnable && m_stOpt.bPosePhoneInterleave)
    {
        runPoseAndPhoneStage(*spPixel, spConf, pstRows, nWritten, llNowMs, *spOut);
    }
    else
    {
        runPoseStage(*spPixel, spConf, pstRows, nWritten, llNowMs, *spOut);
    }

    /* ---------- 5b) 人脸段（P5）：另一条节拍，跑不动就整段跳过 ----------
       排在姿态之后：两段都是「就地补同一批特征行的列」，前后顺序决定同一拍里
       谁看到的行号是最新的——写行的行号与轨迹表在此刻都已定型。
       与姿态段同理，这一段失败不影响本拍产出：没有人脸列只是这个人认不出
       提前提交开（优化3）时这里只做「等+解+配+取特征」：检测的 NPU 早在 §4.9 就投进队列，
       到此刻大概率已经算完，等一个零头而不是整段 60 ms */
    if (m_stOpt.bSubmitAsyncEnable && m_stOpt.bFaceEarlySubmit)
    {
        runFaceFeatAndAssign(*spPixel, spConf, pstRows, nWritten, llNowMs, vecTrack, m_vecBoxOfDet);
    }
    else
    {
        runFaceStage(*spPixel, spConf, pstRows, nWritten, llNowMs, vecTrack, m_vecBoxOfDet);
    }

    /* ---------- 5c) 玩手机复核（P6b）：排在最后一段 ----------
       两条理由使它必须落在所有段之后：它写的是特征行的最后一列，而挑人要用到已经定型的
       体框（裁剪输入）与遮挡/贴边标志（那两道否证闸）；它吃的名单来自**上一拍**的规则层，
       本拍的规则判定还没跑，放在前面也拿不到更新的名单。
       与前两段同理，这一段失败不影响本拍产出：没有复核只是这一行没有否决权 */
    if (!(m_stOpt.bSubmitAsyncEnable && m_stOpt.bPosePhoneInterleave))
    {
        /* 交错开时这一段已并入 6 的统一驱动（同一拍跑两遍会双计 tried 并互踩在途槽），
           这里只保留开关关闭时的原串行路径 */
        runPhoneStage(*spPixel, spConf, pstRows, nWritten, llNowMs);
    }

    /* ---------- 5d) 情绪列回填：每拍都跑，而不是只在人脸那一节拍跑 ----------
       表情约 16s 才轮到一个人，而总线槽位是复用的：不在每一拍把这一列过一遍，
       读者拿到的会是"上一节课那个人在这行的情绪"。这一段既不碰 NPU 也不碰像素，
       一次查找一行，代价与逐行的 llFaceMs 比较同量级（放在 6b 里则会让没跑人脸的那些拍
       把这一列留在旧值上，那正是要防的那种错）*/
    fillExpressionRows(pstRows, nWritten, llNowMs);

    /* 以实际写入行数收尾：reservePersons 按「想要的行数」占了位，
       真数只可能更小（确认轨迹的判据在写循环里），把 nPersonNum 交回去才不谎报 */
    spOut->nPersonNum = nWritten;
    spOut->nHeadDetectNum = static_cast<int32_t>(m_vecHead.size());
    m_stStat.u64Rows += static_cast<uint64_t>(nWritten);

    /* ---------- 6) OSD：P1 的验收手段，也是现场唯一的「它在跑」证据 ---------- */
    if (spConf->stEgress.bOsdEnable)
    {
        pushOsd(*spOut);
    }

    const int64_t llCostMs = static_cast<int64_t>((classroomNowUs() - u64BeginUs) / 1000);
    m_stStat.llLastRoundMs = llCostMs;
    spOut->llPerceptCostMs = llCostMs;
    if (llCostMs > m_nInferWaitMs * 2 && logOpen(m_stStat.u64HeadRounds))
    {
        dlog_warn("classroom perceive: 本轮耗时 %lldms 超预算 (wait=%dms), head=%d track=%d rows=%d",
                  static_cast<long long>(llCostMs), m_nInferWaitMs,
                  static_cast<int32_t>(m_vecHead.size()), m_stStat.nLastTrackNum, nWritten);
    }
    return true;
}

void CPerceivePipeline::syncConfig(const ConfigPtr &spConf)
{
    if (spConf->llVersion == m_llConfVersion)
    {
        return;
    }
    m_llConfVersion = spConf->llVersion;

    const CConfigSnapshot_S &stSnap = *spConf;

    /* 跟踪器的小框门与解码层强制同源：两处各判一套尺寸会出现「解码留下的小框被跟踪器丢掉」，
       框数对不上但没有任何一处报错。以配置里的 min_head_pixel 为准覆盖跟踪参数 */
    CTrackerParam_S stTrk = stSnap.stTracker;
    stTrk.nMinBoxEdge = stSnap.nMinHeadPixel;
    m_stTrk.setParam(stTrk);
    /* 刻意不 reset()：改一个门限不该让所有人的已持续时长归零（时长门在规则层，
       轨迹一清就要重新攒命中次数，现象是「下发一次配置，行为整体断几秒」）。
       几何类参数改了总线不重建，引擎已置 geom_stale 要求重启，那时管线一起重建 */

    const int32_t nCap = clampValue(stSnap.nMaxPersons, 1, CLASSROOM_MAX_PERSONS);

    m_stHeadParam.fScoreThreshold = stSnap.fDetectScoreThreshold;
    m_stHeadParam.nMinBoxEdge = stSnap.nMinHeadPixel;
    m_stHeadParam.nMaxBoxEdge = stSnap.nMaxHeadPixel;
    m_stHeadParam.nMaxBoxNum = nCap;
    /* 远带人头（#90）：分带与折扣只在人头表生效（decodeBoxes 按 enSource 取用），
       人体表连字段都不同步——留给它只会是"看起来可配"的假开关 */
    m_stHeadParam.nFarEdgePx = stSnap.nHeadFarEdgePx;
    m_stHeadParam.fFarGateRatio = stSnap.fHeadFarGateRatio;

    /* 外扩系数整份抄：body_expand.* 是「体框全部由头框外扩」（#96）之后的主几何旋钮。
       fContain 用宏而不用配置键——检测体框摘除后，关联层的 contain 门只在主机单测
       喂真体表时才走得到，为它留一个「看起来可配」的键只会误导现场 */
    m_stAssembleParam.fWidthHead = stSnap.fBodyExpandW;
    m_stAssembleParam.fDownHead = stSnap.fBodyExpandDown;
    m_stAssembleParam.fUpHead = stSnap.fBodyExpandUp;
    m_stAssembleParam.fContain = CLASSROOM_BODY_HEAD_CONTAIN;
    m_stAssembleParam.nMinBodyEdge = stSnap.nMinBodyPixel;

    m_nInferWaitMs = (stSnap.stSample.nInferWaitMs > 0) ? stSnap.stSample.nInferWaitMs
                                                        : CLASSROOM_INFER_WAIT_MS;
    /* 置信度门在检测两路共用一个配置项，但下发点已挪进 inferOne（那里只认模型键），
       所以这里单独留一份。取的就是上面 m_stBodyParam.fScoreThreshold 同一个值 */
    m_fDetectScoreThreshold = stSnap.fDetectScoreThreshold;
    m_fNmsThreshold = stSnap.fDetectNmsThreshold;
    m_fBlurFullGrad = stSnap.fBlurFullGrad;

    /* 骨骼的分数门与检测门**刻意不同源**：检测门低是为了不漏人（人数是基数），
       骨骼门高是为了不用别人的手腕判这只手举没举。把两者并成一个数，
       下场必然是「远处后排一堆低分点被当成有效证据」，而那正是几何判据最忌的抖动 */
    m_stPoseParam.fKeyScore = stSnap.fPoseScoreThreshold;
    m_stPoseParam.nMinTrunkPoints = CLASSROOM_POSE_MIN_TRUNK_POINTS;
    m_stPoseParam.nKeypointNum = CLASSROOM_POSE_KEYPOINT_NUM;
    m_nPoseBeatBudgetMs = stSnap.stSample.nPoseBeatBudgetMs;

    /* 人脸段：分数门与头/体共用一个 det_score（同一场景同一曝光），尺寸门独立配。
       刻意不拿 min_face_px（120）当这道门：那是账本命名链上的闸，闸在这里会让
       后排真人的脸连「到过场」都记不上。短边填 0 就是关掉这道闸（全部往下走）。
       min_face_px 只作为远近带的分界下发给解码（P2-6）：够到这条线的脸用基门，
       够不到的（=本就够不上命名）用基门×远带折扣——两条线在配置里已是同一场景的距离口径 */
    m_stFaceParam.fScoreThreshold = stSnap.fDetectScoreThreshold;
    m_stFaceParam.nMinBoxEdge = stSnap.nMinFaceDetectPixel;
    m_stFaceParam.nNearEdge = stSnap.nMinFacePixel;
    m_stFaceParam.nMaxBoxNum = nCap;
    /* 人脸段的节拍门与预热读同一个判据（faceStageWanted）：考勤关（含教师机被角色矩阵
       关掉）时 G_FACE 不驻留，管线若还按 face_interval 提交，就是"每拍一次、每次被
       『没有裁剪档』拒掉"的死循环——face try 冻结、「没有裁剪池」刷屏。置 0 即整段不提交 */
    m_nFaceIntervalMs = faceStageWanted(stSnap) ? stSnap.stSample.nFaceIntervalMs : 0;

    /* 对齐的三道闸门与模板点整份抄：配置层已经钳过、校验过，这里不再判一遍
       （两处钳制就是「配置下发了但行为不一致」的现成事故点） */
    m_stFaceAlignParam = stSnap.stAlign;
    m_nAlignPointOrder = stSnap.nAlignPointOrder;
    m_bAlignOrderProbe = stSnap.bAlignOrderProbe;
    m_nAlignCropPx = stSnap.nAlignCropPx;
    /* warp 输出档不在这里建：配置版本不变而模型后到位（首版快照 apply 时 FaceFeature
       还没登记）会让这一段永久缺池，而现象只是"考勤一直没名字"。见 ensureWarpPool */
    m_stFeatConf = stSnap.stFaceFeat;
    /* 第 13 页「人脸检测区域」：随快照整表抄（空 = 全画面）。拷贝而不存引用：
       快照会被下一版换掉，而这一段的循环跑在节拍线程里，跨拍持引用就是悬垂 */
    m_vecGalleryRegions = stSnap.stGallery.vecRegionIds;

    /* 玩手机复核：成本档整份抄，解码门限单独一份。置信度与头/体/脸同源（threshold.det_score）
       是刻意的——多一个门限键就多一处「改了配置没生效」的地方，而这一路的不对称口径
       （漏检=错杀真事件，误检=只是没否决）要求它跟着 det_score 一起动。
       nMaxBoxNum 给的是解码容量而不是「一个人最多几部手机」：手机框天然只有一个，
       给人数上限只是让解码层那道「超容量丢弃」的闸在这一路不可能误触发 */
    m_stPhoneConf = stSnap.stPhoneRecheck;
    m_stPhoneParam.fScoreThreshold = stSnap.fDetectScoreThreshold;
    m_stPhoneParam.nMinBoxEdge = stSnap.stPhoneRecheck.nMinBoxPx;
    m_stPhoneParam.nMaxBoxNum = nCap;

    /* 表情：判定门限整份交给平滑器（它才是消费方），本层只留开关与行 TTL。
       setParam 在类别数变化时自己清表，这里不额外 reset——但**关掉整段时必须清**：
       留着上一节课的 EMA 向量，等某天重新打开时第一拍就能给出曲线值，
       而那个值是关着期间根本没人喂出来的旧人的。清一次代价是几十个字节的浮点，
       而漏清的形态是"这节课的情绪曲线和昨天那节一模一样"，读数上一切正常 */
    m_stExprConf = stSnap.stExpr;
    m_stExpr.setParam(stSnap.stExpr.stParam);
    if (!m_stExprConf.bEnable)
    {
        m_stExpr.reset();
    }

    /* 灰度开关整份抄（COptimizeConf_S：六个默认全 false = 与旧串行管线行为一致）。
       生效判定在调用点做「主闸 && 分开关」，见 COptimizeConf_S 的注释 */
    m_stOpt = stSnap.stOpt;

    dlog_info("classroom perceive: 配置 v%lld 已生效 det=%.2f/%.2f pose_key=%.2f min_head=%d "
              "max_head=%d "
              "min_body=%d wait=%dms cap=%d expand=%.1f/%.1f/%.1f pose_budget=%dms "
              "face_min=%d face_interval=%dms align[order=%s crop=%d probe=%d resid=%.2f floor=%.1f eps=%d] "
              "feat=%d max=%d ttl=%dms phn=%d max=%d gap=%dms ttl=%dms box=%d "
              "expr=%d ttl=%dms slots=%d p=%.2f m=%.2f slotmap=%s map=%s opt=%d%d%d%d%d",
              static_cast<long long>(stSnap.llVersion), stSnap.fDetectScoreThreshold,
              stSnap.fDetectNmsThreshold, stSnap.fPoseScoreThreshold, stSnap.nMinHeadPixel,
              stSnap.nMaxHeadPixel,
              stSnap.nMinBodyPixel, m_nInferWaitMs, nCap, m_stAssembleParam.fWidthHead,
              m_stAssembleParam.fDownHead, m_stAssembleParam.fUpHead, m_nPoseBeatBudgetMs,
              m_stFaceParam.nMinBoxEdge, m_nFaceIntervalMs,
              alignPermName(m_nAlignPointOrder), m_nAlignCropPx, m_bAlignOrderProbe ? 1 : 0,
              m_stFaceAlignParam.fMaxResidualRatio, m_stFaceAlignParam.fNoiseFloorSrcPx,
              static_cast<int>(m_stFaceAlignParam.fMinEyeSpanPx),
              m_stFeatConf.bEnable ? 1 : 0, m_stFeatConf.nMaxPersons, m_stFeatConf.nTtlMs,
              m_stPhoneConf.bEnable ? 1 : 0, m_stPhoneConf.nMaxPersons, m_stPhoneConf.nMinIntervalMs,
              m_stPhoneConf.nTtlMs, m_stPhoneParam.nMinBoxEdge,
              m_stExprConf.bEnable ? 1 : 0, m_stExprConf.nTtlMs,
              m_stExpr.param().nSlotNum, m_stExpr.param().fProbMin, m_stExpr.param().fMarginMin,
              expressionSlotMapState(m_stExpr.param()),
              expressionMapText(m_stExpr.param()).c_str(),
              m_stOpt.bSubmitAsyncEnable ? 1 : 0,
              m_stOpt.bFaceEarlySubmit ? 1 : 0, m_stOpt.bFeatPerBeatEnable ? 1 : 0,
              m_stOpt.bPosePhoneInterleave ? 1 : 0, m_stOpt.bUrgencyPrecompute ? 1 : 0);
}

CPerceivePipeline::WaitSlotPtr CPerceivePipeline::slotOf(const std::string &strKey)
{
    /* 无锁：m_mapSlot 只在节拍线程被碰（等待槽内部的 mutex 服务的是回调线程，不是这张表） */
    auto it = m_mapSlot.find(strKey);
    if (it != m_mapSlot.end())
    {
        return it->second;
    }

    if (m_mapSlot.size() >= static_cast<size_t>(CLASSROOM_MAX_MODELS))
    {
        /* 超上限说明键表与配置对不上（配置校验也按这个上限拦），到这里只可能是外部注入的键 */
        dlog_error("classroom perceive: 槽位表满, 拒收模型 %s", strKey.c_str());
        return WaitSlotPtr();
    }

    WaitSlotPtr spSlot = std::make_shared<SWaitSlot_S>();
    spSlot->vecRaw.reserve(static_cast<size_t>(CLASSROOM_MAX_PERSONS) * CLASSROOM_BOX_FLOAT_NUM);
    m_mapSlot.emplace(strKey, spSlot);
    return spSlot;
}

bool CPerceivePipeline::beginRound(const std::string &strKey, WaitSlotPtr &rspSlot, uint32_t &ru32Gen)
{
    rspSlot.reset();
    ru32Gen = 0;

    if (nullptr == m_pHost->scheduler())
    {
        return false;
    }

    const WaitSlotPtr spSlot = slotOf(strKey);
    if (!spSlot)
    {
        return false;
    }

    const int32_t nWaitMs = m_nInferWaitMs;
    const uint32_t u32Gen = ++m_u32SubmitGen;
    const int64_t llWedgeMs = static_cast<int64_t>(nWaitMs) * CLASSROOM_INFER_WEDGE_TIMES;

    bool bSkipInflight = false;
    bool bWedgeCleared = false;
    int64_t llInflightMs = 0;
    {
        /* 锁内只改状态：日志与调度器/池子的调用一律放到出锁之后（本层纪律，见头文件） */
        std::lock_guard<std::mutex> stLock(spSlot->mtx);
        if (spSlot->bPending)
        {
            llInflightMs = classroomNowMs() - spSlot->llPendingSinceMs;
            if (llInflightMs < llWedgeMs)
            {
                bSkipInflight = true;
            }
            else
            {
                /* 看门狗：调度器承诺每个被接收的提交恰好回调一次。真丢了那一次回调，
                   这一路就会被 bPending 永久锁死，而「某一路永远没结果」是最难发现的失效形态。
                   宁可放行一次并把告警灯点亮。序号归 0 让那个永久迟到的回调自己作废 */
                bWedgeCleared = true;
                spSlot->bPending = false;
                spSlot->bDone = false;
                spSlot->u32PendingSeq = 0;
            }
        }

        if (!bSkipInflight)
        {
            spSlot->bDone = false;
            spSlot->bOk = false;
            spSlot->bPending = true;
            spSlot->u32PendingSeq = u32Gen;
            spSlot->llPendingSinceMs = classroomNowMs();
        }
    }

    if (bSkipInflight)
    {
        /* 上一拍还在途：本拍不提交。NPU 供不上时继续提交只会把队列堆满再丢弃，
           白烧一次 RGA 拷贝，而结果一律过期 */
        ++m_stStat.u64InferInflight;
        if (logOpen(m_stStat.u64InferInflight))
        {
            dlog_warn("classroom perceive: %s 在途 %lldms 未归，本轮跳过",
                      strKey.c_str(), static_cast<long long>(llInflightMs));
        }
        return false;
    }
    if (bWedgeCleared)
    {
        /* 这一行只写读数，不写结论。实测形状：回调会回来，只是迟到
           ——缺的是「迟到期间谁占着块/设备」这一格，所以这里配三个可证的量：
           实际在途时长、队列深度、本模型裁剪池的空闲块数（0 = 块全被在途请求占着）。 */
        ++m_stStat.u64WedgeCleared;
        const CropUnitPtr spCropWedge = m_pHost->cropUnitOf(strKey);
        const int nPoolIdle = spCropWedge ? spCropWedge->poolIdle() : -1;
        dlog_error("classroom perceive: %s 在途 %lldms 超上限 %lldms，强制释放；"
                   "队列=%u 裁剪空闲=%d",
                   strKey.c_str(), static_cast<long long>(llInflightMs),
                   static_cast<long long>(llWedgeMs),
                   nullptr != m_pHost->scheduler() ? m_pHost->scheduler()->pending() : 0u,
                   nPoolIdle);
    }

    rspSlot = spSlot;
    ru32Gen = u32Gen;
    return true;
}

void CPerceivePipeline::abortRound(const WaitSlotPtr &rspSlot)
{
    if (!rspSlot)
    {
        return;
    }
    std::lock_guard<std::mutex> stLock(rspSlot->mtx);
    rspSlot->bPending = false;
    rspSlot->u32PendingSeq = 0;
}

bool CPerceivePipeline::inferBusy(const std::string &strKey)
{
    /* 只查不建：见头文件注释。没有槽 = 这一路从没被用过 = 不在途 */
    const std::map<std::string, WaitSlotPtr>::const_iterator cit = m_mapSlot.find(strKey);
    if (m_mapSlot.end() == cit || !cit->second)
    {
        return false;
    }
    /* 看门狗线以内才算"堵着"：越过那条线的在途是调度器丢了回调，
       把它读成"忙"会让调用方永远绕开这一路，而 beginRound 自己会放行它 */
    const int64_t llWedgeMs = static_cast<int64_t>(m_nInferWaitMs) * CLASSROOM_INFER_WEDGE_TIMES;
    std::lock_guard<std::mutex> stLock(cit->second->mtx);
    return (cit->second->bPending &&
            (classroomNowMs() - cit->second->llPendingSinceMs) < llWedgeMs);
}

bool CPerceivePipeline::submitAndWait(const std::string &strKey, const WaitSlotPtr &rspSlot,
                                      uint32_t u32Gen, int32_t nPriority,
                                      const RgaBufPtr &rspBuf,
                                      const std::vector<float> **ppvecRaw)
{
    /* 兼容壳：与拆分前逐行为一致。开关关闭的既有调用点都走这里，不必随开关改动 */
    if (!submitAsync(strKey, rspSlot, u32Gen, nPriority, rspBuf))
    {
        return false;
    }
    return waitAsyncResult(strKey, rspSlot, u32Gen, m_nInferWaitMs, ppvecRaw);
}

bool CPerceivePipeline::submitAsync(const std::string &strKey, const WaitSlotPtr &rspSlot,
                                    uint32_t u32Gen, int32_t nPriority, const RgaBufPtr &rspBuf)
{
    if (!rspSlot || !rspBuf || rspBuf->empty() || 0 == u32Gen)
    {
        /* 空缓冲绝不能提交：后端会按 0 字节比期望值，报回来的是「尺寸不符」，
           而真实原因是调用方没备料——在这里挡住并归到 crop 那一档，读数才对得上 */
        abortRound(rspSlot);
        ++m_stStat.u64CropFail;
        return false;
    }

    CInferScheduler *pSched = m_pHost->scheduler();
    if (nullptr == pSched)
    {
        abortRound(rspSlot);
        return false;
    }

    const int32_t nWaitMs = m_nInferWaitMs;

    /* 门限下发：peek 只碰已驻留的句柄，不加载、不计租约。
       用 acquire() 会当场把加载压在节拍线程上，
       并且 nUsers+1 会永久挡住重资源组的时间片轮转。
       骨骼/分类这类后端不吃这两个值（IModelHandle::setThresholds 默认空实现），
       这里不做例外：少一处「这一路要不要下发」的判断，就少一处会漏改的地方 */
    const float fConf = m_fDetectScoreThreshold;
    const float fNms = m_fNmsThreshold;
    m_pHost->pool().peek(strKey, [fConf, fNms](IModelHandle &stHandle)
    {
        stHandle.setThresholds(fConf, fNms);
    });

    CInferReq_S stReq;
    stReq.strModelKey = strKey;
    stReq.spInput = rspBuf;           /* 引用计数持有：缓冲活到推理结束，随任务析构归还 RGA 池 */
    stReq.u32InBytes = static_cast<uint32_t>(rspBuf->size());
    stReq.u32Seq = u32Gen;
    stReq.nPriority = nPriority;
    stReq.llSubmitMs = classroomNowMs();
    /* 过期线必须比等待长：调用方不等了之后任务仍要跑完才能清等待槽，
       若 deadline 先到，worker 会立刻丢掉它并回调，「慢一拍」就变成「每拍白提交一次」的死循环 */
    stReq.llDeadlineMs = stReq.llSubmitMs + static_cast<int64_t>(nWaitMs) * CLASSROOM_INFER_DEADLINE_TIMES;
    stReq.fnDone = [rspSlot](const CInferResult_S &stRes)
    {
        std::lock_guard<std::mutex> stLock(rspSlot->mtx);
        if (stRes.u32Seq != rspSlot->u32PendingSeq)
        {
            /* 看门狗放行之后迟到的旧回调：整份结果丢弃，也不动新一拍的在途标志。
               少了这道判据，旧结果会被当成新结果解码一次——现象是「框偶尔来自上一帧」 */
            return;
        }
        rspSlot->bOk = (stRes.bOk && nullptr != stRes.pvfOutput);
        rspSlot->enReason = stRes.enReason;
        rspSlot->llCostMs = stRes.llCostMs;
        if (rspSlot->bOk)
        {
            rspSlot->vecRaw = *stRes.pvfOutput;    /* 必须复制：指针指向 worker 的复用缓冲 */
        }
        else
        {
            rspSlot->vecRaw.clear();
        }
        rspSlot->u32DoneSeq = stRes.u32Seq;
        rspSlot->bPending = false;
        rspSlot->bDone = true;
        rspSlot->cv.notify_all();
    };

    if (!pSched->submit(std::move(stReq)))
    {
        abortRound(rspSlot);
        ++m_stStat.u64SubmitFail;
        if (logOpen(m_stStat.u64SubmitFail))
        {
            dlog_warn("classroom perceive: %s 提交被拒，队列=%u 待收",
                      strKey.c_str(), pSched->pending());
        }
        return false;
    }
    return true;
}

bool CPerceivePipeline::waitAsyncResult(const std::string &strKey, const WaitSlotPtr &rspSlot,
                                        uint32_t u32Gen, int32_t nTimeoutMs,
                                        const std::vector<float> **ppvecRaw,
                                        InferResultReason_E *penReason)
{
    if (nullptr != ppvecRaw)
    {
        *ppvecRaw = nullptr;
    }
    if (nullptr != penReason)
    {
        *penReason = InferResultReason::IR_BACKEND_FAIL;
    }

    /* 只在提交成功后进入（成对纪律见头文件 submitAndWait 的 @warning），
       所以这里的调度器必然已在——不再重复判空 */
    CInferScheduler *pSched = m_pHost->scheduler();

    /* 优化6 挂点：等结果的这段时间在旧形态里是节拍线程的纯空转。开关打开时把宿主的
       预计算压进来；引擎侧核实结论 = 无可安全外提的计算（见 CClassroomEngine::
       precomputeUrgency 注释），现落为空体，正式计算留在 ruleStep。默认关=零开销 */
    if (m_stOpt.bUrgencyPrecompute)
    {
        m_pHost->precomputeUrgency(classroomNowMs());
    }

    bool bOk = false;
    bool bTimedOut = false;
    InferResultReason_E enReason = InferResultReason::IR_BACKEND_FAIL;
    {
        std::unique_lock<std::mutex> stLock(rspSlot->mtx);
        const bool bWaked = rspSlot->cv.wait_for(
            stLock, std::chrono::milliseconds(nTimeoutMs),
            [rspSlot, u32Gen] { return (rspSlot->bDone && rspSlot->u32DoneSeq == u32Gen); });
        if (bWaked)
        {
            rspSlot->bDone = false;
            bOk = rspSlot->bOk;
            enReason = rspSlot->enReason;
        }
        else
        {
            bTimedOut = true;
            /* 超时后**不清 bPending**：让那个迟到的回调自己清。
               于是「超时」自然演变成「这一路暂时停一拍」，而不是「再来一次」把 NPU 埋了 */
        }
    }
    if (bTimedOut)
    {
        ++m_stStat.u64InferTimeout;
        if (logOpen(m_stStat.u64InferTimeout))
        {
            dlog_warn("classroom perceive: %s %dms 没等到结果，NPU忙=%.1f%% 队列=%u",
                      strKey.c_str(), nTimeoutMs, pSched->npuBusyPercent(), pSched->pending());
        }
        return false;
    }

    if (nullptr != penReason)
    {
        *penReason = enReason;
    }

    if (!bOk)
    {
        switch (enReason)
        {
            case InferResultReason::IR_MODEL_UNREADY:  ++m_stStat.u64InferUnready;   break;
            case InferResultReason::IR_STALE_DROPPED:  ++m_stStat.u64InferStale;     break;
            case InferResultReason::IR_CANCELLED:      /* 停止路径：不计数，静默返回 */ break;
            default:                                   ++m_stStat.u64InferBackendFail; break;
        }
        /* 三个失败计数之和每次恰好 +1，拿它当闸门就是每 32 次一条，不会因为
           「这一类失败换了个原因」而重新逐拍打 */
        const uint64_t u64FailSum = m_stStat.u64InferUnready + m_stStat.u64InferStale +
                                    m_stStat.u64InferBackendFail;
        if (InferResultReason::IR_CANCELLED != enReason && logOpen(u64FailSum))
        {
            dlog_warn("classroom perceive: %s 推理失败 reason=%d, npu_busy=%.1f%% queue=%u",
                      strKey.c_str(), static_cast<int>(enReason), pSched->npuBusyPercent(),
                      pSched->pending());
        }
        return false;
    }

    if (nullptr != ppvecRaw)
    {
        *ppvecRaw = &rspSlot->vecRaw;
    }
    return true;
}

bool CPerceivePipeline::inferOne(const std::string &strKey, const CFrame_S &stFrame,
                                 int32_t nPriority, const CRectf_S *pstRectSrc, CDetMap &rstMap,
                                 const std::vector<float> **ppvecRaw, CRectf_S &rstUsedSrc)
{
    if (nullptr != ppvecRaw)
    {
        *ppvecRaw = nullptr;
    }
    rstUsedSrc = CRectf_S();

    /* 装配顺序与拆分前一致：备料+提交（含全部分档计数）→ 等+建映射。
       usedRect 在提交成功即可读、等待失败清掉——与拆分前「crop 成功即可读、
       submitAndWait 失败清掉」的口径相同 */
    SPendingTok_S stTok;
    if (!inferSubmitOne(strKey, stFrame, nPriority, pstRectSrc, stTok))
    {
        return false;
    }
    rstUsedSrc = stTok.stUsedSrc;

    const std::vector<float> *pvecRaw = nullptr;
    if (!inferWaitOne(strKey, stFrame, stTok, &pvecRaw, rstMap))
    {
        rstUsedSrc = CRectf_S();
        return false;
    }

    if (nullptr != ppvecRaw)
    {
        *ppvecRaw = pvecRaw;
    }
    /* 结果条数 0 同样返回 true：那是「这一屏没有目标」的有效证据，与「没跑出来」必须严格区分——
       前者可以让下游记 0，后者只能记 UNKNOWN */
    return true;
}

bool CPerceivePipeline::inferSubmitOne(const std::string &strKey, const CFrame_S &stFrame,
                                       int32_t nPriority, const CRectf_S *pstRectSrc,
                                       SPendingTok_S &rstTok)
{
    rstTok = SPendingTok_S();

    if (nullptr == m_pHost->scheduler())
    {
        return false;
    }

    /* 裁剪档随组驻留有、随组卸载无。取不到就是这一路现在跑不了（模型没到位 / 被时间片换下 /
       守护已卸组）——注意这**不是**「屏上没人」，调用方必须按证据不足处理 */
    const CropUnitPtr spCrop = m_pHost->cropUnitOf(strKey);
    if (!spCrop || !spCrop->isReady())
    {
        ++m_stStat.u64InferUnready;
        if (logOpen(m_stStat.u64InferUnready))
        {
            dlog_warn("classroom perceive: %s 没有裁剪池, pool state=%d",
                      strKey.c_str(), static_cast<int>(m_pHost->pool().stateOf(strKey)));
        }
        return false;
    }

    if (!beginRound(strKey, rstTok.spSlot, rstTok.u32Gen))
    {
        return false;
    }

    /* RGA 出输入缓冲：借不到就是像素侧的背压（该降采样，而不是加大队列） */
    RgaBufPtr spBuf;
    bool bCropOk = false;
    if (nullptr != pstRectSrc)
    {
        /* 区域档：真正用到的矩形由裁剪档回传（钳制与取偶都在它内部做），
           后面建映射必须以它为准——用调用方给的浮点框会让关键点整体偏移半个框 */
        bCropOk = spCrop->cropRoi(stFrame, *pstRectSrc, spBuf, &rstTok.stUsedSrc);
    }
    else
    {
        bCropOk = spCrop->cropWhole(stFrame, spBuf);
        if (bCropOk)
        {
            rstTok.stUsedSrc = CRectf_S{ 0.0f, 0.0f, static_cast<float>(stFrame.nSrcW),
                                         static_cast<float>(stFrame.nSrcH) };
        }
    }
    if (!bCropOk || !spBuf)
    {
        abortRound(rstTok.spSlot);
        ++m_stStat.u64CropFail;
        if (logOpen(m_stStat.u64CropFail))
        {
            const CCropStat_S stCropStat = spCrop->stat();
            /* badrect 与 small 必须分开读：前者是"下发的区域里有 NaN/退化框"（查平台），
               后者是"这一帧这个目标太小"（查相机与焦距）——合成一格就没法分工 */
            dlog_warn("classroom perceive: %s 裁剪失败，成=%llu 过小=%llu 坏框=%llu 无缓冲=%llu RGA失败=%llu",
                      strKey.c_str(), static_cast<unsigned long long>(stCropStat.u64Ok),
                      static_cast<unsigned long long>(stCropStat.u64RejectTooSmall),
                      static_cast<unsigned long long>(stCropStat.u64RejectBadRect),
                      static_cast<unsigned long long>(stCropStat.u64RejectNoBuf),
                      static_cast<unsigned long long>(stCropStat.u64RgaFail));
        }
        return false;
    }

    /* 档随凭据带走：等待侧建映射要用「裁剪那一拍」的档的尺寸，
       重取会吃进提交与等待之间档被换出重建的错位（见头文件 SPendingTok_S 的注） */
    rstTok.spCrop = spCrop;

    if (!submitAsync(strKey, rstTok.spSlot, rstTok.u32Gen, nPriority, spBuf))
    {
        rstTok = SPendingTok_S();   /* 槽已由 submitAsync 内部 abortRound 放回 */
        return false;
    }
    return true;
}

bool CPerceivePipeline::inferWaitOne(const std::string &strKey, const CFrame_S &stFrame,
                                     const SPendingTok_S &rstTok,
                                     const std::vector<float> **ppvecRaw, CDetMap &rstMap)
{
    if (nullptr != ppvecRaw)
    {
        *ppvecRaw = nullptr;
    }

    const std::vector<float> *pvecRaw = nullptr;
    if (!waitAsyncResult(strKey, rstTok.spSlot, rstTok.u32Gen, m_nInferWaitMs, &pvecRaw))
    {
        return false;
    }

    /* 映射：以**真正喂进去的那块区域**为准建（整帧档就是全帧，区域档是钳制取偶后的那块），
       输入尺寸取凭据里的裁剪档而不是模型描述表——建了映射就得与真正喂进去的那张图一致，
       描述表说 640 而档建在 416 时该以档为准 */
    std::string strErr;
    /* letterbox（#93-1）：像素侧（CCropUnit::cropWhole）与坐标侧（这里）调的是**同一个**
       letterboxPlan——两侧同源才不会"图加了 pad、映射还在按整幅算"的静默错位；
       档没开 flag 时计划恒 disable，与八参 setup 逐字节同形 */
    CLetterboxPlan_S stLb;
    if (rstTok.spCrop->letterbox())
    {
        stLb = letterboxPlan(static_cast<int>(rstTok.stUsedSrc.width()),
                             static_cast<int>(rstTok.stUsedSrc.height()),
                             static_cast<int>(rstTok.spCrop->inputWidth()),
                             static_cast<int>(rstTok.spCrop->inputHeight()));
    }
    if (!rstMap.setup(stFrame.stGeom,
                      static_cast<int>(rstTok.stUsedSrc.fX1), static_cast<int>(rstTok.stUsedSrc.fY1),
                      static_cast<int>(rstTok.stUsedSrc.width()), static_cast<int>(rstTok.stUsedSrc.height()),
                      static_cast<int>(rstTok.spCrop->inputWidth()),
                      static_cast<int>(rstTok.spCrop->inputHeight()),
                      stLb, strErr))
    {
        ++m_stStat.u64MapFail;
        if (logOpen(m_stStat.u64MapFail))
        {
            dlog_error("classroom perceive: %s 检测图建立失败: %s", strKey.c_str(), strErr.c_str());
        }
        return false;
    }

    if (nullptr != ppvecRaw)
    {
        *ppvecRaw = pvecRaw;
    }
    return true;
}

bool CPerceivePipeline::inferWhole(const std::string &strKey, const CFrame_S &stFrame, int32_t nPriority,
                                   DetectSource_E enSource, const CDetectParam_S &rstParam,
                                   CDetDecodeStat_S &rstDecodeStat, std::vector<CDetection_S> &rvecOut)
{
    rvecOut.clear();

    CDetMap stMap;
    const std::vector<float> *pvecRaw = nullptr;
    CRectf_S stUnused;
    if (!inferOne(strKey, stFrame, nPriority, nullptr, stMap, &pvecRaw, stUnused) || nullptr == pvecRaw)
    {
        return false;
    }

    decodeBoxes(*pvecRaw, stMap, rstParam, enSource, rvecOut, rstDecodeStat);
    return true;
}

int32_t CPerceivePipeline::poseWantNum(CInferScheduler *pSched, const ConfigPtr &spConf,
                                       int64_t llNowMs, int32_t nRowNum, int32_t &rnLeftInWindow)
{
    /* 判据本体在 pose_admit.cpp（P2-8 搬出）：perceive_pipeline 链 common_process.h
       进不了主机编译面，而三道闸（配额/忙闲迟滞/预算自适应）是要被单测逐格钉住的决策。
       本函数只做两件纯函数做不了的事：读调度器、记忙让路的账与日志 */
    rnLeftInWindow = 0;

    const CSampleConf_S &stSample = spConf->stSample;
    SPoseAdmitIn_S stIn;
    stIn.nRowNum = nRowNum;
    stIn.nBudgetMs = m_nPoseBeatBudgetMs;
    stIn.nWindowMs = stSample.nPoseIntervalMs;
    stIn.nMaxPersons = stSample.nMaxPosePersons;
    stIn.nBusyHighPct = stSample.nNpuBusyHighPercent;
    stIn.nBusyLowPct = stSample.nNpuBusyLowPercent;
    stIn.llNowMs = llNowMs;
    if (nullptr != pSched)
    {
        stIn.dBusyPct = pSched->npuBusyPercent();
        stIn.dCostMs = pSched->avgCostMs(CLASSROOM_MODEL_KEY_POSE);
    }

    SPoseAdmitOut_S stOut;
    poseAdmit(m_stPoseAdmit, stIn, stOut);
    rnLeftInWindow = stOut.nLeftInWindow;

    if (stOut.bBusySkip)
    {
        ++m_stStat.u64PoseBusySkip;
        if (logOpen(m_stStat.u64PoseBusySkip))
        {
            dlog_warn("classroom perceive: 姿态在 npu_busy=%.1f%% 暂停 (high=%d low=%d), "
                      "rows=%d pose_usable=%llu", stIn.dBusyPct, stIn.nBusyHighPct,
                      stIn.nBusyLowPct, nRowNum,
                      static_cast<unsigned long long>(m_stStat.u64PoseUsable));
        }
    }
    return stOut.nWant;
}

void CPerceivePipeline::runPoseStage(const CFrame_S &stFrame, const ConfigPtr &spConf,
                                     CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs,
                                     CFeatureFrame_S &stOut)
{
    const CSampleConf_S &stSample = spConf->stSample;

    /* 新鲜度窗口与规则层同一个函数、同一组入参：两侧对「这副骨骼还作不作数」必须同值，
       否则会出现感知层还在往帧里写、规则层已经判它过期的两种口径。
       两个节拍入参取（本函数自己一路喂出来的 EWMA），不取配置：
       nMaxPosePersons 是"希望一拍做几个"，本函数下面那三道闸任一收紧就做不到，
       拿希望值算窗口就是 #69-A 那条后排漏检的成因 */
    const uint64_t u64TriedBefore = m_stStat.u64PoseTried;
    int32_t nRoundMs = 0;
    int32_t nPerRound = 0;
    poseCadenceOf(m_stPoseCadence, nRoundMs, nPerRound);
    int32_t nTtlMs = poseEvidenceTtlMs(nRoundMs, nPerRound, nRowNum);

    /* 先把当前产能发布到帧上：下面三条提前返回的路径（模型没驻留 / 行表空）
       本拍一个样本都不会产生，帧上那两列仍是目前最好的估计。
       留 0 会让消费侧退回下限窗口，等于在时间片空窗里再把已有骨骼判死一遍 */
    stOut.nPoseRoundMs = nRoundMs;
    stOut.nPosePerRound = nPerRound;

    /* 先清过期项：表按人数上限定容，装不下的唯一原因是 ID 洪泛，
       而过期项不清就会让那些早走掉的人一直占着槽 */
    poseCachePrune(m_vecPoseCache, m_nPoseCacheInUse, llNowMs, nTtlMs);

    m_stStat.nLastPoseWant = 0;
    m_stStat.nLastPoseLeft = 0;

    /* 裁剪档不在 = 模型没驻留（没到位 / 被时间片换下 / 守护降级）。
       这时**一次尝试都没发生**，既不该盖时间戳也不该扣配额：配额会被空烧，
       时间戳会让这一屋子人在模型回来后的一整个周期里都排不上队 */
    const CropUnitPtr spCropPose = m_pHost->cropUnitOf(CLASSROOM_MODEL_KEY_POSE);
    if (!spCropPose || !spCropPose->isReady() || nullptr == patRows || nRowNum <= 0)
    {
        m_stStat.nLastPoseRows = applyPoseCache(patRows, nRowNum, m_vecPoseCache, llNowMs, nTtlMs);
        return;
    }

    CInferScheduler *pSched = m_pHost->scheduler();
    int32_t nLeft = 0;
    const int32_t nWant = poseWantNum(pSched, spConf, llNowMs, nRowNum, nLeft);
    m_stStat.nLastPoseWant = nWant;
    m_stStat.nLastPoseLeft = nLeft;

    if (nWant > 0)
    {
        CPoseSelectParam_S stSel;
        stSel.llNowMs = llNowMs;
        stSel.nIntervalMs = stSample.nPoseIntervalMs;
        stSel.nWantNum = nWant;
        stSel.bSkipOccluded = true;
        stSel.fBlurMin = spConf->fPoseBlurMin;
        /* 规则层的紧急度与实测窗口一起进挑选（P0-1）：排序从「最久没做」升级为
           「规则在等 → 临近过期 → 最久没做」。表是引擎在 perceiveStep 抄进本帧的
           上一拍结论，与行表同一帧同一线程，无跨线程问题；窗口用本函数刚量出的
           nTtlMs，与回填骨骼时的新鲜度判据同源 */
        stSel.pastUrgency = stOut.astPoseUrgency;
        stSel.nUrgencyNum = stOut.nPoseUrgencyNum;
        stSel.nPoseTtlMs = nTtlMs;
        selectPoseTargets(patRows, nRowNum, m_vecPoseCache, stSel, m_vecPoseCand, m_vecPoseSel);

        for (size_t nSel = 0; nSel < m_vecPoseSel.size(); ++nSel)
        {
            CPersonFeature_S &stRow = patRows[m_vecPoseSel[nSel]];

            /* 体框在分析帧，像素在源帧：先换算过去裁剪，回来再经 CDetMap 换算到分析帧。
               两次换算都过同一个 CFrameGeometry，所以镜像机位上点与框仍然对得上（§4.8） */
            const CRectf_S stBodySrc = stFrame.stGeom.dstToSrc(stRow.stBodyBox);

            const uint64_t u64InflyBefore = m_stStat.u64InferInflight;
            const uint64_t u64TmoBefore = m_stStat.u64InferTimeout;

            CPoseSkeleton_S stSkl;
            CDetMap stMap;
            const std::vector<float> *pvecRaw = nullptr;
            CRectf_S stUsed;
            bool bDecoded = false;
            if (inferOne(CLASSROOM_MODEL_KEY_POSE, stFrame, nPRIO_POSE, &stBodySrc, stMap,
                         &pvecRaw, stUsed) && nullptr != pvecRaw)
            {
                bDecoded = decodePose(*pvecRaw, stMap, m_stPoseParam, stSkl, m_stPoseStat);
            }

            /* 上一拍还在途 / 等到超时：说明 NPU 已经供不上，本拍后面的几个人一并放弃。
               这两种情况**不盖时间戳**——一次推理都没发生，记它等于让这个人白等一个周期。
               其余失败（裁剪太小、后端拒绝、映射建不起来）都是真的试过，必须盖：
               不盖的话顶名那个人每拍都排第一，全班其他人永远轮不到（饿死一整排）*/
            const bool bGiveUpRound = (m_stStat.u64InferInflight != u64InflyBefore) ||
                                      (m_stStat.u64InferTimeout != u64TmoBefore);
            if (bGiveUpRound)
            {
                break;
            }

            ++m_stStat.u64PoseTried;
            if (bDecoded && stSkl.bValid)
            {
                ++m_stStat.u64PoseUsable;
                if (1 == m_stStat.u64PoseUsable)
                {
                    dlog_info("classroom perceive: 首个骨架成功 id=%d pts_score=%.2f "
                              "crop=%dx%d from src(%.0f,%.0f)", stRow.nPersonId, stSkl.fScore,
                              static_cast<int>(stUsed.width()), static_cast<int>(stUsed.height()),
                              stUsed.fX1, stUsed.fY1);
                }
            }

            /* 无效结果也写表：那副骨骼的产出时刻是真实发生过的，
               applyPoseCache 会因 bValid=false 而不把它当证据，而挑选据此把人推后一个周期 */
            if (!poseCacheUpsert(m_vecPoseCache, m_nPoseCacheInUse, stRow.nPersonId, stSkl, llNowMs))
            {
                ++m_stStat.u64PoseCacheFull;
                if (logOpen(m_stStat.u64PoseCacheFull))
                {
                    dlog_warn("classroom perceive: 姿态缓存满（%d 槽, id 可能在泛滥）, "
                              "trk=%d rows=%d", static_cast<int32_t>(m_vecPoseCache.size()),
                              m_stStat.nLastTrackNum, nRowNum);
                }
            }
            /* 配额按「含失败尝试」记账：判据层（pose_admit）只读这本人次账，
               记账的义务在提交侧——这里就是那支笔 */
            ++m_stPoseAdmit.nInWindow;
        }
    }

    /* 本拍真的做了 nTried 个人 → 刷新实测产能，再按新产能重算窗口并重发帧上那两列。
       顺序不能反：帧上的两列必须与本函数回填时用的窗口同源，
       否则消费侧照帧重算出的 TTL 会与本拍刚用过的值差一拍（同一件事两个口径）*/
    poseCadenceFeed(m_stPoseCadence,
                    static_cast<int32_t>(m_stStat.u64PoseTried - u64TriedBefore), llNowMs);
    poseCadenceOf(m_stPoseCadence, nRoundMs, nPerRound);
    stOut.nPoseRoundMs = nRoundMs;
    stOut.nPosePerRound = nPerRound;
    nTtlMs = poseEvidenceTtlMs(nRoundMs, nPerRound, nRowNum);

    if (poseCapacityShort(nRoundMs, nPerRound, nRowNum))
    {
        /* 产能撑不起这一屋子人：窗口已经被上限钳住，排在圈尾的人回填时就是过期的。
           这条计数是「后排整节课零事件」在机上唯一的出口——没有它，
           「这一排真的没人举手」与「我们根本轮不到那一排」在日志里长得一模一样。
           注意它**不是**放宽窗口的理由：放宽由上限定死，这里只负责说出来 */
        ++m_stStat.u64PoseCapacityShort;
        if (logOpen(m_stStat.u64PoseCapacityShort))
        {
            dlog_warn("classroom perceive: 姿态产能跟不上教室人数, rows=%d "
                      "round=%dms per_round=%d cycle=%lldms ttl=%dms(cap %d) want=%d left=%d",
                      nRowNum, nRoundMs, nPerRound,
                      static_cast<long long>(poseCycleMs(nRoundMs, nPerRound, nRowNum)),
                      nTtlMs, CLASSROOM_POSE_TTL_MAX_MS, nWant, nLeft);
        }
    }

    /* 回填放在最后：本拍刚做的几个人和轮不到的老骨骼走同一条路进帧，
       规则层看到的每一行都是「这屏此刻最该有证据的那副骨骼」，不必知道它是哪一拍产的 */
    m_stStat.nLastPoseRows = applyPoseCache(patRows, nRowNum, m_vecPoseCache, llNowMs, nTtlMs);
}

void CPerceivePipeline::runFaceStage(const CFrame_S &stFrame, const ConfigPtr &spConf,
                                     CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs,
                                     const std::vector<CTrackItem_S> &vecTrack,
                                     const std::vector<int32_t> &vecRowOfHead)
{
    /* 关着（配 0）与没人是两种"不该提交"：前者是现场决定不做考勤，
       后者提交一次也只是拿回一张空表，白占一次 NPU 与一次同步等待 */
    if (m_nFaceIntervalMs <= 0 || nRowNum <= 0 || nullptr == patRows)
    {
        return;
    }
    if (!m_stFaceBeat.due(llNowMs, m_nFaceIntervalMs))
    {
        return;
    }
    ++m_stStat.u64FaceRounds;
    m_stStat.nLastExprAngleBad = 0;   /* 本拍值，与 nLastExprRow 同纪律：每拍重算 */

    CDetMap stMap;
    const std::vector<float> *pvecRaw = nullptr;
    CRectf_S stUsed;
    if (!inferOne(CLASSROOM_MODEL_KEY_FACE, stFrame, nPRIO_FACE, nullptr, stMap, &pvecRaw, stUsed))
    {
        /* 不 book：本段没跑成，下一拍就该重试。
           推进了就会变成一个固定 2 秒一次的稳定失败，读数与「屏上就是没人脸」完全一样。
           判定与记账分离后这句话是结构性保证（SBeatGate_S::due 只判不写），
           不再依赖注释自觉——P0-4 修的正是旧实现「判到期即记账」 */
        return;
    }
    m_stFaceBeat.book(llNowMs);
    ++m_stStat.u64FaceOk;

    m_vecFace.clear();
    decodeFaces(*pvecRaw, stMap, m_stFaceParam, m_vecFace, m_stFaceStat);
    m_stStat.nLastFaceDet = static_cast<int32_t>(m_vecFace.size());
    if (m_vecFace.empty())
    {
        /* 检出 0 张脸是**结果**而不是故障：本节拍的读数到此为止。
           但 nLastFaceRow 必须清 0——留着上一次的数会让人以为人脸列还在刷新 */
        m_stStat.nLastFaceRow = 0;
        return;
    }

    /* ---- 配行交给纯函数（detect_decode 里的 assignFaceRows，主机单测逐条钉住）----
       本层只做两件它做不了的事：写特征行、把失败计数落到心跳上 */
    int32_t nDropped = 0;    /* 被同拍更可信的那张脸挤掉的张数：不算失败，故只接不住不累加 */
    assignFaceRows(m_vecFace, patRows, nRowNum, vecTrack, vecRowOfHead, m_vecFaceRow, nDropped);

    /* 特征段要按跟踪 ID 记"这个人上次取特征是什么时候"，而 ID 只有配到行的脸才有。
       这张表就是配行结果的另一种形状：-1 的行在挑选时一律不参与（没 ID 就无从限频） */
    m_vecFeatTrack.resize(m_vecFace.size());

    /* 换序用的两块小表放栈上：定长五点、每拍复用，比给管线再加两个成员更说得清"只在本拍内有效" */
    CPointf_S astPerm[CLASSROOM_FACE_LANDMARK_NUM];
    float astPermScore[CLASSROOM_FACE_LANDMARK_NUM];
    /* 噪声地板门的尺度：画布解的输入空间是分析帧，点噪声是源帧常量，
       折算比 = 画布缩放（镜像/旋转不影响缩放，setup 时已计入 scaleX/Y）。
       x/y 两轴各取一半取均值：地板本身是标量近似，不必沿轴分解 */
    const float fCanvasPxPerSrcPx =
        0.5f * (stFrame.stGeom.scaleXOf() + stFrame.stGeom.scaleYOf());

    for (size_t f = 0; f < m_vecFace.size(); ++f)
    {
        const CFaceDet_S &stFace = m_vecFace[f];

        /* 探针吃**模型原始点序**（它内部逐表重排后比平均残差）。
           喂已排好的点等于给当前那张表额外一次偏好，六格读数就不平行了——
           而这个探针存在的唯一目的就是在这六格里挑一个 */
        if (m_bAlignOrderProbe)
        {
            (void)alignProbeFeed(stFace.astPoints, m_stFaceAlignParam.astTemplate,
                                 m_stFaceAlignParam, m_stAlignProbe);
        }

        /* 解算吃**置换后**的点：与 runFaceFeatStage 同一条口径。
           留原始序的话心跳里 align= 那一项说的是"没应用配置时的残差"，
           而现场要判断的恰恰是"配置里那张表对不对"，两个数一混就都失去意义。
           这里解出的矩阵本段不使用（没有像素可 warp），要的是那份按原因分项的计数：
           交付模型的点序与镜像口径是这条链上最靠后的未知项，而它的失败形态
           （残差全部超限）在框这一级完全看不出来 */
        if (applyAlignPerm(stFace.astPoints, stFace.afPointScore, astPerm, astPermScore,
                           m_nAlignPointOrder))
        {
            CFaceAlign_S stAlign;
            AlignFail_E enFail = AlignFail_E::AF_OK;
            (void)solveFaceAlign(astPerm, astPermScore, m_stFaceAlignParam.astTemplate,
                                 m_stFaceAlignParam, stAlign, &m_stFaceAlignStat, enFail,
                                 fCanvasPxPerSrcPx);
        }

        const int32_t nRow = m_vecFaceRow[f];
        if (nRow < 0)
        {
            ++m_stStat.u64FaceUnmatched;
            m_vecFeatTrack[f] = -1;
            continue;
        }
        if (nRow >= nRowNum)
        {
            m_vecFeatTrack[f] = -1;
            continue;
        }
        CPersonFeature_S &stRow = patRows[nRow];
        stRow.stFaceBox = stFace.stBox;
        stRow.fFaceScore = stFace.fScore;
        stRow.llFaceMs = llNowMs;
        m_vecFeatTrack[f] = stRow.nPersonId;
        /* enExpression 不在这里写：它的数据来源是取特征段顺路算出的平滑值，
           落行统一在每拍的 fillExpressionRows（那里没曲线时显式清成 UNKNOWN，不留上次的） */
    }

    int32_t nRow = 0;
    for (int32_t r = 0; r < nRowNum; ++r)
    {
        if (patRows[r].llFaceMs == llNowMs)
        {
            ++nRow;
        }
    }
    m_stStat.nLastFaceRow = nRow;

    /* 取特征紧跟在配行之后：它吃的是**本拍**的框与点，隔一拍就不成立了（理由见其注释）。
       放在最后是因为它要把 nLastFaceRow 先落定——那段里任何一步失败都不该回头改人脸列，
       两段的失败计数各记各的，排除法才用得起来 */
    runFaceFeatStage(stFrame, nRowNum, llNowMs);
}

bool CPerceivePipeline::runFaceDetectOnly(const CFrame_S &stFrame, int32_t nRowNum, int64_t llNowMs)
{
    m_bFacePending = false;

    /* 关着（配 0）与没人是两种"不该提交"（与 runFaceStage 同一句话）：
       前者是现场决定不做考勤，后者提交一次也只是拿回一张空表，白占一次 NPU */
    if (m_nFaceIntervalMs <= 0 || nRowNum <= 0)
    {
        return false;
    }
    if (!m_stFaceBeat.due(llNowMs, m_nFaceIntervalMs))
    {
        return false;
    }
    ++m_stStat.u64FaceRounds;

    /* 只提交不等：NPU 跑人脸的这几十毫秒里，CPU 去做姿态的挑选与裁剪（优化3 的全部意义）。
       提交失败（在途拒绝/模型没驻留）不 book：本段没跑成，下一拍就该重试。
       推进了就会变成一个固定 2 秒一次的稳定失败，读数与「屏上就是没人脸」完全一样 */
    if (!inferSubmitOne(CLASSROOM_MODEL_KEY_FACE, stFrame, nPRIO_FACE, nullptr, m_stFaceTok))
    {
        return false;
    }
    m_bFacePending = true;
    return true;
}

void CPerceivePipeline::runFaceFeatAndAssign(const CFrame_S &stFrame, const ConfigPtr &spConf,
                                             CPersonFeature_S *patRows, int32_t nRowNum,
                                             int64_t llNowMs,
                                             const std::vector<CTrackItem_S> &vecTrack,
                                             const std::vector<int32_t> &vecRowOfHead)
{
    (void)spConf;

    /* 本拍没提交（没到期 / 提交失败 / 段被关）= 与原段提前返回完全同形：
       不解码不配行不取特征，人脸列与 nLastFaceDet/Row 都留在上一拍。
       pending 在等待入口即清：成对纪律由「同一拍内必达」保证，凭据不跨拍存活 */
    if (!m_bFacePending)
    {
        return;
    }
    m_bFacePending = false;

    CDetMap stMap;
    const std::vector<float> *pvecRaw = nullptr;
    if (!inferWaitOne(CLASSROOM_MODEL_KEY_FACE, stFrame, m_stFaceTok, &pvecRaw, stMap))
    {
        /* 不 book（与原段同一句话）：本段没跑成，下一拍就该重试。
           推进了就会变成一个固定 2 秒一次的稳定失败，读数与「屏上就是没人脸」完全一样 */
        return;
    }
    m_stFaceBeat.book(llNowMs);
    ++m_stStat.u64FaceOk;

    m_vecFace.clear();
    decodeFaces(*pvecRaw, stMap, m_stFaceParam, m_vecFace, m_stFaceStat);
    m_stStat.nLastFaceDet = static_cast<int32_t>(m_vecFace.size());
    if (m_vecFace.empty())
    {
        /* 检出 0 张脸是**结果**而不是故障：本节拍的读数到此为止。
           但 nLastFaceRow 必须清 0——留着上一次的数会让人以为人脸列还在刷新 */
        m_stStat.nLastFaceRow = 0;
        return;
    }

    /* ---- 配行交给纯函数（detect_decode 里的 assignFaceRows，主机单测逐条钉住）----
       本层只做两件它做不了的事：写特征行、把失败计数落到心跳上 */
    int32_t nDropped = 0;    /* 被同拍更可信的那张脸挤掉的张数：不算失败，故只接不住不累加 */
    assignFaceRows(m_vecFace, patRows, nRowNum, vecTrack, vecRowOfHead, m_vecFaceRow, nDropped);

    /* 特征段要按跟踪 ID 记"这个人上次取特征是什么时候"，而 ID 只有配到行的脸才有。
       这张表就是配行结果的另一种形状：-1 的行在挑选时一律不参与（没 ID 就无从限频） */
    m_vecFeatTrack.resize(m_vecFace.size());

    /* 换序用的两块小表放栈上：定长五点、每拍复用，比给管线再加两个成员更说得清"只在本拍内有效" */
    CPointf_S astPerm[CLASSROOM_FACE_LANDMARK_NUM];
    float astPermScore[CLASSROOM_FACE_LANDMARK_NUM];
    /* 噪声地板门的尺度：画布解的输入空间是分析帧，点噪声是源帧常量，
       折算比 = 画布缩放（镜像/旋转不影响缩放，setup 时已计入 scaleX/Y）。
       x/y 两轴各取一半取均值：地板本身是标量近似，不必沿轴分解 */
    const float fCanvasPxPerSrcPx =
        0.5f * (stFrame.stGeom.scaleXOf() + stFrame.stGeom.scaleYOf());

    for (size_t f = 0; f < m_vecFace.size(); ++f)
    {
        const CFaceDet_S &stFace = m_vecFace[f];

        /* 探针吃**模型原始点序**（它内部逐表重排后比平均残差）。
           喂已排好的点等于给当前那张表额外一次偏好，六格读数就不平行了——
           而这个探针存在的唯一目的就是在这六格里挑一个 */
        if (m_bAlignOrderProbe)
        {
            (void)alignProbeFeed(stFace.astPoints, m_stFaceAlignParam.astTemplate,
                                 m_stFaceAlignParam, m_stAlignProbe);
        }

        /* 解算吃**置换后**的点：与 runFaceFeatStage 同一条口径。
           留原始序的话心跳里 align= 那一项说的是"没应用配置时的残差"，
           而现场要判断的恰恰是"配置里那张表对不对"，两个数一混就都失去意义。
           这里解出的矩阵本段不使用（没有像素可 warp），要的是那份按原因分项的计数：
           交付模型的点序与镜像口径是这条链上最靠后的未知项，而它的失败形态
           （残差全部超限）在框这一级完全看不出来 */
        if (applyAlignPerm(stFace.astPoints, stFace.afPointScore, astPerm, astPermScore,
                           m_nAlignPointOrder))
        {
            CFaceAlign_S stAlign;
            AlignFail_E enFail = AlignFail_E::AF_OK;
            (void)solveFaceAlign(astPerm, astPermScore, m_stFaceAlignParam.astTemplate,
                                 m_stFaceAlignParam, stAlign, &m_stFaceAlignStat, enFail,
                                 fCanvasPxPerSrcPx);
        }

        const int32_t nRow = m_vecFaceRow[f];
        if (nRow < 0)
        {
            ++m_stStat.u64FaceUnmatched;
            m_vecFeatTrack[f] = -1;
            continue;
        }
        if (nRow >= nRowNum)
        {
            m_vecFeatTrack[f] = -1;
            continue;
        }
        CPersonFeature_S &stRow = patRows[nRow];
        stRow.stFaceBox = stFace.stBox;
        stRow.fFaceScore = stFace.fScore;
        stRow.llFaceMs = llNowMs;
        m_vecFeatTrack[f] = stRow.nPersonId;
        /* enExpression 不在这里写：它的数据来源是取特征段顺路算出的平滑值，
           落行统一在每拍的 fillExpressionRows（那里没曲线时显式清成 UNKNOWN，不留上次的） */
    }

    int32_t nRow = 0;
    for (int32_t r = 0; r < nRowNum; ++r)
    {
        if (patRows[r].llFaceMs == llNowMs)
        {
            ++nRow;
        }
    }
    m_stStat.nLastFaceRow = nRow;

    /* 取特征紧跟在配行之后：它吃的是**本拍**的框与点，隔一拍就不成立了（理由见其注释）。
       放在最后是因为它要把 nLastFaceRow 先落定——那段里任何一步失败都不该回头改人脸列，
       两段的失败计数各记各的，排除法才用得起来 */
    runFaceFeatStage(stFrame, nRowNum, llNowMs);
}

bool CPerceivePipeline::takeFaceMatch(int32_t nTrackId, CFaceMatch_S &rstOut)
{
    return faceFeatTake(m_vecFeat, nTrackId, rstOut, m_stFeatStat);
}

bool CPerceivePipeline::ensureWarpPool()
{
    CModelDesc_S stDesc;
    if (!m_pHost->pool().descOf(CLASSROOM_MODEL_KEY_FACE_FEATURE, stDesc) ||
        0 == stDesc.u32InputW || stDesc.u32InputW != stDesc.u32InputH)
    {
        /* 非方形的模型输入直接判"这段做不了"，而不是横竖各算一个缩放：对齐解的是
           4 自由度（单一标量），非方形意味着 warp 出来的脸在某个方向被拉长了，
           现象是"正着的脸认得出、侧着的认不出"——与 faceAlignCropRect 强制正方形同一条理由。
           交付的这颗是 112×112（设备实测），走到这里只可能是描述表读错了 */
        return false;
    }

    const int32_t nW = static_cast<int32_t>(stDesc.u32InputW);

    /* 表情那颗复用下面这块 warp 缓冲，尺寸是否对得上一律在这里判，且**在提前返回之前**判：
       走到"池已就绪、尺寸没变"那条早退时，FaceExpression 可能刚刚才登记（模型后到位、
       或配置换版把这一段打开），把 m_bExprSizeOk 留在上一轮的 false 就是"表情永远不做
       而考勤一切正常"——这一档没有第二个读数能说明它 */
    CModelDesc_S stExprDesc;
    const bool bExprOk = (nullptr != m_pHost) &&
                         m_pHost->pool().descOf(CLASSROOM_MODEL_KEY_EXPRESSION, stExprDesc) &&
                         stExprDesc.u32InputW == stDesc.u32InputW &&
                         stExprDesc.u32InputH == stDesc.u32InputH;
    if (bExprOk != m_bExprSizeOk)
    {
        dlog_info("classroom perceive: 表情输入 %ux%u 与 warp 缓冲 %ux%u 不符, 阶段 %s",
                  stExprDesc.u32InputW, stExprDesc.u32InputH, stDesc.u32InputW, stDesc.u32InputH,
                  bExprOk ? "on" : "off(size mismatch or model absent)");
    }
    m_bExprSizeOk = bExprOk;

    if (m_stWarpPool.isReady() && nW == m_nWarpW)
    {
        return true;
    }

    /* 按**模型的输入尺寸**建，不按 align.crop_px（160）：裁剪档是对齐的输入，
       对齐的输出才是模型的输入（112）。raw 后端按字节数比期望值、不符就拒（它不猜），
       所以这一档错了整段静默零产出——唯一可接受的错法是让它当场可见。
       只在尺寸真变了时重建：init 一轮是 2×37KB 的 new，每拍都来一次就把
       「运行期零分配」改成了「每拍两次大块分配」 */
    const uint32_t u32Bytes = stDesc.u32InputW * stDesc.u32InputH * 3u;
    m_stWarpPool.deinit();
    if (!m_stWarpPool.init(nWARP_BUF_NUM, u32Bytes))
    {
        /* 建不起来就整段不做（不是退化成把裁剪档直接喂进去）：退化的结果是
           "识别率暴跌但没有任何一处报错"，而这一段的目的恰恰是拿到可信的名字 */
        m_nWarpW = 0;
        m_nWarpH = 0;
        dlog_error("classroom perceive: 人脸特征 warp 池 %uB x%u 初始化失败, "
                   "人脸特征级关闭", u32Bytes, nWARP_BUF_NUM);
        return false;
    }
    m_nWarpW = nW;
    m_nWarpH = static_cast<int32_t>(stDesc.u32InputH);
    dlog_info("classroom perceive: 人脸特征 warp 缓冲 %dx%d x%u 就绪",
              m_nWarpW, m_nWarpH, nWARP_BUF_NUM);
    return true;
}

void CPerceivePipeline::runFaceFeatStage(const CFrame_S &stFrame, int32_t nRowNum, int64_t llNowMs)
{
    m_stStat.nLastFeatNum = 0;

    if (!m_stFeatConf.bEnable || m_vecFace.empty())
    {
        /* 关着 = 现场决定这一阵不做考勤；没脸 = 本拍无从挑选。
           两种都不该碰下面的任何计数，否则"关掉功能"会读成"功能坏了" */
        return;
    }

    /* 先清过期行再挑人：表容量就是人数上限，死人不清活人就进不来（一槽一人）。
       刻意放在三道前置检查**之前**：过期与"这一拍能不能真做"无关，
       模型掉线一分钟后回来发现整表还是老的，比清干净糟得多 */
    faceFeatPrune(m_vecFeat, llNowMs, m_stFeatConf.nTtlMs, m_stFeatStat);

    /* 下面三段前置检查都是"这一拍一次都没试"：既不盖尝试戳也不扣配额，
       盖了会让模型回来之后全班白等一个周期（与 runPoseStage 同一纪律） */
    if (!ensureWarpPool())
    {
        ++m_stStat.u64InferUnready;
        if (logOpen(m_stStat.u64InferUnready))
        {
            dlog_warn("classroom perceive: 人脸特征 warp 尺寸不可得, pool state=%d",
                      static_cast<int>(m_pHost->pool().stateOf(CLASSROOM_MODEL_KEY_FACE_FEATURE)));
        }
        return;
    }

    const CropUnitPtr spCrop = m_pHost->cropUnitOf(CLASSROOM_MODEL_KEY_FACE_FEATURE);
    if (!spCrop || !spCrop->isReady())
    {
        ++m_stStat.u64InferUnready;
        if (logOpen(m_stStat.u64InferUnready))
        {
            dlog_warn("classroom perceive: 人脸特征没有裁剪池, pool state=%d",
                      static_cast<int>(m_pHost->pool().stateOf(CLASSROOM_MODEL_KEY_FACE_FEATURE)));
        }
        return;
    }
    if (spCrop->inputWidth() != spCrop->inputHeight())
    {
        /* 裁剪档非见方 = 引擎那侧的 crop_px 覆盖没生效（接线错，不是现场配错）。
           以它为准换算五点会得出横竖两个尺度，正好是"单一标量缩放"这个前提的破法，
           所以整段放弃而不是将错就错——将错就错的样子是识别率减半且无人报错 */
        ++m_stStat.u64InferUnready;
        if (logOpen(m_stStat.u64InferUnready))
        {
            dlog_error("classroom perceive: 人脸特征裁剪池是 %ux%u（非正方形）, "
                       "跳过人脸特征级", spCrop->inputWidth(), spCrop->inputHeight());
        }
        return;
    }

    /* 裁剪边长以裁剪档自己为准而不是 align.crop_px：前者是真正会产出的像素数，
       后者只是意图。两者不等时按前者走（几何全链路都用它），并留一条 warn——
       "配了 160 实际裁 192"这件事除了这里没别的地方能看见 */
    const int32_t nCropPx = static_cast<int32_t>(spCrop->inputWidth());
    if (nCropPx != m_nAlignCropPx && logOpen(m_stStat.u64FaceRounds))
    {
        dlog_warn("classroom perceive: 人脸裁剪池是 %d px 而配置 align.crop_px=%d, "
                  "按裁剪池执行", nCropPx, m_nAlignCropPx);
    }

    CFaceFeatSelectParam_S stSel;
    stSel.llNowMs = llNowMs;
    /* 优化4（bFeatPerBeatEnable，独立开关不需主闸——它不改提交/等待协议，只改挑选个数）：
       每拍只取 per_beat_persons 张，把一轮 nMaxPersons 张的 NPU 峰值摊到多拍。
       挑几个 = nWantNum：挑选只返回前 nWantNum 个，没被选中的脸本拍根本不进循环，
       llTryMs 自然不推进（下一拍照样有机会被选到——face_feature.cpp 防饿死口径不变）；
       轮转一圈的时长随之自动重推：faceFeatIntervalMs 按同一把 nWantNum 算圈数
       （nMinIntervalMs 配了固定值时仍直通，不受本开关影响）。
       ★ #151 预热全速：节流档下，班里还有人没攒够两次尝试时按容量口径取特征——
       名字要两轮特征才出齐，只全速第一轮等于把轮转的账只算一半。判据与自限条件
       （表满不算、失败尝试也计数）都在 faceFeatWantNum 里，这里只收结果；
       退场后自动回到 per_beat_persons，不需要会话起止信号（感知拍与引擎会话不同频） */
    const int32_t nSteadyWant = m_stOpt.bFeatPerBeatEnable
                                    ? clampValue(m_stFeatConf.nPerBeatPersons, 1, CLASSROOM_MAX_PERSONS)
                                    : clampValue(m_stFeatConf.nMaxPersons, 1, CLASSROOM_MAX_PERSONS);
    stSel.nWantNum = faceFeatWantNum(m_stOpt.bFeatPerBeatEnable, m_stFeatConf.nPerBeatPersons,
                                     m_stFeatConf.nMaxPersons, m_vecFace, m_vecFeatTrack, m_vecFeat);
    if (stSel.nWantNum > nSteadyWant)
    {
        ++m_stStat.u64FeatWarmBeats;
    }
    /* 单人间隔的推导用**本拍检出的人脸数**而不是行数：挑选是在人脸表上轮的，
       一圈的长度由它决定；行数只影响"这些脸里有多少配得到人" */
    stSel.nMinIntervalMs = faceFeatIntervalMs(m_stFeatConf.nMinIntervalMs, m_nFaceIntervalMs,
                                              stSel.nWantNum, static_cast<int32_t>(m_vecFace.size()));
    (void)selectFaceFeatTargets(m_vecFace, m_vecFeatTrack, m_vecFeat, stSel,
                               m_vecFeatCand, m_vecFeatSel);
    if (m_vecFeatSel.empty())
    {
        return;
    }

    const int64_t llStageBeginMs = classroomNowMs();
    /* 外扩与钳制的边界是**画布**（分析帧）而不是源帧：人脸框本来就解在分析帧上，
       跨坐标系钳一次会让"外扩 2 倍"在镜像+旋转机位上变成另一个数 */
    const CRectf_S stCanvasLimit{ 0.0f, 0.0f, static_cast<float>(stFrame.stGeom.dstWidth()),
                                  static_cast<float>(stFrame.stGeom.dstHeight()) };

    CPointf_S astSrcPt[CLASSROOM_FACE_LANDMARK_NUM];
    CPointf_S astCropPt[CLASSROOM_FACE_LANDMARK_NUM];
    CPointf_S astPermPt[CLASSROOM_FACE_LANDMARK_NUM];
    float afPermScore[CLASSROOM_FACE_LANDMARK_NUM];
    const uint32_t u32WarpBytes = static_cast<uint32_t>(m_nWarpW) * static_cast<uint32_t>(m_nWarpH) * 3u;
    int32_t nDone = 0;

    for (size_t nSel = 0; nSel < m_vecFeatSel.size(); ++nSel)
    {
        const int32_t nFaceIdx = m_vecFeatSel[nSel];
        const CFaceDet_S &stFace = m_vecFace[static_cast<size_t>(nFaceIdx)];
        const int32_t nTrackId = m_vecFeatTrack[static_cast<size_t>(nFaceIdx)];

        /* 「人脸检测区域」：框不在区域内就整张跳过，**连槽都不占**。
           放在 beginRound 之前是有意的——区域是纯几何事实，不该为一张注定不做的脸
           付出一次排队与一次让路（让路会让后面的人白等一个周期）。
           区域几何与坐标换算在规则层（IPerceiveHost::headInRegion），这里只做判定 */
        if (!m_pHost->headInRegion(stFace.stBox, m_vecGalleryRegions))
        {
            ++m_stStat.u64FeatRegionSkip;
            continue;
        }

        /* 先占槽再花钱：本段一次尝试的代价是一次 RGA 搬运 + 一次 CPU 重采样，
           NPU 排不上时这些全白做（inferOne 把背压协议拆成三段的理由就在这）。
           beginRound 失败**不盖尝试戳**：一次推理都没发生，记它等于让这个人白等一个周期 */
        WaitSlotPtr spSlot;
        uint32_t u32Gen = 0;
        if (!beginRound(CLASSROOM_MODEL_KEY_FACE_FEATURE, spSlot, u32Gen))
        {
            ++m_stStat.u64FeatBusySkip;
            /* 让路而不是接着裁：这一路堵着，后面几张脸裁完也提交不了，
               白烧的却是节拍线程上的 CPU（重采样）而不只是 NPU */
            break;
        }
        ++m_stStat.u64FeatTried;

        /* ---- 备料：见方裁剪 → 五点落到裁剪图 → 解算 → 重采样成模型输入 ----
           写成 do/while(false) 只为给 abortRound 留一个出口：槽此刻已被占住，
           四步里任何一步提前失败都得还回去；散在四个分支里各写一遍必有一处漏还，
           漏还的形态是这一路被 bPending 锁死一整拍 */
        RgaBufPtr spWarp;
        CRectf_S stUsedSrc;    /* 提到块外：首次成功那条日志要报"特征是从源帧哪一块来的" */
        do
        {
            CRectf_S stRectCanvas;
            if (!faceAlignCropRect(stFace.stBox, CLASSROOM_ALIGN_CROP_EXPAND, stCanvasLimit, stRectCanvas))
            {
                ++m_stStat.u64FeatNoCrop;
                break;
            }

            /* 框在分析帧、像素在源帧：先 dstToSrc 再裁。裁剪档本身不施加镜像/旋转
               （rga_image_transform 的旋转角恒 0），所以裁出来的永远是未镜像的源像素，
               而点是镜像过的——这个错位由 align.point_order 表达，不在这里翻像素 */
            const CRectf_S stRectSrc = stFrame.stGeom.dstToSrc(stRectCanvas);
            RgaBufPtr spFaceRgb;
            if (!spCrop->cropRoi(stFrame, stRectSrc, spFaceRgb, &stUsedSrc) || !spFaceRgb)
            {
                ++m_stStat.u64CropFail;
                break;
            }

            for (int32_t i = 0; i < CLASSROOM_FACE_LANDMARK_NUM; ++i)
            {
                astSrcPt[i] = stFrame.stGeom.dstToSrc(stFace.astPoints[i]);
            }
            /* 用 stUsedSrc 而不是 stRectSrc：RGA 内部把 xy 与宽高向下取偶，
               按请求矩形换算会让五点整体偏移最多 1 px，小脸上这就是残差门的三分之一 */
            mapPointsToRect(astSrcPt, CLASSROOM_FACE_LANDMARK_NUM, stUsedSrc,
                            nCropPx, nCropPx, astCropPt);

            if (!applyAlignPerm(astCropPt, stFace.afPointScore, astPermPt, afPermScore,
                                m_nAlignPointOrder))
            {
                ++m_stStat.u64FeatNoPerm;
                break;
            }

            CFaceAlign_S stAlign;
            AlignFail_E enFail = AlignFail_E::AF_OK;
            /* 计数传 nullptr：同一张脸在 runFaceStage 里已按同一组参数解过一次并计过了，
               这里再计会让 align 段读数翻倍。而且两次解算的**坐标系不同**（画布 px vs
               裁剪 px），f_min_eye_span_px 在两者里根本是同一个数的两种含义——
               那一段要说的是"这套点能不能解出变换"，本段要说的是"这张脸能不能喂模型"。
               尺度传 crop px/源帧 px：裁剪档是按脸外扩取的，每张脸的放大率都不同，
               噪声地板门据此把源帧常量噪声折进本张脸的解算空间 */
            const float fCropPxPerSrcPx =
                static_cast<float>(nCropPx) / stUsedSrc.width();
            if (!solveFaceAlign(astPermPt, afPermScore, m_stFaceAlignParam.astTemplate,
                                m_stFaceAlignParam, stAlign, nullptr, enFail,
                                fCropPxPerSrcPx))
            {
                ++m_stStat.u64FeatNoAlign;
                /* 档距 10 不是 5：侧脸被对齐门拒绝是预期行为，聚合读数看心跳
                   align=/eyespan= 列；这里只保留低频样例供事后翻日志 */
                if (logOpen(m_stStat.u64FeatNoAlign, 10))
                {
                    dlog_warn("classroom perceive: face feat 对齐失败 reason=%s eye=%.1fpx "
                              "crop=%d trk=%d box=%.0fx%.0f", alignFailName(enFail),
                              eyeSpanPx(astPermPt), nCropPx, nTrackId,
                              stFace.stBox.width(), stFace.stBox.height());
                }
                break;
            }

            /* 借池里的块而不是本地 vector：它的 shared_ptr 语义正是调度器要的那种所有权移交
               （缓冲活到推理结束自动归还），而自己攒一份就要回答"什么时候能覆盖它"。
               2 块 + 环形游标同时满足"连续两次推理不得同指针"那条实测约束 */
            spWarp = m_stWarpPool.acquire(u32WarpBytes);
            if (!spWarp)
            {
                ++m_stStat.u64FeatNoPool;
                break;
            }
            /* 填充值与是否钳边走默认（黑边、不钳）而不是配置：warpFaceRgb 留了这两个参数，
               但"填黑边与拉涂抹带哪个更伤"要等真模型跑通对齐才有依据，现在配一个数出来
               只会多一处没人知道该填几的旋钮（需求 11 要避免的那类配置）。
               统计传非空只为一件事：warp_pixel 那一项（越界填了多少像素）全项目只有这里产得出，
               而它是"出画半张脸还剩多少真像素"的读数。输出尺寸即建池尺寸，
               函数内部的 resize 因字节数相等是空操作，块地址不动（改池的建法要同时回来看这行） */
            if (!warpFaceRgb(spFaceRgb->data(), nCropPx, nCropPx, stAlign, m_nWarpW, m_nWarpH,
                             *spWarp, 0, false, &m_stFaceAlignStat))
            {
                ++m_stStat.u64FeatNoWarp;
                spWarp.reset();
                break;
            }
        } while (false);

        if (!spWarp)
        {
            abortRound(spSlot);
            /* 真试过但没走到推理：推轮转键。不推就会被"每张脸都裁不出正方形"的远焦机位
               钉死在同一批人身上——他们永远排队首，其余人永远轮不到（见 llTryMs） */
            (void)faceFeatStampAttempt(m_vecFeat, nTrackId, llNowMs, m_stFeatStat);
            continue;
        }

        const std::vector<float> *pvecRaw = nullptr;
        if (!submitAndWait(CLASSROOM_MODEL_KEY_FACE_FEATURE, spSlot, u32Gen, nPRIO_FACE,
                           spWarp, &pvecRaw))
        {
            (void)faceFeatStampAttempt(m_vecFeat, nTrackId, llNowMs, m_stFeatStat);
            /* 排队被拒 / 等到超时 / 后端说不做，三条都说明这一路此刻不通，本拍剩下几张脸一起让路。
               与姿态段"只认两种计数变动才让路"不同，是因为这里每一步都在同一颗模型上，
               判断依据就是现成的返回值，不必拿计数倒推 */
            break;
        }
        if (nullptr == pvecRaw || pvecRaw->empty())
        {
            /* 后端报了成功却不给数据：契约破了。归后端失败而不是本段的计数——
               这一档在心跳里与推理那一路同属"NPU 给了假的成功" */
            ++m_stStat.u64InferBackendFail;
            (void)faceFeatStampAttempt(m_vecFeat, nTrackId, llNowMs, m_stFeatStat);
            break;
        }

        /* 维度以真正拿到的数为准，不拿描述表的 u32OutElems 裁一刀：裁了就是"两处真值"，
           而模型输出多一个 bias 这种事只有在真机上看得到。写进表后与底库维度不符时
           由比对侧整行作废并计数（face_gallery 的维度口径），那条读数才是该看的地方 */
        if (faceFeatUpsert(m_vecFeat, nTrackId, pvecRaw->data(),
                           static_cast<int32_t>(pvecRaw->size()), llNowMs, m_stFeatStat))
        {
            ++m_stStat.u64FeatOk;
            ++nDone;
            if (1 == m_stStat.u64FeatOk)
            {
                dlog_info("classroom perceive: 首个人脸特征成功 dim=%d cost=%lldms trk=%d "
                          "crop=%d src=(%.0f,%.0f,%.0fx%.0f) rows=%d",
                          static_cast<int32_t>(pvecRaw->size()),
                          static_cast<long long>(classroomNowMs() - llStageBeginMs), nTrackId,
                          nCropPx, stUsedSrc.fX1, stUsedSrc.fY1,
                          stUsedSrc.width(), stUsedSrc.height(), nRowNum);
            }
        }
        else
        {
            /* 表满 / 维度超容量，原因已在 face_feature 的分项计数（try/wide/full）里。
               照样推轮转键：他这一拍确实占掉了一个配额，不推就是每拍重来一次 */
            (void)faceFeatStampAttempt(m_vecFeat, nTrackId, llNowMs, m_stFeatStat);
        }

        /* 表情排在取特征之后：两问共用这一张已经备好的对齐图（框架图 组合4=复用组合2），
           而这一路的成败与名单无关——上面 upsert 失败（表满）时这张脸照样是张好脸，
           情绪该照算；反过来表情排不上时名单也不该跟着少一个人。
           spWarp 此刻仍在自己手里（调度器那份随回调析构），缓冲内容就是刚喂过 FaceFeature
           的那张图：两颗不同的模型读同一块缓冲是两个不同的 rknn 上下文，
           与"同一颗模型连续两次读到同一指针"（那条实测约束）不是一回事。
           偏转角在**这里**估而不是在表情函数里：五点（已按模板槽排好）只在这一层存在，
           表情那一路拿到的只有一张 warp 图，从图反推角度要再解一次五点，重复且口径不一致 */
        const float fYawDeg = faceYawDeg(astPermPt, CLASSROOM_FACE_LANDMARK_NUM);
        runExpressionOfWarp(spWarp, nTrackId, llNowMs, fYawDeg);
    }

    /* 比对一次做完而不是每张脸拿到就扫库：一次全库遍历是数百行 × 512 维的乘加，
       攒在这里做既不占 NPU 也好估上界。更关键的是判据用 faceFeatNeedMatchNum，
       它与"本拍有没有新特征"无关——上一拍因底库空而留着没比的特征，这一拍名单下发了就该补上，
       所以这段必须在 nDone 为 0 时也照跑（写在循环里就会跟着 nDone 一起被跳过） */
    const int32_t nNeed = faceFeatNeedMatchNum(m_vecFeat);
    if (nNeed > 0)
    {
        IFaceGallery *pGallery = m_pHost->faceGallery();
        if (nullptr == pGallery || !pGallery->available())
        {
            m_stStat.u64FeatNoGallery += static_cast<uint64_t>(nNeed);
            if (logOpen(m_stStat.u64FeatNoGallery))
            {
                dlog_warn("classroom perceive: %d 个特征在等但花名册不可用, "
                          "live feat rows=%d", nNeed, static_cast<int32_t>(m_vecFeat.size()));
            }
        }
        else
        {
            for (size_t i = 0; i < m_vecFeat.size(); ++i)
            {
                if (!faceFeatNeedsMatch(m_vecFeat[i]))
                {
                    continue;
                }
                /* 先把键抄出来：match 之后要把结论写回同一行，
                   拿引用一路走到写侧的话，将来谁在 match 里加一次 prune 就写成悬垂引用 */
                const int32_t nRowId = m_vecFeat[i].nTrackId;
                const int32_t nRowDim = m_vecFeat[i].nDim;

                CFaceMatch_S stMatch;
                (void)pGallery->match(m_vecFeat[i].afFeat, nRowDim, llNowMs, stMatch);
                /* false 也要落表：那是"比过了，没上"。留成"没比过"会让这个人下一拍又被挑去比，
                   配额白烧，而账本要区分"没证据"与"有证据但不是本班人" */
                if (faceFeatSetMatch(m_vecFeat, nRowId, stMatch, llNowMs, m_stFeatStat) && stMatch.bHit)
                {
                    ++m_stStat.u64FeatHit;
                }
            }
        }
    }

    m_stStat.nLastFeatNum = nDone;
    m_stStat.llLastFeatCostMs = classroomNowMs() - llStageBeginMs;
}

void CPerceivePipeline::runExpressionOfWarp(const RgaBufPtr &rspBuf, int32_t nTrackId,
                                            int64_t llNowMs, float fYawDeg)
{
    if (!m_stExprConf.bEnable)
    {
        /* 关着 = 现场决定这一阵不量情绪。**不碰任何计数**：把"没做"记进 try 会让
           下面那行的 ok/try 比失去意义，而那个比是唯一能说明"门限收得太紧还是模型根本没跑"的读数 */
        return;
    }
    if (!m_bExprSizeOk)
    {
        /* 开着却做不了：这颗没登记、或与取特征那块 warp 档尺寸不符（在 ensureWarpPool 里判的，
           那里已经打过一条 change 日志）。按**被跳过的人次**记，与 u64ExprTried 同量纲——
           两个数一减就是"这一段本该做而一次没做"的人数，而这正是现场只看"expr 全 0"时
           分不清的那两种情况（没开 / 开坏了）的分界 */
        ++m_stStat.u64ExprSizeBad;
        return;
    }
    if (nTrackId < 0)
    {
        /* 没配到行就没有跟踪 ID，而平滑表按 ID 存。不记数：这不是失败，
           是这个人这一拍根本没进那条表（runFaceStage 里 u64FaceUnmatched 已经记过了） */
        return;
    }
    /* 角度门（「允许人脸角度」）：超限时连推理都不提交——侧脸在 112 模板上
       被拉成一条竖条纹，模型照样会给出一个很高的 top1，事后再用概率门挡是挡不住的。
       量不出角度（NaN，五点退化/瞳距 <1px）时**放行**并计一档：缺观测不否决，
       与全工程「没有证据不等于否定」同一口径；若这一档持续增长，说明这批脸太小、
       角度门事实上没生效，此时该看的是人脸检测的下限而不是这一格 */
    if (!std::isfinite(fYawDeg))
    {
        ++m_stStat.nLastExprAngleBad;
    }
    else if (std::fabs(fYawDeg) > m_stExprConf.stParam.fMaxAngleDeg)
    {
        ++m_stStat.u64ExprAngleSkip;
        if (logOpen(m_stStat.u64ExprAngleSkip, 32))
        {
            dlog_info("classroom expr: 人脸偏转 %.0f° 超限 %.0f°，跳过 trk=%d (累计 %llu)",
                      fYawDeg, m_stExprConf.stParam.fMaxAngleDeg, nTrackId,
                      static_cast<unsigned long long>(m_stStat.u64ExprAngleSkip));
        }
        return;
    }
    if (!rspBuf || rspBuf->empty())
    {
        /* 走到这里为空是编程缺陷（调用方只在 spWarp 有效时才调）。归 crop 那一档：
           与 submitAndWait 里对空缓冲的归档口径一致，两处同一本账 */
        ++m_stStat.u64CropFail;
        return;
    }

    WaitSlotPtr spSlot;
    uint32_t u32Gen = 0;
    if (!beginRound(CLASSROOM_MODEL_KEY_EXPRESSION, spSlot, u32Gen))
    {
        /* 只跳过这一个人，不 break 整段：表情排不上不该连带让考勤那一路少做几张脸，
           而取特征那一路此刻可能完全是空的（两颗模型两张等待槽，各堵各的） */
        ++m_stStat.u64ExprBusySkip;
        return;
    }
    ++m_stStat.u64ExprTried;

    const std::vector<float> *pvecRaw = nullptr;
    if (!submitAndWait(CLASSROOM_MODEL_KEY_EXPRESSION, spSlot, u32Gen, nPRIO_EXPR,
                       rspBuf, &pvecRaw))
    {
        /* 三条失败原因（未驻留 / 过期 / 后端拒）submitAndWait 已各自分档计过，
           这里不再另记一套：同一件事在两个地方计数，早晚会不一致 */
        return;
    }
    if (nullptr == pvecRaw || pvecRaw->empty())
    {
        ++m_stStat.u64InferBackendFail;
        return;
    }
    ++m_stStat.u64ExprFed;

    CExpressionSample_S stSample;
    (void)m_stExpr.feed(nTrackId, *pvecRaw, llNowMs, stSample);

    /* 原始概率的一行采样：槽位序没标定之前，这一行是把「哪个下标对应哪种情绪」
       问清楚的唯一办法——对着镜头让一个人持续笑，看 argmax 与这串数里哪一位抬起来。
       取"首帧 + 每 32 次"：首帧证明这一路通了（与 first face feature 同一用途），
       每 32 次一节课约十几行，够画出一条时间线，也不会把日志刷满 */
    if (1 == m_stStat.u64ExprFed || logOpen(m_stStat.u64ExprFed))
    {
        char szProb[128] = {0};
        int32_t nOff = 0;
        for (size_t i = 0; i < pvecRaw->size(); ++i)
        {
            const int32_t nLeft = static_cast<int32_t>(sizeof(szProb)) - nOff;
            if (nLeft < 16)
            {
                /* 一格最短是 "-1.000," 六个字符，留 16 是防最后一格贴边写不全。
                   类别数多到装不下时宁可少打几格：out= 只是采样看数值形态的，
                   真正要用来标定的分布统计在心跳的 slot= 那几格，那里不会少 */
                break;
            }
            const int nAdd = snprintf(szProb + nOff, static_cast<size_t>(nLeft), "%s%.3f",
                                      (0 == i) ? "" : ",", (*pvecRaw)[i]);
            if (nAdd < 0 || nAdd >= nLeft)
            {
                break;
            }
            nOff += nAdd;
        }
        dlog_info("classroom expr: 目标=%d 最大槽=%d 定判=%d 情绪=%s 概率=%.2f 差距=%.2f 维数=%d 原始=[%s]",
                  nTrackId, stSample.nArgmaxSlot, stSample.bDecided ? 1 : 0,
                  expressionCnName(stSample.enLabel), stSample.fProb, stSample.fMargin,
                  static_cast<int32_t>(pvecRaw->size()), szProb);
    }
}

void CPerceivePipeline::fillExpressionRows(CPersonFeature_S *patRows, int32_t nRowNum,
                                           int64_t llNowMs)
{
    m_stStat.nLastExprRow = 0;
    if (nullptr == patRows || nRowNum <= 0)
    {
        return;
    }

    /* 清表放在这里而不是取特征段里：那一段约 16s 才跑一次，而 TTL 是从"最后一次喂入"起算的。
       只在有人被喂时才清，等于模型掉线或整段被关的那段时间里表一直留着上一节课的人，
       重新打开的第一拍就把旧人的情绪发出去（姿态/复核两张缓存表同一处理，理由也相同） */
    (void)m_stExpr.prune(llNowMs, m_stExprConf.nTtlMs);

    int32_t nHit = 0;
    for (int32_t r = 0; r < nRowNum; ++r)
    {
        CPersonFeature_S &stRow = patRows[r];
        Expression_E enLabel = Expression_E::EXP_UNKNOWN;
        float fProb = 0.0f;
        int64_t llFeedMs = 0;
        /* 事件读**瞬时判定**而不是 EMA 曲线：曲线的职责是
           "平滑出情绪走势"，按设计要记住前几帧；拿它当事件证据会把一次 2 秒的笑
           拖成十几秒，也会把已经停止采样的旧结论拖到 60s 行 TTL。事件新鲜度改由
           expression.event_ttl_ms 管（见 student_behavior 的 112 判定），
           曲线仍按 curveOf 在 feed 侧统计，互不替代。
           bCount=false 的记账口径不变（每拍逐行问一次的数量不能进覆盖率） */
        if (m_stExprConf.bEnable &&
            m_stExpr.latestOf(stRow.nPersonId, enLabel, fProb, &llFeedMs))
        {
            stRow.enExpression = enLabel;
            stRow.fExpressionScore = fProb;
            stRow.llExprMs = llFeedMs;
            ++nHit;
            continue;
        }
        /* 显式清列而不是留着：总线槽位复用，而这一行的 nPersonId 每拍可能已经是另一个人。
           留着会出现"新来的这个人一进门就被量出在开心"，而那张脸连一次推理都没做过 */
        stRow.enExpression = Expression_E::EXP_UNKNOWN;
        stRow.fExpressionScore = 0.0f;
        stRow.llExprMs = 0;
    }
    m_stStat.nLastExprRow = nHit;
}

void CPerceivePipeline::runPhoneStage(const CFrame_S &stFrame, const ConfigPtr &spConf,
                                      CPersonFeature_S *patRows, int32_t nRowNum, int64_t llNowMs)
{
    /* 先清过期：与人脸特征段同一纪律——这一段现在跑不跑得成，与「老结论该不该作废」无关 */
    phoneCachePrune(m_vecPhoneCache, m_nPhoneCacheInUse, llNowMs, m_stPhoneConf.nTtlMs);

    m_stStat.nLastPhoneWant = 0;
    m_stStat.nLastPhoneRows = 0;
    m_stStat.nLastPhoneList = 0;

    if (!m_stPhoneConf.bEnable || nullptr == patRows || nRowNum <= 0)
    {
        /* 关着 = 现场决定不要这一路否决，特征行保持"没有复核结论"（llPhoneMs=0），
           规则层走「只有规则证据」那条分支。这里刻意不回填：回填会让关掉的功能
           在心跳里读起来像"还在出结论" */
        return;
    }

    /* 名单长度在取裁剪档**之前**记：它属于规则层的事实，与这一路现在跑不跑得动无关。
       放在后面就只剩"want 恒 0"，而「模型没驻留」与「没人低头」在心跳里会长得一模一样 */
    const std::vector<int32_t> &vecWantIds = m_pHost->phoneRecheckIds();
    m_stStat.nLastPhoneList = static_cast<int32_t>(vecWantIds.size());

    /* 模型没驻留 = 一次都没法看。整段退出且**不盖任何否决**：跑不了不等于没目标。
       与姿态段一样不在此计数（inferOne 里那把 u64InferUnready 才是这条链的口径），
       这一路的缺位从"list 非 0 而 want 恒 0"读得出来 */
    const CropUnitPtr spCropPhone = m_pHost->cropUnitOf(CLASSROOM_MODEL_KEY_PHONE);
    if (!spCropPhone || !spCropPhone->isReady())
    {
        return;
    }

    if (vecWantIds.empty())
    {
        /* 名单空是常态而不是故障：本拍（严格说是上一拍）没人低头。这段一次推理都不提交，
           但表里还没过期的结论照样要回填——名单断一拍不该打断 106 正在攒的时长门 */
        m_stStat.nLastPhoneRows = applyPhoneCache(patRows, nRowNum, m_vecPhoneCache, llNowMs,
                                                 m_stPhoneConf.nTtlMs);
        return;
    }

    CPhoneSelectParam_S stSel;
    stSel.llNowMs = llNowMs;
    stSel.nIntervalMs = m_stPhoneConf.nMinIntervalMs;
    stSel.nWantNum = clampValue(m_stPhoneConf.nMaxPersons, 1, CLASSROOM_MAX_PERSONS);
    /* 小框下限取检测层那把同名闸：本段不做第二个"多小算太小"的口径 */
    stSel.nMinBodyEdge = spConf->nMinBodyPixel;
    CPhoneSelectStat_S stSkip;
    const size_t nSelNum = selectPhoneTargets(patRows, nRowNum, vecWantIds, m_vecPhoneCache, stSel,
                                              m_vecPhoneCand, m_vecPhoneSel, &stSkip);
    m_stStat.nLastPhoneWant = static_cast<int32_t>(nSelNum);
    m_stStat.u64PhoneSkipOccl += static_cast<uint64_t>(stSkip.nSkipOccluded);
    m_stStat.u64PhoneSkipBox += static_cast<uint64_t>(stSkip.nSkipBox);
    m_stStat.u64PhoneSkipIv += static_cast<uint64_t>(stSkip.nSkipInterval);

    for (size_t nSel = 0; nSel < nSelNum; ++nSel)
    {
        CPersonFeature_S &stRow = patRows[m_vecPhoneSel[nSel]];

        /* 裁剪档吃源帧坐标，而体框在分析帧：与姿态段同一个换算、同一个裁剪档口径
           （§4.5 ①/④——两次裁剪看到的是同一块地方，规则层与复核层不会错位） */
        const CRectf_S stBodySrc = stFrame.stGeom.dstToSrc(stRow.stBodyBox);

        const uint64_t u64InflyBefore = m_stStat.u64InferInflight;
        const uint64_t u64TmoBefore = m_stStat.u64InferTimeout;

        CDetMap stMap;
        const std::vector<float> *pvecRaw = nullptr;
        CRectf_S stUsed;
        bool bLooked = false;    /* 真拿到了一张可判的图（推理成功且映射建得起来） */
        bool bFound = false;
        float fScore = 0.0f;
        if (inferOne(CLASSROOM_MODEL_KEY_PHONE, stFrame, nPRIO_FACE, &stBodySrc, stMap,
                     &pvecRaw, stUsed) && nullptr != pvecRaw)
        {
            bLooked = true;
            /* 解出 0 个框同样是 bLooked：那是"看了，手里没有"，正是否决的依据所在。
               只有推理/映射失败才是"没看"，那一档由 bValid=false 表达 */
            decodeBoxes(*pvecRaw, stMap, m_stPhoneParam, DetectSource_E::DET_SRC_PHONE,
                        m_vecPhoneBox, m_stPhoneDetStat);
            for (const CDetection_S &stBox : m_vecPhoneBox)
            {
                if (stBox.fScore > fScore)
                {
                    fScore = stBox.fScore;
                    bFound = true;
                }
            }
        }

        /* 与姿态段同一条规则：上一拍还在途 / 等到超时说明 NPU 供不上，本拍剩下的人一起放弃，
           且**不盖时间戳**（一次推理都没发生）。裁剪太小、后端拒绝、映射建不起来是真的试过，
           必须盖——不盖的话名单顶名那个人每拍排第一，别人永远轮不到 */
        if (m_stStat.u64InferInflight != u64InflyBefore || m_stStat.u64InferTimeout != u64TmoBefore)
        {
            break;
        }

        /* 记账只有这一支笔：串行/交错两条路径曾各抄一份，ysb 直方图
           只加进了交错那份，板上看到的就是恒 0 的直方图配 97 个 yes。
           收成 accountPhoneAttempt 一个出口，两条路径永远同源 */
        accountPhoneAttempt(stRow, bLooked, bFound, fScore, stUsed, llNowMs, vecWantIds, nRowNum);
    }

    m_stStat.nLastPhoneRows = applyPhoneCache(patRows, nRowNum, m_vecPhoneCache, llNowMs,
                                              m_stPhoneConf.nTtlMs);
}

void CPerceivePipeline::runPoseAndPhoneStage(const CFrame_S &stFrame, const ConfigPtr &spConf,
                                             CPersonFeature_S *patRows, int32_t nRowNum,
                                             int64_t llNowMs, CFeatureFrame_S &stOut)
{
    const CSampleConf_S &stSample = spConf->stSample;

    /* ---------- 姿态侧前奏：与 runPoseStage 逐行同源（注释也在）----------
       新鲜度窗口与规则层同一个函数、同一组入参：两侧对「这副骨骼还作不作数」必须同值，
       否则会出现感知层还在往帧里写、规则层已经判它过期的两种口径。
       两个节拍入参取**实测产能**（本函数自己一路喂出来的 EWMA），不取配置：
       nMaxPosePersons 是"希望一拍做几个"，产能三道闸任一收紧就做不到，
       拿希望值算窗口就是 #69-A 那条后排漏检的成因 */
    const uint64_t u64TriedBefore = m_stStat.u64PoseTried;
    int32_t nRoundMs = 0;
    int32_t nPerRound = 0;
    poseCadenceOf(m_stPoseCadence, nRoundMs, nPerRound);
    int32_t nTtlMs = poseEvidenceTtlMs(nRoundMs, nPerRound, nRowNum);

    /* 先把当前产能发布到帧上：下面提前收工的路径（模型没驻留 / 行表空）
       本拍一个样本都不会产生，帧上那两列仍是目前最好的估计。
       留 0 会让消费侧退回下限窗口，等于在时间片空窗里再把已有骨骼判死一遍 */
    stOut.nPoseRoundMs = nRoundMs;
    stOut.nPosePerRound = nPerRound;

    /* 先清过期项：表按人数上限定容，装不下的唯一原因是 ID 洪泛，
       而过期项不清就会让那些早走掉的人一直占着槽 */
    poseCachePrune(m_vecPoseCache, m_nPoseCacheInUse, llNowMs, nTtlMs);

    m_stStat.nLastPoseWant = 0;
    m_stStat.nLastPoseLeft = 0;

    /* 裁剪档不在 = 模型没驻留（没到位 / 被时间片换下 / 守护降级）。
       这时**一次尝试都没发生**，既不该盖时间戳也不该扣配额：配额会被空烧，
       时间戳会让这一屋子人在模型回来后的一整个周期里都排不上队。
       交错版里「提前收工」= 本侧置闲：姿态不进下面的循环，手机照跑 */
    bool bPoseActive = true;
    const CropUnitPtr spCropPose = m_pHost->cropUnitOf(CLASSROOM_MODEL_KEY_POSE);
    if (!spCropPose || !spCropPose->isReady() || nullptr == patRows || nRowNum <= 0)
    {
        m_stStat.nLastPoseRows = applyPoseCache(patRows, nRowNum, m_vecPoseCache, llNowMs, nTtlMs);
        bPoseActive = false;
    }

    /* ---------- 手机侧前奏：与 runPhoneStage 逐行同源 ---------- */
    /* 先清过期：与人脸特征段同一纪律——这一段现在跑不跑得成，与「老结论该不该作废」无关 */
    phoneCachePrune(m_vecPhoneCache, m_nPhoneCacheInUse, llNowMs, m_stPhoneConf.nTtlMs);

    m_stStat.nLastPhoneWant = 0;
    m_stStat.nLastPhoneRows = 0;
    m_stStat.nLastPhoneList = 0;

    /* 名单只在「这一路要跑」时才向宿主要：关着（配 0）与没人是两种"不该提交"。
       关着时连名单都不取，特征行保持"没有复核结论"（llPhoneMs=0），
       规则层走「只有规则证据」那条分支。这里刻意不回填：回填会让关掉的功能
       在心跳里读起来像"还在出结论" */
    bool bPhoneActive = true;
    const std::vector<int32_t> *pvecWantIds = nullptr;
    if (!m_stPhoneConf.bEnable || nullptr == patRows || nRowNum <= 0)
    {
        bPhoneActive = false;
    }
    else
    {
        pvecWantIds = &m_pHost->phoneRecheckIds();
        /* 名单长度在取裁剪档**之前**记：它属于规则层的事实，与这一路现在跑不跑得动无关。
           放在后面就只剩"want 恒 0"，而「模型没驻留」与「没人低头」在心跳里会长得一模一样 */
        m_stStat.nLastPhoneList = static_cast<int32_t>(pvecWantIds->size());

        /* 模型没驻留 = 一次都没法看。本侧收工且**不盖任何否决**：跑不了不等于没目标。
           与姿态段一样不在此计数（inferSubmitOne 里那把 u64InferUnready 才是这条链的口径），
           这一路的缺位从"list 非 0 而 want 恒 0"读得出来 */
        const CropUnitPtr spCropPhone = m_pHost->cropUnitOf(CLASSROOM_MODEL_KEY_PHONE);
        if (!spCropPhone || !spCropPhone->isReady())
        {
            bPhoneActive = false;
        }
        else if (pvecWantIds->empty())
        {
            /* 名单空是常态而不是故障：本拍（严格说是上一拍）没人低头。这段一次推理都不提交，
               但表里还没过期的结论照样要回填——名单断一拍不该打断 106 正在攒的时长门 */
            m_stStat.nLastPhoneRows = applyPhoneCache(patRows, nRowNum, m_vecPhoneCache, llNowMs,
                                                      m_stPhoneConf.nTtlMs);
            bPhoneActive = false;
        }
    }

    /* ---------- 两路一次挑好（挑选本身不碰 NPU，先做完再交错提交）---------- */
    int32_t nWant = 0;
    int32_t nLeft = 0;
    if (bPoseActive)
    {
        CInferScheduler *pSched = m_pHost->scheduler();
        nWant = poseWantNum(pSched, spConf, llNowMs, nRowNum, nLeft);
        m_stStat.nLastPoseWant = nWant;
        m_stStat.nLastPoseLeft = nLeft;
    }
    if (bPoseActive && nWant > 0)
    {
        CPoseSelectParam_S stSel;
        stSel.llNowMs = llNowMs;
        stSel.nIntervalMs = stSample.nPoseIntervalMs;
        stSel.nWantNum = nWant;
        stSel.bSkipOccluded = true;
        stSel.fBlurMin = spConf->fPoseBlurMin;
        /* 规则层的紧急度与实测窗口一起进挑选（P0-1）：排序从「最久没做」升级为
           「规则在等 → 临近过期 → 最久没做」。表是引擎在 perceiveStep 抄进本帧的
           上一拍结论，与行表同一帧同一线程，无跨线程问题；窗口用本函数刚量出的
           nTtlMs，与回填骨骼时的新鲜度判据同源 */
        stSel.pastUrgency = stOut.astPoseUrgency;
        stSel.nUrgencyNum = stOut.nPoseUrgencyNum;
        stSel.nPoseTtlMs = nTtlMs;
        selectPoseTargets(patRows, nRowNum, m_vecPoseCache, stSel, m_vecPoseCand, m_vecPoseSel);
    }
    else
    {
        /* 挑选没跑的这一拍，选中表必须显式清空：它是成员、上一拍的残留还在里面，
           留着会让下面的循环对已失效的行下标再裁一次 */
        m_vecPoseSel.clear();
    }
    const size_t nPoseSelNum = m_vecPoseSel.size();

    size_t nPhoneSelNum = 0;
    if (bPhoneActive)
    {
        CPhoneSelectParam_S stSel;
        stSel.llNowMs = llNowMs;
        stSel.nIntervalMs = m_stPhoneConf.nMinIntervalMs;
        stSel.nWantNum = clampValue(m_stPhoneConf.nMaxPersons, 1, CLASSROOM_MAX_PERSONS);
        /* 小框下限取检测层那把同名闸：本段不做第二个"多小算太小"的口径 */
        stSel.nMinBodyEdge = spConf->nMinBodyPixel;
        CPhoneSelectStat_S stSkip;
        nPhoneSelNum = selectPhoneTargets(patRows, nRowNum, *pvecWantIds, m_vecPhoneCache, stSel,
                                          m_vecPhoneCand, m_vecPhoneSel, &stSkip);
        m_stStat.nLastPhoneWant = static_cast<int32_t>(nPhoneSelNum);
        m_stStat.u64PhoneSkipOccl += static_cast<uint64_t>(stSkip.nSkipOccluded);
        m_stStat.u64PhoneSkipBox += static_cast<uint64_t>(stSkip.nSkipBox);
        m_stStat.u64PhoneSkipIv += static_cast<uint64_t>(stSkip.nSkipInterval);
    }
    else
    {
        m_vecPhoneSel.clear();
    }

    /* ---------- 交错驱动：提交姿态 → 等手机 → 提交手机 → 等姿态 ----------
       NPU 队列里始终压着两路任务：A 刚提交的姿态在跑时，CPU 把手机的裁剪备好提交进队，
       D 等姿态返回的这段时间 NPU 已经在跑手机。让路（giveup）两侧各自独立：
       一侧供不上只收工那一侧，另一侧的任务与槽完全独立（m_mapSlot 各一条目）。
       结账时点与原两段的唯一差别：提交即真败（裁剪太小/后端拒绝）在提交格当场记，
       不再等一个永远不会来的等待返回——计数语义与原循环体逐格相同 */
    size_t nPoseIdx = 0;
    size_t nPhoneIdx = 0;
    SPendingTok_S stPoseTok;
    SPendingTok_S stPhoneTok;
    bool bPoseInFlight = false;
    bool bPhoneInFlight = false;
    bool bPoseGiveup = false;
    bool bPhoneGiveup = false;
    while ((!bPoseGiveup && nPoseIdx < nPoseSelNum) || (!bPhoneGiveup && nPhoneIdx < nPhoneSelNum) ||
           bPoseInFlight || bPhoneInFlight)
    {
        /* A) 提交一个姿态 */
        if (!bPoseGiveup && !bPoseInFlight && nPoseIdx < nPoseSelNum)
        {
            CPersonFeature_S &stRow = patRows[m_vecPoseSel[nPoseIdx]];

            /* 体框在分析帧，像素在源帧：先换算过去裁剪，回来再经 CDetMap 换算到分析帧。
               两次换算都过同一个 CFrameGeometry，所以镜像机位上点与框仍然对得上（§4.8） */
            const CRectf_S stBodySrc = stFrame.stGeom.dstToSrc(stRow.stBodyBox);

            const uint64_t u64InflyBefore = m_stStat.u64InferInflight;
            if (inferSubmitOne(CLASSROOM_MODEL_KEY_POSE, stFrame, nPRIO_POSE, &stBodySrc, stPoseTok))
            {
                bPoseInFlight = true;
            }
            else if (m_stStat.u64InferInflight != u64InflyBefore)
            {
                /* 上一拍还在途：NPU 已经供不上，本拍姿态侧剩下的人一并放弃。
                   这**不盖时间戳**——一次推理都没发生，记它等于让这个人白等一个周期 */
                bPoseGiveup = true;
            }
            else
            {
                /* 其余失败（裁剪太小、后端拒绝）都是真的试过，必须盖：
                   不盖的话顶名那个人每拍都排第一，全班其他人永远轮不到（饿死一整排）。
                   骨骼默认无效，applyPoseCache 不会把它当证据，只是把人推后一个周期 */
                accountPoseAttempt(stRow, false, CPoseSkeleton_S(), CRectf_S(), llNowMs, nRowNum);
                ++nPoseIdx;
            }
        }

        /* B) 等一个手机（上一轮 C 提交的） */
        if (bPhoneInFlight)
        {
            CPersonFeature_S &stRow = patRows[m_vecPhoneSel[nPhoneIdx]];

            const uint64_t u64TmoBefore = m_stStat.u64InferTimeout;
            CDetMap stMap;
            const std::vector<float> *pvecRaw = nullptr;
            bool bLooked = false;    /* 真拿到了一张可判的图（推理成功且映射建得起来） */
            bool bFound = false;
            float fScore = 0.0f;
            if (inferWaitOne(CLASSROOM_MODEL_KEY_PHONE, stFrame, stPhoneTok, &pvecRaw, stMap) &&
                nullptr != pvecRaw)
            {
                bLooked = true;
                /* 解出 0 个框同样是 bLooked：那是"看了，手里没有"，正是否决的依据所在。
                   只有推理/映射失败才是"没看"，那一档由 bValid=false 表达 */
                decodeBoxes(*pvecRaw, stMap, m_stPhoneParam, DetectSource_E::DET_SRC_PHONE,
                            m_vecPhoneBox, m_stPhoneDetStat);
                for (const CDetection_S &stBox : m_vecPhoneBox)
                {
                    if (stBox.fScore > fScore)
                    {
                        fScore = stBox.fScore;
                        bFound = true;
                    }
                }
            }
            bPhoneInFlight = false;

            /* 等到超时同样说明 NPU 供不上：本侧收工，这人不记账不盖戳 */
            if (m_stStat.u64InferTimeout != u64TmoBefore)
            {
                bPhoneGiveup = true;
            }
            else
            {
                accountPhoneAttempt(stRow, bLooked, bFound, fScore, stPhoneTok.stUsedSrc,
                                    llNowMs, *pvecWantIds, nRowNum);
                ++nPhoneIdx;
            }
        }

        /* C) 提交一个手机 */
        if (!bPhoneGiveup && !bPhoneInFlight && nPhoneIdx < nPhoneSelNum)
        {
            CPersonFeature_S &stRow = patRows[m_vecPhoneSel[nPhoneIdx]];

            /* 裁剪档吃源帧坐标，而体框在分析帧：与姿态段同一个换算、同一个裁剪档口径
               （§4.5 ①/④——两次裁剪看到的是同一块地方，规则层与复核层不会错位） */
            const CRectf_S stBodySrc = stFrame.stGeom.dstToSrc(stRow.stBodyBox);

            const uint64_t u64InflyBefore = m_stStat.u64InferInflight;
            if (inferSubmitOne(CLASSROOM_MODEL_KEY_PHONE, stFrame, nPRIO_FACE, &stBodySrc, stPhoneTok))
            {
                bPhoneInFlight = true;
            }
            else if (m_stStat.u64InferInflight != u64InflyBefore)
            {
                /* 上一拍还在途：本侧收工，不盖时间戳（与姿态侧同一条规则） */
                bPhoneGiveup = true;
            }
            else
            {
                /* 裁剪太小、后端拒绝是真的试过，必须盖——不盖的话名单顶名那个人
                   每拍排第一，别人永远轮不到。bLooked=false 进表即「没看成」 */
                accountPhoneAttempt(stRow, false, false, 0.0f, CRectf_S(), llNowMs,
                                    *pvecWantIds, nRowNum);
                ++nPhoneIdx;
            }
        }

        /* D) 等一个姿态（本轮 A 提交的） */
        if (bPoseInFlight)
        {
            CPersonFeature_S &stRow = patRows[m_vecPoseSel[nPoseIdx]];

            const uint64_t u64TmoBefore = m_stStat.u64InferTimeout;
            CPoseSkeleton_S stSkl;
            CDetMap stMap;
            const std::vector<float> *pvecRaw = nullptr;
            bool bDecoded = false;
            if (inferWaitOne(CLASSROOM_MODEL_KEY_POSE, stFrame, stPoseTok, &pvecRaw, stMap) &&
                nullptr != pvecRaw)
            {
                bDecoded = decodePose(*pvecRaw, stMap, m_stPoseParam, stSkl, m_stPoseStat);
            }
            bPoseInFlight = false;

            if (m_stStat.u64InferTimeout != u64TmoBefore)
            {
                /* 等到超时：本侧收工，不盖时间戳（与原 giveup 同一条规则） */
                bPoseGiveup = true;
            }
            else
            {
                accountPoseAttempt(stRow, bDecoded, stSkl, stPoseTok.stUsedSrc, llNowMs, nRowNum);
                ++nPoseIdx;
            }
        }
    }

    /* ---------- 姿态尾段：与 runPoseStage 尾段逐行同源 ----------
       本拍真的做了 nTried 个人 → 刷新实测产能，再按新产能重算窗口并重发帧上那两列。
       顺序不能反：帧上的两列必须与本函数回填时用的窗口同源，
       否则消费侧照帧重算出的 TTL 会与本拍刚用过的值差一拍（同一件事两个口径）*/
    if (bPoseActive)
    {
        poseCadenceFeed(m_stPoseCadence,
                        static_cast<int32_t>(m_stStat.u64PoseTried - u64TriedBefore), llNowMs);
        poseCadenceOf(m_stPoseCadence, nRoundMs, nPerRound);
        stOut.nPoseRoundMs = nRoundMs;
        stOut.nPosePerRound = nPerRound;
        nTtlMs = poseEvidenceTtlMs(nRoundMs, nPerRound, nRowNum);

        if (poseCapacityShort(nRoundMs, nPerRound, nRowNum))
        {
            /* 产能撑不起这一屋子人：窗口已经被上限钳住，排在圈尾的人回填时就是过期的。
               这条计数是「后排整节课零事件」在机上唯一的出口——没有它，
               「这一排真的没人举手」与「我们根本轮不到那一排」在日志里长得一模一样。
               注意它**不是**放宽窗口的理由：放宽由上限定死，这里只负责说出来 */
            ++m_stStat.u64PoseCapacityShort;
            if (logOpen(m_stStat.u64PoseCapacityShort))
            {
                dlog_warn("classroom perceive: 姿态产能跟不上教室人数, rows=%d "
                          "round=%dms per_round=%d cycle=%lldms ttl=%dms(cap %d) want=%d left=%d",
                          nRowNum, nRoundMs, nPerRound,
                          static_cast<long long>(poseCycleMs(nRoundMs, nPerRound, nRowNum)),
                          nTtlMs, CLASSROOM_POSE_TTL_MAX_MS, nWant, nLeft);
            }
        }

        /* 回填放在最后：本拍刚做的几个人和轮不到的老骨骼走同一条路进帧，
           规则层看到的每一行都是「这屏此刻最该有证据的那副骨骼」，不必知道它是哪一拍产的 */
        m_stStat.nLastPoseRows = applyPoseCache(patRows, nRowNum, m_vecPoseCache, llNowMs, nTtlMs);
    }

    /* ---------- 手机尾段：与 runPhoneStage 尾段逐行同源 ---------- */
    if (bPhoneActive)
    {
        m_stStat.nLastPhoneRows = applyPhoneCache(patRows, nRowNum, m_vecPhoneCache, llNowMs,
                                                  m_stPhoneConf.nTtlMs);
    }
}

void CPerceivePipeline::accountPoseAttempt(CPersonFeature_S &stRow, bool bDecoded,
                                           const CPoseSkeleton_S &stSkl, const CRectf_S &stUsed,
                                           int64_t llNowMs, int32_t nRowNum)
{
    ++m_stStat.u64PoseTried;
    if (bDecoded && stSkl.bValid)
    {
        ++m_stStat.u64PoseUsable;
        if (1 == m_stStat.u64PoseUsable)
        {
            dlog_info("classroom perceive: 首个骨架成功 id=%d pts_score=%.2f "
                      "crop=%dx%d from src(%.0f,%.0f)", stRow.nPersonId, stSkl.fScore,
                      static_cast<int>(stUsed.width()), static_cast<int>(stUsed.height()),
                      stUsed.fX1, stUsed.fY1);
        }
    }

    /* 无效结果也写表：那副骨骼的产出时刻是真实发生过的，
       applyPoseCache 会因 bValid=false 而不把它当证据，而挑选据此把人推后一个周期 */
    if (!poseCacheUpsert(m_vecPoseCache, m_nPoseCacheInUse, stRow.nPersonId, stSkl, llNowMs))
    {
        ++m_stStat.u64PoseCacheFull;
        if (logOpen(m_stStat.u64PoseCacheFull))
        {
            dlog_warn("classroom perceive: 姿态缓存满（%d 槽, id 可能在泛滥）, "
                      "trk=%d rows=%d", static_cast<int32_t>(m_vecPoseCache.size()),
                      m_stStat.nLastTrackNum, nRowNum);
        }
    }
    /* 配额按「含失败尝试」记账：判据层（pose_admit）只读这本人次账，
       记账的义务在提交侧——这里就是那支笔 */
    ++m_stPoseAdmit.nInWindow;
}

void CPerceivePipeline::accountPhoneAttempt(CPersonFeature_S &stRow, bool bLooked, bool bFound,
                                            float fScore, const CRectf_S &stUsed, int64_t llNowMs,
                                            const std::vector<int32_t> &vecWantIds,
                                            int32_t nRowNum)
{
    ++m_stStat.u64PhoneTried;
    if (bLooked)
    {
        if (bFound)
        {
            ++m_stStat.u64PhoneFound;
            /* #87：yes 的分数落直方图，分布进 metric phone 段（ysb=8 桶） */
            ++m_stStat.au64PhoneYesBucket[phoneYesScoreBucket(fScore)];
            if (1 == m_stStat.u64PhoneFound)
            {
                dlog_info("classroom perceive: 首个手机框 id=%d score=%.2f boxes=%zu "
                          "crop=%dx%d from src(%.0f,%.0f)", stRow.nPersonId, fScore,
                          m_vecPhoneBox.size(), static_cast<int>(stUsed.width()),
                          static_cast<int>(stUsed.height()), stUsed.fX1, stUsed.fY1);
            }
        }
        else
        {
            ++m_stStat.u64PhoneNo;
        }
    }

    if (!phoneCacheUpsert(m_vecPhoneCache, m_nPhoneCacheInUse, stRow.nPersonId, bLooked, bFound,
                          fScore, llNowMs))
    {
        ++m_stStat.u64PhoneCacheFull;
        if (logOpen(m_stStat.u64PhoneCacheFull))
        {
            dlog_warn("classroom perceive: 手机缓存满（%d 槽, id 可能在泛滥）, "
                      "want=%d rows=%d", static_cast<int32_t>(m_vecPhoneCache.size()),
                      static_cast<int32_t>(vecWantIds.size()), nRowNum);
        }
    }
}

float CPerceivePipeline::blurScoreOf(const CFrame_S &stFrame, const CRectf_S &stBoxCanvas) const
{
    if (!stFrame.pucNv12 || stFrame.nSrcW <= 4 || stFrame.nSrcH <= 4 || m_fBlurFullGrad <= 0.0f)
    {
        return 0.0f;
    }

    /* 框在源帧侧取：像素从未镜像的源帧上读，与分析帧坐标不是一套。
       少这一步的后果是镜像机位上「清晰度」与画面内容无关 */
    const CRectf_S stSrc = stFrame.stGeom.dstToSrc(stBoxCanvas);
    const int nW = stFrame.nSrcW;
    const int nH = stFrame.nSrcH;

    int nX1 = static_cast<int>(clampValue(stSrc.fX1, 0.0f, static_cast<float>(nW - 1)));
    int nY1 = static_cast<int>(clampValue(stSrc.fY1, 0.0f, static_cast<float>(nH - 1)));
    int nX2 = static_cast<int>(clampValue(stSrc.fX2, 0.0f, static_cast<float>(nW - 1)));
    int nY2 = static_cast<int>(clampValue(stSrc.fY2, 0.0f, static_cast<float>(nH - 1)));
    if (nX2 - nX1 < 4 || nY2 - nY1 < 4)
    {
        return 0.0f;
    }

    /* 隔 2 像素采样：清晰度代理要的是量级不是精度，逐像素扫会让 60 人×大框吃掉整拍的预算。
       只读亮度平面（NV12 前 W×H 字节），碰色度会让带宽翻倍而没有信息增量 */
    const uint8_t *pucY = stFrame.pucNv12;
    const int nStep = 2;
    uint64_t u64Sum = 0;
    uint32_t u32Num = 0;
    for (int nY = nY1 + nStep; nY < nY2; nY += nStep)
    {
        const uint8_t *pucRow = pucY + static_cast<size_t>(nY) * nW;
        const uint8_t *pucPrev = pucY + static_cast<size_t>(nY - nStep) * nW;
        for (int nX = nX1 + nStep; nX < nX2; nX += nStep)
        {
            const int nCur = pucRow[nX];
            int nDx = nCur - pucRow[nX - nStep];
            int nDy = nCur - pucPrev[nX];
            u64Sum += static_cast<uint64_t>((nDx < 0 ? -nDx : nDx) + (nDy < 0 ? -nDy : nDy));
            ++u32Num;
        }
    }

    if (0 == u32Num)
    {
        return 0.0f;
    }
    const float fMean = static_cast<float>(u64Sum) / static_cast<float>(u32Num);
    return clampValue(fMean / m_fBlurFullGrad, 0.0f, 1.0f);
}

void CPerceivePipeline::pushOsd(const CFeatureFrame_S &stFrame)
{
    m_vecOsdBox.clear();

    const int32_t nCap = static_cast<int32_t>(stFrame.vecPersons.size());
    const int32_t nNum = (stFrame.nPersonNum < nCap) ? stFrame.nPersonNum : nCap;
    for (int32_t i = 0; i < nNum; ++i)
    {
        /* 只画头框：体框全部由头框外扩推得（#96），是几何猜测而非检测证据，
           常态画出来只会让人以为「检测框大得离谱」 */
        m_vecOsdBox.push_back(stFrame.vecPersons[i].stHeadBox);
    }

    /* 空表也要送：整机结果管理器只缓存 450ms，收到空表才会抹掉上一次的框 */
    classroomOsdPush(stFrame.stGeom, m_vecOsdBox);
    ++m_stStat.u64OsdPush;
}

std::string CPerceivePipeline::statText() const
{
    /* 一张表看全链路：这四组数的分工是「排除法」的四级台阶——
       管线说本拍跑没跑，解码说框为什么少，关联说外扩框铺没铺满每人一行，跟踪说 ID 有没有洪泛。
       少任何一级，现场都要靠加日志重新编译一次才能定位 */
    /* 缓冲大小不再靠手算钉死。上一版在这里写着「字面量 635B + 数值 97 槽 ×20 + 子表 1152B
       = 3565B，故取 3584」，那笔账的维护方式是"每加一个字段回来重算一遍"，而重算漏了的形态
       是尾巴被砍——被砍的读数看着和「后面那些计数都是 0」一模一样，会把人引去查一条
       根本不存在的丢帧。改成两趟：先按栈上快路径缓冲试一趟（绝大多数调用就到此为止），
       量出来的长度装不下就照那个长度在堆上重来。与 dlog 单条超 4096 B 时换 std::string
       同一手法，理由也同：宁可多一次格式化，也不要少一个字段。
       格式串与实参表整体包进一个 lambda 而不是抄两遍：一处改动两处生效，
       而且 snprintf 的格式检查照样跟着走 */
    char szFeat[256] = {0};
    char szProbe[256] = {0};
    /* 特征表与探针各自的串由各自的属主函数产（字段含义只有它们自己知道），
       这里只负责拼进同一段：两处格式串分在两个文件里，比一处 200 字符的续行好改 */
    (void)faceFeatStatText(m_vecFeat, m_stFeatStat, szFeat, static_cast<int32_t>(sizeof(szFeat)));
    (void)alignProbeText(m_stAlignProbe, szProbe, static_cast<int32_t>(sizeof(szProbe)));
    /* 判定级的分解（概率不足/差值不足/长度不符/槽位分布）归平滑器自己产：那些字段含义
       只有它知道，与上面两张子表同一分工。它自己带溢出判据，截断时给一整句 marker */
    const std::string strExpr = m_stExpr.statText();
    const CTrackerStat_S stTrk = m_stTrk.stat();   /* 一次取表：stat() 逐次调用会反复整表拷贝 */
    char szBuf[3584] = {0};                        /* 快路径缓冲：装得下就不碰堆 */
    char szTbf[224] = {0};                         /* T37 背身面部分数直方图：4 组 × 8 桶 */
    turnbackFaceStatText(szTbf, static_cast<int32_t>(sizeof(szTbf)));
    const auto fnStat = [&](char *pszDst, int32_t nCap) -> int
    {
        return snprintf(pszDst, static_cast<size_t>(nCap),
                              "head=%llu/%llu det=%d trk=%d rows=%llu cost=%lldms"
                              " |drop crop=%llu sub=%llu tmo=%llu fly=%llu unr=%llu stale=%llu be=%llu"
                              " map=%llu wedge=%llu"
                              " |hdec ok=%llu low=%llu small=%llu huge=%llu trunc=%llu mal=%llu off=%llu nmap=%llu"
                              " |asm expand=%llu"
                              " |pose try=%llu ok=%llu rows=%d want=%d left=%d busy=%llu cfull=%llu over=%llu"
                              " dec=%llu short=%llu nan=%llu weak=%llu"
                              " ear=%llu above=%llu eline=%llu dup=%llu ddn=%llu"
                              " %s"
                              " |face try=%llu ok=%llu det=%d row=%d unmatch=%llu"
                              " dec=%llu ok=%llu low=%llu small=%llu mal=%llu pob=%llu nmap=%llu"
                              " align=%llu/%llu resid=%llu low=%llu spread=%llu eyeline=%llu"
                              " eyespan=%llu scale=%llu finite=%llu"
                              " |feat try=%llu ok=%llu nocrop=%llu noperm=%llu noalign=%llu"
                              " nopool=%llu nowarp=%llu busy=%llu nogal=%llu hit=%llu rgn=%llu"
                              " n=%d cost=%lldms warm=%llu %s"
                              " |expr try=%llu fed=%llu busy=%llu szbad=%llu ang=%llu nobad=%d rows=%d %s"
                              " |phone try=%llu yes=%llu no=%llu want=%d list=%d rows=%d cfull=%llu"
                              " skip=%llu/%llu/%llu"
                              " dec low=%llu small=%llu"
                              " ysb=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu"
                              " |probe n=%llu bad=%d %s"
                              " |trk upd=%llu match=%llu new=%llu drop=%llu idrej=%llu idmax=%d confmax=%d nextid=%d"
                              " |osd=%llu",
                              static_cast<unsigned long long>(m_stStat.u64HeadRounds),
                              static_cast<unsigned long long>(m_stStat.u64HeadOk),
                              m_stStat.nLastHeadDet, m_stStat.nLastTrackNum,
                              static_cast<unsigned long long>(m_stStat.u64Rows),
                              static_cast<long long>(m_stStat.llLastRoundMs),
                              static_cast<unsigned long long>(m_stStat.u64CropFail),
                              static_cast<unsigned long long>(m_stStat.u64SubmitFail),
                              static_cast<unsigned long long>(m_stStat.u64InferTimeout),
                              static_cast<unsigned long long>(m_stStat.u64InferInflight),
                              static_cast<unsigned long long>(m_stStat.u64InferUnready),
                              static_cast<unsigned long long>(m_stStat.u64InferStale),
                              static_cast<unsigned long long>(m_stStat.u64InferBackendFail),
                              static_cast<unsigned long long>(m_stStat.u64MapFail),
                              static_cast<unsigned long long>(m_stStat.u64WedgeCleared),
                              static_cast<unsigned long long>(m_stDecodeStat.u64Accepted),
                              static_cast<unsigned long long>(m_stDecodeStat.u64LowScore),
                              static_cast<unsigned long long>(m_stDecodeStat.u64TooSmall),
                              static_cast<unsigned long long>(m_stDecodeStat.u64TooLarge),
                              static_cast<unsigned long long>(m_stDecodeStat.u64Truncated),
                              static_cast<unsigned long long>(m_stDecodeStat.u64Malformed),
                              static_cast<unsigned long long>(m_stDecodeStat.u64OffCanvas),
                              static_cast<unsigned long long>(m_stDecodeStat.u64MapNotReady),
                              static_cast<unsigned long long>(m_stAssembleStat.u64Expanded),
                              static_cast<unsigned long long>(m_stStat.u64PoseTried),
                              static_cast<unsigned long long>(m_stStat.u64PoseUsable),
                              m_stStat.nLastPoseRows, m_stStat.nLastPoseWant,
                              m_stStat.nLastPoseLeft,
                              static_cast<unsigned long long>(m_stStat.u64PoseBusySkip),
                              static_cast<unsigned long long>(m_stStat.u64PoseCacheFull),
                              static_cast<unsigned long long>(m_stStat.u64PoseCapacityShort),
                              static_cast<unsigned long long>(m_stPoseStat.u64Decoded),
                              static_cast<unsigned long long>(m_stPoseStat.u64TooShort),
                              static_cast<unsigned long long>(m_stPoseStat.u64NotFinite),
                              static_cast<unsigned long long>(m_stPoseStat.u64WeakTrunk),
                              static_cast<unsigned long long>(m_stPoseStat.u64EarUsable),
                              static_cast<unsigned long long>(m_stPoseStat.u64EarNoseAbove),
                              static_cast<unsigned long long>(m_stPoseStat.u64EarLineUsable),
                              static_cast<unsigned long long>(m_stPoseStat.u64DocUpShape),
                              static_cast<unsigned long long>(m_stPoseStat.u64DocDownShape),
                              szTbf,
                              static_cast<unsigned long long>(m_stStat.u64FaceRounds),
                              static_cast<unsigned long long>(m_stStat.u64FaceOk),
                              m_stStat.nLastFaceDet, m_stStat.nLastFaceRow,
                              static_cast<unsigned long long>(m_stStat.u64FaceUnmatched),
                              static_cast<unsigned long long>(m_stFaceStat.u64Rows),
                              static_cast<unsigned long long>(m_stFaceStat.u64Accepted),
                              static_cast<unsigned long long>(m_stFaceStat.u64LowScore),
                              static_cast<unsigned long long>(m_stFaceStat.u64TooSmall),
                              static_cast<unsigned long long>(m_stFaceStat.u64Malformed),
                              static_cast<unsigned long long>(m_stFaceStat.u64PointOutOfBox),
                              static_cast<unsigned long long>(m_stFaceStat.u64MapNotReady),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64Ok),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64Tried),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64FailResidual),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64FailLowScore),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64FailSpread),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64FailEyeLine),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64FailEyeSpan),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64FailScale),
                              static_cast<unsigned long long>(m_stFaceAlignStat.u64FailNotFinite),
                              static_cast<unsigned long long>(m_stStat.u64FeatTried),
                              static_cast<unsigned long long>(m_stStat.u64FeatOk),
                              static_cast<unsigned long long>(m_stStat.u64FeatNoCrop),
                              static_cast<unsigned long long>(m_stStat.u64FeatNoPerm),
                              static_cast<unsigned long long>(m_stStat.u64FeatNoAlign),
                              static_cast<unsigned long long>(m_stStat.u64FeatNoPool),
                              static_cast<unsigned long long>(m_stStat.u64FeatNoWarp),
                              static_cast<unsigned long long>(m_stStat.u64FeatBusySkip),
                              static_cast<unsigned long long>(m_stStat.u64FeatNoGallery),
                              static_cast<unsigned long long>(m_stStat.u64FeatHit),
                              static_cast<unsigned long long>(m_stStat.u64FeatRegionSkip),
                              m_stStat.nLastFeatNum,
                              static_cast<long long>(m_stStat.llLastFeatCostMs),
                              static_cast<unsigned long long>(m_stStat.u64FeatWarmBeats),
                              szFeat,
                              static_cast<unsigned long long>(m_stStat.u64ExprTried),
                              static_cast<unsigned long long>(m_stStat.u64ExprFed),
                              static_cast<unsigned long long>(m_stStat.u64ExprBusySkip),
                              static_cast<unsigned long long>(m_stStat.u64ExprSizeBad),
                              static_cast<unsigned long long>(m_stStat.u64ExprAngleSkip),
                              m_stStat.nLastExprAngleBad,
                              m_stStat.nLastExprRow,
                              strExpr.c_str(),
                              static_cast<unsigned long long>(m_stStat.u64PhoneTried),
                              static_cast<unsigned long long>(m_stStat.u64PhoneFound),
                              static_cast<unsigned long long>(m_stStat.u64PhoneNo),
                              m_stStat.nLastPhoneWant, m_stStat.nLastPhoneList,
                              m_stStat.nLastPhoneRows,
                              static_cast<unsigned long long>(m_stStat.u64PhoneCacheFull),
                              static_cast<unsigned long long>(m_stStat.u64PhoneSkipOccl),
                              static_cast<unsigned long long>(m_stStat.u64PhoneSkipBox),
                              static_cast<unsigned long long>(m_stStat.u64PhoneSkipIv),
                              static_cast<unsigned long long>(m_stPhoneDetStat.u64LowScore),
                              static_cast<unsigned long long>(m_stPhoneDetStat.u64TooSmall),
                              static_cast<unsigned long long>(m_stStat.au64PhoneYesBucket[0]),
                              static_cast<unsigned long long>(m_stStat.au64PhoneYesBucket[1]),
                              static_cast<unsigned long long>(m_stStat.au64PhoneYesBucket[2]),
                              static_cast<unsigned long long>(m_stStat.au64PhoneYesBucket[3]),
                              static_cast<unsigned long long>(m_stStat.au64PhoneYesBucket[4]),
                              static_cast<unsigned long long>(m_stStat.au64PhoneYesBucket[5]),
                              static_cast<unsigned long long>(m_stStat.au64PhoneYesBucket[6]),
                              static_cast<unsigned long long>(m_stStat.au64PhoneYesBucket[7]),
                              static_cast<unsigned long long>(m_stAlignProbe.u64Face),
                              m_stAlignProbe.nBad,
                              szProbe,
                              static_cast<unsigned long long>(stTrk.u64Updates),
                              static_cast<unsigned long long>(stTrk.u64Matched),
                              static_cast<unsigned long long>(stTrk.u64TracksCreated),
                              static_cast<unsigned long long>(stTrk.u64TracksDropped),
                              static_cast<unsigned long long>(stTrk.u64TrackPoolFull),
                              stTrk.nActiveMax, stTrk.nConfirmedMax,
                              stTrk.nNextId,
                              static_cast<unsigned long long>(m_stStat.u64OsdPush));
    };

    const int nLen = fnStat(szBuf, static_cast<int32_t>(sizeof(szBuf)));
    if (nLen < 0)
    {
        return std::string("perceive_stat=fmt_error");
    }
    if (nLen < static_cast<int32_t>(sizeof(szBuf)))
    {
        return std::string(szBuf);
    }

    /* 快路径装不下：照量出来的长度在堆上重来一趟，不砍尾巴。
       砍尾的读数看着和「后面那些计数都是 0」一模一样，而后者会把人引去查一条根本不存在的
       丢帧；两趟同参必然同长，所以第二趟要么正好填满要么就是格式化本身出错，没有第三种。
       串尾留 widened=1：这一条走过宽路径是**读数自己**说出来的，不另开日志行——
       缓冲常年不够用只在该加缓冲，而该不该加由 CSV 里这个标记的密度决定 */
    std::string strWide(static_cast<size_t>(nLen) + 1u, '\0');
    const int nAgain = fnStat(&strWide[0], nLen + 1);
    if (nAgain < 0 || nAgain > nLen)
    {
        return std::string("perceive_stat=fmt_error");
    }
    strWide.resize(static_cast<size_t>(nAgain));
    return strWide + " widened=1";
}
}
