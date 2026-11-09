/**
 * @FilePath     : classroom_config.cpp
 * @Author       : leiyy
 * @Date         : 2026-11-03 11:28:37
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-11-03 11:28:37
 * @Description  : 课堂配置解析 / 校验 / 快照换入实现
 */

#include "classroom_config.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <unistd.h>

#include "Json.h"
#include "classroom_clock.h"
#include "classroom_warm_policy.h"    /* 时间片的上界由复议窗口推出，见那个头里的 CLASSROOM_WARM_SLICE_MAX_MS */
#include "dlog.h"
#include "duration_gate.h"
#include "path_define.h"

namespace Classroom
{
namespace
{
/* 配置文件尺寸上限：正常配置约 6KB，给足余量但不能让一个坏文件吃进内存 */
const long CONFIG_FILE_MAX_BYTES = 256 * 1024;

/* 时长门默认参数表 */
struct GateDefault_S
{
    Behavior_E enBehavior;
    const char *pszName;
    bool       bEnable;
    int        nSampleIntervalMs;
    int        nMinDurationMs;
    int        nGapToleranceMs;
    int        nMinTrueCount;
};

const GateDefault_S AST_GATE_DEFAULTS[] = {
    /* ---------- 学生：姿态类走 Pose 组，采样节拍按 ---------- */
    { Behavior_E::STU_STAND_UP,     "stu_stand_up",     true,  200, 1500,  500, 2 }, /* 站立是点名级事件，宁可慢半拍也不闪报 */
    /* 举手/抬头的出厂值都取量程下限 1s */
    { Behavior_E::STU_RAISE_HAND,   "stu_raise_hand",   true,  200, 1000,  400, 2 }, 
    { Behavior_E::STU_HEAD_UP,      "stu_head_up",      true, 1000, 1500,  800, 1 },
    { Behavior_E::STU_HEAD_DOWN,    "stu_head_down",    true,  500, 3000, 1000, 1 }, /* 低头：翻书/掉笔都算，门槛拉长到 3s */
    { Behavior_E::STU_TURN_BACK,    "stu_turn_back",    true,  500, 3000, 1000, 1 },
    { Behavior_E::STU_PHONE,        "stu_phone",        true,   500, 3000, 1500, 2 },
    { Behavior_E::STU_LISTEN,       "stu_listen",       true,  1000, 5000, 2000, 1 }, /* 听讲/阅读/讨论是群体口径，单人也记 */
    { Behavior_E::STU_READING,      "stu_reading",      true,  1000, 5000, 2000, 1 },
    { Behavior_E::STU_DISCUSS,      "stu_discuss",      true,  1000, 5000, 2000, 1 },
    { Behavior_E::STU_ON_PODIUM,    "stu_on_podium",    true,  1000, 2000, 1000, 1 },
    { Behavior_E::STU_PRESENT,      "stu_present",      false, 1000, 3000, 1500, 1 },
    { Behavior_E::STU_EXPRESSION,   "stu_expression",   true,  2000,    0, 1000, 1 },

    /* ---------- 教师 ---------- */
    { Behavior_E::TEA_STAND_UP,     "tea_stand_up",     true,  200, 1500,  500, 2 },
    { Behavior_E::TEA_HEAD_UP,      "tea_head_up",      true, 1000, 1500,  800, 1 },
    { Behavior_E::TEA_HEAD_DOWN,    "tea_head_down",    true,  500, 3000, 1000, 1 },
    { Behavior_E::TEA_TURN_BACK,    "tea_turn_back",    true,  500, 2000, 1000, 1 }, /* 教师背身多半在板书，与 BOARD_WRITE 天然重叠 */
    { Behavior_E::TEA_PHONE,        "tea_phone",        false, 500, 5000, 2000, 2 },
    { Behavior_E::TEA_TEACHING,     "tea_teaching",     true,  500, 5000, 2000, 1 },
    { Behavior_E::TEA_GUIDING,      "tea_guiding",      true, 1000, 5000, 2000, 1 },
    { Behavior_E::TEA_BOARD_WRITE,  "tea_board_write",  true,  500, 3000, 1500, 1 },
    { Behavior_E::TEA_INTERACT,     "tea_interact",     true, 1000, 5000, 2000, 1 },
    { Behavior_E::TEA_PATROL,       "tea_patrol",       true, 1000, 5000, 2000, 1 },
    { Behavior_E::TEA_ENTER_PODIUM, "tea_enter_podium", true,  500, 2000, 1000, 1 },
    { Behavior_E::TEA_TRACK,        "tea_track",        true,  200,    0,    0, 1 }, /* 跟踪是持续态而非事件态，不设时长门 */
    { Behavior_E::TEA_IDENTITY,     "tea_identity",     false, 2000, 1000, 1000, 1 }, /* 依赖人脸底库，之前默认关 */
};

const int GATE_DEFAULT_NUM = static_cast<int>(sizeof(AST_GATE_DEFAULTS) / sizeof(AST_GATE_DEFAULTS[0]));

/* 内置默认模型表 */
struct ModelRow_S
{
    const char   *pszKey;
    const char   *pszJson;
    const char   *pszModel;
    ModelGroup_E  enGroup;
    int64_t       llResidentKb; /* 0 = 未上机实测，池子按保守上限记账 */
    uint32_t      u32W;
    uint32_t      u32H;
    const char   *pszBackend;
};
const ModelRow_S AST_MODEL_DEFAULTS[] = {
    { CLASSROOM_MODEL_KEY_HEAD,  "/opt/cam/model/HeadDetect.json",
      "/opt/cam/model/RV1126B_HeadDetect_yolov5s_640x640_v1.0.1.rknn",
      ModelGroup_E::GROUP_BASE,  16800, 640, 640, CLASSROOM_BACKEND_YOLOV5 },
    { CLASSROOM_MODEL_KEY_POSE,    "/opt/cam/model/FastPose.json",
      "/opt/cam/model/RV1126B_FastPose_Feature_640x640_v1.0.1.rknn",
      ModelGroup_E::GROUP_POSE,  99328, 192, 256, CLASSROOM_BACKEND_RAW },
    { CLASSROOM_MODEL_KEY_FACE,  "/opt/cam/model/FaceDetect.json",
      "/opt/cam/model/RV1126B_FaceDetect_yolov8n_640x640_V2_0.rknn",
      ModelGroup_E::GROUP_FACE,  13000, 640, 640, CLASSROOM_BACKEND_YOLOV8_POINT },
    { CLASSROOM_MODEL_KEY_EXPRESSION, "/opt/cam/model/FaceExpression.json",
      "/opt/cam/model/FaceExpression_rv1126b.rknn",
      ModelGroup_E::GROUP_FACE,   6000, 112, 112, CLASSROOM_BACKEND_RAW },
    { CLASSROOM_MODEL_KEY_FACE_FEATURE, "/opt/cam/model/FaceFeature.json",
      "/opt/cam/model/RV1126B_FaceFeature_160x160_v2.0.1.rknn",
      ModelGroup_E::GROUP_FACE,  15000, 112, 112, CLASSROOM_BACKEND_RAW },
    { CLASSROOM_MODEL_KEY_PHONE, "/opt/cam/model/PlayPhoneDetect.json",
      "/opt/cam/model/RV1126B_PlayPhoneDetect_yolov8n_640x384_v1.0.1.rknn",
      ModelGroup_E::GROUP_OTHER,  3676, 640, 384, CLASSROOM_BACKEND_YOLOV8 },
    { CLASSROOM_MODEL_KEY_OCR_DET, "",
      "/opt/cam/model/RV1126B_PPOcrt_Detect960x960_V6_0.rknn",
      ModelGroup_E::GROUP_OTHER, 133684, 960, 960, CLASSROOM_BACKEND_RAW },
    { CLASSROOM_MODEL_KEY_OCR_REC, "",
      "/opt/cam/model/RV1126B_PPOcr_Rec48x320_V6.0.rknn",
      ModelGroup_E::GROUP_OTHER, 18000, 320,  48, CLASSROOM_BACKEND_RAW },
};
const int MODEL_DEFAULT_NUM = static_cast<int>(sizeof(AST_MODEL_DEFAULTS) / sizeof(AST_MODEL_DEFAULTS[0]));

/* ---------- 类型严格的 JSON 读取器：读不到返回 false 并保留调用方传入的默认值 ---------- */

/*  逐键点名：类型不符不能只报数量——「改了没生效」的排查要从日志/读数直接看到键名 */
static const char *s_pszSec = "";
static char s_szLastBadKey[96] = "";

static void noteSection(const char *pszSec)
{
    s_pszSec = pszSec;
}

static void noteBadKey(const char *pszKey)
{
    snprintf(s_szLastBadKey, sizeof(s_szLastBadKey), "%s%s%s",
             s_pszSec, ('\0' != s_pszSec[0]) ? "." : "", pszKey);
    dlog_warn("classroom_config: 键类型不符 [%s] %s，保留默认", s_pszSec, pszKey);
}

Json::Object *jsonItem(Json::Object *pParent, const char *pszKey)
{
    if (nullptr == pParent || nullptr == pszKey)
    {
        return nullptr;
    }
    return cJSON_GetObjectItem(pParent, pszKey);
}

bool jsonReadInt(Json::Object *pParent, const char *pszKey, int32_t &rnOut, int32_t &rnBadType)
{
    Json::Object *pItem = jsonItem(pParent, pszKey);
    if (nullptr == pItem)
    {
        return false;
    }
    if (!cJSON_IsNumber(pItem))
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }
    const double rdValue = pItem->valuedouble;
    if (rdValue > static_cast<double>(std::numeric_limits<int32_t>::max()) ||
        rdValue < static_cast<double>(std::numeric_limits<int32_t>::min()))
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }
    rnOut = static_cast<int32_t>(std::lround(rdValue));
    return true;
}

bool jsonReadInt64(Json::Object *pParent, const char *pszKey, int64_t &rllOut, int32_t &rnBadType)
{
    Json::Object *pItem = jsonItem(pParent, pszKey);
    if (nullptr == pItem)
    {
        return false;
    }
    if (!cJSON_IsNumber(pItem))
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }
    const double rdValue = pItem->valuedouble;
    if (rdValue > 9.0e18 || rdValue < -9.0e18)
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }
    rllOut = static_cast<int64_t>(std::llround(rdValue));
    return true;
}

/* 浮点：允许写成 1 / 0 这类整数（cJSON 都是 Number），拒绝字符串 */
bool jsonReadFloat(Json::Object *pParent, const char *pszKey, float &rfOut, int32_t &rnBadType)
{
    Json::Object *pItem = jsonItem(pParent, pszKey);
    if (nullptr == pItem)
    {
        return false;
    }
    if (!cJSON_IsNumber(pItem))
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }
    rfOut = static_cast<float>(pItem->valuedouble);
    return true;
}

/* 布尔：同时接受 true/false 与 1/0 —— 现网下发侧两种写法都有 */
bool jsonReadBool(Json::Object *pParent, const char *pszKey, bool &rbOut, int32_t &rnBadType)
{
    Json::Object *pItem = jsonItem(pParent, pszKey);
    if (nullptr == pItem)
    {
        return false;
    }
    if (cJSON_IsBool(pItem))
    {
        rbOut = cJSON_IsTrue(pItem);
        return true;
    }
    if (cJSON_IsNumber(pItem))
    {
        rbOut = (pItem->valuedouble != 0.0);
        return true;
    }
    ++rnBadType;
        noteBadKey(pszKey);
    return false;
}

bool jsonReadString(Json::Object *pParent, const char *pszKey, std::string &rstrOut, int32_t &rnBadType)
{
    Json::Object *pItem = jsonItem(pParent, pszKey);
    if (nullptr == pItem)
    {
        return false;
    }
    if (!cJSON_IsString(pItem) || nullptr == pItem->valuestring)
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }
    rstrOut = pItem->valuestring;
    return true;
}

/* 区域 id 列表：整表替换语义（给了数组就以此为准），元素类型不对逐个跳过并计数。
   长度不在这里钳制 —— 超上限由 validate() 整份拒绝：截断一个 id 列表等于悄悄改掉
   这条行为关心的区域集合，比拒一次配置难查得多 */
bool jsonReadStringArray(Json::Object *pParent, const char *pszKey,
                         std::vector<std::string> &rvecOut, int32_t &rnBadType)
{
    Json::Object *pArr = jsonItem(pParent, pszKey);
    if (nullptr == pArr)
    {
        return false;
    }
    if (!cJSON_IsArray(pArr))
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }

    rvecOut.clear();
    const int nCount = cJSON_GetArraySize(pArr);
    for (int i = 0; i < nCount; ++i)
    {
        Json::Object *pEle = cJSON_GetArrayItem(pArr, i);
        if (nullptr == pEle || !cJSON_IsString(pEle) || nullptr == pEle->valuestring)
        {
            ++rnBadType;
            noteBadKey(pszKey);
            continue;
        }
        rvecOut.push_back(std::string(pEle->valuestring));
    }
    return true;
}

bool jsonReadIdArray(Json::Object *pParent, const char *pszKey, std::vector<int32_t> &rvecOut,
                     int32_t &rnBadType)
{
    Json::Object *pArr = jsonItem(pParent, pszKey);
    if (nullptr == pArr)
    {
        return false;
    }
    if (!cJSON_IsArray(pArr))
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }

    rvecOut.clear();
    const int nCount = cJSON_GetArraySize(pArr);
    for (int i = 0; i < nCount; ++i)
    {
        Json::Object *pEle = cJSON_GetArrayItem(pArr, i);
        if (nullptr == pEle || !cJSON_IsNumber(pEle))
        {
            ++rnBadType;
        noteBadKey(pszKey);
            continue;
        }
        rvecOut.push_back(static_cast<int32_t>(std::lround(pEle->valuedouble)));
    }
    return true;
}

/* 多边形顶点，JSON 形态 [[x,y],…]：比 [{"x":..,"y":..},…] 少一半字节，
   而 Web 划线器手里本来就是一个点序列，两种写法对它都不更难 */
bool jsonReadPointArray(Json::Object *pParent, const char *pszKey, std::vector<CPointf_S> &rvecOut,
                        int32_t &rnBadType)
{
    Json::Object *pArr = jsonItem(pParent, pszKey);
    if (nullptr == pArr)
    {
        return false;
    }
    if (!cJSON_IsArray(pArr))
    {
        ++rnBadType;
        noteBadKey(pszKey);
        return false;
    }

    rvecOut.clear();
    const int nCount = cJSON_GetArraySize(pArr);
    for (int i = 0; i < nCount; ++i)
    {
        Json::Object *pPt = cJSON_GetArrayItem(pArr, i);
        if (nullptr == pPt || !cJSON_IsArray(pPt) || 2 != cJSON_GetArraySize(pPt))
        {
            ++rnBadType;
        noteBadKey(pszKey);
            continue;
        }
        Json::Object *pX = cJSON_GetArrayItem(pPt, 0);
        Json::Object *pY = cJSON_GetArrayItem(pPt, 1);
        if (nullptr == pX || nullptr == pY || !cJSON_IsNumber(pX) || !cJSON_IsNumber(pY))
        {
            ++rnBadType;
        noteBadKey(pszKey);
            continue;
        }
        rvecOut.push_back(CPointf_S{ static_cast<float>(pX->valuedouble),
                                     static_cast<float>(pY->valuedouble) });
    }
    return true;
}

/* ---------- 默认快照 ---------- */

void fillDefaultSnapshot(CConfigSnapshot_S &stSnap)
{
    stSnap.vecGates.assign(static_cast<size_t>(CDurationGate::columnCount()), CGateParam_S());
    /* 与 vecGates 同长度同下标：默认行是「不限区域」，即没画区域时行为照跑 */
    stSnap.vecBehaviorRegions.assign(static_cast<size_t>(CDurationGate::columnCount()), CRegionRef_S());

    bool abCovered[static_cast<size_t>(CDurationGate::BEHAVIOR_COLUMN_NUM)] = { false };

    for (int i = 0; i < GATE_DEFAULT_NUM; ++i)
    {
        const GateDefault_S &stDef = AST_GATE_DEFAULTS[i];
        const int nCol = CDurationGate::behaviorColumn(stDef.enBehavior);
        if (nCol < 0 || nCol >= static_cast<int>(stSnap.vecGates.size()))
        {
            /* 枚举加了行为但没补默认表：编译期发现不了，只能运行期喊出来，同时不影响其它行为 */
            dlog_warn("classroom_config: behavior %d 无默认门控行",
                      static_cast<int>(stDef.enBehavior));
            continue;
        }
        abCovered[static_cast<size_t>(nCol)] = true;
        CGateParam_S &stGate = stSnap.vecGates[static_cast<size_t>(nCol)];
        stGate.bEnable = stDef.bEnable;
        stGate.nSampleIntervalMs = stDef.nSampleIntervalMs;
        stGate.nMinDurationMs = stDef.nMinDurationMs;
        stGate.nGapToleranceMs = stDef.nGapToleranceMs;
        stGate.nMinTrueCount = stDef.nMinTrueCount;
    }

    for (size_t nCol = 0; nCol < sizeof(abCovered) / sizeof(abCovered[0]); ++nCol)
    {
        if (!abCovered[nCol])
        {
            /* 漏行的那一列会保持 CGateParam_S 的构造值即「关闭」：现象是这个行为怎么配都开不起来，
               与「Web 没下发」长得一模一样，所以必须在这里留一句话说明是代码漏了而非配置漏了 */
            Behavior_E enMiss = Behavior_E::STU_STAND_UP;
            if (CDurationGate::behaviorAtColumn(static_cast<int>(nCol), enMiss))
            {
                dlog_warn("classroom_config: behavior %s (col %d) 无默认门控行，保持禁用",
                          behaviorName(enMiss), static_cast<int>(nCol));
            }
        }
    }

    /* 模型表取自文件作用域的 AST_MODEL_DEFAULTS（同一份表也是整表替换时的核对基准） */
    stSnap.vecModels.clear();
    for (const auto &stRow : AST_MODEL_DEFAULTS)
    {
        CModelDesc_S stDesc;
        stDesc.strKey = stRow.pszKey;
        stDesc.strJsonPath = stRow.pszJson;
        stDesc.strModelPath = stRow.pszModel;
        stDesc.enGroup = stRow.enGroup;
        stDesc.llResidentKb = stRow.llResidentKb;
        stDesc.u32InputW = stRow.u32W;
        stDesc.u32InputH = stRow.u32H;
        stDesc.strBackend = stRow.pszBackend;
        /* 一律 uint8 + pass_through=0 送输入：实测证明 mean/std 已烧进 .rknn 属性并由
           rknn 运行时在该路径上生效（等价于在网 CVInferenceRK 的算法路径）。
           自己按 mean/std 手算 fp16 反而拿到全零垃圾输出，故这里不配 float 输入 */
        stDesc.bFloatInput = false;
        stSnap.vecModels.push_back(stDesc);
    }
}

/* ---------- 文件读取：只做「整读 + 尺寸闸门」，不解析 ---------- */

bool readWholeFile(const std::string &strPath, std::string &rstrText, std::string &rstrErr)
{
    FILE *pf = fopen(strPath.c_str(), "rb");
    if (nullptr == pf)
    {
        rstrErr = "open failed: " + strPath;
        return false;
    }

    if (0 != fseek(pf, 0, SEEK_END))
    {
        fclose(pf);
        rstrErr = "seek failed: " + strPath;
        return false;
    }
    const long lSize = ftell(pf);
    rewind(pf);

    if (lSize <= 0)
    {
        fclose(pf);
        rstrErr = "empty file";
        return false;
    }
    if (lSize > CONFIG_FILE_MAX_BYTES)
    {
        fclose(pf);
        rstrErr = "file too large: " + std::to_string(lSize);
        return false;
    }

    std::vector<char> vecBuf;
    try
    {
        /* 本函数会被下发线程调用，容量再大也有上限，分配失败必须转成错误而不是抛出去 */
        vecBuf.resize(static_cast<size_t>(lSize) + 1);
    }
    catch (const std::bad_alloc &)
    {
        fclose(pf);
        rstrErr = "no memory";
        return false;
    }

    const size_t nRead = fread(vecBuf.data(), 1, static_cast<size_t>(lSize), pf);
    fclose(pf);

    if (nRead != static_cast<size_t>(lSize))
    {
        rstrErr = "short read";
        return false;
    }

    rstrText.assign(vecBuf.data(), nRead);
    return true;
}

/* 钳制并留痕：返回是否发生了钳制 */
template <typename T>
bool clampInPlace(T &tValue, T tMin, T tMax)
{
    const T tClamped = clampValue<T>(tValue, tMin, tMax);
    const bool bChanged = (tClamped != tValue);
    tValue = tClamped;
    return bChanged;
}

/* 校验用：在区域描述表里按 id 找一条（表长上限 CLASSROOM_MAX_REGIONS，线性扫即可）。
   跨表引用（角色 → 区域、行为 → 区域）全部用它判存在性 */
const CRegionDesc_S *findRegionDesc(const std::vector<CRegionDesc_S> &vecDesc, int32_t nId)
{
    for (size_t i = 0; i < vecDesc.size(); ++i)
    {
        if (vecDesc[i].nId == nId)
        {
            return &vecDesc[i];
        }
    }
    return nullptr;
}

