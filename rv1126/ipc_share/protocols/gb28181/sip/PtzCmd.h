/***
 * @FilePath     : PtzCmd.h
 * @Author       : cyc
 * @Date         : 2024-09-29 09:52:16
 * @LastEditors  : cyc
 * @LastEditTime : 2024-10-25 17:30:52
 * @Description  : 云台控制命令类，提供对云台的操控指令和解析功能，解析逻辑详细见GB/T-28181-2022文档 107页指令格式
 */
#pragma once
#include "PtzType.h"
#include "SipType.h"
#include <memory>

/*******PTZ 指令码**************/
#define PTZ_RIGHT_CODE 0x01    /* 云台向右移指令码 */
#define PTZ_LEFT_CODE 0x02     /* 云台向左移指令码 */
#define PTZ_DOWN_CODE 0x04     /* 云台向下移指令码 */
#define PTZ_UP_CODE 0x08       /* 云台向上移指令码 */
#define PTZ_ZOOM_IN_CODE 0x10  /* 镜头放大指令码 */
#define PTZ_ZOOM_OUT_CODE 0x20 /* 镜头缩小指令码 */
/*******FI 指令码**************/
#define FI_FOCUS_FAR_CODE 0x41  /* 聚焦调远指令码 */
#define FI_FOCUS_NEAR_CODE 0x42 /* 聚焦调近指令码 */
#define FI_IRIS_IN_CODE 0x44    /* 光圈放大指令码 */
#define FI_IRIS_OUT_CODE 0x48   /* 光圈缩小指令码 */
/*******预置位指令码**************/
#define PRESET_SET_CODE 0x81  /* 预置位设置指令码 */
#define PRESET_CALL_CODE 0x82 /* 预置位调用指令码 */
#define PRESET_DEL_CODE 0x83  /* 预置位删除指令码 */
/*******巡航指令码**************/
#define PATROL_ADD_CODE 0x84       /* 加入巡航点 */
#define PATROL_DEL_CODE 0x85       /* 删除一个巡航点 */
#define PATROL_SET_SPEED_CODE 0x86 /* 设置巡航速度 */
#define PATROL_SET_TIME_CODE 0x87  /* 设置巡航停留时间 */
#define PATROL_START_CODE 0x88     /* 开始巡航 */

/******解析指令的宏**************/
#define PARSE_CMD_PTZ(x) (((x) >> 6) == 0x0)                 /* 解析到指令为ptz*/
#define PARSE_CMD_FI(x) (((x) >> 4) == 0x04)                 /* 解析到指令为FI*/
#define PARSE_CMD_SCNA(x) ((x) == 0x89 || (x) == 0x8a)       /* 解析到指令为scna */
#define PARSE_CMD_ZOOM_IN(x) ((((x) & 0x30) >> 4) == 0x01)   /* 解析到指令为ZOOM IN*/
#define PARSE_CMD_ZOOM_OUT(x) ((((x) & 0x30) >> 4) == 0x02)  /* 解析到指令为ZOOM OUT*/
#define PARSE_CMD_TILT_DOWN(x) ((((x) & 0x0c) >> 2) == 0x01) /* 解析到指令为云台向下移动*/
#define PARSE_CMD_TILT_UP(x) ((((x) & 0x0c) >> 2) == 0x02)   /* 解析到指令为云台向上移动*/
#define PARSE_CMD_PAN_RIGHT(x) (((x) & 0x03) == 0x01)        /* 解析到指令为云台向右移动*/
#define PARSE_CMD_PAN_LEFT(x) (((x) & 0x03) == 0x02)         /* 解析到指令为云台向左移动*/
#define PARSE_CMD_IRIS_IN(x) ((((x) & 0x0c) >> 2) == 0x01)   /* 解析到指令为光圈放大 */
#define PARSE_CMD_IRIS_OUT(x) ((((x) & 0x0c) >> 2) == 0x02)  /* 解析到指令为光圈缩小 */
#define PARSE_CMD_FOCUS_FAR(x) (((x) & 0x03) == 0x01)        /* 解析到指令为聚焦远 */
#define PARSE_CMD_FOCUS_NEAR(x) (((x) & 0x03) == 0x02)       /* 解析到指令为聚焦近 */

