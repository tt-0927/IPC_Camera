/*
 * @Author       : EasonLu
 * @Date         : 2025-02-16 21:52:10
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-05-07 21:15:02
 * @FilePath     : PtzType.h
 * @Description  : 云台类型
 */
#ifndef _PTZ_TYPE_H_
#define _PTZ_TYPE_H_
#include <stdint.h>
///< 云台 镜头变倍
struct ptz_cmd_zoom_t
{
    enum cmd_type_e
    {
        ZOOM_UNKNOWN = 0,
        ZOOM_OUT, /* 缩小 */
        ZOOM_IN,  /* 放大 */
    };
    cmd_type_e cmdtype = ZOOM_UNKNOWN;
    uint8_t speed = 0;
};

///< 云台垂直方向控制
struct ptz_cmd_tilt_t
{
    enum cmd_type_e
    {
        TILT_UNKNOWN = 0,
        TILT_UP,   /* 云台水平向上 */
        TILT_DOWN, /* 云台水平向下 */
    };
    cmd_type_e cmdtype = TILT_UNKNOWN;
    uint8_t speed = 0;
};

///< 云台水平控制方向
struct ptz_cmd_pan_t
{
    enum cmd_type_e
    {
        PAN_UNKNOWN = 0,
        PAN_LEFT,  /* 云台水平向左 */
        PAN_RIGHT, /* 云台水平向右 */
    };
    cmd_type_e cmdtype = PAN_UNKNOWN;
    uint8_t speed = 0;
};

///< FI指令 光圈
struct fi_cmd_iris_t
{
    enum cmd_type_e
    {
        IFIS_UNKNOWN = 0,
        IFIS_SHRINK,        ///< 缩小
        IFIS_AMPLIFICATION, ///< 放大
    };
    cmd_type_e cmdtype = IFIS_UNKNOWN;
    uint8_t speed = 0;
};

///< FI指令 聚焦 光圈
struct fi_cmd_focus_t
{
    enum cmd_type_e
    {
        FOCUS_UNKNOWN = 0,
        FOCUS_NEAR,
        FOCUS_FAR,
    };
    cmd_type_e cmdtype = FOCUS_UNKNOWN;
    uint8_t speed = 0;
};

///< 预置位指令
struct preset_cmd_t
{
    enum cmd_type_e
    {
        PRESET_UNKNOWN = 0,
        PRESET_SET,
        PRESET_CALL,
        PRESET_DELE
    };
    cmd_type_e cmdtype = PRESET_UNKNOWN;
    uint8_t index;
};

///< 巡航指令
struct patrol_cmd_t
{
    enum cmd_type_e
    {
        PATROL_UNKNOWN = 0,
        PATROL_ADD,
        PATROL_DELE,
        PATROL_SET_SPEED,
        PATROL_SET_TIME, ///< 设置停留时间
        PATROL_START,    ///< 开始巡航
        PATROL_STOP
    };
    cmd_type_e cmdtype = PATROL_UNKNOWN;
    uint8_t patrol_id; ///< 巡航组号
    uint8_t preset_id; ///< 预置位号
    uint16_t value;    ///< 数据，速度和停留时间使用
};

///< 自动扫描指令
struct scan_cmd_t
{
    enum cmd_type_e
    {
        SCAN_UNKNOWN = 0,
        SCAN_START,
        SCAN_STOP,
        SCAN_SET_LEFT_BOADER,
        SCAN_SET_RIGHT_BOADER,
        SCAN_SET_SPEED
    };
    cmd_type_e cmdtype = SCAN_UNKNOWN;
    uint8_t scan_id;
    uint16_t speed; ///< 设置scan速度使用
};

enum control_type_e
{
    CTRL_CMD_UNKNOWN = 0,
    PTZ_TYPE,    ///< PTZ控制
    FI_TYPE,     ///< 光圈、聚焦控制
    PRESET_TYPE, ///< 预置位
    PATROL_TYPE, ///< 巡航
    SCAN_TYPE,   ///< 扫描
    AUX_TYPE,    ///< 辅助开关

    CONTROL_STOP ///< 停止控制
};

struct control_cmd_t
{
    uint8_t first_byte; ///< A5H
    uint8_t version;    ///< 版本号
    uint8_t check;      ///< 校验位
    control_type_e ctrltype = CTRL_CMD_UNKNOWN;
    ptz_cmd_pan_t ptz_pan;
    ptz_cmd_tilt_t ptz_tilt;
    ptz_cmd_zoom_t ptz_zoom;
    fi_cmd_focus_t fi_focus;
    fi_cmd_iris_t fi_iris;
    preset_cmd_t preset;
    patrol_cmd_t patrol;
    scan_cmd_t autoscan;
};

/// @brief 云台操作枚举
typedef enum class PtzCommand : int
{
    LEFT,               // 向左移动
    RIGHT,              // 向右移动
    UP,                 // 向上移动
    DOWN,               // 向下移动
    UPLEFT,             // 左上移动
    UPRIGHT,            // 右上移动
    DOWNLEFT,           // 左下移动
    DOWNRIGHT,          // 右下移动
    ZOOMIN,             // 变焦 +
    ZOOMOUT,            // 变焦 -
    IFIS_SHRINK,        // 光圈缩小
    IFIS_AMPLIFICATION, // 光圈放大
    FOCUS_NEAR,         // 聚焦近
    FOCUS_FAR,          // 聚焦远
    STOP,               // 停止移动或变焦
    NONE
} PtzCommand_E;

#endif // _PTZ_TYPE_H_