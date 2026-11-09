/**
 * @FilePath     : classroom_define.h
 * @Author       : leiyy
 * @Date         : 2026-10-23 13:58:00
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-23 13:58:00
 * @Description  : 课堂分析子系统公共类型定义（枚举 / 常量 / 基础结构体
 */

#pragma once

#include <cstdint>
#include <cstddef>

namespace Classroom
{
/* ============================ 编译期常量 ============================ */

/* 课堂内同时跟踪的最大人数：决定各类有界表的容量，禁止无界容器 */
#define CLASSROOM_MAX_PERSONS (60)

/* FastPose 输出关键点数，与模型输出 [1,26,3] 一致 */
#define CLASSROOM_POSE_KEYPOINT_NUM (26)

/* 关键点置信度低于该门限则不参与几何判定 */
#define CLASSROOM_POSE_SCORE_THRESHOLD (0.3f)

/* 帧总线槽位数：1080p NV12 ≈ 3.11MB × 4 ≈ 12.4MB，为内存预算里的固定项 */
#define CLASSROOM_FRAME_BUS_SLOTS (4)

/* 申请空闲帧的最长等待（毫秒），超时按丢帧处理，绝不无限阻塞 */
#define CLASSROOM_FRAME_ACQUIRE_TIMEOUT_MS (200)

/* 一拍里允许给姿态花多少毫秒 */
#define CLASSROOM_POSE_BEAT_BUDGET_MS (140)

/* 模型组空闲多久后卸载（毫秒） */
#define CLASSROOM_MODEL_IDLE_UNLOAD_MS (8000)

/* 加载失败后多久之内不再重试（毫秒）*/
#define CLASSROOM_MODEL_LOAD_FAIL_COOLDOWN_MS (30000)

/* 参与考勤的最小人脸边长（像素），对齐企划 PPT 指标 */
#define CLASSROOM_MIN_FACE_PIXEL (120)

/* 人脸检测框的短边下限（分析帧像素），比上一行低一个量级，且两者判的不是同一件事 */
#define CLASSROOM_MIN_FACE_DETECT_PIXEL (32)

/* 检测器置信度门限：同时是送后端的 NMS 前置门与解析后的复检门（一处配，两端用） */
#define CLASSROOM_DETECT_SCORE_THRESHOLD (0.30f)

/* NMS 门限：课堂里人头天然密集（前后排重叠），比通用检测默认值 0.45 收紧到 0.40 */
#define CLASSROOM_DETECT_NMS_THRESHOLD (0.40f)

/* 小于该边长的人头框直接丢：1080p 远排人头约 18~26px，再小的框基本都是误检 */
#define CLASSROOM_MIN_HEAD_PIXEL (14)

/* 远带人头（#90）：画布短边 < 60px 的人头框用 基门×0.6 的折扣门放行 */
#define CLASSROOM_HEAD_FAR_EDGE_PX (60)
#define CLASSROOM_HEAD_FAR_GATE_RATIO (0.6f)

/* 人体框最小边长：人体检测在远处退化成「半个人」，宁可让该行回落成头框外扩 */
#define CLASSROOM_MIN_BODY_PIXEL (40)

/* 人头框落在人体框内的面积占比低于此值，认为这两个框不是同一个人 */
#define CLASSROOM_BODY_HEAD_CONTAIN (0.60f)

/* 人头框 → 人体框外扩倍数 */
#define CLASSROOM_BODY_EXPAND_W_HEAD (3.0f)
#define CLASSROOM_BODY_EXPAND_DOWN_HEAD (7.0f)
#define CLASSROOM_BODY_EXPAND_UP_HEAD (0.5f)

/* ---- 以下四项是配置默认值---- */

/* 邻框 IoU 超此值即标「被遮挡」：课堂前后排人头天然错开，真重叠到 0.3 基本只剩
   「一个人的框压在另一个人的框上」这一种情形。规则层据此把朝向/姿态类判定降级为 UNKNOWN */
#define CLASSROOM_OCCLUDE_IOU (0.30f)

/* 框边到画面边的距离小于此值即算「贴边被切」：按 1080p 上人头 ~20px 的一半取 8px，
   再小就是纯抖动了（跟踪框平滑后每帧动 1~2px） */
#define CLASSROOM_EDGE_BAND_PX (8)

/* 清晰度代理的满刻度 */
#define CLASSROOM_BLUR_FULL_GRAD (16.0f)

/* 姿态判定可用的清晰度下限（对 fBlurScore 这一 [0,1] 归一量取门限） */
#define CLASSROOM_POSE_BLUR_MIN (0.10f)

/* ==================== 姿态判据距离分档 ==================== */

/* 头高（分析帧头框高，像素）≥ 此值：不折扣（近带维持原门） */
#define CLASSROOM_POSE_FAR_HEAD_PX (45.0f)
/* 头高 ≤ 此值：折扣到满档（fBase × CLASSROOM_POSE_FAR_KEY_RATIO）；两点之间线性过渡 */
#define CLASSROOM_POSE_NEAR_HEAD_PX (25.0f)
/* 满档折扣系数：key_score 0.8 → 远带 0.48。与 #90 人头折扣 0.6 同量级 */
#define CLASSROOM_POSE_FAR_KEY_RATIO (0.6f)
/* blur 门按头高缩放的满档头高：门 = fPoseBlurMin × min(1, 头高/此值)。
   头框内梯度能量随框面积缩，小头天然低分——不缩放则远带整排被 blur 闸吃掉 */
#define CLASSROOM_POSE_BLUR_SCALE_HEAD_PX (34.0f)
/* 双眼距低于此值（像素）：头部点不可信——几个像素里眼距的读数误差与信号同量级，
   103/104 角度类在此尺度上出 UNKNOWN（噪声不能当证据）*/
#define CLASSROOM_POSE_MIN_EYE_GAP_PX (6.0f)

/* 关键点分数门按头高折扣。几何判据读点统一走本函数，规则层不得各处自己乘系数。
   头高非法（<=0，框没画出来）与远带之外都原样返回：折扣只给"量到了且确实小"的头，
   量不到尺度时收门（维持原判据）比放门保守 */
inline float poseKeyScoreForHeadPx(float fBase, float fHeadPx)
{
    if (fHeadPx <= 0.0f || fHeadPx >= CLASSROOM_POSE_FAR_HEAD_PX)
    {
        return fBase;
    }
    if (fHeadPx <= CLASSROOM_POSE_NEAR_HEAD_PX)
    {
        return fBase * CLASSROOM_POSE_FAR_KEY_RATIO;
    }
    const float fT = (fHeadPx - CLASSROOM_POSE_NEAR_HEAD_PX) /
                     (CLASSROOM_POSE_FAR_HEAD_PX - CLASSROOM_POSE_NEAR_HEAD_PX);
    return fBase * (CLASSROOM_POSE_FAR_KEY_RATIO +
                    (1.0f - CLASSROOM_POSE_FAR_KEY_RATIO) * fT);
}

/* blur 门按头高缩放：采样侧（selectPoseTargets）与规则侧（student/teacher/arbiter）
   必须同走本函数——两侧各写一份就会出现"感知层采了、规则层扔了"的错位。
   头高非法或到满档头高时原样返回；fBase=0（配置关闸）缩放后仍为 0 */
inline float poseBlurMinForHeadPx(float fBase, float fHeadPx)
{
    if (fHeadPx <= 0.0f || fHeadPx >= CLASSROOM_POSE_BLUR_SCALE_HEAD_PX)
    {
        return fBase;
    }
    return fBase * (fHeadPx / CLASSROOM_POSE_BLUR_SCALE_HEAD_PX);
}

/* 一拍里等一次推理结果的上限（毫秒） */
#define CLASSROOM_INFER_WAIT_MS (180)

/* 207 停留计时在「这一行本拍没出现」后的宽限（毫秒） */
#define CLASSROOM_STAY_GAP_MS (1000)

/* 「最近还见过主体」的记忆窗（毫秒），给 210 巡视当证据前提 */
#define CLASSROOM_SUBJECT_MEMORY_MS (30000)

/* 一次玩手机复核结论的新鲜度窗口（毫秒）*/
#define CLASSROOM_PHONE_TTL_MS (3000)

/* 一次课堂会话的时长上限（秒），默认 6h */
#define CLASSROOM_SESSION_MAX_SEC (21600)

/* 会话静默收尾（秒），默认 600（10 分钟），0=关 */
#define CLASSROOM_SESSION_SILENCE_SEC (600)

/* 模型池预留内存 pool.reserve_kb 的上限 */
#define CLASSROOM_POOL_RESERVE_MAX_KB (4 * 1024 * 1024)

/* 二进制构建号（心跳 bin= 列） */
#define CLASSROOM_BIN_BUILD __DATE__ " " __TIME__

/* 事件原文日志的限流 */
#define CLASSROOM_EVT_LOG_WIN_MS (60000)
#define CLASSROOM_EVT_LOG_MAX (30)

/* 检测后端打平输出的框长 */
#define CLASSROOM_BOX_FLOAT_NUM (6)

/* 人脸关键点数。写死 5 而不是可配 */
#define CLASSROOM_FACE_LANDMARK_NUM (5)

/* 人脸打平输出的一行长度 */
#define CLASSROOM_FACE_BOX_FLOAT_NUM (CLASSROOM_BOX_FLOAT_NUM + CLASSROOM_FACE_LANDMARK_NUM * 3)

/* 姿态打平输出的「每个关键点几个 float」 */
#define CLASSROOM_POSE_POINT_FLOAT_NUM (3)

/* 一副骨骼「可用」的下限 */
#define CLASSROOM_POSE_MIN_TRUNK_POINTS (3)

/* ---- 模型键：配置默认表（classroom_config.cpp）与感知层查表用的是同一个名字 ----
   必须在公共头单点定义：键写两遍而拼错其中一处时，池子里那行永远查不到，
   现象是"模型在、日志说加载成功、这一路永远 0 框"——没有任何一处会报错 */
#define CLASSROOM_MODEL_KEY_HEAD ("HeadDetect")
#define CLASSROOM_MODEL_KEY_POSE ("FastPose")
/* 五点对齐的点**由这颗检测器一起给出** */
#define CLASSROOM_MODEL_KEY_FACE ("FaceDetect")
#define CLASSROOM_MODEL_KEY_EXPRESSION ("FaceExpression")
/* 512 维人脸特征（考勤比对的输出），键名与 CAttendParam_S 那侧的维度常量配对 */
#define CLASSROOM_MODEL_KEY_FACE_FEATURE ("FaceFeature")
#define CLASSROOM_MODEL_KEY_PHONE ("PlayPhoneDetect")
/* 板书 OCR 的两颗：det 出「哪几块是文字」，rec 出「这一条线上写的是什么」 */
#define CLASSROOM_MODEL_KEY_OCR_DET ("OcrDet")
#define CLASSROOM_MODEL_KEY_OCR_REC ("OcrRec")

/* 键区的末标记：不指代任何一颗模型，只把「上面这一段是全部内置键」这句话变成一个可解析的事实 */
#define CLASSROOM_MODEL_KEYS_LAST CLASSROOM_MODEL_KEY_OCR_REC

/* ============================ 枚举 ============================ */

/**
 * @brief   : 模型后端可用状态
 */
typedef enum class BackendState : int
{
    READY = 0,       /* 正常可用 */
    MODEL_MISSING,   /* 模型或配置未在机上找到*/
    OUT_OF_BUDGET,   /* 驻留内存准入被拒：只有这一条是"整机装不下"的证据 */
    LOAD_FAILED,     /* rknn 加载/建输入输出失败 */
    DISABLED,        /* 能力位或配置显式关闭 */
    /* 已注册、未驻留：还没人 acquire 过，或上一次租约已放掉。
       这个值单列是因为混用过：它曾与 MODEL_MISSING 同值 */
    UNLOADED,
    /* 内存不够，而且这会儿也腾不出地方 */
    GROUP_BUSY,
    CAPTURE_MISS,    /* 时机到了但这一拍交不出一张可识别的板面图（没有帧、裁剪档没建、
                        RGA 拒、或装法带镜像/旋转而这条链路还不处理像素翻转） */
    BACKEND_BUSY,    /* 图有了但后端排不上（信箱那一格还占着，或上一张的作业超时没回结果） */
} BackendState_E;

/**
 * @brief   : 模型共存分组
 */
typedef enum class ModelGroup : int
{
    GROUP_BASE = 0, /* 常驻：人头 + 人体，所有功能的底座 */
    GROUP_POSE,     /* 按需：FastPose，服务站立/举手/抬头/低头/背身/教师姿态 */
    GROUP_FACE,     /* 按需：人脸检测/质量/特征/表情，服务考勤与情绪 */
    GROUP_OTHER,    /* 按需：玩手机、OCR 等未归组模型 */
} ModelGroup_E;

/**
 * @brief   : 内存守护档位
 */
typedef enum class GuardLevel : int
{
    GL_NORMAL = 0,    /* avail >  reserve×1.20：全速 */
    GL_SOFT = 1,      /* avail ≤  reserve×1.20：拉长采样周期，先降频再动模型 */
    GL_HARD = 2,      /* avail ≤  reserve×1.05：卸载非必需组（OTHER/FACE），保 BASE+POSE */
    GL_EMERGENCY = 3, /* avail <  reserve      ：放掉全部租约并 unloadAll，只留帧总线，置 degraded */
} GuardLevel_E;

/**
 * @brief   : 分析主体类别
 */
typedef enum class Subject : int
{
    SUBJECT_UNKNOWN = 0,
    SUBJECT_STUDENT = 1, /* 学生 */
    SUBJECT_TEACHER = 2, /* 教师 */
} Subject_E;

/**
 * @brief   : 教师所处区域
 */
typedef enum class TeacherArea : int
{
    TEA_AREA_UNKNOWN = -1, /* 无教师 / 区域事实不可用（未画区域、无槽位） */
    TEA_AREA_PODIUM = 0,   /* 讲台区 */
    TEA_AREA_BOARD = 1,    /* 黑板区 */
    TEA_AREA_STUDENT = 2,  /* 学生席 */
    TEA_AREA_OTHER = 3,    /* 教室里的别处（讲台、黑板、学生席之外） */
} TeacherArea_E;

/**
 * @brief   : 一行检测来自哪个检测器
 */
typedef enum class DetectSource : int
{
    DET_SRC_HEAD = 0,     /* 人头检测 */
    DET_SRC_PHONE = 1,    /* 玩手机复核（PlayPhoneDetect，只喂 106 的复核，不参与任何框的降权） */
} DetectSource_E;

/**
 * @brief   : 课堂行为类型（子系统内部编号）
 */
typedef enum class Behavior : int
{
    /* ---------- 学生行为 ---------- */
    STU_STAND_UP = 101,   /* 站立 */
    STU_RAISE_HAND = 102, /* 举手 */
    STU_HEAD_UP = 103,    /* 抬头 */
    STU_HEAD_DOWN = 104,  /* 低头 */
    STU_TURN_BACK = 105,  /* 背身 */
    STU_PHONE = 106,      /* 玩手机 */
    STU_LISTEN = 107,     /* 听讲 */
    STU_READING = 108,    /* 阅读 */
    STU_DISCUSS = 109,    /* 讨论 */
    STU_ON_PODIUM = 110,  /* 上讲台 */
    STU_PRESENT = 111,    /* 演示 */
    STU_EXPRESSION = 112, /* 表情识别 */

    /* ---------- 教师行为 ---------- */
    TEA_STAND_UP = 201,    /* 站立 */
    TEA_HEAD_UP = 202,     /* 抬头 */
    TEA_HEAD_DOWN = 203,   /* 低头 */
    TEA_TURN_BACK = 204,   /* 背身 */
    TEA_PHONE = 205,       /* 玩手机 */
    TEA_TEACHING = 206,    /* 讲授 */
    TEA_GUIDING = 207,     /* 指导 */
    TEA_BOARD_WRITE = 208, /* 板书 */
    TEA_INTERACT = 209,    /* 互动 */
    TEA_PATROL = 210,      /* 巡视 */
    TEA_ENTER_PODIUM = 211,/* 进入讲台 */
    TEA_TRACK = 212,       /* 教师跟踪 */
    TEA_IDENTITY = 213,    /* 身份确认 */
} Behavior_E;

/**
 * @brief   : 表情枚举（课堂情绪曲线的原子类别）
 */
typedef enum class Expression : int
{
    EXP_UNKNOWN = -1,
    EXP_NEUTRAL = 0, /* 平静 */
    EXP_HAPPY = 1,   /* 开心 */
    EXP_SURPRISE = 2,/* 惊讶 */
    EXP_SAD = 3,     /* 悲伤 */
    EXP_ANGRY = 4,   /* 愤怒 */
    EXP_FEAR = 5,    /* 恐惧 */
    EXP_DISGUST = 6, /* 厌恶 */
} Expression_E;

/* 情绪类别的个数（EXP_NEUTRAL..EXP_DISGUST，**不含** EXP_UNKNOWN） */
constexpr int CLASSROOM_EXPRESSION_CLASS_NUM = static_cast<int>(Expression_E::EXP_DISGUST) + 1;

/**
 * @brief   : 三态判定结果
 */
typedef enum class TriState : int
{
    TS_UNKNOWN = -1, /* 证据不足，不可判定 */
    TS_FALSE = 0,    /* 判定为否 */
    TS_TRUE = 1,     /* 判定为是 */
} TriState_E;

/**
 * @brief   : 镜像模式
 */
typedef enum class MirrorMode : int
{
    MIRROR_NONE = 0,  /* 不处理 */
    MIRROR_H_FLIP = 1,/* 水平镜像：x → W-1-x */
    MIRROR_V_FLIP = 2,/* 垂直镜像：y → H-1-y */
    MIRROR_BOTH = 3,  /* 水平 + 垂直（等价于旋转 180°） */
} MirrorMode_E;

/**
 * @brief   : 设备角色（#126：教师机/学生机互斥。平台按角色下发顶层配置键 "role"，
 *            配置层在 validate() 里把不属于本角色的功能**强制关**——role 赢过文件里
 *            任何单项 enable；只关不开，开哪路仍由现场逐项配）
 */
typedef enum class DeviceRole : int
{
    ROLE_TEACHER = 0, /* 教师机：板书 OCR 独占（按需加载已有）；考勤与学生行为被强制关 */
    ROLE_STUDENT = 1, /* 学生机：考勤独占；板书 OCR、教师身份与教师行为被强制关 */
    ROLE_BOTH = 2,    /* 未分档（默认）：维持现状，全部功能可用 */
} DeviceRole_E;

/**
 * @brief   : Web/平台下发区域坐标的取值空间
 */
typedef enum class CoordSpace : int
{
    COORD_SPACE_PIXEL = 0,   /* 绝对像素，基准为下发时约定的参考分辨率 */
    COORD_SPACE_NORM_8192 = 1,/* 0~8191 归一化，现网 nCode 划线惯用口径 */
} CoordSpace_E;

/**
 * @brief   : 送模型图像的通道顺序
 */
typedef enum class ChannelOrder : int
{
    CH_RGB = 0, /* RGA 输出 RK_FORMAT_RGB_888 */
    CH_BGR = 1, /* RGA 输出 RK_FORMAT_BGR_888 */
} ChannelOrder_E;

/* ============================ 基础结构体 ============================ */

/**
 * @brief 浮点像素坐标点
 */
struct CPointf_S
{
    float fX = 0.0f;
    float fY = 0.0f;
};

/**
 * @brief 浮点矩形框（左上/右下，开区间不做要求，允许 w/h 为 0）
 */
struct CRectf_S
{
    float fX1 = 0.0f;
    float fY1 = 0.0f;
    float fX2 = 0.0f;
    float fY2 = 0.0f;

