/**
 * @FilePath     : geometry.h
 * @Author       : leiyy
 * @Date         : 2026-10-19 10:10:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 10:10:00
 * @Description  : 几何判定纯函数
 */

#pragma once

#include "classroom_define.h"

namespace Classroom
{
/* ============================ 判据参数（与 Web 下发参数一一对应） ============================ */

/* 低头：眉心垂点到鼻子 / 眉心垂点到颈部 的比值 */
struct HeadDownRule_S
{
    float fKeyScore = 0.5f;  /* 用到的关键点最低得分 */
    float fRatio = 0.75f;    /* 比值门限，越大越严格 */
};

/* 抬头：左右眼连线与鼻子构成的夹角。
   2默认 120→110：学生机位同批交替 A/B（student3×600，a=120+b=110，
   各 3 跑）里 120° 落在可判样本的 p99 上，听讲中位 0/6/6 起；110° 臂中位 30/31/40 起、
   抬头中位 15→50，零重叠。定门限的素材与读数见  */
struct HeadUpRule_S
{
    float fKeyScore = 0.8f;
    float fAngle = 110.0f;
};

/* 站立：髋-膝近似竖直 + 手臂近似伸直 */
struct StandRule_S
{
    float fKeyScore = 0.8f;
    float fLegVerticalAngle = 20.0f;  /* 膝-髋连线与竖直方向夹角上限 */
    float fArmStraightAngle = 165.0f; /* 肘-腕 与 肘-肩 夹角下限（手臂伸直） */
    /* >0 时要求「颈→髋」长度至少为肩宽的该倍数，用来识别"髋被课桌挡住"的坐姿。
       坐着的人髋在桌沿以下，姿态模型把髋点补在桌沿 ⇒ 躯干被截短。 */
    float fMinTorsoShoulder = 1.45f;
};

/* 举手：腕高于肘 + 肘-腕连线与竖直方向夹角足够小 */
struct RaiseHandRule_S
{
    float fKeyScore = 0.8f;
    float fVerticalAngle = 30.0f;
    /* >0 时要求手腕至少高出头顶该比例（按肩宽归一），抑制"摸头/托腮"误判。
       从 0.0 提到 0.3：s3+s1 两路 67 条 stu_raise_hand 的该归一量全部 <0
       （-3.60~-0.68），抽帧 8/8 是托腮/扶眼镜；板上同批 A/B（s3 各 3 跑）102 从 110 段降到 0。
       0.0 等于把这道闸关掉，而它正是「举手」二字的定义。 */
    float fMinWristAboveHead = 0.3f;
    /* 「举手有效」门：哪一种姿态算一次举手。
       0=仅单手（恰好一只手过闸，双手同时举**不算**）1=仅双手（必须两只都过）
       2=单/双手（任意一只过即算，默认——历史行为就是这一档）
       它与 fMinWristAboveHead 是两个正交的闸：后者管"手腕要举多高"，
       前者管"几只算举手"。缺了前者，网页「举手有效」三选一没有落点，
       现场只能去改别的不相关参数去凑。 */
    int32_t nHandMode = 2;
};

/* 背身：以"面部证据是否可见"判定，镜像无关 */
struct TurnBackRule_S
{
    /* 鼻与双眼的得分上限：三者都低于此值说明镜头看到的是后脑勺 */
    float fFaceScore = 0.5f;
    /* 头顶与双肩的最低得分：身体证据也不足时判据失效，必须返回 UNKNOWN */
    float fBodyScore = 0.7f;
    /* 人脸证人 TTL（毫秒，0=关）：FaceDetect 全帧每 face_interval 一轮、独立于姿态，
       本行有新鲜人脸 ⇒ 面部可见，把几何判据的 TRUE 否决成 FALSE。
       动机（§1.90 ttea6 tbf= 直方图）：面点塌陷这一路分不出「背身」与「3/4 侧脸」——
       两侧判定的 max 面分被 0.5 整齐劈开（back 全 <0.5、vis 全 ≥0.5），但讲课侧脸的
       max 面分同样落在 <0.5，单纯调门救不了。否决只做 TRUE→FALSE 单向，UNKNOWN 原样；
       学生路不接 */
    float fFaceWitnessTtlMs = 2000.0f;
    /* 人脸证人的镜像签名否决（T45），0=关（默认，失败安全：新判据不上电）。
       >0 时先看姿态双眼的**布局**：眼Lx>f 且 眼Rx<f（左眼落框右半、右眼落框左半的交叉
       布局）＝后脑幻脸指纹——真背身 36/36 行、真正面 315/315 行在 tt16/tt18/tt19 三跑
       5454 探针行上互斥（正面全落 >0.55，0.45~0.55 是空带），这种布局任何真实人脸都摆
       不出来。指纹在场 ⇒ 与 FaceDetect 同时在场的「新鲜人脸」按幻觉处理，证人不再把
       TRUE 翻成 FALSE（这正是 204/208 全程 0 的成因之一：幻脸替真脸作证）。
       刻意只杀交叉布局、不杀「双眼同侧」（那是真侧脸的形状，证人的本职就是救它——
       §1.90 ttea6 3/4 侧脸假背）。 */
    float fFaceWitnessMirrorRx = 0.0f;
};

/* 趴桌：头顶不高于双肩较高一侧 */
struct LieOnDeskRule_S
{
    float fKeyScore = 0.1f;
};

/* 玩手机：本判据只管"手"的那一半——双手落在肩线附近的带内且屈肘。
   "低头"那一半不在这里重复算，由采样器拿头部一族已消歧的结果来合取
   （见 behavior/student_behavior.cpp）：同一批面部点算两遍，两份读数在边界上会各说各话 */
struct PhoneRule_S
{
    float fKeyScore = 0.8f;
    /* 腕相对双肩连线中点的高度差 ÷ 肩宽，负值在肩线以上。
       手机举在胸前到脸前这一截：再高就是举手/摸头，再低就是手搭在腿上 */
    float fBandAboveShoulder  = 0.8f;
    float fBandBelowShoulder  = 1.2f;
    /* 肩-肘-腕 夹角上限：手收在身前必然屈肘，直臂下垂（手搭腿上）与直臂前伸都不成立 */
    float fMaxElbowAngle = 120.0f;
    /* 两只手都成立时的最大间距（肩宽倍数）：手机是小物件，双手并用必然靠拢；
       只有一侧成立时这一道没有可比的两端，不因此判 UNKNOWN（见 judgePhoneUse 的口径） */
    float fMaxWristGapNorm = 1.5f;
};

/* 朝向（听讲 107 / 讨论 109 / 演示 111 共用一把尺）：把"头转向哪一侧"折到"朝参考点多近"这一根轴上。
   参考点由调用方给（讲台区域 / 邻座头框 / 学生席区域），本函数不认识区域 */
struct FaceDirRule_S
{
    float fKeyScore = 0.5f;   /* 鼻与双眼的最低得分：面都看不清就没有朝向可读 */
    /* 折到参考侧之后的三档：>= 判「朝」、<= 判「背朝」、中间是「不知道」。
       两条必须留出中间带：远焦加模糊时读数本身有 ±0.1 的抖，两档相接会让判定每拍翻一次，
       时长门的中断容忍扛不住反复翻转（它管的是"断了多久"，不是"翻了几次"） */
    float fTrueAbove  = 0.12f;
    float fFalseBelow = -0.12f;
};

/* 一次几何判定所需的关键点集合（指向 CPoseSkeleton_S 内部，不持有所有权） */
struct CGeometryInput
{
    const CPoseSkeleton_S *pstSkeleton = nullptr;
    /* 肩宽基准（像素），用于把绝对距离归一化成比例，抗远焦尺度变化 */
    float fShoulderWidth = 0.0f;
    /* 人头框（分析帧坐标，与骨骼点同一坐标系，见 §4.8），用于区域/尺寸类辅助判据 */
    CRectf_S stHeadBox;
    /* 体框（分析帧坐标），由头框外扩推得（expandHeadToBody） */
    CRectf_S stBodyBox;
};

/* ============================ 基础算子 ============================ */

/* 两点距离 */
float distance2d(const CPointf_S &stA, const CPointf_S &stB);

/**
 * @brief   : 线段 a-b 上取点 p 的垂足
 * @note    : 低头判据里的"眉心"即由鼻子向双眼连线作垂线得到
 */
CPointf_S perpendicularFoot(const CPointf_S &stA, const CPointf_S &stB, const CPointf_S &stP);

/**
 * @brief   : 连线与竖直方向（Y 轴）的夹角，返回 [0,90] 度
 */
float angleWithVertical(const CPointf_S &stA, const CPointf_S &stB);

/**
 * @brief   : 以 stPivot 为顶点，到 stA 与 stB 的夹角，返回 [0,180] 度；两点重合时返回 UNKNOWN 语义所需的 0
 */
float angleAtVertex(const CPointf_S &stPivot, const CPointf_S &stA, const CPointf_S &stB);

/**
 * @brief   : 取关键点，越界或空指针返回 false
 */
bool posePoint(const CPoseSkeleton_S *pstSkeleton, PosePoint_E enPoint, CPointf_S &stOut, float &rfScore);

/* 关键点是否可用（含 score 门限） */
bool posePointUsable(const CPoseSkeleton_S *pstSkeleton, PosePoint_E enPoint, float fScoreThreshold);

/**
 * @brief   : 肩宽（分析帧像素），给需要"绝对距离÷身体尺度"这把尺子的判据用（举手、玩手机）
 * @return   {float} 两侧肩任一不可用（点缺、分低于门限）时返回 0，两种用法各自处理：
 *            举手侧见 0 即跳过那道加强条件（退回与在网一致的默认行为）；
 *            玩手机侧见 0 直接判不出——judgePhoneUse 的带和间距整条都拿这把尺子量，
 *            没有尺就不该读数（返回 0 与 CGeometryInput::fShoulderWidth 的哨兵同一语义）
 * @note    : 学生/老师两路共用这一份（老师机 205 玩手机与学生 106 同形），判据只留一份
 */
float shoulderWidthOf(const CPoseSkeleton_S &stSkeleton, float fScoreThreshold);

/* ============================ 行为判据 ============================ */

/**
 * @brief   : 站立判定
 * @return   {TriState_E} 关键点不足时为 TS_UNKNOWN（不得当作"未站立"参与统计）
 */
TriState_E judgeStanding(const CGeometryInput &stInput, const StandRule_S &stRule);

/**
 * @brief   : 举手判定（左右手任一成立即为真）
 */
TriState_E judgeRaiseHand(const CGeometryInput &stInput, const RaiseHandRule_S &stRule);

/**
 * @brief   : 低头判定
 */
TriState_E judgeHeadDown(const CGeometryInput &stInput, const HeadDownRule_S &stRule);

/**
 * @brief   : 抬头判定
 */
TriState_E judgeHeadUp(const CGeometryInput &stInput, const HeadUpRule_S &stRule);

/**
 * @brief   : 背身判定
 * @note    : 只用"面部证据可见性"，因此天然不受镜像 / 旋转影响，也不需要讲台参考方向；
 *            面部与身体证据同时不足时返回 UNKNOWN，由 CDurationGate 的时间平滑决定归属
 */
TriState_E judgeTurnBack(const CGeometryInput &stInput, const TurnBackRule_S &stRule);

/* ---- T37 背身面部分数直方图（唯一的埋点例外，见 geometry.cpp 定义处的理由） ----
   judgeTurnBack 每走到面部门一次就记一笔：带=头框高 <34px / ≥34px，桶=max 鼻眼分
   0.1 步进 8 桶，组序=背身小脸 / 背身大脸 / 可见小脸 / 可见大脸。
   文本形如 "tbf=a,a,.../a,.../v,.../v,..."（4 组 × 8 个）。 */
void turnbackFaceStatText(char *pszBuf, int32_t nSize);
void turnbackFaceStatReset();

/* T45 镜像签名：姿态双眼交叉布局（眼Lx>fSplit 且 眼Rx<fSplit）＝后脑幻脸指纹。
   关位（fSplit<=0）/眼点缺失/头框退化一律 false；双眼同侧（真侧脸）永不为 true。
   语义与数据见 TurnBackRule_S::fFaceWitnessMirrorRx 注释 */
bool turnbackFaceMirrored(const CGeometryInput &stInput, float fSplit);

/**
 * @brief   : 趴桌判定
 */
TriState_E judgeLieOnDesk(const CGeometryInput &stInput, const LieOnDeskRule_S &stRule);

/**
 * @brief   : 玩手机判定（只管"手"，低头由调用方合取；见 PhoneRule_S）
 * @note    : 只有一侧手臂成立也算成立——另一侧被前排或桌面挡住是课堂常态，
 *            要求双侧会让这条几乎永不满足，而代价是"配了、开了、永远零事件"；
 *            随之而来的已知误报是摸脸/揉眼/托腮（同样是低头 + 屈肘 + 手在带内），
 *            这三者与玩手机的区分是 PlayPhoneDetect 的活，不是本函数的（见 §9）
 * @return   {TriState_E} 两侧手臂都不可用时 TS_UNKNOWN（没有手可看，不等于手不在身前）
 */
TriState_E judgePhoneUse(const CGeometryInput &stInput, const PhoneRule_S &stRule);

/**
 * @brief   : 头部偏航的归一化读数：鼻到「双眼连线中点」的横向偏移 ÷ 肩宽
 * @param    {bool&} rbUsable：OUT=false 表示读不出来（面点分不够、没有肩宽这把尺）
 * @return   {float} 正 = 头转向分析帧 +x 一侧；钳在 [-1,1]
 * @note    : **分母取肩宽而不是眼距**：转头时眼距本身在透视里缩短，快到侧脸时趋近 0，
 *            拿它做除数会把"转向哪儿"放大成"转了多少度都不够"，而符号恰恰是唯一要的数；
 *            肩是躯干的尺度，几乎不参与这个自由度。读数不是角度，别拿 CSV 的 60° 门限直接对
 */
float headYawNorm(const CPoseSkeleton_S *pstSkeleton, float fKeyScore, float fShoulderWidth, bool &rbUsable);

/**
 * @brief   : 「朝参考点」判定：把偏航读数折到参考点所在那一侧，再比三档
 * @param    {float} fRefSign：+1 / -1 = 参考点在这一行的哪一侧；0 = 本拍没有横向参考
 *            （邻座不在距离内、讲台区域没画）⇒ 判 UNKNOWN，不得当成"朝"或"不朝"
 * @note    : **镜像安全**：判据取的是 `headYawNorm() × fRefSign` 这个乘积。水平镜像会把
 *            偏航符号翻一次，同时把参考点相对人的那一侧也翻一次，两次符号翻转互相抵消；
 *            所以参考点必须由同一坐标系里的几何算出来（分析帧区域中心、邻座分析帧头框），
 *            不能由人往配置里填一个"左/右"——那才是镜像机位上整体判反的那一类错误
 *            （与背身改用"面部证据可见性"是同一条理由，见文件头第 3 点）
 */
TriState_E judgeFacingRef(const CGeometryInput &stInput, const FaceDirRule_S &stRule, float fRefSign);

/**
 * @brief   : 由骨骼点估算人体框（头顶到脚踝，左右肩/髋取外包络）
 * @return   {CRectf_S} 可用点太少时返回面积为 0 的框，调用方须判 area()>0
 */
CRectf_S estimateBodyBoxFromSkeleton(const CPoseSkeleton_S *pstSkeleton, float fScoreThreshold);
}