/* 钳制留痕：日志里写 JSON 键名而不是 C++ 成员名——现场只会对着配置文件搜字符串。
   同名只记一次：逐列钳制时二十个重复的 behaviors[].region_coverage 既顶日志单行长度，
   又把同一条里其它真正越界的键挤出视野 */
#define CLAMP_WARN(field, lo, hi, name)                                                  \
    do                                                                                   \
    {                                                                                    \
        if (clampInPlace(field, static_cast<decltype(field)>(lo), static_cast<decltype(field)>(hi)) && \
            std::string::npos == strSoft.find(" " name))                                 \
        {                                                                                \
            strSoft += " " name;                                                         \
        }                                                                                \
    } while (0)
}

/* ============================ CConfigSnapshot_S ============================ */

const char *behaviorName(Behavior_E enBehavior)
{
    /* 线性扫 25 行而不是算下标：这张表就是「哪些行为存在」的权威，
       调用频率是心跳级与 statusJson 级，省这几纳秒不值得换一个会漂移的下标推导 */
    for (int i = 0; i < GATE_DEFAULT_NUM; ++i)
    {
        if (AST_GATE_DEFAULTS[i].enBehavior == enBehavior)
        {
            return AST_GATE_DEFAULTS[i].pszName;
        }
    }
    return "unknown";
}

const char *behaviorCnName(Behavior_E enBehavior)
{
    /* 与 AST_GATE_DEFAULTS 一一对应的中文面：枚举注释里的叫法是权威（学生 101~112、教师 201~213），
       带"学生/教师"前缀是因为事件行没有角色列，前缀本身就是消歧 */
    switch (enBehavior)
    {
    case Behavior_E::STU_STAND_UP:    return "学生站立";
    case Behavior_E::STU_RAISE_HAND:  return "学生举手";
    case Behavior_E::STU_HEAD_UP:     return "学生抬头";
    case Behavior_E::STU_HEAD_DOWN:   return "学生低头";
    case Behavior_E::STU_TURN_BACK:   return "学生背身";
    case Behavior_E::STU_PHONE:       return "学生玩手机";
    case Behavior_E::STU_LISTEN:      return "学生听讲";
    case Behavior_E::STU_READING:     return "学生阅读";
    case Behavior_E::STU_DISCUSS:     return "学生讨论";
    case Behavior_E::STU_ON_PODIUM:   return "学生上讲台";
    case Behavior_E::STU_PRESENT:     return "学生演示";
    case Behavior_E::STU_EXPRESSION:  return "学生表情";
    case Behavior_E::TEA_STAND_UP:    return "教师站立";
    case Behavior_E::TEA_HEAD_UP:     return "教师抬头";
    case Behavior_E::TEA_HEAD_DOWN:   return "教师低头";
    case Behavior_E::TEA_TURN_BACK:   return "教师背身";
    case Behavior_E::TEA_PHONE:       return "教师玩手机";
    case Behavior_E::TEA_TEACHING:    return "教师讲授";
    case Behavior_E::TEA_GUIDING:     return "教师指导";
    case Behavior_E::TEA_BOARD_WRITE: return "教师板书";
    case Behavior_E::TEA_INTERACT:    return "教师互动";
    case Behavior_E::TEA_PATROL:      return "教师巡视";
    case Behavior_E::TEA_ENTER_PODIUM:return "教师上讲台";
    case Behavior_E::TEA_TRACK:       return "教师跟踪";
    case Behavior_E::TEA_IDENTITY:    return "教师身份确认";
    default:                          return "未知行为";
    }
}

const char *deviceRoleName(DeviceRole_E enRole)
{
    switch (enRole)
    {
    case DeviceRole_E::ROLE_TEACHER: return "teacher";
    case DeviceRole_E::ROLE_STUDENT: return "student";
    case DeviceRole_E::ROLE_BOTH: return "both";
    default: return "unknown";
    }
}

const CGateParam_S &CConfigSnapshot_S::gateOf(Behavior_E enBehavior) const
{
    static const CGateParam_S S_ST_DISABLED;

    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    if (nCol < 0 || nCol >= static_cast<int>(vecGates.size()))
    {
        return S_ST_DISABLED;
    }
    return vecGates[static_cast<size_t>(nCol)];
}

const CRegionRef_S &CConfigSnapshot_S::regionRefOf(Behavior_E enBehavior) const
{
    /* 返回的是「不限区域」而不是「哪儿都不许」：越界一般是枚举加了行为而默认表没补行，
       此时把该行为整个禁掉会让现场以为功能坏了，而放行只是少了道区域闸 */
    static const CRegionRef_S S_ST_NO_REGION;

    const int nCol = CDurationGate::behaviorColumn(enBehavior);
    if (nCol < 0 || nCol >= static_cast<int>(vecBehaviorRegions.size()))
    {
        return S_ST_NO_REGION;
    }
    return vecBehaviorRegions[static_cast<size_t>(nCol)];
}

bool CConfigSnapshot_S::behaviorOn(Behavior_E enBehavior) const
{
    if (!bEnable)
    {
        return false;
    }
    return gateOf(enBehavior).bEnable;
}

bool CConfigSnapshot_S::anyBehaviorOn() const
{
    if (!bEnable)
    {
        return false;
    }
    for (const auto &stGate : vecGates)
    {
        if (stGate.bEnable)
        {
            return true;
        }
    }
    return false;
}

/* ============================ CClassroomConfig ============================ */

void CClassroomConfig::loadDefaults(const std::string &strSourcePath)
{
    CConfigSnapshot_S stSnap;
    fillDefaultSnapshot(stSnap);

    std::string strErr;
    if (!validate(stSnap, strErr))
    {
        /* 默认值本身不合法属于程序缺陷，不静默：退回到「全关」这份最小可用快照 */
        dlog_error("classroom_config: 内置默认配置被拒: %s，全部功能关闭", strErr.c_str());
        for (auto &stGate : stSnap.vecGates)
        {
            stGate.bEnable = false;
        }
        stSnap.bEnable = false;
    }

    stSnap.strSourcePath = strSourcePath;

    ConfigPtr spNew;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        stSnap.llVersion = m_spCurrent ? m_spCurrent->llVersion + 1 : 1;
        stSnap.llLoadMonoMs = classroomNowMs();
        spNew = std::make_shared<CConfigSnapshot_S>(std::move(stSnap));
        m_spCurrent = spNew;
    }

    dlog_info("classroom_config: 默认配置已加载, version=%lld source=%s models=%zu",
              static_cast<long long>(spNew->llVersion), spNew->strSourcePath.c_str(),
              spNew->vecModels.size());
}

ConfigPtr CClassroomConfig::current() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    if (!m_spCurrent)
    {
        return ConfigPtr();
    }
    return m_spCurrent;
}

std::string CClassroomConfig::defaultPath()
{
    return std::string(USER_DATA_PATH) + CLASSROOM_CONFIG_FILE_NAME;
}

std::string CClassroomConfig::lastConfigText() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_strLastText;
}

const char *CClassroomConfig::lastBadKeyText()
{
    return s_szLastBadKey;
}

bool CClassroomConfig::reloadFromFile(const std::string &strPath, std::string &rstrErr)
{
    std::string strText;
    if (!readWholeFile(strPath, strText, rstrErr))
    {
        return false;
    }
    return reloadFromText(strText, strPath, rstrErr);
}

bool CClassroomConfig::reloadFromText(const std::string &strJson, const std::string &strSourceTag,
                                      std::string &rstrErr)
{
    /* 起点必须是默认快照：这样「缺键保留默认」是结构性保证，而不是逐字段的 if 补齐 */
    CConfigSnapshot_S stSnap;
    fillDefaultSnapshot(stSnap);

    /* 先整段严格解析一遍。Json::init 用的是 cJSON_Parse，它读完第一个完整值就停手，后面的内容
       静默丢弃：现场手改配置时多出来的那个 '}' 就是这么让其后所有键「写了没生效」，
       而日志照样报 swapped 成功——这种失效比直接报错难查一个量级，必须在入口拦下 */
    const char *pszBad = nullptr;
    {
        Json::Object *pChecked = cJSON_ParseWithOpts(strJson.c_str(), &pszBad, 1);
        if (nullptr != pChecked)
        {
            cJSON_Delete(pChecked);
            pszBad = nullptr;   /* 成功时它指向串尾，不是错误位置 */
        }
    }
    if (nullptr != pszBad)
    {
        /* 只报偏移量不回抄原文：配置里可能夹着现场写的备注，抄进日志等于把外部字节喂进日志行 */
        size_t nOffset = static_cast<size_t>(pszBad - strJson.c_str());
        if (nOffset > strJson.size())
        {
            nOffset = strJson.size();
        }
        rstrErr = "json invalid at offset " + std::to_string(nOffset) +
                  " (a stray brace makes cJSON ignore everything after it)";
        return false;
    }

    Json::Object *pRoot = Json::init(strJson);
    if (nullptr == pRoot)
    {
        rstrErr = "json parse failed";
        return false;
    }

    parseIntoJson(pRoot, stSnap);
    Json::deinit(pRoot);

    if (!validate(stSnap, rstrErr))
    {
        dlog_error("classroom_config: 被拒 (%s): %s", strSourceTag.c_str(), rstrErr.c_str());
        return false;
    }

    stSnap.strSourcePath = strSourceTag;

    ConfigPtr spNew;
    int64_t llPrev = 0;
    {
        std::lock_guard<std::mutex> stLock(m_mtx);
        llPrev = m_spCurrent ? m_spCurrent->llVersion : 0;
        stSnap.llVersion = llPrev + 1;
        stSnap.llLoadMonoMs = classroomNowMs();
        spNew = std::make_shared<CConfigSnapshot_S>(std::move(stSnap));
        m_spCurrent = spNew;
        m_strLastText = strJson;
    }

    int32_t nOn = 0;
    for (const auto &stGate : spNew->vecGates)
    {
        nOn += stGate.bEnable ? 1 : 0;
    }
    dlog_info("classroom_config: 配置快照已换 版本 %lld -> %lld 总开关=%d 行为=%d/%zu 模型=%zu 区域=%zu",
              static_cast<long long>(llPrev), static_cast<long long>(spNew->llVersion),
              spNew->bEnable ? 1 : 0, nOn, spNew->vecGates.size(), spNew->vecModels.size(),
              spNew->vecRegions.size());
    return true;
}

/* 用户档位 → 源图瞳距下限（px） */
static float faceSizeOf(const char *pszName)
{
    if (nullptr != pszName)
    {
        if (0 == strcmp(pszName, "far"))
        {
            return 12.0f;
        }
        if (0 == strcmp(pszName, "normal"))
        {
            return 16.0f;
        }
        if (0 == strcmp(pszName, "near"))
        {
            return 24.0f;
        }
    }
    return -1.0f;
}