    float width() const { return fX2 - fX1; }
    float height() const { return fY2 - fY1; }
    float area() const { return (width() > 0.0f && height() > 0.0f) ? width() * height() : 0.0f; }
    CPointf_S center() const { return CPointf_S{ (fX1 + fX2) * 0.5f, (fY1 + fY2) * 0.5f }; }
};

/**
 * @brief 检测器的一次原始输出（跟踪器的输入）
 */
typedef struct _Detection_
{
    CRectf_S  stBox;
    float     fScore = 0.0f;
    int32_t   nLabel = 0;       /* 模型内类别号；人头/人体这类单类检测恒为 0 */
    DetectSource_E enSource = DetectSource_E::DET_SRC_HEAD; /* 这一行是谁给的（人体框优先于外扩框） */
} CDetection_S;

/**
 * @brief 单个骨骼关键点
 */
struct CPosePoint_S
{
    float fX = 0.0f;     /* 横坐标，坐标系见 CPoseSkeleton_S */
    float fY = 0.0f;     /* 纵坐标，坐标系见 CPoseSkeleton_S */
    float fScore = 0.0f; /* 置信度 0~1 */

    /* 是否可用于几何判定 */
    bool isValid(float fScoreThreshold = CLASSROOM_POSE_SCORE_THRESHOLD) const
    {
        return fScore >= fScoreThreshold;
    }
};

/**
 * @brief   FastPose 26 点拓扑
 */
typedef enum class PosePoint : int
{
    POSE_NOSE = 0,       /* 鼻子 */
    POSE_R_EYE = 1,      /* 右眼 */
    POSE_L_EYE = 2,      /* 左眼 */
    POSE_R_EAR = 3,      /* 右耳 */
    POSE_L_EAR = 4,      /* 左耳 */
    POSE_R_SHOULDER = 5, /* 右肩 */
    POSE_L_SHOULDER = 6, /* 左肩 */
    POSE_R_ELBOW = 7,    /* 右手肘 */
    POSE_L_ELBOW = 8,    /* 左手肘 */
    POSE_R_WRIST = 9,    /* 右手腕 */
    POSE_L_WRIST = 10,   /* 左手腕 */
    POSE_R_HIP = 11,     /* 右臀 */
    POSE_L_HIP = 12,     /* 左臀 */
    POSE_R_KNEE = 13,    /* 右膝盖 */
    POSE_L_KNEE = 14,    /* 左膝盖 */
    POSE_R_ANKLE = 15,   /* 右脚踝 */
    POSE_L_ANKLE = 16,   /* 左脚踝 */
    POSE_HEAD_TOP = 17,  /* 头顶 */
    POSE_NECK = 18,      /* 颈部 */
    POSE_HIP = 19,       /* 臀部（左右臀中点语义由模型给出） */
    POSE_RESERVED_20 = 20,
    POSE_RESERVED_21 = 21,
    POSE_RESERVED_22 = 22,
    POSE_RESERVED_23 = 23,
    POSE_RESERVED_24 = 24,
    POSE_RESERVED_25 = 25,
} PosePoint_E;

/**
 * @brief 骨骼点集合：定长数组，避免逐帧堆分配
 */
struct CPoseSkeleton_S
{
    CPosePoint_S astPoints[CLASSROOM_POSE_KEYPOINT_NUM];
    int   nPersonId = -1;   /* 关联的跟踪 ID */
    float fScore = 0.0f;    /* 整体可信度（各点均值） */
    bool  bValid = false;   /* 模型未就绪或推理失败时为 false，上层不得据此改状态 */
};

/* ============================ 内联小工具 ============================ */

/**
 * @brief   : 矩形与矩形求交面积
 */
inline float rectIntersectionArea(const CRectf_S &stA, const CRectf_S &stB)
{
    float fX1 = (stA.fX1 > stB.fX1) ? stA.fX1 : stB.fX1;
    float fY1 = (stA.fY1 > stB.fY1) ? stA.fY1 : stB.fY1;
    float fX2 = (stA.fX2 < stB.fX2) ? stA.fX2 : stB.fX2;
    float fY2 = (stA.fY2 < stB.fY2) ? stA.fY2 : stB.fY2;

    if (fX2 <= fX1 || fY2 <= fY1)
    {
        return 0.0f;
    }
    return (fX2 - fX1) * (fY2 - fY1);
}

/**
 * @brief   : 矩形 IoU
 */
inline float rectIoU(const CRectf_S &stA, const CRectf_S &stB)
{
    float fInter = rectIntersectionArea(stA, stB);
    float fUnion = stA.area() + stB.area() - fInter;

    if (fUnion <= 0.0f)
    {
        return 0.0f;
    }
    return fInter / fUnion;
}

/**
 * @brief   : 数值钳制
 */
template <typename T>
inline T clampValue(T tValue, T tMin, T tMax)
{
    if (tValue < tMin)
    {
        return tMin;
    }
    if (tValue > tMax)
    {
        return tMax;
    }
    return tValue;
}
}
