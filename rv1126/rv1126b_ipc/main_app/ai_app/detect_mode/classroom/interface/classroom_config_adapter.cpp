/**
 * @FilePath     : classroom_config_adapter.cpp
 * @Author       : leiyy
 * @Date         : 2026-10-27 14:12:04
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-29 16:22:00
 * @Description  : 课堂 AI 分析配置下发适配层（取值/合并）
 */

#include "classroom_config_adapter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "cJSON.h"
#include "Json.h"

/* Common::Rect_S（nX/nY/nWidth/nHeight + IsValid + ConvertResolution）与
   PIXEL_WIDTH_1920 / PIXEL_HEIGHT_1080 都在 share 的公共定义里——区域换算直接用旧项目
   那套结构，不在这里另写一份 int 四元组：两套矩形结构必然有一处先漂移 */
#include "video_define.h"

#include "dlog.h"

namespace Classroom
{

namespace
{

/* 字段类型 */
enum
{
    FLD_T_BOOL = 0,
    FLD_T_INT  = 1,
    FLD_T_FLOAT = 2,
    FLD_T_STRING = 3,
    FLD_T_IDS = 4    /* 整型数组（区域 id 列表） */
};

struct SFeatureDef_S
{
    ConfigPage_E                enFeature;
    const char                 *pszName;
    std::vector<CConfigPageField_S>  vecFields;
};

/* 供表里引用的常量：avoid 每处写字面量后漂移。
   WF = 纯比例；WFL = 线性（带偏置）；WFS = 角度→-sin（见头文件 nConv 的说明） */
#define WF(k, p, sc, t)        { k, p, sc, t, 0.0f, 0 }
#define WFL(k, p, sc, bias, t) { k, p, sc, t, bias, 1 }
#define WFS(k, p, t)           { k, p, 1.0f, t, 0.0f, 2 }
/* WFP = 「像素，按 1920×1080 」 */
#define WFP(k, p, t)           { k, p, 1.0f, t, 0.0f, 3 }
/* WFD = 同 WF，但给出"配置原文缺这个键时的下发默认值" */
#define WFD(k, p, sc, t, d)    { k, p, sc, t, 0.0f, 0, (d), true }

const std::vector<SFeatureDef_S> &featureTable()
{
    static const std::vector<SFeatureDef_S> s_tbl = {
    /* 1 摄像机类型 */
    { ConfigPage_E::DEVICE_ROLE, "摄像机类型", {
        WF("role", "role", 1.0f, FLD_T_STRING),
        WFD("checked_stu_stand",      "ui.checked.stu_stand",      1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_raise_hand", "ui.checked.stu_raise_hand", 1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_head_up",    "ui.checked.stu_head_up",    1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_head_down",  "ui.checked.stu_head_down",  1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_turn_back",  "ui.checked.stu_turn_back",  1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_phone",      "ui.checked.stu_phone",      1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_listen",     "ui.checked.stu_listen",     1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_reading",    "ui.checked.stu_reading",    1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_discuss",    "ui.checked.stu_discuss",    1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_expression", "ui.checked.stu_expression", 1.0f, FLD_T_BOOL, 0),
        WFD("checked_attendance",     "ui.checked.attendance",     1.0f, FLD_T_BOOL, 0),
        WFD("checked_face_detect",    "ui.checked.face_detect",    1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_guiding",    "ui.checked.tea_guiding",    1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_interact",   "ui.checked.tea_interact",   1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_patrol",     "ui.checked.tea_patrol",     1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_stand",      "ui.checked.tea_stand",      1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_head_up",    "ui.checked.tea_head_up",    1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_head_down",  "ui.checked.tea_head_down",  1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_phone",      "ui.checked.tea_phone",      1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_teaching",   "ui.checked.tea_teaching",   1.0f, FLD_T_BOOL, 0),
        WFD("checked_tea_board_write","ui.checked.tea_board_write",1.0f, FLD_T_BOOL, 0),
        WFD("checked_enter_podium",   "ui.checked.enter_podium",   1.0f, FLD_T_BOOL, 0),
        WFD("checked_board_ocr",      "ui.checked.board_ocr",      1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_podium",     "ui.checked.stu_podium",     1.0f, FLD_T_BOOL, 0),
        WFD("checked_stu_present",    "ui.checked.stu_present",    1.0f, FLD_T_BOOL, 0),
    } },
    /* 2 学生-站立（101） */
    { ConfigPage_E::STU_STAND, "学生站立检测", {
        WF("enable",            "behaviors.101.enable",           1.0f,   FLD_T_BOOL),
        WF("min_duration_s",    "behaviors.101.min_duration_ms",  0.001f, FLD_T_INT),
        WF("gap_tolerance_s",   "behaviors.101.gap_tolerance_ms", 0.001f, FLD_T_INT),
        WF("confidence",        "rules.stand.key_score",          10.0f,  FLD_T_FLOAT),
    } },
    /* 3 学生-举手（102） */
    { ConfigPage_E::STU_RAISE_HAND, "学生举手检测", {
        WF("enable",            "behaviors.102.enable",           1.0f,   FLD_T_BOOL),
        WF("min_duration_s",    "behaviors.102.min_duration_ms",  0.001f, FLD_T_INT),
        WF("confidence",        "rules.raise_hand.key_score",     10.0f,  FLD_T_FLOAT),
        WF("hand_mode",         "rules.raise_hand.hand_mode",     1.0f,   FLD_T_INT),
    } },
    /* 4 学生-抬头（103） */
    { ConfigPage_E::STU_HEAD_UP, "学生抬头检测", {
        WF("enable",            "behaviors.103.enable",           1.0f,   FLD_T_BOOL),
        WFL("angle",            "rules.head_up.angle",            1.0f / 3.0f, 90.0f, FLD_T_FLOAT),
        WF("sample_gap_s",      "behaviors.103.sample_interval_ms", 0.001f, FLD_T_INT),
        WF("confidence",        "rules.head_up.key_score",        10.0f,  FLD_T_FLOAT),
    } },
    /* 5 学生-低头（104） */
    { ConfigPage_E::STU_HEAD_DOWN, "学生低头检测", {
        WF("enable",            "behaviors.104.enable",           1.0f,   FLD_T_BOOL),
        WFL("angle",            "rules.head_down.ratio",          1.0f / 60.0f, 0.0f, FLD_T_FLOAT),
        WF("min_duration_s",    "behaviors.104.min_duration_ms",  0.001f, FLD_T_INT),
        WF("confidence",        "rules.head_down.key_score",      10.0f,  FLD_T_FLOAT),
    } },
    /* 6 学生-背身（105） */
    { ConfigPage_E::STU_TURN_BACK, "学生背身检测", {
        WF("enable",            "behaviors.105.enable",           1.0f,   FLD_T_BOOL),
        /* "朝向阈值"落到双肩关键点分数门（judgeTurnBack 的 fBodyScore，0..1）：
           1..90 度线性映射到 0.05..1.00（90 度 = 1.0 = clamp 上沿，45 度 ≈ 0.52）。
           口径是"越严越要证据"，单调；不是实测角度——内部没有角度可读。 */
        WFL("angle",            "rules.turn_back.body_score",     (1.0f - 0.05f) / 89.0f, 0.05f - (1.0f - 0.05f) / 89.0f, FLD_T_FLOAT),
        WF("min_duration_s",    "behaviors.105.min_duration_ms",  0.001f, FLD_T_INT),
        WF("confidence",        "rules.turn_back.face_score",     10.0f,  FLD_T_FLOAT),
    } },
    /* 7 学生-使用手机（106） */
    { ConfigPage_E::STU_PHONE, "学生手机检测", {
        WF("enable",            "behaviors.106.enable",           1.0f,   FLD_T_BOOL),
        WF("merge_gap_s",       "behaviors.106.gap_tolerance_ms", 0.001f, FLD_T_INT),
        WF("confidence",        "rules.phone.key_score",          10.0f,  FLD_T_FLOAT),
    } },
    /* 8 学生-听讲（107） */
    { ConfigPage_E::STU_LISTEN, "学生听讲检测", {
        WF("enable",            "behaviors.107.enable",           1.0f,   FLD_T_BOOL),
        /* 朝向阈值是归一横摆 fYaw（鼻相对双眼中点 / 肩宽，-1..1），不是角度。
           按 sin 换算：config = -sin(deg)。出厂 -0.12 → 6.9 度、-0.30 → 17.5 度，
           正好落在需求给的小角度段，且 1..90 度全落在 clamp 内。 */
        WFS("body_angle",       "rules.group.listen.toward_true",  FLD_T_FLOAT),
        WFS("head_angle",       "rules.group.listen.toward_false", FLD_T_FLOAT),
        WF("headup_persons",    "rules.group.listen_min_persons", 1.0f,   FLD_T_INT),
    } },
    /* 9 学生-阅读（108） */
    { ConfigPage_E::STU_READING, "学生阅读检测", {
        WF("enable",            "behaviors.108.enable",           1.0f,   FLD_T_BOOL),
        WF("min_duration_s",    "behaviors.108.min_duration_ms",  0.001f, FLD_T_INT),
        WF("merge_gap_s",       "behaviors.108.gap_tolerance_ms", 0.001f, FLD_T_INT),
        WF("headdown_persons",  "rules.group.reading_min_persons", 1.0f,  FLD_T_INT),
        WF("confidence",        "rules.head_down.key_score",      10.0f,  FLD_T_FLOAT),
    } },
    /* 10 学生-讨论（109） */
    { ConfigPage_E::STU_DISCUSS, "学生讨论检测", {
        WF("enable",            "behaviors.109.enable",           1.0f,   FLD_T_BOOL),
        WF("min_persons",       "rules.group.discuss_min_persons", 1.0f,  FLD_T_INT),
        WF("min_duration_s",    "behaviors.109.min_duration_ms",  0.001f, FLD_T_INT),
        /* 本实现只有一个计数（朝向邻座成立的生效人数），"转身人数"与"参与人数"
           是同一个数的两条下界，**不是笔误也不是同一个键** */
        WFD("turn_persons",     "rules.group.discuss_turn_persons", 1.0f, FLD_T_INT, 2),
    } },
    /* 11 学生-表情（112） */
    { ConfigPage_E::STU_EXPRESSION, "学生表情识别", {
        WF("enable",            "behaviors.112.enable",           1.0f,   FLD_T_BOOL),
        WFP("min_face_px",      "threshold.min_face_px",                  FLD_T_INT),
        WF("confidence",        "expression.prob_min",            10.0f,  FLD_T_FLOAT),
        /* 允许人脸角度（度，1..90）：消费方是感知入口的角度门（runExpressionOfWarp），
           超限时连推理都不提交。量不出来（五点退化）时放行，心跳 expr 段 nobad= 计这一档 */
        WF("max_angle",         "expression.max_angle",            1.0f,   FLD_T_FLOAT),
    } },
    /* 12 考勤人数统计 */
    { ConfigPage_E::ATTENDANCE, "考勤人数统计", {
        WF("enable",            "attendance.enable",              1.0f,   FLD_T_BOOL),
    } },
    /* 13 学生人脸检测（同上：人脸链没有区域参数，区域键等实现一起进来） */
    { ConfigPage_E::FACE_DETECT, "学生人脸检测", {
        WF("enable",            "gallery.enable",                 1.0f,   FLD_T_BOOL),
        WF("sensitivity",       "threshold.det_score",            10.0f,  FLD_T_FLOAT),
        /* 是"最小瞳距" */
        WFP("min_eye_span_px",  "align.min_eye_span_px",                 FLD_T_INT),
    } },
    /* 14 教师-指导学生（207） */
    { ConfigPage_E::TEA_GUIDING, "教师指导学生", {
        WF("enable",            "behaviors.207.enable",           1.0f,   FLD_T_BOOL),
        /* 「区域停留时间」约束的是单个学生：同一行在页 14 框内连续站够这么久才算
           "站住了" */
        WF("stay_s",            "rules.guiding.stay_ms",          0.001f, FLD_T_INT),
        WF("min_duration_s",    "behaviors.207.min_duration_ms",  0.001f, FLD_T_INT),
    } },
    /* 15 教师-师生互动（209）。置信度落 rules.interact.key_score（骨骼均分门），
       不再借学生听讲的朝向键——那会让"改互动置信度"改变 107 的判据（串台） */
    { ConfigPage_E::TEA_INTERACT, "教师师生互动提示", {
        WF("enable",            "behaviors.209.enable",           1.0f,   FLD_T_BOOL),
        WF("confidence",        "rules.interact.key_score",       10.0f,  FLD_T_FLOAT),
    } },
    /* 16 教师-巡视（210）。网页第 16 页只有「启用 / 最短巡视时间 / 最小移动距离 / 区域」，
       **没有** patrol_window_s。
       位移累计窗仍按配置里的 teacher.patrol_window_ms 走（出厂 5s），
       但它不是网页可配项，就不出现在本页字段表里——GET 不再回一个网页无处显示、
       SET 也永远不会送来的键 */
    { ConfigPage_E::TEA_PATROL, "教师巡视", {
        WF("enable",            "behaviors.210.enable",           1.0f,   FLD_T_BOOL),
        WF("min_duration_s",    "behaviors.210.min_duration_ms",  0.001f, FLD_T_INT),
        WFP("min_move_px",      "teacher.patrol_move_px",                FLD_T_INT),
    } },
    /* 17 教师-站立（201） */
    { ConfigPage_E::TEA_STAND, "教师站立检测", {
        WF("enable",            "behaviors.201.enable",           1.0f,   FLD_T_BOOL),
        WF("min_duration_s",    "behaviors.201.min_duration_ms",  0.001f, FLD_T_INT),
        WF("confidence",        "rules.stand.key_score",          10.0f,  FLD_T_FLOAT),
    } },
    /* 18 教师-抬头（202） 与 203 低头是两列，别配反。
       角度换算与第 4 页同一口径（同一个键 rules.head_up.angle、同一条判据） */
    { ConfigPage_E::TEA_HEAD_UP, "教师抬头检测", {
        WF("enable",            "behaviors.202.enable",           1.0f,   FLD_T_BOOL),
        WFL("angle",            "rules.head_up.angle",            1.0f / 3.0f, 90.0f, FLD_T_FLOAT),
        WF("sample_gap_s",      "behaviors.202.sample_interval_ms", 0.001f, FLD_T_INT),
        WF("confidence",        "rules.head_up.key_score",        10.0f,  FLD_T_FLOAT),
    } },
    /* 19 教师-低头（203） */
    { ConfigPage_E::TEA_HEAD_DOWN, "教师低头检测", {
        WF("enable",            "behaviors.203.enable",           1.0f,   FLD_T_BOOL),
        /* 与学生低头同一口径：60 度 = 1.0 */
        WFL("angle",            "rules.head_down.ratio",          1.0f / 60.0f, 0.0f, FLD_T_FLOAT),
        WF("min_duration_s",    "behaviors.203.min_duration_ms",  0.001f, FLD_T_INT),
        WF("confidence",        "rules.head_down.key_score",      10.0f,  FLD_T_FLOAT),
    } },
    /* 20 教师-使用手机（205） */
    { ConfigPage_E::TEA_PHONE, "教师手机检测", {
        WF("enable",            "behaviors.205.enable",           1.0f,   FLD_T_BOOL),
        WF("confidence",        "rules.phone.key_score",          10.0f,  FLD_T_FLOAT),
        WF("min_duration_s",    "behaviors.205.min_duration_ms",  0.001f, FLD_T_INT),
    } },
    /* 21 教师-讲授（206） */
    { ConfigPage_E::TEA_TEACHING, "教师讲授", {
        WF("enable",            "behaviors.206.enable",           1.0f,   FLD_T_BOOL),
        /* 置信度 = 骨骼均分门（rules.teaching.key_score），不是 107 的朝向门 */
        WF("confidence",        "rules.teaching.key_score",       10.0f,  FLD_T_FLOAT),
        WF("min_duration_s",    "behaviors.206.min_duration_ms",  0.001f, FLD_T_INT),
    } },
    /* 22 教师-书写板书（208） */
    { ConfigPage_E::TEA_BOARD_WRITE, "教师书写板书", {
        WF("enable",            "behaviors.208.enable",           1.0f,   FLD_T_BOOL),
        /* 置信度 = 骨骼均分门（rules.board_write.key_score）；board.min_line_score 是
           OCR 行分门（第 24 页），接在这里等于"改板书行为顺手改 OCR" */
        WF("confidence",        "rules.board_write.key_score",    10.0f,  FLD_T_FLOAT),
        WF("min_duration_s",    "behaviors.208.min_duration_ms",  0.001f, FLD_T_INT),
    } },
    /* 23 进入讲台（211）。停留门是 211 自己的 min_duration_ms——
       原映射写的 teacher.confirm_ms 是"教师身份确认"的时长，改讲台停留会顺手改身份链。
        区域走 region_roles 的 PODIUM 角色（=0）而不是 behaviors.211.region_ids：
       这一页画的是**讲台**，而讲台是一个"用途"（region.h 的 REGION_ROLE_PODIUM），
       上讲台（110）、演示（111）、巡视的"讲台没人"判据都靠它。
       写进行为自己的 region_ids 只会约束 211 这一条，讲台角色仍空着——
       现场表现是"进入讲台能出、上讲台永远不出"。行为侧 behaviorRegionIds() 的
       顺序是"自己有点位就用自己的、没有才回落到角色"，所以写角色同时把三条都喂到了 */
    { ConfigPage_E::ENTER_PODIUM, "进入讲台", {
        WF("enable",            "behaviors.211.enable",           1.0f,   FLD_T_BOOL),
        WF("min_stay_s",        "behaviors.211.min_duration_ms",  0.001f, FLD_T_INT),
    } },
    /* 24 黑板内容识别 */
    /* 24 黑板内容识别。
       区域走 region_roles 的 BOARD 角色（=1）而不是 board.region_ids：
       板书这块区域的归属在角色表里（region.h 的 REGION_ROLE_BOARD），
       board 段自己没有区域键——写一个解析器不认的键就是装一个不反应的旋钮。
       "分析间隔时间"映射到 board.reentry_ms：板书这条路是边沿触发的（board_interval_ms
       已在 P6d 删除，见 conf/classroom_config.json 的 _board_notes），两次采集之间的
       真实节奏就是 reentry_ms 这道下限，配置层的字段名干脆就叫 llIntervalMs。
       不映射到 ocr.box_max：那是一次检测最多交出去多少行，与时间不是一个量纲，
       网页送进来的秒数会变成一个框数上限，报错都不报一声。 */
    { ConfigPage_E::BOARD_OCR, "黑板内容识别", {
        WF("enable",            "board.enable",                   1.0f,   FLD_T_BOOL),
        /* 「分析间隔」= 同一板书期间的重识别间隔（board.scan_interval_ms），
           不是 board.reentry_ms——后者是两次板书结束之间的最小间隔（防一转身采两次），
           语义不同、量级也不同（那个默认 60s，这个 5s） */
        WF("interval_s",        "board.scan_interval_ms",         0.001f, FLD_T_INT),
    } },
    /* 25 学生-上讲台（110） */
    { ConfigPage_E::STU_PODIUM, "学生上讲台检测", {
        WF("enable",            "behaviors.110.enable",           1.0f,   FLD_T_BOOL),
        /* 同上：讲台是角色不是某一页的私有区域，三页（23/25/26）画的是同一块 */
        WF("min_stay_s",        "behaviors.110.min_duration_ms",  0.001f, FLD_T_INT),
    } },
    /* 26 学生-演示（111） */
    { ConfigPage_E::STU_PRESENT, "学生演示检测", {
        WF("enable",            "behaviors.111.enable",           1.0f,   FLD_T_BOOL),
        /* 演示发生在讲台→ 同一块讲台角色 */
        WF("min_duration_s",    "behaviors.111.min_duration_ms",  0.001f, FLD_T_INT),
        WF("confidence",        "rules.group.present.key_score",  10.0f,  FLD_T_FLOAT),
    } },
    };
    return s_tbl;
}

const SFeatureDef_S *findFeature(ConfigPage_E enFeature)
{
    for (const SFeatureDef_S &stDef : featureTable())
    {
        if (stDef.enFeature == enFeature)
        {
            return &stDef;
        }
    }
    return nullptr;
}

/* ---------------- cJSON 辅助 ---------------- */

cJSON *deepCopy(const cJSON *pSrc)
{
    if (nullptr == pSrc)
    {
        return nullptr;
    }
    cJSON *pDst = nullptr;
    switch (pSrc->type)
    {
    case cJSON_Object:
        pDst = cJSON_CreateObject();
        if (nullptr != pDst)
        {
            for (const cJSON *pChild = pSrc->child; nullptr != pChild; pChild = pChild->next)
            {
                cJSON *pCopy = deepCopy(pChild);
                if (nullptr == pCopy || 0 == cJSON_AddItemToObject(pDst, pChild->string, pCopy))
                {
                    cJSON_Delete(pCopy);
                    cJSON_Delete(pDst);
                    return nullptr;
                }
            }
        }
        break;
    case cJSON_Array:
        pDst = cJSON_CreateArray();
        if (nullptr != pDst)
        {
            for (const cJSON *pChild = pSrc->child; nullptr != pChild; pChild = pChild->next)
            {
                cJSON *pCopy = deepCopy(pChild);
                if (nullptr == pCopy || 0 == cJSON_AddItemToArray(pDst, pCopy))
                {
                    cJSON_Delete(pCopy);
                    cJSON_Delete(pDst);
                    return nullptr;
                }
            }
        }
        break;
    case cJSON_String:
        pDst = cJSON_CreateString((nullptr != pSrc->valuestring) ? pSrc->valuestring : "");
        break;
    case cJSON_Number:
        pDst = cJSON_CreateNumber(pSrc->valuedouble);
        break;
    case cJSON_True:
        pDst = cJSON_CreateTrue();
        break;
    case cJSON_False:
        pDst = cJSON_CreateFalse();
        break;
    case cJSON_NULL:
        pDst = cJSON_CreateNull();
        break;
    default:
        break;
    }
    return pDst;
}

/* 按点分路径取节点。数组成员段：**按 id 匹配优先**，全数字且没有同 id 成员时才退按下标
   （字段表写的是 behaviors.101.enable，101 是 Behavior_E 编号而不是数组下标：
   按下标取会拿到第 101 个元素或空，症状是"Get 整页 null 而 Set 正常"） */
cJSON *nodeAt(cJSON *pRoot, const char *pszPath)
{
    if (nullptr == pRoot || nullptr == pszPath)
    {
        return nullptr;
    }
    const std::string strPath(pszPath);
    size_t nPos = 0;
    cJSON *pCur = pRoot;
    while (nullptr != pCur && nPos < strPath.size())
    {
        const size_t nDot = strPath.find('.', nPos);
        const std::string strSeg = (std::string::npos == nDot)
                                   ? strPath.substr(nPos)
                                   : strPath.substr(nPos, nDot - nPos);
        if (strSeg.empty())
        {
            return nullptr;
        }
        if (cJSON_IsArray(pCur))
        {
            /* 全数字段按 id 找；找不到同 id 的成员时才退按下标
               （id 优先的理由：字段表里的 101/102… 是 Behavior_E 编号，
                 与数组下标是两套编号；"没有这一行"与"这一行在第几个"是两件事） */
            cJSON *pNext = nullptr;
            if (strSeg.find_first_not_of("0123456789") == std::string::npos)
            {
                const int nId = atoi(strSeg.c_str());
                /* 两种成员编号并存：behaviors 用 id（Behavior_E 编号），
                   region_roles 用 role（RegionRole_E 编号）。字段表写的是
                   region_roles.1.region_ids，那一行里没有 id 键 */
                for (cJSON *pItem = pCur->child; nullptr != pItem; pItem = pItem->next)
                {
                    cJSON *pId = cJSON_GetObjectItem(pItem, "id");
                    if (nullptr == pId)
                    {
                        pId = cJSON_GetObjectItem(pItem, "role");
                    }
                    if (nullptr != pId && cJSON_IsNumber(pId) && pId->valueint == nId)
                    {
                        pNext = pItem;
                        break;
                    }
                }
            }
            if (nullptr == pNext)
            {
                /* 非数字段，或数字段没有同 id 成员：退按下标（数组型配置没有 id 键时的退路） */
                if (strSeg.find_first_not_of("0123456789") == std::string::npos)
                {
                    pNext = cJSON_GetArrayItem(pCur, atoi(strSeg.c_str()));
                }
                else
                {
                    for (cJSON *pItem = pCur->child; nullptr != pItem; pItem = pItem->next)
                    {
                        cJSON *pId = cJSON_GetObjectItem(pItem, "id");
                        if (nullptr != pId && cJSON_IsString(pId) &&
                            0 == strcmp(pId->valuestring, strSeg.c_str()))
                        {
                            pNext = pItem;
                            break;
                        }
                    }
                }
            }
            pCur = pNext;
        }
        else
        {
            pCur = cJSON_GetObjectItem(pCur, strSeg.c_str());
        }
        nPos = (std::string::npos == nDot) ? strPath.size() : nDot + 1;
    }
    return pCur;
}

/* 按点分路径写值（路径不存在的中间层一律补齐） */
bool nodeSet(cJSON *pRoot, const char *pszPath, cJSON *pValue)
{
    if (nullptr == pRoot || nullptr == pszPath || nullptr == pValue)
    {
        return false;
    }
    const std::string strPath(pszPath);
    const size_t nDot = strPath.find('.', pszPath ? 0 : 0);
    if (std::string::npos == nDot)
    {
        /* 顶层键 */
        cJSON *pOld = cJSON_GetObjectItem(pRoot, strPath.c_str());
        if (nullptr != pOld)
        {
            /* cJSON_ReplaceItemViaPointer 只重链兄弟指针，**不搬键名**（本仓 cJSON 实现
               见 replace_item 一节）：replacement 自己不带 string，换进去之后这个键
               就变成空串。现场症状是"保存成功、返回接受，但那一项回到默认值"——
               最难查的那一类，因为两侧日志都说没问题。
               所以这里把旧键名整块交过去，再把旧 item 的 string 置空，
               它被 delete 时就不会把名字一起释放掉 */
            char *pszOldKey = pOld->string;
            pOld->string = nullptr;
            if (nullptr != pszOldKey)
            {
                pValue->string = pszOldKey;
            }
            if (0 == cJSON_ReplaceItemViaPointer(pRoot, pOld, pValue))
            {
                /* 换入失败（只有三个入参为空才会走到这里，此时旧 item 未被释放）：
                   把名字还给旧 item，别让它悬空 */
                pOld->string = pszOldKey;
                if (nullptr != pszOldKey)
                {
                    pValue->string = nullptr;
                }
                return false;
            }
            /* 注意：cJSON_ReplaceItemViaPointer 内部已经把旧 item 置空链表并 delete 掉
               （本仓 cJSON.c 的 replace 末尾），所以这里不能再 delete 一次。
               string 已经被摘走，那次 delete 只释放旧值本体，不会碰键名 */
        }
        else
        {
            if (0 == cJSON_AddItemToObject(pRoot, strPath.c_str(), pValue))
            {
                return false;
            }
        }
        return true;
    }
    const std::string strHead = strPath.substr(0, nDot);
    const std::string strTail = strPath.substr(nDot + 1);
    cJSON *pChild = cJSON_GetObjectItem(pRoot, strHead.c_str());
    if (nullptr == pChild)
    {
        pChild = cJSON_CreateObject();
        if (nullptr == pChild || 0 == cJSON_AddItemToObject(pRoot, strHead.c_str(), pChild))
        {
            cJSON_Delete(pChild);
            return false;
        }
    }
    if (cJSON_IsArray(pChild))
    {
        /* 数组成员：按 id 找，找不到就按数字下标找 */
        const bool bDigit = (strTail.find_first_not_of("0123456789") == std::string::npos);
        cJSON *pItem = nullptr;
        if (bDigit)
        {
            pItem = cJSON_GetArrayItem(pChild, atoi(strTail.c_str()));
        }
        else
        {
            const int nId = atoi(strTail.c_str());
            for (cJSON *p = pChild->child; nullptr != p; p = p->next)
            {
                /* id 优先，region_roles 那一族没有 id 键、只有 role */
                cJSON *pId = cJSON_GetObjectItem(p, "id");
                if (nullptr == pId)
                {
                    pId = cJSON_GetObjectItem(p, "role");
                }
                if (nullptr != pId && cJSON_IsNumber(pId) && pId->valueint == nId)
                {
                    pItem = p;
                    break;
                }
            }
        }
        if (nullptr == pItem)
        {
            /* 这一行不在配置里：补一行 {id, key} 而不是拒绝。
               拒绝的后果是"网页把某个行为打开了，保存回来说不接受"，
               而现场无法从这条回执判断该去改哪里；
               补一行只动这一页的两个键，其余行原样保留（仍是逐键并入，不是整表替换）。
               解析器对缺字段一律保留默认值（classroom_config.h 的既有契约），
               所以新行里没有的键不会变成 0 */
            const int nId = atoi(strTail.c_str());
            /* 新行的编号键名跟着段名走：behaviors 用 id，region_roles 用 role。
               写错键名的后果是解析器整行不认识，而现场看到的是"保存成功、没有任何反应" */
            const char *pszNumKey = (0 == strHead.compare("region_roles")) ? "role" : "id";
            cJSON *pNew = cJSON_CreateObject();
            cJSON *pIdVal = cJSON_CreateNumber(static_cast<double>(nId));
            if (nullptr == pNew || nullptr == pIdVal ||
                0 == cJSON_AddItemToObject(pNew, pszNumKey, pIdVal) ||
                0 == cJSON_AddItemToArray(pChild, pNew))
            {
                cJSON_Delete(pIdVal);
                cJSON_Delete(pNew);
                return false;
            }
            pItem = pNew;
        }
        return nodeSet(pItem, strTail.substr(strTail.find('.') == std::string::npos
                                             ? strTail.size() : strTail.find('.') + 1).c_str(),
                       pValue);
    }
    return nodeSet(pChild, strTail.c_str(), pValue);
}

/* 深合并（数组按 id 配对逐键并入） */
void mergeInto(cJSON *pDst, const cJSON *pSrc)
{
    if (nullptr == pDst || nullptr == pSrc)
    {
        return;
    }
    if (cJSON_IsObject(pSrc) && cJSON_IsObject(pDst))
    {
        for (const cJSON *pChild = pSrc->child; nullptr != pChild; pChild = pChild->next)
        {
            const char *pszKey = pChild->string;
            if (nullptr == pszKey)
            {
                continue;
            }
            cJSON *pDstChild = cJSON_GetObjectItem(pDst, pszKey);
            if (nullptr != pDstChild && cJSON_IsObject(pDstChild) && cJSON_IsObject(pChild))
            {
                mergeInto(pDstChild, pChild);
            }
            else if (nullptr != pDstChild && cJSON_IsArray(pDstChild) && cJSON_IsArray(pChild))
            {
                mergeInto(pDstChild, pChild);
            }
            else
            {
                cJSON *pCopy = deepCopy(pChild);
                if (nullptr != pCopy)
                {
                    cJSON_ReplaceItemViaPointer(pDst, pDstChild, pCopy);
                }
            }
        }
        return;
    }
    if (cJSON_IsArray(pSrc) && cJSON_IsArray(pDst))
    {
        for (const cJSON *pChild = pSrc->child; nullptr != pChild; pChild = pChild->next)
        {
            const cJSON *pId = cJSON_GetObjectItem(pChild, "id");
            if (nullptr == pId || !cJSON_IsNumber(pId))
            {
                cJSON *pCopy = deepCopy(pChild);
                if (nullptr != pCopy)
                {
                    cJSON_AddItemToArray(pDst, pCopy);
                }
                continue;
            }
            cJSON *pMatch = nullptr;
            for (cJSON *pItem = pDst->child; nullptr != pItem; pItem = pItem->next)
            {
                const cJSON *pItemId = cJSON_GetObjectItem(pItem, "id");
                if (nullptr != pItemId && cJSON_IsNumber(pItemId) &&
                    pItemId->valueint == pId->valueint)
                {
                    pMatch = pItem;
                    break;
                }
            }
            if (nullptr != pMatch)
            {
                mergeInto(pMatch, pChild);
            }
            else
            {
                cJSON *pCopy = deepCopy(pChild);
                if (nullptr != pCopy)
                {
                    cJSON_AddItemToArray(pDst, pCopy);
                }
            }
        }
    }
}

/* 把 config 值按 scale 换算成下发值，加进输出对象 */
/* 配置值 → 下发值（Get 方向）。nConv 见头文件：0=比例、1=线性带偏置、2=-sin。 */
double wireFromConfig(double dCfg, const CConfigPageField_S &stField)
{
    switch (stField.nConv)
    {
    case 1:
        if (std::fabs((double)stField.fScale) < 1e-9)
        {
            return dCfg;
        }
        return (dCfg - (double)stField.fBias) / (double)stField.fScale;
    case 2:
    {
        /* config = -sin(deg) ⇒ deg = asin(-config)。夹到 [-1,1] 再 asin：
           配置被钳制之外的值（手改文件）不该让 asin 出 NaN */
        double dSin = -dCfg;
        if (dSin > 1.0)
        {
            dSin = 1.0;
        }
        else if (dSin < -1.0)
        {
            dSin = -1.0;
        }
        return std::asin(dSin) * 180.0 / M_PI;
    }
    default:
        /* 表的口径：下发值 = config × fScale（秒↔毫秒 0.001、置信度 10） */
        return dCfg * (double)stField.fScale;
    }
}

/* 下发值 → 配置值（Set 方向），与 wireFromConfig 严格互逆 */
double configFromWire(double dWire, const CConfigPageField_S &stField)
{
    switch (stField.nConv)
    {
    case 1:
        return dWire * (double)stField.fScale + (double)stField.fBias;
    case 2:
        return -std::sin(dWire * M_PI / 180.0);
    default:
        /* 与 wireFromConfig 严格互逆：下发值是乘过 fScale 的，回写要除 */
        if (std::fabs((double)stField.fScale) < 1e-9)
        {
            return dWire;
        }
        return dWire / (double)stField.fScale;
    }
}

/* ---- 网页侧时间字段量程上限---- */
typedef struct _WebTimeCap_
{
    ConfigPage_E enPage;
    const char  *pszPubKey;
    int32_t      nMaxSec;   /* 网页量程上限（秒） */
} CWebTimeCap_S;

const CWebTimeCap_S AST_WEB_TIME_CAP[] = {
    /* 学生 */
    { ConfigPage_E::STU_STAND,       "min_duration_s",  10 },
    { ConfigPage_E::STU_STAND,       "gap_tolerance_s", 10 },
    { ConfigPage_E::STU_RAISE_HAND,  "min_duration_s",  10 },
    { ConfigPage_E::STU_HEAD_UP,     "sample_gap_s",    10 },
    { ConfigPage_E::STU_HEAD_DOWN,   "min_duration_s",  10 },
    { ConfigPage_E::STU_TURN_BACK,   "min_duration_s",  10 },
    { ConfigPage_E::STU_PHONE,       "merge_gap_s",     10 },
    { ConfigPage_E::STU_READING,     "min_duration_s",  10 },
    { ConfigPage_E::STU_READING,     "merge_gap_s",     10 },
    { ConfigPage_E::STU_DISCUSS,     "min_duration_s",  10 },
    { ConfigPage_E::STU_PODIUM,      "min_stay_s",      10 },
    { ConfigPage_E::STU_PRESENT,     "min_duration_s",  10 },
    /* 教师 */
    { ConfigPage_E::TEA_GUIDING,     "stay_s",          10 },
    { ConfigPage_E::TEA_GUIDING,     "min_duration_s",  10 },
    { ConfigPage_E::TEA_PATROL,      "min_duration_s",  10 },
    { ConfigPage_E::TEA_STAND,       "min_duration_s",  10 },
    { ConfigPage_E::TEA_HEAD_UP,     "sample_gap_s",    10 },
    { ConfigPage_E::TEA_HEAD_DOWN,   "min_duration_s",  10 },
    { ConfigPage_E::TEA_PHONE,       "min_duration_s",  10 },
    { ConfigPage_E::TEA_TEACHING,    "min_duration_s",  10 },
    { ConfigPage_E::TEA_BOARD_WRITE, "min_duration_s",  10 },
    { ConfigPage_E::ENTER_PODIUM,    "min_stay_s",      10 },
};

/* 命中返回 true 并给出上限（秒）；未列出的页/字段完全不受影响 */
bool webTimeCapOf(ConfigPage_E enPage, const char *pszPubKey, int32_t &rnMaxSec)
{
    for (size_t i = 0; i < sizeof(AST_WEB_TIME_CAP) / sizeof(AST_WEB_TIME_CAP[0]); ++i)
    {
        if (AST_WEB_TIME_CAP[i].enPage == enPage &&
            0 == strcmp(AST_WEB_TIME_CAP[i].pszPubKey, pszPubKey))
        {
            rnMaxSec = AST_WEB_TIME_CAP[i].nMaxSec;
            return true;
        }
    }
    return false;
}

/* 下发值去浮点噪声：fScale 是 float（0.001f 的真值是 0.001000000047…），
   1500ms × 0.001f 得 1.500000071246177——网页上就是"1.500000071"这种数 */
double snapWire(double dVal)
{
    /* 5 位小数：float 配置值的相对误差约 1e-7，乘到角度上约 2e-6
       （0.75f→45.000002、0.55f→61.894737），5 位足够把它们收成 45 / 61.89474，
       而回写时 5 位带来的误差在配置侧只有 1e-7 量级，仍然逐键幂等 */
    const double dScaled = dVal * 1e5;
    return std::floor(dScaled + (dScaled >= 0.0 ? 0.5 : -0.5)) / 1e5;
}

void addConverted(cJSON *pOut, const char *pszPubKey, const cJSON *pVal,
                  const CConfigPageField_S &stField)
{
    if (nullptr == pOut || nullptr == pszPubKey || nullptr == pVal)
    {
        return;
    }
    switch (stField.nType)
    {
    case FLD_T_BOOL:
        /*  必须显式转 bool：cJSON_IsTrue() 返回的是 cJSON_bool（就是 int），
           直接传会命中 Json::add 的 int 重载，Get 出来是 "enable":1 而不是 true */
        Json::add(pOut, pszPubKey, static_cast<bool>(0 != cJSON_IsTrue(pVal)));
        break;
    case FLD_T_INT:
        if (cJSON_IsNumber(pVal))
        {
            /* 不强行取整：毫秒→秒 的换算会把 1500ms 变成 1.5s */
            double dPub = snapWire(wireFromConfig(pVal->valuedouble, stField));
            const double dRound = std::floor(dPub + 0.5);
            if (std::fabs(dPub - dRound) < 1e-6)
            {
                dPub = dRound;
            }
            Json::add(pOut, pszPubKey, dPub);
        }
        else
        {
            Json::add(pOut, pszPubKey);
        }
        break;
    case FLD_T_FLOAT:
        if (cJSON_IsNumber(pVal))
        {
            Json::add(pOut, pszPubKey, snapWire(wireFromConfig(pVal->valuedouble, stField)));
        }
        else
        {
            Json::add(pOut, pszPubKey);
        }
        break;
    case FLD_T_STRING:
        if (cJSON_IsString(pVal) && nullptr != pVal->valuestring)
        {
            Json::add(pOut, pszPubKey, std::string(pVal->valuestring));
        }
        else
        {
            Json::add(pOut, pszPubKey);
        }
        break;
    case FLD_T_IDS:
    {
        cJSON *pArr = cJSON_CreateArray();
        if (nullptr != pArr && cJSON_IsArray(pVal))
        {
            for (const cJSON *p = pVal->child; nullptr != p; p = p->next)
            {
                if (cJSON_IsNumber(p))
                {
                    cJSON *pNum = cJSON_CreateNumber(p->valuedouble);
                    if (nullptr == pNum || 0 == cJSON_AddItemToArray(pArr, pNum))
                    {
                        cJSON_Delete(pNum);
                        break;
                    }
                }
            }
        }
        if (nullptr == pArr || 0 == cJSON_AddItemToObject(pOut, pszPubKey, pArr))
        {
            cJSON_Delete(pArr);
        }
        break;
    }
    default:
        break;
    }
}

/* 按下发比例把下发值换算回 config 值：dCfg = dPub / fScale。
   fScale == 0 视为 1（1:1），表里不会出现 0，这里只是不制造一次除零崩溃 */
static inline double divByScale(double dPub, float fScale)
{
    const double dScale = (fScale == 0.0f) ? 1.0 : (double)fScale;
    return dPub / dScale;
}

/* 把下发值按 scale 换算成 config 值，返回新节点（调用方持有） */
cJSON *toConfigValue(const cJSON *pPubVal, const CConfigPageField_S &stField, bool &rbOk)
{
    rbOk = false;
    if (nullptr == pPubVal)
    {
        return nullptr;
    }
    cJSON *pOut = nullptr;
    switch (stField.nType)
    {
    case FLD_T_BOOL:
        pOut = cJSON_CreateBool(cJSON_IsTrue(pPubVal) ? 1 : 0);
        break;
    case FLD_T_INT:
        if (!cJSON_IsNumber(pPubVal))
        {
            return nullptr;
        }
        {
            /* 本表的口径是 下发值 = config 值 × fScale（秒↔毫秒 0.001、置信度 10），
               所以回写是**除**而不是乘。乘过的版本把 5 秒写成 0.005ms（取整后 0）、
               把置信度 8 写成 80——两个方向都不报错，只是那一项从此再也不变化 */
            const double dCfg = configFromWire(pPubVal->valuedouble, stField);
            pOut = cJSON_CreateNumber((double)(int64_t)(dCfg + (dCfg >= 0 ? 0.5 : -0.5)));
        }
        break;
    case FLD_T_FLOAT:
        if (!cJSON_IsNumber(pPubVal))
        {
            return nullptr;
        }
        /* 同上：除（9.0 → 0.9）。不 clamp 到 0~1：区间校验是配置层的职责，
           这里只做单位换算，越界值让配置层按既有口径拒绝或钳制 */
        {
            double dCfg = configFromWire(pPubVal->valuedouble, stField);
            /* 线性换算的浮点尾巴（0.75 → 45 → 0.750000039、120 → 90 → 120.000002）
               会让"打开再保存"改动配置文本，而那份差异没有语义。收到 6 位小数：
               量纲换算的最大精度是 0.001（毫秒↔秒）与 1/90（角度），6 位远够；
               真实带小数的值（1.5 秒、0.85 置信度）原样保留 */
            dCfg = std::round(dCfg * 1e4) / 1e4;
            pOut = cJSON_CreateNumber(dCfg);
        }
        break;
    case FLD_T_STRING:
        if (!cJSON_IsString(pPubVal))
        {
            return nullptr;
        }
        pOut = cJSON_CreateString(pPubVal->valuestring);
        break;
    case FLD_T_IDS:
    {
        pOut = cJSON_CreateArray();
        if (nullptr != pOut && cJSON_IsArray(pPubVal))
        {
            for (const cJSON *p = pPubVal->child; nullptr != p; p = p->next)
            {
                if (cJSON_IsNumber(p))
                {
                    cJSON *pNum = cJSON_CreateNumber(p->valuedouble);
                    if (nullptr == pNum || 0 == cJSON_AddItemToArray(pOut, pNum))
                    {
                        cJSON_Delete(pNum);
                        cJSON_Delete(pOut);
                        return nullptr;
                    }
                }
            }
            if (0 == cJSON_GetArraySize(pOut))
            {
                /* 空数组也是合法值（"不限区域"） */
            }
        }
        break;
    }
    default:
        break;
    }
    rbOk = (nullptr != pOut);
    return pOut;
}

} /* anonymous namespace */

const std::vector<CConfigPageField_S> &configPageFields(ConfigPage_E enFeature)
{
    static const std::vector<CConfigPageField_S> s_vecEmpty;
    const SFeatureDef_S *pDef = findFeature(enFeature);
    return (nullptr != pDef) ? pDef->vecFields : s_vecEmpty;
}

/* wire 名表：接口层唯一口径（ASCII 短名）。中文名只进日志与回显 */
static const struct { const char *pszName; ConfigPage_E enFeature; } s_aWireNames[] = {
        { "device_role",   ConfigPage_E::DEVICE_ROLE },
        { "stu_stand",     ConfigPage_E::STU_STAND },
        { "stu_raise_hand", ConfigPage_E::STU_RAISE_HAND },
        { "stu_head_up",   ConfigPage_E::STU_HEAD_UP },
        { "stu_head_down", ConfigPage_E::STU_HEAD_DOWN },
        { "stu_turn_back", ConfigPage_E::STU_TURN_BACK },
        { "stu_phone",     ConfigPage_E::STU_PHONE },
        { "stu_listen",    ConfigPage_E::STU_LISTEN },
        { "stu_reading",   ConfigPage_E::STU_READING },
        { "stu_discuss",   ConfigPage_E::STU_DISCUSS },
        { "stu_expression", ConfigPage_E::STU_EXPRESSION },
        { "attendance",    ConfigPage_E::ATTENDANCE },
        { "face_detect",   ConfigPage_E::FACE_DETECT },
        { "tea_guiding",   ConfigPage_E::TEA_GUIDING },
        { "tea_interact",  ConfigPage_E::TEA_INTERACT },
        { "tea_patrol",    ConfigPage_E::TEA_PATROL },
        { "tea_stand",     ConfigPage_E::TEA_STAND },
        { "tea_head_up",   ConfigPage_E::TEA_HEAD_UP },
        { "tea_head_down", ConfigPage_E::TEA_HEAD_DOWN },
        { "tea_phone",     ConfigPage_E::TEA_PHONE },
        { "tea_teaching",  ConfigPage_E::TEA_TEACHING },
        { "tea_board_write", ConfigPage_E::TEA_BOARD_WRITE },
        { "enter_podium",  ConfigPage_E::ENTER_PODIUM },
        { "board_ocr",     ConfigPage_E::BOARD_OCR },
        { "stu_podium",    ConfigPage_E::STU_PODIUM },
        { "stu_present",   ConfigPage_E::STU_PRESENT },
};

const char *configPageName(ConfigPage_E enFeature)
{
    for (const auto &stItem : s_aWireNames)
    {
        if (stItem.enFeature == enFeature)
        {
            return stItem.pszName;
        }
    }
    return "unknown";
}

const char *configPageTitle(ConfigPage_E enFeature)
{
    const SFeatureDef_S *pDef = findFeature(enFeature);
    return (nullptr != pDef) ? pDef->pszName : "unknown";
}

bool parseConfigPage(const char *pszName, ConfigPage_E &renFeature)
{
    if (nullptr == pszName)
    {
        return false;
    }
    for (const auto &stItem : s_aWireNames)
    {
        if (0 == strcmp(stItem.pszName, pszName))
        {
            renFeature = stItem.enFeature;
            return true;
        }
    }
    return false;
}


namespace
{

typedef struct _PageRegionPath_
{
    ConfigPage_E enPage;
    const char   *pszPath;
} CPageRegionPath_S;

const CPageRegionPath_S s_pageRegionPaths[] = {
    { ConfigPage_E::STU_STAND,      "behaviors.101.region_ids" },
    { ConfigPage_E::STU_RAISE_HAND, "behaviors.102.region_ids" },
    { ConfigPage_E::STU_HEAD_UP,    "behaviors.103.region_ids" },
    { ConfigPage_E::STU_HEAD_DOWN,  "behaviors.104.region_ids" },
    { ConfigPage_E::STU_TURN_BACK,  "behaviors.105.region_ids" },
    { ConfigPage_E::STU_PHONE,      "behaviors.106.region_ids" },
    { ConfigPage_E::STU_LISTEN,     "behaviors.107.region_ids" },
    { ConfigPage_E::STU_READING,    "behaviors.108.region_ids" },
    { ConfigPage_E::STU_DISCUSS,    "behaviors.109.region_ids" },
    { ConfigPage_E::STU_EXPRESSION, "behaviors.112.region_ids" },
    /* 12/13：考勤与人脸链自己的区域（消费者在 attendance 喂入点与人脸段） */
    { ConfigPage_E::ATTENDANCE,     "attendance.region_ids" },
    { ConfigPage_E::FACE_DETECT,    "gallery.region_ids" },
    { ConfigPage_E::TEA_STAND,      "behaviors.201.region_ids" },
    { ConfigPage_E::TEA_HEAD_UP,    "behaviors.202.region_ids" },
    { ConfigPage_E::TEA_HEAD_DOWN,  "behaviors.203.region_ids" },
    { ConfigPage_E::TEA_PHONE,      "behaviors.205.region_ids" },
    { ConfigPage_E::TEA_TEACHING,   "behaviors.206.region_ids" },
    { ConfigPage_E::TEA_GUIDING,    "behaviors.207.region_ids" },
    { ConfigPage_E::TEA_BOARD_WRITE,"behaviors.208.region_ids" },
    { ConfigPage_E::TEA_INTERACT,   "behaviors.209.region_ids" },
    { ConfigPage_E::TEA_PATROL,     "behaviors.210.region_ids" },
    /* 讲台/黑板走角色：三页共用同一块几何（画一次、改一次） */
    { ConfigPage_E::ENTER_PODIUM,   "region_roles.0.region_ids" },
    { ConfigPage_E::STU_PODIUM,     "region_roles.0.region_ids" },
    { ConfigPage_E::STU_PRESENT,    "region_roles.0.region_ids" },
    { ConfigPage_E::BOARD_OCR,      "region_roles.1.region_ids" },
};

const char *regionIdsPath(ConfigPage_E enFeature)
{
    for (const CPageRegionPath_S &stEntry : s_pageRegionPaths)
    {
        if (stEntry.enPage == enFeature)
        {
            return stEntry.pszPath;
        }
    }
    return nullptr;
}

/* 严格取节点：数组成员段只按 id/role 配对，**不做"按数字下标"的退路** */
cJSON *strictNodeAt(cJSON *pRoot, const char *pszPath)
{
    if ((nullptr == pRoot) || (nullptr == pszPath))
    {
        return nullptr;
    }
    const std::string strPath(pszPath);
    size_t nPos = 0;
    cJSON *pCur = pRoot;
    while ((nullptr != pCur) && (nPos < strPath.size()))
    {
        const size_t nDot = strPath.find('.', nPos);
        const std::string strSeg = (std::string::npos == nDot)
                                   ? strPath.substr(nPos)
                                   : strPath.substr(nPos, nDot - nPos);
        if (strSeg.empty())
        {
            return nullptr;
        }
        if (cJSON_IsArray(pCur))
        {
            if (strSeg.find_first_not_of("0123456789") != std::string::npos)
            {
                return nullptr;     /* 数组里只认数字编号，别的段名没有意义 */
            }
            const int nId = atoi(strSeg.c_str());
            cJSON *pNext = nullptr;
            for (cJSON *pItem = pCur->child; nullptr != pItem; pItem = pItem->next)
            {
                cJSON *pKey = cJSON_GetObjectItemCaseSensitive(pItem, "id");
                if (nullptr == pKey)
                {
                    pKey = cJSON_GetObjectItemCaseSensitive(pItem, "role");
                }
                if ((nullptr != pKey) && cJSON_IsNumber(pKey) && (pKey->valueint == nId))
                {
                    pNext = pItem;
                    break;
                }
            }
            pCur = pNext;       /* 配不上就是 nullptr，不猜下标 */
        }
        else
        {
            pCur = cJSON_GetObjectItemCaseSensitive(pCur, strSeg.c_str());
        }
        nPos = (std::string::npos == nDot) ? strPath.size() : nDot + 1;
    }
    return pCur;
}

/* region_ref：Web 坐标口径 + 参考分辨率。缺省与出厂配置一致（1920x1080 / 归一 0~8191） */
void readRegionRef(cJSON *pRoot, int &rnRefW, int &rnRefH, int &rnSpace)
{
    rnRefW = PIXEL_WIDTH_1920;
    rnRefH = PIXEL_HEIGHT_1080;
    rnSpace = 1;

    const cJSON *pRef = (nullptr != pRoot)
                        ? cJSON_GetObjectItemCaseSensitive(pRoot, "region_ref") : nullptr;
    if ((nullptr == pRef) || !cJSON_IsObject(pRef))
    {
        return;
    }
    const cJSON *pW = cJSON_GetObjectItemCaseSensitive(pRef, "width");
    const cJSON *pH = cJSON_GetObjectItemCaseSensitive(pRef, "height");
    const cJSON *pS = cJSON_GetObjectItemCaseSensitive(pRef, "coord_space");
    if ((nullptr != pW) && cJSON_IsNumber(pW) && (pW->valueint > 0))
    {
        rnRefW = pW->valueint;
    }
    if ((nullptr != pH) && cJSON_IsNumber(pH) && (pH->valueint > 0))
    {
        rnRefH = pH->valueint;
    }
    if ((nullptr != pS) && cJSON_IsNumber(pS))
    {
        rnSpace = pS->valueint;
    }
}

bool wireItemToRect(const cJSON *pItem, Common::Rect_S &rstRect, std::string &rstrErr, int nIndex)
{
    const cJSON *pRect = pItem;
    const char *pszX = "X";
    const char *pszY = "Y";
    const char *pszW = "W";
    const char *pszH = "H";
    if (nullptr == cJSON_GetObjectItemCaseSensitive(pItem, pszX))
    {
        const cJSON *pSt = cJSON_GetObjectItemCaseSensitive(pItem, "stRect");
        if ((nullptr != pSt) && cJSON_IsObject(pSt))
        {
            pRect = pSt;
            pszX = "nX";
            pszY = "nY";
            pszW = "nWidth";
            pszH = "nHeight";
        }
        else
        {
            const cJSON *pLegacy = cJSON_GetObjectItemCaseSensitive(pItem, "Rect");
            if (nullptr != pLegacy)
            {
                pRect = pLegacy;
            }
        }
    }
    if (!cJSON_IsObject(pRect))
    {
        rstrErr = "regions[" + std::to_string(nIndex) + "] is not an object";
        return false;
    }

    const cJSON *pX = cJSON_GetObjectItemCaseSensitive(pRect, pszX);
    const cJSON *pY = cJSON_GetObjectItemCaseSensitive(pRect, pszY);
    const cJSON *pW = cJSON_GetObjectItemCaseSensitive(pRect, pszW);
    const cJSON *pH = cJSON_GetObjectItemCaseSensitive(pRect, pszH);
    if ((nullptr == pX) || (nullptr == pY) || (nullptr == pW) || (nullptr == pH) ||
        !cJSON_IsNumber(pX) || !cJSON_IsNumber(pY) || !cJSON_IsNumber(pW) || !cJSON_IsNumber(pH))
    {
        rstrErr = "regions[" + std::to_string(nIndex) + "] needs X/Y/W/H (numbers, 1920x1080)";
        return false;
    }

    rstRect.nX = pX->valueint;
    rstRect.nY = pY->valueint;
    rstRect.nWidth = pW->valueint;
    rstRect.nHeight = pH->valueint;

    /* 与旧项目同一条纪律：先 IsValid 再换算。负值/越界**拒**而不是钳制——
       钳制会把"画到外面"变成"悄悄换了个框"，现场看不出自己画错了 */
    if (!rstRect.IsValid(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080))
    {
        rstrErr = "regions[" + std::to_string(nIndex) + "].stRect invalid (must be inside 1920x1080)";
        return false;
    }
    /* 零宽高：IsValid 只查边界，不查面积；画了一条线不算画了区域 */
    if (rstRect.isEmpty())
    {
        rstrErr = "regions[" + std::to_string(nIndex) + "].stRect empty (nWidth/nHeight must be > 0)";
        return false;
    }
    return true;
}

/* 分析画布宽度（像素字段的换算基准）。缺键按 1920 = 不做换算，与旧行为一致：
   出厂与现网都是 1920×1080，只有把 video.canvas_width 改小时才真正生效 */
double canvasWidthOf(cJSON *pRoot)
{
    if (nullptr == pRoot)
    {
        return 1920.0;
    }
    const cJSON *pVideo = cJSON_GetObjectItemCaseSensitive(pRoot, "video");
    const cJSON *pW = (nullptr != pVideo)
                      ? cJSON_GetObjectItemCaseSensitive(pVideo, "canvas_width") : nullptr;
    if ((nullptr == pW) || !cJSON_IsNumber(pW))
    {
        return 1920.0;
    }
    const double dW = pW->valuedouble;
    if (!(dW >= 1.0) || dW > 8192.0)
    {
        return 1920.0;
    }
    return dW;
}

/* 区域换算：与 Common::Rect_S::ConvertResolution 同一个公式（dst/src 线性缩放），
   但每个边**四舍五入**而不是截断 */
void scaleRect(Common::Rect_S &rstRect, int nSrcW, int nSrcH, int nDstW, int nDstH)
{
    const double dX = (static_cast<double>(nDstW) / nSrcW);
    const double dY = (static_cast<double>(nDstH) / nSrcH);
    const int nX1 = static_cast<int>(std::lround(rstRect.nX * dX));
    const int nY1 = static_cast<int>(std::lround(rstRect.nY * dY));
    const int nX2 = static_cast<int>(std::lround((rstRect.nX + rstRect.nWidth) * dX));
    const int nY2 = static_cast<int>(std::lround((rstRect.nY + rstRect.nHeight) * dY));
    rstRect.nX = nX1;
    rstRect.nY = nY1;
    rstRect.nWidth = (nX2 > nX1) ? (nX2 - nX1) : 0;
    rstRect.nHeight = (nY2 > nY1) ? (nY2 - nY1) : 0;
}

/* 配置里的 regions[] 按 id 找一块 */
cJSON *findRegionItem(cJSON *pRegions, int nId)
{
    if ((nullptr == pRegions) || !cJSON_IsArray(pRegions))
    {
        return nullptr;
    }
    for (cJSON *pItem = pRegions->child; nullptr != pItem; pItem = pItem->next)
    {
        const cJSON *pId = cJSON_GetObjectItemCaseSensitive(pItem, "id");
        if ((nullptr != pId) && cJSON_IsNumber(pId) && (pId->valueint == nId))
        {
            return pItem;
        }
    }
    return nullptr;
}

/* 写一个数字键（已存在就改值，不重链——重链会丢键名，见 nodeSet 顶层的注释） */
bool setNumber(cJSON *pParent, const char *pszKey, double dVal)
{
    cJSON *pOld = cJSON_GetObjectItemCaseSensitive(pParent, pszKey);
    if (nullptr != pOld)
    {
        if (cJSON_IsNumber(pOld))
        {
            cJSON_SetNumberValue(pOld, dVal);
            return true;
        }
        cJSON *pNew = cJSON_CreateNumber(dVal);
        if (nullptr == pNew)
        {
            return false;
        }
        return 0 != cJSON_ReplaceItemInObjectCaseSensitive(pParent, pszKey, pNew);
    }
    return nullptr != cJSON_AddNumberToObject(pParent, pszKey, dVal);
}

/* nodeSet 的中间层缺失时会补一个**对象**，而 behaviors / region_roles 是数组。
   少了这一段，配置里整个键缺失时写出来是 {"region_roles":{"0":{...}}}，
   解析器按数组读，等于"保存成功、一点反应都没有"。只在缺键时补，已有就原样用 */
bool ensureArraySection(cJSON *pRoot, const char *pszIdsPath)
{
    /* 把 region_ids 的**父容器链**建出来（缺哪层补哪层）*/
    std::vector<std::string> vecSeg;
    const std::string strPath(pszIdsPath);
    size_t nPos = 0;
    while (nPos <= strPath.size())
    {
        const size_t nDot = strPath.find('.', nPos);
        const std::string strSeg = (std::string::npos == nDot)
                                   ? strPath.substr(nPos)
                                   : strPath.substr(nPos, nDot - nPos);
        if (!strSeg.empty())
        {
            vecSeg.push_back(strSeg);
        }
        if (std::string::npos == nDot)
        {
            break;
        }
        nPos = nDot + 1;
    }
    if (vecSeg.size() < 2)
    {
        return true;   /* 顶层键：nodeSet 自己会建 */
    }

    const auto fnNumeric = [](const std::string &str) -> bool {
        return !str.empty() && (str.find_first_not_of("0123456789") == std::string::npos);
    };

    cJSON *pCur = pRoot;
    std::string strArrayName;
    for (size_t i = 0; i + 1 < vecSeg.size(); ++i)
    {
        const std::string &strSeg = vecSeg[i];
        const bool bNextNumeric = fnNumeric(vecSeg[i + 1]);
        cJSON *pChild = nullptr;
        if (cJSON_IsArray(pCur))
        {
            if (!fnNumeric(strSeg))
            {
                return false;   /* 数组里只能按编号走 */
            }
            const int nKey = static_cast<int>(atoi(strSeg.c_str()));
            for (cJSON *pIt = pCur->child; nullptr != pIt; pIt = pIt->next)
            {
                const cJSON *pId = cJSON_GetObjectItemCaseSensitive(pIt, "id");
                if (nullptr == pId)
                {
                    pId = cJSON_GetObjectItemCaseSensitive(pIt, "role");
                }
                if ((nullptr != pId) && cJSON_IsNumber(pId) && (pId->valueint == nKey))
                {
                    pChild = pIt;
                    break;
                }
            }
            if ((nullptr == pChild) && (0 == strcmp(strArrayName.c_str(), "region_roles")))
            {
                /* 只有角色表允许补行：三页共用一块讲台区域，若配置里原本只有黑板，
                   画讲台时要能新建 role=0 那一行 */
                pChild = cJSON_CreateObject();
                if (nullptr == pChild)
                {
                    return false;
                }
                cJSON_AddNumberToObject(pChild, "role", nKey);
                if (0 == cJSON_AddItemToArray(pCur, pChild))
                {
                    cJSON_Delete(pChild);
                    return false;
                }
                pCur = pChild;
                continue;
            }
            if (nullptr == pChild)
            {
                return false;   /* behaviors 缺行：不凭空造 */
            }
        }
        else if (cJSON_IsObject(pCur))
        {
            pChild = cJSON_GetObjectItemCaseSensitive(pCur, strSeg.c_str());
            if ((nullptr == pChild) && bNextNumeric)
            {
                /* 数组整段缺失（老配置没有 behaviors）：建空数组交给下一轮按 id 找，
                   找不到会返回 false，不会伪造一行 */
                pChild = cJSON_CreateArray();
                if ((nullptr == pChild) || (0 == cJSON_AddItemToObject(pCur, strSeg.c_str(), pChild)))
                {
                    cJSON_Delete(pChild);
                    return false;
                }
            }
            else if (nullptr == pChild)
            {
                pChild = cJSON_CreateObject();
                if ((nullptr == pChild) || (0 == cJSON_AddItemToObject(pCur, strSeg.c_str(), pChild)))
                {
                    cJSON_Delete(pChild);
                    return false;
                }
            }
        }
        else
        {
            return false;
        }

        if (cJSON_IsArray(pChild))
        {
            strArrayName = strSeg;
        }
        pCur = pChild;
    }
    return true;
}

/* 整份配置里所有 region_ids 引用方（消费者只有这四处：行为行、角色行、考勤、人脸） */
void collectReferencedRegionIds(const cJSON *pRoot, std::set<int> &rsetIds)
{
    const auto fnAdd = [&rsetIds](const cJSON *pNode) {
        if ((nullptr == pNode) || !cJSON_IsArray(pNode))
        {
            return;
        }
        for (const cJSON *pId = pNode->child; nullptr != pId; pId = pId->next)
        {
            if (cJSON_IsNumber(pId))
            {
                rsetIds.insert(pId->valueint);
            }
        }
    };

    const char *apszRowLists[] = { "behaviors", "region_roles" };
    for (const char *pszList : apszRowLists)
    {
        const cJSON *pRows = cJSON_GetObjectItemCaseSensitive(pRoot, pszList);
        if ((nullptr == pRows) || !cJSON_IsArray(pRows))
        {
            continue;
        }
        for (const cJSON *pRow = pRows->child; nullptr != pRow; pRow = pRow->next)
        {
            fnAdd(cJSON_GetObjectItemCaseSensitive(pRow, "region_ids"));
        }
    }

    const char *apszSections[] = { "attendance", "gallery" };
    for (const char *pszSec : apszSections)
    {
        const cJSON *pSec = cJSON_GetObjectItemCaseSensitive(pRoot, pszSec);
        if ((nullptr == pSec) || !cJSON_IsObject(pSec))
        {
            continue;
        }
        fnAdd(cJSON_GetObjectItemCaseSensitive(pSec, "region_ids"));
    }
}

/* 给"新画的一块"分配 id：先复用没人引用的旧块（清空/重画留下的孤儿），没有再取
   现有最大 id + 1 */
int allocRegionId(cJSON *pRoot, cJSON *pRegions, const std::vector<int> &vecTaken)
{
    std::set<int> setReferenced;
    collectReferencedRegionIds(pRoot, setReferenced);

    for (cJSON *pItem = pRegions->child; nullptr != pItem; pItem = pItem->next)
    {
        const cJSON *pId = cJSON_GetObjectItemCaseSensitive(pItem, "id");
        if ((nullptr == pId) || !cJSON_IsNumber(pId))
        {
            continue;
        }
        const int nId = pId->valueint;
        if ((0 == setReferenced.count(nId)) &&
            (vecTaken.end() == std::find(vecTaken.begin(), vecTaken.end(), nId)))
        {
            return nId;
        }
    }

    int nNextId = 1;
    for (cJSON *pItem = pRegions->child; nullptr != pItem; pItem = pItem->next)
    {
        const cJSON *pId = cJSON_GetObjectItemCaseSensitive(pItem, "id");
        if ((nullptr != pId) && cJSON_IsNumber(pId) && (pId->valueint >= nNextId))
        {
            nNextId = pId->valueint + 1;
        }
    }
    for (const int nId : setReferenced)
    {
        if (nId >= nNextId)
        {
            nNextId = nId + 1;
        }
    }
    for (const int nId : vecTaken)
    {
        if (nId >= nNextId)
        {
            nNextId = nId + 1;
        }
    }
    return nNextId;
}

/* 把一页送来的 regions 落成几何 + 回填本页 region_ids。
   语义：① 不带 regions 不动（调用方保证）；② [] = 该页不限制区域；
   ③ 其余按顺序与已有 region_ids 配对（重画 = 换几何，不换引用），
      多出来的框新分配 id，少了的从本页摘掉；别的页引用的 id 一律不碰。 */
bool applyPageRegions(cJSON *pBase, const char *pszIdsPath, const cJSON *pIn,
                      std::string &rstrErr)
{
    int nRefW = 0;
    int nRefH = 0;
    int nSpace = 1;
    readRegionRef(pBase, nRefW, nRefH, nSpace);

    /* 本页现有的 region_ids */
    std::vector<int> vecIds;
    cJSON *pIdsNode = strictNodeAt(pBase, pszIdsPath);
    if ((nullptr != pIdsNode) && cJSON_IsArray(pIdsNode))
    {
        for (cJSON *pChild = pIdsNode->child; nullptr != pChild; pChild = pChild->next)
        {
            if (cJSON_IsNumber(pChild))
            {
                vecIds.push_back(pChild->valueint);
            }
        }
    }

    cJSON *pRegions = cJSON_GetObjectItemCaseSensitive(pBase, "regions");
    if (nullptr == pRegions)
    {
        /* 老配置可能整个 regions 键都没有（出厂是 []）：补一张空表再接 */
        pRegions = cJSON_CreateArray();
        if ((nullptr == pRegions) || (0 == cJSON_AddItemToObject(pBase, "regions", pRegions)))
        {
            cJSON_Delete(pRegions);
            rstrErr = "out of memory";
            return false;
        }
    }
    if (!cJSON_IsArray(pRegions))
    {
        rstrErr = "config regions[] is not an array";
        return false;
    }

    const int nCount = cJSON_GetArraySize(pIn);
    std::vector<int> vecNewIds;
    for (int i = 0; i < nCount; ++i)
    {
        const cJSON *pItem = cJSON_GetArrayItem(pIn, i);
        if (!cJSON_IsObject(pItem))
        {
            rstrErr = "regions[" + std::to_string(i) + "] is not an object";
            return false;
        }

        Common::Rect_S stRect;
        if (!wireItemToRect(pItem, stRect, rstrErr, i))
        {
            return false;
        }

        /* 「整幅画面」与「不限区域」是同一件事 */
        if (vecIds.empty() && (stRect.nX <= 0) && (stRect.nY <= 0) &&
            (stRect.nWidth >= PIXEL_WIDTH_1920) && (stRect.nHeight >= PIXEL_HEIGHT_1080))
        {
            continue;
        }

        /* 1920x1080 → 配置口径：coord_space=1 落到 0~8191 归一，=0 落到参考分辨率像素。
           用旧项目同一个 ConvertResolution，两个方向（Set 正、Get 反）才是同一把尺 */
        const int nDstW = (1 == nSpace) ? 8191 : nRefW;
        const int nDstH = (1 == nSpace) ? 8191 : nRefH;
        scaleRect(stRect, PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, nDstW, nDstH);
        if ((stRect.nWidth <= 0) || (stRect.nHeight <= 0))
        {
            rstrErr = "region too small after conversion";
            return false;
        }

        const int nId = (static_cast<size_t>(i) < vecIds.size())
                        ? vecIds[i]
                        : allocRegionId(pBase, pRegions, vecNewIds);
        vecNewIds.push_back(nId);

        cJSON *pRect = cJSON_CreateObject();
        if (nullptr == pRect)
        {
            rstrErr = "out of memory";
            return false;
        }
        cJSON_AddNumberToObject(pRect, "x1", stRect.nX);
        cJSON_AddNumberToObject(pRect, "y1", stRect.nY);
        cJSON_AddNumberToObject(pRect, "x2", stRect.nX + stRect.nWidth);
        cJSON_AddNumberToObject(pRect, "y2", stRect.nY + stRect.nHeight);

        cJSON *pTarget = findRegionItem(pRegions, nId);
        if (nullptr != pTarget)
        {
            cJSON *pOldRect = cJSON_GetObjectItemCaseSensitive(pTarget, "rect");
            bool bSameRect = false;
            if ((nullptr != pOldRect) && cJSON_IsObject(pOldRect))
            {
                const char *apszKey[4] = { "x1", "y1", "x2", "y2" };
                const int anNew[4] = { stRect.nX, stRect.nY,
                                       stRect.nX + stRect.nWidth, stRect.nY + stRect.nHeight };
                bSameRect = true;
                for (int k = 0; k < 4; ++k)
                {
                    const cJSON *pOldV = cJSON_GetObjectItemCaseSensitive(pOldRect, apszKey[k]);
                    if ((nullptr == pOldV) || !cJSON_IsNumber(pOldV) ||
                        (std::abs(pOldV->valueint - anNew[k]) > 1))
                    {
                        bSameRect = false;
                        break;
                    }
                }
            }
            if (bSameRect)
            {
                cJSON_Delete(pRect);
                if (!setNumber(pTarget, "shape", 0))
                {
                    rstrErr = "cannot set shape of region " + std::to_string(nId);
                    return false;
                }
                continue;
            }
            if (nullptr != pOldRect)
            {
                if (0 == cJSON_ReplaceItemInObjectCaseSensitive(pTarget, "rect", pRect))
                {
                    cJSON_Delete(pRect);
                    rstrErr = "cannot replace rect of region " + std::to_string(nId);
                    return false;
                }
            }
            else if (0 == cJSON_AddItemToObject(pTarget, "rect", pRect))
            {
                cJSON_Delete(pRect);
                rstrErr = "cannot add rect to region " + std::to_string(nId);
                return false;
            }
            /* 原来是多边形/未知形状：改成矩形，否则规则层会按 points 取几何 */
            if (!setNumber(pTarget, "shape", 0))
            {
                rstrErr = "cannot set shape of region " + std::to_string(nId);
                return false;
            }
        }
        else
        {
            cJSON *pNew = cJSON_CreateObject();
            if (nullptr == pNew)
            {
                cJSON_Delete(pRect);
                rstrErr = "out of memory";
                return false;
            }
            cJSON_AddNumberToObject(pNew, "id", (double)nId);
            cJSON_AddNumberToObject(pNew, "shape", 0);
            cJSON_AddItemToObject(pNew, "rect", pRect);
            if (0 == cJSON_AddItemToArray(pRegions, pNew))
            {
                cJSON_Delete(pNew);
                rstrErr = "out of memory";
                return false;
            }
        }
    }

    if (vecNewIds.empty() && (nullptr == pIdsNode))
    {
        return true;
    }

    if (!ensureArraySection(pBase, pszIdsPath))
    {
        rstrErr = "config section of region_ids is not an array";
        return false;
    }
    cJSON *pIdsNew = cJSON_CreateArray();
    if (nullptr == pIdsNew)
    {
        rstrErr = "out of memory";
        return false;
    }
    for (size_t i = 0; i < vecNewIds.size(); ++i)
    {
        cJSON *pIdVal = cJSON_CreateNumber((double)vecNewIds[i]);
        if ((nullptr == pIdVal) || (0 == cJSON_AddItemToArray(pIdsNew, pIdVal)))
        {
            cJSON_Delete(pIdVal);
            cJSON_Delete(pIdsNew);
            rstrErr = "out of memory";
            return false;
        }
    }
    if (!nodeSet(pBase, pszIdsPath, pIdsNew))
    {
        cJSON_Delete(pIdsNew);
        rstrErr = "cannot write region_ids";
        return false;
    }
    return true;
}

/* Get 方向：把本页 region_ids 指向的几何转回网页的 1920x1080 stRect。
   与 Set 严格互逆（同一个 ConvertResolution，src/dst 对调）。 */
void appendPageRegions(cJSON *pOut, cJSON *pRoot, const char *pszIdsPath)
{
    Common::Rect_S stOut;
    stOut.nX = 0;
    stOut.nY = 0;
    stOut.nWidth = PIXEL_WIDTH_1920;
    stOut.nHeight = PIXEL_HEIGHT_1080;

    int nRefW = 0;
    int nRefH = 0;
    int nSpace = 1;
    readRegionRef(pRoot, nRefW, nRefH, nSpace);

    const cJSON *pIds = strictNodeAt(pRoot, pszIdsPath);
    cJSON *pRegions = cJSON_GetObjectItemCaseSensitive(pRoot, "regions");
    if ((nullptr != pIds) && cJSON_IsArray(pIds) &&
        (nullptr != pRegions) && cJSON_IsArray(pRegions))
    {
        const int nSrcW = (1 == nSpace) ? 8191 : nRefW;
        const int nSrcH = (1 == nSpace) ? 8191 : nRefH;
        for (const cJSON *pId = pIds->child; nullptr != pId; pId = pId->next)
        {
            if (!cJSON_IsNumber(pId))
            {
                continue;
            }
            /* 引用了不存在的块：配置校验会整份拒，这里只跳过（不让 Get 整页失败） */
            const cJSON *pItem = findRegionItem(pRegions, pId->valueint);
            if (nullptr == pItem)
            {
                continue;
            }
            const cJSON *pRect = cJSON_GetObjectItemCaseSensitive(pItem, "rect");
            if ((nullptr == pRect) || !cJSON_IsObject(pRect))
            {
                continue;
            }
            const cJSON *pX1 = cJSON_GetObjectItemCaseSensitive(pRect, "x1");
            const cJSON *pY1 = cJSON_GetObjectItemCaseSensitive(pRect, "y1");
            const cJSON *pX2 = cJSON_GetObjectItemCaseSensitive(pRect, "x2");
            const cJSON *pY2 = cJSON_GetObjectItemCaseSensitive(pRect, "y2");
            if ((nullptr == pX1) || (nullptr == pY1) || (nullptr == pX2) || (nullptr == pY2) ||
                !cJSON_IsNumber(pX1) || !cJSON_IsNumber(pY1) ||
                !cJSON_IsNumber(pX2) || !cJSON_IsNumber(pY2))
            {
                continue;
            }

            Common::Rect_S stRect;
            stRect.nX = (int)(pX1->valuedouble + 0.5);
            stRect.nY = (int)(pY1->valuedouble + 0.5);
            stRect.nWidth = (int)(pX2->valuedouble + 0.5) - stRect.nX;
            stRect.nHeight = (int)(pY2->valuedouble + 0.5) - stRect.nY;
            if ((stRect.nWidth <= 0) || (stRect.nHeight <= 0))
            {
                continue;
            }
            scaleRect(stRect, nSrcW, nSrcH, PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080);
            if ((stRect.nWidth <= 0) || (stRect.nHeight <= 0))
            {
                continue;
            }
            stOut = stRect;   /* 只取第一块：一个事件就一块区域 */
            break;
        }
    }

    cJSON *pOne = cJSON_CreateObject();
    if (nullptr == pOne)
    {
        return;
    }
    cJSON_AddNumberToObject(pOne, "X", stOut.nX);
    cJSON_AddNumberToObject(pOne, "Y", stOut.nY);
    cJSON_AddNumberToObject(pOne, "W", stOut.nWidth);
    cJSON_AddNumberToObject(pOne, "H", stOut.nHeight);
    cJSON_AddItemToObject(pOut, "regions", pOne);
}

} /* anonymous namespace */

std::string readConfigPage(const std::string &strConfigJson, ConfigPage_E enFeature)
{
    const std::vector<CConfigPageField_S> &vecFields = configPageFields(enFeature);
    if (vecFields.empty())
    {
        return "{}";
    }
    Json::Object *pRoot = Json::init(strConfigJson.empty() ? "{}" : strConfigJson);
    cJSON *pOut = cJSON_CreateObject();
    if (nullptr == pRoot || nullptr == pOut)
    {
        Json::deinit(pRoot);
        cJSON_Delete(pOut);
        /* 原文为空/坏：返回一个"每键都是 null"的对象，让网页能把整页画出来。
            返回 "{}" 会让网页以为"这一页没有参数"，那是另一种错 */
        cJSON *pNull = cJSON_CreateObject();
        for (const CConfigPageField_S &stField : vecFields)
        {
            Json::add(pNull, stField.pszPubKey);
        }
        char *pszText = (nullptr != pNull) ? cJSON_PrintUnformatted(pNull) : nullptr;
        std::string strOut = (nullptr != pszText) ? pszText : "{}";
        if (nullptr != pszText)
        {
            free(pszText);
        }
        cJSON_Delete(pNull);
        return strOut;
    }

    for (const CConfigPageField_S &stField : vecFields)
    {
        cJSON *pVal = nodeAt(pRoot, stField.pszPath);
        if (nullptr == pVal)
        {
            /* 键不存在（老配置没这个键）：有明确默认值的字段回默认，
               其余给 null 而不是给 0。给 0 会让网页把它显示成一个真实值，
               用户没动、保存就写回 0；null 表示"配置里没有这一项" */
            if (stField.bHasDefault)
            {
                cJSON *pSynth = cJSON_CreateNumber(stField.dDefault);
                addConverted(pOut, stField.pszPubKey, pSynth, stField);
                cJSON_Delete(pSynth);
            }
            else
            {
                Json::add(pOut, stField.pszPubKey);
            }
            continue;
        }
        if ((3 == stField.nConv) && cJSON_IsNumber(pVal))
        {
            /* 画布像素 → 网页的 1920 口径（canvas=1920 时原样） */
            Json::add(pOut, stField.pszPubKey, pVal->valuedouble * 1920.0 / canvasWidthOf(pRoot));
            continue;
        }
        addConverted(pOut, stField.pszPubKey, pVal, stField);
    }

    /* 这一页有区域键：把 region_ids 指向的几何一并回给网页（1920x1080 口径）。
       没有区域键的页（摄像机类型/考勤/人脸）不会多出这个键，网页按"这一页没有绘制区域"画 */
    const char *pszIdsPath = regionIdsPath(enFeature);
    if (nullptr != pszIdsPath)
    {
        appendPageRegions(pOut, pRoot, pszIdsPath);
    }

    char *pszText = cJSON_PrintUnformatted(pOut);
    std::string strOut = (nullptr != pszText) ? pszText : "{}";
    if (nullptr != pszText)
    {
        free(pszText);
    }
    cJSON_Delete(pOut);
    Json::deinit(pRoot);
    return strOut;
}

bool mergeConfigPage(const std::string &strConfigJson, ConfigPage_E enFeature,
                    const std::string &strPartial, std::string &rstrMerged,
                    std::string &rstrErr)
{
    const std::vector<CConfigPageField_S> &vecFields = configPageFields(enFeature);
    if (vecFields.empty())
    {
        rstrErr = "this page has no configurable field";
        return false;
    }

    Json::Object *pBase = Json::init(strConfigJson.empty() ? "{}" : strConfigJson);
    if (nullptr == pBase)
    {
        rstrErr = "current config text is not valid json";
        Json::deinit(pBase);
        return false;
    }
    cJSON *pPart = cJSON_Parse(strPartial.c_str());
    if (nullptr == pPart)
    {
        rstrErr = "web payload is not valid json";
        Json::deinit(pPart);
        Json::deinit(pBase);
        return false;
    }

    /* 只认本页声明的字段；一个都没命中说明网页与本页口径不一致，报错而不是静默成功 */
    int32_t nApplied = 0;
    for (const CConfigPageField_S &stField : vecFields)
    {
        cJSON *pPubVal = cJSON_GetObjectItem(pPart, stField.pszPubKey);
        if ((nullptr == pPubVal) || cJSON_IsNull(pPubVal))
        {
            /* 没送这个字段：不动它（这正是"只改一项"的语义） */
            continue;
        }

        if (0 == strcmp(stField.pszPubKey, "role"))
        {
            const char *pszRole = cJSON_IsString(pPubVal) ? pPubVal->valuestring : nullptr;
            if ((nullptr == pszRole) ||
                (0 != strcmp(pszRole, "student") && 0 != strcmp(pszRole, "teacher")))
            {
                rstrErr = "role must be \"student\" or \"teacher\" (they are mutually exclusive)";
                cJSON_Delete(pPart);
                Json::deinit(pBase);
                return false;
            }
        }
        bool bOk = false;
        cJSON *pCfgVal = nullptr;
        if (3 == stField.nConv)
        {
            /* 网页 1920 口径 → 画布像素（四舍五入；canvas=1920 时原样） */
            if (cJSON_IsNumber(pPubVal))
            {
                pCfgVal = cJSON_CreateNumber(
                    std::round(pPubVal->valuedouble * canvasWidthOf(pBase) / 1920.0));
                bOk = (nullptr != pCfgVal);
            }
        }
        else
        {
            pCfgVal = toConfigValue(pPubVal, stField, bOk);
        }
        if (!bOk || nullptr == pCfgVal)
        {
            cJSON_Delete(pCfgVal);
            rstrErr = std::string("bad value for field ") + stField.pszPubKey;
            cJSON_Delete(pPart);
            Json::deinit(pBase);
            return false;
        }
        /* 网页量程上限（秒 → 配置量纲）：超过 10s 就夹回 10s 再落盘，负值归零 */
        int32_t nCapSec = 0;
        if (cJSON_IsNumber(pCfgVal) && webTimeCapOf(enFeature, stField.pszPubKey, nCapSec))
        {
            const double dMaxCfg = configFromWire(static_cast<double>(nCapSec), stField);
            const double dHi = static_cast<double>(static_cast<int64_t>(dMaxCfg + 0.5));
            const double dVal = pCfgVal->valuedouble;
            if (dVal < 0.0)
            {
                cJSON_SetNumberValue(pCfgVal, 0.0);
            }
            else if (dVal > dHi)
            {
                cJSON_SetNumberValue(pCfgVal, dHi);
            }
        }
        if (!nodeSet(pBase, stField.pszPath, pCfgVal))
        {
            cJSON_Delete(pCfgVal);
            rstrErr = std::string("cannot write ") + stField.pszPath;
            cJSON_Delete(pPart);
            Json::deinit(pBase);
            return false;
        }
        ++nApplied;
    }

    /* 这一页的「绘制区域」：与上面那些字段同一条语义——送了就改、没送不动。
       [] 表示"这一页不限制区域"；逐条先 IsValid(1920,1080) 再换算，
       任一条不合法就整份拒绝 */
    const cJSON *pRegionsIn = cJSON_GetObjectItemCaseSensitive(pPart, "regions");
    if (nullptr != pRegionsIn)
    {
        const char *pszIdsPath = regionIdsPath(enFeature);
        if (nullptr == pszIdsPath)
        {
            rstrErr = "this page has no region field";
            cJSON_Delete(pPart);
            Json::deinit(pBase);
            return false;
        }

        cJSON *pArrIn = cJSON_CreateArray();
        if (nullptr == pArrIn)
        {
            rstrErr = "out of memory";
            cJSON_Delete(pPart);
            Json::deinit(pBase);
            return false;
        }
        const cJSON *pOne = pRegionsIn;
        if (cJSON_IsArray(pRegionsIn))
        {
            /* [] = 清空（要求里的"单击清除后重新绘制"）；>1 块拒（每页只有一块） */
            const int nSize = cJSON_GetArraySize(pRegionsIn);
            if (nSize > 1)
            {
                cJSON_Delete(pArrIn);
                rstrErr = "each page has exactly one region (send [] to clear)";
                cJSON_Delete(pPart);
                Json::deinit(pBase);
                return false;
            }
            if (0 == nSize)
            {
                const bool bCleared = applyPageRegions(pBase, pszIdsPath, pArrIn, rstrErr);
                if (!bCleared)
                {
                    cJSON_Delete(pArrIn);
                    cJSON_Delete(pPart);
                    Json::deinit(pBase);
                    return false;
                }
                ++nApplied;
                pOne = nullptr;   /* 已清空：下面不再写几何 */
            }
            else
            {
                pOne = cJSON_GetArrayItem(pRegionsIn, 0);
            }
        }
        if (nullptr != pOne)
        {
            if (!cJSON_IsObject(pOne))
            {
                cJSON_Delete(pArrIn);
                rstrErr = "regions must be one object {X,Y,W,H}";
                cJSON_Delete(pPart);
                Json::deinit(pBase);
                return false;
            }
            cJSON *pCopy = cJSON_Duplicate(pOne, 1);
            if ((nullptr == pCopy) || (0 == cJSON_AddItemToArray(pArrIn, pCopy)))
            {
                cJSON_Delete(pCopy);
                cJSON_Delete(pArrIn);
                rstrErr = "out of memory";
                cJSON_Delete(pPart);
                Json::deinit(pBase);
                return false;
            }
            const bool bApplied = applyPageRegions(pBase, pszIdsPath, pArrIn, rstrErr);
            cJSON_Delete(pArrIn);
            if (!bApplied)
            {
                cJSON_Delete(pPart);
                Json::deinit(pBase);
                return false;
            }
            ++nApplied;
        }
        else
        {
            cJSON_Delete(pArrIn);
        }
    }

    if (0 == nApplied)
    {
        rstrErr = "no recognized field in payload for this page";
        cJSON_Delete(pPart);
        Json::deinit(pBase);
        return false;
    }

    char *pszText = cJSON_PrintUnformatted(pBase);
    if (nullptr == pszText)
    {
        rstrErr = "out of memory";
        cJSON_Delete(pPart);
        Json::deinit(pBase);
        return false;
    }
    rstrMerged = pszText;
    free(pszText);

    cJSON_Delete(pPart);
    Json::deinit(pBase);
    return true;
}

} /* namespace Classroom */
