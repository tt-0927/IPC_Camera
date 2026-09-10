/*
 * @Author       : EasonLu
 * @Date         : 2025-02-13 17:32:26
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-18 17:04:34
 * @FilePath     : MANSCDP.h
 * @Description  : 国标协议的控制命令分类
 * @note         : MANSCDP:监控报警联网系统控制描述协议（Monitoring and Alarming Network System Control Description Protocol)
 * @note         : 可参考国标协议的标准文档
 */
#ifndef _MANSCDP_H_
#define _MANSCDP_H_
#include <string>
#include <vector>

enum manscdp_cmd_category_e
{
    MANSCDP_CMD_CATEGORY_CONTROL,
    MANSCDP_CMD_CATEGORY_QUERY,
    MANSCDP_CMD_CATEGORY_NOTIFY,
    MANSCDP_CMD_CATEGORY_RESPONSE,

    MANSCDP_CMD_CATEGORY_MAX,
    MANSCDP_CMD_CATEGORY_UNKNOWN = MANSCDP_CMD_CATEGORY_MAX
};

enum manscdp_cmdtype_e
{
    MANSCDP_NONE = 0, /* 默认值 */
    //< Control
    MANSCDP_CONTROL_CMD_DEVICE_CONTROL, ///< 设备控制
    MANSCDP_CONTROL_CMD_DEVICE_CONFIG,  ///< 设备配置

    //< Query
    MANSCDP_QUERY_CMD_DEVICE_STATUS,   ///< 设备控制
    MANSCDP_QUERY_CMD_CATALOG,         ///< 设备目录查询
    MANSCDP_QUERY_CMD_DEVICE_INFO,     ///< 设备信息查询
    MANSCDP_QUERY_CMD_RECORD_INFO,     ///< 录制文件检索
    MANSCDP_QUERY_CMD_ALARM,           ///< 报警查询
    MANSCDP_QUERY_CMD_CONFIG_DOWNLOAD, ///< 设备配置查询
    MANSCDP_QUERY_CMD_PRESET_QUERY,    ///< 预置位查询
    MANSCDP_QUERY_CMD_MOBILE_POSITION, ///< 移动设备位置数据查询
    MANSCDP_QUERY_CMD_CRUISETRACKQUERY, ///< 巡航轨迹查询
    MANSCDP_QUERY_CMD_CRUISETRACKLISTQUERY, ///< 巡航轨迹列表查询
    MANSCDP_QUERY_CMD_HOMEPOSITIONQUERY, ///< 看守位信息查询
    MANSCDP_QUERY_CMD_PTZPOSITION,      ///< PTZ精准状态查询

    //< Notify
    MANSCDP_NOTIFY_CMD_KEEPALIVE,       ///< 设备状态信息报送，保活
    MANSCDP_NOTIFY_CMD_ALARM,           ///< 报警通知
    MANSCDP_NOTIFY_CMD_MEDIA_STATUS,    ///< 媒体通知
    MANSCDP_NOTIFY_CMD_BROADCASE,       ///< 语音广播通知
    MANSCDP_NOTIFY_CMD_MOBILE_POSITION, ///< 移动设备位置通知
    MANSCDP_NOTIFY_CMD_UPLOADSNAPSHOTFINISHED, ///< 图像抓拍传输完成通知

    //< Response
    MANSCDP_RESOPNSE_CMD_DEVICE_CONTROL, ///< 设备控制响应
    MANSCDP_RESOPNSE_CMD_DEVICE_CONFIG,  ///< 设备配置响应
    MANSCDP_RESOPNSE_CMD_DEVICE_STATUS,  ///< 设备状态查询响应
    MANSCDP_RESOPNSE_CMD_DEVICE_CATALOG, ///< 设备目录查询响应

    MANSCDP_CMD_TYPE_MAX,
    MANSCDP_CMD_TYPE_UNKNOWN = MANSCDP_CMD_TYPE_MAX
};

enum manscdp_devicecontrol_subcmd_e
{
    PTZCmd = 1,
    TeleBoot,
    RecordCmd,
    GuardCmd,
    AlarmCmd = 5,
    IFrameCmd,
    DragZoomIn,
    DragZoomOut,
    HomePosition
};

enum manscdp_deviceconfig_subcmd_e
{
    BasicParam = 1,
    SVACEncodeConfig,
    SVACDecodeConfig
};

typedef std::vector<manscdp_devicecontrol_subcmd_e> manscdp_devicecontrol_subcmd_t;
typedef std::vector<manscdp_deviceconfig_subcmd_e> manscdp_deviceconfig_subcmd_t;
typedef std::vector<std::string> manscdp_configdownload_subcmd_t;

struct manscdp_msgbody_header_t
{
    manscdp_cmd_category_e cmd_category;
    manscdp_cmdtype_e cmd_type;
    std::string strSN;
    std::string strDevID;
    std::string strCmdType;
    /* 默认构造函数 */
    manscdp_msgbody_header_t()
        : cmd_category(MANSCDP_CMD_CATEGORY_UNKNOWN),
          cmd_type(MANSCDP_CMD_TYPE_UNKNOWN)
    {
    }
    /* 重载赋值运算符 */
    manscdp_msgbody_header_t &operator=(const manscdp_msgbody_header_t &rhs)
    {
        if (this != &rhs)
        {
            cmd_category = rhs.cmd_category;
            cmd_type = rhs.cmd_type;
            strSN = rhs.strSN;
            strDevID = rhs.strDevID;
            strCmdType = rhs.strCmdType;
        }
        return *this;
    }
};

#endif