void CClassroomConfig::parseIntoJson(void *pJson, CConfigSnapshot_S &stSnap)
{
    Json::Object *pRoot = static_cast<Json::Object *>(pJson);
    if (nullptr == pRoot)
    {
        return;
    }

    /* 每次解析重置逐键点名的两张便签：上一份配置的坏键不该赖在这一份头上 */
    noteSection("");
    s_szLastBadKey[0] = '\0';

    int32_t nBadType = 0;
    int32_t nValue = 0;

    /* ---- 顶层 ---- */
    jsonReadBool(pRoot, "enable", stSnap.bEnable, nBadType);
    jsonReadBool(pRoot, "linkage_enable", stSnap.bLinkageEnable, nBadType);

    if (jsonReadInt(pRoot, "mirror_mode", nValue, nBadType))
    {
        stSnap.enMirror = static_cast<MirrorMode_E>(nValue);
    }
    if (jsonReadInt(pRoot, "rotate", nValue, nBadType))
    {
        stSnap.nRotate = nValue;
    }

    /* 设备角色 */
    {
        std::string strRole;
        if (jsonReadString(pRoot, "role", strRole, nBadType))
        {
            if ("teacher" == strRole)
            {
                stSnap.enRole = DeviceRole_E::ROLE_TEACHER;
            }
            else if ("student" == strRole)
            {
                stSnap.enRole = DeviceRole_E::ROLE_STUDENT;
            }
            else if ("both" == strRole)
            {
                stSnap.enRole = DeviceRole_E::ROLE_BOTH;
            }
            else
            {
                stSnap.enRole = static_cast<DeviceRole_E>(-1);
            }
        }
    }

    /* ---- 画面几何 ---- */
    Json::Object *pVideo = jsonItem(pRoot, "video");
    noteSection("video");
    if (nullptr != pVideo)
    {
        jsonReadInt(pVideo, "src_width", stSnap.nSrcWidth, nBadType);
        jsonReadInt(pVideo, "src_height", stSnap.nSrcHeight, nBadType);
        jsonReadInt(pVideo, "canvas_width", stSnap.nCanvasWidth, nBadType);
        jsonReadInt(pVideo, "canvas_height", stSnap.nCanvasHeight, nBadType);
    }

    /* ---- Web 下发区域的参考分辨率与取值空间 ---- */
    Json::Object *pRegion = jsonItem(pRoot, "region_ref");
    noteSection("region_ref");
    if (nullptr != pRegion)
    {
        jsonReadInt(pRegion, "width", stSnap.nRegionRefWidth, nBadType);
        jsonReadInt(pRegion, "height", stSnap.nRegionRefHeight, nBadType);
        if (jsonReadInt(pRegion, "coord_space", nValue, nBadType))
        {
            stSnap.enRegionSpace = static_cast<CoordSpace_E>(nValue);
        }
    }

    /* ---- 分析区域 ---- */
    Json::Object *pRegions = jsonItem(pRoot, "regions");
    noteSection("regions");

    if (nullptr != pRegions && !cJSON_IsArray(pRegions))
    {
        ++nBadType;
    }
    if (nullptr != pRegions && cJSON_IsArray(pRegions))
    {
        stSnap.vecRegions.clear();
        const int nCount = cJSON_GetArraySize(pRegions);
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object *pItem = cJSON_GetArrayItem(pRegions, i);
            if (nullptr == pItem || !cJSON_IsObject(pItem))
            {
                ++nBadType;
                continue;
            }

            CRegionDesc_S stDesc;
            jsonReadInt(pItem, "id", stDesc.nId, nBadType);
            jsonReadString(pItem, "name", stDesc.strName, nBadType);
            if (jsonReadInt(pItem, "shape", nValue, nBadType))
            {
                stDesc.enShape = static_cast<RegionShape_E>(nValue);
            }

            Json::Object *pRect = jsonItem(pItem, "rect");
            if (nullptr != pRect)
            {
                jsonReadFloat(pRect, "x1", stDesc.stRectWeb.fX1, nBadType);
                jsonReadFloat(pRect, "y1", stDesc.stRectWeb.fY1, nBadType);
                jsonReadFloat(pRect, "x2", stDesc.stRectWeb.fX2, nBadType);
                jsonReadFloat(pRect, "y2", stDesc.stRectWeb.fY2, nBadType);
            }
            jsonReadPointArray(pItem, "points", stDesc.vecPointsWeb, nBadType);

            stSnap.vecRegions.push_back(stDesc);
        }
    }

    /* 角色 → 区域：与 regions[] 分开发，是因为 Web 侧「画区域」和「指定这块是讲台」是两步操作 */
    Json::Object *pRoles = jsonItem(pRoot, "region_roles");
    noteSection("region_roles");

    if (nullptr != pRoles && !cJSON_IsArray(pRoles))
    {
        ++nBadType;
    }
    if (nullptr != pRoles && cJSON_IsArray(pRoles))
    {
        stSnap.vecRegionRoles.clear();
        const int nCount = cJSON_GetArraySize(pRoles);
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object *pItem = cJSON_GetArrayItem(pRoles, i);
            if (nullptr == pItem || !cJSON_IsObject(pItem))
            {
                ++nBadType;
                continue;
            }

            CRegionRoleRef_S stRef;
            if (!jsonReadInt(pItem, "role", nValue, nBadType))
            {
                continue;
            }
            stRef.enRole = static_cast<RegionRole_E>(nValue);
            jsonReadIdArray(pItem, "region_ids", stRef.vecRegionIds, nBadType);
            stSnap.vecRegionRoles.push_back(stRef);
        }
    }


    /* ---- 送模型像素的通道顺序（RGB/BGR 未定论期间的切换阀）---- */
    Json::Object *pPre = jsonItem(pRoot, "preprocess");
    noteSection("preprocess");
    if (nullptr != pPre)
    {
        if (jsonReadInt(pPre, "channel_order", nValue, nBadType))
        {
            stSnap.enModelChannel = static_cast<ChannelOrder_E>(nValue);
        }
        jsonReadBool(pPre, "letterbox", stSnap.bLetterboxEnable, nBadType);
        jsonReadStringArray(pPre, "letterbox_models", stSnap.vecLetterboxModels, nBadType);
    }

    /* ---- 阈值 ---- */
    Json::Object *pThreshold = jsonItem(pRoot, "threshold");
    noteSection("threshold");
    if (nullptr != pThreshold)
    {
        jsonReadFloat(pThreshold, "pose_score", stSnap.fPoseScoreThreshold, nBadType);
        jsonReadInt(pThreshold, "min_face_px", stSnap.nMinFacePixel, nBadType);
        jsonReadInt(pThreshold, "min_face_detect_px", stSnap.nMinFaceDetectPixel, nBadType);
        jsonReadInt(pThreshold, "max_persons", stSnap.nMaxPersons, nBadType);
        jsonReadFloat(pThreshold, "det_score", stSnap.fDetectScoreThreshold, nBadType);
        jsonReadFloat(pThreshold, "det_nms", stSnap.fDetectNmsThreshold, nBadType);
        jsonReadInt(pThreshold, "min_head_px", stSnap.nMinHeadPixel, nBadType);
        jsonReadInt(pThreshold, "max_head_px", stSnap.nMaxHeadPixel, nBadType);
        jsonReadInt(pThreshold, "min_body_px", stSnap.nMinBodyPixel, nBadType);
        jsonReadInt(pThreshold, "head_far_edge_px", stSnap.nHeadFarEdgePx, nBadType);
        jsonReadFloat(pThreshold, "head_far_gate_ratio", stSnap.fHeadFarGateRatio, nBadType);
        jsonReadFloat(pThreshold, "occlude_iou", stSnap.fOccludeIou, nBadType);
        jsonReadInt(pThreshold, "edge_band_px", stSnap.nEdgeBandPx, nBadType);
        jsonReadFloat(pThreshold, "blur_full_grad", stSnap.fBlurFullGrad, nBadType);
        jsonReadFloat(pThreshold, "pose_blur_min", stSnap.fPoseBlurMin, nBadType);
    }

    /* ---- 人头跟踪 ---- */
    Json::Object *pTracker = jsonItem(pRoot, "tracker");
    noteSection("tracker");
    if (nullptr != pTracker)
    {
        jsonReadFloat(pTracker, "gate_norm_dist", stSnap.stTracker.fGateNormDist, nBadType);
        jsonReadFloat(pTracker, "move_slack_px_ms", stSnap.stTracker.fMoveSlackPxPerMs, nBadType);
        jsonReadFloat(pTracker, "size_penalty", stSnap.stTracker.fSizePenalty, nBadType);
        jsonReadInt(pTracker, "min_hits", stSnap.stTracker.nMinHits, nBadType);
        jsonReadInt(pTracker, "keep_ms", stSnap.stTracker.nKeepMs, nBadType);
    }

    /* ---- 几何判据 ---- */
    Json::Object *pRules = jsonItem(pRoot, "rules");
    noteSection("rules");
    if (nullptr != pRules)
    {
        Json::Object *pDown = jsonItem(pRules, "head_down");
        if (nullptr != pDown)
        {
            jsonReadFloat(pDown, "key_score", stSnap.stHeadDown.fKeyScore, nBadType);
            jsonReadFloat(pDown, "ratio", stSnap.stHeadDown.fRatio, nBadType);
        }
        Json::Object *pUp = jsonItem(pRules, "head_up");
        if (nullptr != pUp)
        {
            jsonReadFloat(pUp, "key_score", stSnap.stHeadUp.fKeyScore, nBadType);
            jsonReadFloat(pUp, "angle", stSnap.stHeadUp.fAngle, nBadType);
        }
        Json::Object *pStand = jsonItem(pRules, "stand");
        if (nullptr != pStand)
        {
            jsonReadFloat(pStand, "key_score", stSnap.stStand.fKeyScore, nBadType);
            jsonReadFloat(pStand, "leg_vertical_angle", stSnap.stStand.fLegVerticalAngle, nBadType);
            jsonReadFloat(pStand, "arm_straight_angle", stSnap.stStand.fArmStraightAngle, nBadType);
            jsonReadFloat(pStand, "min_torso_shoulder", stSnap.stStand.fMinTorsoShoulder, nBadType);
        }
        /* 教师三条事件的置信度门（206/208/209）。与 group.listen / board 的键分开：
           那两处各管"学生朝向证据"与"OCR 行分"，接在一起就是跨页串台 */
        Json::Object *pTeach = jsonItem(pRules, "teaching");
        if (nullptr != pTeach)
        {
            jsonReadFloat(pTeach, "key_score", stSnap.fTeachKeyScore, nBadType);
        }
        Json::Object *pBWrite = jsonItem(pRules, "board_write");
        if (nullptr != pBWrite)
        {
            jsonReadFloat(pBWrite, "key_score", stSnap.fBoardWriteKeyScore, nBadType);
        }
        Json::Object *pInteract = jsonItem(pRules, "interact");
        if (nullptr != pInteract)
        {
            jsonReadFloat(pInteract, "key_score", stSnap.fInteractKeyScore, nBadType);
        }
        /* 207 页「区域停留时间」（网页 1..10s）：与 206/208/209 的置信门同处一个 rules
           小节，但它是时间量、不是分数，单独一个对象存以免和 key_score 混读 */
        Json::Object *pGuiding = jsonItem(pRules, "guiding");
        if (nullptr != pGuiding)
        {
            int32_t nStayMs = stSnap.nGuidingStayMs;
            if (jsonReadInt(pGuiding, "stay_ms", nStayMs, nBadType))
            {
                stSnap.nGuidingStayMs = nStayMs;
            }
        }
        Json::Object *pRaise = jsonItem(pRules, "raise_hand");
        if (nullptr != pRaise)
        {
            jsonReadFloat(pRaise, "key_score", stSnap.stRaiseHand.fKeyScore, nBadType);
            jsonReadFloat(pRaise, "vertical_angle", stSnap.stRaiseHand.fVerticalAngle, nBadType);
            jsonReadFloat(pRaise, "min_wrist_above_head", stSnap.stRaiseHand.fMinWristAboveHead, nBadType);
            jsonReadInt(pRaise, "hand_mode", stSnap.stRaiseHand.nHandMode, nBadType);
        }
        Json::Object *pBack = jsonItem(pRules, "turn_back");
        if (nullptr != pBack)
        {
            jsonReadFloat(pBack, "face_score", stSnap.stTurnBack.fFaceScore, nBadType);
            jsonReadFloat(pBack, "body_score", stSnap.stTurnBack.fBodyScore, nBadType);
            jsonReadFloat(pBack, "face_witness_ttl_ms", stSnap.stTurnBack.fFaceWitnessTtlMs, nBadType);
            jsonReadFloat(pBack, "face_witness_mirror_rx", stSnap.stTurnBack.fFaceWitnessMirrorRx, nBadType);
        }
        Json::Object *pLie = jsonItem(pRules, "lie_on_desk");
        if (nullptr != pLie)
        {
            jsonReadFloat(pLie, "key_score", stSnap.stLieOnDesk.fKeyScore, nBadType);
        }
        Json::Object *pPhone = jsonItem(pRules, "phone");
        if (nullptr != pPhone)
        {
            jsonReadFloat(pPhone, "key_score", stSnap.stPhone.fKeyScore, nBadType);
            jsonReadFloat(pPhone, "band_above_shoulder", stSnap.stPhone.fBandAboveShoulder, nBadType);
            jsonReadFloat(pPhone, "band_below_shoulder", stSnap.stPhone.fBandBelowShoulder, nBadType);
            jsonReadFloat(pPhone, "max_elbow_angle", stSnap.stPhone.fMaxElbowAngle, nBadType);
            jsonReadFloat(pPhone, "wrist_gap_norm", stSnap.stPhone.fMaxWristGapNorm, nBadType);
        }
        /* 107/109/111 的三档朝向 + 108 的书写代理闸 + 三道人数阈值。
           读法与上面每一条一样：缺键保默认，所以"只改讨论"不会把听讲那一对数冲成 0 */
        Json::Object *pGroup = jsonItem(pRules, "group");
        if (nullptr != pGroup)
        {
            struct DirRow_S { const char *pszKey; FaceDirRule_S *pstDir; };
            const DirRow_S astDir[3] = {
                { "listen",  &stSnap.stGroup.stListenDir  },
                { "discuss", &stSnap.stGroup.stDiscussDir },
                { "present", &stSnap.stGroup.stPresentDir },
            };
            for (const DirRow_S &stRow : astDir)
            {
                Json::Object *pDir = jsonItem(pGroup, stRow.pszKey);
                if (nullptr == pDir)
                {
                    continue;
                }
                jsonReadFloat(pDir, "key_score", stRow.pstDir->fKeyScore, nBadType);
                jsonReadFloat(pDir, "toward_true", stRow.pstDir->fTrueAbove, nBadType);
                jsonReadFloat(pDir, "toward_false", stRow.pstDir->fFalseBelow, nBadType);
            }
            jsonReadBool(pGroup, "forbid_hand", stSnap.stGroup.bListenForbidHand, nBadType);
            jsonReadFloat(pGroup, "neighbor_max_head", stSnap.stGroup.fNeighborMaxHead, nBadType);
            jsonReadFloat(pGroup, "ref_deadband_head", stSnap.stGroup.fRefDeadbandHead, nBadType);
            jsonReadInt(pGroup, "listen_min_persons", stSnap.stGroup.nListenMinPersons, nBadType);
            jsonReadInt(pGroup, "reading_min_persons", stSnap.stGroup.nReadingMinPersons, nBadType);
            jsonReadInt(pGroup, "discuss_min_persons", stSnap.stGroup.nDiscussMinPersons, nBadType);
            jsonReadInt(pGroup, "discuss_turn_persons", stSnap.stGroup.nDiscussTurnPersons, nBadType);
        }
    }

    Json::Object *pExpand = jsonItem(pRoot, "body_expand");
    noteSection("body_expand");
    if (nullptr != pExpand)
    {
        jsonReadFloat(pExpand, "w", stSnap.fBodyExpandW, nBadType);
        jsonReadFloat(pExpand, "down", stSnap.fBodyExpandDown, nBadType);
        jsonReadFloat(pExpand, "up", stSnap.fBodyExpandUp, nBadType);
    }

    /* ---- 人脸五点对齐 ----
       阈值一项不夹：这里的每个数都是"能不能拿这张脸去认人"的一道闸，
       钳到边界会让人以为配生效了。能不能用由 validate 单点判（与 attendance 同一口径） */
    Json::Object *pAlign = jsonItem(pRoot, "align");
    noteSection("align");
    if (nullptr != pAlign)
    {
        jsonReadFloat(pAlign, "point_score_min", stSnap.stAlign.fPointScoreMin, nBadType);
        jsonReadFloat(pAlign, "min_eye_span_px", stSnap.stAlign.fMinEyeSpanPx, nBadType);

        Json::Object *pFaceSize = jsonItem(pAlign, "face_size");
        if (nullptr != pFaceSize && cJSON_IsString(pFaceSize) && nullptr != pFaceSize->valuestring)
        {
            const float fPresetSpan = faceSizeOf(pFaceSize->valuestring);
            if (fPresetSpan > 0.0f)
            {
                stSnap.stAlign.fMinEyeSpanPx = fPresetSpan;
                Json::Object *pSpanKey = jsonItem(pAlign, "min_eye_span_px");
                if (nullptr != pSpanKey && cJSON_IsNumber(pSpanKey))
                {
                    dlog_warn("classroom_config: align.face_size=%s 压过 min_eye_span_px"
                              "（A/B 请删掉 face_size）",
                              pFaceSize->valuestring);
                }
            }
            else
            {
                ++nBadType;
                /* 与 point_order 同一条规矩：未知名字不静默退回默认，
                   否则「改了没生效」读成「档位本来就是这样」 */
                dlog_warn("classroom_config: align.face_size 档位未知，沿用 %.0fpx",
                          stSnap.stAlign.fMinEyeSpanPx);
            }
        }
        jsonReadFloat(pAlign, "scale_min", stSnap.stAlign.fScaleMin, nBadType);
        jsonReadFloat(pAlign, "scale_max", stSnap.stAlign.fScaleMax, nBadType);
        jsonReadFloat(pAlign, "max_residual_ratio", stSnap.stAlign.fMaxResidualRatio, nBadType);
        /* 0 = 关掉地板门（旧行为，留给 A/B）；语义与默认值见 face_align.h 那条字段注释 */
        jsonReadFloat(pAlign, "noise_floor_src_px", stSnap.stAlign.fNoiseFloorSrcPx, nBadType);
        jsonReadBool(pAlign, "order_probe", stSnap.bAlignOrderProbe, nBadType);
        jsonReadInt(pAlign, "crop_px", stSnap.nAlignCropPx, nBadType);

        /* point_order 收两种写法：表名（"eye_swap"）或表下标。名字优先是因为**下标会随表序漂**：
           探针那 6 张表将来加一张就把后面的顶位，现场抄下来的"3"过一版程序指向另一张表，
           而错的方向恰好是"这台设备突然认不出人了"——最难查的那类。数字保留给脚本下发 */
        Json::Object *pOrder = jsonItem(pAlign, "point_order");
        if (nullptr != pOrder && cJSON_IsString(pOrder) && nullptr != pOrder->valuestring)
        {
            const int32_t nPerm = alignPermOf(pOrder->valuestring);
            if (nPerm >= 0)
            {
                stSnap.nAlignPointOrder = nPerm;
            }
            else
            {
                ++nBadType;
                /* 未知名字不静默退回恒等：退回会让"改了没生效"读成"点序本来就是这样"，
                   而这正是本项唯一要解决的那个疑问 */
                dlog_warn("classroom_config: align.point_order 名未知，沿用 %s",
                          alignPermName(stSnap.nAlignPointOrder));
            }
        }
        else if (jsonReadInt(pAlign, "point_order", nValue, nBadType))
        {
            stSnap.nAlignPointOrder = nValue;    /* 越界由 validate 当场拒 */
        }

        /* 模板五点：算法侧换模型时唯一需要动的量，故允许整组覆盖。
           只给 4 个点时整组不采纳而不是"覆盖前 4 个"——半套模板会让第五点
           仍以旧基准参与最小二乘，解出来的角差一点又残差没过门，读数上一头雾水 */
        std::vector<CPointf_S> vecTpl;
        if (jsonReadPointArray(pAlign, "template", vecTpl, nBadType))
        {
            if (vecTpl.size() == static_cast<size_t>(CLASSROOM_FACE_LANDMARK_NUM))
            {
                for (int32_t i = 0; i < CLASSROOM_FACE_LANDMARK_NUM; ++i)
                {
                    stSnap.stAlign.astTemplate[i] = vecTpl[static_cast<size_t>(i)];
                }
            }
            else
            {
                ++nBadType;
                dlog_warn("classroom_config: align.template 有 %zu 个点，需要 %d；保留默认",
                          vecTpl.size(), CLASSROOM_FACE_LANDMARK_NUM);
            }
        }
    }

    /* ---- 行为表：按 Behavior_E 编号下发，缺行的行为保留默认（不是关掉）---- */
    Json::Object *pBehaviors = jsonItem(pRoot, "behaviors");
    noteSection("behaviors");

    if (nullptr != pBehaviors && !cJSON_IsArray(pBehaviors))
    {
        ++nBadType;
    }
    if (nullptr != pBehaviors && cJSON_IsArray(pBehaviors))
    {
        const int nCount = cJSON_GetArraySize(pBehaviors);
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object *pItem = cJSON_GetArrayItem(pBehaviors, i);
            if (nullptr == pItem || !cJSON_IsObject(pItem))
            {
                ++nBadType;
                continue;
            }
            if (!jsonReadInt(pItem, "id", nValue, nBadType))
            {
                continue;
            }
            const int nCol = CDurationGate::behaviorColumn(static_cast<Behavior_E>(nValue));
            if (nCol < 0 || nCol >= static_cast<int>(stSnap.vecGates.size()))
            {
                /* 未知行为号：可能是新程序下发给老程序，忽略而非拒绝整份配置 */
                dlog_warn("classroom_config: 未知行为 id %d 已忽略", nValue);
                continue;
            }
            CGateParam_S &stGate = stSnap.vecGates[static_cast<size_t>(nCol)];
            jsonReadBool(pItem, "enable", stGate.bEnable, nBadType);
            jsonReadInt(pItem, "sample_interval_ms", stGate.nSampleIntervalMs, nBadType);
            jsonReadInt(pItem, "min_duration_ms", stGate.nMinDurationMs, nBadType);
            jsonReadInt(pItem, "gap_tolerance_ms", stGate.nGapToleranceMs, nBadType);
            jsonReadInt(pItem, "min_true_count", stGate.nMinTrueCount, nBadType);

            /* 同一行里带上区域引用：CSV 里「分析区域」本来就是这条行为的一个参数，
               拆成两份表会让下发端发两次、现场改两次，漏一次就是改了开关没改区域 */
            CRegionRef_S &stRef = stSnap.vecBehaviorRegions[static_cast<size_t>(nCol)];
            jsonReadIdArray(pItem, "region_ids", stRef.vecRegionIds, nBadType);
            jsonReadFloat(pItem, "region_coverage", stRef.fMinCoverage, nBadType);
        }
    }

    /* ---- 模型表：非空数组才整体替换默认表（半合并会留下「谁覆盖了谁」的歧义）；
       空数组按「未提供」处理：下发端偶发发出空列表，若当成清空模型表，
       整机会静默失去所有检测能力——这种失效既不可见也难查，代价远大于尊重一次误操作 ---- */
    Json::Object *pModels = jsonItem(pRoot, "models");
    noteSection("models");

    if (nullptr != pModels && !cJSON_IsArray(pModels))
    {
        ++nBadType;
    }
    if (nullptr != pModels && cJSON_IsArray(pModels))
    {
        const int nCount = cJSON_GetArraySize(pModels);
        if (nCount > 0)
        {
            stSnap.vecModels.clear();
        }
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object *pItem = cJSON_GetArrayItem(pModels, i);
            if (nullptr == pItem || !cJSON_IsObject(pItem))
            {
                ++nBadType;
                continue;
            }
            CModelDesc_S stDesc;
            jsonReadString(pItem, "key", stDesc.strKey, nBadType);
            jsonReadString(pItem, "json_path", stDesc.strJsonPath, nBadType);
            jsonReadString(pItem, "model_path", stDesc.strModelPath, nBadType);
            if (jsonReadInt(pItem, "group", nValue, nBadType))
            {
                stDesc.enGroup = static_cast<ModelGroup_E>(nValue);
            }
            jsonReadInt64(pItem, "resident_kb", stDesc.llResidentKb, nBadType);
            if (jsonReadInt(pItem, "input_w", nValue, nBadType))
            {
                stDesc.u32InputW = (nValue > 0) ? static_cast<uint32_t>(nValue) : 0;
            }
            if (jsonReadInt(pItem, "input_h", nValue, nBadType))
            {
                stDesc.u32InputH = (nValue > 0) ? static_cast<uint32_t>(nValue) : 0;
            }
            jsonReadBool(pItem, "float_input", stDesc.bFloatInput, nBadType);
            /* backend 缺省保留默认值 raw：宁缺毋滥——raw 后端在 P1 只登记不加载，
               表现为 LOAD_FAILED（可观测），而猜成 yolov5/yolov8 会解出错框（不可观测） */
            jsonReadString(pItem, "backend", stDesc.strBackend, nBadType);
            stSnap.vecModels.push_back(stDesc);
        }

        /* 整表替换后核对一遍默认表里有哪些键被丢了：只 warn 不拒绝——现场有权只注册一部分模型
           （内存吃紧先关人脸链就是正常操作），但这件事必须留在日志里。它拦的是最典型的一种：
           固件升级后机上还留着上一版的配置文件，那份 models[] 正好少掉新链路的键，
           池子从来没见过这些键、acquire 只回 DISABLED，现象是「新芯片一颗都不加载、
           而 statusJson 一切正常」——不数配置文件行数是查不出来的 */
        if (!stSnap.vecModels.empty())
        {
            for (int nDef = 0; nDef < MODEL_DEFAULT_NUM; ++nDef)
            {
                const std::string strDefKey = AST_MODEL_DEFAULTS[nDef].pszKey;
                bool bKept = false;
                for (const auto &stDesc : stSnap.vecModels)
                {
                    if (stDesc.strKey == strDefKey)
                    {
                        bKept = true;
                        break;
                    }
                }
                if (!bKept)
                {
                    dlog_warn("classroom_config: models[] 丢弃内置键 %s（该管线保持"
                              "未注册——不是部署缺件，是这张表本身旧了）",
                              strDefKey.c_str());
                }
            }
        }
    }

    /* ---- 模型池参数 ---- */
    Json::Object *pPool = jsonItem(pRoot, "pool");
    noteSection("pool");
    if (nullptr != pPool)
    {
        jsonReadInt64(pPool, "idle_unload_ms", stSnap.stPool.llIdleUnloadMs, nBadType);
        jsonReadInt64(pPool, "reserve_kb", stSnap.stPool.llReserveKb, nBadType);
        jsonReadInt64(pPool, "slice_pose_ms", stSnap.stPool.llSlicePoseMs, nBadType);
        jsonReadInt64(pPool, "slice_face_ms", stSnap.stPool.llSliceFaceMs, nBadType);
        jsonReadBool(pPool, "evict_thread", stSnap.stPool.bEvictThread, nBadType);
    }

    /* ---- 采样节拍 ---- */
    Json::Object *pSample = jsonItem(pRoot, "sampling");
    noteSection("sampling");
    if (nullptr != pSample)
    {
        jsonReadInt(pSample, "head_interval_ms", stSnap.stSample.nHeadIntervalMs, nBadType);
        jsonReadInt(pSample, "pose_interval_ms", stSnap.stSample.nPoseIntervalMs, nBadType);
        jsonReadInt(pSample, "face_interval_ms", stSnap.stSample.nFaceIntervalMs, nBadType);
        jsonReadInt(pSample, "max_pose_persons", stSnap.stSample.nMaxPosePersons, nBadType);
        jsonReadInt(pSample, "pose_beat_budget_ms", stSnap.stSample.nPoseBeatBudgetMs, nBadType);
        jsonReadInt(pSample, "group_interval_ms", stSnap.stSample.nGroupIntervalMs, nBadType);
        jsonReadInt(pSample, "npu_busy_high", stSnap.stSample.nNpuBusyHighPercent, nBadType);
        jsonReadInt(pSample, "npu_busy_low", stSnap.stSample.nNpuBusyLowPercent, nBadType);
        jsonReadInt(pSample, "infer_wait_ms", stSnap.stSample.nInferWaitMs, nBadType);
    }

    /* ---- 主体归属（教师是谁）---- */
    Json::Object *pTeacher = jsonItem(pRoot, "teacher");
    noteSection("teacher");
    if (nullptr != pTeacher)
    {
        jsonReadBool(pTeacher, "enable", stSnap.stTeacher.bEnable, nBadType);
        jsonReadInt(pTeacher, "confirm_ms", stSnap.stTeacher.nConfirmMs, nBadType);
        jsonReadInt(pTeacher, "release_ms", stSnap.stTeacher.nReleaseMs, nBadType);
        jsonReadInt(pTeacher, "max_num", stSnap.stTeacher.nMaxNum, nBadType);
        jsonReadFloat(pTeacher, "coverage", stSnap.stTeacher.fCoverage, nBadType);
        jsonReadInt(pTeacher, "patrol_move_px", stSnap.stTeacher.nPatrolMovePx, nBadType);
        jsonReadInt(pTeacher, "patrol_window_ms", stSnap.stTeacher.nPatrolWindowMs, nBadType);
        jsonReadInt(pTeacher, "stay_evidence_ms", stSnap.stTeacher.nStayEvidenceMs, nBadType);
        jsonReadInt(pTeacher, "absent_hold_ms", stSnap.stTeacher.nAbsentHoldMs, nBadType);
        jsonReadInt(pTeacher, "reentry_ms", stSnap.stTeacher.nReentryMs, nBadType);
        jsonReadInt(pTeacher, "reentry_px", stSnap.stTeacher.nReentryPx, nBadType);
        jsonReadFloat(pTeacher, "pose_witness_ttl_ms", stSnap.stTeacher.fPoseWitnessTtlMs, nBadType);
        jsonReadInt(pTeacher, "pose_witness_own_head_pct", stSnap.stTeacher.nWitOwnHeadPct, nBadType);
        jsonReadFloat(pTeacher, "pose_witness_min_score", stSnap.stTeacher.fPoseWitMinScore, nBadType);
    }

    /* ---- 考勤账本（CSV 25~30）----
       键名与 CAttendParam_S 一一对应，不在这里做范围处理：这套参数的"能不能用"
       由 attendanceParamOk 单点判定（账本装载时走的是同一个函数），
       配置层再写一套区间就是第二个口径，两边不一致时改配置会时灵时不灵 */
    Json::Object *pAttend = jsonItem(pRoot, "attendance");
    noteSection("attendance");
    if (nullptr != pAttend)
    {
        jsonReadBool(pAttend, "enable", stSnap.stAttend.bEnable, nBadType);
        jsonReadInt64(pAttend, "present_min_ms", stSnap.stAttend.llPresentMinMs, nBadType);
        jsonReadInt64(pAttend, "gap_max_ms", stSnap.stAttend.llGapMaxMs, nBadType);
        jsonReadInt(pAttend, "face_min_px", stSnap.stAttend.nMinFacePx, nBadType);
        jsonReadInt(pAttend, "face_far_min_px", stSnap.stAttend.nFaceFarMinPx, nBadType);
        jsonReadInt(pAttend, "match_min_hits", stSnap.stAttend.nMinMatchHits, nBadType);
        jsonReadFloat(pAttend, "match_score_min", stSnap.stAttend.fMinMatchScore, nBadType);
        jsonReadFloat(pAttend, "face_far_match_score", stSnap.stAttend.fFarMatchScore, nBadType);
        jsonReadInt(pAttend, "face_far_min_hits", stSnap.stAttend.nFarMinHits, nBadType);
        jsonReadInt64(pAttend, "row_idle_ms", stSnap.stAttend.llRowIdleMs, nBadType);
        jsonReadInt(pAttend, "expected_num", stSnap.stAttend.nExpectedNum, nBadType);
        /* 考勤检测区域。空 = 全画面；校验由 regions[] 的引用完整性统一做 */
        (void)jsonReadIdArray(pAttend, "region_ids", stSnap.stAttend.vecRegionIds, nBadType);
    }

    /* ---- 底库来源（名单从哪里来）---- */
    Json::Object *pGallery = jsonItem(pRoot, "gallery");
    noteSection("gallery");
    if (nullptr != pGallery)
    {
        jsonReadBool(pGallery, "enable", stSnap.stGallery.bEnable, nBadType);
        jsonReadString(pGallery, "path", stSnap.stGallery.strPath, nBadType);
        /* 人脸检测区域。空 = 全画面 */
        (void)jsonReadIdArray(pGallery, "region_ids", stSnap.stGallery.vecRegionIds, nBadType);
        /* table 原样读入，不在这里判字符集：能不能用由 validate 用 galleryTableOk 单点判
           （公共仓那句 SQL 是直接拼表名的，这道校验是那条路上唯一的闸门） */
        jsonReadString(pGallery, "table", stSnap.stGallery.strTable, nBadType);
        jsonReadInt(pGallery, "reload_sec", stSnap.stGallery.nReloadSec, nBadType);
        jsonReadInt(pGallery, "max_enroll", stSnap.stGallery.nMaxEnroll, nBadType);

        /* source 只认三个值，第四个都不采纳（含拼错的 "sqllite"） */
        std::string strSource;
        if (jsonReadString(pGallery, "source", strSource, nBadType))
        {
            if ("file" == strSource || "sqlite" == strSource || "none" == strSource)
            {
                stSnap.stGallery.strSource = strSource;
            }
            else
            {
                /* 坏值也要点名：其余所有坏类型/坏值都走 noteBadKey，只有这里漏了的话，
                   心跳/日志侧的 "最后一个坏键" 会指向上一处，现场照着它去改会改错键 */
                ++nBadType;
                noteBadKey("source");
                dlog_warn("classroom_config: gallery.source 须 file/sqlite/none, 沿用 %s",
                          stSnap.stGallery.strSource.c_str());
            }
        }
    }

    /* ---- 取特征段（花多少钱）---- */
    Json::Object *pFeat = jsonItem(pRoot, "face_feature");
    noteSection("face_feature");
    if (nullptr != pFeat)
    {
        jsonReadBool(pFeat, "enable", stSnap.stFaceFeat.bEnable, nBadType);
        jsonReadInt(pFeat, "max_persons", stSnap.stFaceFeat.nMaxPersons, nBadType);
        jsonReadInt(pFeat, "per_beat_persons", stSnap.stFaceFeat.nPerBeatPersons, nBadType);
        jsonReadInt(pFeat, "min_interval_ms", stSnap.stFaceFeat.nMinIntervalMs, nBadType);
        jsonReadInt(pFeat, "ttl_ms", stSnap.stFaceFeat.nTtlMs, nBadType);
    }

    /* ---- CPU-NPU 流水线优化开关（灰度用，默认全 false＝旧串行管线）----
       整块缺席不算错：这组键是现场逐项打开的旋钮，出厂 json 落 false，
       老配置文件（没有 optimization 块）必须原样跑在旧行为上 */
    {
        Json::Object *pOpt = jsonItem(pRoot, "optimization");
    noteSection("optimization");
        if (nullptr != pOpt)
        {
            jsonReadBool(pOpt, "submit_async_enable", stSnap.stOpt.bSubmitAsyncEnable, nBadType);
            jsonReadBool(pOpt, "face_early_submit", stSnap.stOpt.bFaceEarlySubmit, nBadType);
            jsonReadBool(pOpt, "feat_per_beat_enable", stSnap.stOpt.bFeatPerBeatEnable, nBadType);
            jsonReadBool(pOpt, "pose_phone_interleave", stSnap.stOpt.bPosePhoneInterleave, nBadType);
            jsonReadBool(pOpt, "urgency_precompute", stSnap.stOpt.bUrgencyPrecompute, nBadType);
        }
    }

    /* ---- 表情/情绪（P5e）----
       这一段没有 max_persons / min_interval_ms 不是漏配：表情没有自己的节拍，
       它吃的是取特征段为某一个人**已经算好的那张对齐图**（框架图 组合4=复用组合2），
       一个人多久被喂一次由 face_feature.ttl_ms 与轮转人数决定。这里只留两个旋钮：
       enable（省掉那次 1.4ms 推理）与 ttl_ms（多久没喂就把平滑行清掉）。
       判定门限一项不夹，与 align 同一口径：能不能用由 expressionParamOk 单点判 */
    Json::Object *pExpr = jsonItem(pRoot, "expression");
    noteSection("expression");
    if (nullptr != pExpr)
    {
        jsonReadBool(pExpr, "enable", stSnap.stExpr.bEnable, nBadType);
        jsonReadInt(pExpr, "ttl_ms", stSnap.stExpr.nTtlMs, nBadType);
        jsonReadInt(pExpr, "event_ttl_ms", stSnap.stExpr.nEventTtlMs, nBadType);
        jsonReadInt(pExpr, "slot_num", stSnap.stExpr.stParam.nSlotNum, nBadType);
        jsonReadFloat(pExpr, "prob_min", stSnap.stExpr.stParam.fProbMin, nBadType);
        jsonReadFloat(pExpr, "margin_min", stSnap.stExpr.stParam.fMarginMin, nBadType);
        jsonReadFloat(pExpr, "ema_alpha", stSnap.stExpr.stParam.fEmaAlpha, nBadType);
        jsonReadInt(pExpr, "min_frames", stSnap.stExpr.stParam.nMinFrames, nBadType);
        jsonReadFloat(pExpr, "curve_prob_min", stSnap.stExpr.stParam.fCurveProbMin, nBadType);
        /* 允许人脸角度（度，1..90）：Web 第 11 页那一格的落点。消费方是感知入口
           （perceive_pipeline 的 runExpressionOfWarp），不是判定函数——超限时连推理都不提交 */
        jsonReadFloat(pExpr, "max_angle", stSnap.stExpr.stParam.fMaxAngleDeg, nBadType);

        /* 槽位→语义映射表：模型交付时唯一需要动的东西 */
        Json::Object *pMap = jsonItem(pExpr, "slot_map");
        if (nullptr != pMap && !cJSON_IsArray(pMap))
        {
            /* 写成对象或写成字符串（"neutral,happy,…"）都在这一档：只给一条原因不给值本身，
               理由与 gallery 表名那条相同——配置里的这一段可能是平台下发的 */
            ++nBadType;
            stSnap.stExpr.stParam.enSlotMapSrc = SlotMapSrc_E::SMS_REJECTED;
            dlog_warn("classroom_config: expression.slot_map 不是数组，保留默认");
        }
        else if (nullptr != pMap && cJSON_IsArray(pMap))
        {
            const int nMapNum = cJSON_GetArraySize(pMap);
            if (nMapNum > CLASSROOM_EXPRESSION_SLOT_MAX)
            {
                stSnap.stExpr.stParam.enSlotMapSrc = SlotMapSrc_E::SMS_REJECTED;
                ++nBadType;
                dlog_warn("classroom_config: expression.slot_map 有 %d 项，上限 %d；保留默认",
                          nMapNum, CLASSROOM_EXPRESSION_SLOT_MAX);
            }
            else
            {
                Expression_E anMap[CLASSROOM_EXPRESSION_SLOT_MAX];
                for (int i = 0; i < CLASSROOM_EXPRESSION_SLOT_MAX; ++i)
                {
                    /* 先整表置 UNKNOWN 再逐项覆盖：只给 3 项时后面 13 项显式为 UNKNOWN，
                       expressionParamOk 会因"槽位在 nSlotNum 之内却映射到 UNKNOWN"当场拒——
                       半套表比没给表更危险，因为前 3 个槽位看起来是配好了的 */
                    anMap[i] = Expression_E::EXP_UNKNOWN;
                }
                bool bMapBad = false;
                for (int i = 0; i < nMapNum; ++i)
                {
                    Json::Object *pEle = cJSON_GetArrayItem(pMap, i);
                    if (nullptr == pEle)
                    {
                        bMapBad = true;
                        break;
                    }
                    if (cJSON_IsString(pEle) && nullptr != pEle->valuestring)
                    {
                        const int32_t nLabel = expressionOf(pEle->valuestring);
                        if (nLabel < 0)
                        {
                            bMapBad = true;
                            dlog_warn("classroom_config: expression.slot_map[%d] 名未知", i);
                            break;
                        }
                        anMap[i] = static_cast<Expression_E>(nLabel);
                    }
                    else if (cJSON_IsNumber(pEle))
                    {
                        anMap[i] = static_cast<Expression_E>(
                            static_cast<int32_t>(std::lround(pEle->valuedouble)));
                    }
                    else
                    {
                        bMapBad = true;
                        break;
                    }
                }
                if (bMapBad)
                {
                    /* 整组不采纳而不是覆盖能认的那几项：混着半张默认表用，
                       读到的"这个槽位是开心"到底是下发的还是出厂的，事后无从分辨 */
                    stSnap.stExpr.stParam.enSlotMapSrc = SlotMapSrc_E::SMS_REJECTED;
                    ++nBadType;
                    dlog_warn("classroom_config: expression.slot_map 不可读，保留默认");
                }
                else
                {
                    for (int i = 0; i < CLASSROOM_EXPRESSION_SLOT_MAX; ++i)
                    {
                        stSnap.stExpr.stParam.anSlotMap[i] = anMap[i];
                    }
                    stSnap.stExpr.stParam.enSlotMapSrc = SlotMapSrc_E::SMS_CFG;
                }
            }
        }
    }

    /* ---- 玩手机复核（与 rules.phone 那三条判据门限无关）---- */
    Json::Object *pPhoneRc = jsonItem(pRoot, "phone_recheck");
    noteSection("phone_recheck");
    if (nullptr != pPhoneRc)
    {
        jsonReadBool(pPhoneRc, "enable", stSnap.stPhoneRecheck.bEnable, nBadType);
        jsonReadInt(pPhoneRc, "max_persons", stSnap.stPhoneRecheck.nMaxPersons, nBadType);
        jsonReadInt(pPhoneRc, "min_interval_ms", stSnap.stPhoneRecheck.nMinIntervalMs, nBadType);
        jsonReadInt(pPhoneRc, "ttl_ms", stSnap.stPhoneRecheck.nTtlMs, nBadType);
        jsonReadInt(pPhoneRc, "min_box_px", stSnap.stPhoneRecheck.nMinBoxPx, nBadType);
    }

    /* ---- 座位就坐热力----
       与 attendance 同一口径：这里只搬运，不做范围处理。"这套值能不能用"由
       seatHeatParamOk 单点判定（引擎装载时走的是同一个函数），配置层再写一套区间
       就是第二个口径，两边不一致时改配置会时灵时不灵 */
    Json::Object *pSeat = jsonItem(pRoot, "seat_heat");
    noteSection("seat_heat");
    if (nullptr != pSeat)
    {
        jsonReadBool(pSeat, "enable", stSnap.stSeat.bEnable, nBadType);
        jsonReadInt(pSeat, "cols", stSnap.stSeat.nCols, nBadType);
        jsonReadInt(pSeat, "rows", stSnap.stSeat.nRows, nBadType);
        jsonReadInt(pSeat, "hold_beats", stSnap.stSeat.nHoldBeats, nBadType);
        jsonReadInt(pSeat, "min_cell_cover_percent", stSnap.stSeat.nMinCellCoverPercent, nBadType);
        jsonReadInt(pSeat, "idle_beats", stSnap.stSeat.nIdleBeats, nBadType);
    }

    /* ---- 座位先验----
       同样只搬运不解释：形状判据归 seatPriorParamOk，跨块那条算术归 seatPriorIdleOk，
       两处都在下面的校验段里一次跑完（与 seat_heat / attendance 同一口径）*/
    Json::Object *pPrior = jsonItem(pRoot, "seat_prior");
    noteSection("seat_prior");
    if (nullptr != pPrior)
    {
        jsonReadBool(pPrior, "enable", stSnap.stPrior.bEnable, nBadType);
        jsonReadInt(pPrior, "inherit_beats", stSnap.stPrior.nInheritBeats, nBadType);
        jsonReadInt(pPrior, "release_beats", stSnap.stPrior.nReleaseBeats, nBadType);
    }

    /* ---- 板书记录----
       reentry_ms 落进 `llIntervalMs` 这个字段名：它在 CBoardWatcher 里的语义已由"周期"改成
       "两次板书结束边沿之间的最小间隔"（见 board_watcher.h 的 bRequireEpisode）。字段名不改是
       权衡过的结果——改要连动感知层与三处日志判读，而"这个数到底管什么"已经写在头文件与这里
       的键名上；留一个名字与语义分叉的字段比留一个名字过时的字段危险得多 */
    Json::Object *pBoard = jsonItem(pRoot, "board");
    noteSection("board");
    if (nullptr != pBoard)
    {
        jsonReadBool(pBoard, "enable", stSnap.stBoard.bEnable, nBadType);
        jsonReadBool(pBoard, "episode", stSnap.stBoard.bRequireEpisode, nBadType);
        jsonReadInt64(pBoard, "reentry_ms", stSnap.stBoard.llIntervalMs, nBadType);
        /*「分析间隔时间」：同一板书 episode 内的重识别间隔（0=关） */
        jsonReadInt64(pBoard, "scan_interval_ms", stSnap.stBoard.llScanIntervalMs, nBadType);
        jsonReadInt64(pBoard, "inflight_timeout_ms", stSnap.stBoard.llInflightTimeoutMs, nBadType);
        jsonReadInt64(pBoard, "stay_exit_edge_ms", stSnap.stBoard.llStayExitEdgeMs, nBadType);
        jsonReadFloat(pBoard, "max_occlude_ratio", stSnap.stBoard.fMaxOccludeRatio, nBadType);
        jsonReadFloat(pBoard, "min_line_score", stSnap.stBoard.fMinLineScore, nBadType);
        jsonReadInt(pBoard, "text_max_bytes", stSnap.stBoard.nTextMaxBytes, nBadType);
        jsonReadInt(pBoard, "item_max", stSnap.stBoard.nItemMax, nBadType);
        jsonReadInt(pBoard, "line_max", stSnap.stBoard.nLineMax, nBadType);
        jsonReadBool(pBoard, "debug_text", stSnap.stBoard.bDebugText, nBadType);
    }

    /* ---- 板书 OCR 的 DB 后处理----
       这一段整块以前不存在：三道门与 box_max 全都取后端编译期默认值，
       而 ocr_decode.h 那句"配置可覆盖"说的就是这三道门——**从本刀起那句话才成立**。
       装配点在引擎 ocrAttach（每次热更都把这一份交给后端），一次作业内不会换门限 */
    Json::Object *pOcr = jsonItem(pRoot, "ocr");
    noteSection("ocr");
    if (nullptr != pOcr)
    {
        jsonReadFloat(pOcr, "mask_threshold", stSnap.stOcr.fMaskThreshold, nBadType);
        jsonReadFloat(pOcr, "box_threshold", stSnap.stOcr.fBoxThreshold, nBadType);
        jsonReadFloat(pOcr, "unclip_ratio", stSnap.stOcr.fUnclipRatio, nBadType);
        jsonReadInt(pOcr, "box_max", stSnap.stOcr.nBoxMax, nBadType);
    }

    Json::Object *pScene = jsonItem(pRoot, "scene");
    noteSection("scene");
    if (nullptr != pScene)
    {
        jsonReadBool(pScene, "enable", stSnap.stScene.bEnable, nBadType);
        jsonReadInt(pScene, "enter_ms", stSnap.stScene.nEnterMs, nBadType);
        jsonReadInt(pScene, "exit_ms", stSnap.stScene.nExitMs, nBadType);
        jsonReadInt(pScene, "min_segment_ms", stSnap.stScene.nMinSegmentMs, nBadType);
        jsonReadFloat(pScene, "raise_ratio_qa", stSnap.stScene.fRaiseRatioQa, nBadType);
        jsonReadFloat(pScene, "head_down_ratio_practice", stSnap.stScene.fHeadDownRatioPractice, nBadType);
        jsonReadBool(pScene, "teaching_needs_listen", stSnap.stScene.bTeachingNeedsListen, nBadType);
        jsonReadFloat(pScene, "ground_shift", stSnap.stScene.fGroundShiftHeads, nBadType);
    }

    /* ---- 输出侧 ---- */
    Json::Object *pEgress = jsonItem(pRoot, "egress");
    noteSection("egress");
    if (nullptr != pEgress)
    {
        jsonReadBool(pEgress, "osd_enable", stSnap.stEgress.bOsdEnable, nBadType);
        jsonReadInt(pEgress, "report_interval_sec", stSnap.stEgress.nReportIntervalSec, nBadType);
        jsonReadInt(pEgress, "session_max_sec", stSnap.stEgress.nSessionMaxSec, nBadType);
        jsonReadInt(pEgress, "session_silence_sec", stSnap.stEgress.nSilenceSec, nBadType);
        jsonReadBool(pEgress, "spool_enable", stSnap.stEgress.bSpoolEnable, nBadType);
        jsonReadString(pEgress, "spool_dir", stSnap.stEgress.strSpoolDir, nBadType);
        jsonReadInt(pEgress, "spool_max_mb", stSnap.stEgress.nSpoolMaxMb, nBadType);
        jsonReadInt(pEgress, "spool_seg_max_kb", stSnap.stEgress.nSpoolSegMaxKb, nBadType);
        jsonReadInt(pEgress, "spool_drain_max_kb", stSnap.stEgress.nSpoolDrainMaxKb, nBadType);
        jsonReadBool(pEgress, "metric_own_file", stSnap.stEgress.bMetricOwnFile, nBadType);
        jsonReadString(pEgress, "metric_file", stSnap.stEgress.strMetricFile, nBadType);
        jsonReadInt(pEgress, "metric_max_kb", stSnap.stEgress.nMetricMaxKb, nBadType);
        jsonReadInt(pEgress, "metric_max_files", stSnap.stEgress.nMetricMaxFiles, nBadType);
        jsonReadInt(pEgress, "metric_head_period_ms", stSnap.stEgress.nMetricHeadPeriodMs, nBadType);
    }

    /* ---- 平台统计推送----
       单独一节而不是塞进 egress：报表链路与平台推送的调参主体不同（前者是运维保数据，
       后者是平台要显示的时效），现场改一个不影响另一个 */
    Json::Object *pStats = jsonItem(pRoot, "stats");
    noteSection("stats");
    if (nullptr != pStats)
    {
        jsonReadBool(pStats, "enable", stSnap.stStats.bEnable, nBadType);
        jsonReadInt(pStats, "push_interval_ms", stSnap.stStats.nPushIntervalMs, nBadType);
        jsonReadInt(pStats, "seg_batch_ms", stSnap.stStats.nSegBatchMs, nBadType);
        jsonReadInt(pStats, "seg_batch_max", stSnap.stStats.nSegBatchMax, nBadType);
        jsonReadInt(pStats, "alert_batch_max", stSnap.stStats.nAlertBatchMax, nBadType);
        jsonReadInt(pStats, "lesson_min", stSnap.stStats.nLessonMin, nBadType);
        jsonReadInt(pStats, "arrive_grace_sec", stSnap.stStats.nArriveGraceSec, nBadType);
        jsonReadInt(pStats, "overrun_grace_min", stSnap.stStats.nOverrunGraceMin, nBadType);
        jsonReadInt(pStats, "leave_absent_sec", stSnap.stStats.nLeaveAbsentSec, nBadType);
        jsonReadInt(pStats, "patrol_interval_min", stSnap.stStats.nPatrolIntervalMin, nBadType);
        jsonReadInt(pStats, "participate_min_pct", stSnap.stStats.nParticipateMinPct, nBadType);
        jsonReadInt(pStats, "participate_hold_sec", stSnap.stStats.nParticipateHoldSec, nBadType);
        jsonReadInt(pStats, "discipline_bad_pct", stSnap.stStats.nDisciplineBadPct, nBadType);
        jsonReadInt(pStats, "discipline_hold_sec", stSnap.stStats.nDisciplineHoldSec, nBadType);
        jsonReadInt(pStats, "gather_num", stSnap.stStats.nGatherNum, nBadType);
        jsonReadFloat(pStats, "gather_radius_heads", stSnap.stStats.fGatherRadiusHeads, nBadType);
        jsonReadInt(pStats, "gather_hold_sec", stSnap.stStats.nGatherHoldSec, nBadType);
        jsonReadInt(pStats, "desk_hold_sec", stSnap.stStats.nDeskHoldSec, nBadType);
    }

    if (nBadType > 0)
    {
        /* 不拒绝整份配置：类型写错的字段留默认，但必须在日志里点名数量，
           否则「改了没生效」会变成排查半天的哑巴亏 */
        dlog_warn("classroom_config: %d 个键类型不符，保留默认", nBadType);
    }
}

