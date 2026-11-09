/**
 * @FilePath     : classroom_config_adapter.h
 * @Author       : leiyy
 * @Date         : 2026-10-27 14:12:04
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-27 14:12:04
 * @Description  : 课堂 AI 分析配置下发适配层（取值/回填）
 */

#pragma once

#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG

#include <string>
#include <vector>

namespace Classroom
{

/**
 * @brief 功能页标识（与 Task::Classroom 的接口一一对应）
 */
enum class ConfigPage_E
{
    DEVICE_ROLE = 0,     /* 1 摄像机类型 */
    STU_STAND,           /* 2 学生-站立 */
    STU_RAISE_HAND,      /* 3 学生-举手 */
    STU_HEAD_UP,         /* 4 学生-抬头 */
    STU_HEAD_DOWN,       /* 5 学生-低头 */
    STU_TURN_BACK,       /* 6 学生-背身 */
    STU_PHONE,           /* 7 学生-使用手机 */
    STU_LISTEN,          /* 8 学生-听讲 */
    STU_READING,         /* 9 学生-阅读 */
    STU_DISCUSS,         /* 10 学生-讨论 */
    STU_EXPRESSION,      /* 11 学生-表情 */
    ATTENDANCE,          /* 12 考勤人数统计 */
    FACE_DETECT,         /* 13 学生人脸检测 */
    TEA_GUIDING,         /* 14 教师-指导学生 */
    TEA_INTERACT,        /* 15 教师-师生互动 */
    TEA_PATROL,          /* 16 教师-巡视 */
    TEA_STAND,           /* 17 教师-站立 */
    TEA_HEAD_UP,         /* 18 教师-抬头 */
    TEA_HEAD_DOWN,       /* 19 教师-低头 */
    TEA_PHONE,           /* 20 教师-使用手机 */
    TEA_TEACHING,        /* 21 教师-讲授 */
    TEA_BOARD_WRITE,     /* 22 教师-书写板书 */
    ENTER_PODIUM,        /* 23 进入讲台 */
    BOARD_OCR,           /* 24 黑板内容识别 */
    STU_PODIUM,          /* 25 学生-上讲台 */
    STU_PRESENT,         /* 26 学生-演示 */
    PAGE_NUM
};

/**
 * @brief   一个下发字段 ↔ 配置键的映射
 */
typedef struct _ConfigPageField_
{
    const char *pszPubKey;        /* 下发字段名（wire 名，中文页标题不进来） */
    const char *pszPath;          /* 配置点分路径；"behaviors.101" 走数组成员 */
    float       fScale;
    int32_t     nType;
    float       fBias;
    int32_t     nConv;
    double      dDefault;
    bool        bHasDefault;
} CConfigPageField_S;

/**
 * @brief   : 取一页的字段映射表（下发字段 ↔ 配置键的唯一映射）
 */
const std::vector<CConfigPageField_S> &configPageFields(ConfigPage_E enFeature);

/**
 * @brief   : 从当前配置原文抽一页的值，按网页字段名与单位输出（Get 用）
 * @param    {const std::string&} strConfigJson：当前生效的配置原文
 * @param    {ConfigPage_E} enFeature：功能页
 * @return   {std::string} JSON 对象；该页无字段时返回 "{}"
 */
std::string readConfigPage(const std::string &strConfigJson, ConfigPage_E enFeature);

/**
 * @brief   : 把网页送来的字段合进当前配置（Set 用）
 * @param    {const std::string&} strConfigJson：当前生效的配置原文
 * @param    {ConfigPage_E} enFeature：功能页
 * @param    {const std::string&} strPartial：网页送来的 JSON（网页字段名）
 * @param    {std::string&} rstrMerged：OUT=合并后的完整配置 JSON
 * @param    {std::string&} rstrErr：OUT=失败原因（给调用方回显）
 * @return   {bool} true 成功
 */
bool mergeConfigPage(const std::string &strConfigJson, ConfigPage_E enFeature,
                    const std::string &strPartial, std::string &rstrMerged,
                    std::string &rstrErr);

/**
 * @brief   : 网页字段名 → 功能页 + 字段（供 Task 层解析 "feature" 字符串）
 * @return   {bool} false 表示名字不认识
 */
bool parseConfigPage(const char *pszName, ConfigPage_E &renFeature);


/**
 * @brief   : 功能页 → wire 名（ASCII 短名，接口层唯一口径）
 */
const char *configPageName(ConfigPage_E enFeature);

/**
 * @brief   : 功能页 → 中文显示名（只用于日志与调用方回显，不落 PII、不上 wire）
 */
const char *configPageTitle(ConfigPage_E enFeature);

} /* namespace Classroom */

#endif /* CAP_TV3881TJY_CLASSROOM_CONFIG */