#define PARSE_CMD_SCNA_START(x, y) ((x) == 0x89 && (y) == 0x00) /* 解析到指令为扫描开始 */
#define PARSE_CMD_SCNA_LEFT(x, y) ((x) == 0x89 && (y) == 0x01)  /* 解析到指令为向左扫描 */
#define PARSE_CMD_SCNA_RIGHT(x, y) ((x) == 0x89 && (y) == 0x02) /* 解析到指令为向右扫描 */
#define PARSE_CMD_SCNA_SPEED(x) ((x) == 0x8a)                   /* 解析到指令为扫描速度 */

#define PARSE_CMD_ZOOM_SPEED(x) (((x) & 0xf0) >> 4) /* 取ZOOM speed的值 */

namespace SIP
{
    // @brief 云台控制类，负责生成和发送云台控制命令。
    class PtzCmd
    {
    public:
        typedef std::shared_ptr<PtzCmd> Ptr;

        /**
         * @brief 生成云台控制指令码。
         * @param [PtzCommand_E] enLeftRight 镜头左右移动指令
         * @param [PtzCommand_E] enUpDown 镜头上下移动指令
         * @param [PtzCommand_E] enInOut 镜头放大缩小指令
         * @param nMoveSpeed 镜头移动速度，默认值为 0xFF (范围0-255)。
         * @param nZoomSpeed 镜头缩放速度，默认值为 0x01 (范围0-255)。
         * @return 生成的控制指令字符串。
         */
        static std::string cmdString(PtzCommand_E enLeftRight, PtzCommand_E enUpDown, PtzCommand_E enInOut, int nMoveSpeed, int nZoomSpeed);

        /**
         * @brief 生成云台控制指令码。
         * @param nFourthByte 第四个字节。
         * @param nFifthByte 第五个字节。
         * @param nSixthByte 第六个字节。
         * @param nSeventhByte 第七个字节。
         * @return 生成的控制指令字符串。
         */
        static std::string cmdCode(int nFourthByte, int nFifthByte, int nSixthByte, int nSeventhByte);

        /**
         * @brief 计算镜头的缩放和焦距控制指令码。
         * @param [PtzCommand_E] enIris 镜头缩放控制
         * @param [PtzCommand_E] enFocus 镜头焦距控制
         * @param nIris_speed 镜头缩放速度，取值范围为 0-255。
         * @param nFocus_speed 镜头焦距速度，取值范围为 0-255。
         * @return 计算得到的指令码字符串。
         */
        static std::string cmdLens(PtzCommand_E enIris, PtzCommand_E enFocus, int nIris_speed, int nFocus_speed);
    };

    // @brief 云台控制指令解析类，负责解析接收到的云台控制指令。
    class PtzParser
    {
    public:
        PtzParser() = default;
        ~PtzParser() = default;

        /**
         * @brief 解析云台控制指令。
         * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
         * @param cmdstr 云台控制指令字符串。
         * @return int 解析结果，0 表示成功，其他值表示失败。
         */
        int ParseControlCmd(control_cmd_t &ctrlcmd, const std::string &cmdstr);

    private:
        /**
         * @brief 解析 PTZ 控制指令。
         * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
         */
        void parse_ptz(control_cmd_t &ctrlcmd);

        /**
         * @brief 解析与焦点相关的控制指令。
         * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
         */
        void parse_fi(control_cmd_t &ctrlcmd);

        /**
         * @brief 解析与预设点相关的控制指令。
         * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
         */
        void parse_preset(control_cmd_t &ctrlcmd);

        /**
         * @brief 解析巡逻功能相关的控制指令。
         * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
         */
        void parse_patrol(control_cmd_t &ctrlcmd);

        /**
         * @brief 解析扫描功能相关的控制指令。
         * @param ctrlcmd 用于存储解析结果的 control_cmd_t 对象。
         */
        void parse_scan(control_cmd_t &ctrlcmd);

    private:
        uint8_t chB4; /**< 第四个字节 */
        uint8_t chB5; /**< 第五个字节 */
        uint8_t chB6; /**< 第六个字节 */
        uint8_t chB7; /**< 第七个字节 */
    };
}