bool CClassroomConfig::validate(CConfigSnapshot_S &stSnap, std::string &rstrErr)
{
    /* ---------- 硬错误：这些值一旦错，画面/坐标/内存预算整条链路都不可信 ---------- */
    if (stSnap.nSrcWidth <= 0 || stSnap.nSrcHeight <= 0 ||
        stSnap.nSrcWidth > 8192 || stSnap.nSrcHeight > 8192)
    {
        rstrErr = "video.src_width/src_height out of range";
        return false;
    }
    if (stSnap.nCanvasWidth <= 0 || stSnap.nCanvasHeight <= 0 ||
        stSnap.nCanvasWidth > 4096 || stSnap.nCanvasHeight > 4096)
    {
        rstrErr = "video.canvas_width/canvas_height out of range";
        return false;
    }
    if (stSnap.nRotate != 0 && stSnap.nRotate != 90 && stSnap.nRotate != 180 && stSnap.nRotate != 270)
    {
        /* CFrameGeometry::setup 对非 90 倍数直接返回 false；放到这里拒绝，
           比让它「静默不旋转」好——后者表现为整场行为判反，无人会怀疑到配置 */
        rstrErr = "rotate must be 0/90/180/270";
        return false;
    }
    const int nMirror = static_cast<int>(stSnap.enMirror);
    if (nMirror < 0 || nMirror > static_cast<int>(MirrorMode_E::MIRROR_BOTH))
    {
        rstrErr = "mirror_mode out of range";
        return false;
    }
    const int nRole = static_cast<int>(stSnap.enRole);
    if (nRole < static_cast<int>(DeviceRole_E::ROLE_TEACHER) ||
        nRole > static_cast<int>(DeviceRole_E::ROLE_BOTH))
    {
        /* 拼错的 role 绝不静默当 both 跑：现场以为自己下发了教师机，考勤却一直在记，
           两侧各自一份账没人对得上。这句话要含全部三个合法值，让运维不用翻文档 */
        rstrErr = "role must be teacher|student|both";
        return false;
    }
    const int nSpace = static_cast<int>(stSnap.enRegionSpace);
    if (nSpace != static_cast<int>(CoordSpace_E::COORD_SPACE_PIXEL) &&
        nSpace != static_cast<int>(CoordSpace_E::COORD_SPACE_NORM_8192))
    {
        rstrErr = "region_ref.coord_space out of range";
        return false;
    }
    const int nChannel = static_cast<int>(stSnap.enModelChannel);
    if (nChannel != static_cast<int>(ChannelOrder_E::CH_RGB) && nChannel != static_cast<int>(ChannelOrder_E::CH_BGR))
    {
        /* 越界值会直接落到 rga_image_transform 的 dst_format 上，得到一个未定义的像素格式 */
        rstrErr = "preprocess.channel_order must be 0(RGB) or 1(BGR)";
        return false;
    }
    if (stSnap.nRegionRefWidth <= 0 || stSnap.nRegionRefHeight <= 0)
    {
        rstrErr = "region_ref.width/height must be positive";
        return false;
    }
    /* 时长门是定长 CLASSROOM_MAX_PERSONS 表，超出会静默取模复用他人槽位 —— 后果是张冠李戴，必须硬拒 */
    if (stSnap.nMaxPersons <= 0 || stSnap.nMaxPersons > CLASSROOM_MAX_PERSONS)
    {
        rstrErr = "max_persons out of (0, " + std::to_string(CLASSROOM_MAX_PERSONS) + "]";
        return false;
    }
    if (stSnap.fPoseScoreThreshold < 0.0f || stSnap.fPoseScoreThreshold > 1.0f)
    {
        rstrErr = "threshold.pose_score must be in [0,1]";
        return false;
    }
    if (stSnap.nMinFacePixel <= 0 || stSnap.nMinFacePixel > 4096)
    {
        rstrErr = "threshold.min_face_px out of range";
        return false;
    }
    /* 0 合法：关掉这道闸，让所有检出的人脸都往下走。与上一项（命名门）不给 0 是两回事——
       关掉命名门的后果是"谁都别想有名字"，关掉这道的后果只是多解几张废脸 */
    if (stSnap.nMinFaceDetectPixel < 0 || stSnap.nMinFaceDetectPixel > 4096)
    {
        rstrErr = "threshold.min_face_detect_px out of range";
        return false;
    }
    if (stSnap.nMinFaceDetectPixel > stSnap.nMinFacePixel)
    {
        /* 关系硬错误而不是钳制：这两个数是两台机器上分别调出来的，替现场挑一个赢家
           等于悄悄改掉另一条他们刚下发的判据。现场真正撞上的形态是"到课人数恒 0"，
           这一句直接把原因说出来 */
        rstrErr = "threshold.min_face_detect_px must be <= min_face_px, otherwise faces are "
                  "dropped before the attendance gate can ever see them";
        return false;
    }
    if (stSnap.fDetectScoreThreshold <= 0.0f || stSnap.fDetectScoreThreshold >= 1.0f)
    {
        rstrErr = "threshold.det_score must be in (0,1)";
        return false;
    }
    if (stSnap.fDetectNmsThreshold <= 0.0f || stSnap.fDetectNmsThreshold >= 1.0f)
    {
        rstrErr = "threshold.det_nms must be in (0,1)";
        return false;
    }
    if (stSnap.nMinHeadPixel <= 0 || stSnap.nMinHeadPixel > 4096)
    {
        rstrErr = "threshold.min_head_px out of range";
        return false;
    }
    /* max 允许 0（=关，旧形态），开着时必须真的能放下 min——
       max<=min 的配置等于把所有头全拒（全班消失且没有任何一处报错），
       这种错宁可整份拒绝 */
    if (stSnap.nMaxHeadPixel < 0 || stSnap.nMaxHeadPixel > 4096)
    {
        rstrErr = "threshold.max_head_px out of range";
        return false;
    }
    if (stSnap.nMaxHeadPixel > 0 && stSnap.nMaxHeadPixel <= stSnap.nMinHeadPixel)
    {
        rstrErr = "threshold.max_head_px must be > min_head_px when enabled";
        return false;
    }
    if (stSnap.nMinBodyPixel <= 0 || stSnap.nMinBodyPixel > 4096)
    {
        rstrErr = "threshold.min_body_px out of range";
        return false;
    }
    /* 远带人头（#90）：edge 允许 0（分带关闭 = 旧形态），也允许 <= min_head_px
       （远带空集，同样回落旧形态）——这两档都是现场显式关掉折扣的合法姿势。
       ratio 不给这个余地：>=1 的"折扣"会把远带门抬得比基门还严，
       与这条门存在的目的整个相反，宁可整份拒绝 */
    if (stSnap.nHeadFarEdgePx < 0 || stSnap.nHeadFarEdgePx > 4096)
    {
        rstrErr = "threshold.head_far_edge_px out of range";
        return false;
    }
    if (stSnap.fHeadFarGateRatio <= 0.0f || stSnap.fHeadFarGateRatio >= 1.0f)
    {
        rstrErr = "threshold.head_far_gate_ratio must be in (0,1)";
        return false;
    }
    if (stSnap.stSample.nNpuBusyHighPercent < 0 || stSnap.stSample.nNpuBusyHighPercent > 100 ||
        stSnap.stSample.nNpuBusyLowPercent < 0 || stSnap.stSample.nNpuBusyLowPercent > 100 ||
        stSnap.stSample.nNpuBusyLowPercent >= stSnap.stSample.nNpuBusyHighPercent)
    {
        /* 迟滞区间反向 = 降采样每几秒抖一次，是比"性能差"更难查的问题 */
        rstrErr = "sampling.npu_busy_low must be < npu_busy_high in [0,100]";
        return false;
    }
    /* 考勤参数：判定函数就是账本装载时用的那一个（behavior/attendance.cpp）。
       配置层不再抄一套区间：两套区间迟早会不一致，而不一致的表现是
       "这个值在配置里被拒、在别处却被接受"，现场无法解释。
       硬拒绝（整份配置不生效、旧的继续跑）而不是钳制：这几个数没有一个有物理边界，
       替现场挑一个值就是替他们决定"到课要呆够多久" */
    std::string strAttendErr;
    if (!attendanceParamOk(stSnap.stAttend, strAttendErr))
    {
        rstrErr = "attendance: " + strAttendErr;
        return false;
    }
    /* 座位热力：同上，判据只有一份（behavior/seat_heat.cpp 装载时走同一个函数）。
       这一块的 cols/rows 同时是累加数组的编译期上界，绕过这里直接 setParam 的人
       会在 rebuildGrid 里被第二道同判据的闸门挡住——两道判一个真值，不会因为多一道而漂移 */
    std::string strSeatErr;
    if (!seatHeatParamOk(stSnap.stSeat, strSeatErr))
    {
        rstrErr = "seat_heat: " + strSeatErr;
        return false;
    }
    /* 座位先验。形状判据不在这里再写一套；那条**跨块算术**也刻意不放在这里——
       此刻 sampling.group_interval_ms 还没钳完，判的是一个这台机器根本不会用的数，
       它的落点在文件末尾所有钳位之后 */
    std::string strPriorErr;
    if (!seatPriorParamOk(stSnap.stPrior, strPriorErr))
    {
        rstrErr = "seat_prior: " + strPriorErr;
        return false;
    }

    /* ---------- 人脸对齐 / 底库 / 取特征 ---------- */
    if (stSnap.nAlignPointOrder < 0 || stSnap.nAlignPointOrder >= CLASSROOM_ALIGN_PERM_NUM)
    {
        /* 越界不钳 */
        rstrErr = "align.point_order out of [0," + std::to_string(CLASSROOM_ALIGN_PERM_NUM - 1) + "]";
        return false;
    }
    if (!(stSnap.stAlign.fScaleMin > 0.0f) || !(stSnap.stAlign.fScaleMax > stSnap.stAlign.fScaleMin))
    {
        /* 反向的一对数没有"就近钳一下"的说法：fScaleMax 抬到 fScaleMin 之上是替现场决定门限，
           而这两道界哪个该让步取决于机位与模板尺寸，只有下发的人知道 */
        rstrErr = "align.scale_min must be > 0 and < scale_max";
        return false;
    }
    if (!(stSnap.stAlign.fMaxResidualRatio > 0.0f))
    {
        /* 残差门的判据是 rms <= ratio × 模板瞳距，ratio=0 在实现里是"只有五点完全重合才通过"，
           也就是**整段永不通过**——而 0 在配置里长得像"关掉这道闸"。
           这一条必须硬拒并说清方向：想放开这道闸请配一个大数，它自然永不触发。
           关掉它的后果是镜像/点序错配的脸全部进识别，特征照算、名单照错，且没有任何失败计数 */
        rstrErr = "align.max_residual_ratio must be > 0: 0 is not 'off', it rejects every face "
                  "(use a large value such as 10 if the gate is really unwanted)";
        return false;
    }
    /* 表情：判据同样只有一份（感知层 setParam 时走的是同一个函数）。这里不区分 enable
       真假再决定要不要查——关着时放过一张坏映射表，等到某天把它打开的那一刻，
       设备读到的是"槽位 2 是惊讶"而下发的人以为写的是"开心"，而这份配置早就通过了校验、
       没有任何一处会再报。宁可让改这个旋钮的人在下发时就拿到一条原因 */
    std::string strExprErr;
    if (!expressionParamOk(stSnap.stExpr.stParam, strExprErr))
    {
        rstrErr = "expression: " + strExprErr;
        return false;
    }
    if ("file" != stSnap.stGallery.strSource && "sqlite" != stSnap.stGallery.strSource &&
        "none" != stSnap.stGallery.strSource)
    {
        /* 解析处已经把过一道，这里是第二道：单测与课表/远程下发可能直接构造快照，
           那一路上没有解析器。两道同判一个真值，不会因为多一道而漂移 */
        rstrErr = "gallery.source must be file/sqlite/none, got: " + stSnap.stGallery.strSource;
        return false;
    }
    if ("file" == stSnap.stGallery.strSource &&
        (stSnap.stGallery.strPath.empty() || '/' != stSnap.stGallery.strPath[0]))
    {
        /* 相对路径按进程 cwd 解析（与 spool_dir 同一坑）。这里不"退回默认值"而是拒：
           默认路径上可能真的躺着一份旧名单，静默退回去会把一份过期名单激活 */
        rstrErr = "gallery.path must be absolute";
        return false;
    }
    /* 空表名是合法值，含义是「全部名单组」（适配器据此走 getAllData），
       而 galleryTableOk 判的是"这个词能不能当标识符"——空串在那边必然返回 false，
       故这里必须先放行空值，否则跨组考勤这个用法一配就整份配置被拒 */
    if ("sqlite" == stSnap.stGallery.strSource &&
        !stSnap.stGallery.strTable.empty() && !galleryTableOk(stSnap.stGallery.strTable))
    {
        /**
         * 表名会被公共仓直接拼进 SQL（face_sqlite.cpp 的 SELECT * FROM "<表名>"），
         * 没有参数化可走，所以这里那句白名单是这条路上唯一的闸门。
         * 错误串只给长度不给内容：表名是平台侧下发的，回显它等于把一份名单组的命名
         * 抄进日志，而配错的表名往往正是那种带引号带分号的串
         */
        rstrErr = "gallery.table rejected (" + std::to_string(stSnap.stGallery.strTable.size())
                + " byte(s), only letters/digits/underscore or UTF-8 names allowed)";
        return false;
    }

    if (stSnap.vecModels.size() > static_cast<size_t>(CLASSROOM_MAX_MODELS))
    {
        rstrErr = "too many models: " + std::to_string(stSnap.vecModels.size());
        return false;
    }
    for (size_t i = 0; i < stSnap.vecModels.size(); ++i)
    {
        const CModelDesc_S &stDesc = stSnap.vecModels[i];
        if (stDesc.strKey.empty() || stDesc.strModelPath.empty())
        {
            rstrErr = "models[" + std::to_string(i) + "] missing key or model_path";
            return false;
        }

        if (!galleryTableOk(stDesc.strKey))
        {
            rstrErr = "models[" + std::to_string(i) + "].key rejected ("
                    + std::to_string(stDesc.strKey.size())
                    + " byte(s), only letters/digits/underscore or UTF-8 names allowed)";
            return false;
        }
        if ('/' != stDesc.strModelPath.front())
        {
            /* 相对路径按进程工作目录解析，与 JSON 所在目录无关 —— ai_share 的资产债 #2 就是这么来的 */
            rstrErr = "models[" + stDesc.strKey + "].model_path must be absolute";
            return false;
        }
        const int nGroup = static_cast<int>(stDesc.enGroup);
        if (nGroup < static_cast<int>(ModelGroup_E::GROUP_BASE) || nGroup > static_cast<int>(ModelGroup_E::GROUP_OTHER))
        {
            rstrErr = "models[" + stDesc.strKey + "].group out of range";
            return false;
        }
        if (CLASSROOM_BACKEND_YOLOV5 != stDesc.strBackend &&
            CLASSROOM_BACKEND_YOLOV8 != stDesc.strBackend &&
            CLASSROOM_BACKEND_YOLOV8_POINT != stDesc.strBackend &&
            CLASSROOM_BACKEND_RAW != stDesc.strBackend)
        {
            /* 未知后端名必须当场拒：拼错一个字母（yolov8n / YOLOv8）若静默退化成 raw，
               现象是"模型在、框全不出"，比启动失败难查一个量级 */
            rstrErr = "models[" + stDesc.strKey + "].backend unknown: " + stDesc.strBackend;
            return false;
        }
        if (CLASSROOM_BACKEND_RAW != stDesc.strBackend && stDesc.strJsonPath.empty())
        {
            /* 封装类后端（yolov5/yolov8/yolov8_point）的解码参数——类别数、锚点、point_num——
               **只写在 json 里**，json_path 为空时 CRkModelHandle::create 直接返回 nullptr。
               那一格不是崩溃而是 LOAD_FAILED（可观测），但可观测的位置在池子的状态列，
               而错的位置在配置里；能在配置这一层说出"哪一行的哪个键空了"就不要留到运行期。
               raw 反过来允许空：CRawModelHandle 不读 json（见板书那两行的注释） */
            rstrErr = "models[" + stDesc.strKey + "].json_path required for backend " + stDesc.strBackend;
            return false;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (stSnap.vecModels[j].strKey == stDesc.strKey)
            {
                /* 同键两行会被 registerModel 拒掉第二行，与其让池子少一个模型，不如当场拒绝配置 */
                rstrErr = "duplicated model key: " + stDesc.strKey;
                return false;
            }
        }
    }

    /* 反向守卫：整表替换把上一版配置里**多出来**的键也原样收下，
       池子把没人 acquire 的模型真加载——摘件后换件，resident 当场回到 165128KB/7，
       NPU 白吃 15MB（键名打错是同一种浪费）。过滤放在逐行硬校验**之后**：未注册键的行
       先过同一套校验（空路径/坏组/坏后端/重键照样整份拒，别让"丢弃"把配置错误顺手吞了），
       过了校验的未注册行才整行丢弃——本工程对未注册键没有任何 acquire 读者，
       与未知标量键「按未知丢弃」同口径 */
    size_t nKeptRows = 0;
    for (size_t i = 0; i < stSnap.vecModels.size(); ++i)
    {
        bool bKnownKey = false;
        for (int nDef = 0; nDef < MODEL_DEFAULT_NUM; ++nDef)
        {
            if (stSnap.vecModels[i].strKey == AST_MODEL_DEFAULTS[nDef].pszKey)
            {
                bKnownKey = true;
                break;
            }
        }
        if (bKnownKey)
        {
            if (nKeptRows != i)
            {
                stSnap.vecModels[nKeptRows] = stSnap.vecModels[i];
            }
            ++nKeptRows;
        }
        else
        {
            dlog_warn("classroom_config: models[] 键 %s 不在内置表——整行丢弃"
                      "（#96 已摘或键名打错；没人 acquire 的模型加载了也只是白占 NPU）",
                      stSnap.vecModels[i].strKey.c_str());
        }
    }
    if (nKeptRows != stSnap.vecModels.size())
    {
        stSnap.vecModels.resize(nKeptRows);
    }

    /* ---------- 分析区域：画歪了不会崩，只会让该区域里的行为永远 UNKNOWN 或永远命中，
       所以跨表一致性必须在这里当场拒掉，不留到运行期靠日志反查 ---------- */
    if (stSnap.vecRegions.size() > static_cast<size_t>(CLASSROOM_MAX_REGIONS))
    {
        rstrErr = "too many regions: " + std::to_string(stSnap.vecRegions.size());
        return false;
    }
    const bool bNormSpace = (CoordSpace_E::COORD_SPACE_NORM_8192 == stSnap.enRegionSpace);
    const float fMaxX = bNormSpace ? CLASSROOM_REGION_NORM_MAX
                                   : static_cast<float>(stSnap.nRegionRefWidth) - 1.0f;
    const float fMaxY = bNormSpace ? CLASSROOM_REGION_NORM_MAX
                                   : static_cast<float>(stSnap.nRegionRefHeight) - 1.0f;
    bool bCoordClamped = false;

    for (size_t i = 0; i < stSnap.vecRegions.size(); ++i)
    {
        CRegionDesc_S &stDesc = stSnap.vecRegions[i];
        const std::string strTag = "regions[" + std::to_string(i) + "]";

        if (stDesc.nId < 0)
        {
            rstrErr = strTag + ": id must be >= 0";
            return false;
        }
        const int nShape = static_cast<int>(stDesc.enShape);
        if (nShape != static_cast<int>(RegionShape_E::REGION_RECT) &&
            nShape != static_cast<int>(RegionShape_E::REGION_POLY))
        {
            rstrErr = strTag + ".shape out of range";
            return false;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (stSnap.vecRegions[j].nId == stDesc.nId)
            {
                /* 重复 id 让「这条行为引用的是哪块区域」变成不可答问题：留第一条拒第二条，
                   并在日志里说明，比两条都留（后建者覆盖前建者）明确 */
                rstrErr = "duplicated region id: " + std::to_string(stDesc.nId);
                return false;
            }
        }

        if (RegionShape_E::REGION_RECT == stDesc.enShape)
        {
            if (stDesc.stRectWeb.fX1 == 0.0f && stDesc.stRectWeb.fY1 == 0.0f &&
                stDesc.stRectWeb.fX2 == 0.0f && stDesc.stRectWeb.fY2 == 0.0f)
            {
                /* 四值全零＝解析时一个数都没读到（Web 误发成 [x1,y1,x2,y2] 数组、或整段漏了
                   这个键）。这与「画反了」是两个病因：报成 inverted 会让人去核明明正常的数字，
                   而真正的错误形态在报文里一个字都没提 */
                rstrErr = strTag + ".rect missing or malformed (want {x1,y1,x2,y2})";
                return false;
            }
            if (stDesc.stRectWeb.fX2 <= stDesc.stRectWeb.fX1 ||
                stDesc.stRectWeb.fY2 <= stDesc.stRectWeb.fY1)
            {
                rstrErr = strTag + ".rect is inverted or empty";
                return false;
            }
        }
        else
        {
            const size_t nPts = stDesc.vecPointsWeb.size();
            if (nPts < 3 || nPts > static_cast<size_t>(CLASSROOM_REGION_MAX_POINTS))
            {
                rstrErr = strTag + ".points out of [3," + std::to_string(CLASSROOM_REGION_MAX_POINTS) + "]";
                return false;
            }
        }

        /* 越界钳制而非拒绝：Web 划线时分辨率对不上是常见误操作，画到画面外那一截没有意义，
           但为此整份配置被拒会让现场以为「下发失败」而去重发全部参数 */
        bCoordClamped |= clampInPlace(stDesc.stRectWeb.fX1, 0.0f, fMaxX);
        bCoordClamped |= clampInPlace(stDesc.stRectWeb.fY1, 0.0f, fMaxY);
        bCoordClamped |= clampInPlace(stDesc.stRectWeb.fX2, 0.0f, fMaxX);
        bCoordClamped |= clampInPlace(stDesc.stRectWeb.fY2, 0.0f, fMaxY);
        for (size_t p = 0; p < stDesc.vecPointsWeb.size(); ++p)
        {
            bCoordClamped |= clampInPlace(stDesc.vecPointsWeb[p].fX, 0.0f, fMaxX);
            bCoordClamped |= clampInPlace(stDesc.vecPointsWeb[p].fY, 0.0f, fMaxY);
        }
    }

    for (size_t r = 0; r < stSnap.vecRegionRoles.size(); ++r)
    {
        const CRegionRoleRef_S &stRef = stSnap.vecRegionRoles[r];
        const int nRole = static_cast<int>(stRef.enRole);

        if (nRole < 0 || nRole >= static_cast<int>(RegionRole_E::REGION_ROLE_NUM))
        {
            rstrErr = "region_roles[" + std::to_string(r) + "].role out of range";
            return false;
        }
        for (size_t k = 0; k < stRef.vecRegionIds.size(); ++k)
        {
            if (nullptr == findRegionDesc(stSnap.vecRegions, stRef.vecRegionIds[k]))
            {
                /* 方括号里必须是行号：旧码填的是 role 值，一份两行的 roles 表里
                   第二行 role=0 报错，现场会去改第一行，怎么改都不消失 */
                rstrErr = "region_roles[" + std::to_string(r) + "].role="
                        + std::to_string(nRole) + " refers missing region id "
                        + std::to_string(stRef.vecRegionIds[k]);
                return false;
            }
        }
    }

    for (size_t c = 0; c < stSnap.vecBehaviorRegions.size(); ++c)
    {
        const std::vector<int32_t> &vecIds = stSnap.vecBehaviorRegions[c].vecRegionIds;
        for (size_t k = 0; k < vecIds.size(); ++k)
        {
            if (nullptr == findRegionDesc(stSnap.vecRegions, vecIds[k]))
            {
                /* 引用不存在的区域：判定时只能给 TS_UNKNOWN，表现为「这条行为配了却永不出事件」。
                   运行期确实可能出现（总线几何未就绪时区域表是空的），但那是暂时的；
                   配置里就写错属于必须当场说清的那类错 */
                rstrErr = "behaviors col " + std::to_string(c) + " refers missing region id "
                        + std::to_string(vecIds[k]);
                return false;
            }
        }
    }

    /* ---------- 页 16「绘制区域」覆盖讲台角色 ----------
       210 的判据是「讲台区域里有没有人」，讲台范围原先只能由 region_roles 的
       role=podium 给；网页在 210 页画的框如果只写进 behaviors.210.region_ids，
       仲裁器看不见——典型表现就是"页面画了框，210 照样按老讲台判"。
       这里在引用校验之后把它并进 podium 角色：页面画了框，它就是这个设备眼里的讲台；
       页面没画（空表）保持角色表原样，出厂态与旧配置逐字节不变。
       同时清掉同角色的其余行：idsOfRole 会把同角色的多行并起来，留着旧行等于没覆盖 */
    {
        const int nPatrolCol = CDurationGate::behaviorColumn(Behavior_E::TEA_PATROL);
        if (nPatrolCol >= 0 && nPatrolCol < static_cast<int>(stSnap.vecBehaviorRegions.size()))
        {
            const std::vector<int32_t> &vecPageIds =
                stSnap.vecBehaviorRegions[static_cast<size_t>(nPatrolCol)].vecRegionIds;
            if (!vecPageIds.empty())
            {
                bool bAssigned = false;
                for (size_t r = 0; r < stSnap.vecRegionRoles.size(); ++r)
                {
                    CRegionRoleRef_S &stRef = stSnap.vecRegionRoles[r];
                    if (RegionRole_E::REGION_ROLE_PODIUM != stRef.enRole)
                    {
                        continue;
                    }
                    if (!bAssigned)
                    {
                        stRef.vecRegionIds = vecPageIds;
                        bAssigned = true;
                    }
                    else
                    {
                        stRef.vecRegionIds.clear();
                    }
                }
                if (!bAssigned)
                {
                    CRegionRoleRef_S stNew;
                    stNew.enRole = RegionRole_E::REGION_ROLE_PODIUM;
                    stNew.vecRegionIds = vecPageIds;
                    stSnap.vecRegionRoles.push_back(stNew);
                }
            }
        }
    }

    /* ---------- 软错误：钳制到边界并记名，不拒绝整份配置 ---------- */
    std::string strSoft;

    /* ---------- 设备角色互斥（#126）：role 赢过本文件里任何单项 enable ----------
       只做强制关、不做强制开：开哪路功能仍由现场逐项配（教师机的 board.enable、
       学生机的 attendance.enable 都等运维下发），role 只保证「不该有的不会有」。
       不做硬拒的理由：reloadFromText 是部分 JSON，显式配的 true 与默认 true 不可区分，
       按「越权开关出现即拒」会把 both 时代的存量整份配置全部打死。
       记 strSoft 而不另起 warn：与其它钳制同一口径，「我改了什么」只有这一个出口 */
    if (DeviceRole_E::ROLE_BOTH != stSnap.enRole)
    {
        const bool bTeacher = (DeviceRole_E::ROLE_TEACHER == stSnap.enRole);
        /* 行为表列序是算术映射（duration_gate.h）：学生列 [0, STUDENT_COLUMN_NUM)、
           教师列紧随其后——关一整侧就是关一段连续列，不必逐枚举点名 */
        int nGatesOff = 0;
        const int nColFrom = bTeacher ? 0 : CDurationGate::STUDENT_COLUMN_NUM;
        const int nColTo = bTeacher ? CDurationGate::STUDENT_COLUMN_NUM
                                    : CDurationGate::BEHAVIOR_COLUMN_NUM;
        const int nColEnd = nColTo < static_cast<int>(stSnap.vecGates.size())
                                ? nColTo : static_cast<int>(stSnap.vecGates.size());
        for (int c = nColFrom; c < nColEnd; ++c)
        {
            if (stSnap.vecGates[static_cast<size_t>(c)].bEnable)
            {
                stSnap.vecGates[static_cast<size_t>(c)].bEnable = false;
                ++nGatesOff;
            }
        }
        /* 207 指导 / 209 互动 是**学生机**上的班级级列，编号却落在
           教师段里（行为编号连续，不按设备归属分段）。学生机那一圈只关 [0,12)，
           碰不到它们，它们照旧在校验后的表里；教师机那一圈也不关教师段，
           所以这里单独把教师机上的这两列关掉——教师机前没有学生画面，
           开着只会得到一条永远 UNKNOWN 的列。210 巡视在教师段里，教师机保留、
           学生机随教师段整段关掉（归属总开关在学生机上也是关的） */
        if (bTeacher)
        {
            for (const Behavior_E enStudentCam : { Behavior_E::TEA_GUIDING, Behavior_E::TEA_INTERACT })
            {
                const int nCol = CDurationGate::behaviorColumn(enStudentCam);
                if (nCol >= 0 && nCol < static_cast<int>(stSnap.vecGates.size()) &&
                    stSnap.vecGates[static_cast<size_t>(nCol)].bEnable)
                {
                    stSnap.vecGates[static_cast<size_t>(nCol)].bEnable = false;
                    ++nGatesOff;
                }
            }
        }
        int nBlocksOff = 0;
        if (bTeacher)
        {
            /* 教师机关考勤：到课/名单是学生机的事，同一间教室两台机器各记一份考勤
               没有读者，底库比对还白烧 NPU */
            if (stSnap.stAttend.bEnable)
            {
                stSnap.stAttend.bEnable = false;
                ++nBlocksOff;
            }
            /* 教师机关学生侧功能段：gallery/face_feature/expression/seat
               都是学生机的链路（命名/表情/座位），教师机上无人消费还白烧 NPU。
               phone_recheck 不关：205 教师玩手机复核用的是同一颗 */
            if (stSnap.stGallery.bEnable)
            {
                stSnap.stGallery.bEnable = false;
                ++nBlocksOff;
            }
            if (stSnap.stFaceFeat.bEnable)
            {
                stSnap.stFaceFeat.bEnable = false;
                ++nBlocksOff;
            }
            if (stSnap.stExpr.bEnable)
            {
                stSnap.stExpr.bEnable = false;
                ++nBlocksOff;
            }
            if (stSnap.stSeat.bEnable)
            {
                stSnap.stSeat.bEnable = false;
                ++nBlocksOff;
            }
        }
        else
        {
            /* 学生机关板书 OCR（按需加载那条链的入口闸，warm 策略读的就是它）
               与教师身份整段（身份不认，201~213 那段判据随之整列全关——
               例外是 207/209：它们由学生画面驱动，只是编号借住在教师段里，见上） */
            if (stSnap.stBoard.bEnable)
            {
                stSnap.stBoard.bEnable = false;
                ++nBlocksOff;
            }
            if (stSnap.stTeacher.bEnable)
            {
                stSnap.stTeacher.bEnable = false;
                ++nBlocksOff;
            }
        }
        if (nGatesOff + nBlocksOff > 0)
        {
            strSoft += std::string(" role=") + deviceRoleName(stSnap.enRole)
                     + "(forced off: behaviors x" + std::to_string(nGatesOff)
                     + ", blocks x" + std::to_string(nBlocksOff) + ")";
        }
    }

    if (bCoordClamped)
    {
        strSoft += " regions[].coord";
    }
    CLAMP_WARN(stSnap.fBodyExpandW, 0.0f, 20.0f, "body_expand.w");
    CLAMP_WARN(stSnap.fBodyExpandDown, 0.0f, 30.0f, "body_expand.down");
    CLAMP_WARN(stSnap.fBodyExpandUp, 0.0f, 30.0f, "body_expand.up");
    /* 0 是合法值（只有框完全出画才算贴边），故下限取 0 而不是 1 */
    CLAMP_WARN(stSnap.fOccludeIou, 0.0f, 0.95f, "threshold.occlude_iou");
    CLAMP_WARN(stSnap.nEdgeBandPx, 0, 128, "threshold.edge_band_px");
    CLAMP_WARN(stSnap.fBlurFullGrad, 1.0f, 128.0f, "threshold.blur_full_grad");
    /* 0 合法：关掉清晰度闸。1 也合法但等于「只有完全清晰才判」，现场会看到 SR_BLUR 吃满 */
    CLAMP_WARN(stSnap.fPoseBlurMin, 0.0f, 1.0f, "threshold.pose_blur_min");
    /* gate=0 会把每一次匹配都拒掉：表现为"人数每帧翻倍又清零"，
       比"跟不准"严重得多，所以下限必须留出一条能匹配的距离 */
    CLAMP_WARN(stSnap.stTracker.fGateNormDist, 0.05f, 3.0f, "tracker.gate_norm_dist");
    CLAMP_WARN(stSnap.stTracker.fMoveSlackPxPerMs, 0.0f, 5.0f, "tracker.move_slack_px_ms");
    /* 负权重会把"尺寸不符"反向变成"尺寸不符优先"——那等于故意要跟错人，直接钳没 */
    CLAMP_WARN(stSnap.stTracker.fSizePenalty, 0.0f, 10.0f, "tracker.size_penalty");
    CLAMP_WARN(stSnap.stTracker.nMinHits, 1, 30, "tracker.min_hits");
    CLAMP_WARN(stSnap.stTracker.nKeepMs, 0, 60000, "tracker.keep_ms");
    /* 下限 20ms：比一次推理还短的等待等于每拍必超时，整条感知链路会静默变成"永远 0 人" */
    CLAMP_WARN(stSnap.stSample.nInferWaitMs, 20, 2000, "sampling.infer_wait_ms");
    CLAMP_WARN(stSnap.stSample.nHeadIntervalMs, 0, 60000, "sampling.head_interval_ms");
    CLAMP_WARN(stSnap.stSample.nPoseIntervalMs, 0, 60000, "sampling.pose_interval_ms");
    CLAMP_WARN(stSnap.stSample.nFaceIntervalMs, 0, 60000, "sampling.face_interval_ms");
    CLAMP_WARN(stSnap.stSample.nMaxPosePersons, 1, CLASSROOM_MAX_PERSONS, "sampling.max_pose_persons");
    /* 下限留 0：配 0 就是「这一路整段不做」，是现场 NPU 不够时的合法逃生阀
       （规则层据此输出 UNKNOWN，而不是硬判一个姿态）。上限 500ms：再大就等于允许姿态
       一段连吃掉两三个节拍，而被挤掉的每一拍都是人头检测——人数才是所有指标的基数 */
    CLAMP_WARN(stSnap.stSample.nPoseBeatBudgetMs, 0, 500, "sampling.pose_beat_budget_ms");
    /* 下限 1s：比一次感知节拍还短的群体采样只是把同一帧重复积分，
       账本不会更准，却会让会话累计量按拍数虚增 */
    CLAMP_WARN(stSnap.stSample.nGroupIntervalMs, 1000, 60000, "sampling.group_interval_ms");
    /* ---- 主体归属 ----
       下限 1000ms：比一次姿态刷新还短的确认时长，等于「有一拍证据就是老师」。
       强判据是头框落在讲台里，而学生路过讲台也就一两拍——确认时长一短，
       归属跟着人走，教师那 13 列就变成「谁的走动多谁当老师」 */
    CLAMP_WARN(stSnap.stTeacher.nConfirmMs, 1000, 60000, "teacher.confirm_ms");
    /* 下限 1000ms 的理由同上，但更要紧的是下面那条交叉修正：放弃时长比确认时长还短，
       会出现「攒 5 秒才当上老师，断 1 秒就被开除」，而断的那一秒多半只是被前排挡了一下 */
    CLAMP_WARN(stSnap.stTeacher.nReleaseMs, 1000, 120000, "teacher.release_ms");
    /* 下限 1 而不是 0：关掉请用 enable。max_num=0 的现象是"区域画了、开关开了、教师恒为 0"，
       与配置写错无法区分，而 enable=false 在观测里是一眼能看出来的 */
    CLAMP_WARN(stSnap.stTeacher.nMaxNum, 1, CLASSROOM_MAX_PERSONS, "teacher.max_num");
    if (stSnap.stTeacher.nReleaseMs < stSnap.stTeacher.nConfirmMs)
    {
        /* 与 egress 那对（session_max_sec / report_interval_sec）同一处置方式：就地抬高并留线索，
           而不是拒绝整份配置——这两个数在现场是成对调的，拒一次就得让 Web 重发一遍 */
        stSnap.stTeacher.nReleaseMs = stSnap.stTeacher.nConfirmMs;
        strSoft += " teacher.release_ms(raised to confirm_ms)";
    }
    /* 下限不给 0：coverage=0 在区域判定里是「沾到边就算」，讲台区域画大一点就等于全员在讲台，
       归属整条反向。上限 1.0 是「整个头框都在内」，实际用法里 0.5~0.8 就够 */
    CLAMP_WARN(stSnap.stTeacher.fCoverage, 0.05f, 1.0f, "teacher.coverage");
    /* 下限 30px：人头框每拍的检测抖动在个位数像素，5s 窗口里随机游走就能凑出更大的数。
       配 0 等于关掉位移这条否决——全班起立问好那 30 秒里，仲裁会挑证据最连续的一个学生当老师，
       而那个人同时停止学生侧记账（本块最容易配错、又最难从现象倒推的一条） */
    CLAMP_WARN(stSnap.stTeacher.nPatrolMovePx, 30, 2000, "teacher.patrol_move_px");
    /* 上限与仲裁器里的 CLASSROOM_MOVE_WINDOW_MAX 同值：窗口再长，位移就不再是"这段时间走动过"
       而是"这节课走过"，巡视判据随之失效（那边还留了一道兜底，因为单测里手搓快照不过 validate） */
    CLAMP_WARN(stSnap.stTeacher.nPatrolWindowMs, 1000, 30000, "teacher.patrol_window_ms");
    /* 静止指导证据 0=关是默认档（失败安全：新判据不上电），下限给 0 不给"一次指导的量级"；
       上限按归属放弃时长的量级给——比 release_ms 还长的停留证据意味着身份总在放弃边缘续命 */
    CLAMP_WARN(stSnap.stTeacher.nStayEvidenceMs, 0, 600000, "teacher.stay_evidence_ms");
    /* 缺席容忍 0=关是默认档（失败安全：旧行为=缺一拍当拍收）。上限 30s 是"人还没走远"的量级：
       比它还长的容忍等于把出画当在场，身份收不回、tea_track 也发不出 END */
    CLAMP_WARN(stSnap.stTeacher.nAbsentHoldMs, 0, 30000, "teacher.absent_hold_ms");
    /* 重入快轨 0=关是默认档（失败安全：新判据不上电）。上限与 absent_hold 同量级：
       比它还长的记忆窗口等于把"早已换人"的讲台交给一个刚站上去的陌生人 */
    CLAMP_WARN(stSnap.stTeacher.nReentryMs, 0, 30000, "teacher.reentry_ms");
    /* 位置连续半径 0 合法（=只认原地重入，几乎不触发）；上限 2000 与 patrol_move_px 同量级，
       再大就是把半个教室都算"同一个人走过去的" */
    CLAMP_WARN(stSnap.stTeacher.nReentryPx, 0, 2000, "teacher.reentry_px");
    if (stSnap.stTeacher.nPatrolWindowMs > stSnap.stTeacher.nReleaseMs)
    {
        /* 弱判据（巡视）读的是"本窗口内累计位移够不够"，因此一次跨越门限的走动会在整个窗口内
           持续算作证据——身份的实际存续长度是 release_ms + 一个位移窗口。窗口比放弃时长还长时，
           决定"他还能当多久老师"的是这个窗口而不是 release_ms，而配置里读到的却是后者。
           就地压低窗口（而不是抬高放弃时长）：位移门限是"走动过"的尺度，本来就该是秒级；
           放弃时长是现场按课型调的那个数，不该被一个判据内部量反过来改写 */
        stSnap.stTeacher.nPatrolWindowMs = stSnap.stTeacher.nReleaseMs;
        strSoft += " teacher.patrol_window_ms(capped by release_ms)";
    }
    /* 缺席容忍也不得盖过放弃时长。
       两者管的是同一件东西的两端：absent_hold_ms 决定"人不见了多久还留着这个身份"，
       release_ms 决定"证据断了多久就收回身份"。配成 hold > release 时，
        tolerate 窗口内教师仍会因证据超过 release 被降职——而 `subject_arbiter.cpp`
       那头注释明写"容忍期内教师槽原样保留…本值只保身份不保事件"，
       身份恰恰在这一格里被收走，"只保身份"成为空话。
       与上面 patrol_window_ms 那条同一形状：就地压低新加的那个键，而不是抬高 release
       （release 是现场按课型调的数，不该被内部量改写）。
       0 = 关，所以默认档不受影响（hold=0 < release=20000 恒成立）。 */
    if (stSnap.stTeacher.nAbsentHoldMs > stSnap.stTeacher.nReleaseMs)
    {
        stSnap.stTeacher.nAbsentHoldMs = stSnap.stTeacher.nReleaseMs;
        strSoft += " teacher.absent_hold_ms(capped by release_ms)";
    }
    /* 上限 60s 是事件通道的吞吐不变量 */
    CLAMP_WARN(stSnap.stEgress.nReportIntervalSec, 1, CLASSROOM_REPORT_INTERVAL_MAX_SEC,
               "egress.report_interval_sec");
    /* 下限 300s（5 分钟）：比一次课间还短的"会话"只是把一次课切成一串没有分母的碎片；
       上限 86400s：跨过一天就没有"这是哪节课"可言了，而且 NTP 校时通常以天为尺度回拨 */
    CLAMP_WARN(stSnap.stEgress.nSessionMaxSec, 300, 86400, "egress.session_max_sec");
    /* 0 是合法值（= 关静默收尾），范围里给它留座；上界与 max_sec 同款（一天的量级） */
    CLAMP_WARN(stSnap.stEgress.nSilenceSec, 0, 86400, "egress.session_silence_sec");
    if (stSnap.stEgress.nSessionMaxSec < stSnap.stEgress.nReportIntervalSec)
    {
        /* 会话比上报周期还短 = 每一份周期报表都正好撞上收尾，bSessionReset 恒真。
           平台侧靠这个标志区分"过程值"与"最终值"，恒真就等于这个字段没有信息量，
           而且一节 40 分钟的课会被切成一堆互相不连续的片段。两个数都是现场会改的
           （改上报周期做压力测试时最容易撞到），所以就地抬高并留一条线索。
           注：周期上限 60s、会话下限 300s 之后这条关系结构上恒成立；保留它是因为
           任一侧边界改动（压测放宽周期上限 / 调低会话下限）时这条仍必须成立 */
        stSnap.stEgress.nSessionMaxSec = stSnap.stEgress.nReportIntervalSec;
        strSoft += " egress.session_max_sec(raised to report_interval_sec)";
    }
    /* 滞留环三个数。上限 4GB 级的字面量不是"能给这么多"，而是"超过这个数说明填错了单位" */
    CLAMP_WARN(stSnap.stEgress.nSpoolMaxMb, 1, 512, "egress.spool_max_mb");
    CLAMP_WARN(stSnap.stEgress.nSpoolSegMaxKb, 4, 8192, "egress.spool_seg_max_kb");
    CLAMP_WARN(stSnap.stEgress.nSpoolDrainMaxKb, 4, 4096, "egress.spool_drain_max_kb");
    if (stSnap.stEgress.nSpoolSegMaxKb > stSnap.stEgress.nSpoolMaxMb * 1024)
    {
        /* 段比总量还大 → 整个环永远只有一段，"超限丢最旧"退化成"丢当前"，
           而后者是就地截断，读侧拿不到"丢了哪一段"的线索 */
        stSnap.stEgress.nSpoolSegMaxKb = stSnap.stEgress.nSpoolMaxMb * 1024;
        strSoft += " egress.spool_seg_max_kb(capped by spool_max_mb)";
    }
    if (stSnap.stEgress.strSpoolDir.empty() || '/' != stSnap.stEgress.strSpoolDir[0])
    {
        /* 相对路径不能要：滞留数据落在哪儿取决于 stream 进程当时的 cwd，
           升级换目录后旧积压就成了一批没人认领的孤儿文件 */
        stSnap.stEgress.strSpoolDir = CLASSROOM_SPOOL_DIR;
        strSoft += " egress.spool_dir(not absolute, reverted to default)";
    }
    /* ---- 心跳落点----
       下限 256KB：环小到几拍就转一遍，keep= 这个读数本身就失去意义，而每次轮转都是
       一次真实的 rename + 新建，flash 白写。上限 32MB 单份，配合下面的环预算一起看 */
    CLAMP_WARN(stSnap.stEgress.nMetricMaxKb, 256, 32768, "egress.metric_max_kb");
    CLAMP_WARN(stSnap.stEgress.nMetricMaxFiles, 1, 32, "egress.metric_max_files");
    if (static_cast<int64_t>(stSnap.stEgress.nMetricMaxKb) * (stSnap.stEgress.nMetricMaxFiles + 1) >
        CLASSROOM_METRIC_RING_MAX_KB)
    {
        /* 超预算时收份数不收单份大小：单份大小是轮转粒度（决定一次丢多少），
           份数才是"能回溯多久"——现场要调的是前者，误填的是后者，就地折回来代价最小。
           拒整份配置也是一种选项，但一个日志文件的尺寸不足以让课堂功能整体不可用 */
        stSnap.stEgress.nMetricMaxFiles =
            CLASSROOM_METRIC_RING_MAX_KB / stSnap.stEgress.nMetricMaxKb - 1;
        if (stSnap.stEgress.nMetricMaxFiles < 1)
        {
            stSnap.stEgress.nMetricMaxFiles = 1;
        }
        strSoft += " egress.metric_max_files(capped by ring budget)";
    }
    /* 0 是合法值（业务日志里从此不出现课堂读数），所以只夹下界的负数与上界的荒谬值。
       上界 24h：比这更长，业务日志里那条"最近还有心跳"的证据就太久没出现了 */
    CLAMP_WARN(stSnap.stEgress.nMetricHeadPeriodMs, 0, 86400000, "egress.metric_head_period_ms");
    if (stSnap.stEgress.strMetricFile.empty() || '/' != stSnap.stEgress.strMetricFile[0])
    {
        stSnap.stEgress.strMetricFile = CLASSROOM_METRIC_FILE;
        strSoft += " egress.metric_file(not absolute, reverted to default)";
    }
    else if (stSnap.stEgress.strMetricFile.find('"') != std::string::npos ||
             stSnap.stEgress.strMetricFile.find(' ') != std::string::npos)
    {
        /* 这一串会被原样抄进 statusJson 的 metric_log 字段与每拍心跳：带引号的那个把 JSON
           撑破（课堂侧不做转义，见 statusJson 里那条约定），带空格的把板上那几个读日志的
           脚本全撑破（log_ring.sh、长跑采样器都是按空白切列的）。路径里有这两种字符
           没有正当用法，所以不"支持一下"，直接退回默认 */
        stSnap.stEgress.strMetricFile = CLASSROOM_METRIC_FILE;
        strSoft += " egress.metric_file(quote or space in path, reverted to default)";
    }
    else if (stSnap.stEgress.strMetricFile == STREAM_LOG_PATH)
    {
        /* 这一条不是"不推荐"，是必须挡掉：同一个文件被两个 rotating_file_sink 各自持有句柄、
           各自按自己的阈值 rename，两边的计数立刻对不上，结果比塞满业务日志糟得多——
           时间轴会被两套轮转各抹一遍。心跳另开文件的全部理由就是不再抢这只环 */
        stSnap.stEgress.strMetricFile = CLASSROOM_METRIC_FILE;
        strSoft += " egress.metric_file(is the business log, reverted to default)";
    }

    /* ---- 平台统计推送（0x62xx）----
       下限 1s / 上限 300s：比 1s 还密不是"更实时"，而是把总线让给统计报文并让平台侧
       以毫秒级反复重绘同一个数；比 300s 还疏，一节课只有 8 个点，"尽快推送"就不成立。
       段批量周期上限 60s：它只影响"多快看到一条段的结束"，不该比报表还慢 */
    CLAMP_WARN(stSnap.stStats.nPushIntervalMs, 1000, 300000, "stats.push_interval_ms");
    CLAMP_WARN(stSnap.stStats.nSegBatchMs, 500, 60000, "stats.seg_batch_ms");
    /* 两个批量上限受协议固定数组长度约束：段 24、预警 8。改大要同步改
       tvsdk_stats.h 的数组长度与 static_assert，否则平台按结构体长度解析会读越界 */
    CLAMP_WARN(stSnap.stStats.nSegBatchMax, 1, 24, "stats.seg_batch_max");
    CLAMP_WARN(stSnap.stStats.nAlertBatchMax, 1, 8, "stats.alert_batch_max");
    /* 课时 5..240 分钟：比 5 分钟短的一"课"只是会话碎片，比 4 小时长的多半是单位填错。
       宽限同理，全部给一个宽松但有限的上界，防手滑写出"300"当秒用 */
    CLAMP_WARN(stSnap.stStats.nLessonMin, 5, 240, "stats.lesson_min");
    CLAMP_WARN(stSnap.stStats.nArriveGraceSec, 30, 3600, "stats.arrive_grace_sec");
    CLAMP_WARN(stSnap.stStats.nOverrunGraceMin, 0, 120, "stats.overrun_grace_min");
    CLAMP_WARN(stSnap.stStats.nLeaveAbsentSec, 30, 3600, "stats.leave_absent_sec");
    /* 0 = 不判未巡视（有的课型全程讲授，本就无巡视）；上界一天 */
    CLAMP_WARN(stSnap.stStats.nPatrolIntervalMin, 0, 1440, "stats.patrol_interval_min");
    CLAMP_WARN(stSnap.stStats.nParticipateMinPct, 1, 100, "stats.participate_min_pct");
    CLAMP_WARN(stSnap.stStats.nParticipateHoldSec, 30, 3600, "stats.participate_hold_sec");
    CLAMP_WARN(stSnap.stStats.nDisciplineBadPct, 1, 100, "stats.discipline_bad_pct");
    CLAMP_WARN(stSnap.stStats.nDisciplineHoldSec, 30, 3600, "stats.discipline_hold_sec");
    /* 聚集人数下限 2：1 个人谈不上"聚集"；半径 0.5..10 个头宽，上界防"全班都算一堆" */
    CLAMP_WARN(stSnap.stStats.nGatherNum, 2, CLASSROOM_MAX_PERSONS, "stats.gather_num");
    CLAMP_WARN(stSnap.stStats.fGatherRadiusHeads, 0.5f, 10.0f, "stats.gather_radius_heads");
    CLAMP_WARN(stSnap.stStats.nGatherHoldSec, 5, 3600, "stats.gather_hold_sec");
    /* 趴桌时长必须显著大于低头门（默认 3s），否则"低头"一发生就被报成"趴桌" */
    CLAMP_WARN(stSnap.stStats.nDeskHoldSec, 30, 3600, "stats.desk_hold_sec");
    CLAMP_WARN(stSnap.stPool.llIdleUnloadMs, static_cast<int64_t>(1000), static_cast<int64_t>(3600000),
               "pool.idle_unload_ms");
    /* 上限提成常量 */
    CLAMP_WARN(stSnap.stPool.llReserveKb, static_cast<int64_t>(0),
               CLASSROOM_POOL_RESERVE_MAX_KB, "pool.reserve_kb");
    /* 时间片小于 1s 会让 301ms 的加载成本被反复支付，直接钳到 1s 起。
       上界 25s 不是"保守"而是为了让下面那句复议的话说得通：轮转的复议窗口是 60s，
       而它的职责是"最多 60s 一定重问一次池子两组还装不装得下"（P7-f 那条自锁的出口）。
       片长配到比窗口还长的话，这个旋钮写了但行为不受它管——引擎会在片走完之前就解除轮转，
       现场读到的是"我把 slice 调成一分钟，它还是十几秒就换"，比报错更难查。
       推法与余量写在 classroom_warm_policy.h 的 CLASSROOM_WARM_SLICE_MAX_MS 那条注释里 */
    CLAMP_WARN(stSnap.stPool.llSlicePoseMs, static_cast<int64_t>(1000),
               static_cast<int64_t>(CLASSROOM_WARM_SLICE_MAX_MS), "pool.slice_pose_ms");
    CLAMP_WARN(stSnap.stPool.llSliceFaceMs, static_cast<int64_t>(1000),
               static_cast<int64_t>(CLASSROOM_WARM_SLICE_MAX_MS), "pool.slice_face_ms");

    CLAMP_WARN(stSnap.stHeadDown.fKeyScore, 0.0f, 1.0f, "rules.head_down.key_score");
    CLAMP_WARN(stSnap.stHeadDown.fRatio, 0.01f, 10.0f, "rules.head_down.ratio");
    CLAMP_WARN(stSnap.stHeadUp.fKeyScore, 0.0f, 1.0f, "rules.head_up.key_score");
    /* 三条教师事件的置信度（骨骼均分门）：0..1，0 = 不设门 */
    CLAMP_WARN(stSnap.fTeachKeyScore, 0.0f, 1.0f, "rules.teaching.key_score");
    CLAMP_WARN(stSnap.fBoardWriteKeyScore, 0.0f, 1.0f, "rules.board_write.key_score");
    CLAMP_WARN(stSnap.fInteractKeyScore, 0.0f, 1.0f, "rules.interact.key_score");
    /* 207 区域停留时间：0 = 关，网页上限 10s；给出 60s 的软上界只是防手滑写出"半小时"，
       不在 [0,60000] 内的值钳到边界并记名 */
    CLAMP_WARN(stSnap.nGuidingStayMs, 0, 60000, "rules.guiding.stay_ms");
    CLAMP_WARN(stSnap.stHeadUp.fAngle, 0.0f, 180.0f, "rules.head_up.angle");
    CLAMP_WARN(stSnap.stStand.fKeyScore, 0.0f, 1.0f, "rules.stand.key_score");
    CLAMP_WARN(stSnap.stStand.fLegVerticalAngle, 0.0f, 90.0f, "rules.stand.leg_vertical_angle");
    CLAMP_WARN(stSnap.stStand.fArmStraightAngle, 0.0f, 180.0f, "rules.stand.arm_straight_angle");
    CLAMP_WARN(stSnap.stStand.fMinTorsoShoulder, 0.0f, 5.0f, "rules.stand.min_torso_shoulder");
    CLAMP_WARN(stSnap.stRaiseHand.fKeyScore, 0.0f, 1.0f, "rules.raise_hand.key_score");
    CLAMP_WARN(stSnap.stRaiseHand.fVerticalAngle, 0.0f, 90.0f, "rules.raise_hand.vertical_angle");
    CLAMP_WARN(stSnap.stRaiseHand.fMinWristAboveHead, 0.0f, 20.0f, "rules.raise_hand.min_wrist_above_head");
    /* 举手有效三选一：0 单手 / 1 双手 / 2 单双手（默认=历史行为）。
       钳 [0,2] 并把未知值按默认档处理——写成"配错就整份拒"会让现场一个手滑
       把整份配置打回来，而这一项本身有安全默认值 */
    CLAMP_WARN(stSnap.stRaiseHand.nHandMode, 0, 2, "rules.raise_hand.hand_mode");
    CLAMP_WARN(stSnap.stTurnBack.fFaceScore, 0.0f, 1.0f, "rules.turn_back.face_score");
    CLAMP_WARN(stSnap.stTurnBack.fBodyScore, 0.0f, 1.0f, "rules.turn_back.body_score");
    CLAMP_WARN(stSnap.stTurnBack.fFaceWitnessTtlMs, 0.0f, 60000.0f, "rules.turn_back.face_witness_ttl_ms");
    /* 上限 1.0 之上没有意义（眼Lx>分界在分界≥1 时永不成立，等于关位多占一个键）。
       下限不给 0.5 之外的建议值：分界 <0.45 会把「双眼同落左半」的真侧脸也划进交叉
       （Lx=0.35/Rx=0.25 在分界 0.3 下被杀，而证人的本职恰是救侧脸）；推荐取值在
       0.45~0.55 空带（T 侧上限 <0.45、正面下限 >0.55，见 geometry.h 字段注释） */
    CLAMP_WARN(stSnap.stTurnBack.fFaceWitnessMirrorRx, 0.0f, 1.0f, "rules.turn_back.face_witness_mirror_rx");
    CLAMP_WARN(stSnap.stTeacher.fPoseWitnessTtlMs, 0.0f, 60000.0f, "teacher.pose_witness_ttl_ms");
    CLAMP_WARN(stSnap.stTeacher.nWitOwnHeadPct, 0, 200, "teacher.pose_witness_own_head_pct");
    CLAMP_WARN(stSnap.stTeacher.fPoseWitMinScore, 0.0f, 1.0f, "teacher.pose_witness_min_score");
    CLAMP_WARN(stSnap.stLieOnDesk.fKeyScore, 0.0f, 1.0f, "rules.lie_on_desk.key_score");
    CLAMP_WARN(stSnap.stPhone.fKeyScore, 0.0f, 1.0f, "rules.phone.key_score");
    CLAMP_WARN(stSnap.stPhone.fBandAboveShoulder, 0.0f, 10.0f, "rules.phone.band_above_shoulder");
    /* 下界取正值而不是 0：带是 [-above, +below]，两边同时为 0 就是"没有带"，
       这一列从此恒不成立而心跳上它长得像"没人玩手机"。不额外写关系检查，让区间本身
       使那种配法不可表示（与上面 region_coverage 钳到 1 同一条路：能钳死的不开硬错误） */
    CLAMP_WARN(stSnap.stPhone.fBandBelowShoulder, 0.05f, 10.0f, "rules.phone.band_below_shoulder");
    CLAMP_WARN(stSnap.stPhone.fMaxElbowAngle, 0.0f, 180.0f, "rules.phone.max_elbow_angle");
    CLAMP_WARN(stSnap.stPhone.fMaxWristGapNorm, 0.05f, 10.0f, "rules.phone.wrist_gap_norm");

    /* ---- 107/109/111 的三档朝向与三道人数阈值 ----
       区间钳到 [-1,1] 是把"把 60° 抄进来"这类配错挡在物理范围外：读数本身就是钳在 ±1 的
       比例，配 60 会得到一个恒真（或恒假）的判据，而那在心跳上与"标定过了"看不出区别 */
    CLAMP_WARN(stSnap.stGroup.stListenDir.fKeyScore, 0.0f, 1.0f, "rules.group.listen.key_score");
    CLAMP_WARN(stSnap.stGroup.stListenDir.fTrueAbove, -1.0f, 1.0f, "rules.group.listen.toward_true");
    CLAMP_WARN(stSnap.stGroup.stListenDir.fFalseBelow, -1.0f, 1.0f, "rules.group.listen.toward_false");
    CLAMP_WARN(stSnap.stGroup.stDiscussDir.fKeyScore, 0.0f, 1.0f, "rules.group.discuss.key_score");
    CLAMP_WARN(stSnap.stGroup.stDiscussDir.fTrueAbove, -1.0f, 1.0f, "rules.group.discuss.toward_true");
    CLAMP_WARN(stSnap.stGroup.stDiscussDir.fFalseBelow, -1.0f, 1.0f, "rules.group.discuss.toward_false");
    CLAMP_WARN(stSnap.stGroup.stPresentDir.fKeyScore, 0.0f, 1.0f, "rules.group.present.key_score");
    CLAMP_WARN(stSnap.stGroup.stPresentDir.fTrueAbove, -1.0f, 1.0f, "rules.group.present.toward_true");
    CLAMP_WARN(stSnap.stGroup.stPresentDir.fFalseBelow, -1.0f, 1.0f, "rules.group.present.toward_false");
    /* 中间带不可表示 = 这一列永远不会 UNKNOWN。两档相接（false>=true）时"判不出来"这个状态
       消失了，而它是这几列唯一的诚实出口：远焦糊掉、侧脸、点塌陷都该落在这里，
       被挤成 TRUE 是虚高、挤成 FALSE 是漏判，且报表上看不出任何异常。
       拒而不是钳：两档谁该挪是判据问题，配置层没有资格替标定的人决定 */
    struct DirBand_S { const char *pszName; const FaceDirRule_S *pstDir; };
    const DirBand_S astDir[] = {
        { "listen",  &stSnap.stGroup.stListenDir  },
        { "discuss", &stSnap.stGroup.stDiscussDir },
        { "present", &stSnap.stGroup.stPresentDir },
    };
    for (const DirBand_S &stRow : astDir)
    {
        if (stRow.pstDir->fFalseBelow < stRow.pstDir->fTrueAbove)
        {
            continue;
        }
        rstrErr = std::string("rules.group.") + stRow.pszName + ": toward_false (" +
                  std::to_string(stRow.pstDir->fFalseBelow) + ") must be below toward_true (" +
                  std::to_string(stRow.pstDir->fTrueAbove) +
                  "), otherwise this judgment can never answer UNKNOWN";
        return false;
    }
    CLAMP_WARN(stSnap.stGroup.fNeighborMaxHead, 0.5f, 30.0f, "rules.group.neighbor_max_head");
    /* 死区下界给正值：0 意味着"参考点与头框几乎同侧"时也照取符号，而那一档的符号只剩噪声 */
    CLAMP_WARN(stSnap.stGroup.fRefDeadbandHead, 0.01f, 10.0f, "rules.group.ref_deadband_head");
    CLAMP_WARN(stSnap.stGroup.nListenMinPersons, 1, CLASSROOM_MAX_PERSONS,
               "rules.group.listen_min_persons");
    CLAMP_WARN(stSnap.stGroup.nReadingMinPersons, 1, CLASSROOM_MAX_PERSONS,
               "rules.group.reading_min_persons");
    /* 下界是 2 不是 1：讨论按定义要 "两人以上"，出厂值也是 2。下界留 1 等于允许
       配成 "一个人也算讨论"，而那与 107 听讲/108 阅读的判据直接冲突 */
    CLAMP_WARN(stSnap.stGroup.nDiscussMinPersons, 2, CLASSROOM_MAX_PERSONS,
               "rules.group.discuss_min_persons");
    /* 转身人数阈值：需求写的是"限制最小为1"，比最少参与人数宽一档（它是辅助下界） */
    CLAMP_WARN(stSnap.stGroup.nDiscussTurnPersons, 1, CLASSROOM_MAX_PERSONS,
               "rules.group.discuss_turn_persons");
    /* 网页两个字段（最少参与人数 / 转身人数阈值）描述同一个计数门——本实现只有一个
       计数：朝向邻座成立的生效人数。生效门取两者之大（两条下界同时满足）。
       合并只在这里做一次：上面两道钳制已完成，合并后的数就是消费者唯一看到的数；
       两个字段本身各占一个键、各回各值，网页整表回存时不会互相覆盖 */
    if (stSnap.stGroup.nDiscussTurnPersons > stSnap.stGroup.nDiscussMinPersons)
    {
        stSnap.stGroup.nDiscussMinPersons = stSnap.stGroup.nDiscussTurnPersons;
    }
    /* 场景层派生（#164）：必须在合并之后，否则场景层还按未合并的下界判 */
    stSnap.stScene.nDiscussMinPersons = stSnap.stGroup.nDiscussMinPersons;

    /* ---- 人脸对齐 ----
       这几项都是"量出来的数"，有明确物理区间，故钳制而不是拒（与上面的点序/反向区间不同类）。
       上限一律放得比常态值宽一档：现场要在验收时试不同档位，钳太紧等于替他们决定试到哪 */
    CLAMP_WARN(stSnap.stAlign.fPointScoreMin, 0.0f, 1.0f, "align.point_score_min");
    CLAMP_WARN(stSnap.stAlign.fMinEyeSpanPx, 0.0f, 1024.0f, "align.min_eye_span_px");
    CLAMP_WARN(stSnap.stAlign.fMaxResidualRatio, 0.01f, 10.0f, "align.max_residual_ratio");
    /* 下限 0 是合法档（= 关掉地板门），不是错误 */
    CLAMP_WARN(stSnap.stAlign.fNoiseFloorSrcPx, 0.0f, 64.0f, "align.noise_floor_src_px");
    CLAMP_WARN(stSnap.stAlign.fScaleMax, 0.01f, 64.0f, "align.scale_max");
    CLAMP_WARN(stSnap.nAlignCropPx, CLASSROOM_ALIGN_CROP_MIN_PX, CLASSROOM_ALIGN_CROP_MAX_PX,
               "align.crop_px");
    if (0 != (stSnap.nAlignCropPx % 4))
    {
        /* 向下取到 4 的倍数：RGA 对目标宽度有对齐要求，配 150 会让裁剪档 setup 直接失败，
           现象是"一张人脸特征都算不出来"而日志只说一句 setup failed。
           向下而不是向上：越界一档就可能顶到上面的内存上限 */
        const int32_t nRound = stSnap.nAlignCropPx - (stSnap.nAlignCropPx % 4);
        stSnap.nAlignCropPx = (nRound >= CLASSROOM_ALIGN_CROP_MIN_PX)
                                  ? nRound : (nRound + 4);
        strSoft += " align.crop_px(rounded to 4)";
    }

    /* ---- 底库 ---- */
    CLAMP_WARN(stSnap.stGallery.nReloadSec, 1, 3600, "gallery.reload_sec");
    /* 上限就是 store 的容量口径，超过不会更安全，只会让"名单被截断"这件事换一处发生 */
    CLAMP_WARN(stSnap.stGallery.nMaxEnroll, 1, CLASSROOM_GALLERY_MAX_ENROLL, "gallery.max_enroll");

    /* ---- 板书记录----
       下限都留到"一次采集的量级"而不是 0：reentry 的下限 1s 是因为一次 OCR 是几十到几百毫秒，
       比它短的下限等于没有下限；在途超时下限 1s 同理（比一次取图还短的超时会在采集进行中就把
       请求放行，随后回填写进下一次请求，报表上的时间戳会整体错位）。
       上限全部按容量给：这两个数配大不会更安全，只会把一次分钟尺度的事件挤进每一拍 */
    CLAMP_WARN(stSnap.stBoard.llIntervalMs, 1000, 3600000, "board.reentry_ms");
    /* 0 是合法值（关掉周期重识别）；下界不给 1000 是因为现场可能要更稀的采样 */
    CLAMP_WARN(stSnap.stBoard.llScanIntervalMs, 0, 60000, "board.scan_interval_ms");
    CLAMP_WARN(stSnap.stBoard.llInflightTimeoutMs, 1000, 60000, "board.inflight_timeout_ms");
    /* 停留边沿 0=关是合法档（现场不要这条触发源），所以下限给 0 而不是"一次停留的量级"：
       把 0 抬起来就抹掉了"关"这个选项；上限按一节课的量级给，配再大也只会整节课不触发 */
    CLAMP_WARN(stSnap.stBoard.llStayExitEdgeMs, 0, 600000, "board.stay_exit_edge_ms");
    /* 遮挡比上限 1（不是 0）：配 1 的语义是"整块板被身体挡住也照采"，那是现场要试的对照档
       （要看 OCR 在被挡住时给出什么），而 0 会让这条路恒不采集且与 enable=false 无法区分 */
    CLAMP_WARN(stSnap.stBoard.fMaxOccludeRatio, 0.0f, 1.0f, "board.max_occlude_ratio");
    CLAMP_WARN(stSnap.stBoard.fMinLineScore, 0.0f, 1.0f, "board.min_line_score");
    /* 两条容量钳：单条字节数与环条数的上界就是感知层那两张定长表，超过不会多存下任何东西，
       只会让 setParam 重建出一个比容量还大的"期望"，现象是永远采不满 */
    CLAMP_WARN(stSnap.stBoard.nTextMaxBytes, 16, CLASSROOM_BOARD_TEXT_MAX_BYTES, "board.text_max_bytes");
    CLAMP_WARN(stSnap.stBoard.nItemMax, 1, CLASSROOM_BOARD_ITEM_MAX, "board.item_max");
    /* 行上限的**上界就是那个宏**：一次采集的行数上限决定的是"多少行会进哈希与报文"，
       而报文体积那侧已经改成只放计数，所以这里不再有字节预算，只有"定长表按宏开"这一条。
       下限给 1 而不是 0：配 0 的语义不是"最保守"，是"这条功能整条关掉而读数上看不出来" */
    CLAMP_WARN(stSnap.stBoard.nLineMax, 1, CLASSROOM_BOARD_LINE_MAX, "board.line_max");
    /* ---- OCR 的 DB 三道门与容量（#64 第 9 刀）----
       两道阈值都钳在 0.05..0.95 而不是 0..1：0 会让整片概率图都算文字（BFS 栈触顶、
       一块反光的板能吐出几百个"行"），1 会让一条都出不来——而后者的读数形状是
       "empty= 一直涨"，与"这板子真没写字"逐字同形。
       unclip 钳 1..3：<1 是收缩框（DB 的外扩语义就是为了让笔画不断成几段），
       >3 会让相邻两行的框互相吞并，症状是行数变少而 lines=/bytes= 一起抖 */
    CLAMP_WARN(stSnap.stOcr.fMaskThreshold, 0.05f, 0.95f, "ocr.mask_threshold");
    CLAMP_WARN(stSnap.stOcr.fBoxThreshold, 0.05f, 0.95f, "ocr.box_threshold");
    CLAMP_WARN(stSnap.stOcr.fUnclipRatio, 1.0f, 3.0f, "ocr.unclip_ratio");
    CLAMP_WARN(stSnap.stOcr.nBoxMax, 1, CLASSROOM_OCR_BOX_MAX, "ocr.box_max");
    if (stSnap.stBoard.llInflightTimeoutMs > stSnap.stBoard.llIntervalMs)
    {
        /* 关系钳：在途超时比下限还长时，决定"什么时候允许下一次采集"的是那个超时而不是
           配置里刚写下的 reentry_ms——改了不生效，比改错更难查。就地压到相等并留线索
           （与 teacher.release_ms / egress.session_max_sec 那两条同一处置：成对调的数不拒） */
        stSnap.stBoard.llInflightTimeoutMs = stSnap.stBoard.llIntervalMs;
        strSoft += " board.inflight_timeout_ms(capped to board.reentry_ms)";
    }

    /* ---- 课堂场景时段----
       三个时间数都留下限而不给 0：`enter_ms=0` 的语义不是"立刻切"而是"滞回没有了"，
       现象是一节正常的课被证据抖动切成几十段，而那在报表上看着像"课堂活动很丰富"。
       最短段允许 0（= 关差不合并），因为"我要看每一次切换"是压测时的真实需求 */
    CLAMP_WARN(stSnap.stScene.nEnterMs, 2000, 600000, "scene.enter_ms");
    CLAMP_WARN(stSnap.stScene.nExitMs, 1000, 600000, "scene.exit_ms");
    CLAMP_WARN(stSnap.stScene.nMinSegmentMs, 0, 600000, "scene.min_segment_ms");
    /* 两个密度的上界给到 1 而不是"看起来合理"的 0.5：现场要能配出"全班都举手才算问答"
       这种几乎不成立的档来验证这条判据真的在起作用（配成永远不可能满足的门限，
       是确认一条判据是否在跑的少数几种干净办法之一） */
    CLAMP_WARN(stSnap.stScene.fRaiseRatioQa, 0.0f, 1.0f, "scene.raise_ratio_qa");
    CLAMP_WARN(stSnap.stScene.fHeadDownRatioPractice, 0.0f, 1.0f, "scene.head_down_ratio_practice");
    /* 俯角修正只往画面下方挪（负数会把采样点挪进头顶上方的空气里，口径说不清）；
       下界 0 同时挡 NaN（NaN 过不了比较走钳下界） */
    CLAMP_WARN(stSnap.stScene.fGroundShiftHeads, 0.0f, 2.0f, "scene.ground_shift");
    /* 三条**有顺序**的关系钳。顺序不是讲究：先把窗口撑开，再抬 exit 的下界，
       最后压它的上界，这样三步做完一定自洽；反过来做会留下一组"两条钳互相打架"的配法 */
    if (stSnap.stScene.bEnable && stSnap.stSample.nGroupIntervalMs > 0 &&
        stSnap.stScene.nEnterMs < 2 * stSnap.stSample.nGroupIntervalMs)
    {
        /* 之一：进入窗口要至少两个群体采样周期宽。一拍的窗口等于没有滞回
           （单拍抖动就能切段），而这一层的输入只有群体样本这一条时间轴 */
        stSnap.stScene.nEnterMs = 2 * stSnap.stSample.nGroupIntervalMs;
        strSoft += " scene.enter_ms(raised to 2x sampling.group_interval_ms)";
    }
    if (stSnap.stScene.bEnable && stSnap.stSample.nGroupIntervalMs > 0 &&
        stSnap.stScene.nExitMs < stSnap.stSample.nGroupIntervalMs)
    {
        /* 之二（**这条不钳就会静默停摆**）：exit_ms 比两拍之间的间隔还短时，
           "这一拍反证接着上一轮"永远不成立，反证累计每次都被清零——**时段永远不再切换**，
           整节课缩成一段，而这三个数单独看个个合法。P6e 的滞回用例先是撞上了这个形状，
           才发现它不是测试写错而是配置能写得出来的一种死法 */
        stSnap.stScene.nExitMs = stSnap.stSample.nGroupIntervalMs;
        strSoft += " scene.exit_ms(raised to sampling.group_interval_ms)";
    }
    if (stSnap.stScene.nExitMs >= stSnap.stScene.nEnterMs)
    {
        /* 之三：反证间隔不小于进入窗口的话，"隔久了重新累计"这一条永远不会触发，
           配置里那个 exit_ms 就成了装饰——而现场会以为自己在调灵敏度 */
        stSnap.stScene.nExitMs = stSnap.stScene.nEnterMs / 2;
        strSoft += " scene.exit_ms(capped to enter_ms/2)";
    }

    /* ---- 取特征段 ---- */
    CLAMP_WARN(stSnap.stFaceFeat.nMaxPersons, 1, CLASSROOM_MAX_PERSONS, "face_feature.max_persons");
    /* 每拍人数的钳制上界是 max_persons（容量口径）而不是 CLASSROOM_MAX_PERSONS：
       现场把 max_persons 压到 4 时 per_beat_persons=8 会让一拍做的比容量还多 */
    CLAMP_WARN(stSnap.stFaceFeat.nPerBeatPersons, 1, stSnap.stFaceFeat.nMaxPersons,
               "face_feature.per_beat_persons");
    CLAMP_WARN(stSnap.stFaceFeat.nMinIntervalMs, 0, 600000, "face_feature.min_interval_ms");
    CLAMP_WARN(stSnap.stFaceFeat.nTtlMs, 1000, 600000, "face_feature.ttl_ms");
    if (stSnap.stFaceFeat.bEnable && stSnap.stSample.nFaceIntervalMs > 0 &&
        stSnap.stFaceFeat.nTtlMs < 2 * stSnap.stSample.nFaceIntervalMs)
    {
        /* TTL 比人脸段的周期还短，等于"刚写进表的特征在能被账本取走之前先被清掉"：
           名单迟迟不出、而 pend 恒高。就地抬到两圈，并留下线索说明是谁抬的 */
        stSnap.stFaceFeat.nTtlMs = 2 * stSnap.stSample.nFaceIntervalMs;
        strSoft += " face_feature.ttl_ms(raised to 2x sampling.face_interval_ms)";
    }

    /* ---- 表情 ----
       这里只钳 expressionParamOk 没管的那几项：prob_min / margin_min / ema_alpha / slot_num
       在上面已被硬拒（越界没有"就近取一个"的说法），两套界只留一套口径。
       curve_prob_min 留在这里是因为它有明确的物理边界（概率 0~1），
       越过边界是手滑而不是决定，钳住并留线索比对整份配置不生效有用 */
    CLAMP_WARN(stSnap.stExpr.stParam.fCurveProbMin, 0.0f, 1.0f, "expression.curve_prob_min");
    CLAMP_WARN(stSnap.stExpr.nTtlMs, 1000, 600000, "expression.ttl_ms");
    /* 至少 1 帧：配 0 的含义是"没喂过也给曲线值"，而冷启动那道闸（min_frames）就没东西可挡了，
       现象是刚出勤的人第一帧就带情绪值，而那个值是零向量的 argmax（恒等于槽位 0） */
    CLAMP_WARN(stSnap.stExpr.stParam.nMinFrames, 1, 64, "expression.min_frames");
    if (stSnap.stExpr.bEnable && stSnap.stFaceFeat.bEnable &&
        stSnap.stExpr.nTtlMs < 2 * stSnap.stFaceFeat.nTtlMs)
    {
        /* 表情没有自己的节拍，一个人两次被喂的间隔就是取特征轮转一圈的时间：人数超过
           max_persons 时要两圈以上才轮到他。平滑行若比取特征那侧的 TTL 还短，就会出现
           "名单里他的特征还有效，情绪曲线却已经把这人清了"——同一个人在同一拍一个有值一个没值，
           现场读到的是曲线缺人。就地抬到两圈，并留下线索说明是谁抬的 */
        stSnap.stExpr.nTtlMs = 2 * stSnap.stFaceFeat.nTtlMs;
        strSoft += " expression.ttl_ms(raised to 2x face_feature.ttl_ms)";
    }
    /* 事件新鲜度不超过行 TTL：超过没有意义（行先被清掉，事件照样断），
       而配小了会让两次取特征之间的正常空档被当成"没情绪" */
    CLAMP_WARN(stSnap.stExpr.nEventTtlMs, 1000, stSnap.stExpr.nTtlMs, "expression.event_ttl_ms");

    /* ---- 玩手机复核 ---- */
    CLAMP_WARN(stSnap.stPhoneRecheck.nMaxPersons, 1, CLASSROOM_MAX_PERSONS, "phone_recheck.max_persons");
    /* 下限不是 0：配 0 = 不设同人间隔，而挑选是「最久没做优先」，间隔一停就用面积决定谁在做，
       结果是大框那几个人每拍被反复复核、别人的结论一直过期。100ms 是一次推理的量级 */
    CLAMP_WARN(stSnap.stPhoneRecheck.nMinIntervalMs, 100, 600000, "phone_recheck.min_interval_ms");
    CLAMP_WARN(stSnap.stPhoneRecheck.nTtlMs, 1000, 600000, "phone_recheck.ttl_ms");
    /* 0 是合法值（放弃尺寸闸）：手机在人体裁剪图里到底占多少像素，取决于机位与坐姿，
       现场标定阶段需要能看见所有框再决定切在哪 */
    CLAMP_WARN(stSnap.stPhoneRecheck.nMinBoxPx, 0, 200, "phone_recheck.min_box_px");
    if (stSnap.stPhoneRecheck.bEnable &&
        stSnap.stPhoneRecheck.nTtlMs < stSnap.stPhoneRecheck.nMinIntervalMs)
    {
        /* 结论比"下次能再复核他"的间隔先过期，中间那一段时间里这一列既没有旧结论也没有新结论，
           现象是 106 的事件在时长门上反复断档。就地抬到间隔值，并留下线索说明是谁抬的 */
        stSnap.stPhoneRecheck.nTtlMs = stSnap.stPhoneRecheck.nMinIntervalMs;
        strSoft += " phone_recheck.ttl_ms(raised to phone_recheck.min_interval_ms)";
    }

    for (size_t i = 0; i < stSnap.vecGates.size(); ++i)
    {
        CGateParam_S &stGate = stSnap.vecGates[i];
        CLAMP_WARN(stGate.nSampleIntervalMs, 0, 60000, "behaviors[].sample_interval_ms");
        CLAMP_WARN(stGate.nMinDurationMs, 0, 3600000, "behaviors[].min_duration_ms");
        CLAMP_WARN(stGate.nGapToleranceMs, 0, 60000, "behaviors[].gap_tolerance_ms");
        CLAMP_WARN(stGate.nMinTrueCount, 1, 1000, "behaviors[].min_true_count");

        /* 覆盖率上限必须是 1：配成 1.5 之类等于要求「框的 150% 在区域内」，
           这条行为就此永不出事件，而界面上它明明显示已启用 */
        if (i < stSnap.vecBehaviorRegions.size())
        {
            CLAMP_WARN(stSnap.vecBehaviorRegions[i].fMinCoverage, 0.01f, 1.0f, "behaviors[].region_coverage");
        }
    }

    /* 座位先验的跨块算术。判据住在 seatPriorIdleOk，本处只负责在**三个数都钳完**
       之后问一次：放上面那一段形状校验旁边会拿到一个还没定型的 group_interval_ms
       （它在 sampling 那一段被 CLAMP 到 [1000,60000]，先钳后判才是这台机器真正会用的值）。
       硬拒而不是钳 release_beats：那会把现场刚改的 row_idle_ms 或采样周期变成"生效了、
       但另一个数被偷偷挪了"，而那两个数各自都有人在调。
       只在 enable=true 时判：关掉的时候这张表虽然在折样，考勤读的不是它的号（见 CClassroomEngine
       的 attendStep），这条不等式此刻不保护任何人；而"先关着量一节"正是出厂姿态，
       此时因为一个用不上的算术拒绝整份配置，会把唯一的测量路径也堵掉。
       关掉时不判 ≠ 开着时漏判：开这一位必须改这份配置，一改就重过这道门 */
    if (stSnap.stPrior.bEnable)
    {
        std::string strPriorIdleErr;
        if (!seatPriorIdleOk(stSnap.stPrior.nReleaseBeats, stSnap.stSample.nGroupIntervalMs,
                             stSnap.stAttend.llRowIdleMs, strPriorIdleErr))
        {
            rstrErr = "seat_prior: " + strPriorIdleErr;
            return false;
        }
    }

    if (!strSoft.empty())
    {
        dlog_warn("classroom_config: 越界值已夹取:%s", strSoft.c_str());
    }

    rstrErr.clear();
    return true;
}
bool CClassroomConfig::saveTextToFile(const std::string &strPath, const std::string &strText,
                                           std::string &rstrErr)
{
    if (strPath.empty() || strText.empty())
    {
        rstrErr = "empty path or text";
        return false;
    }
    if (strText.size() > CONFIG_FILE_MAX_BYTES)
    {
        rstrErr = "text too large: " + std::to_string(strText.size());
        return false;
    }

    /* 同目录的临时文件：rename 只在同一文件系统内是原子的，放 /tmp 会变成跨设备拷贝 */
    const std::string strTmp = strPath + ".tmp";
    FILE *pf = fopen(strTmp.c_str(), "wb");
    if (nullptr == pf)
    {
        rstrErr = "open for write failed: " + strTmp;
        return false;
    }

    const size_t nWrite = fwrite(strText.data(), 1, strText.size(), pf);
    bool bOk = (nWrite == strText.size());
    if (bOk && 0 != fflush(pf))
    {
        bOk = false;
    }
    if (bOk && 0 != fsync(fileno(pf)))
    {
        /* 刷盘失败不等于写失败（有些文件系统不支持 fsync），记下来但不当硬错误：
           真正的判据是 rename 之后文件能不能被下一次开机读到 */
        dlog_warn("classroom_config: fsync 失败（%s），继续 rename", strTmp.c_str());
    }
    fclose(pf);

    if (!bOk)
    {
        std::remove(strTmp.c_str());
        rstrErr = "short write: " + strTmp;
        return false;
    }

    if (0 != std::rename(strTmp.c_str(), strPath.c_str()))
    {
        std::remove(strTmp.c_str());
        rstrErr = "rename failed: " + strTmp + " -> " + strPath;
        return false;
    }
    return true;
}

}
