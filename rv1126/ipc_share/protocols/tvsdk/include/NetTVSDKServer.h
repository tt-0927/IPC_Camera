#ifndef NETTVSDK_H
#define NETTVSDK_H

// 定义此宏以避免其他文件重复包含 NetTVSDKCommon.h
#define NETTVSDK_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STATIC
    #define STATIC                  static
#endif

#ifndef CONST
    #define CONST                   const
#endif

#ifndef EXTERN
    #define EXTERN                  extern
#endif

#ifndef INLINE
    #define INLINE                  __inline
#endif

#ifndef UNION
    #define UNION                   union
#endif

#ifndef IN
    #define IN
#endif

#ifndef OUT
    #define OUT
#endif

#ifndef INOUT
    #define INOUT
#endif

/* Keep the public SDK parameter annotations source-compatible. */
#ifndef NET_IN
    #define NET_IN IN
#endif

#ifndef NET_OUT
    #define NET_OUT OUT
#endif

#ifndef NET_INOUT
    #define NET_INOUT INOUT
#endif

#ifndef NEWINTERFACE
    #define NEWINTERFACE
#endif

/* Public SDK APIs must remain visible when the shared library hides internals. */
#if defined(_WIN32)
    #if defined(NET_SDK_SERVER_API) || defined(NET_SDK_CLIENT_API)
        #define NET_API __declspec(dllexport)
    #else
        #define NET_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define NET_API __attribute__((visibility("default")))
#else
    #define NET_API
#endif

#ifdef i386
    #ifdef LINUX
        #ifndef STDCALL
        #define STDCALL                 __attribute__((stdcall))__attribute__((visibility ("default")))
        #endif
    #else
        #ifndef STDCALL
        #define STDCALL                 __attribute__((stdcall))
        #endif
    #endif
#else
    #ifdef _WIN32
        #ifndef STDCALL
        #define STDCALL                 __stdcall
        #endif
    #else
        #ifdef LINUX
            #ifndef STDCALL
            #define STDCALL
            #endif
        #else
            #ifndef STDCALL
            #define STDCALL
            #endif
        #endif
    #endif
#endif

#ifndef NET_STDCALL
    #define NET_STDCALL            STDCALL
#endif


#ifndef UCHAR_DEF
#define UCHAR_DEF
    typedef unsigned char           UCHAR;
#endif

#ifndef CHAR_DEF
#define CHAR_DEF
    typedef char                    CHAR;
#endif

#ifndef BYTE_DEF
#define BYTE_DEF
    typedef unsigned char           BYTE;
#endif

#ifndef UINT16_DEF
#define UINT16_DEF
    typedef unsigned short          UINT16;
#endif

#ifndef UINT_DEF
#define UINT_DEF
    typedef unsigned int            UINT32;
#endif

#ifndef INT16_DEF
#define INT16_DEF
    typedef  short                  INT16;
#endif

#ifndef INT32_DEF
#define INT32_DEF
    typedef  int                    INT32;
#endif

#ifndef LPVOID_DEF
#define LPVOID_DEF
    typedef void*                   LPVOID;
#endif

#ifndef VOID
#ifndef VOID_DEF
#define VOID_DEF
    typedef void                    VOID;
#endif
#endif

#ifndef INT64_DEF
#define INT64_DEF
    typedef long long               INT64;
#endif

#ifndef UINT64_DEF
#define UINT64_DEF
    typedef unsigned long long      UINT64;
#endif

#ifndef BOOL_DEF
#define BOOL_DEF
    #ifndef __OBJC__
        typedef int                 BOOL;
    #else
        #import<objc/objc.h>
    #endif
#endif

#ifndef FLOAT
#ifndef VOID_FLOAT
#define VOID_FLOAT
    typedef float                   FLOAT;
#endif
#endif

#ifndef DOUBLE
#ifndef DOUBLE_DEF
#define DOUBLE_DEF
    typedef double                   DOUBLE;
#endif
#endif

#ifndef FALSE
    #define FALSE                   0
#endif

#ifndef TRUE
    #define TRUE                    1
#endif

#ifndef NET_FALSE
    #define NET_FALSE               FALSE
#endif

#ifndef NET_TRUE
    #define NET_TRUE                TRUE
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
    typedef LPVOID                 HWND;
#endif

/************************************************************************/
/*                     模块宏定义                                       */
/************************************************************************/
#ifndef NET_SDK_NO_MEDIA
#define NET_WITH_MEDIA              1           /* 包含媒体模块 */
#endif

#ifndef NET_SDK_NO_CLOUD
#define NET_WITH_CLOUD              1           /* 包含云服务模块 */
#endif
#define NET_WITH_XW                 1           /* 包含电视墙、拼接模块 */
#define NET_WITH_VMS                1           /* 包含VMS模块 */
#define NET_WITH_PTZ                1           /* 包含云台模块 */
#define NET_WITH_SMART              1           /* 包含智能业务模块 */
#define NET_WITH_CONFIG             1           /* 包含配置业务模块 */
#define NET_WITH_BASIC              1           /* 包含基础管理业务模块 */
#define NET_ALARM_RECOVER_BASE      1           /* 告警恢复基数 Alarm recover base */
#define NET_WITH_TRANS_CHANEL       1           /* 包含透明通道模块 */

/********************************** 常用数值宏  Commonly used numerical macros *************** */
#define NET_STREAM_ID_LEN                       32          /* 流ID标识长度  Length of stream ID*/
#define NET_FILE_NAME_LEN                       (256u)      /* 文件名长度  Length of filename */
#define NET_USERNAME_LEN                        (128 + 4)   /* Maximum length of username */
#define NET_PASSWORD_LEN                        128         /* Maximum length of password */
#define NET_DESCRIBE_MAX_LEN                    (512 + 4)   /* 描述最大长度 ：128 * 4，任意字符达到长度128 */
#define NET_DOMAIN_LEN                          64          /* 域名最大长度  Maximum length of domain name */
#define NET_PATH_LEN                            128         /* 路径最大长度:包括文件名称  Maximum length of path, including filename */
#define NET_MAX_URL_LEN                         512         /* URL 的最大长度  Maximum length of URL */
#define NET_INVALID_CHANNEL_ID                  (0XFFFFFF)  /* 无效的通道ID */
#define NET_INVALID_ID                          0            /* 无效ID */

/* 通用长度  Common length */
#define NET_LEN_2                               2
#define NET_LEN_4                               4
#define NET_LEN_6                               6
#define NET_LEN_8                               8
#define NET_LEN_16                              16
#define NET_LEN_18                              18
#define NET_LEN_32                              32
#define NET_LEN_40                              40
#define NET_LEN_64                              64
#define NET_LEN_128                             128
#define NET_LEN_132                             132
#define NET_LEN_256                             256
#define NET_LEN_260                             260
#define NET_LEN_480                             480
#define NET_LEN_512                             512
#define NET_LEN_1024                            1024
#define NET_LEN_2000                            2000
#define NET_LEN_4096                            4096
#define NET_LEN_8192                            8192

#define NET_IPADDR_STR_MAX_LEN                  (64u)        /* IP 地址信息字符串长度  Length of IP address string */
#define NET_IPV4_LEN_MAX                        16          /* IPV4地址字符串长度 Length of IPV4 address string */
#define NET_IPV6_LEN_MAX                        128         /* IPV6地址字符串长度 Length of IPV6 address string */
#define NET_NAME_MAX_LEN                        (256u)       /* 通用名称字符串长度  Length of common name string */

#define NET_CODE_STR_MAX_LEN                    (256u)       /* 通用CODE 长度  Length of common code */
#define NET_MAX_DATE_STRING_LEN                 (64u)        /* 最大日期字符长度 Maximum length of date string "2008-10-02 09:25:33.001 GMT" */
#define NET_MAX_ALARM_IN_NUM                    64          /* 告警输入最大数量  Maximum number of alarm inputs */
#define NET_MAX_ALARM_OUT_NUM                   64          /* 告警输出最大数量  Maximum number of alarm outputs */
#define NET_PLAN_SECTION_NUM                    8           /* 一天中的计划时间段  Number of scheduled time sections in a day */
#define NET_PLAN_NUM_AWEEK                      8           /* 一周总共可配置的计划个数,包括周一至周日和假日  Total number of plans allowed in a week, including Monday to Sunday, and holidays */

#define NET_MAX_PRESET_NUM                      256         /* 预置位最大数  Maximum number of presets */
#define NET_MAX_CRUISEPOINT_NUM                 32          /* 巡航路径中预置位点最大个数  Maximum number of presets for preset patrol */
#define NET_MAX_CRUISEROUTE_NUM                 16          /* 预置位巡航路径最大条数  Maximum number of routes for preset patrol */
#define NET_MIN_PTZ_SPEED_LEVEL                 1           /* 云台移动最小速度  Maximum PTZ rotating speed */
#define NET_MAX_PTZ_SPEED_LEVEL                 9           /* 云台移动最大速度  MinimumPTZ rotating speed */
#define NET_MAX_VIDEO_EFFECT_VALUE              255         /* 图像参数（亮度 对比度 色度 饱和度）最大值  Maximum values for image parameters (brightness, contrast, hue, saturation) */
#define NET_MIN_VIDEO_EFFECT_VALUE              0           /* 图像参数（亮度 对比度 色度 饱和度）最小值  Minimum values for image parameters (brightness, contrast, hue, saturation) */
#define NET_MAX_VIDEO_EFFECT_GAMMA_VALUE        10          /* 图像参数（伽马值）最大值 Minimum values for image parameters (Gama) */

#define NET_MAX_PRIVACY_MASK_AREA_NUM           8           /* 最大可配置遮盖区域个数  Maximum number of privacy mask areas allowed */
#define NET_OSD_TEXTOVERLAY_NUM                 6           /* 通道 OSD 字符叠加数量  Number of OSD text overlays */
#define NET_OSD_TEXT_MAX_LEN                    (64 + 4)    /* 通道 OSD 字符长度  Length of OSD texts */
#define NET_OSD_TEXT_MAX_LEN_EX                 (512 + 4)   /* 通道 OSD 字符长度(扩展)  Length of OSD texts */
#define NET_OSD_TYPE_MAX_NUM                    32          /* 通道 OSD 最大类型个数  Maximum number of OSD type */
#define NET_OSD_CUSTOM_MAX_NUM                  4           /* 通道 OSD 自定义字符叠加最大个数  Maximum number of custom OSD texts */
#define NET_OSD_FONT_SIZE_TYPE_MAX_NUM          4           /* 通道 OSD 字体最大类型个数  Maximum number of OSD font size type */
#define NET_OSD_FONT_STYLE_TYPE_MAX_NUM         4           /* 通道 OSD 样式最大类型个数  Maximum number of OSD font style type */
#define NET_OSD_TIME_FORMAT_MAX_NUM             7           /* 通道 OSD 最大时间格式个数  Maximum number of OSD time format type */
#define NET_OSD_DATE_FORMAT_MAX_NUM             15          /* 通道 OSD 最大日期格式个数  Maximum number of OSD date format type */
#define NET_PULL_ALARM_MAX_NUM                  8           /* 拉告警最大告警个数  Maximum number of alarms a user can get */
#define NET_TRACK_CRUISE_MAXNUM                 1           /* 支持的轨迹巡航的最大条数  Maximum number of patrol routes allowed  */
#define NET_AUDIO_INPUT_TYPE_MAX                2           /* 最大输入类型数量 */
#define NET_AUDIO_OUTPUT_TYPE_MAX               3           /* 最大输出类型数量 */
#define NET_AUDIO_FORMAT_MAX                    8           /* 最大音频格式数量 */
#define NET_AUDIO_SAMPRATE_MAX                  11          /* 最大采样率数量 */
#define NET_AUDIO_BITRATE_MAX                   7           /* 最大码率数量 */
#define NET_AUDIO_SOUND_MIN_VALUE               0           /* 音量调节最小值  Minimum volume */
#define NET_AUDIO_SOUND_MAX_VALUE               255         /* 音量调节最大值  Maximum volume */
#define NET_MIC_SOUND_MIN_VALUE                 0           /* 麦克风音量调节最小值  Minimum volume */
#define NET_MIC_SOUND_MAX_VALUE                 255         /* 麦克风音量调节最大值  Maximum volume */
#define NET_SCREEN_INFO_ROW                     18          /* 屏幕信息行数  Screen Info Row */
#define NET_SCREEN_INFO_COLUMN                  22          /* 屏幕信息列数  Screen Info Column */
#define NET_CHANNEL_MAX                         512         /* 最大通道数 Maximum number of channel */
#define NET_RESOLUTION_NUM_MAX                  32          /* 分辨率总个数 Maximum number of resolution */
#define NET_MONTH_DAY_MAX                       31          /* 每月天数最大值 Maximum number of days in a month */
#define NET_VIDEO_ENCODE_TYPE_MAX               16          /* 编码格式类型总个数 Maximum number of encode type */
#define NET_PEOPLE_CNT_MAX_NUM                  60          /* 客流量统计数组最大值（分报表） Maximum number of people count */
#define NET_WIFISNIFFER_MAC_MAX_NUM             64          /* wifi sniffer MAC地址最大长度  Length of wifi sniffer MAC */
#define NET_WIFISNIFFER_MAC_ARRY_MAX_NUM        128         /* wifi sniffer MAC地址数组最大值 Maximum number of wifi sniffer MAC array */
#define NET_HOTSPOT_CONN_MAX_NUM                128         /* 热点连接设备最大数量 Maximum number of hotspot connected devices */
#define NET_DISK_MAX_NUM                        256         /* 磁盘最大数量 Maximum number of Disk */
#define NET_LOCAL_DISK_MAX_NUM                  32          /* 本地磁盘最大数量 local Maximum number of Disk */
#define NET_SD_CARD_DISK_MAX_NUM                16          /* SD卡最大数量 SD Maximum number of Disk */
#define NET_ARRAY_MAX_NUM                       16          /* 阵列最大数量 array Maximum number of Disk */
#define NET_EXTEND_CABINET_DISK_MAX_NUM         32          /* 扩展柜硬盘最大数量 extend cabinet Maximum number of Disk */
#define NET_NAS_MAX_NUM                         16          /* NAS最大数量 NAS Maximum number of Disk */
#define NET_ESATA_MAX_NUM                       4           /* ESATA最大数量 eSATA Maximum number of Disk */
#define NET_DISK_SMART_MAX_NUM                  128         /* 硬盘SMART信息最大数量 Maximum number of Disk Smart Info */
#define NET_ENCODE_FORMAT_MAX_NUM               3           /* 最大视频编码格式数 Maximum number of video compression */
#define NET_SMART_ENCODE_MODEL_MAX_NUM          3           /* 最大智能图像扩展编码模式数 Maximum number of smart image encoding mode */
#define NET_GOP_TYPE_MAX_NUM                    4           /* 最大GOP类型数量 Maximum number of GOP type */
#define NET_IPSAN_MAX_NUM                       4           /* IPSAN最大数量 IPSAN Maximum number of Disk */

#define NET_PHOTO_SERVER_MAX_NUM                4           /* 照片服务器数量上限 Maximum number of Photo Server */

#define NET_INTELLIGENT_SERVER_MAX_NUM          4           /* 智能服务器数量上限 Maximum number of Intelligent Server */

#define NET_MANAGER_SERVER_MAX_NUM              4           /* 管理服务器数量上限 Maximum number of Manager Server */

#define NET_DEV_OTHER_LEN_MAX                   32          /* 其他字段 */
#define NET_DEV_NAME_LEN_MAX                    64          /* 设备名称长度 */

#define NET_DEV_PASSWORD_LEN_MAX                64          /* 设备密码长度 */
#define NET_CLOUD_DEV_USER_NAME_LEN             260         /* 云端设备ID长度 */
#define NET_CLOUD_USER_NAME_LEN                 260         /* 云端用户名长度 */
#define NET_CLOUD_DEV_USER_AUTH_LEN             260         /* 云端设备用户权限名称长度 */
#define NET_CLOUD_SHARE_TARGET_NAME_LEN         64          /* 云端设备共享对象名称长度 */
#define NET_CLOUD_SHARE_DESCRIBE_LEN            260         /* 云端设备共享描述长度 */
#define NET_CLOUD_DEV_NAME_LEN                  260         /* 云端设备名称长度 */
#define NET_XW_MAX_PANE_NUM                     64          /* 窗口最大分屏数量 */
#define NET_NTP_SERVER_LIST_NUM                 5           /* NTP服务列表数量 */

#define NET_TMS_FACE_RECORD_ID_LEN              32          /* 记录ID缓存长度 */
#define NET_TMS_CAMER_ID_LEN                    32          /* 相机ID缓存长度 */
#define NET_TMS_PASSTIME_LEN                    32          /* 通过时间字符串缓存长度 */
#define NET_TMS_FACE_TOLLGATE_ID_LEN            32          /* 卡口编号缓存长度 */
#define NET_TMS_HEAT_MAP_DEVID_LEN              32          /* 热度图DevID字段长度 */
#define NET_TMS_HEAT_MAP_RECORD_ID_LEN          16          /* 热度图RecordID字段长度 */
#define NET_TMS_HEAT_MAP_COllECT_TIME_LEN       18          /* 热度图CollectTime 字段长度 */
#define NET_TMS_PIC_COMMON_NUM                  10          /* 图片或区域上限个数 */
#define NET_TMS_CAR_PLATE_CAMID_LEN             32          /* 车牌识别CamID字段长度 */
#define NET_TMS_CAR_PLATE_RECORDID_LEN          32          /* 车牌识别RecordID字段长度 */
#define NET_TMS_CAR_PLATE_TOLLGATE_LEN          32          /* 车牌识别TollgateID字段长度 */
#define NET_TMS_CAR_PLATE_PASSTIME_LEN          18          /* 车牌识别PassTime字段长度 */
#define NET_TMS_CAR_PLATE_LANEID_LEN            18          /* 车牌识别LaneID字段长度 */
#define NET_TMS_CAR_PLATE_CARPLATE_LEN          32          /* 车牌识别CarPlate字段长度 */
#define NET_USER_NAME_ENCRYPT_LEN               256         /* 加密后的用户名长度 */
#define NET_PASSWORD_ENCRYPT_LEN                256         /* 加密后的密码长度 */
#define NET_VIDEO_FORMAT_MAX                    32          /* 支持的视频输出制式最大数量 */
#define NET_VIDEO_FORMAT_NAME_LEN               32          /* 支持的视频输出制式名称长度 */
#define NET_TVWALL_NAME_LEN                     260         /* 电视墙名称长度 */
#define NET_FORMAT_SPEC_MAX                     256         /* 特殊输出制式的最大个数量 */
#define NET_LED_SPEC_MAX                        256         /* 特殊模组框的最大个数量 */
#define NET_FORMAT_NAME_LEN                     32          /* 输出制式名称长度 */
#define NET_VIDEO_OUT_MAX                       64          /* 物理输出端口的最大个数量 */
#define NET_SCENE_NAME_LEN                      260         /* 场景名称长度 */
#define NET_TIME_LEN                            16          /* 时间字符串长度 */
#define NET_WND_NAME_LEN                        260         /* 窗口名称长度 */
#define NET_SEQUENCE_SRC_MAX                    128         /* 轮巡时视频源最大个数 */
#define NET_TEXT_LEN                            1024        /* 虚拟LED文字内容长度 */
#define NET_BMAP_NAME_LEN                       256         /* 底图名称长度 */
#define NET_SEQ_RES_WIN_MAX                     288         /* 轮巡资源中窗口的最大个数 */
#define NET_MAX_DAY_NUM                         8           /* 最大天数 */
#define NET_MAX_TIME_SECTION_NUM                8           /* 时间段数量 */

#define NET_ALARM_SOURCE_MAX_LEN                  (64 + 4)    /* 告警资源字符描述长度 */
#define NET_MAX_EVENT_RES_SIZE                    1024        /* 事件上报最大资源数 */

#define NET_VIID_CODE_LEN                         48          /* 视图编码长度 */
#define NET_VIDEO_FORMAT_CAP_NUM                  64          /* 编码制式能力集 */
#define NET_VIDEO_FRAME_RATE_MAX_NUM              64          /* 视频能力集支持帧率最大数量 */
#define NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM       3           /* 编码复杂度最大数量 */
#define NET_LAYOUT_CAP_NUM                        64          /* 分屏能力集 */

#define NET_DA_POINT_CODE_LEN                     48          /* 代理设备 点位 编码长度 */
#define NET_DA_AREA_CODE_LEN                      48          /* 代理设备 区域 编码长度 */
#define NET_VIRTUAL_MEM_TABLE_MAX                 32          /* 虚拟内存表元素最大数量 */
#define NET_EVENT_STORE_TYPE_NUM                    128         /* 时间存储类型数量 */
#define NET_MAX_PANE_NUM                            36          /* DC业务分屏数量最大为36分屏 */
#define NET_OSD_MAX_NUM_EX                          8           /* 通道 OSD 最大数量  Maximum Number of OSD */
#define NET_RSA_MAX_VALUE                           3           /* 表示最多尝试密钥生成次数 */

#define NET_MAX_VIDEO_BRIGHT_EFFECT_VALUE           199             /* 图像参数（亮度）最大值 */
#define NET_MAX_VIDEO_CONTRAST_EFFECT_VALUE         199             /* 图像参数（对比度）最大值 */
#define NET_MAX_VIDEO_SATURATION_EFFECT_VALUE       359             /* 图像参数（饱和度）最大值 */
#define NET_MAX_VIDEO_HUE_EFFECT_VALUE              359             /* 图像参数（色度）最大值 */
#define NET_MAX_VIDEO_GAMMA_EFFECT_VALUE            99              /* 图像参数（伽玛）最大值 */
#define NET_PIXEL_CONVERT_RATIO                     5000            /* 像素转换比例 用于区域设置,如拉框放大区域 */
#define NET_PANES_NUM                               16              /* 单通道最大分屏数 */

#define NET_XW_AUDIO_NUM                            16              /* 音频输出通道最大个数 */
#define NET_IP_ADDRESS_LEN                          64              /* IP地址长度 */

#define NET_TVWALLPLAN_NUM                          4               /* 电视墙预案个数 */
#define NET_PLAN_MAX_TVWALL_NUM                     4               /* 预案下电视墙最大数量 */
#define NET_TVWALL_MAX_WIN_NUM                      81              /* 电视墙最大的窗口数量 */
#define NET_TVWALL_MAX_LAYOUT_NUM                   64              /* 电视墙最大的分屏数量,即为子窗口数量 */
#define NET_ALARM_LINK_PRESET_NUM                   16              /* 告警预案联动预置位数量  */
#define NET_ALARM_LINK_SWITCHOUT_NUM                16              /* 告警预案联动告警输出通道数量 */
#define NET_ALARM_LINK_MONITOR_NUM                  16              /* 告警预案联动实况数量 */
#define NET_ALARM_LINK_TVWALL_NUM                   32              /* 告警预案联动电视墙最大数量*/
#define NET_ALARM_LINK_SOUND_LEN                    512             /* 告警预案联动声音信息最大长度 */
#define NET_ALARM_SOURCE_NUM                        1               /* 告警源数量 */
#define NET_ALARM_LINK_NUM                          128             /* 告警预案数量 */

#define NET_TIME_TEMPLATE_NUM                       32              /* 时间模板数量 */
#define NET_DC_SCHEME_RES_CHN_MAX_NUM               256             /* DC轮巡最大资源数 */
#define NET_VIEW_MAX_WIN_NUM                        100             /* 视图最大的窗口数量 */
#define NET_MAX_ROLE_RIGHT_SIZE                     256             /* 用户权限菜单项数量 */
#define NET_MAX_QUERY_CHANNEL_NUM                   500             /* 单次查询最大通道个数 */
#define NET_MAX_QUERY_DEV_NUM                       500             /* 单次查询最大设备个数 */
#define NET_GRID_AREAS_LEN                          256             /* 宏块值数组长度 */
#define NET_MAX_ORG_ROOT_ID_NUM                     32              /* 组织树根节点最大个数 */
#define NET_VOICE_BROADCAST_CHANNEL_NUM_MAX         128             /* 一个语音广播组支持最大通道个数 */
#define NET_RECORD_LOCK_ID_LEN                      64              /* 录像锁定ID最大长度 */
#define NET_RECORD_LOCK_DESC_LEN                    64              /* 锁定录像段的描述最大长度 */
#define NET_REPLAY_SESSION_ID_LEN                   64              /* 回放会话ID最大长度 */
#define NET_REPLAY_RECORD_SEGMENT_MAX               128             /* 单次回放录像查询最多返回的时间段数量 */
#define NET_NOTIME                                  0               /* 时间无值 */
#define NET_WHITE_BALANCE_MODE_MAX_NUM              16              /* 最多支持的白平衡模式个数 Maximum number of Image white balance mode count */
#define NET_FOCUS_MODE_MAX_NUM                      16              /* 最多支持的对焦模式个数 Maximum number of Image focus mode count */
#define NET_FOCUS_SCENE_MAX_NUM                     16              /* 最多支持的对焦场景个数 Maximum number of Image focus scene count */
#define NET_IMAGE_ROTATION_MODE_MAX_NUM             16              /* 最多支持的图像镜像模式个数 Maximum number of Image rotation mode count */
#define NET_LAMP_CTRL_TYPE_MAX_NUM                  16              /* 最多支持的支持的补光灯类型个数 Maximum number of lamp ctrl type count */
#define NET_LAMP_CTRL_MODE_MAX_NUM                  16              /* 最多支持的补光灯空控制模式个数 Maximum number of lamp ctrl mode count */
#define NET_EXPOSURE_MODE_MAX_NUM                   16              /* 最多支持的曝光模式个数 Maximum number of exposure mode count */
#define NET_IRIS_RANGE_MAX_NUM                      16              /* 最多支持的光圈取值个数  Maximum number of Iris Range count */
#define NET_METERING_MODE_MAX_NUM                   16              /* 最多支持的测光控制模式个数  Maximum number of Metering mode count */
#define NET_SHUTTER_TIME_RANGE_MAX_NUM              28              /* 最多支持的快门时间的取值的个数  Maximum number of shutter Time Range count */
#define NET_SLOW_SHUTTER_TIME_RANGE_MAX_NUM         16              /* 最多支持的慢快门时间的取值的个数 Maximum number of slow shutter Time Range count */
#define NET_WIDE_DYNAMIC_MODE_MAX_NUM               16              /* 最多支持的宽动态模式个数 Maximum number of wide dynamic mode count */
#define NET_DAY_NIGHT_MODE_MAX_NUM                  16              /* 最多支持的昼夜模式类型个数 Maximum number of slow Day Night Mode count */
#define NET_AUDIO_IN_MAX_NUM                        16              /* 最多支持的音频口输入个数 Maximum number of Audio input count */
#define NET_AUDIO_IN_CHL_MODE_MAX_NUM               8               /* 最多支持的音频输入通道模式个数 Maximum number of Audio input mode count */
#define NET_AUDIO_IN_ENCODE_FORMAT_MAX_NUM          16              /* 最多支持的音频输入编码格式个数 Maximum number of Audio input encode format count */
#define NET_AUDIO_SAMPLING_RATE_MAX_NUM             8               /* 最多支持的音频采样率个数 Maximum number of Audio sampling rate count */
#define NET_SERIAL_IN_MAX_NUM                       16              /* 最多支持的串口输入个数 Maximum number of serial input count */
#define NET_SERIAL_IN_ENCODE_FORMAT_MAX_NUM         16              /* 最多支持的串口输入编码格式个数 Maximum number of serial input encode format count */
#define NET_FACE_FEATURE_SIZE                       512             /* 人脸特征信息 512B */
#define NET_FACE_FEATURE_VERSION_LEN                40              /* 人脸特征模型版本号最大长度 */
#define NET_FACE_FEATURE_LIST_FILE_LEN              256             /* 人脸特征库文件名最大长度 */
#define NET_FACE_FEATURE_FILE_MD5_LEN               16              /* 人脸特征库文件的MD5值长度 */
#define NET_FACE_FEATURE_GALLEY_ID_LEN              20              /* 人脸半结构化特征名单库ID长度 */
#define NET_FACE_FEATURE_MAX_NUM                    3               /* 人脸半结构化特征最大数目 */
#define NET_OBJ_TRACK_MODE_NUM                      8               /* 设备支持的智能跟踪模式数量 */
#define NET_STREAM_MAX_NUM                          3               /* 最大支持的码流数量 */
#define NET_PLAN_DAY_NUM_AWEEK                      7               /* 一周总共可配置的计划天数，包含周一到周日 */
#define NET_PLAN_TIME_SECTION_NUM_ADAY              4               /* 一天可配置的时间段数 Total number of plans allowed in a day*/
#define NET_XW_SERIAL_NUM                           16              /* 串口数量 */
#define NET_DNS_LIST_NUM                            2               /* DNS列表数量 */
#define NET_NETWORK_MACNAME_LEN                     48              /* MAC地址名称长度 */
#define NET_LOG_QUERY_COND_NUM                      48              /* 日志查询条件数量 */
#define NET_RECORD_FILE_MAX_NUM                     48              /* 录像查询结果最大数量 */
#define NET_RECORD_DATE_MAX_NUM                     64              /* 录像日期结果最大数量 */
#define NET_RECORD_DOWNLOAD_MAX_NUM                 16              /* 录像下载任务最大数量 */
#define NET_FACE_DB_NAME_LEN                        256             /* 人脸库名称长度最大值 */
#define NET_FACE_MEMBER_NAME_LEN                    256             /* 人脸库成员名称长度最大值 */
#define NET_FACE_MEMBER_REGION_LEN                  256             /* 人脸库成员所在地区名称最大值 */
#define NET_FACE_MEMBER_CUSTOM_NUM                  5               /* 自定义属性列表个数 */
#define NET_FACE_MEMBER_CUSTOM_LEN                  255             /* 自定义属性值长度 */
#define NET_FACE_IMAGE_MAX_LEN                      (2*1024*1024)   /* 人脸图片数据的最大长度，2M   2097152字节*/
#define NET_FACE_DB_TITLE_NAME_LEN                  508             /* 人脸库自定义属性名称最大长度 */
#define NET_FACE_MONITOR_RULE_NAME_LEN              508             /* 人脸布控任务的布控名称最大值 */
#define NET_FACE_MONITOR_RULE_REASON_LEN            508             /* 人脸布控的布控原因最大值 */
#define NET_FACE_ALARM_SRC_LEN                      256             /* 抓拍通道名称长度 */
#define NET_FACE_ANALYSIS_SKILL_NUM                 16              /* 设备支持的人脸分析能力数量 */
#define NET_FACE_MEMBER_BIRTHDAY_LEN                31              /* 成员出生日期字符串最大长度 */
#define NET_FACE_IDNUMBER_LEN                       128             /* 证件号最大范围*/
#define NET_FACE_LIB_MAX_NUM                        64              /* 目标库最大数量 */
#define NET_FACE_INFO_MAX_NUM                       128             /* 人脸信息最大数量 */
#define NET_FACE_ID_MAX_NUM                         128             /* 人脸ID最大数量 */
#define NET_LABEL_ID_MAX_LEN                        32
#define NET_TIME_RANGE_NUM                          8               /* 时间模板时间范围个数(周一到周日再加假日) */
#define NET_TIME_DURATION_NUM                       8               /* 时间模板中一天最多8个片段 */
#define NET_HOLIDAY_INFO_NUM                        32              /* 假日配置数量 */
#define NET_AUDIO_MAX_NUM                           18              /* 音频输出业务数量 */
#define NET_CREATE_CONNECT_NUMBER                   1               /* 创建连接数量 */
#define NET_EMERGENCY_BRLL_NAME_LEN                 128             /* 紧急铃名称最大长度*/
#define NET_EMERGENCY_BRLL_MAX_NUM                  120             /* 紧急铃信息最大数量*/
#define NET_VEHICLE_COMP_IMAGE_MAX_LEN              2097152         /* 车辆布控比对图片的最大长度 2M*/
#define NET_VEHICLE_IMAGE_MAX_LEN                   4194304         /* 车辆图片数据最大字节数 4M */
#define NET_PIC_DATA_MAX_LEN                        (1024*1024)     /* 图片数据信息加密后最大大小 */

#define NET_RES_CHANGE_INFO_LIST_NUM                64              /* 定义LAPI事件上报信息结构体 */

#define NET_OUTPUT_NI_RECV_CARD_MAX_NUM             64              /* 输出网口下的接收卡最大数量 */
#define NET_IMG_IN_MODE_LIST_MAX_NUM                16              /* 图像输入模式最大数量 */
#define NET_GAMMA_INFO_LIST_MAX_NUM                 1024            /* 伽马表最大数量值 */

#define NET_MAX_SERIAL_PROT_NUM                     1               /* 设备当前只有一个485串口 */
#define NET_MAX_TRANS_CHANEL_NUM                    1               /* 一个485串口目前只支持一个透明通道 */

#define NET_MAX_NIC_WORK_MODE_NUM                   8               /* 最大网卡支持的工作模式数量 */
#define NET_MAX_PORT_WORK_MODE_NUM                  24              /* 最大网口工作模式数量 */
#define NET_MAX_NET_WORK_CARD_NUM                   8               /* 最大网卡数量 */
#define NET_MAX_LINK_ACTION_NUM                     9               /* 最大联动动作数量 */

#define NET_IVA_REPORT_COORD_NUM                    16              /* 一条规则的最大坐标点数 */

#define NET_MAX_SCENE_INFO_NUM                      5               /* 最大场景信息数量 */
#define NET_MAX_TRIGGER_DETAIL_INFO_NUM             4               /* 最大场景自动切换触发条件数量 */
#define NET_MAX_ENV_PARAM_NUM                       2               /* 最大环境参数数量 */
#define NET_MAX_SCENE_TYPE_NUM                      16              /* 最大支持的场景类型数量 */
#define NET_MAX_ENV_TYPE_NUM                        2               /* 最大支持的环境类型数量 */

/* 告警周布防时间表包含的天数。 */
#define NET_ALARM_SCHEDULE_DAY_COUNT            7
/* 告警周布防时间表中的最小小时值。 */
#define NET_ALARM_SCHEDULE_HOUR_MIN             0
/* 告警周布防时间表中的最大小时值。 */
#define NET_ALARM_SCHEDULE_HOUR_MAX             23
/* 告警周布防时间表中的最小分钟值。 */
#define NET_ALARM_SCHEDULE_MINUTE_MIN           0
/* 告警周布防时间表中的最大分钟值。 */
#define NET_ALARM_SCHEDULE_MINUTE_MAX           59
/* 报警输入处理方式：禁用报警输入。 */
#define NET_ALARM_INPUT_DEAL_TYPE_DISABLED      0
/* 报警输入处理方式：启用报警输入。 */
#define NET_ALARM_INPUT_DEAL_TYPE_ENABLED       1
/* 声音报警重复播放的最小次数。 */
#define NET_AUDIBLE_ALARM_PLAY_TIMES_MIN        1
/* 声音报警重复播放的最大次数。 */
#define NET_AUDIBLE_ALARM_PLAY_TIMES_MAX        50
/* 闪光报警灯持续时间的最小值，单位为秒。 */
#define NET_FLASHING_LIGHT_ALARM_TIME_MIN       1
/* 闪光报警灯持续时间的最大值，单位为秒。 */
#define NET_FLASHING_LIGHT_ALARM_TIME_MAX       300
/* 声音报警可配置的自定义音频最大数量。 */
#define NET_AUDIBLE_ALARM_CUSTOM_AUDIO_MAX_NUM 3
/* 报警输入或输出地址字符串的最大长度。 */
#define NET_ALARM_ADDRESS_LEN                  64
/* 报警名称字符串的最大长度。 */
#define NET_ALARM_NAME_LEN                     64
/* 自定义音频名称字符串的最大长度。 */
#define NET_ALARM_CUSTOM_AUDIO_NAME_LEN        64
/* 自定义音频文件路径字符串的最大长度。 */
#define NET_ALARM_CUSTOM_AUDIO_PATH_LEN        64
/* 报警配置可关联的最大复制目标数量。 */
#define NET_ALARM_COPY_TO_MAX_NUM              64
/* 报警配置结构体中保留字段的字节长度。 */
#define NET_ALARM_CONFIG_RESERVED_LEN          64

#define NET_INVALID_PARAM                           (0xffffff)

/* 停车场车牌、车辆图片大小 单位：字节*/
#define NET_VEH_PLATE_IMAGE_LEN                 (1024*1024)

/* 停车场车牌、车辆加密后图片大小 单位：字节*/
#define NET_VEH_PLATE_ENCODE_IMAGE_LEN          1400000

#define NET_UINT32_INVALID                      0xFFFFFFFF               /* UINT32类型无效值定义 */

/* BIT位定义 */
#ifndef BIT0
#define BIT0  (0x1 << 0)
#define BIT1  (0x1 << 1)
#define BIT2  (0x1 << 2)
#define BIT3  (0x1 << 3)
#define BIT4  (0x1 << 4)
#define BIT5  (0x1 << 5)
#define BIT6  (0x1 << 6)
#define BIT7  (0x1 << 7)
#define BIT8  (0x1 << 8)
#define BIT9  (0x1 << 9)
#define BIT10 (0x1 << 10)
#define BIT11 (0x1 << 11)
#define BIT12 (0x1 << 12)
#define BIT13 (0x1 << 13)
#define BIT14 (0x1 << 14)
#define BIT15 (0x1 << 15)
#define BIT16 (0x1 << 16)
#define BIT17 (0x1 << 17)
#define BIT18 (0x1 << 18)
#define BIT19 (0x1 << 19)
#define BIT20 (0x1 << 20)
#define BIT21 (0x1 << 21)
#define BIT22 (0x1 << 22)
#define BIT23 (0x1 << 23)
#define BIT24 (0x1 << 24)
#define BIT25 (0x1 << 25)
#define BIT26 (0x1 << 26)
#define BIT27 (0x1 << 27)
#define BIT28 (0x1 << 28)
#define BIT29 (0x1 << 29)
#define BIT30 (0x1 << 30)
#define BIT31 (0x1 << 31)

#define BIT(nr)     (1UL << (nr))
#endif  /* BIT位定义 */

#ifndef BIT32_MAX
    #define BIT32_MAX ((UINT32)(~0UL))
#endif


/* BEGIN****************************  Alarm type ************************************************************/
/**
 * @brief 基础/常规报警 (0x1000 - 0x10FF)
 * @struct NET_AlarmBasicInfo_S
 */
#define NET_ALARM_BASE_BASIC         0x1000
#define NET_ALARM_MOTION_DETECT      (NET_ALARM_BASE_BASIC + 0x01) // 移动侦测
#define NET_ALARM_OCCLUSION          (NET_ALARM_BASE_BASIC + 0x02) // 视频遮挡
#define NET_ALARM_ANOMALY            (NET_ALARM_BASE_BASIC + 0x03) // 异常报警(通用)
#define NET_ALARM_AUDIO              (NET_ALARM_BASE_BASIC + 0x04) // 声音报警(分贝阈值)
#define NET_ALARM_INPUT              (NET_ALARM_BASE_BASIC + 0x05) // 报警输入(IO)
#define NET_ALARM_OUTPUT             (NET_ALARM_BASE_BASIC + 0x06) // 报警输出(IO)
#define NET_ALARM_FLASH              (NET_ALARM_BASE_BASIC + 0x07) // 闪光灯报警
#define NET_ALARM_PIR                (NET_ALARM_BASE_BASIC + 0x08) // PIR红外报警

/**
 * @brief 区域/周界规则报警 (0x2000 - 0x20FF)
 * @struct NET_AlarmRuleInfo_S
 */
#define NET_ALARM_BASE_RULE          0x2000
#define NET_ALARM_LINE_CROSSING      (NET_ALARM_BASE_RULE + 0x01)  // 越界侦测
#define NET_ALARM_INTRUSION          (NET_ALARM_BASE_RULE + 0x02)  // 区域入侵
#define NET_ALARM_ENTER_REGION       (NET_ALARM_BASE_RULE + 0x03)  // 进入区域
#define NET_ALARM_LEAVE_REGION       (NET_ALARM_BASE_RULE + 0x04)  // 离开区域
#define NET_ALARM_OBJECT_REMOVAL     (NET_ALARM_BASE_RULE + 0x05)  // 物品拿取
#define NET_ALARM_UNATTENDED_OBJECT  (NET_ALARM_BASE_RULE + 0x06)  // 物品遗留

/**
 * @brief Smart/AI 行为分析 (0x3000 - 0x3FFF)
 * @struct NET_AlarmAiObjectInfo_S
 */
// ------------------------------------------
#define NET_ALARM_BASE_AI            0x3000

// > 人脸/人员相关
#define NET_ALARM_FACE_DETECT        (NET_ALARM_BASE_AI + 0x01)    // 人脸侦测
#define NET_ALARM_FACE_CAPTURE       (NET_ALARM_BASE_AI + 0x02)    // 人脸抓拍
#define NET_ALARM_CROWD_GATHERING    (NET_ALARM_BASE_AI + 0x03)    // 人员聚集
#define NET_ALARM_LOITERING          (NET_ALARM_BASE_AI + 0x04)    // 徘徊侦测
#define NET_ALARM_PERSON_FALL        (NET_ALARM_BASE_AI + 0x05)    // 人员倒地
#define NET_ALARM_RUNNING            (NET_ALARM_BASE_AI + 0x06)    // 快速奔跑
#define NET_ALARM_FACE_COMPARE       (NET_ALARM_BASE_AI + 0x07)    // 人脸比对
#define NET_ALARM_PARKING_DETECT     (NET_ALARM_BASE_AI + 0x08)    // 停车侦测

// > 行为监管/安防
#define NET_ALARM_SLEEP_ON_DUTY      (NET_ALARM_BASE_AI + 0x20)    // 睡岗
#define NET_ALARM_LEAVE_POST         (NET_ALARM_BASE_AI + 0x21)    // 离岗
#define NET_ALARM_SMOKING            (NET_ALARM_BASE_AI + 0x22)    // 抽烟
#define NET_ALARM_PHONE_USAGE        (NET_ALARM_BASE_AI + 0x23)    // 玩手机
#define NET_ALARM_HELMET_MISSING     (NET_ALARM_BASE_AI + 0x24)    // 未戴安全帽 (SAFETY_HELMET)
#define NET_ALARM_NO_REFLECTIVE_VEST (NET_ALARM_BASE_AI + 0x25)    // 未穿反光衣 (REFLECTIVE_CLOTHING)
#define NET_ALARM_SMOKE_FIRE         (NET_ALARM_BASE_AI + 0x26)    // 烟火检测
#define NET_ALARM_PERSON_TRIP        (NET_ALARM_BASE_AI + 0x27)    // 人员绊倒识别
#define NET_ALARM_ELECTRIC_VEHICLE_IN_ELEVATOR \
                                        (NET_ALARM_BASE_AI + 0x28)    // 电瓶车进电梯识别
#define NET_ALARM_GARBAGE_EXPOSURE   (NET_ALARM_BASE_AI + 0x29)    // 垃圾暴露识别
#define NET_ALARM_GARBAGE_OVERFLOW   (NET_ALARM_BASE_AI + 0x2A)    // 垃圾满溢识别
#define NET_ALARM_MANHOLE_COVER_ABNORMAL \
                                        (NET_ALARM_BASE_AI + 0x2B)    // 井盖异常检测
#define NET_ALARM_FENCE_CLIMBING      (NET_ALARM_BASE_AI + 0x2C)    // 翻越围栏识别
#define NET_ALARM_BARE_SOIL           (NET_ALARM_BASE_AI + 0x2D)    // 黄土裸露识别
#define NET_ALARM_HOLE_PROTECTION_BAR (NET_ALARM_BASE_AI + 0x2E)    // 洞口防护栏识别
#define NET_ALARM_PEDESTRIAN_INTRUSION \
                                        (NET_ALARM_BASE_AI + 0x2F)    // 行人入侵识别
#define NET_ALARM_HIGH_ALTITUDE_SEATBELT \
                                        (NET_ALARM_BASE_AI + 0x30)    // 高空安全带识别
#define NET_ALARM_CONSTRUCTION_OCCUPY_ROAD \
                                        (NET_ALARM_BASE_AI + 0x31)    // 施工占道识别
#define NET_ALARM_EMERGENCY_LANE_OCCUPANCY \
                                        (NET_ALARM_BASE_AI + 0x32)    // 应急车道占用识别
#define NET_ALARM_REVERSE_DIRECTION   (NET_ALARM_BASE_AI + 0x33)    // 逆行识别
#define NET_ALARM_NON_MOTOR_VEHICLE_INTRUSION \
                                        (NET_ALARM_BASE_AI + 0x34)    // 非机动车入侵识别
#define NET_ALARM_ROAD_PONDING         (NET_ALARM_BASE_AI + 0x35)    // 道路积水识别
#define NET_ALARM_CONGESTION           (NET_ALARM_BASE_AI + 0x36)    // 拥堵识别
#define NET_ALARM_ILLEGAL_LANE_CHANGE  (NET_ALARM_BASE_AI + 0x37)    // 违法变道识别
#define NET_ALARM_SCENE_CHANGE         (NET_ALARM_BASE_AI + 0x38)    // 场景变更识别
#define NET_ALARM_OPEN_FLAME           (NET_ALARM_BASE_AI + 0x39)    // 明火识别


// > 音频智能
#define NET_ALARM_AUDIO_ANOMALY      (NET_ALARM_BASE_AI + 0x50)    // 音频异常
#define NET_ALARM_AUDIO_SUDDEN_RISE  (NET_ALARM_BASE_AI + 0x51)    // 声强陡升检测
#define NET_ALARM_AUDIO_SUDDEN_DROP  (NET_ALARM_BASE_AI + 0x52)    // 声强突降检测

/**
 * @brief 交通/车辆相关 (0x4000 - 0x40FF)
 * @struct NET_AlarmPlateInfo_S
 */
#define NET_ALARM_BASE_TRAFFIC       0x4000
#define NET_ALARM_PLATE_RECOGNITION  (NET_ALARM_BASE_TRAFFIC + 0x01) // 车牌识别
#define NET_ALARM_ILLEGAL_PARKING    (NET_ALARM_BASE_TRAFFIC + 0x02) // 违章停车
#define NET_ALARM_TRAFFIC_CONGESTION (NET_ALARM_BASE_TRAFFIC + 0x03) // 交通拥堵

/**
 * 设备异常/状态事件 (0x5000 - 0x50FF)
 * @struct NET_AlarmExceptionInfo_S
*/
#define NET_ALARM_BASE_EXCEPTION     0x5000
#define NET_ALARM_DISK_FULL          (NET_ALARM_BASE_EXCEPTION + 0x01) // 硬盘满
#define NET_ALARM_DISK_ERROR         (NET_ALARM_BASE_EXCEPTION + 0x02) // 硬盘坏
#define NET_ALARM_NET_BROKEN         (NET_ALARM_BASE_EXCEPTION + 0x03) // 网络断开
#define NET_ALARM_IP_CONFLICT        (NET_ALARM_BASE_EXCEPTION + 0x04) // IP冲突
#define NET_ALARM_VIDEO_LOSS         (NET_ALARM_BASE_EXCEPTION + 0x05) // 视频丢失

/**
 * 统计类告警 (0x6000 - 0x60FF)
 * @struct NET_AlarmStatisticsInfo_S
 */
#define NET_ALARM_BASE_STATISTICS             0x6000
#define NET_ALARM_PEOPLE_FLOW_STATISTICS      (NET_ALARM_BASE_STATISTICS + 0x01) // 人流统计
#define NET_ALARM_PEOPLE_DENSITY_STATISTICS   (NET_ALARM_BASE_STATISTICS + 0x02) // 人员密度统计
#define NET_ALARM_STATISTICS_TARGET_MAX_NUM   2

/**
 * 抓拍类告警 (0x6100 - 0x61FF)
 * @struct NET_AlarmCaptureInfo_S
 */
#define NET_ALARM_BASE_CAPTURE                0x6100
#define NET_ALARM_CAPTURE_PEOPLE              (NET_ALARM_BASE_CAPTURE + 0x01)    // 行人抓拍
#define NET_ALARM_CAPTURE_FACE                (NET_ALARM_BASE_CAPTURE + 0x02)    // 人脸抓拍
#define NET_ALARM_CAPTURE_VEHICLE             (NET_ALARM_BASE_CAPTURE + 0x03)    // 机动车抓拍
#define NET_ALARM_CAPTURE_NON_MOTOR           (NET_ALARM_BASE_CAPTURE + 0x04)    // 非机动车抓拍

/**
 * SDK事件通知 (0x7000 - 0x70FF)
 */
#define NET_NOTIFY_BASE_EVENT                  0x7000
#define NET_NOTIFY_CHANNEL_STATUS              (NET_NOTIFY_BASE_EVENT + 0x01) // 通道上下线状态变更
/* END****************************  Alarm type ************************************************************/


/* BEGIN****************************  Enum ************************************************************/
/**
* @enum tagNETTVCommonErrCode
* @brief 通用错误码
* @attention 无 None
*/
typedef enum tagNETTVCommonErrCode
{
    NET_E_FAILED                         = -1,           /* 失败  Failed*/
    NET_E_SUCCEED                        = 0,            /* 成功  Succeeded*/
    NET_E_SVC_FAILED                     = 1,            /* 服务器失败错误码  Common Failed*/
    NET_E_NOT_AUTHORIZED                 = 3,            /* 用户无权限 User Not Authorized*/
    NET_E_NO_USER                        = 5,            /* 用户不存在 User does not exist*/
    NET_E_SDK_NOT_INIT                   = 6,            /* 未初始化SDK Not Init SDK*/

    NET_E_NO_RESULT                      = 11,           /* 查无结果 No result */
    NET_E_NOENOUGH_BUF                   = 12,           /* 缓冲区太小: 接收设备数据的缓冲区  Buffer is too small for receiving device data */
    NET_E_SDK_SOCKET_LSN_FAIL            = 13,           /* 创建socket listen失败  Failed to create socket listen */
    NET_E_INIT_MUTEX_FAIL                = 14,           /* 初始化锁失败  Failed to initialize lock */
    NET_E_INIT_SEMA_FAIL                 = 15,           /* 初始化信号量失败  Failed to initialize semaphore */
    NET_E_ALLOC_RESOURCE_ERROR           = 16,           /* SDK资源分配错误  Error occurred during SDK resource allocation */
    NET_E_HAVEDATA                       = 17,           /* 还有数据   Data not all sent */
    NET_E_NEEDMOREDATA                   = 18,           /* 需要更多数据  More data required  */
    NET_E_TRANSFILE_FAIL                 = 19,           /* 文件传输失败  File transmission failed */
    NET_E_DEVICE_TYPE_ERR                = 20,           /* 不支持的设备类型 Device type that are not supported */
    NET_E_NONCE_TIMEOUT                  = 21,           /* nonce过期 Nonce expired */
    NET_E_INNER_ERR                      = 22,           /* 系统内部错误 System internal error */
    NET_E_BINDNOTIFY_FAIL                = 24,           /* 绑定告警失败  Failed to bind alarms*/

    NET_E_SYSCALL_FALIED                 = 100,          /* 系统函数调用失败，请查看errno  Failed to call system function. See errno */
    NET_E_NULL_POINT                     = 101,          /* 空指针  Null pointer */
    NET_E_INVALID_PARAM                  = 102,          /* 无效参数  Invalid parameter */
    NET_E_INVALID_MODULEID               = 103,          /* 无效模块ID  Invalid module ID */
    NET_E_INVALID_HANDLE                 = 104,          /* 无效的句柄 Invalid handle */
    NET_E_NO_MEMORY                      = 105,          /* 内存分配失败 Memory allocation failed */
    NET_E_FILE_NO_EXIST                  = 106,          /* 文件等不存在 File does not exist */
    NET_E_NO_DEV                         = 107,          /* 设备不存在 Device does not exist*/
    NET_E_NO_FIT_LOG                     = 108,          /* 符合条件的日志不存在 Qualified logs do not exist*/
    NET_E_BUSY                           = 109,          /* busy状态 busy status */
    NET_E_TIMER_REG_FAILED               = 110,          /* 注册定时器失败 Register timer failed */
    NET_E_COMMON_FAILED                  = 111,          /* 通用错误 General error */
    NET_E_CMD_NOT_SUPPORT                = 112,          /* 命令不支持 Command does not support */
    NET_E_NOT_SUPPORT                    = 113,          /* 设备不支持该功能 The device does not support this function */
    NET_E_TIMEOUT                        = 114,          /* 超时 Overtime */
    NET_E_MSG_ERR                        = 115,          /* 消息不匹配 Message mismatch */
    NET_E_MODULE_INEXIST                 = 116,          /* 模块不存在 Module does not exist */
    NET_E_SOCKET_RECV_ERR                = 117,          /* 消息接收失败 Message acceptance failure */
    NET_E_DECODE_IE_FAILED               = 118,          /* 获取消息IE失败 Failure to get message IE */
    NET_E_ENCODE_IE_FAILED               = 119,          /* 添加消息IE失败 Failed to add message IE */
    NET_E_SDK_NOINTE_ERROR               = 120,          /* SDK未初始化 SDK uninitialized */
    NET_E_ALREDY_INIT_ERROR              = 121,          /* SDK已经初始化 SDK has been initialized */
    NET_E_DEVICE_FACTURER_ERR            = 122,          /* 不支持的设备厂商 Unsupported equipment manufacturer */
    NET_E_NAME_EXIST                     = 123,          /* 名称已存在 Name already exists */
    NET_E_GET_CFG_FAILED                 = 124,          /* 获取配置信息出错 Error acquiring configuration information */
    NET_E_SET_CFG_FAILED                 = 125,          /* 设置配置信息出错 Error setting configuration information */
    NET_E_CHANNEL_OVER_SPEC              = 126,          /* 通道数超规格 Channel number exceeding specification */
    NET_E_CALL_DRV_COMMON                = 127,          /* 调用驱动通用失败 Call driver universal failure */
    NET_E_TOTAL_QUOTA_FULL               = 128,          /* 可分配的配额空间不足 Insufficient allocatable quota space */
    NET_E_CALL_DB_COMMON                 = 129,          /* 调用数据库通用失败 Failure to invoke database universality */
    NET_E_NEED_MORE_MEMORY               = 130,          /* 内存分配不足 Insufficient memory allocation */
    NET_E_T2U_CONNECT_FAILED             = 131,          /* T2U连接失败 Failure of T2U connection */
    NET_E_FUNC_IS_INITIALIZING           = 132,          /* 功能正在初始化中 Functions are being initialized */

    NET_E_CONNECT_ERROR                  = 200,          /* 创建连接失败  Failed to create connection */
    NET_E_SEND_MSG_ERROR                 = 201,          /* 发送消息失败 Failed to send message */
    NET_E_DECODE_RSP_ERROR               = 202,          /* 解析响应消息失败  Failed to decode response message */
    NET_E_NONSUPPORT                     = 203,          /* 该功能函数未实现  Function not supported */
    NET_E_JSON_ERROR                     = 204,          /* Json 通用错误  Json common error */
    NET_E_NORESULT                       = 205,          /* 查询结果为空 The query result is empty */
    NET_E_SOCKET_RECV_ERROR              = 206,          /* Socket接收消息失败  Socket failed to receive message */
    NET_E_CREATE_THREAD_FAIL             = 207,          /* 创建线程失败  Failed to create thread */
    NET_E_RESCODE_NO_EXIST               = 208,          /* 资源编码不存在  Resource code not exist */
    NET_E_MSG_DATA_INVALID               = 209,          /* 消息内容错误  Incorrect message content */
    NET_E_JSON_NO_IMAGE                  = 210,          /* 图片数据为空 Picture data is empty */
    NET_E_IMAGE_SIZE_BEYOND_THE_LIMIT    = 211,          /* 图片大小超出限制 Image size beyond the limit*/

}NET_COMMON_ECODE_E;

/**
* @enum tagNETTVMediaErrCode
* @brief 媒体相关错误码
* @attention 无 None
*/
typedef enum tagNETTVMediaErrCode
{
    NET_E_PLAYER_FAIL                        = 1001,     /* 执行失败 Execution failure */
    NET_E_PLAYER_INVALID_PARAM               = 1002,     /* 输入参数非法 Illegal input parameters */
    NET_E_PLAYER_NO_MEMORY                   = 1003,     /* 系统内存不足 Insufficient system memory */
    NET_E_PLAYER_SOCKET_FAIL                 = 1004,     /* 创建SOCKET失败 Failure to create SOCKET */
    NET_E_PLAYER_RECV_FAIL                   = 1005,     /* 接收失败 Failure to receive */
    NET_E_PLAYER_RECV_ZERO                   = 1006,     /* 接收为零 Receive zero */
    NET_E_PLAYER_NOT_SUPPORT                 = 1007,     /* 功能暂不支持 Function not supported for the time being */
    NET_E_PLAYER_CREATETHREAD_FAILED         = 1008,     /* 创建线程失败 Failed to create thread */
    NET_E_PLAYER_OPENDL_FAILED               = 1009,     /* 加载动态库失败 Failure to load dynamic library */
    NET_E_PLAYER_SYMDL_FAILED                = 1010,     /* 获取动态方法失败 Failure to obtain dynamic methods */
    NET_E_PLAYER_SEND_FAILED                 = 1011,     /* 发送失败 Fail in send */
    NET_E_PLAYER_EACCES                      = 1012,     /* 创建文件权限不足 Insufficient permission to create files */
    NET_E_PLAYER_FILE_NOT_FIND               = 1013,     /* 读文件未找到 Reading file not found */
    NET_E_PLAYER_LOG_CLOSE                   = 1014,     /* 日志关闭 Log closes */
    NET_E_PLAYER_MEDIA_EXCEPTION             = 1017,     /* 内部处理异常 Media exception  */
    NET_E_PLAYER_SYS_FAIL                    = 1018,     /* 系统通用错误 */
    NET_E_PLAYER_INIT_DONE                   = 1019,     /* 已经初始化 */
    NET_E_PLAYER_SYS_RES_FAILED              = 1020,     /* 创建系统资源失败 */
    NET_E_PLAYER_INVALID_IP                  = 1021,     /* IP有误 */
    NET_E_PLAYER_EZSTREAMER_FULL             = 1022,     /* EZStreamer 规格满 */
    NET_E_PLAYER_VOD_OVER_ABILITY            = 1023,     /* 一体机流规格满 */
    NET_E_PLAYER_STREAM_IN_PROCESS           = 1024,     /* 流存储已经处理中 */
    NET_E_PLAYER_NO_SPARE_SESSION            = 1025,     /* 无空闲会话 */
    NET_E_PLAYER_NEED_AUTHENTICATE           = 1026,     /* 需要鉴权 */
    NET_E_PLAYER_GET_AUTHENTICATE_FAID       = 1027,     /* 获取鉴权失败 */
    NET_E_PLAYER_MAKE_AUTHENTICATE_FAID      = 1028,     /* 制作鉴权失败 */
    NET_E_PLAYER_AUTHENTICATEINFO_DIFF       = 1029,     /* 鉴权信息不一致 */
    NET_E_PLAYER_SESSION_CLOSED              = 1030,     /* 会话已关闭 */

    /*********************** Player 资源相关********************************/
    NET_E_FAIL_TO_INIT_EZPLAYER              = 1257,     /* 初始化播放器失败 Initialization player failed */
    NET_E_FAIL_TO_ALLOC_PORT_RES             = 1258,     /* 分配播放通道资源失败 Failed to allocate playback channel resources */
    NET_E_FAIL_TO_GET_PORT_RES               = 1259,     /* 获取播放通道资源失败 Failed to obtain playback channel resources*/
    NET_E_BUFFER_QUEUE_FULL                  = 1260,     /* 缓存队列已满 Cache queue full */
    NET_E_BUFFER_QUEUE_EMPTY                 = 1261,     /* 缓存队列空 Cache queue empty */
    NET_E_OPEN_FILE_FAILED                   = 1262,     /* 打开文件失败 Fail to open file */
    NET_E_FILE_READ_END                      = 1263,     /* 文件已经读取完毕 The file has been read out */
    NET_E_FILE_DISKSPACE_FULL                = 1264,     /* 磁盘空间满 Disk space is full */
    NET_E_FILE_READ_FAIL                     = 1265,     /* 读取失败 Read failure */
    NET_E_MCM_MIC_NOT_EXIST                  = 1266,     /* 麦克风不存在 Microphone does not exist */
    NET_E_TS_PACKET_IN_THE_ROUGH             = 1267,     /* TS打包未完成 TS packaging not completed */
    NET_E_FILE_RECORD_FINISH                 = 1268,     /* 录像保存完毕 The video has been saved.*/
    NET_E_VIDEO_RESOLUTION_CHANGE            = 1269,     /* 分辨率发生变化 Resolution changed */

    NET_E_FAIL_TO_OPEN_STREAM                = 1513,     /* 启动媒体流播放失败 Failed to start media stream playback */
    NET_E_FAIL_TO_CLOSE_STREAM               = 1514,     /* 关闭媒体流播放失败 Failed to shut down media stream playback */
    NET_E_FAIL_TO_RECV_DATA                  = 1515,     /* 网络故障导致接收数据失败 Failure of receiving data due to network failure */
    NET_E_FAIL_TO_PROCESS_MEDIA_DATA         = 1516,     /* 媒体数据处理失败 Media data processing failure */
    NET_E_NOT_START_PLAY                     = 1517,     /* 播放通道未开始播放操作 Play channel did not start playback operation */
    NET_E_FAIL_TO_INPUT_DATA                 = 1518,     /* 输入媒体流数据失败 Input media streaming data failed */
    NET_E_INPUTDATA_BUFFER_FULL              = 1519,     /* 输入数据缓存满 Input data cached full */
    NET_E_FAIL_TO_SET_PROCESS_DATA_CB        = 1520,     /* 设置媒体流数据回调函数失败 Failed to set media stream data callback function */
    NET_E_VOICE_RUNNING                      = 1521,     /* 语音业务运行过程中出错 Errors in Voice Service Operation */
    NET_E_FAIL_TO_OPEN_VOICE_SVC             = 1522,     /* 启动语音业务失败 Failure to start voice service */
    NET_E_FAIL_TO_CLOSE_VOICE_SVC            = 1523,     /* 关闭语音业务失败 Failed to shut down voice service */
    NET_E_UNKNOWN_STREAM_TYPE                = 1524,     /* 未知媒体流 Unknown media stream */
    NET_E_PACKET_LOSE                        = 1525,     /* 丢包 Packet loss */
    NET_E_NEED_MORE_PACKET                   = 1526,     /* 拼包未完成，需要更多包 Packing is not completed, more packages are needed */
    NET_E_FAIL_TO_CREATE_DECODE              = 1527,     /* 创建解码器失败 Failed to create decoder */
    NET_E_FAIL_TO_DECODE                     = 1528,     /* 解码失败 Decoding failure */
    NET_E_RECV_DATA_NOTENOUGH                = 1529,     /* 接收数据不足 Insufficient data received */
    NET_E_RENDER_RES_FULL                    = 1530,     /* 显示资源满 Display full resources */
    NET_E_RENDER_RES_NOT_EXIST               = 1531,     /* 显示资源不存在 Show that resources do not exist */
    NET_E_CREATE_DEV_FAILED                  = 1532,     /* 资源创建失败 Resource creation failed */
    NET_E_AUDIO_RES_NOT_EXIST                = 1533,     /* 音频资源不存在 Audio resources do not exist */
    NET_E_IHW265D_NEED_MORE_BITS             = 1534,     /* 解码器需要更多数据 Decoder needs more data */
    NET_E_FAIL_TO_CREATE_ENCODE              = 1535,     /* 创建编码器失败 Failure to create encoder */
    NET_E_CAPTURE_RES_EXIST                  = 1536,     /* 采集资源不存在 Collection resources do not exist */
    NET_E_RECORD_STARTED                     = 1537,     /* 录像已打开 The video has been turned on */
    NET_E_NEED_WAIT_DECODEC                  = 1538,     /* 未解码完成，需要等待 Undecoded, need to wait */
    NET_E_MORE_DATA_NEED_PACKET              = 1539,     /* 数据过多，还需要继续打包 There's too much data to pack. */
    NET_E_AAC_LC_DECODE_FAIL                 = 1540,     /* AAC_LC解码失败 AAC_LC decode failure*/
    NET_E_RENDER_SURFACELOST                 = 1541,     /* 显示表面丢失 */
    NET_E_FILE_ENCRYPED                      = 1543,     /* 文件已加密 */
    NET_E_SCRAMBLING_INFO_FAILED             = 1544,     /* 加扰信息异常 */

    /* 媒体会话业务异常上报错误码 */
    NET_E_LIVE_EXISTED                       = 2000,     /* 实况业务已经建立 Live business has been established */
    NET_E_LIVE_INPUT_NOT_READY               = 2001,     /* 媒体流未准备就绪 Media streaming is not ready */
    NET_E_LIVE_OUTPUT_BUSY                   = 2002,     /* 实况业务显示资源忙 Live business display resources busy */
    NET_E_LIVE_CB_NOTEXIST                   = 2003,     /* 实况控制块不存在 Real-time control block does not exist */
    NET_E_LIVE_STREAM_FULL                   = 2004,     /* 实况流资源已满 Real-time flow resources are full */
    NET_E_LIVE_NET_FAILED                    = 2005,     /* 会话网络错误 */
    NET_E_LIVE_NET_TIMEOUT                   = 2006,     /* 会话网络超时 */
    NET_E_LIVE_SHAKE_FAILED                  = 2007,     /* 会话交互错误 */
    NET_E_LIVE_AUTH_FAILED                   = 2008,     /* 鉴权失败 */
    NET_E_LIVE_INNER_ERROR                   = 2009,     /* 设备侧内部处理错误 */
    NET_E_LIVE_INNER_TIMEOUT                 = 2010,     /* 内部处理超时 */
    NET_E_LIVE_KEEP_ALIVE_FAILED             = 2011,     /* 保活失败 */
    NET_E_LIVE_SESSION_NOT_EXIST             = 2012,     /* 会话不存在 */
    NET_E_LIVE_NOT_ENOUGH_BANDWIDTH2         = 2013,     /* 带宽不足 */
    NET_E_LIVE_REALPLAY_ESTABLISHED          = 2014,     /* 实况业务已经建立 */
    NET_E_LIVE_REALPLAY_RES_BUSY             = 2015,     /* 实况业务显示资源忙 */
    NET_E_LIVE_MULTICAST_DISABLED            = 2016,     /* 组播使能关闭 */
    NET_E_LIVE_MULTICAST_PORT_OCCUPIED       = 2017,     /* 组播端口已被占用 */
    NET_E_LIVE_MULTICAST_PORT_EXHAUSTED      = 2018,     /* 组播端口已耗尽 */
    NET_E_LIVE_MULTICAST_USER_NOT_EXIST      = 2019,     /* 组播用户不存在 */
    NET_E_LIVE_CHANNEL_NOT_ONLINE            = 2020,     /* 通道不在线 */
    NET_E_LIVE_TALKBACK_ENCODED_INVALID      = 2021,     /* 语音对讲资源编码无效 */
    NET_E_LIVE_VOICE_RES_USED_BY_TALKBACK    = 2022,     /* 语音资源已被对讲使用 */
    NET_E_LIVE_TALKBACK_EXISTS               = 2023,     /* 语音对讲已存在 */
    NET_E_LIVE_VOICE_WORK_NOT_EXIST          = 2024,     /* 语音业务不存在 */
    NET_E_LIVE_TALKBACK_TIMEOUT              = 2025,     /* 建立语音对讲业务超时 */
    NET_E_LIVE_TALKBACK_ERROR                = 2026,     /* 语音对讲失败 */
    NET_E_LIVE_UNDEFINED_ERROR               = 2027,     /* 未定义错误 */
    NET_E_LIVE_BAD_REQUEST                   = 2028,     /* 错误的请求 */
    NET_E_LIVE_UNAUTHORIZED                  = 2029,     /* 未通过认证 */
    NET_E_LIVE_PAYMENT_REQUIRED              = 2030,     /* 需要付费 */
    NET_E_LIVE_FORIBIDDEN                    = 2031,     /* 禁止 */
    NET_E_LIVE_METHOD_NOT_ALLOWED            = 2032,     /* 不允许该方法 */
    NET_E_LIVE_NOT_ACCEPTABLE                = 2033,     /* 不接受 */
    NET_E_LIVE_PROXY_REQUIRED                = 2034,     /* 代理需要认证 */
    NET_E_LIVE_REQUEST_TIMEOUT               = 2035,     /* 请求超时 */
    NET_E_LIVE_GONE                          = 2036,     /* 不在服务器 */
    NET_E_LIVE_LENGTH_REQUIRED               = 2037,     /* 需要长度 */
    NET_E_LIVE_PRECONDITION_FAILED           = 2038,     /* 预处理失败 */
    NET_E_LIVE_ENTITY_TOO_LARGE              = 2039,     /* 请求实体过长 */
    NET_E_LIVE_URI_TOO_LARGE                 = 2040,     /* 请求-URI过长 */
    NET_E_LIVE_UNSUPPORTED_TYPE              = 2041,     /* 媒体类型不支持 */
    NET_E_LIVE_NOT_UNDERSTOOD                = 2042,     /* 不理解此参数 */
    NET_E_LIVE_CONFERENCE_NOT_FOUND          = 2043,     /* 找不到会议 */
    NET_E_LIVE_NOT_ENOUGH_BANDWIDTH          = 2044,     /* 带宽不足 */
    NET_E_LIVE_SESSION_NOT_FOUND             = 2045,     /* 找不到会话 */
    NET_E_LIVE_METHOD_NOT_VALID              = 2046,     /* 此状态下此方法无效 */
    NET_E_LIVE_HEADER_NOT_VALID              = 2047,     /* 此头部域对该资源无效 */
    NET_E_LIVE_INVALID_RANGE                 = 2048,     /* 无效范围 */
    NET_E_LIVE_PARAMETER_READ_ONLY           = 2049,     /* 参数是只读的 */
    NET_E_LIVE_AO_NOT_ALLOWED                = 2050,     /* 不允许合控制 */
    NET_E_LIVE_ONLY_AO_ALLOWED               = 2051,     /* 只允许合控制 */
    NET_E_LIVE_UNSUPPORTED_TRANSPORT         = 2052,     /* 传输方式不支持 */
    NET_E_LIVE_DESTINATION_UNREACHABLE       = 2053,     /* 无法到达目的地址 */
    NET_E_LIVE_INTERNAL_SERVER_ERROR         = 2054,     /* 服务器内部错误 */
    NET_E_LIVE_NOT_IMPLEMENTED               = 2055,     /* 未实现 */
    NET_E_LIVE_BAD_GATEWAY                   = 2056,     /* 网关错误 */
    NET_E_LIVE_SERVICE_UNAVAILABLE           = 2057,     /* 无法得到服务 */
    NET_E_LIVE_VERSION_NOT_SUPPORTED         = 2058,     /* 不支持此RTSP版本 */
    NET_E_LIVE_GATEWAY_TIMEOUT               = 2059,     /* 网关超时 */
    NET_E_LIVE_OPTION_NOT_SUPPORTED          = 2060,     /* 不支持选项 */
    NET_E_LIVE_MALLOC_FAIL                   = 2061,     /* 内存分配失败 */
    NET_E_LIVE_REALLOC_FAIL                  = 2062,     /* 内存再分配失败 */
    NET_E_LIVE_DESCRIBE_TIMEOUT              = 2063,     /* describe超时（大GOP、磁盘读取数据慢导致） */
    NET_E_LIVE_IPC_NOTBIND                   = 2064,     /* 通道未绑定，POE通道未接入IPC，非POE通道未添加IPC */
    NET_E_LIVE_DISK_ABNOMAL                  = 2065,     /* 磁盘异常 */

    NET_E_AUDIO_EXISTED                      = 2100,     /* 语音对讲已存在 Speech intercom already exists */
    NET_E_AUDIO_NO_EXISTED                   = 2101,     /* 语音业务不存在 Voice service does not exist */
    NET_E_AUDIO_RESCODE_INVALID              = 2102,     /* 语音对讲资源编码无效 Invalid encoding of voice intercom resources */
    NET_E_AUDIO_RES_USED_BY_TALK             = 2103,     /* 语音资源已被对讲使用  Audio resource is being used by two-way audio */
    NET_E_AUDIO_FAILED                       = 2104,     /* 语音对讲失败 Speech intercom failure */
    NET_E_AUDIO_AUDIOBCAST_FULL              = 2205,     /* 语音业务已满  No more audio service allowed */

    NET_E_CAPTURE_NO_SUPPORT_FORMAT          = 2200,     /* 抓拍格式不支持 Snapshot format does not support  */
    NET_E_CAPTURE_NO_ENOUGH_CAPACITY         = 2201,     /* 硬盘空间不足 Insufficient hard disk space */
    NET_E_CAPTURE_NO_DECODED_PICTURE         = 2202,     /* 没有解码过的图片可供抓拍 Undecoded pictures can be captured */
    NET_E_CAPTURE_SINGLE_FAILED              = 2203,     /* 单次抓拍操作失败 Single snap operation failed */

    NET_E_OVER_ABILITY                       = 2301,     /* 码流超出能力集 Bit stream excess capability set */

    /* 云媒体业务异常上报  Cloud media view exception report 2793~2809 */
    NET_E_CLOUD_DOWNLOAD_FINISH              = 2793,     /* 下载完成 */
    NET_E_CLOUD_PARSE_DOMAIN_FAIL            = 2794,     /* 解析域名失败 */
    NET_E_CLOUD_CONNECT_FAIL                 = 2795,     /* 连接失败 */
    NET_E_CLOUD_CONNECT_TIMEOUT              = 2796,     /* 连接超时 */
    NET_E_CLOUD_DOWNLOAD_TIMEOUT             = 2797,     /* 下载超时 */
    NET_E_CLOUD_DOWNLOAD_FAIL                = 2798,     /* 下载失败 */
    NET_E_CLOUD_NETWORK_POOR                 = 2799,     /* 网络较差 */
    NET_E_CLOUD_PLAY_FINISH                  = 2800,     /* 播放完成 */
    NET_E_CLOUD_DISK_FULL                    = 2801,     /* 磁盘空间满 */
    NET_E_CLOUD_AUTH_FAIL                    = 2802,     /* 鉴权失败 */
    NET_E_CLOUD_CURRENT_TIME                 = 2803,     /* 当前播放时间，仅用于上报 */
    NET_E_CLOUD_PRIOR_DISK_FULL              = 2804,     /* 磁盘预值满 */
    NET_E_CLOUD_NODE_NOT_EXIST               = 2805,     /* 时间节点不存在 */
    NET_E_CLOUD_NO_CACHE_PATH                = 2806,     /* 未设置缓存路径 */
    NET_E_CLOUD_MSG_SEND_FAIL                = 2807,     /* 消息发送失败 */
    NET_E_CLOUD_TASK_CANCELLED               = 2808,     /* 任务已取消 */
    NET_E_CLOUD_TASK_STREAM_CONTINUE         = 2809,     /* 流继续播放 */

    NET_E_MEDIA_INPUT_NOT_READY              = 10000,    /* 媒体流未准备就绪 Media streaming is not ready */
    NET_E_CCB_STATR_INVALID                  = 10001,    /* 控制块状态不可用 Control block state unavailable */
    NET_E_MEDIA_OUTPUT_BUSY                  = 10002,    /* 实况业务显示资源繁忙 Live business display resource busy */
    NET_E_MEDIA_START_LOCAL_LIVE_ERR         = 10003,    /* 实况媒体流未准备就绪 Live media streams are not ready */
    NET_E_MEDIA_START_LOCAL_REPLAY_ERR       = 10004,    /* 回放媒体流未准备就绪 Playback media streams are not ready */

    NET_E_MEDIA_BW_RECV_NOT_ENOUGH           = 10007,    /* 网络接收带宽不足 Insufficient network reception bandwidth */
    NET_E_MEDIA_BW_SEND_NOT_ENOUGH           = 10008,    /* 网络发送带宽不足 Insufficient network transmission bandwidth */
    NET_E_MEDIA_AUDIO_BROADCAST_TO_LIMIT     = 10009,    /* 语音广播业务已达上限 Voice broadcasting service has reached the upper limit */
    NET_E_MEDIA_AUDIO_CHL_BING_USED          = 10010,    /* 音频通道已被占用 Audio channel has been occupied */

    NET_E_MEDIA_NOT_SUPPORT_ENCODETYPE       = 10012,    /* 码流格式不支持 Encode type Not supported */

    NET_E_MEDIA_MAX                          = 10399     /* 媒体相关错误码最大值 Maximum Media Related Error Code */
}NET_MEDIA_ECODE_E;

/**
 * enum tagNETTVDeviceType
 * @brief 设备类型
 * @attention 无 None
 */
typedef enum tagNETTVDeviceType
{
    NET_DTYPE_UNKNOWN                        = 0,            /* Unknown type */
    NET_DTYPE_IPC                            = 1,            /* IPC range */
    NET_DTYPE_NVR                            = 2,            /* NVR range */
    NET_DTYPE_INVALID                        = 0xFFFF        /* 无效值  Invalid value */
}NET_DEVICE_TYPE_E;

/**
 * @enum tagNETTVException
 * @brief 异常回调的消息类型 枚举定义 Exception callback message types Enumeration definition
 * @attention 无 None
 */
typedef enum tagNETTVException
{
   NET_EXCEPTION_REPORT_REMUXING_FINISH     = 284,          /* 转封装完成 */

    /* 回放业务异常上报  Playback exceptions report 300~399 */
   NET_EXCEPTION_REPORT_VOD_END             = 300,          /* 回放结束  Playback ended*/
   NET_EXCEPTION_REPORT_VOD_ABEND           = 301,          /* 回放异常  Playback exception occured */
   NET_EXCEPTION_REPORT_BACKUP_END          = 302,          /* 备份结束  Backup ended */
   NET_EXCEPTION_REPORT_BACKUP_DISC_OUT     = 303,          /* 磁盘被拔出  Disk removed */
   NET_EXCEPTION_REPORT_BACKUP_DISC_FULL    = 304,          /* 磁盘已满  Disk full */
   NET_EXCEPTION_REPORT_BACKUP_ABEND        = 305,          /* 其他原因导致备份失败   Backup failure caused by other reasons */

   NET_EXCEPTION_EXCHANGE                   = 0x8000,       /* 用户交互时异常（用户保活超时）  Exception occurred during user interaction (keep-alive timeout) */
   NET_EXCEPTION_REPORT_ALARM_INTERRUPT     = 0x8001,       /* 告警上报异常结束 保活失败或者长连接断开重订阅成功上报新的订阅ID Failure to report abnormal termination of life preservation or disconnection of long connection */
   NET_EXCEPTION_ALARM_SUBSCRIBE_FAILED     = 0x8002,       /* 告警重订阅失败异常上报 */
   NET_EXCEPTION_COMMON_ALARM_RENEW_FAIL    = 0x8003,       /* 快速告警对接刷新失败异常上报 */


   NET_EXCEPTION_REPORT_MAX,                                /* 最大值  Maximum value */
   NET_EXCEPTION_REPORT_NOT_VALID_PERIOD,                   /* 不在有效期内 Not Valid period */
   NET_EXCEPTION_REPORT_NOT_VALID_TIME,                     /* 不在有效时段内 Not Valid Time */

   NET_EXCEPTION_REPORT_INVALID             = 0xFFFF        /* 无效值  Invalid value */
}NET_EXCEPTION_TYPE_E;

/**
 * @enum tagNET_CapabilityCommand
 * @brief 能力集命令 Device capability command
 * @attention 无
*/
typedef enum tagNET_CapabilityCommand
{
    NET_CAP_VIDEO_ENCODE             = 1,            /* 视频编码能力集 参见# NET_VideoStreamCap_S。 Video encoding capability. See # NET_VideoStreamCap_S for reference*/
    NET_CAP_OSD                      = 2,            /* OSD参数能力集 参见# NET_OsdCap_S。 OSD parameter capability. See # NET_OsdCap_S for reference*/
    NET_CAP_SMART                    = 3,            /* 智能能力集 参见# NET_SmartCap_S。 Smart capability. See # NET_SmartCap_S for reference */
    NET_CAP_IMAGE                    = 5,            /* 图像参数能力集 参见#NET_ImageCap_S。 Image capability See # NET_ImageCap_S for reference*/
    NET_CAP_AUDIO                    = 6,            /* 音频能力集 参见 NET_AudioCap_S */
    NET_CAP_CHANNELS_ALARM           = 13,           /* 通道告警能力集, 参见 NET_ChnAlarmCapInfo_S （单通道IPC对应SDK通道号传入参数1；多通道IPC对应SDK通道号传入参数1+IPC实际通道号；NVR下对应通道号传入实际通道号） */
    NET_CAP_SYS                      = 14,           /* 系统能力集 参见 NET_SysCapability_S  */
    NET_CAP_USER_MANAGE              = 23,           /* 用户管理能力集, 参见 NET_UserManageCapInfo_S */
    NET_CAP_MEDIA                    = 28,           /* 视频通道媒体能力集，详见 NET_MediaCapInfo_S */
    NET_CAP_INVALID                  = 0Xff
}NET_CapabilityCommand_E;

typedef enum tagNETTVCfgCmd
{
    NET_GET_DEVICECFG                = 100,              /* 获取设备信息,参见#NET_DeviceBasicInfo_S  Get device information, see #NET_DeviceBasicInfo_S */
    NET_SET_DEVICECFG                = 101,              /* 保留 Reserved */

    NET_GET_UPGRADESTATUS            = 102,              /* 获取设备升级状态信息 */
    NET_SET_UPGRADE                  = 103,              /* 设置设备升级信息 */
    NET_GET_UPGRADEVERSION           = 104,              /* 获取设备升级版本信息 */

    NET_GET_NTPCFG                   = 110,              /* 获取NTP参数,参见#NET_SystemNtpInfo_S  Get NTP parameter, see #NET_SystemNtpInfo_S */
    NET_SET_NTPCFG                   = 111,              /* 设置NTP参数,参见#NET_SystemNtpInfo_S  Set NTP parameter, see #NET_SystemNtpInfo_S */

    NET_GET_STREAMCFG                = 120,              /* 获取视频编码参数,参见#NET_VideoEncodeOption_S  Get video encoding parameter, see #NET_VideoEncodeOption_S */
    NET_SET_STREAMCFG                = 121,              /* 设置视频编码参数,参见#NET_VideoEncodeOption_S  Set video encoding parameter, see #NET_VideoEncodeOption_S */
    NET_GET_RTSPURLCFG               = 122,              /* 获取RTSP流地址,参见#NET_RtspUrlInfo_S  Get RTSP URL, see #NET_RtspUrlInfo_S */
    NET_GET_REPLAY_URLCFG            = 123,              /* 获取回放播放地址,参见#NET_ReplayUrlInfo_S  Get playback URL */
    NET_GET_REPLAY_RECORD_LIST       = 124,              /* 获取NVR回放录像时间段,参见#NET_ReplayRecordList_S */
    NET_SET_REPLAY_CTRL              = 125,              /* 控制回放开始/停止/暂停/倍速,参见#NET_ReplayCtrlInfo_S */

    NET_GET_AUDIOCFG                 = 130,              /* 获取音频编码参数,参见#NET_AudioCfg_S  Get audio encoding parameter, see #NET_AudioCfg_S */
    NET_SET_AUDIOCFG                 = 131,              /* 设置音频编码参数,参见#NET_AudioCfg_S  Set audio encoding parameter, see #NET_AudioCfg_S */

    NET_GET_OSDCAPCFG                = 140,              /* 获取OSD能力集配置信息,参见#NET_VideoOsdCfg_S  Get OSD configuration information, see #NET_VideoOsdCfg_S */
    NET_SET_OSDCAPCFG                = 141,              /* 设置OSD能力集配置信息,参见#NET_VideoOsdCfg_S  Set OSD configuration information, see #NET_VideoOsdCfg_S */

    NET_GET_IMAGECFG                 = 160,              /* 获取图像配置信息,参见#NET_ImageSetting_S  Get image configuration information, see #NET_ImageSetting_S */
    NET_SET_IMAGECFG                 = 161,              /* 设置图像配置信息,参见#NET_ImageSetting_S  Set image configuration information, see #NET_ImageSetting_S */

    NET_GET_NETWORKCFG               = 170,              /* 获取网络配置信息,参见#NET_NetworkCfg_S  Get network configuration information, see #NET_NetworkCfg_S */
    NET_SET_NETWORKCFG               = 171,              /* 设置网络配置信息,参见#NET_NetworkCfg_S  Set network configuration information, see #NET_NetworkCfg_S */

    NET_GET_PRIVACYMASKCFG           = 180,              /* 获取隐私遮盖配置信息,参见#NET_PrivacyMaskCfg_S  Get privacy mask configuration information, see #NET_PrivacyMaskCfg_S */
    NET_SET_PRIVACYMASKCFG           = 181,              /* 设置隐私遮盖配置信息,参见#NET_PrivacyMaskCfg_S  Set privacy mask configuration information, see #NET_PrivacyMaskCfg_S */

    NET_GET_TAMPERALARM              = 190,              /* 获取遮挡检测告警信息  参见#NET_TamperAlarmInfo_S  Get tamper alarm configuration information, see #NET_TamperAlarmInfo_S */
    NET_SET_TAMPERALARM              = 191,              /* 设置遮挡检测告警信息  参见#NET_TamperAlarmInfo_S  Set tamper alarm configuration information, see #NET_TamperAlarmInfo_S */

    NET_GET_MOTIONALARM              = 200,              /* 获取运动检测告警信息 参见#NET_MotionAlarmInfo_S  Get motion alarm configuration information, see #NET_MotionAlarmInfo_S */
    NET_SET_MOTIONALARM              = 201,              /* 设置运动检测告警信息 参见#NET_MotionAlarmInfo_S  Set motion alarm configuration information, see #NET_MotionAlarmInfo_S */

    NET_GET_CROSSLINEALARM           = 202,              /* 获取越界检测告警信息 参见NET_CROSS_LINE_ALARM_INFO_S Get Cross Line alarm configuration information, see #NET_CrossLineAlarmInfo_S*/
    NET_SET_CROSSLINEALARM           = 203,              /* 设置越界检测告警信息 参见NET_CROSS_LINE_ALARM_INFO_S Set Cross Line alarm configuration information, see #NET_CrossLineAlarmInfo_S*/

    NET_GET_INTRUSIONALARM           = 204,              /* 获取入侵检测告警信息 参见NET_INTRUSION_ALARM_INFO_S Get intrusion alarm configuration information, see #NET_IntrusionAlarmInfo_S*/
    NET_SET_INTRUSIONALARM           = 205,              /* 设置入侵检测告警信息 参见NET_INTRUSION_ALARM_INFO_S Set intrusion alarm configuration information, see #NET_IntrusionAlarmInfo_S*/

    NET_GET_LOITERINGALARM           = 206,              /* 获取徘徊侦测告警信息 参见NET_LOITERING_ALARM_INFO_S Get loitering alarm configuration information, see #NET_LoiteringAlarmInfo_S*/
    NET_SET_LOITERINGALARM           = 207,              /* 设置徘徊侦测告警信息 参见NET_LOITERING_ALARM_INFO_S Set loitering alarm configuration information, see #NET_LoiteringAlarmInfo_S*/

    NET_GET_CAPTURE_PLAN_INFO        = 208,              /* 获取抓图计划信息 */
    NET_SET_CAPTURE_PLAN_INFO        = 209,              /* 设置抓图计划信息 */
    NET_GET_CAPTURE_PARAM_INFO       = 210,              /* 获取抓图参数信息 */
    NET_SET_CAPTURE_PARAM_INFO       = 211,              /* 设置抓图参数信息 */

    NET_GET_EXPOSURE_INFO            = 212,              /* 获取曝光信息 */
    NET_SET_EXPOSURE_INFO            = 213,              /* 设置曝光信息 */
    NET_GET_DAYNIGHT_INFO            = 214,              /* 获取日夜转换信息 */
    NET_SET_DAYNIGHT_INFO            = 215,              /* 设置日夜转换信息 */
    NET_GET_BACKLIGHT_INFO           = 216,              /* 获取背光信息 */
    NET_SET_BACKLIGHT_INFO           = 217,              /* 设置背光信息 */
    NET_GET_DENOISE_INFO             = 218,              /* 获取降噪信息 */
    NET_SET_DENOISE_INFO             = 219,              /* 设置降噪信息 */
    NET_GET_WHITEBALANCE_INFO        = 220,              /* 获取白平衡信息 */
    NET_SET_WHITEBALANCE_INFO        = 221,              /* 设置白平衡信息 */

    NET_GET_AUDIOANOMALYALARM        = 222,              /* 获取音频异常侦测告警信息 参见NET_AUDIO_ANOMALY_ALARM_INFO_S */
    NET_SET_AUDIOANOMALYALARM        = 223,              /* 设置音频异常侦测告警信息 参见NET_AUDIO_ANOMALY_ALARM_INFO_S */
    NET_GET_PREVIEW_INFO             = 224,              /* 获取预览信息 参见NET_PREVIEW_INFO_S */
    NET_SET_PREVIEW_INFO             = 225,              /* 设置预览信息 参见NET_PREVIEW_INFO_S */
    NET_GET_SCENECHANGEALARM         = 226,              /* 获取场景变更侦测告警信息 参见NET_SCENE_CHANGE_ALARM_INFO_S */
    NET_SET_SCENECHANGEALARM         = 227,              /* 设置场景变更侦测告警信息 参见NET_SCENE_CHANGE_ALARM_INFO_S */
    NET_GET_CROWDGATHERINGALARM      = 228,              /* 获取人员聚集侦测告警信息 参见NET_CROWD_GATHERING_ALARM_INFO_S */
    NET_SET_CROWDGATHERINGALARM      = 229,              /* 设置人员聚集侦测告警信息 参见NET_CROWD_GATHERING_ALARM_INFO_S */
    NET_GET_PARKINGALARM             = 230,              /* 获取停车侦测告警信息 参见NET_PARKING_ALARM_INFO_S */
    NET_SET_PARKINGALARM             = 231,              /* 设置停车侦测告警信息 参见NET_PARKING_ALARM_INFO_S */
    NET_GET_UNATTENDEDOBJECTALARM    = 232,              /* 获取物品遗留侦测告警信息 参见NET_UNATTENDED_OBJECT_ALARM_INFO_S */
    NET_SET_UNATTENDEDOBJECTALARM    = 233,              /* 设置物品遗留侦测告警信息 参见NET_UNATTENDED_OBJECT_ALARM_INFO_S */
    NET_GET_OBJECTREMOVALALARM       = 234,              /* 获取物品拿取侦测告警信息 参见NET_OBJECT_REMOVAL_ALARM_INFO_S */
    NET_SET_OBJECTREMOVALALARM       = 235,              /* 设置物品拿取侦测告警信息 参见NET_OBJECT_REMOVAL_ALARM_INFO_S */
    NET_SET_CONFIG_WIFI_STA          = 236,              /* WIFI配置（STA） 参见 NET_WifiStaCfg_S */
    NET_CONNECT_WIFI_STA             = 237,              /* WIFI连接（STA） 参见 NET_WifiStaConnect_S */
    NET_DISCONNECT_WIFI_STA          = 238,              /* WIFI断开（STA） */
    NET_GET_4G_INFO                  = 239,              /* 获取4G配置 参见 NET_4GInfo_S */
    NET_SET_4G_INFO                  = 240,              /* 设置4G配置 参见 NET_4GInfo_S */
    NET_SET_HOTSPOT_INFO             = 241,              /* 设置热点配置 参见 NET_HotspotInfo_S */

    NET_GET_ENTERREGIONALARM         = 242,              /* 获取进入区域侦测告警信息 参见NET_ENTER_REGION_ALARM_INFO_S */
    NET_SET_ENTERREGIONALARM         = 243,              /* 设置进入区域侦测告警信息 参见NET_ENTER_REGION_ALARM_INFO_S */
    NET_GET_LEAVEREGIONALARM         = 244,              /* 获取离开区域侦测告警信息 参见NET_LEAVE_REGION_ALARM_INFO_S */
    NET_SET_LEAVEREGIONALARM         = 245,              /* 设置离开区域侦测告警信息 参见NET_LEAVE_REGION_ALARM_INFO_S */
    NET_GET_FACECAPTUREINFO          = 246,              /* 获取人脸抓拍配置信息 参见NET_FACE_CAPTURE_INFO_S */
    NET_SET_FACECAPTUREINFO          = 247,              /* 设置人脸抓拍配置信息 参见NET_FACE_CAPTURE_INFO_S */
    NET_GET_HOTSPOT_CONN             = 248,              /* 获取热点连接设备 参见 NET_HotspotConnInfo_S */
    NET_GET_FACECAPTUREOVERLAYINFO   = 249,              /* 获取人脸抓拍图片叠加配置 */
    NET_SET_FACECAPTUREOVERLAYINFO   = 250,              /* 设置人脸抓拍图片叠加配置 */

    NET_GET_CHANNEL_INFO             = 300,              /* 获取通道信息 参见NET_ChannelList_S（传channel=单通道；不传channel=全通道列表） */

    NET_STATE_TALKBACK               = 400,              /* 设置对讲状态信息 参见NET_INTERCOM_INFO_S */
    NET_TO_STREAM_TALKBACK           = 401,              /* 流媒体对讲：发送对讲数据 参见NET_REPLAY_TALKBACK_INFO_S */
    NET_FROM_STREAM_TALKBACK         = 402,              /* 流媒体对讲：接收对讲数据 参见NET_REPLAY_TALKBACK_INFO_S */
    NET_REPLAY_TALKBACK              = 403,              /* 流媒体对讲：回放对讲数据 参见NET_REPLAY_TALKBACK_INFO_S */

    NET_GET_GARBAGE_EXPOSURE_CFG     = 404,              /* 获取垃圾暴露配置 参见NET_GARBAGE_EXPOSURE_CFG_S */
    NET_SET_GARBAGE_EXPOSURE_CFG     = 405,              /* 设置垃圾暴露配置 参见NET_GARBAGE_EXPOSURE_CFG_S */
    NET_GET_GARBAGE_OVERFLOW_CFG     = 406,              /* 获取垃圾满溢配置 参见NET_GARBAGE_OVERFLOW_CFG_S */
    NET_SET_GARBAGE_OVERFLOW_CFG     = 407,              /* 设置垃圾满溢配置 参见NET_GARBAGE_OVERFLOW_CFG_S */

    NET_GET_PEOPLE_FLOW_STATISTICS_CFG = 408,           /* 获取人流统计配置 参见NET_PEOPLE_FLOW_STATISTICS_CFG_S */
    NET_SET_PEOPLE_FLOW_STATISTICS_CFG = 409,           /* 设置人流统计配置 参见NET_PEOPLE_FLOW_STATISTICS_CFG_S */
    NET_RESET_PEOPLE_FLOW_STATISTICS   = 410,           /* 立即清零人流统计结果 */
    NET_GET_PEOPLE_DENSITY_DETECTION_CFG = 411,         /* 获取人员密度检测配置 参见NET_PEOPLE_DENSITY_DETECTION_CFG_S */
    NET_SET_PEOPLE_DENSITY_DETECTION_CFG = 412,         /* 设置人员密度检测配置 参见NET_PEOPLE_DENSITY_DETECTION_CFG_S */

    NET_GET_MANHOLE_COVER_ABNORMAL_CFG = 413,           /* 获取井盖异常检测配置 参见NET_MANHOLE_COVER_ABNORMAL_CFG_S */
    NET_SET_MANHOLE_COVER_ABNORMAL_CFG = 414,           /* 设置井盖异常检测配置 参见NET_MANHOLE_COVER_ABNORMAL_CFG_S */
    NET_GET_SLEEP_ON_DUTY_CFG          = 415,           /* 获取睡岗识别配置 参见NET_SLEEP_ON_DUTY_CFG_S */
    NET_SET_SLEEP_ON_DUTY_CFG          = 416,           /* 设置睡岗识别配置 参见NET_SLEEP_ON_DUTY_CFG_S */
    NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG = 417,     /* 获取电瓶车进电梯识别配置 参见NET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S */
    NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG = 418,     /* 设置电瓶车进电梯识别配置 参见NET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S */
    NET_GET_PERSON_FALL_DOWN_CFG       = 419,           /* 获取人员倒地识别配置 参见NET_PERSON_FALL_DOWN_CFG_S */
    NET_SET_PERSON_FALL_DOWN_CFG       = 420,           /* 设置人员倒地识别配置 参见NET_PERSON_FALL_DOWN_CFG_S */
    NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG = 421,         /* 获取施工占道识别配置 参见NET_CONSTRUCTION_OCCUPY_ROAD_CFG_S */
    NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG = 422,         /* 设置施工占道识别配置 参见NET_CONSTRUCTION_OCCUPY_ROAD_CFG_S */
    NET_GET_CONGESTION_CFG             = 423,           /* 获取拥堵识别配置 参见NET_CONGESTION_CFG_S */
    NET_SET_CONGESTION_CFG             = 424,           /* 设置拥堵识别配置 参见NET_CONGESTION_CFG_S */
    NET_GET_LICENSE_PLATE_RECOGNITION_CFG = 425,        /* 获取车牌识别配置 参见NET_LICENSE_PLATE_RECOGNITION_CFG_S */
    NET_SET_LICENSE_PLATE_RECOGNITION_CFG = 426,        /* 设置车牌识别配置 参见NET_LICENSE_PLATE_RECOGNITION_CFG_S */
    NET_GET_HIGH_ALTITUDE_SEATBELT_CFG = 427,           /* 获取高空安全带识别配置 参见NET_HIGH_ALTITUDE_SEATBELT_CFG_S */
    NET_SET_HIGH_ALTITUDE_SEATBELT_CFG = 428,           /* 设置高空安全带识别配置 参见NET_HIGH_ALTITUDE_SEATBELT_CFG_S */
    NET_GET_SAFETY_HELMET_CFG          = 429,           /* 获取安全帽识别配置 参见NET_SAFETY_HELMET_CFG_S */
    NET_SET_SAFETY_HELMET_CFG          = 430,           /* 设置安全帽识别配置 参见NET_SAFETY_HELMET_CFG_S */
    NET_GET_PERSON_FALL_CFG            = 431,           /* 获取摔倒识别配置 参见NET_PERSON_FALL_CFG_S */
    NET_SET_PERSON_FALL_CFG            = 432,           /* 设置摔倒识别配置 参见NET_PERSON_FALL_CFG_S */
    NET_GET_PHONE_USAGE_CFG            = 433,           /* 获取玩手机识别配置 参见NET_PHONE_USAGE_CFG_S */
    NET_SET_PHONE_USAGE_CFG            = 434,           /* 设置玩手机识别配置 参见NET_PHONE_USAGE_CFG_S */
    NET_GET_SMOKING_CFG                = 435,           /* 获取抽烟识别配置 参见NET_SMOKING_CFG_S */
    NET_SET_SMOKING_CFG                = 436,           /* 设置抽烟识别配置 参见NET_SMOKING_CFG_S */
    NET_GET_OPEN_FLAME_CFG             = 437,           /* 获取明火识别配置 参见NET_OPEN_FLAME_CFG_S */
    NET_SET_OPEN_FLAME_CFG             = 438,           /* 设置明火识别配置 参见NET_OPEN_FLAME_CFG_S */
    NET_GET_BARE_SOIL_CFG              = 439,           /* 获取黄土裸露识别配置 参见NET_BARE_SOIL_CFG_S */
    NET_SET_BARE_SOIL_CFG              = 440,           /* 设置黄土裸露识别配置 参见NET_BARE_SOIL_CFG_S */
    NET_GET_HOLE_PROTECTION_BAR_CFG    = 441,           /* 获取洞口防护栏识别配置 参见NET_HOLE_PROTECTION_BAR_CFG_S */
    NET_SET_HOLE_PROTECTION_BAR_CFG    = 442,           /* 设置洞口防护栏识别配置 参见NET_HOLE_PROTECTION_BAR_CFG_S */
    NET_GET_REFLECTIVE_CLOTHING_CFG    = 443,           /* 获取反光衣识别配置 参见NET_REFLECTIVE_CLOTHING_CFG_S */
    NET_SET_REFLECTIVE_CLOTHING_CFG    = 444,           /* 设置反光衣识别配置 参见NET_REFLECTIVE_CLOTHING_CFG_S */

    NET_GET_PET_RECOGNITION_INFO       = 445,           /* 获取宠物识别配置 参见NET_PET_RECOGNITION_INFO_S */
    NET_SET_PET_RECOGNITION_INFO       = 446,           /* 设置宠物识别配置 参见NET_PET_RECOGNITION_INFO_S */
    NET_GET_CLIMB_FENCE_INFO           = 447,           /* 获取翻越围栏配置 参见NET_CLIMB_FENCE_INFO_S */
    NET_SET_CLIMB_FENCE_INFO           = 448,           /* 设置翻越围栏配置 参见NET_CLIMB_FENCE_INFO_S */
    NET_GET_DIMISSION_INFO             = 449,           /* 获取离岗配置 参见NET_DIMISSION_INFO_S */
    NET_SET_DIMISSION_INFO             = 450,           /* 设置离岗配置 参见NET_DIMISSION_INFO_S */
    NET_GET_ILLEGAL_LANE_INFO          = 451,           /* 获取违规变道配置 参见NET_ILLEGAL_LANE_INFO_S */
    NET_SET_ILLEGAL_LANE_INFO          = 452,           /* 设置违规变道配置 参见NET_ILLEGAL_LANE_INFO_S */
    NET_GET_RETROGRADE_INFO            = 453,           /* 获取逆行配置 参见NET_RETROGRADE_INFO_S */
    NET_SET_RETROGRADE_INFO            = 454,           /* 设置逆行配置 参见NET_RETROGRADE_INFO_S */
    NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO = 455,      /* 获取非机动车闯入配置 参见NET_NONMOTOR_VEHICLE_INTRUSION_INFO_S */
    NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO = 456,      /* 设置非机动车闯入配置 参见NET_NONMOTOR_VEHICLE_INTRUSION_INFO_S */
    NET_GET_OCCUPATION_EMERGENCY_INFO  = 457,           /* 获取应急车道占用识别配置 参见NET_OCCUPATION_EMERGENCY_INFO_S */
    NET_SET_OCCUPATION_EMERGENCY_INFO  = 458,           /* 设置应急车道占用识别配置 参见NET_OCCUPATION_EMERGENCY_INFO_S */
    NET_GET_PEDESTRIAN_INTRUSION_INFO  = 459,           /* 获取行人闯入配置 参见NET_PEDESTRIAN_INTRUSION_INFO_S */
    NET_SET_PEDESTRIAN_INTRUSION_INFO  = 460,           /* 设置行人闯入配置 参见NET_PEDESTRIAN_INTRUSION_INFO_S */
    NET_GET_SMOKE_FIRE_CFG             = 461,           /* 获取烟火识别配置 参见NET_SMOKE_FIRE_CFG_S */
    NET_SET_SMOKE_FIRE_CFG             = 462,           /* 设置烟火识别配置 参见NET_SMOKE_FIRE_CFG_S */
    NET_GET_ROAD_PONDING_CFG           = 463,           /* 获取道路积水检测配置 参见NET_ROAD_PONDING_CFG_S */
    NET_SET_ROAD_PONDING_CFG           = 464,           /* 设置道路积水检测配置 参见NET_ROAD_PONDING_CFG_S */
    NET_GET_SECURITY_SERVICES_INFO     = 465,           /* 获取安全服务配置 参见NET_SECURITY_SERVICES_INFO_S */
    NET_SET_SECURITY_SERVICES_INFO     = 466,           /* 设置安全服务配置 参见NET_SECURITY_SERVICES_INFO_S */
    NET_GET_SSH_COUNTDOWN              = 467,           /* 获取SSH倒计时 参见NET_SSH_COUNTDOWN_INFO_S */
    NET_FIND_LOG                       = 468,           /* 查询日志 参见NET_LOG_LIST_S */
    NET_EXPORT_LOG                     = 469,           /* 导出日志 参见NET_LOG_LIST_S */
    NET_GET_LOG_SERVER                 = 470,           /* 获取日志服务器配置 参见NET_LOG_SERVER_INFO_S */
    NET_SET_LOG_SERVER                 = 471,           /* 设置日志服务器配置 参见NET_LOG_SERVER_INFO_S */
    NET_TEST_LOG_SERVER                = 472,           /* 测试日志服务器配置 参见NET_LOG_SERVER_INFO_S */
    NET_CONTROL_RECORD_INFO            = 473,           /* 手动录像/停止录像 参见NET_RECORD_INFO_S，IPC实际接AC_SET_HUMAN_RECORD */
    NET_GET_RECORD_STATUS              = 474,           /* 获取录像状态 参见NET_RECORD_STATUS_INFO_S */
    NET_GET_RECORD_SCHEDULE            = 475,           /* 获取录像计划 参见NET_RECORD_SCHEDULE_S */
    NET_SET_RECORD_SCHEDULE            = 476,           /* 设置录像计划 参见NET_RECORD_SCHEDULE_S */
    NET_GET_RECORD_ADVANCED_PARAM      = 477,           /* 获取录像高级参数 参见NET_RECORD_ADVANCED_PARAM_S */
    NET_SET_RECORD_ADVANCED_PARAM      = 478,           /* 设置录像高级参数 参见NET_RECORD_ADVANCED_PARAM_S */
    NET_FIND_RECORD_FILE_INFO          = 479,           /* 查找录像文件 参见NET_RECORD_FILE_LIST_S */
    NET_DOWNLOAD_RECORD_FILE           = 480,           /* 下载录像文件 参见NET_RECORD_DOWNLOAD_LIST_S */
    NET_NOTICE_DOWNLOAD_RECORD_PROGRESS = 481,          /* 录像下载进度通知 参见NET_RECORD_DOWNLOAD_PROGRESS_S */
    NET_SET_FACE_COMPARE_INFO          = 482,           /* 设置人脸比对配置 参见NET_FACE_COMPARE_INFO_S */
    NET_ADD_TARGET_LIB                 = 483,           /* 添加目标库 参见NET_FACE_LIB_INFO_S */
    NET_DEL_TARGET_LIB                 = 484,           /* 删除目标库 参见NET_FACE_LIB_INFO_S */
    NET_SET_TARGET_LIB                 = 485,           /* 修改目标库 参见NET_FACE_LIB_INFO_S */
    NET_GET_TARGET_LIB                 = 486,           /* 获取目标库 参见NET_FACE_LIB_LIST_S */
    NET_ADD_FACE_INFO                  = 487,           /* 添加人脸 参见NET_FACE_INFO_S */
    NET_DEL_FACE_INFO                  = 488,           /* 删除人脸 参见NET_FACE_ID_INFO_S */
    NET_SET_FACE_INFO                  = 489,           /* 修改人脸 参见NET_FACE_INFO_S */
    NET_GET_FACE_INFO                  = 490,           /* 获取人脸 参见NET_FACE_INFO_LIST_S */
    NET_GET_VOICECOM_AUDIO_CFG         = 491,           /* 获取对讲音频参数 参见NET_VOICECOM_AUDIO_CFG_S */
    NET_SET_VOICECOM_AUDIO_CFG         = 492,           /* 设置对讲音频参数 参见NET_VOICECOM_AUDIO_CFG_S */

    NET_GET_SD_CARD_STATUS                = 493,           /* 获取 SD 卡状态 参见NET_SdCardStatus_S */
    NET_GET_AUDIBLE_ALARM_INFO            = 494,           /* 获取声音报警配置 参见NET_AudibleAlarmInfo_S */
    NET_SET_AUDIBLE_ALARM_INFO            = 495,           /* 设置声音报警配置 参见NET_AudibleAlarmInfo_S */
    NET_GET_ALARM_INPUT_INFO              = 496,           /* 获取报警输入配置 参见NET_AlarmInputInfoList_S */
    NET_SET_ALARM_INPUT_INFO              = 497,           /* 设置报警输入配置 参见NET_AlarmInputInfo_S */
    NET_GET_ALARM_OUTPUT_INFO             = 498,           /* 获取报警输出配置 参见NET_AlarmOutputInfoList_S */
    NET_SET_ALARM_OUTPUT_INFO             = 499,           /* 设置报警输出配置 参见NET_AlarmOutputInfo_S */
    NET_GET_FLASHING_LIGHT_ALARM_INFO     = 500,           /* 获取闪光报警配置 参见NET_FlashingLightAlarmInfo_S */
    NET_SET_FLASHING_LIGHT_ALARM_INFO     = 501,           /* 设置闪光报警配置 参见NET_FlashingLightAlarmInfo_S */
    NET_GET_PIR_ALARM_INFO                = 502,           /* 获取 PIR 报警配置 参见NET_PirAlarmInfo_S */
    NET_SET_PIR_ALARM_INFO                = 503,           /* 设置 PIR 报警配置 参见NET_PirAlarmInfo_S */
    NET_GET_STORAGE_INFO                  = 504,           /* 获取设备存储信息 参见NET_DeviceStorageInfo_S */
    NET_GET_AUDIO_ANOMALY_CURRENT_DB      = 505,           /* 获取音频异常侦测实时音量 参见NET_AudioAnomalyCurrentDb_S */

    NET_GET_REGISTERINFO                  = 520,           /* 获取注册信息 参见NET_RegisterInfo_S */
    NET_SET_REGISTERINFO                  = 521,           /* 设置注册信息 参见NET_RegisterInfo_S */

    NET_CFG_INVALID                  = 0xFFFF            /* 无效值  Invalid value */

}NET_CONFIG_COMMAND_E;

typedef enum tagNETTVReplayCtrlCmd
{
    NET_REPLAY_CTRL_START            = 1,                /* 开始播放，同时返回回放URL和会话ID */
    NET_REPLAY_CTRL_STOP             = 2,                /* 停止播放 */
    NET_REPLAY_CTRL_SET_SPEED        = 3,                /* 倍速播放 */
    NET_REPLAY_CTRL_PAUSE            = 4,                /* 暂停播放 */
    NET_REPLAY_CTRL_SET_SEEK         = 5,                /* 跳转播放时间 */
    NET_REPLAY_CTRL_RESUME           = 6,                /* 恢复播放 */
    NET_REPLAY_CTRL_INVALID          = 0xff
} NET_REPLAY_CTRL_CMD_E;

typedef enum tagNETTVReplayPlatformCtrlType
{
    NET_REPLAY_PLATFORM_CTRL_NONE                = 0,    /* 默认值/未指定 */
    NET_REPLAY_PLATFORM_CTRL_JUMP_TIME           = 1,    /* 跳进度条 */
    NET_REPLAY_PLATFORM_CTRL_BACKWARD_30S        = 2,    /* 后退30秒 */
    NET_REPLAY_PLATFORM_CTRL_FORWARD_30S         = 3,    /* 前进30秒 */
    NET_REPLAY_PLATFORM_CTRL_SPEED               = 4,    /* 倍速 */
    NET_REPLAY_PLATFORM_CTRL_PERSON_EVENT        = 5,    /* 人员事件 */
    NET_REPLAY_PLATFORM_CTRL_VEHICLE_EVENT       = 6,    /* 车辆事件 */
    NET_REPLAY_PLATFORM_CTRL_PERSON_VEHICLE_EVENT= 7,    /* 人车事件 */
    NET_REPLAY_PLATFORM_CTRL_CANCEL_EVENT        = 8,    /* 取消事件 */
    NET_REPLAY_PLATFORM_CTRL_INVALID             = 0xff
} NET_REPLAY_PLATFORM_CTRL_TYPE_E;

/**
 * @enum tagNETTVDeviceControlType
 * @brief 设备硬件控制类型枚举，作为 DeviceControl 统一入口的一级分类。
 */
typedef enum tagNETTVDeviceControlType
{
    NET_DEVICE_CTRL_TYPE_PTZ          = 1,       /* 云台控制 */
    NET_DEVICE_CTRL_TYPE_ALARM_LIGHT  = 2,       /* 声光控制 */
    NET_DEVICE_CTRL_TYPE_WIPER        = 3,       /* 雨刷控制 */
    NET_DEVICE_CTRL_TYPE_FILL_LIGHT   = 4,       /* 补光灯控制 */
    NET_DEVICE_CTRL_TYPE_RELAY        = 5,       /* 继电器控制 */
    NET_DEVICE_CTRL_TYPE_REBOOT       = 6,       /* 重启设备 */
    NET_DEVICE_CTRL_TYPE_RESET        = 7,       /* 配置重置，dwCommand指定重置类型 */
    NET_DEVICE_CTRL_TYPE_CUSTOM       = 1000,    /* 厂商自定义控制 */
    NET_DEVICE_CTRL_TYPE_INVALID      = 0xff
} NET_DEVICE_CONTROL_TYPE_E;

/**
 * @enum tagNETTVPtzControlCmd
 * @brief 云台/镜头控制命令枚举，dwControlType 为 NET_DEVICE_CTRL_TYPE_PTZ 时使用。
 */
typedef enum tagNETTVPtzControlCmd
{
    NET_PTZ_CTRL_UP          = 1,
    NET_PTZ_CTRL_DOWN        = 2,
    NET_PTZ_CTRL_LEFT        = 3,
    NET_PTZ_CTRL_RIGHT       = 4,
    NET_PTZ_CTRL_LEFT_UP     = 5,
    NET_PTZ_CTRL_LEFT_DOWN   = 6,
    NET_PTZ_CTRL_RIGHT_UP    = 7,
    NET_PTZ_CTRL_RIGHT_DOWN  = 8,
    NET_PTZ_CTRL_ZOOM_IN     = 9,
    NET_PTZ_CTRL_ZOOM_OUT    = 10,
    NET_PTZ_CTRL_FOCUS_NEAR  = 11,
    NET_PTZ_CTRL_FOCUS_FAR   = 12,
    NET_PTZ_CTRL_IRIS_OPEN   = 13,
    NET_PTZ_CTRL_IRIS_CLOSE  = 14,
    NET_PTZ_CTRL_STOP        = 15,
    NET_PTZ_CTRL_INVALID     = 0xff
} NET_PTZ_CONTROL_CMD_E;

/**
 * @enum tagNETTVAlarmLightControlCmd
 * @brief 声光控制命令枚举，dwControlType 为 NET_DEVICE_CTRL_TYPE_ALARM_LIGHT 时使用。
 */
typedef enum tagNETTVAlarmLightControlCmd
{
    NET_ALARM_LIGHT_CTRL_START    = 1,       /* 开启声光 */
    NET_ALARM_LIGHT_CTRL_STOP     = 2,       /* 停止声光 */
    NET_ALARM_LIGHT_CTRL_SET_MODE = 3,       /* 设置声光模式，参数由 dwParam1/dwParam2 或 szExt 承载 */
    NET_ALARM_LIGHT_CTRL_INVALID  = 0xff
} NET_ALARM_LIGHT_CONTROL_CMD_E;

/**
 * @enum tagNETTVResetControlCmd
 * @brief 配置重置命令枚举，dwControlType 为 NET_DEVICE_CTRL_TYPE_RESET 时使用。
 * @details 数据流向说明：
 *          1. 客户端调用 NET_DeviceControl 接口时，填充 NET_DeviceControlInfo_S 结构体
 *          2. 设置 dwControlType = NET_DEVICE_CTRL_TYPE_RESET
 *          3. 设置 dwCommand = NET_RESET_CTRL_FULL(1) 或 NET_RESET_CTRL_SIMPLE(2)
 *          4. 服务端在 DeviceControl 回调中读取 dwCommand 字段判断重置类型
 */
typedef enum tagNETTVResetControlCmd
{
    NET_RESET_CTRL_FULL       = 1,       /* 完全恢复：恢复默认参数包括IP地址，所有参数恢复至出厂默认 */
    NET_RESET_CTRL_SIMPLE     = 2,       /* 简单恢复：不包括IP地址、设备用户名、密码、激活状态 */
    NET_RESET_CTRL_INVALID    = 0xff
} NET_RESET_CONTROL_CMD_E;

/**
 * @struct tagNET_DeviceControlInfo
 * @brief 设备硬件控制统一参数结构体，可承载云台、声光、雨刷、补光灯、继电器和自定义控制。
 */
typedef struct tagNET_DeviceControlInfo
{
    UINT32 uSize;           /* 结构体大小，调用方填 sizeof(NET_DeviceControlInfo_S)，用于后续兼容扩展 */
    INT32  uChannelID;      /* 通道号 */
    INT32  uControlType;    /* 控制类型，参见 NET_DEVICE_CONTROL_TYPE_E */
    INT32  uCommand;        /* 控制命令，不同 uControlType 对应不同命令枚举 */
    INT32  uSpeed;          /* 云台速度，范围 NET_MIN_PTZ_SPEED_LEVEL ~ NET_MAX_PTZ_SPEED_LEVEL，非云台可忽略 */
    INT32  uDurationMs;     /* 持续时间，单位毫秒，0 表示由设备侧按命令语义自行处理 */
    INT32  uParam1;         /* 扩展参数1，例如声光模式、亮度、继电器号等 */
    INT32  uParam2;         /* 扩展参数2，例如音量、持续次数等 */
    CHAR   szExt[256];      /* 扩展参数，建议存放 JSON 字符串或厂商自定义参数 */
} NET_DeviceControlInfo_S;

typedef NET_DeviceControlInfo_S* pNET_DeviceControlInfo_S;

#define NET_RECORD_FRAME_FLAG_MARKER        0x00000001u  /* RTP marker，通常表示一帧结束 */
#define NET_RECORD_FRAME_FLAG_KEY_FRAME     0x00000002u  /* 视频关键帧 */
#define NET_RECORD_FRAME_FLAG_STREAM_END    0x00000004u  /* 录像帧流结束 */
#define NET_RECORD_FRAME_PAYLOAD_TYPE_VIDEO 96           /* 动态负载类型: 视频 */
#define NET_RECORD_FRAME_PAYLOAD_TYPE_AUDIO 97           /* 动态负载类型: 音频 */
#define NET_RECORD_FRAME_PAYLOAD_TYPE_END   127          /* 流结束包 */
#define NET_RECORD_FRAME_MAX_PAYLOAD_SIZE   (4u * 1024u * 1024u) /* 单包最大负载，4MB */

/**
 * @enum tagNET_RecordFrameMediaType
 * @brief 录像帧媒体类型
 */
typedef enum tagNET_RecordFrameMediaType
{
    NET_RECORD_FRAME_MEDIA_VIDEO = 1,
    NET_RECORD_FRAME_MEDIA_AUDIO = 2,
    NET_RECORD_FRAME_MEDIA_END   = 3
} NET_RecordFrameMediaType_E;

/**
 * @enum tagNET_RecordFrameCodec
 * @brief 录像帧编码类型
 */
typedef enum tagNET_RecordFrameCodec
{
    NET_RECORD_FRAME_CODEC_UNKNOWN = 0,
    NET_RECORD_FRAME_CODEC_H264    = 1,
    NET_RECORD_FRAME_CODEC_H265    = 2,
    NET_RECORD_FRAME_CODEC_AAC     = 3,
    NET_RECORD_FRAME_CODEC_G711A   = 4,
    NET_RECORD_FRAME_CODEC_G711U   = 5,
    NET_RECORD_FRAME_CODEC_PCM     = 6
} NET_RecordFrameCodec_E;

/**
 * @struct tagNET_RecordFrameStreamCond
 * @brief 录像帧流启动条件。客户端传入通道和起止时间，服务端返回TCP端口和流标识。
 */
typedef struct tagNET_RecordFrameStreamCond
{
    UINT32 uSize;
    INT32  uChannel;
    CHAR   szStartTime[NET_LEN_64];
    CHAR   szEndTime[NET_LEN_64];
    INT32  uStreamIndex;        /* 码流索引，参见 NET_LIVE_STREAM_INDEX_E；0 表示默认主码流 */
    INT32  uMediaType;          /* 请求媒体类型，参见 NET_RecordFrameMediaType_E；默认视频 */
    INT32  uCodecType;          /* 期望编码，参见 NET_RecordFrameCodec_E；0 表示由服务端决定 */
    UINT32 uTcpPort;            /* 服务端录像帧TCP端口；0 表示使用服务端默认端口 */
    BYTE   byRes[128];
} NET_RecordFrameStreamCond_S;

typedef NET_RecordFrameStreamCond_S* pNET_RecordFrameStreamCond_S;

/**
 * @struct tagNET_RecordFrameStreamInfo
 * @brief 录像帧流启动结果。
 */
typedef struct tagNET_RecordFrameStreamInfo
{
    UINT32 uSize;
    CHAR   szStreamId[NET_STREAM_ID_LEN];
    INT32  uChannel;
    UINT32 uTcpPort;
    INT32  uMediaType;
    INT32  uCodecType;
    INT32  uWidth;
    INT32  uHeight;
    BYTE   byRes[128];
} NET_RecordFrameStreamInfo_S;

typedef NET_RecordFrameStreamInfo_S* pNET_RecordFrameStreamInfo_S;

/**
 * @struct tagNET_RecordFrameStopInfo
 * @brief 录像帧流停止参数。
 */
typedef struct tagNET_RecordFrameStopInfo
{
    UINT32 uSize;
    CHAR   szStreamId[NET_STREAM_ID_LEN];
    BYTE   byRes[64];
} NET_RecordFrameStopInfo_S;

typedef NET_RecordFrameStopInfo_S* pNET_RecordFrameStopInfo_S;

/**
 * @struct tagNET_RecordFrameInfo
 * @brief 录像帧信息。服务端取帧回调填充该结构，客户端收到TCP包后通过回调返回该结构。
 */
typedef struct tagNET_RecordFrameInfo
{
    UINT32 uSize;
    INT32  uMediaType;
    INT32  uCodecType;
    UINT32 uSeq;
    UINT32 uTimestamp;
    UINT64 ullPtsMs;
    UINT32 uPayloadLen;
    UINT32 uFlags;
    BYTE   byRes[64];
} NET_RecordFrameInfo_S;

typedef NET_RecordFrameInfo_S* pNET_RecordFrameInfo_S;

/**
 * @struct tagNETTVRecordFrameRtpHeader
 * @brief TCP上传输的简化RTP风格包头，网络字节序，固定20字节。
 * @note  TCP是字节流，因此比标准RTP多 uPayloadLen 字段用于分包。
 */
typedef struct tagNET_RecordFrameRtpHeader
{
    UCHAR  byVersion;      /* 固定为2 */
    UCHAR  byPayloadType;  /* 96视频，97音频，127结束 */
    UINT16 wSeq;
    UINT32 uTimestamp;
    UINT32 uSsrc;
    UINT32 uPayloadLen;
    UINT32 uFlags;
} NET_RecordFrameRtpHeader_S;

typedef NET_RecordFrameRtpHeader_S* pNET_RecordFrameRtpHeader_S;

/**
 * @enum tagNETTVVideoCodeType
 * @brief 视频编码格式 枚举定义  Video encoding format Enumeration definition
 * @attention 无 None
 */
typedef enum tagNETTVVideoCodeType
{
    NET_VIDEO_CODE_H264      = 0,          /* H.264 */
    NET_VIDEO_CODE_H265      = 1,          /* H.265 */
    NET_VIDEO_CODE_JPEG      = 2,          /* JPEG */
    NET_VIDEO_CODE_MJPEG     = 3,          /* MJPEG */
    NET_VIDEO_CODE_SVAC3     = 4,          /* SVAC3 */
    NET_VIDEO_CODE_MPEG4     = 5,          /* MPEG4 */
    NET_VIDEO_CODE_INVALID
}NET_VIDEO_CODE_TYPE_E;

/**
 * @enum tagNET_AudioInputType
 * @brief 音频输入类型 枚举定义 Audio input type Enumeration definition
 * @attention 无 None
 */
typedef enum tagNET_AudioInputType
{
    NET_AUDIO_INPUT_MICIN   = 0,   /* 麦克风输入 */
    NET_AUDIO_INPUT_LINEIN  = 1,   /* 线路输入  */
    NET_AUDIO_INPUT_INVALID
}NET_AudioInputType_E;


/**
 * @enum tagNET_AudioOutputType
 * @brief 音频输出类型 枚举定义 Audio output type Enumeration definition
 * @attention 无 None
 */
typedef enum tagNET_AudioOutputType
{
    NET_AUDIO_OUTPUT_SPEAKER  = 0, /* 扬声器输出 */
    NET_AUDIO_OUTPUT_LINEOUT  = 1, /* 线路输出   */
    NET_AUDIO_OUTPUT_MUTE     = 2, /* 静音       */
    NET_AUDIO_OUTPUT_INVALID
}NET_AudioOutputType_E;


/**
 * @enum tagNET_AudioFormat
 * @brief 音频编码格式 枚举定义 Audio encoding format Enumeration definition
 * @attention 无 None
 */
typedef enum tagNET_AudioFormat
{
    NET_AUDIO_FORMAT_G722_1  = 0,  /* G722.1 */
    NET_AUDIO_FORMAT_G711U   = 1,  /* G711U  */
    NET_AUDIO_FORMAT_G711A   = 2,  /* G711A  */
    NET_AUDIO_FORMAT_MP2L2   = 3,  /* MP2L2  */
    NET_AUDIO_FORMAT_G726    = 4,  /* G726   */
    NET_AUDIO_FORMAT_AAC     = 5,  /* AAC    */
    NET_AUDIO_FORMAT_PCM     = 6,  /* PCM    */
    NET_AUDIO_FORMAT_MP3     = 7,  /* MP3    */
    NET_AUDIO_FORMAT_INVALID
}NET_AudioFormat_E;


/**
 * @enum tagNET_AudioSampRate
 * @brief 音频采样率 枚举定义 Audio sample rate Enumeration definition
 * @attention 无 None
 */
typedef enum tagNET_AudioSampRate
{
    NET_AUDIO_SAMPRATE_8000   = 8000,   /* 8KHz   */
    NET_AUDIO_SAMPRATE_11025  = 11025,  /* 11.025KHz */
    NET_AUDIO_SAMPRATE_12000  = 12000,  /* 12KHz  */
    NET_AUDIO_SAMPRATE_16000  = 16000,  /* 16KHz  */
    NET_AUDIO_SAMPRATE_22050  = 22050,  /* 22.05KHz */
    NET_AUDIO_SAMPRATE_24000  = 24000,  /* 24KHz  */
    NET_AUDIO_SAMPRATE_32000  = 32000,  /* 32KHz  */
    NET_AUDIO_SAMPRATE_44100  = 44100,  /* 44.1KHz */
    NET_AUDIO_SAMPRATE_48000  = 48000,  /* 48KHz  */
    NET_AUDIO_SAMPRATE_64000  = 64000,  /* 64KHz  */
    NET_AUDIO_SAMPRATE_96000  = 96000,  /* 96KHz  */
    NET_AUDIO_SAMPRATE_INVALID = -1
}NET_AudioSampRate_E;


/**
 * @enum tagNET_AudioBitRate
 * @brief 音频码率 枚举定义 Audio bit rate Enumeration definition
 * @attention 无 None
 */
typedef enum tagNET_AudioBitRate
{
    NET_AUDIO_BITRATE_16K    = 16000,   /* 16Kbps  */
    NET_AUDIO_BITRATE_32K    = 32000,   /* 32Kbps  */
    NET_AUDIO_BITRATE_48K    = 48000,   /* 48Kbps  */
    NET_AUDIO_BITRATE_64K    = 64000,   /* 64Kbps  */
    NET_AUDIO_BITRATE_96K    = 96000,   /* 96Kbps  */
    NET_AUDIO_BITRATE_128K   = 128000,  /* 128Kbps */
    NET_AUDIO_BITRATE_256K   = 256000,   /* 256Kbps */
    NET_AUDIO_BITRATE_INVALID = -1
}NET_AudioBitRate_E;

/**
 * @enum tagNETTVLiveStreamIndex
 * @brief 实况业务流索引 枚举定 义 Live stream index Enumeration definition
 * @attention 无 None
 */
typedef enum tagNET_TVLiveStreamIndex
{
    NET_LIVE_STREAM_INDEX_MAIN       = 0,    /* 主流  Main stream */
    NET_LIVE_STREAM_INDEX_AUX        = 1,    /* 辅流  Sub stream */
    NET_LIVE_STREAM_INDEX_THIRD      = 2,    /* 第三流  Third stream */
    NET_LIVE_STREAM_INDEX_ADAPTIVE   = 10,   /* 自适应  Adaptive stream */
    NET_LIVE_STREAM_INDEX_INVALID    = 0xFF  /* 无效值  Invalid value */
}NET_LIVE_STREAM_INDEX_E;


/**
 * @enum tagNETTVOsdDateFormat
 * @brief OSD日期格式 枚举定义 (对应 OSD_DATE_FORMAT_E)
 * @attention
 */
typedef enum tagNETTVOsdDateFormat
{
    NET_OSD_DATE_YYYY_MM_DD = 0,         /* YYYY-MM-DD(年月日), 例如 2024-10-01 */
    NET_OSD_DATE_MM_DD_YYYY = 1,         /* MM-DD-YYYY(月日年), 例如 10-01-2024 */
    NET_OSD_DATE_DD_MM_YYYY = 2,         /* DD-MM-YYYY(日月年), 例如 01-10-2024 */
    NET_OSD_DATE_YYYY_MM_DD_CHN = 3,     /* YYYY年MM月DD日,     例如 2024年10月01日 */
    NET_OSD_DATE_MM_DD_YYYY_CHN = 4,     /* MM月DD日YYYY年,     例如 10月01日2024年 */
    NET_OSD_DATE_DD_MM_YYYY_CHN = 5,     /* DD日MM月YYYY年,     例如 01日10月2024年 */
    NET_OSD_DATE_YYYY_MM_DD_SLASH = 6,   /* YYYY/MM/DD(年月日), 例如 2024/10/01 */
    NET_OSD_DATE_MM_DD_YYYY_SLASH = 7,   /* MM/DD/YYYY(月日年), 例如 10/01/2024 */
    NET_OSD_DATE_DD_MM_YYYY_SLASH = 8,   /* DD/MM/YYYY(日月年), 例如 01/10/2024 */
    NET_OSD_DATE_FORMAT_INVALID = 0xFF
} NET_OSD_DATE_FORMAT_E;

/**
 * @enum tagNETTVOsdTimeFormat
 * @brief OSD时间格式 枚举定义 (对应 OSD_TIME_FORMAT_E)
 * @attention
 */
typedef enum tagNETTVOsdTimeFormat
{
    NET_OSD_TIME_FORMAT_24 = 0,          /* 24小时制 */
    NET_OSD_TIME_FORMAT_12 = 1,          /* 12小时制 */
    NET_OSD_TIME_FORMAT_INVALID = 0xFF
} NET_OSD_TIME_FORMAT_E;

/**
 * @enum tagNETTVOsdFontSize
 * @brief OSD字体大小 枚举定义 (对应 OSD_FONT_SIZE_E)
 * @attention
 */
typedef enum tagNETTVOsdFontSize
{
    NET_OSD_FONT_SIZE_ADAPTIVE = 0,      /* 自适应 */
    NET_OSD_FONT_SIZE_16 = 1,            /* 16 * 16 */
    NET_OSD_FONT_SIZE_32 = 2,            /* 32 * 32 */
    NET_OSD_FONT_SIZE_48 = 3,            /* 48 * 48 */
    NET_OSD_FONT_SIZE_64 = 4,            /* 64 * 64 */
    NET_OSD_FONT_SIZE_INVALID = 0xFF
} NET_OSD_FONT_SIZE_E;

/**
 * @enum tagNETTVOsdColor
 * @brief OSD颜色 枚举定义 (对应 OSD_COLOR_E)
 * @attention
 */
typedef enum tagNETTVOsdColor
{
    NET_OSD_COLOR_BLACK = 0,             /* 黑色 */
    NET_OSD_COLOR_WHITE = 1,             /* 白色 */
    NET_OSD_COLOR_CUSTOM = 2,            /* 自定义 */
    NET_OSD_COLOR_INVALID = 0xFF
} NET_OSD_COLOR_E;

/**
 * @enum tagNETTVOsdAlign
 * @brief OSD对齐方式 枚举定义 (对应 OSD_ALIGN_E)
 * @attention
 */
typedef enum tagNETTVOsdAlign
{
    NET_OSD_ALIGN_CUSTOMIZE = 0,         /* 自定义 */
    NET_OSD_ALIGN_CHAR_LEFT = 1,         /* 字符左对齐 */
    NET_OSD_ALIGN_CHAR_RIGHT = 2,        /* 字符右对齐 */
    NET_OSD_ALIGN_ALL_LEFT = 3,          /* 全部左对齐 */
    NET_OSD_ALIGN_ALL_RIGHT = 4,         /* 全部右对齐 */
    NET_OSD_ALIGN_GB_MODE = 5,           /* 国标模式 */
    NET_OSD_ALIGN_INVALID = 0xFF
} NET_OSD_ALIGN_E;

/**
 * @enum tagNETTVMotionMode
 * @brief 移动侦测模式  Motion detection mode
 * @attention
 */
typedef enum tagNETTVMotionMode
{
    NET_MOTION_MODE_NORMAL = 0,              /* 普通模式  Normal mode */
    NET_MOTION_MODE_EXPERT = 1               /* 专家模式  Expert mode */
} NET_MOTION_MODE_E;

/**
 * @enum tagNETTVCrossDirection
 * @brief 越界方向  Cross direction
 * @attention
 */
typedef enum tagNETTVCrossDirection
{
    NET_CROSS_BOTH_WAYS = 0,                 /* 双向  Both ways */
    NET_CROSS_A_TO_B = 1,                    /* A到B  A to B */
    NET_CROSS_B_TO_A = 2                     /* B到A  B to A */
} NET_CROSS_DIRECTION_E;

/**
 * @enum tagNETTVDetectionTarget
 * @brief 检测目标类型  Detection target type
 * @attention
 */
typedef enum tagNETTVDetectionTarget
{
    NET_TARGET_ALL = 0,                      /* 所有目标  All targets */
    NET_TARGET_HUMAN = 1,                    /* 人体  Human */
    NET_TARGET_VEHICLE = 2,                  /* 车辆  Vehicle */
    NET_TARGET_HUMAN_AND_VEHICLE = 3         /* 人和车  Human and vehicle */
} NET_DETECTION_TARGET_E;

/**
 * @enum tagNETTVBehaviorType
 * @brief 人员行为类型  Personnel behavior type
 * @attention
 */
typedef enum tagNETTVBehaviorType
{
    NET_BEHAVIOR_SLEEP_ON_DUTY = 0,          /* 睡岗  Sleep on duty */
    NET_BEHAVIOR_LEAVE_POST = 1,             /* 离岗  Leave post */
    NET_BEHAVIOR_SMOKING = 2,                /* 抽烟  Smoking */
    NET_BEHAVIOR_PHONE_USAGE = 3,            /* 玩手机  Phone usage */
    NET_BEHAVIOR_FENCE_CLIMBING = 4,         /* 翻越围栏  Fence climbing */
    NET_BEHAVIOR_FALL_DOWN = 5,              /* 人员倒地  Person fall down */
    NET_BEHAVIOR_TRIP = 6                    /* 摔倒  Trip */
} NET_BEHAVIOR_TYPE_E;

/**
 * @enum tagNETTVSafetyEquipmentType
 * @brief 安全装备类型  Safety equipment type
 * @attention
 */
typedef enum tagNETTVSafetyEquipmentType
{
    NET_EQUIPMENT_HELMET = 0,                /* 安全帽  Helmet */
    NET_EQUIPMENT_REFLECTIVE_CLOTHING = 1,   /* 反光衣  Reflective clothing */
    NET_EQUIPMENT_SEATBELT = 2               /* 高空安全带  High altitude seatbelt */
} NET_SAFETY_EQUIPMENT_TYPE_E;

/**
 * @brief 升级状态
 */
typedef enum _TiUpgradeRuslut_
{
    TI_UPGRADE_NULL              = -1, /* 未升级 */
    TI_UPGRADE_RUNING            = 0,  /* 升级中 */
    TI_UPGRADE_RUNFAIL           = 1,  /* 升级失败 */
    TI_UPGRADE_RUNSUCCESS        = 2,  /* 升级成功 */
    TI_UPGRADE_OTHERUPDATE_FAILE = 3,  /* 其他服务器升级失败 */
} TiUpgradeRuslut_E;

/**
 * @struct tagNETTVCapturePictureFormat
 * @brief 抓图格式
 * @attention
 */
typedef enum tagNETTVCapturePictureFormat
{
    NET_CAPTURE_PICTURE_FORMAT_JPEG = 0,
    NET_CAPTURE_PICTURE_FORMAT_BMP  = 1
} NET_CAPTURE_PICTURE_FORMAT_E;

/**
 * @struct tagNETTVCaptureImageQuality
 * @brief 抓图质量
 * @attention
 */
typedef enum tagNETTVCaptureImageQuality
{
    NET_CAPTURE_IMAGE_QUALITY_LOW    = 0,
    NET_CAPTURE_IMAGE_QUALITY_MEDIUM = 1,
    NET_CAPTURE_IMAGE_QUALITY_HIGH   = 2
} NET_CAPTURE_IMAGE_QUALITY_E;

/**
 * @struct tagNETTVCaptureTimeUnit
 * @brief 抓图时间单位
 * @attention
 */
typedef enum tagNETTVCaptureTimeUnit
{
    NET_CAPTURE_TIME_UNIT_MILLISECONDS = 0,
    NET_CAPTURE_TIME_UNIT_SECONDS      = 1,
    NET_CAPTURE_TIME_UNIT_MINUTES      = 2,
    NET_CAPTURE_TIME_UNIT_HOURS        = 3,
    NET_CAPTURE_TIME_UNIT_DAYS         = 4
} NET_CAPTURE_TIME_UNIT_E;

/**
 * @enum tagNETTVExposureTime
 * @brief 曝光时间  Exposure time
 */
typedef enum tagNETTVExposureTime
{
    NET_EXPOSURE_TIME_1_3      = 0,
    NET_EXPOSURE_TIME_1_6      = 1,
    NET_EXPOSURE_TIME_1_12     = 2,
    NET_EXPOSURE_TIME_1_25     = 3,
    NET_EXPOSURE_TIME_1_50     = 4,
    NET_EXPOSURE_TIME_1_100    = 5,
    NET_EXPOSURE_TIME_1_150    = 6,
    NET_EXPOSURE_TIME_1_200    = 7,
    NET_EXPOSURE_TIME_1_250    = 8,
    NET_EXPOSURE_TIME_1_500    = 9,
    NET_EXPOSURE_TIME_1_750    = 10,
    NET_EXPOSURE_TIME_1_1000   = 11,
    NET_EXPOSURE_TIME_1_2000   = 12,
    NET_EXPOSURE_TIME_1_4000   = 13,
    NET_EXPOSURE_TIME_1_10000  = 14,
    NET_EXPOSURE_TIME_1_100000 = 15,
    NET_EXPOSURE_TIME_INVALID  = 0xFF
} NET_EXPOSURE_TIME_E;

/**
 * @enum tagNETTVDayNightMode
 * @brief 日夜模式  Day/Night mode
 */
typedef enum tagNETTVDayNightMode
{
    NET_DAYNIGHT_MODE_DAY     = 0,
    NET_DAYNIGHT_MODE_NIGHT   = 1,
    NET_DAYNIGHT_MODE_AUTO    = 2,
    NET_DAYNIGHT_MODE_TIMING  = 3,
    NET_DAYNIGHT_MODE_INVALID = 0xFF
} NET_DAYNIGHT_MODE_E;

/**
 * @enum tagNETTVLightBrightMode
 * @brief 光照亮度模式  Light brightness mode
 */
typedef enum tagNETTVLightBrightMode
{
    NET_LIGHT_BRIGHT_MANUAL  = 0,
    NET_LIGHT_BRIGHT_AUTO    = 1,
    NET_LIGHT_BRIGHT_INVALID = 0xFF
} NET_LIGHT_BRIGHT_MODE_E;

/**
 * @enum tagNETTVLightType
 * @brief 灯光类型  Light type
 */
typedef enum tagNETTVLightType
{
    NET_LIGHT_TYPE_WHITE            = 0,
    NET_LIGHT_TYPE_RED              = 1,
    NET_LIGHT_TYPE_SMART            = 2,
    NET_LIGHT_TYPE_CLOSE            = 3,
    NET_LIGHT_TYPE_BOTH             = 4,
    NET_LIGHT_TYPE_RED_ON_WHITE_OFF = 5,
    NET_LIGHT_TYPE_WHITE_ON_RED_OFF = 6,
    NET_LIGHT_TYPE_INVALID          = 0xFF
} NET_LIGHT_TYPE_E;

/**
 * @enum tagNETTVBackLightArea
 * @brief 背光区域  Backlight area
 */
typedef enum tagNETTVBackLightArea
{
    NET_BACKLIGHT_AREA_CLOSE   = 0,
    NET_BACKLIGHT_AREA_UP      = 1,
    NET_BACKLIGHT_AREA_DOWN    = 2,
    NET_BACKLIGHT_AREA_LEFT    = 3,
    NET_BACKLIGHT_AREA_RIGHT   = 4,
    NET_BACKLIGHT_AREA_CENTER  = 5,
    NET_BACKLIGHT_AREA_INVALID = 0xFF
} NET_BACKLIGHT_AREA_E;

/**
 * @enum tagNETTVDnrMode
 * @brief 数字降噪模式  Digital noise reduction mode
 */
typedef enum tagNETTVDnrMode
{
    NET_DNR_MODE_CLOSE    = 0,
    NET_DNR_MODE_NORMAL   = 1,
    NET_DNR_MODE_ADVANCED = 2,
    NET_DNR_MODE_INVALID  = 0xFF
} NET_DNR_MODE_E;

/**
 * @enum tagNETTVAwbMode
 * @brief 自动白平衡模式  Auto white balance mode
 */
typedef enum tagNETTVAwbMode
{
    NET_AWB_MODE_AUTO         = 0,
    NET_AWB_MODE_MANUAL       = 1,
    NET_AWB_MODE_LOCK         = 2,
    NET_AWB_MODE_INCANDESCENT = 3,
    NET_AWB_MODE_WARM         = 4,
    NET_AWB_MODE_FLUORESCENT  = 5,
    NET_AWB_MODE_DAYLIGHT     = 6,
    NET_AWB_MODE_SEMI_AUTO    = 7,
    NET_AWB_MODE_REG_MANUAL   = 8,
    NET_AWB_MODE_INVAL        = 9,
    NET_AWB_MODE_INVALID      = 0xFF
} NET_AWB_MODE_E;

/**
 *  @enum tagNETTVWifiSecurityMode
 *  @brief WIFI 安全模式
 */
typedef enum tagNETTVWifiSecurityMode
{
    NET_WIFI_SECURITY_WPA_PERSONAL = 0,
    NET_WIFI_SECURITY_OPEN         = 1,
    NET_WIFI_SECURITY_WEP          = 2,
    NET_WIFI_SECURITY_EAP_PEAP     = 3,
    NET_WIFI_SECURITY_EAP_TLS      = 4,
    NET_WIFI_SECURITY_INVALID      = 0x7FFFFFFF
} NET_WIFI_SECURITY_MODE_E;

/**
 * @enum tagNETTVPeopleFlowStatType
 * @brief 人流统计类型
 */
typedef enum tagNETTVPeopleFlowStatType
{
    NET_PEOPLE_FLOW_STAT_TOTAL = 0,                /* 总人数 */
    NET_PEOPLE_FLOW_STAT_ENTER = 1,                /* 进入人数 */
    NET_PEOPLE_FLOW_STAT_LEAVE = 2                 /* 离开人数 */
} NET_PEOPLE_FLOW_STAT_TYPE_E;

typedef enum tagNET_StatisticsType
{
    NET_STATISTICS_TYPE_PEOPLE_FLOW = 1,           /* 人流统计 */
    NET_STATISTICS_TYPE_PEOPLE_DENSITY = 2         /* 人员密度统计 */
} NET_StatisticsType_E;

/**
 * @enum tagNET_CaptureType
 * @brief 抓拍类型枚举，用于通用抓拍结构体 NET_CaptureInfo_S 的 uCaptureType 字段
 */
typedef enum tagNET_CaptureType
{
    NET_CAPTURE_TYPE_PEOPLE    = 1,                /* 行人抓拍 */
    NET_CAPTURE_TYPE_FACE      = 2,                /* 人脸抓拍 */
    NET_CAPTURE_TYPE_VEHICLE   = 3,                /* 机动车抓拍 */
    NET_CAPTURE_TYPE_NON_MOTOR = 4,                /* 非机动车抓拍 */
    NET_CAPTURE_TYPE_INVALID   = 0xFF              /* 无效值 */
} NET_CaptureType_E;

/**
 * @enum tagNETTVRecordStatus
 * @brief 录制状态
 */
typedef enum tagNETTVRecordStatus
{
    NET_RECORD_STATUS_NO_OPERATION = 0,            /* 无操作 */
    NET_RECORD_STATUS_RECORDING    = 1,            /* 正在录制 */
    NET_RECORD_STATUS_PAUSE        = 2,            /* 暂停录制 */
    NET_RECORD_STATUS_STOP         = 3,            /* 停止录制 */
    NET_RECORD_STATUS_PRERECORD    = 4             /* 预览录制 */
} NET_RECORD_STATUS_E;

/**
 * @enum tagNETTVPopulationAlarmSeverity
 * @brief 人数报警等级
 */
typedef enum tagNETTVPopulationAlarmSeverity
{
    NET_POPULATION_ALARM_NORMAL = 0,               /* 普通报警 */
    NET_POPULATION_ALARM_MEDIUM = 1,               /* 中度报警 */
    NET_POPULATION_ALARM_SEVERE = 2              /* 严重报警 */
} NET_POPULATION_ALARM_SEVERITY_E;

/* END************* 枚举值  Enumeration value *************************** */


/* BEGIN*********** 结构体  Structure *********************************** */

/**
 * @struct tagNET_TalkbackStateInfo
 * @brief 对讲状态信息结构体
 * @note 对应命令：NET_STATE_TALKBACK
 */
typedef struct tagNET_TalkbackStateInfo
{
    BOOL    bEnable;                     /* 对讲功能使能标记 */
    CHAR    szSdp[NET_MAX_URL_LEN];  /* SDP 协议描述地址 */
    CHAR    szUrl[NET_MAX_URL_LEN];  /* 对讲服务 URL 地址 */
    CHAR    szLocalIP[NET_LEN_64];    /* 本机 IP 地址 */
    BYTE    byRes[128];                 /* 保留字段，未使用 */
} NET_TalkbackStateInfo_S;

typedef NET_TalkbackStateInfo_S* pNET_TalkbackStateInfo_S;

 /**
  * @struct tagNETTVVoiceComAudioParam
  * @brief VoiceCom 对讲音频参数
 * @note VoiceCom TCP 负载格式由 enFormat 指定，当前支持 PCM/G711A/G711U 裸流
  */
typedef struct tagNET_VoiceComAudioParam
{
    INT32   enFormat;             /* 音频格式, 参见 NET_AudioFormat_E, 当前支持 PCM/G711A/G711U */
    INT32   uSampleRate;          /* 采样率, Hz, 参见 NET_AudioSampRate_E */
    INT32   uBitDepth;            /* 位深, PCM=16, G711=8 */
    INT32   uChannels;            /* 声道数, 当前支持 1(单声道) */
    INT32   uFrameIntervalMs;     /* 单帧时长, ms */
    INT32   uFrameBytes;          /* 单帧字节数 */
    INT32   uBitRate;             /* 音频比特率, bit/s */
    BOOL    bLittleEndian;        /* PCM 字节序, TRUE 表示 little-endian; G711 忽略 */
    BYTE    byRes[128];           /* 保留字段 */
} NET_VoiceComAudioParam_S;

typedef NET_VoiceComAudioParam_S* pNET_VoiceComAudioParam_S;

/**
 * @struct tagNET_VoiceComStartInfo
 * @brief VoiceCom 启动参数
 */
typedef struct tagNET_VoiceComStartInfo
{
    UINT32  uAudioPort;                               /* 设备端 VoiceCom TCP 端口, 默认 9006 */
    NET_VoiceComAudioParam_S stAudioParam;            /* 本次对讲会话音频参数 */
    BYTE    byRes[128];                               /* 保留字段 */
} NET_VoiceComStartInfo_S;

typedef NET_VoiceComStartInfo_S* pNET_VoiceComStartInfo_S;

/**
 * @struct tagNET_VoiceComAudioCfg
 * @brief 对讲音频参数配置
 * @note 用于NET_GET_VOICECOM_AUDIO_CFG/NET_SET_VOICECOM_AUDIO_CFG
 */
typedef struct tagNET_VoiceComAudioCfg
{
    INT32   enFormat;             /* 音频格式, 参见 NET_AudioFormat_E, 当前支持 PCM/AAC/G711A/G711U */
    INT32   uSampleRate;          /* 采样率, Hz, 参见 NET_AudioSampRate_E */
    INT32   uBitDepth;            /* 位深, PCM=16, G711=8 */
    INT32   uChannels;            /* 声道数, 当前支持 1(单声道) */
    INT32   uFrameIntervalMs;     /* 单帧时长, ms */
    INT32   uFrameBytes;          /* 单帧字节数 */
    INT32   uBitRate;             /* 比特率, bps */
    BOOL    bLittleEndian;        /* 是否小端序 */
    BYTE    byRes[128];           /* 保留字段 */
} NET_VoiceComAudioCfg_S;

typedef NET_VoiceComAudioCfg_S* pNET_VoiceComAudioCfg_S;

/**
 * @struct tagNET_TalkbackStreamInfo
 * @brief 对讲流信息结构体
 * @note 对应命令：NET_TO_STREAM_TALKBACK / NET_FROM_STREAM_TALKBACK
 */
typedef struct tagNET_TalkbackStreamInfo
{
    CHAR    szHost[NET_LEN_64];      /* 对讲服务器主机地址/IP */
    INT32   nPort;                      /* 对讲服务端口号 */
    INT32   nChnId;                     /* 通道 ID，对应对讲通道 */
    INT32   nUserID;                    /* 用户 ID，标识操作会话 */
    BOOL    bMainStream;                /* 主码流标记 (true/false) */
    CHAR    szProtocol[NET_LEN_32]; /* 传输协议类型 (如 rtp/raw 等) */
    CHAR    szStartTime[NET_LEN_64];/* 对讲开始时间戳 */
    CHAR    szEndTime[NET_LEN_64];    /* 对讲结束时间戳 */
    CHAR    szFileName[NET_LEN_260]; /* 对讲音频文件名/路径 */
    BYTE    byRes[128];                 /* 保留字段，未使用 */
} NET_TalkbackStreamInfo_S;

typedef NET_TalkbackStreamInfo_S* pNET_TalkbackStreamInfo_S;

/**
 * @struct tagNET_ReplayTalkbackInfo
 * @brief 对讲回放信息结构体
 * @note 对应命令：NET_REPLAY_TALKBACK
 */
typedef struct tagNET_ReplayTalkbackInfo
{
    CHAR    szNvrIp[NET_LEN_64];     /* NVR 网络摄像机 IP 地址 */
    CHAR    szRemoteIp[NET_LEN_64];  /* 远端设备 IP 地址 */
    NET_TalkbackStreamInfo_S stIPCInfo;  /* IPC 端对讲流详细信息 */
    BYTE    byRes[128];                 /* 保留字段，未使用 */
} NET_ReplayTalkbackInfo_S;

typedef NET_ReplayTalkbackInfo_S* pNET_ReplayTalkbackInfo_S;

/**
 * @struct tagNETTVExposureInfo
 * @brief 曝光信息结构体
 * @attention
 */
typedef struct tagNET_ExposureInfo
{
    INT32               enExpTime;          /* ISP::ExpTimeMode_E */
    BOOL                bAntiBanding;       /* TRUE/FALSE */
    BYTE                byRes[64];
} NET_ExposureInfo_S;

typedef NET_ExposureInfo_S* pNET_ExposureInfo_S;

/**
 * @struct tagNETTVDayNightInfo
 * @brief 日夜切换信息结构体
 * @attention
 */
typedef struct tagNET_DayNightInfo
{
    INT32               enDayNightMode;     /* ISP::DayNightMode_E */
    INT32               nBeginHour;
    INT32               nBeginMinute;
    INT32               nBeginSecond;
    INT32               nBeginMilliSec;
    INT32               nEndHour;
    INT32               nEndMinute;
    INT32               nEndSecond;
    INT32               nEndMilliSec;
    UINT32              nSensitivityLevel;
    UINT32              nFilterTime;
    BOOL                bFillLightExp;
    INT32               enLightMode;        /* ISP::LightBrightMode_E */
    INT32               enLightType;        /* ISP::LightType_E */
    BOOL                bWhiteLightEnable;
    INT32               nWhiteLightLevel;
    BOOL                bRedLightEnable;
    INT32               nRedLightLevel;
    BYTE                byRes[64];
} NET_DayNightInfo_S;

typedef NET_DayNightInfo_S* pNET_DayNightInfo_S;

/**
 * @struct tagNETTVBackLightInfo
 * @brief 背光信息结构体
 * @attention
 */
typedef struct tagNET_BackLightInfo
{
    INT32               enBackLightArea;    /* ISP::BackLightArea_E */
    BOOL                bWdrEnable;
    INT32               nWdrLevel;
    BOOL                bHlsEnable;
    INT32               nHlsLevel;
    BYTE                byRes[64];
} NET_BackLightInfo_S;

typedef NET_BackLightInfo_S* pNET_BackLightInfo_S;

/**
 * @struct tagNETTVDenoiseInfo
 * @brief 降噪信息结构体
 * @attention
 */
typedef struct tagNET_DenoiseInfo
{
    INT32               enDnrMode;          /* ISP::DnrMode_E */
    UINT32              nDnrLevel;
    UINT32              nSnrLevel;
    UINT32              nTnrLevel;
    BYTE                byRes[64];
} NET_DenoiseInfo_S;

typedef NET_DenoiseInfo_S* pNET_DenoiseInfo_S;

/**
 * @struct tagNETTVWhiteBalanceInfo
 * @brief 白平衡信息结构体
 * @attention
 */
typedef struct tagNET_WhiteBalanceInfo
{
    INT32               enAwbMode;          /* ISP::AwbMode_E */
    UINT32              nRGain;
    UINT32              nBGain;
    BYTE                byRes[64];
} NET_WhiteBalanceInfo_S;

typedef NET_WhiteBalanceInfo_S* pNET_WhiteBalanceInfo_S;

/**
 * @struct tagNETTVUpgradeInfo
 * @brief 升级文件信息
 * @attention
 */
typedef struct tagNET_UpgradeInfo
{
    CHAR                szUpgradePath[NET_FILE_NAME_LEN];
    BYTE                byRes[64];
} NET_UpgradeInfo_S;

typedef NET_UpgradeInfo_S* pNET_UpgradeInfo_S;

/**
 * @struct tagNETTVUpgradeStatus
 * @brief 系统升级状态
 * @attention
 */
typedef struct tagNET_UpgradeStatus
{
    INT32               nUpgradeStatus;
    BYTE                byRes[64];
} NET_UpgradeStatus_S;

typedef NET_UpgradeStatus_S* pNET_UpgradeStatus_S;

/**
 * @struct tagNETTVUpgradeVersion
 * @brief 升级文件版本
 * @attention
 */
typedef struct tagNET_UpgradeVersion
{
    CHAR                szVersion[NET_LEN_64];
    BYTE                byRes[64];
} NET_UpgradeVersion_S;

typedef NET_UpgradeVersion_S* pNET_UpgradeVersion_S;

/**
 * @struct tagNETTVCaptureTime
 * @brief 抓图时间
 * @attention
 */
typedef struct tagNET_CaptureTime
{
    INT32               nStartTime;
    INT32               nEndTime;
    BYTE                byRes[32];
} NET_CaptureTime_S;

typedef NET_CaptureTime_S* pNET_CaptureTime_S;

/**
 * @struct tagNETTVCaptureDaySchedule
 * @brief 抓图日程
 * @attention
 */
typedef struct tagNET_CaptureDaySchedule
{
    INT32               nDayOfWeek; /* 1~7: Monday~Sunday */
    UINT32              udwTimeCount;
    NET_CaptureTime_S astTimes[NET_PLAN_TIME_SECTION_NUM_ADAY];
    BYTE                byRes[64];
} NET_CaptureDaySchedule_S;

typedef NET_CaptureDaySchedule_S* pNET_CaptureDaySchedule_S;

/**
 * @struct tagNETTVCapturePlanInfo
 * @brief 抓图计划信息
 * @attention
 */
typedef struct tagNET_CapturePlanInfo
{
    NET_CaptureDaySchedule_S astDaySchedules[NET_PLAN_DAY_NUM_AWEEK];
    BYTE                byRes[256];
} NET_CapturePlanInfo_S;

typedef NET_CapturePlanInfo_S* pNET_CapturePlanInfo_S;

/**
 * @struct tagNET_CaptureConfig
 * @brief 抓图配置
 * @attention
 */
typedef struct tagNET_CaptureConfig
{
    BOOL                bEnable;
    INT32               enPictureFormat; /* NET_CAPTURE_PICTURE_FORMAT_E */
    INT32               nWidth;
    INT32               nHeight;
    INT32               enImageQuality;  /* NET_CAPTURE_IMAGE_QUALITY_E */
    UINT32              unInterval;
    INT32               enTimeUnit;      /* NET_CAPTURE_TIME_UNIT_E */
    UINT32              unNumber;
    BYTE                byRes[64];
} NET_CaptureConfig_S;

typedef NET_CaptureConfig_S* pNET_CaptureConfig_S;

/**
 * @struct tagNET_CaptureParamInfo
 * @brief 抓图参数信息
 * @attention
 */
typedef struct tagNET_CaptureParamInfo
{
    NET_CaptureConfig_S stCaptureTimingConfig;
    NET_CaptureConfig_S stCaptureEventConfig;
    BYTE                byRes[128];
} NET_CaptureParamInfo_S;

typedef NET_CaptureParamInfo_S* pNET_CaptureParamInfo_S;

typedef struct tagNET_CaptureInfo
{
    INT32               nChnId;
    INT32               enType;
    CHAR                szStartTime[NET_MAX_DATE_STRING_LEN];
    CHAR                szEndTime[NET_MAX_DATE_STRING_LEN];
    CHAR                szImagePath[NET_FILE_NAME_LEN];
    INT32               nImageSize;
    BYTE                byRes[128];
} NET_CaptureInfo_S;

typedef NET_CaptureInfo_S* pNET_CaptureInfo_S;

/**
 * @struct tagstNETTVLoginInfo
 * @brief 设备登录信息
 * @attention
 */
typedef struct tagNET_DeviceLoginInfo
{
    CHAR    szIPAddr[NET_LEN_260];       /* IP地址/域名 */
    INT32   uPort;                         /* 端口号 */
    CHAR    szUserName[NET_LEN_132];     /* 用户名 */
    CHAR    szPassword[NET_LEN_128];     /* 密码 */
    BYTE    byRes[256];                     /* 保留字段 */
}NET_DeviceLoginInfo_S;

typedef NET_DeviceLoginInfo_S* pNET_DeviceLoginInfo_S;

/**
 * @brief 设备规模信息结构体（NVR侧专用）
 * @note  NVR规模/能力数量信息：设备类型、报警输入/输出端口数、通道数。
 *        此为NVR偏向的硬件资源计量，非全设备通用，归 BU_SJCL/NVR 侧
 *        （回调见 NetTVNvrDeviceCb.c）。区别于通用设备基本信息 NET_DeviceBasicInfo_S。
 */
typedef struct tagNET_DeviceInfo
{
    INT32   uDevType;                           /* 设备类型,参见枚举#NET_DEVICE_TYPE_E */
    INT32   uAlarmInPortNum;                    /* 报警输入个数 */
    INT32   uAlarmOutPortNum;                   /* 报警输出个数 */
    INT32   uChannelNum;                        /* 通道个数 */
    BYTE    byReserved[48];                     /* 预留字段 */
} NET_DeviceInfo_S;

/**
 * @brief 设备规模信息结构体指针类型
 */
typedef NET_DeviceInfo_S* pNET_DeviceInfo_S;

/**
 * @brief 设备基本信息结构体（通用身份信息 + 通用运行状态）
 * @note  全设备通用，通用设备身份属性：型号、序列号、固件版本、MAC地址、设备名称、厂商等
 *        全设备通用，收口于 Common 设备回调（NetTVDeviceCb.c），
 *        对应 NET_GET/SET_DEVICECFG。身份字段只读，仅 strDeviceName 可写。
 */
typedef struct tagNET_DeviceBasicInfo
{
    /* ========== 身份信息（只读，strDeviceName 可写） ========== */
    CHAR    strDevModel[NET_LEN_64];                /* 设备型号(只读) */
    CHAR    strSerialNum[NET_LEN_64];               /* 硬件序列号(只读) */
    CHAR    strFirmwareVersion[NET_LEN_64];         /* 软件版本号(只读) */
    CHAR    strWebVersion[NET_LEN_64];              /* Web版本/版本号(只读) */
    CHAR    strMacAddress[NET_LEN_64];              /* IPv4的Mac地址(只读) */
    CHAR    strDeviceName[NET_LEN_64];              /* 设备名称(可写) */
    CHAR    strManufacturer[NET_LEN_64];            /* 厂商信息(只读) */
    CHAR    strDeviceTypeV2[NET_LEN_128];           /* 设备类型(只读) */

    /* ========== 通用运行状态（只读） ========== */
    FLOAT   fCPULoadRatio;                          /* CPU负载率(只读) */
    FLOAT   fMemoryUsage;                           /* 内存使用率(只读) */
    INT32   nBootTime;                              /* 启动时间/运行时长-秒(只读) */

    BYTE    byReserved[220];                        /* 预留字段 */
} NET_DeviceBasicInfo_S;

/**
 * @brief 设备基本信息结构体指针类型
 */
typedef NET_DeviceBasicInfo_S* pNET_DeviceBasicInfo_S;

/**
 * @brief 设备存储信息结构体（NVR/录播等有硬盘的设备专用）
 * @note  只有具备存储能力的设备才需要实现此结构体回调。
 *        编码器、矩阵等无存储设备返回不支持即可。
 */
typedef struct tagNET_DeviceStorageInfo
{
    INT32   nHardDiskCount;                         /* 硬盘个数 */
    INT32   nHardDiskStatus;                        /* 硬盘状态 */
    CHAR    strDiskTotal[NET_LEN_32];               /* 硬盘总容量 */
    CHAR    strDiskAvailable[NET_LEN_32];           /* 硬盘可用空间 */
    CHAR    strDiskUsedSpace[NET_LEN_32];           /* 硬盘已用空间 */
    CHAR    strDiskFileType[NET_LEN_32];            /* 硬盘文件系统类型 */
    BYTE    byReserved[128];                        /* 预留字段 */
} NET_DeviceStorageInfo_S;

/**
 * @brief 设备存储信息结构体指针类型
 */
typedef NET_DeviceStorageInfo_S* pNET_DeviceStorageInfo_S;

/**
 * @brief 激活时长类型
 */
typedef enum tagNET_ActivationTime
{
    NET_AT_ONE_WEEK    = 0,   /* 一周   */
    NET_AT_ONE_MONTH   = 1,   /* 一月   */
    NET_AT_TWO_MONTH   = 2,   /* 两月   */
    NET_AT_THREE_MONTH = 3,   /* 三月   */
    NET_AT_HALF_YEAR   = 4,   /* 半年   */
    NET_AT_FOREVER     = 5,   /* 永久   */
    NET_AT_NULL        = -1,  /* 未注册/激活 */
} NET_ActivationTime_E;

/**
 * @brief 注册信息结构体
 * @note  包含机器码、注册码、注册时间、可用时长及激活类型。
 *        对应 NET_GET_REGISTERINFO / NET_SET_REGISTERINFO。
 */
typedef struct tagNET_RegisterInfo
{
    CHAR   strMachinSn[NET_LEN_64];      /* 机器码     */
    CHAR   strRegisterEg[NET_LEN_64];    /* 注册码     */
    CHAR   strStartTime[NET_LEN_64];     /* 注册时间   */
    INT64  nUsableTimer;                  /* 可用时长（分钟） */
    NET_ActivationTime_E enActionTime;   /* 激活类型   */
    BYTE   byReserved[32];               /* 保留字段   */
} NET_RegisterInfo_S;

/**
 * @brief 注册信息结构体指针类型
 */
typedef NET_RegisterInfo_S* pNET_RegisterInfo_S;
/**
 * @brief 系统时间/NTP校时配置结构体
 * @note  对应IPC侧System::TimeInfo_S，用于NET_GET_NTPCFG/NET_SET_NTPCFG
 */
typedef struct tagNET_SystemNtpInfo
{
    INT32   enTimeZone;                                 /* 时区，取值对应IPC侧System::TimeZone_E */
    INT32   enDateFormat;                               /* 日期格式，取值对应IPC侧System::DateFormat_E */
    BOOL    bEnableNTPSync;                             /* 是否开启NTP校时 */
    BOOL    bManualSync;                                /* 是否手动校时 */
    CHAR    strDateTime[NET_MAX_DATE_STRING_LEN];    /* 手动校时时间，格式按enDateFormat解析 */
    BOOL    bIsSyncWithComputer;                        /* 是否与计算机时间同步 */
    CHAR    strAddress[NET_LEN_128];                 /* NTP服务器地址 */
    INT32   nPort;                                      /* NTP服务器端口 */
    INT32   nSyncInterval;                              /* NTP同步间隔，单位分钟 */
    BYTE    byReserved[128];                            /* 预留字段 */
} NET_SystemNtpInfo_S;

/**
 * @brief 系统时间/NTP校时配置结构体指针类型
 */
typedef NET_SystemNtpInfo_S* pNET_SystemNtpInfo_S;

/**
 * @brief 修改用户密码参数结构体
 * @note  用于 NET_serverRegisterSetUserPasswordCb 回调
 */
typedef struct tagNET_UserPasswordInfo
{
    CHAR    strUserName[NET_USERNAME_LEN];          /* 用户名 */
    CHAR    strOldPassword[NET_PASSWORD_LEN];       /* 旧密码 */
    CHAR    strNewPassword[NET_PASSWORD_LEN];       /* 新密码 */
    BYTE    byReserved[128];                           /* 预留字段 */
} NET_UserPasswordInfo_S;

/**
 * @brief 修改用户密码参数结构体指针类型
 */
typedef NET_UserPasswordInfo_S* pNET_UserPasswordInfo_S;

/**
 * @struct tagNETTVPageInfo
 * @brief 分页信息
 */
typedef struct tagNET_PageInfo
{
    INT32   nCurPage;
    INT32   nPageSize;
    INT32   nDataTotal;
    INT32   nPageTotal;
    BYTE    byRes[64];
} NET_PageInfo_S;

typedef NET_PageInfo_S* pNET_PageInfo_S;

/**
 * @struct tagNETTVLoginLockInfo
 * @brief 登录锁定信息
 */
typedef struct tagNET_LoginLockInfo
{
    BOOL    bIllegalLoginEnable;
    INT32   nCheckInterval;
    INT32   nMaxErrorTimes;
    INT32   nLockDuration;
    BYTE    byRes[64];
} NET_LoginLockInfo_S;

typedef NET_LoginLockInfo_S* pNET_LoginLockInfo_S;

/**
 * @struct tagNETTVPwdPolicyInfo
 * @brief 密码策略信息
 */
typedef struct tagNET_PwdPolicyInfo
{
    BOOL    bPwdSecurityLevelEnable;
    BOOL    bAllowLowLevelPwdLogin;
    BYTE    byRes[64];
} NET_PwdPolicyInfo_S;

typedef NET_PwdPolicyInfo_S* pNET_PwdPolicyInfo_S;

/**
 * @struct tagNETTVSshAdminInfo
 * @brief SSH管理信息
 */
typedef struct tagNET_SshAdminInfo
{
    BOOL    bSshEnable;
    INT32   nSshPort;
    CHAR    szSshStartTime[NET_LEN_64];
    CHAR    szSshCountdown[NET_LEN_64];
    BYTE    byRes[64];
} NET_SshAdminInfo_S;

typedef NET_SshAdminInfo_S* pNET_SshAdminInfo_S;

/**
 * @struct tagNETTVSecurityServicesInfo
 * @brief 安全服务配置
 */
typedef struct tagNET_SecurityServicesInfo
{
    NET_LoginLockInfo_S stLoginLock;
    NET_PwdPolicyInfo_S stPwdPolicy;
    NET_SshAdminInfo_S stSshAdmin;
    BYTE    byRes[128];
} NET_SecurityServicesInfo_S;

typedef NET_SecurityServicesInfo_S* pNET_SecurityServicesInfo_S;

/**
 * @struct tagNETTVSshCountdownInfo
 * @brief SSH倒计时信息
 */
typedef struct tagNET_SshCountdownInfo
{
    CHAR    szCountdown[NET_LEN_64];
    BYTE    byRes[64];
} NET_SshCountdownInfo_S;

typedef NET_SshCountdownInfo_S* pNET_SshCountdownInfo_S;

/**
 * @struct tagNETTVLogServerInfo
 * @brief 日志服务器配置
 */
typedef struct tagNET_LogServerInfo
{
    BOOL    bEnable;
    BOOL    bEnSsl;
    CHAR    szServerAddr[NET_LEN_256];
    INT32   nPort;
    BYTE    byRes[128];
} NET_LogServerInfo_S;

typedef NET_LogServerInfo_S* pNET_LogServerInfo_S;

/**
 * @struct tagNETTVLogRetrievalCond
 * @brief 日志检索条件
 */
typedef struct tagNET_LogRetrievalCond
{
    INT32   nType;
    INT32   nAction;
    CHAR    szStartTime[NET_LEN_64];
    CHAR    szEndTime[NET_LEN_64];
    BYTE    byRes[64];
} NET_LogRetrievalCond_S;

typedef NET_LogRetrievalCond_S* pNET_LogRetrievalCond_S;

/**
 * @struct tagNETTVLogInfo
 * @brief 日志信息
 */
typedef struct tagNET_LogInfo
{
    CHAR    szStartTime[NET_LEN_64];
    INT32   nType;
    INT32   nAction;
    CHAR    szChnName[NET_LEN_64];
    CHAR    szUser[NET_LEN_64];
    CHAR    szHost[NET_LEN_64];
    CHAR    szContext[NET_LEN_512];
    BYTE    byRes[64];
} NET_LogInfo_S;

typedef NET_LogInfo_S* pNET_LogInfo_S;

/**
 * @struct tagNETTVLogList
 * @brief 日志查询结果列表
 */
typedef struct tagNET_LogList
{
    NET_LogRetrievalCond_S stCond;
    NET_PageInfo_S stPage;
    INT32   nLogCount;
    NET_LogInfo_S astLogs[NET_LOG_QUERY_COND_NUM];
    BYTE    byRes[128];
} NET_LogList_S;

typedef NET_LogList_S* pNET_LogList_S;

/**
 * @struct tagNETTVRecordInfo
 * @brief 手动录像控制信息
 */
typedef struct tagNET_RecordInfo
{
    INT32   nChnId;
    INT32   nVideoStatus;
    INT32   nAudioStatus;
    INT32   nRecordStatus;                      /* NET_RECORD_STATUS_E */
    INT32   nRecordFormat;
    INT32   nEventType;
    CHAR    szPath[NET_MAX_URL_LEN];
    CHAR    szRedunPath[NET_MAX_URL_LEN];
    CHAR    szRecordName[NET_FILE_NAME_LEN];
    CHAR    szRecordTime[NET_LEN_64];
    INT32   nStreamType;
    BYTE    byRes[128];
} NET_RecordInfo_S;

typedef NET_RecordInfo_S* pNET_RecordInfo_S;

/**
 * @struct tagNETTVRecordStatusInfo
 * @brief 录像状态信息
 */
typedef struct tagNET_RecordStatusInfo
{
    INT32   nStatus;                            /* NET_RECORD_STATUS_E */
    BYTE    byRes[64];
} NET_RecordStatusInfo_S;

typedef NET_RecordStatusInfo_S* pNET_RecordStatusInfo_S;

/**
 * @brief SD 卡物理状态码。
 */
typedef enum tagNETSdCardStatusCode
{
    NET_SD_CARD_STATUS_WRITE_ERROR  = -1,  /* SD 卡写入失败。 */
    NET_SD_CARD_STATUS_INITIALIZING = 0,   /* SD 卡正在初始化。 */
    NET_SD_CARD_STATUS_UNPLUGGED    = 1,   /* 未检测到 SD 卡。 */
    NET_SD_CARD_STATUS_FORMATTING   = 2,   /* SD 卡正在格式化。 */
    NET_SD_CARD_STATUS_NORMAL       = 3    /* SD 卡可正常使用。 */
} NET_SdCardStatus_EN;

#define NET_SD_CARD_STATUS_TEXT_LEN     32
#define NET_SD_CARD_STATUS_RESERVED_LEN 64

/**
 * @brief SD 卡物理状态信息。
 * @note 用于 NET_GET_SD_CARD_STATUS。
 */
typedef struct tagNETSdCardStatusInfo
{
    /* NET_SdCardStatus_EN 枚举值。 */
    INT32 nStatus;
    /* 可读的状态文本。 */
    CHAR strStatusText[NET_SD_CARD_STATUS_TEXT_LEN];
    /* SD 卡可用时为 TRUE。 */
    BOOL bReady;
    /* 预留给后续 SDK 扩展。 */
    BYTE abyReserved[NET_SD_CARD_STATUS_RESERVED_LEN];
} NET_SdCardStatus_S, *pNET_SdCardStatus_S;

/**
 * @struct tagNETTVRecordTime
 * @brief 录像计划时间段
 */
typedef struct tagNET_RecordTime
{
    INT32   nType;                              /* 1:定时录像, 2:事件录像 */
    INT32   nStartTime;                         /* 秒 */
    INT32   nEndTime;                           /* 秒 */
    BYTE    byRes[32];
} NET_RecordTime_S;

typedef NET_RecordTime_S* pNET_RecordTime_S;

/**
 * @struct tagNETTVRecordDaySchedule
 * @brief 单日录像计划
 */
typedef struct tagNET_RecordDaySchedule
{
    INT32   nDayOfWeek;                         /* 1:周一 ... 7:周日 */
    INT32   nRecordTimeCount;
    NET_RecordTime_S astRecordTimes[NET_TIME_DURATION_NUM];
    BYTE    byRes[64];
} NET_RecordDaySchedule_S;

typedef NET_RecordDaySchedule_S* pNET_RecordDaySchedule_S;

/**
 * @struct tagNETTVRecordSchedule
 * @brief 录像计划
 */
typedef struct tagNET_RecordSchedule
{
    BOOL    bEnable;
    INT32   nDayScheduleCount;
    NET_RecordDaySchedule_S astDaySchedules[NET_PLAN_DAY_NUM_AWEEK];
    BYTE    byRes[128];
} NET_RecordSchedule_S;

typedef NET_RecordSchedule_S* pNET_RecordSchedule_S;

/**
 * @struct tagNETTVRecordAdvancedParam
 * @brief 录像高级参数
 */
typedef struct tagNET_RecordAdvancedParam
{
    BOOL    bLoopWrite;
    INT32   nPreTime;
    INT32   nDelayTime;
    INT32   nStreamType;
    BYTE    byRes[128];
} NET_RecordAdvancedParam_S;

typedef NET_RecordAdvancedParam_S* pNET_RecordAdvancedParam_S;

/**
 * @struct tagNETTVRecordFindCond
 * @brief 录像文件查找条件
 */
typedef struct tagNET_RecordFindCond
{
    INT32   nChnId;
    INT32   nType;
    CHAR    szYear[NET_LEN_16];
    CHAR    szMonth[NET_LEN_16];
    CHAR    szDate[NET_LEN_64];
    CHAR    szStartTime[NET_LEN_64];
    CHAR    szEndTime[NET_LEN_64];
    CHAR    szFilename[NET_FILE_NAME_LEN];
    BYTE    byRes[128];
} NET_RecordFindCond_S;

typedef NET_RecordFindCond_S* pNET_RecordFindCond_S;

/**
 * @struct tagNETTVRecordVideoTime
 * @brief 指定日期录像时间段
 */
typedef struct tagNET_RecordVideoTime
{
    INT32   nStartTime;
    INT32   nEndTime;
    BYTE    byRes[32];
} NET_RecordVideoTime_S;

typedef NET_RecordVideoTime_S* pNET_RecordVideoTime_S;

/**
 * @struct tagNETTVRecordFindResult
 * @brief 录像查找结果项
 */
typedef struct tagNET_RecordFindResult
{
    INT32   nChnId;
    INT32   nDateCount;
    CHAR    aszDates[NET_RECORD_DATE_MAX_NUM][NET_LEN_64];
    CHAR    szFilename[NET_FILE_NAME_LEN];
    INT32   nVideoTimeCount;
    NET_RecordVideoTime_S astVideoTimes[NET_TIME_DURATION_NUM];
    BYTE    byRes[128];
} NET_RecordFindResult_S;

typedef NET_RecordFindResult_S* pNET_RecordFindResult_S;

/**
 * @struct tagNETTVRecordFileList
 * @brief 录像查找条件与结果
 */
typedef struct tagNET_RecordFileList
{
    NET_RecordFindCond_S stFind;
    INT32   nResultCount;
    NET_RecordFindResult_S astResults[NET_RECORD_FILE_MAX_NUM];
    BYTE    byRes[128];
} NET_RecordFileList_S;

typedef NET_RecordFileList_S* pNET_RecordFileList_S;

/**
 * @struct tagNETTVRecordDownloadInfo
 * @brief 录像下载任务
 */
typedef struct tagNET_RecordDownloadInfo
{
    INT32   nChnId;
    CHAR    szPath[NET_MAX_URL_LEN];
    CHAR    szStartTime[NET_LEN_64];
    CHAR    szEndTime[NET_LEN_64];
    BYTE    byRes[128];
} NET_RecordDownloadInfo_S;

typedef NET_RecordDownloadInfo_S* pNET_RecordDownloadInfo_S;

/**
 * @struct tagNETTVRecordDownloadProgress
 * @brief 录像下载进度
 */
typedef struct tagNET_RecordDownloadProgress
{
    CHAR    szFilename[NET_FILE_NAME_LEN];
    INT32   nProgress;
    BYTE    byRes[64];
} NET_RecordDownloadProgress_S;

typedef NET_RecordDownloadProgress_S* pNET_RecordDownloadProgress_S;

/**
 * @struct tagNETTVRecordDownloadList
 * @brief 录像下载任务列表
 */
typedef struct tagNET_RecordDownloadList
{
    INT32   nDownloadCount;
    NET_RecordDownloadInfo_S astDownloads[NET_RECORD_DOWNLOAD_MAX_NUM];
    INT32   nProgressCount;
    NET_RecordDownloadProgress_S astProgress[NET_RECORD_DOWNLOAD_MAX_NUM];
    BYTE    byRes[128];
} NET_RecordDownloadList_S;

typedef NET_RecordDownloadList_S* pNET_RecordDownloadList_S;

/**
 * @struct tagNETTVAudioCfg
 * @brief 音频配置参数
 * @note 对应 Audio_NS::AudioConfig_S
 */
typedef struct tagNET_AudioCfg
{
    BOOL    bAudioSwitch;                        /* 音频开关 */
    INT32   enInputType;                         /* 输入类型 Audio_NS::AudioInputType_E */
    INT32   enFormat;                            /* 音频格式 Audio_NS::AudioFormat_E */
    INT32   enSampRate;                          /* 采样率 Audio_NS::AudioSamprate_E */
    INT32   enBitRate;                           /* 码率 Audio_NS::AudioBitrate_E */
    UINT32  u32InputVolume;                      /* 输入音量 */
    BOOL    bDenoise;                            /* 降噪开关 */
    INT32   enOutputType;                        /* 输出类型 Audio_NS::AudioOutputType_E */
    UINT32  u32OutputVolume;                     /* 输出音量 */
    BYTE    byRes[128];                          /* 保留字段 */
} NET_AudioCfg_S;

typedef NET_AudioCfg_S* pNET_AudioCfg_S;

/**
 * @struct tagNETTVNetworkInterfaces
 * @brief 网络配置信息 结构体定义 Network configuration information
 * @attention 无 None
 */
typedef struct tagNET_NetworkCfg
{
    INT32   uMTU;                              /* MTU值  MTU value */
    BOOL    bIPv4DHCP;                          /* IPv4的DHCP  DHCP of IPv4 */
    CHAR    szIpv4Address[NET_LEN_32];       /* IPv4的IP地址  IP address of IPv4 */
    CHAR    szIPv4GateWay[NET_LEN_32];       /* IPv4的网关地址  Gateway of IPv4 */
    CHAR    szIPv4SubnetMask[NET_LEN_32];    /* IPv4的子网掩码  Subnet mask of IPv4 */
    BYTE    byRes[480];                         /* 保留字段  Reserved */
}NET_NetworkCfg_S;

typedef NET_NetworkCfg_S* pNET_NetworkCfg_S;

/**
 * @brief 未登录场景下通过 SDK 设备发现组播协议设置摄像机网络参数。
 */
typedef struct tagNET_PoeNetworkConfig
{
    CHAR    szInterfaceIP[NET_IPADDR_STR_MAX_LEN];
    CHAR    szMACAddress[NET_LEN_32];
    CHAR    szTargetIP[NET_IPADDR_STR_MAX_LEN];
    CHAR    szSubnetMask[NET_IPADDR_STR_MAX_LEN];
    CHAR    szGateway[NET_IPADDR_STR_MAX_LEN];
    BOOL    bSetGateway;
    BOOL    bIPv4DHCP;
    UINT32  dwTimeoutMs;
    UINT32  dwSendCount;
    BYTE    byRes[128];
} NET_PoeNetworkConfig_S;

typedef NET_PoeNetworkConfig_S* pNET_PoeNetworkConfig_S;

#ifndef NET_MAX_NET_NUM
#define NET_MAX_NET_NUM 8
#endif

/**
 * @struct tagNET_NetworkCfgList
 * @brief 多网口配置列表  Multiple network interface configuration list
 * @note 支持最多 NET_MAX_NET_NUM 个网口，每个网口独立配置 MTU/DHCP/IP/Gateway/SubnetMask
 */
typedef struct tagNET_NetworkCfgList
{
    UINT32          uNetworkCount;                                /* 实际网口数量  Actual network interface count */
    NET_NetworkCfg_S stNets[NET_MAX_NET_NUM];                    /* 网口数组  Network interface array */
    BYTE            byRes[256];                                   /* 保留字段  Reserved */
}NET_NetworkCfgList_S;

typedef NET_NetworkCfgList_S* pNET_NetworkCfgList_S;

/**
 * @struct tagNETTVRtspUrlInfo
 * @brief RTSP流地址信息  RTSP URL information
 * @note
 * - szRtspUrl 由设备端生成并返回（可包含鉴权信息或 token，按项目约定）
 * - dwStreamIndex 参考 #NET_LIVE_STREAM_INDEX_E（主/辅/第三流等）
 */
typedef struct tagNET_RtspUrlInfo
{
    INT32   uChannel;                           /* 通道号  Channel ID */
    INT32   uStreamIndex;                       /* 码流索引  Stream index */
    CHAR    szRtspUrl[NET_LEN_260];           /* RTSP URL (e.g. rtsp://ip:port/...) */
    BYTE    byRes[256];                          /* 保留字段  Reserved */
}NET_RtspUrlInfo_S;

typedef NET_RtspUrlInfo_S* pNET_RtspUrlInfo_S;

/**
 * @struct tagNETTVReplayUrlInfo
 * @brief 回放播放地址信息  Playback URL information
 * @note
 * - 调用方填充通道和起止时间。
 * - 设备端/服务端申请回放后填充 szUrl。
 */
typedef struct tagNET_ReplayUrlInfo
{
    INT32   uChannel;                           /* 通道号 Channel ID */
    CHAR    szStartTime[NET_LEN_64];          /* 开始时间 "YYYY-MM-DD HH:MM:SS" */
    CHAR    szEndTime[NET_LEN_64];            /* 结束时间 "YYYY-MM-DD HH:MM:SS" */
    CHAR    szUrl[NET_MAX_URL_LEN];           /* 回放播放地址 */
    BYTE    byRes[256];                          /* 保留字段 Reserved */
}NET_ReplayUrlInfo_S;

typedef NET_ReplayUrlInfo_S* pNET_ReplayUrlInfo_S;

/**
 * @struct tagNETTVReplayCtrlInfo
 * @brief 回放控制信息
 * @note
 * - `dwCtrlType=NET_REPLAY_CTRL_START` 时，调用方填写通道/起止时间，服务端返回 `szSessionId` 与 `szUrl`
 * - `dwCtrlType=NET_REPLAY_CTRL_STOP` / `NET_REPLAY_CTRL_PAUSE` / `NET_REPLAY_CTRL_SET_SPEED` / `NET_REPLAY_CTRL_SET_SEEK` 时，优先使用 `szSessionId` 标识会话
 * - `fSpeed` 仅在 `NET_REPLAY_CTRL_SET_SPEED` 时有效，例如 0.5 / 1.0 / 2.0 / 4.0
 * - `nSeekTime` 仅在 `NET_REPLAY_CTRL_SET_SEEK` 时有效，表示回放时间轴上的跳转秒数
 * - `nReplayType` 表示平台点播回放控制类型，参见 `NET_REPLAY_PLATFORM_CTRL_TYPE_E`
 */
typedef struct tagNET_ReplayCtrlInfo
{
    INT32   uChannel;                                   /* 通道号 Channel ID */
    INT32   uCtrlType;                                  /* 控制类型，参见 NET_REPLAY_CTRL_CMD_E */
    FLOAT   fSpeed;                                      /* 播放倍速 */
    INT32   nSeekTime;                                   /* 跳转播放时间，单位秒 */
    INT32   nReplayType;                                 /* 平台点播回放控制类型 */
    CHAR    szSessionId[NET_REPLAY_SESSION_ID_LEN];   /* 回放会话ID */
    CHAR    szStartTime[NET_LEN_64];                  /* 开始时间 "YYYY-MM-DD HH:MM:SS" */
    CHAR    szEndTime[NET_LEN_64];                    /* 结束时间 "YYYY-MM-DD HH:MM:SS" */
    CHAR    szUrl[NET_MAX_URL_LEN];                   /* 当前回放播放地址 */
    BYTE    byRes[128];                                  /* 保留字段 Reserved */
} NET_ReplayCtrlInfo_S;

typedef NET_ReplayCtrlInfo_S* pNET_ReplayCtrlInfo_S;

/**
 * @struct tagNETTVReplayRecordTime
 * @brief 单个录像时间段
 */
typedef struct tagNET_ReplayRecordTime
{
    INT32   nStartTime;                          /* 开始秒数，按当天 00:00:00 起算 */
    INT32   nEndTime;                            /* 结束秒数，按当天 00:00:00 起算 */
    BYTE    byRes[16];                           /* 保留字段 */
} NET_ReplayRecordTime_S;

typedef NET_ReplayRecordTime_S* pNET_ReplayRecordTime_S;

/**
 * @struct tagNETTVReplayRecordList
 * @brief NVR回放录像时间段查询结果
 * @note
 * - 调用方至少填写 `dwChannel`
 * - 兼容旧方式：仅填写 `szDate`
 * - 新方式：可额外填写 `bFilterByEventType`、`dwEventType`、`szStartTime`、`szEndTime`
 * - 返回普通录像、人员事件、车辆事件、其他事件四类时间段
 */
typedef struct tagNET_ReplayRecordList
{
    INT32   uChannel;                                                   /* 通道号 Channel ID */
    BOOL    bFilterByEventType;                                          /* 是否按事件类型过滤 FALSE:不过滤 TRUE:按 uEventType 过滤 */
    INT32   uEventType;                                                 /* 事件类型，取值可对齐 NVR 事件类型定义；兼容旧请求时忽略 */
    CHAR    szDate[NET_LEN_32];                                       /* 日期 "YYYY-MM-DD" */
    CHAR    szStartTime[NET_MAX_DATE_STRING_LEN];                     /* 开始时间 "YYYY-MM-DD HH:MM:SS"，为空则按整天开始 */
    CHAR    szEndTime[NET_MAX_DATE_STRING_LEN];                       /* 结束时间 "YYYY-MM-DD HH:MM:SS"，为空则按整天结束 */
    INT32   nVideoCount;                                                 /* 普通录像时间段数量 */
    NET_ReplayRecordTime_S astVideoTimes[NET_REPLAY_RECORD_SEGMENT_MAX];
    INT32   nPersonEventCount;                                           /* 人员事件时间段数量 */
    NET_ReplayRecordTime_S astPersonEventTimes[NET_REPLAY_RECORD_SEGMENT_MAX];
    INT32   nVehicleEventCount;                                          /* 车辆事件时间段数量 */
    NET_ReplayRecordTime_S astVehicleEventTimes[NET_REPLAY_RECORD_SEGMENT_MAX];
    INT32   nOtherEventCount;                                            /* 其他事件时间段数量 */
    NET_ReplayRecordTime_S astOtherEventTimes[NET_REPLAY_RECORD_SEGMENT_MAX];
    BYTE    byRes[64];                                                   /* 保留字段 */
} NET_ReplayRecordList_S;

typedef NET_ReplayRecordList_S* pNET_ReplayRecordList_S;

/**
 * @struct tagNETTVPreviewRtspUrlInfo
 * @brief 预览RTSP地址信息 Preview RTSP URL information
 */
typedef struct tagNET_PreviewRtspUrl
{
    CHAR    szRtspMainUrl[NET_MAX_URL_LEN];   /* 主码流RTSP地址 */
    CHAR    szRtspSubUrl[NET_MAX_URL_LEN];    /* 子码流RTSP地址 */
    BYTE    byRes[64];                           /* 保留字段 */
}NET_PreviewRtspUrl_S;

typedef NET_PreviewRtspUrl_S* pNET_PreviewRtspUrl_S;

/**
 *  @struct tagNETTVWifiStaCfg
 *  @brief WIFI STA基础配置
 */
typedef struct tagNET_WifiStaCfg
{
    BOOL    bEnableWifi;                        /* 是否开启WiFi */
    BOOL    bEnableBoost;                       /* 是否开启增强功能 */
    BYTE    byRes[64];                          /* 保留字段 */
} NET_WifiStaCfg_S;

typedef NET_WifiStaCfg_S* pNET_WifiStaCfg_S;

/**
 *  @struct tagNETTVWifiWepKey
 *  @brief WIFI WEP 密码项
 */
typedef struct tagNET_WifiWepKey
{
    INT32   nIndex;                             /* 密钥索引 1-4 */
    CHAR    szValue[NET_LEN_132];            /* 密钥内容 */
    BYTE    byRes[32];                          /* 保留字段 */
} NET_WifiWepKey_S;

typedef NET_WifiWepKey_S* pNET_WifiWepKey_S;


/**
 * @struct tagNETTVWifiStaConnect
 * @brief WIFI STA连接参数
 */
typedef struct tagNET_WifiStaConnect
{
    CHAR    szSsid[NET_NAME_MAX_LEN];        /* SSID */
    INT32   nSecurityMode;                      /* NET_WIFI_SECURITY_MODE_E */
    CHAR    szIpAddress[NET_IPADDR_STR_MAX_LEN];
    CHAR    szPassword[NET_LEN_132];
    CHAR    szPairwise[NET_LEN_32];          /* TKIP/CCMP */
    INT32   nWepKeyLen;                         /* 64/128 */
    BOOL    bWepIsHex;                          /* true=hex,false=ascii */
    CHAR    szAuthAlg[NET_LEN_32];           /* OPEN/SHARED */
    INT32   nWepKeyCount;
    NET_WifiWepKey_S astWepKeys[4];

    CHAR    szEapIdentity[NET_LEN_132];
    CHAR    szEapPassword[NET_LEN_132];
    CHAR    szPeapVersion[NET_LEN_16];
    CHAR    szPhase2[NET_LEN_64];
    CHAR    szAnonymousIdentity[NET_LEN_132];
    CHAR    szCaCertPath[NET_LEN_260];
    CHAR    szPeapLabel[NET_LEN_32];

    CHAR    szTlsIdentity[NET_LEN_132];
    CHAR    szPrivateKeyPasswd[NET_LEN_132];
    CHAR    szEapolVersion[NET_LEN_16];
    CHAR    szClientCertPath[NET_LEN_260];
    CHAR    szPrivateKeyPath[NET_LEN_260];
    CHAR    szCtrlInterface[NET_LEN_260];
    CHAR    szInterfaceName[NET_LEN_64];
    BYTE    byRes[256];
} NET_WifiStaConnect_S;

typedef NET_WifiStaConnect_S* pNET_WifiStaConnect_S;

/**
 * @struct tagNETTV4GInfo
 * @brief 4G配置
 */
typedef struct tagNET_4GInfo
{
    CHAR    szApn[NET_LEN_64];
    CHAR    szUserName[NET_LEN_132];
    CHAR    szPassword[NET_LEN_132];
    CHAR    szCallNumber[NET_LEN_32];
    INT32   nMtu;
    INT32   nAuthMode;                          /* 0:None,1:PAP,2:CHAP,3:PAP&CHAP */
    INT32   nNetworkMode;                       /* 0:Auto,1:4G,2:3G,3:2G */
    INT32   nDialMode;                          /* 0:Auto,1:Manual */
    BYTE    byRes[128];
} NET_4GInfo_S;

typedef NET_4GInfo_S* pNET_4GInfo_S;

/**
 * @struct tagNETTVHotspotInfo
 * @brief 热点配置
 */
typedef struct tagNET_HotspotInfo
{
    BOOL    bEnabled;
    CHAR    szSsid[NET_NAME_MAX_LEN];
    CHAR    szSecurityMode[NET_LEN_64];
    CHAR    szEncryptionType[NET_LEN_64];
    CHAR    szPassword[NET_LEN_132];
    CHAR    szConfirmPassword[NET_LEN_132];
    BYTE    byRes[128];
} NET_HotspotInfo_S;

typedef NET_HotspotInfo_S* pNET_HotspotInfo_S;

/**
 * @struct tagNETTVHotspotConnDevice
 * @brief 热点连接设备项
 */
typedef struct tagNET_HotspotConnDevice
{
    INT32   nIndex;
    CHAR    szMac[NET_LEN_64];
    CHAR    szIp[NET_IPADDR_STR_MAX_LEN];
    CHAR    szConnTime[NET_LEN_64];
} NET_HotspotConnDevice_S;

typedef NET_HotspotConnDevice_S* pNET_HotspotConnDevice_S;

/**
 * @struct tagNETTVHotspotConnInfo
 * @brief 热点连接设备列表
 */
typedef struct tagNET_HotspotConnInfo
{
    CHAR    szStatus[NET_LEN_32];
    INT32   nTotal;
    INT32   nDeviceCount;
    NET_HotspotConnDevice_S astDevices[NET_HOTSPOT_CONN_MAX_NUM];
} NET_HotspotConnInfo_S;

typedef NET_HotspotConnInfo_S* pNET_HotspotConnInfo_S;


/**
 * @struct tagNETTVPreviewImageParam
 * @brief 预览图像参数 Preview image parameters
 */
typedef struct tagNET_PreviewImageParam
{
    INT32   nBrightness;                         /* 亮度 [0,100] */
    INT32   nContrast;                           /* 对比度 [0,100] */
    INT32   nSaturation;                         /* 饱和度 [0,100] */
    INT32   nSharpness;                          /* 锐度 [0,100] */
    BYTE    byRes[64];                           /* 保留字段 */
}NET_PreviewImageParam_S;

typedef NET_PreviewImageParam_S* pNET_PreviewImageParam_S;

/**
 * @brief 图像配置参数 Image setting parameters
 * @note 用于 NET_GET_IMAGECFG / NET_SET_IMAGECFG，对应 IPC ISP::ImageParam_S
 */
typedef struct tagNET_ImageSetting
{
    UINT32  nBrightness;                         /* 亮度 [0,100] */
    UINT32  nContrast;                           /* 对比度 [0,100] */
    UINT32  nSaturation;                         /* 饱和度 [0,100] */
    UINT32  nSharpness;                          /* 锐度 [0,100] */
    BYTE    byRes[64];                           /* 保留字段 */
}NET_ImageSetting_S;

typedef NET_ImageSetting_S* pNET_ImageSetting_S;

/**
 * @struct tagNETTVPreviewInfo
 * @brief 预览信息 Preview information
 * @note 用于NET_GET_PREVIEW_INFO/NET_SET_PREVIEW_INFO
 */
typedef struct tagNET_PreviewInfo
{
    NET_PreviewRtspUrl_S stRtspUrl;         /* 预览流地址 */
    NET_PreviewImageParam_S stImageParam;   /* 图像参数 */
    BYTE    byRes[256];                          /* 保留字段 */
}NET_PreviewInfo_S;

typedef NET_PreviewInfo_S* pNET_PreviewInfo_S;

/**
 * @struct tagNETTVChannelInfo
 * @brief 通用通道信息 Channel information
 * @note 可用于单通道查询，也可作为通道列表项。
 */
typedef struct tagNET_ChannelInfo
{
    UINT32  uChannel;                           /* 通道号 */
    BYTE    byEnable;                            /* 是否启用 */
    BYTE    byOnline;                            /* 是否在线 */
    BYTE    byStreamType;                        /* 默认码流：0主码流 1子码流 */
    BYTE    byHasRecord;                         /* 是否有录像 */
    INT32   nRecordStatus;                       /* 当前录制状态 NET_RECORD_STATUS_E */


    INT32   nDevState;                           /* 设备状态 */
    INT32   nAppProto;                           /* 接入协议 */
    INT32   nTransProto;                         /* 传输协议 */
    INT32   nMfrsType;                           /* 厂商类型 */

    INT32   nCtrlPort;                           /* 管理端口 */
    INT32   nReserved[3];                        /* 保留字段 */

    CHAR    szChannelName[NET_LEN_64];        /* 通道名称 */
    CHAR    szDevName[NET_LEN_64];            /* 设备名称 */
    CHAR    szDevType[NET_LEN_64];            /* 设备类型 */
    CHAR    szSerialNum[NET_LEN_64];          /* 序列号 */

    CHAR    szFirmwareVersion[NET_LEN_64];    /* 固件版本 */
    CHAR    szDeviceIP[NET_IPADDR_STR_MAX_LEN]; /* IP地址 */
    CHAR    szMac[NET_LEN_64];                /* MAC地址 */
    CHAR    szSubnetMask[NET_IPADDR_STR_MAX_LEN]; /* 子网掩码 */

    CHAR    szMfrsName[NET_LEN_64];           /* 厂商名称 */
    CHAR    szAppProtoName[NET_LEN_64];       /* 协议名称 */
    CHAR    szOnvifDeviceUrl[NET_MAX_URL_LEN]; /* ONVIF URL */

    CHAR    szPreviewMainUrl[NET_MAX_URL_LEN]; /* 主码流预览地址 */
    CHAR    szPreviewSubUrl[NET_MAX_URL_LEN];  /* 子码流预览地址 */
    CHAR    szRtspMainUrl[NET_MAX_URL_LEN];    /* 直连主码流地址 */
    CHAR    szRtspSubUrl[NET_MAX_URL_LEN];     /* 直连子码流地址 */

    BYTE    byRes[512];                          /* 保留字段 */
}NET_ChannelInfo_S;

typedef NET_ChannelInfo_S* pNET_ChannelInfo_S;

#ifndef NET_MAX_CHANNEL_NUM
#define NET_MAX_CHANNEL_NUM 128
#endif

typedef struct tagNET_ChannelList
{
    UINT32  uChannelCount;                      /* 实际通道数量 */
    NET_ChannelInfo_S stChannels[NET_MAX_CHANNEL_NUM];
    BYTE    byRes[512];
}NET_ChannelList_S;

typedef NET_ChannelList_S* pNET_ChannelList_S;

typedef NET_ChannelInfo_S NET_DirectConnectChanInfo_S, *pNET_DirectConnectChanInfo_S;

/**
 * @struct tagNETTVRevTimeout
 * @brief 超时时间 结构体定义  Timeout structure definition
 * @attention
*/
typedef struct tagNET_RevTimeout
{
    INT32   uRevTimeOut;                 /* 设置接收超时时间 Set timeout for receiving */
    INT32   uFileReportTimeOut;          /* 设置文件操作超时时间 Set timeout for file operation */
    BYTE    byRes[128];                   /* 保留字段  Reserved */
}NET_RevTimeout_S;

typedef NET_RevTimeout_S* pNET_RevTimeout_S;

/**
 * @struct tagNET_AlarmBasicInfo
 * @brief 基础/常规报警 (0x1000 - 0x10FF)
 */
typedef struct tagNET_AlarmBasicInfo
{
    UINT32      uAlarmType;                                     /* 报警类型 */
    UINT32      uAlarmInputNumber;                              /* 报警输入口 */
    BYTE        byAlarmOutputNumber[NET_MAX_ALARM_OUT_NUM];  /* 触发的报警输出口，为1表示触发该口 */
    BYTE        byAlarmRelateChannel[NET_MAX_ALARM_IN_NUM];  /* 触发的录像通道，为1表示触发该通道 */
    BYTE        byChannel[NET_MAX_ALARM_IN_NUM];             /* 报警通道，为1表示触发该通道 */
    BYTE        byDiskNumber[NET_LOCAL_DISK_MAX_NUM];        /* 发生报警的硬盘，为1表示该硬盘异常 */
    BYTE        byPanoramaImg[NET_PIC_DATA_MAX_LEN];         /* 全景 JPEG 二进制图片 */
    UINT32      uPanoramaImgLen;                                /* 全景 JPEG 图片长度 */
    INT64       llTimestampMs;                                  /* 报警时间戳，单位毫秒 */
    BYTE        byRes[128];                                     /* 保留字段 */
} NET_AlarmBasicInfo_S;

/**
 * @brief 基础/常规报警结构体指针类型
 */
typedef NET_AlarmBasicInfo_S* pNET_AlarmBasicInfo_S;

/**
 * @struct tagNET_AlarmRuleInfo
 * @brief 区域/周界规则报警 (0x2000 - 0x20FF)
 * @note 约定：uAlarmType 填写命令码(如 NET_ALARM_LINE_CROSSING)
 */
typedef struct tagNET_AlarmRuleInfo
{
    UINT32      uAlarmType;                          /* 报警类型/命令码 */
    UINT32      uChannel;                            /* 通道号 */
    UINT32      uRuleID;                             /* 规则ID */
    UINT32      uRuleType;                           /* 规则类型(可与uAlarmType对应) */
    CHAR        strRuleName[NET_LEN_64];          /* 规则名称(可选) */
    UINT32      uTargetID;                           /* 目标ID(可选) */
    UINT32      uObjectType;                         /* 目标类型(0:未知 1:人 2:车 ... 可扩展) */
    float       fConfidence;                         /* 置信度 0~1 */
    INT32       nLeft;                               /* 目标框 left */
    INT32       nTop;                                /* 目标框 top */
    INT32       nRight;                              /* 目标框 right */
    INT32       nBottom;                             /* 目标框 bottom */
    BYTE        byPanoramaImg[NET_PIC_DATA_MAX_LEN]; /* 全景 JPEG 二进制图片 */
    UINT32      uPanoramaImgLen;                     /* 全景 JPEG 图片长度 */
    BYTE        byTargetImg[NET_PIC_DATA_MAX_LEN]; /* 目标特写 JPEG 二进制图片 */
    UINT32      uTargetImgLen;                       /* 目标特写 JPEG 图片长度 */
    INT64       llTimestampMs;                       /* 报警时间戳，单位毫秒 */
    BYTE        byRes[128];                          /* 保留字段 */
} NET_AlarmRuleInfo_S;

/**
 * @brief 区域/周界规则报警结构体指针类型
 */
typedef NET_AlarmRuleInfo_S* pNET_AlarmRuleInfo_S;

/**
 * @struct tagNET_AlarmAiObjectInfo
 * @brief Smart/AI 行为分析 (0x3000 - 0x3FFF)
 * @note 约定：uAlarmType 填写命令码(如 NET_ALARM_FACE_CAPTURE)
 */
typedef struct tagNET_AlarmAiObjectInfo
{
    UINT32      uAlarmType;                          /* 报警类型/命令码 */
    UINT32      uChannel;                            /* 通道号 */
    UINT32      uObjectType;                         /* 目标类型(0:未知 1:人 2:车 ... 可扩展) */
    float       fConfidence;                         /* 置信度 0~1 */
    INT32       nLeft;                               /* 目标框 left */
    INT32       nTop;                                /* 目标框 top */
    INT32       nRight;                              /* 目标框 right */
    INT32       nBottom;                             /* 目标框 bottom */
    CHAR        strObjectID[NET_LEN_64];          /* 目标ID(可选) */
    BYTE        byPanoramaImg[NET_PIC_DATA_MAX_LEN]; /* 全景 JPEG 二进制图片 */
    UINT32      uPanoramaImgLen;                     /* 全景 JPEG 图片长度 */
    BYTE        byImgData[NET_PIC_DATA_MAX_LEN];  /* 报警图片数据 */
    UINT32      uImgLen;                             /* 图片长度 */
    INT64       llTimestampMs;                       /* 报警时间戳，单位毫秒 */
    BYTE        byRes[32];                           /* 保留字段 */
} NET_AlarmAiObjectInfo_S;

/**
 * @brief Smart/AI 行为分析结构体指针类型
 */
typedef NET_AlarmAiObjectInfo_S* pNET_AlarmAiObjectInfo_S;

/**
 * @struct tagNET_AlarmFaceCompareInfo
 * @brief 人脸比对结果告警
 * @note 约定：uAlarmType 填写 NET_ALARM_FACE_COMPARE
 */
typedef struct tagNET_AlarmFaceCompareInfo
{
    UINT32      uAlarmType;                          /* 报警类型/命令码 */
    UINT32      uChannel;                            /* 通道号 */
    INT64       llTimestampMs;                       /* 报警时间戳，单位毫秒 */
    INT32       nEventId;                            /* 事件ID */
    INT32       nCompResult;                         /* 比对结果：0-不匹配 1-匹配 */
    INT32       nSimilarity;                         /* 相似度 0-100 */
    INT32       nFaceId;                             /* 人脸ID */
    CHAR        strFaceLibName[NET_FACE_DB_NAME_LEN];      /* 目标库名称 */
    CHAR        strFaceName[NET_FACE_MEMBER_NAME_LEN];     /* 人脸名称 */
    CHAR        strLibFacePath[NET_LEN_260];      /* 目标库人脸图片路径 */
    CHAR        strCapFacePath[NET_LEN_260];      /* 抓拍人脸图片路径 */
    CHAR        strCapImagePath[NET_LEN_260];     /* 抓拍原图路径 */
    BYTE        byLibFaceImg[NET_FACE_IMAGE_MAX_LEN];     /* 目标库人脸 JPEG 二进制图片 */
    UINT32      uLibFaceImgLen;                      /* 目标库人脸 JPEG 图片长度 */
    BYTE        byCapFaceImg[NET_FACE_IMAGE_MAX_LEN];     /* 抓拍人脸 JPEG 二进制图片 */
    UINT32      uCapFaceImgLen;                      /* 抓拍人脸 JPEG 图片长度 */
    BYTE        byRes[256];                          /* 保留字段 */
} NET_AlarmFaceCompareInfo_S;

/**
 * @brief 人脸比对结果告警结构体指针类型
 */
typedef NET_AlarmFaceCompareInfo_S* pNET_AlarmFaceCompareInfo_S;


/**
 * @struct tagNET_AlarmPlateInfo
 * @brief 交通/车辆相关 (0x4000 - 0x40FF)
 * @note 约定：uAlarmType 填写命令码(如 NET_ALARM_PLATE_RECOGNITION)
 */
typedef struct tagNET_AlarmPlateInfo
{
    UINT32      uAlarmType;                          /* 报警类型/命令码 */
    UINT32      uChannel;                            /* 通道号 */
    CHAR        strPlateNumber[NET_LEN_32];       /* 车牌号 */
    UINT32      uPlateColor;                         /* 车牌颜色(枚举可扩展) */
    UINT32      uVehicleType;                        /* 车辆类型(枚举可扩展) */
    float       fConfidence;                         /* 置信度 0~1 */
    UINT32      uSpeed;                              /* 速度(km/h，可选) */
    UINT32      uLaneNo;                             /* 车道号(可选) */
    BYTE        byPlateImg[NET_VEH_PLATE_IMAGE_LEN]; /* 车牌图片 */
    UINT32      uPlateImgLen;                        /* 车牌图片长度 */
    BYTE        byRes[64];                           /* 保留字段 */
} NET_AlarmPlateInfo_S;

/**
 * @brief 交通/车辆相关结构体指针类型
 */
typedef NET_AlarmPlateInfo_S* pNET_AlarmPlateInfo_S;

/**
 * @struct tagNET_AlarmExceptionInfo
 * @brief 设备异常/状态事件 (0x5000 - 0x50FF)
 * @note 约定：uAlarmType 填写命令码(如 NET_ALARM_DISK_FULL)
 */
typedef struct tagNET_AlarmExceptionInfo
{
    UINT32      uAlarmType;                          /* 报警类型/命令码 */
    UINT32      uChannel;                            /* 通道号(若无则填0) */
    UINT32      uDiskNo;                             /* 硬盘号(若无则填0) */
    UINT32      uStatus;                             /* 状态(0:恢复 1:触发) */
    BYTE        byRes[256];                          /* 保留字段 */
} NET_AlarmExceptionInfo_S;

/**
 * @brief 设备异常/状态事件结构体指针类型
 */
typedef NET_AlarmExceptionInfo_S* pNET_AlarmExceptionInfo_S;

/**
 * @struct tagNET_AlarmStatisticsTarget
 * @brief 统计类告警目标快照
 */
typedef struct tagNET_AlarmStatisticsTarget
{
    INT32       nTrackID;                             /* 目标跟踪 ID */
    UINT32      uRuleID;                              /* 规则 ID */
    UINT32      uSnapshotType;                        /* 快照类型：进入/离开/区域当前目标 */
    INT32       nLeft;                                /* 目标框 left */
    INT32       nTop;                                 /* 目标框 top */
    INT32       nRight;                               /* 目标框 right */
    INT32       nBottom;                              /* 目标框 bottom */
    INT64       llTimestampMs;                        /* 快照时间戳，单位毫秒 */
    INT32       nDirection;                           /* 目标方向，跨线类事件填业务方向枚举值 */
    BYTE        byImgData[NET_PIC_DATA_MAX_LEN];   /* 目标图片数据 */
    UINT32      uImgLen;                              /* 目标图片长度 */
    BYTE        byRes[64];                            /* 保留字段 */
} NET_AlarmStatisticsTarget_S;

/**
 * @brief 统计类告警目标快照结构体指针类型
 */
typedef NET_AlarmStatisticsTarget_S* pNET_AlarmStatisticsTarget_S;

/**
 * @struct tagNET_AlarmStatisticsInfo
 * @brief 统计类通用告警，优先用于人流统计和人员密度统计
 */
typedef struct tagNET_AlarmStatisticsInfo
{
    UINT32      uAlarmType;                           /* 报警类型/命令码 */
    UINT32      uChannel;                             /* 通道号 */
    UINT32      uStatisticsType;                      /* 统计子类型 NET_StatisticsType_E */
    UINT32      uRuleID;                              /* 规则 ID */
    INT64       llTimestampMs;                        /* 报告时间戳，单位毫秒 */
    UINT32      uReportSeq;                           /* 统计报告序号 */
    UINT32      uEnterCount;                          /* 累计进入人数 */
    UINT32      uLeaveCount;                          /* 累计离开人数 */
    UINT32      uTotalCount;                          /* 累计通行总人数 */
    UINT32      uCurrentPeopleCount;                  /* 当前区域人数 */
    UINT32      uAverageStayTimeSec;                  /* 平均停留时间，单位秒 */
    UINT32      uTargetCount;                         /* 当前目标快照数量 */
    NET_AlarmStatisticsTarget_S stTargets[NET_ALARM_STATISTICS_TARGET_MAX_NUM]; /* 目标快照列表 */
    BYTE        byPanoramaImg[NET_PIC_DATA_MAX_LEN]; /* 全景 JPEG 二进制图片 */
    UINT32      uPanoramaImgLen;                      /* 全景 JPEG 图片长度 */
    BYTE        byRes[256];                           /* 保留字段 */
} NET_AlarmStatisticsInfo_S;

/**
 * @brief 统计类通用告警结构体指针类型
 */
typedef NET_AlarmStatisticsInfo_S* pNET_AlarmStatisticsInfo_S;

/* ==================== 通用抓拍结构体 ==================== */

#define NET_CAPTURE_CROP_MAX_NUM 8 /* 单次抓拍最多裁剪小图数量 */

/* 专用抓拍兼容结构的字段长度，保持与历史 SDK 一致。 */
#define NET_CAPTURE_REGION_POINT_MAX_NUM    10
#define NET_CAPTURE_TIMESTAMP_MAX_LEN       64
#define NET_CAPTURE_VEHICLE_BRAND_MAX_LEN   128
#define NET_CAPTURE_LICENSE_PLATE_MAX_LEN   64

/**
 * @struct tagNET_ImageBuffer
 * @brief 图片二进制数据缓冲区，指针 + 长度组合，用于存储图片数据。
 * @note  调用方负责 pData 指向内存的分配与释放，结构体本身不持有所有权。
 */
typedef struct tagNET_ImageBuffer
{
    BYTE *pData;     /* 图片二进制数据指针 */
    UINT32 uDataLen; /* 图片数据长度，单位字节 */
} NET_ImageBuffer_S;

/**
 * @struct tagNET_CropImage
 * @brief 从全景图中裁剪出的小图及其坐标信息
 */
typedef struct tagNET_CropImage
{
    UINT32 uCropX;             /* 小图在大图上的起始 X 坐标 */
    UINT32 uCropY;             /* 小图在大图上的起始 Y 坐标 */
    UINT32 uCropWidth;         /* 小图宽度 */
    UINT32 uCropHeight;        /* 小图高度 */
    UINT32 uTargetType;        /* 目标类型（人/车/非机动车等） */
    FLOAT fConfidence;         /* 检测置信度 [0.0, 1.0] */
    INT32 nTrackID;            /* 目标跟踪 ID，无则填 -1 */
    NET_ImageBuffer_S stImage; /* 裁剪小图二进制数据 */
    BYTE byRes[32];            /* 保留字段 */
} NET_CropImage_S;

/** @brief 专用抓拍目标区域多边形，兼容旧抓拍推送接口。 */
typedef struct tagNET_CapturePolygon
{
    UINT32 uPointCount;
    FLOAT afPointX[NET_CAPTURE_REGION_POINT_MAX_NUM];
    FLOAT afPointY[NET_CAPTURE_REGION_POINT_MAX_NUM];
    BYTE abyReserved[32];
} NET_CapturePolygon_S;

typedef NET_CapturePolygon_S* pNET_CapturePolygon_S;

/** @brief 统一抓拍事件的专用属性，承载原人脸、行人和车辆抓拍元数据。 */
typedef struct tagNET_CaptureExtraInfo
{
    BOOL bMale;
    INT32 nAgeLabel;
    BOOL bGlasses;
    BOOL bBeard;
    BOOL bMask;
    INT32 nEmotionLabel;
    BOOL bBag;
    INT32 nTopColorLabel;
    INT32 nBottomColorLabel;
    INT32 nVehicleType;
    INT32 nVehicleColor;
    CHAR strVehicleBrand[NET_CAPTURE_VEHICLE_BRAND_MAX_LEN];
    CHAR strLicensePlateNumber[NET_CAPTURE_LICENSE_PLATE_MAX_LEN];
    CHAR strTimestamp[NET_CAPTURE_TIMESTAMP_MAX_LEN];
    NET_CapturePolygon_S stTargetRegion;
    BYTE byRes[64];
} NET_CaptureExtraInfo_S;

typedef NET_CaptureExtraInfo_S* pNET_CaptureExtraInfo_S;

/**
 * @struct tagNET_AlarmCaptureInfo
 * @brief 通用抓拍告警信息，包含一张全景图及其裁剪出的小图列表
 */
typedef struct tagNET_AlarmCaptureInfo
{
    UINT32 uAlarmType;                                      /* 报警类型/命令码 */
    UINT32 uChannel;                                        /* 通道号 */
    UINT32 uCaptureType;                                    /* 抓拍类型 NET_CaptureType_E */
    INT64 llTimestampMs;                                    /* 抓拍时间戳，单位毫秒 */
    UINT32 uPanoramaWidth;                                  /* 全景图宽度 */
    UINT32 uPanoramaHeight;                                 /* 全景图高度 */
    NET_ImageBuffer_S stPanoramaImg;                        /* 全景 JPEG 二进制图片数据 */
    UINT32 uCropCount;                                      /* 裁剪小图数量 */
    NET_CropImage_S stCropImages[NET_CAPTURE_CROP_MAX_NUM]; /* 裁剪小图数组 */
    NET_CaptureExtraInfo_S stExtraInfo;                      /* 人脸/行人/车辆专用属性 */
    BYTE byRes[128];                                        /* 保留字段 */
} NET_AlarmCaptureInfo_S;

/**
 * @brief 通用抓拍告警结构体指针类型
 */
typedef NET_AlarmCaptureInfo_S* pNET_AlarmCaptureInfo_S;

/** @brief 人脸抓拍专用输入结构，兼容已有调用方；服务端会转为统一抓拍告警。 */
typedef struct tagNET_FaceCapturePushInfo
{
    BOOL bMale;
    INT32 nAgeLabel;
    BOOL bGlasses;
    BOOL bBeard;
    BOOL bMask;
    INT32 nEmotionLabel;
    NET_CapturePolygon_S stFaceRegion;
    UINT32 uFaceImgLen;
    BYTE byFaceImg[NET_PIC_DATA_MAX_LEN];
    UINT32 uPanoramaImgLen;
    BYTE byPanoramaImg[NET_PIC_DATA_MAX_LEN];
    CHAR strTimestamp[NET_CAPTURE_TIMESTAMP_MAX_LEN];
    BYTE abyReserved[64];
} NET_FaceCapturePushInfo_S;

typedef NET_FaceCapturePushInfo_S* pNET_FaceCapturePushInfo_S;

/** @brief 行人抓拍专用输入结构，兼容已有调用方。 */
typedef struct tagNET_PersonCapturePushInfo
{
    BOOL bMale;
    INT32 nAgeLabel;
    BOOL bBag;
    INT32 nTopColorLabel;
    INT32 nBottomColorLabel;
    UINT32 uPersonImgLen;
    BYTE byPersonImg[NET_PIC_DATA_MAX_LEN];
    UINT32 uPanoramaImgLen;
    BYTE byPanoramaImg[NET_PIC_DATA_MAX_LEN];
    CHAR strTimestamp[NET_CAPTURE_TIMESTAMP_MAX_LEN];
    BYTE abyReserved[64];
} NET_PersonCapturePushInfo_S;

typedef NET_PersonCapturePushInfo_S* pNET_PersonCapturePushInfo_S;

/** @brief 机动车抓拍专用输入结构，兼容已有调用方。 */
typedef struct tagNET_MotorvehicleCapturePushInfo
{
    CHAR strVehicleBrand[NET_CAPTURE_VEHICLE_BRAND_MAX_LEN];
    INT32 nVehicleType;
    INT32 nVehicleColor;
    CHAR strLicensePlateNumber[NET_CAPTURE_LICENSE_PLATE_MAX_LEN];
    UINT32 uTargetImgLen;
    BYTE byTargetImg[NET_PIC_DATA_MAX_LEN];
    UINT32 uPanoramaImgLen;
    BYTE byPanoramaImg[NET_PIC_DATA_MAX_LEN];
    CHAR strTimestamp[NET_CAPTURE_TIMESTAMP_MAX_LEN];
    BYTE abyReserved[64];
} NET_MotorvehicleCapturePushInfo_S;

typedef NET_MotorvehicleCapturePushInfo_S* pNET_MotorvehicleCapturePushInfo_S;

/** @brief 非机动车抓拍专用输入结构，兼容已有调用方。 */
typedef struct tagNET_NonMotorvehicleCapturePushInfo
{
    INT32 nVehicleType;
    INT32 nVehicleColor;
    UINT32 uTargetImgLen;
    BYTE byTargetImg[NET_PIC_DATA_MAX_LEN];
    UINT32 uPanoramaImgLen;
    BYTE byPanoramaImg[NET_PIC_DATA_MAX_LEN];
    CHAR strTimestamp[NET_CAPTURE_TIMESTAMP_MAX_LEN];
    BYTE abyReserved[64];
} NET_NonMotorvehicleCapturePushInfo_S;

typedef NET_NonMotorvehicleCapturePushInfo_S* pNET_NonMotorvehicleCapturePushInfo_S;

/* ==================== 布防时间和联动相关结构体 ==================== */

/**
 * @struct tagNETTVSchedTime
 * @brief 时间段结构体 Schedule time structure
 * @note 用于布防时间配置
 */
typedef struct tagNET_SchedTime
{
    INT32       nStartHour;                          /* 开始小时 [0-23] */
    INT32       nStartMinute;                       /* 开始分钟 [0-59] */
    INT32       nEndHour;                           /* 结束小时 [0-23] */
    INT32       nEndMinute;                         /* 结束分钟 [0-59] */
    BYTE        byRes[16];                          /* 保留字段 */
}NET_SchedTime_S;

typedef NET_SchedTime_S* pNET_SchedTime_S;

/**
 * @struct tagNET_AlarmSchedule
 * @brief 布防时间配置 Alarm schedule configuration
 * @note 一周7天，每天最多8个时间段
 */
typedef struct tagNET_AlarmSchedule
{
    INT32       uTimeSectionCount[7];                /* 每天的时间段数量 [0-8] */
    NET_SchedTime_S astTimeSection[7][NET_PLAN_SECTION_NUM]; /* 一周7天，每天最多8个时间段 */
    BYTE        byRes[64];                          /* 保留字段 */
} NET_AlarmSchedule_S;

/**
 * @brief 布防时间配置结构体指针类型
 */
typedef NET_AlarmSchedule_S* pNET_AlarmSchedule_S;

/**
 * @struct tagNET_LinkageList
 * @brief 联动配置列表 Linkage configuration list
 */
typedef struct tagNET_LinkageList
{
    INT32       uAlarmOutputCount;                   /* 报警输出数量 */
    INT32       auAlarmOutput[NET_MAX_ALARM_OUT_NUM]; /* 报警输出通道号数组 */
    INT32       uRecordChannelCount;                 /* 录像通道数量 */
    INT32       auRecordChannel[NET_CHANNEL_MAX]; /* 录像通道号数组 */
    INT32       uSnapshotChannelCount;               /* 抓拍通道数量 */
    INT32       auSnapshotChannel[NET_CHANNEL_MAX]; /* 抓拍通道号数组 */
    BYTE        byRes[256];                         /* 保留字段 */
} NET_LinkageList_S;

/*
 * 描述：声音报警选择的音源类型。
 */
typedef enum tagNETAudibleAlarmSoundType
{
    /* 使用默认警示音。 */
    NET_AUDIBLE_ALARM_SOUND_TYPE_WARNING = 0,
    /* 使用内置警报音。 */
    NET_AUDIBLE_ALARM_SOUND_TYPE_ALERT   = 1,
    /* 使用自定义音频。 */
    NET_AUDIBLE_ALARM_SOUND_TYPE_CUSTOM  = 2
} NET_AudibleAlarmSoundType_EN;

/*
 * 描述：内置声音报警音频的标识。
 */
typedef enum tagNETAudibleAlarmAlertSound
{
    /* 请立即离开警戒区域。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_WARNING_ZONE_LEAVE_IMMEDIATELY = 0,
    /* 危险区域，请勿靠近。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_DANGER_ZONE_DO_NOT_APPROACH   = 1,
    /* 禁止停车区域。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_NO_PARKING_ZONE               = 2,
    /* 已进入监控区域。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_ENTERING_SURVEILLANCE_ZONE    = 3,
    /* 欢迎光临。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_WELCOME_GREETING              = 4,
    /* 请勿触摸贵重物品。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_DO_NOT_TOUCH_VALUABLES        = 5,
    /* 私人区域，禁止入内。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_PRIVATE_PROPERTY_NO_ENTRY     = 6,
    /* 深水危险。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_DEEP_WATER_WARNING            = 7,
    /* 高处危险。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_HIGH_PLACE_DANGER             = 8,
    /* 尖叫报警音。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_SHRIEK_ALARM                  = 9,
    /* 通用警示音。 */
    NET_AUDIBLE_ALARM_ALERT_SOUND_GENERAL_WARNING_TONE          = 10
} NET_AudibleAlarmAlertSound_EN;

/*
 * 描述：报警输出通道的控制状态。
 */
typedef enum tagNETAlarmOutputState
{
    /* 关闭报警输出。 */
    NET_ALARM_OUTPUT_STATE_OFF       = 0,
    /* 开启报警输出。 */
    NET_ALARM_OUTPUT_STATE_ON        = 1,
    /* 人工关闭报警输出。 */
    NET_ALARM_OUTPUT_STATE_HUMAN_OFF = 2,
    /* 人工开启报警输出。 */
    NET_ALARM_OUTPUT_STATE_HUMAN_ON  = 3
} NET_AlarmOutputState_EN;

/*
 * 描述：闪光报警灯的闪烁频率。
 */
typedef enum tagNETFlashingLightFrequency
{
    /* 持续常亮。 */
    NET_FLASHING_LIGHT_FREQUENCY_STEADY_ON = 0,
    /* 低频闪烁。 */
    NET_FLASHING_LIGHT_FREQUENCY_LOW       = 1,
    /* 中频闪烁。 */
    NET_FLASHING_LIGHT_FREQUENCY_MIDDLE    = 2,
    /* 高频闪烁。 */
    NET_FLASHING_LIGHT_FREQUENCY_HIGH      = 3
} NET_FlashingLightFrequency_EN;

/*
 * 描述：声音报警使用的一条自定义音频信息。
 */
typedef struct tagNETAudibleAlarmCustomAudio
{
    /* 是否选中当前自定义音频。 */
    BOOL bSelected;
    /* 自定义音频名称。 */
    CHAR strName[NET_ALARM_CUSTOM_AUDIO_NAME_LEN];
    /* 自定义音频文件路径。 */
    CHAR strPath[NET_ALARM_CUSTOM_AUDIO_PATH_LEN];
    /* 保留字段，调用方应置零。 */
    BYTE abyReserved[NET_ALARM_CONFIG_RESERVED_LEN];
} NET_AudibleAlarmCustomAudio_S, *pNET_AudibleAlarmCustomAudio_S;

/*
 * 描述：声音报警输出配置。
 */
typedef struct tagNETAudibleAlarmInfo
{
    /* 音源类型，取值见 NET_AudibleAlarmSoundType_EN。 */
    INT32 enSoundType;
    /* 内置警报音标识，取值见 NET_AudibleAlarmAlertSound_EN。 */
    INT32 enAlertSound;
    /* 声音重复播放次数。 */
    INT32 nTimes;
    /* 自定义音频数量。 */
    INT32 nCustomAudioCount;
    /* 自定义音频列表。 */
    NET_AudibleAlarmCustomAudio_S astCustomAudios[NET_AUDIBLE_ALARM_CUSTOM_AUDIO_MAX_NUM];
    /* 声音报警的布防时间表。 */
    NET_AlarmSchedule_S stAlarmSchedule;
    /* 保留字段，调用方应置零。 */
    BYTE abyReserved[NET_ALARM_CONFIG_RESERVED_LEN];
} NET_AudibleAlarmInfo_S, *pNET_AudibleAlarmInfo_S;

/*
 * 描述：单个报警输入通道的配置。
 */
typedef struct tagNETAlarmInputInfo
{
    /* 报警输入通道编号。 */
    INT32 nAlarmNumber;
    /* 报警输入通道地址。 */
    CHAR strAlarmAddress[NET_ALARM_ADDRESS_LEN];
    /* 报警输入通道名称。 */
    CHAR strAlarmName[NET_ALARM_NAME_LEN];
    /* 是否采用常开接线方式。 */
    BOOL bNormallyOpen;
    /* 报警输入处理方式。 */
    INT32 nDealType;
    /* 报警输入的布防时间表。 */
    NET_AlarmSchedule_S stAlarmSchedule;
    /* 报警输入触发后的联动配置。 */
    NET_LinkageList_S stLinkageList;
    /* 复制目标通道数量。 */
    INT32 nCopyToCount;
    /* 复制目标通道编号列表。 */
    INT32 anCopyTo[NET_ALARM_COPY_TO_MAX_NUM];
    /* 保留字段，调用方应置零。 */
    BYTE abyReserved[NET_ALARM_CONFIG_RESERVED_LEN];
} NET_AlarmInputInfo_S, *pNET_AlarmInputInfo_S;

/*
 * 描述：获取报警输入配置时返回的通道配置集合。
 */
typedef struct tagNETAlarmInputInfoList
{
    /* 有效报警输入通道数量。 */
    INT32 nAlarmInputCount;
    /* 报警输入通道配置列表。 */
    NET_AlarmInputInfo_S astAlarmInputs[NET_MAX_ALARM_IN_NUM];
    /* 保留字段，调用方应置零。 */
    BYTE abyReserved[NET_ALARM_CONFIG_RESERVED_LEN];
} NET_AlarmInputInfoList_S, *pNET_AlarmInputInfoList_S;

/*
 * 描述：单个报警输出通道的配置。
 */
typedef struct tagNETAlarmOutputInfo
{
    /* 报警输出通道编号。 */
    INT32 nAlarmNumber;
    /* 报警输出通道地址。 */
    CHAR strAlarmAddress[NET_ALARM_ADDRESS_LEN];
    /* 报警输出通道名称。 */
    CHAR strAlarmName[NET_ALARM_NAME_LEN];
    /* 报警输出保持时间，单位为秒。 */
    INT32 nDelayTime;
    /* 报警输出状态，取值见 NET_AlarmOutputState_EN。 */
    INT32 enState;
    /* 报警输出的布防时间表。 */
    NET_AlarmSchedule_S stAlarmSchedule;
    /* 复制目标通道数量。 */
    INT32 nCopyToCount;
    /* 复制目标通道编号列表。 */
    INT32 anCopyTo[NET_ALARM_COPY_TO_MAX_NUM];
    /* 保留字段，调用方应置零。 */
    BYTE abyReserved[NET_ALARM_CONFIG_RESERVED_LEN];
} NET_AlarmOutputInfo_S, *pNET_AlarmOutputInfo_S;

/*
 * 描述：获取报警输出配置时返回的通道配置集合。
 */
typedef struct tagNETAlarmOutputInfoList
{
    /* 有效报警输出通道数量。 */
    INT32 nAlarmOutputCount;
    /* 报警输出通道配置列表。 */
    NET_AlarmOutputInfo_S astAlarmOutputs[NET_MAX_ALARM_OUT_NUM];
    /* 保留字段，调用方应置零。 */
    BYTE abyReserved[NET_ALARM_CONFIG_RESERVED_LEN];
} NET_AlarmOutputInfoList_S, *pNET_AlarmOutputInfoList_S;

/*
 * 描述：闪光报警灯的配置。
 */
typedef struct tagNETFlashingLightAlarmInfo
{
    /* 闪光持续时间，单位为秒。 */
    INT32 nFlashTime;
    /* 闪烁频率，取值见 NET_FlashingLightFrequency_EN。 */
    INT32 enFlashFrequency;
    /* 闪光报警的布防时间表。 */
    NET_AlarmSchedule_S stAlarmSchedule;
    /* 复制目标通道数量。 */
    INT32 nCopyToCount;
    /* 复制目标通道编号列表。 */
    INT32 anCopyTo[NET_ALARM_COPY_TO_MAX_NUM];
    /* 保留字段，调用方应置零。 */
    BYTE abyReserved[NET_ALARM_CONFIG_RESERVED_LEN];
} NET_FlashingLightAlarmInfo_S, *pNET_FlashingLightAlarmInfo_S;

/*
 * 描述：人体红外（PIR）报警配置。
 */
typedef struct tagNETPirAlarmInfo
{
    /* 是否启用人体红外报警。 */
    BOOL bEnable;
    /* 人体红外报警名称。 */
    CHAR strAlarmName[NET_ALARM_NAME_LEN];
    /* 人体红外报警的布防时间表。 */
    NET_AlarmSchedule_S stAlarmSchedule;
    /* 人体红外报警触发后的联动配置。 */
    NET_LinkageList_S stLinkageList;
    /* 复制目标通道数量。 */
    INT32 nCopyToCount;
    /* 复制目标通道编号列表。 */
    INT32 anCopyTo[NET_ALARM_COPY_TO_MAX_NUM];
    /* 保留字段，调用方应置零。 */
    BYTE abyReserved[NET_ALARM_CONFIG_RESERVED_LEN];
} NET_PirAlarmInfo_S, *pNET_PirAlarmInfo_S;

/**
 * @brief 联动配置列表结构体指针类型
 */
typedef NET_LinkageList_S* pNET_LinkageList_S;


/**
 * @struct tagNETTVPeopleFlowRuleLine
 * @brief 人流统计规则线
 */
typedef struct tagNET_PeopleFlowRuleLine
{
    FLOAT       fStartPointX;                         /* 规则线起点X坐标 [0.0-1.0] */
    FLOAT       fStartPointY;                         /* 规则线起点Y坐标 [0.0-1.0] */
    FLOAT       fEndPointX;                           /* 规则线终点X坐标 [0.0-1.0] */
    FLOAT       fEndPointY;                           /* 规则线终点Y坐标 [0.0-1.0] */
    INT32       nDirection;                           /* 统计方向 1-A到B 2-B到A */
    BYTE        byRes[60];                            /* 保留字段 */
} NET_PeopleFlowRuleLine_S;

typedef NET_PeopleFlowRuleLine_S* pNET_PeopleFlowRuleLine_S;

/**
 * @struct tagNET_PeopleAlarmRule
 * @brief 单档人数报警配置
 */
typedef struct tagNET_PeopleAlarmRule
{
    BOOL        bEnable;                               /* 是否启用 */
    INT32       nThreshold;                           /* 人数触发阈值 */
    NET_LinkageList_S stLinkageList;              /* 联动配置 */
    BYTE        byRes[128];                           /* 保留字段 */
} NET_PeopleAlarmRule_S;

typedef NET_PeopleAlarmRule_S* pNET_PeopleAlarmRule_S;

/**
 * @struct tagNET_PeopleAlarmConfig
 * @brief 三级人数报警配置
 */
typedef struct tagNET_PeopleAlarmConfig
{
    NET_PeopleAlarmRule_S stNormal;              /* 普通报警 */
    NET_PeopleAlarmRule_S stMedium;              /* 中度报警 */
    NET_PeopleAlarmRule_S stSevere;              /* 严重报警 */
    BYTE        byRes[256];                           /* 保留字段 */
} NET_PeopleAlarmConfig_S;

typedef NET_PeopleAlarmConfig_S* pNET_PeopleAlarmConfig_S;

/**
 * @struct tagNET_StatisticsResetConfig
 * @brief 定时清零配置
 */
typedef struct tagNET_StatisticsResetConfig
{
    BOOL        bEnable;                               /* 是否启用 */
    INT32       nHour;                                /* 执行小时 [0-23] */
    INT32       nMinute;                              /* 执行分钟 [0-59] */
    BYTE        byRes[16];                            /* 保留字段 */
} NET_StatisticsResetConfig_S;

typedef NET_StatisticsResetConfig_S* pNET_StatisticsResetConfig_S;

/**
 * @struct tagNET_PeopleFlowStatisticsCfg
 * @brief 人流统计配置信息
 * @note 用于NET_GET_PEOPLE_FLOW_STATISTICS_CFG/NET_SET_PEOPLE_FLOW_STATISTICS_CFG
 */
typedef struct tagNET_PeopleFlowStatisticsCfg
{
    BOOL        bEnable;                               /* 是否启用人流统计 */
    INT32       nSensitivity;                           /* 灵敏度[1-100] */
    NET_PeopleFlowRuleLine_S stRuleLine;        /* 规则线 */
    INT32       uPointCount;                         /* 检测区域顶点数，最多32个 */
    FLOAT       afPointX[32];                         /* 检测区域X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                         /* 检测区域Y坐标数组 [0.0-1.0] */
    INT32       nReportInterval;                        /* 数据上报间隔(秒) */
    INT32       enStatisticsType;                      /* 统计类型 NET_PEOPLE_FLOW_STAT_TYPE_E */
    NET_StatisticsResetConfig_S stTimedReset;     /* 定时清零 */
    NET_PeopleAlarmConfig_S stStayAlarm;          /* 滞留人数报警 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    BYTE        byRes[256];                           /* 保留字段 */
} NET_PeopleFlowStatisticsCfg_S;

typedef NET_PeopleFlowStatisticsCfg_S* pNET_PeopleFlowStatisticsCfg_S;

/**
 * @struct tagNET_PeopleDensityDetectionCfg
 * @brief 人员密度检测配置信息
 * @note 用于NET_GET_PEOPLE_DENSITY_DETECTION_CFG/NET_SET_PEOPLE_DENSITY_DETECTION_CFG
 */
typedef struct tagNET_PeopleDensityDetectionCfg
{
    BOOL        bEnable;                               /* 是否启用人员密度检测 */
    INT32       nSensitivity;                           /* 灵敏度[1-100] */
    INT32       uPointCount;                         /* 检测区域顶点数，最多32个 */
    FLOAT       afPointX[32];                         /* 检测区域X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                         /* 检测区域Y坐标数组 [0.0-1.0] */
    INT32       nReportInterval;                        /* 数据上报间隔(秒) */
    NET_PeopleAlarmConfig_S stDensityAlarm;      /* 密度报警 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    BYTE        byRes[256];                           /* 保留字段 */
} NET_PeopleDensityDetectionCfg_S;

typedef NET_PeopleDensityDetectionCfg_S* pNET_PeopleDensityDetectionCfg_S;


/* ==================== 移动侦测相关结构体 ==================== */

/**
 * @struct tagNET_MotionRegion
 * @brief 移动侦测专家模式区域参数 Motion detection expert mode region
 */
typedef struct tagNET_MotionRegion
{
    INT32       nAreaNo;                            /* 侦测区域编号，从1开始 */
    INT32       nRectLeft;                           /* 区域左坐标 */
    INT32       nRectTop;                            /* 区域上坐标 */
    INT32       nRectRight;                          /* 区域右坐标 */
    INT32       nRectBottom;                         /* 区域下坐标 */
    INT32       nCloseSensitivity;                   /* 日夜参数转换为关闭时的灵敏度 [0,100] */
    INT32       nDaytimeSensitivity;                 /* 白天灵敏度 [0,100] */
    INT32       nNightSensitivity;                   /* 夜晚灵敏度 [0,100] */
    BYTE        byRes[32];                           /* 保留字段 */
}NET_MotionRegion_S;

typedef NET_MotionRegion_S* pNET_MotionRegion_S;

/**
 * @struct tagNET_MotionExpertMode
 * @brief 移动侦测专家模式参数 Motion detection expert mode
 */
typedef struct tagNET_MotionExpertMode
{
    INT32       nExpertDayNightCtrl;                /* 日夜控制：0-关闭(默认)，1-自动切换，2-定时切换 */
    NET_SchedTime_S stDayTime;                  /* 日夜切换时间 定时切换时有效 */
    INT32       uRegionCount;                       /* 区域数量 */
    NET_MotionRegion_S astRegion[16];               /* 移动侦测专家模式区域参数，最多16个 */
    BYTE        byRes[128];                         /* 保留字段 */
}NET_MotionExpertMode_S;

typedef NET_MotionExpertMode_S* pNET_MotionExpertMode_S;

/**
 * @struct tagNET_MotionNormalMode
 * @brief 移动侦测普通模式参数 Motion detection normal mode
 */
typedef struct tagNET_MotionNormalMode
{
    INT32       nSensitivity;                        /* 灵敏度 [0,100] */
    INT32       nRegionType;                         /* 区域类型 0：筒型 1：网格 */
    INT32       nRectLeft;                           /* 筒型区域左坐标 (当nRegionType=0时有效) */
    INT32       nRectTop;                            /* 筒型区域上坐标 */
    INT32       nRectRight;                          /* 筒型区域右坐标 */
    INT32       nRectBottom;                         /* 筒型区域下坐标 */
    INT32       uGridWidth;                          /* 网格宽度 (当nRegionType=1时有效，通常22) */
    INT32       uGridHeight;                         /* 网格高度 (当nRegionType=1时有效，通常18) */
    BYTE        abyGridArea[18][22];                /* 网格区域标记，1表示移动侦测区域 (18x22) */
    BYTE        byRes[128];                          /* 保留字段 */
}NET_MotionNormalMode_S;

typedef NET_MotionNormalMode_S* pNET_MotionNormalMode_S;

/**
 * @struct tagNET_MotionAlarmInfo
 * @brief 移动侦测告警配置信息 Motion detection alarm configuration
 * @note 用于NET_GET_MOTIONALARM/NET_SET_MOTIONALARM
 */
typedef struct tagNET_MotionAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    BOOL        bDynamicAnalysisEnable;              /* 是否启用动态分析 */
    INT32       uMode;                               /* 模式 0-普通模式 1-专家模式 NET_MOTION_MODE_E */
    NET_MotionNormalMode_S stNormalMode;            /* 普通模式参数 */
    NET_MotionExpertMode_S stExpertMode;            /* 专家模式参数 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_MotionAlarmInfo_S;

typedef NET_MotionAlarmInfo_S* pNET_MotionAlarmInfo_S;

/* ==================== 隐私遮盖配置相关结构体 ==================== */

/**
 * @struct tagNETTVPrivacyMaskArea
 * @brief 单个隐私遮盖区域 Privacy mask area
 * @note 用于NET_PRIVACY_MASK_CFG_S中配置单个遮盖区域
 */
typedef struct tagNET_PrivacyMaskArea
{
    INT32       nAreaID;                              /* 遮盖区域ID [0, NET_MAX_PRIVACY_MASK_AREA_NUM) */
    BOOL        bEnable;                              /* 是否启用 0-不启用 1-启用 */
    INT32       nRectLeft;                            /* 遮盖区域左坐标 [0, 8191] */
    INT32       nRectTop;                             /* 遮盖区域上坐标 [0, 8191] */
    INT32       nRectRight;                           /* 遮盖区域右坐标 [0, 8191] */
    INT32       nRectBottom;                          /* 遮盖区域下坐标 [0, 8191] */
    BYTE        byRes[32];                            /* 保留字段 */
}NET_PrivacyMaskArea_S;

typedef NET_PrivacyMaskArea_S* pNET_PrivacyMaskArea_S;

/**
 * @struct tagNETTVPrivacyMaskCfg
 * @brief 隐私遮盖配置信息 Privacy mask configuration
 * @note 用于NET_GET_PRIVACYMASKCFG/NET_SET_PRIVACYMASKCFG
 */
typedef struct tagNET_PrivacyMaskCfg
{
    BOOL        bEnable;                              /* 是否启用隐私遮盖 0-不启用 1-启用 */
    INT32       uAreaCount;                          /* 遮盖区域数量 [0, NET_MAX_PRIVACY_MASK_AREA_NUM] */
    NET_PrivacyMaskArea_S astArea[NET_MAX_PRIVACY_MASK_AREA_NUM]; /* 遮盖区域数组 */
    BYTE        byRes[256];                           /* 保留字段 */
}NET_PrivacyMaskCfg_S;

typedef NET_PrivacyMaskCfg_S* pNET_PrivacyMaskCfg_S;

/* ==================== 遮挡报警相关结构体 ==================== */

/**
 * @struct tagNET_TamperAlarmInfo
 * @brief 遮挡检测告警配置信息 Tamper detection alarm configuration
 * @note 用于NET_GET_TAMPERALARM/NET_SET_TAMPERALARM
 */
typedef struct tagNET_TamperAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uSensitivity;                        /* 遮挡报警灵敏度[0,3]，值越大越灵敏 */
    INT32       nRectLeft;                           /* 遮挡区域左坐标 */
    INT32       nRectTop;                            /* 遮挡区域上坐标 */
    INT32       nRectRight;                          /* 遮挡区域右坐标 */
    INT32       nRectBottom;                         /* 遮挡区域下坐标 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TamperAlarmInfo_S;

typedef NET_TamperAlarmInfo_S* pNET_TamperAlarmInfo_S;

/* ==================== 越界检测相关结构体 ==================== */

/**
 * @struct tagNET_BoundaryPlane
 * @brief 越界检测警戒线规则 Boundary detection plane rule
 */
typedef struct tagNET_BoundaryPlane
{
    BOOL        bEnable;                             /* 是否启用 */
    FLOAT       fStartPosX;                          /* 警戒线起始点X坐标 [0.0-1.0] */
    FLOAT       fStartPosY;                          /* 警戒线起始点Y坐标 [0.0-1.0] */
    FLOAT       fEndPosX;                            /* 警戒线终止点X坐标 [0.0-1.0] */
    FLOAT       fEndPosY;                            /* 警戒线终止点Y坐标 [0.0-1.0] */
    INT32       enCrossDirection;                    /* 警戒线的穿越方向 NET_CROSS_DIRECTION_E */
    INT32       nSensitivity;                        /* 警戒线灵敏度[1,100] */
    INT32       uDetectionTargetCount;               /* 检测目标数量 */
    INT32       auDetectionTarget[8];                /* 检测目标数组 NET_DETECTION_TARGET_E，最多8个 */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_BoundaryPlane_S;

typedef NET_BoundaryPlane_S* pNET_BoundaryPlane_S;

/**
 * @struct tagNET_CrossLineAlarmInfo
 * @brief 越界检测告警配置信息 Cross line detection alarm configuration
 * @note 用于NET_GET_CROSSLINEALARM/NET_SET_CROSSLINEALARM
 */
typedef struct tagNET_CrossLineAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多4个 */
    NET_BoundaryPlane_S stRule[4];                   /* 越界检测区域规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_CrossLineAlarmInfo_S;

typedef NET_CrossLineAlarmInfo_S* pNET_CrossLineAlarmInfo_S;

/* ==================== 入侵检测相关结构体 ==================== */

/**
 * @struct tagNET_IntrusionRule
 * @brief 入侵检测区域规则参数 Intrusion detection region rule
 */
typedef struct tagNET_IntrusionRule
{
    BOOL        bEnable;                             /* 是否启用 */
    INT32       uPointCount;                         /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，判断有效报警的时间[0,100] 单位秒 */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       uDetectionTargetCount;               /* 检测目标数量 */
    INT32       auDetectionTarget[8];                /* 检测目标数组 NET_DETECTION_TARGET_E，最多8个 */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_IntrusionRule_S;

typedef NET_IntrusionRule_S* pNET_IntrusionRule_S;

/**
 * @struct tagNET_IntrusionAlarmInfo
 * @brief 入侵检测告警配置信息 Intrusion detection alarm configuration
 * @note 用于NET_GET_INTRUSIONALARM/NET_SET_INTRUSIONALARM
 */
typedef struct tagNET_IntrusionAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多4个 */
    NET_IntrusionRule_S stRule[4];                   /* 区域入侵检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_IntrusionAlarmInfo_S;

typedef NET_IntrusionAlarmInfo_S* pNET_IntrusionAlarmInfo_S;

/**
 * @struct tagNET_EnterRegionAlarmInfo
 * @brief 进入区域侦测告警配置信息 Enter region detection alarm configuration
 * @note 用于NET_GET_ENTERREGIONALARM/NET_SET_ENTERREGIONALARM
 */
typedef struct tagNET_EnterRegionAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多4个 */
    NET_IntrusionRule_S stRule[4];                   /* 进入区域检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_EnterRegionAlarmInfo_S;

typedef NET_EnterRegionAlarmInfo_S* pNET_EnterRegionAlarmInfo_S;

/**
 * @struct tagNET_LeaveRegionAlarmInfo
 * @brief 离开区域侦测告警配置信息 Leave region detection alarm configuration
 * @note 用于NET_GET_LEAVEREGIONALARM/NET_SET_LEAVEREGIONALARM
 */
typedef struct tagNET_LeaveRegionAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多4个 */
    NET_IntrusionRule_S stRule[4];                   /* 离开区域检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_LeaveRegionAlarmInfo_S;

typedef NET_LeaveRegionAlarmInfo_S* pNET_LeaveRegionAlarmInfo_S;

/* ==================== 徘徊侦测相关结构体 ==================== */

/**
 * @struct tagNET_LoiteringRule
 * @brief 徘徊侦测区域规则参数 Loitering detection region rule
 */
typedef struct tagNET_LoiteringRule
{
    BOOL        bEnable;                             /* 是否启用 */
    INT32       uPointCount;                         /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，判断有效报警的时间[0,100] 单位秒 */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       uDetectionTargetCount;               /* 检测目标数量 */
    INT32       auDetectionTarget[8];                /* 检测目标数组 NET_DETECTION_TARGET_E，最多8个 */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_LoiteringRule_S;

typedef NET_LoiteringRule_S* pNET_LoiteringRule_S;

/**
 * @struct tagNET_LoiteringAlarmInfo
 * @brief 徘徊告警配置信息 Loitering detection alarm configuration
 * @note 用于NET_GET_LOITERINGALARM/NET_SET_LOITERINGALARM
 */
typedef struct tagNET_LoiteringAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多4个 */
    NET_LoiteringRule_S stRule[4];                   /* 区域入侵检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_LoiteringAlarmInfo_S;

typedef NET_LoiteringAlarmInfo_S* pNET_LoiteringAlarmInfo_S;

/**
 * @struct tagNET_SceneChangeAlarmInfo
 * @brief 场景变更侦测告警配置信息 Scene change detection alarm configuration
 * @note 用于NET_GET_SCENECHANGEALARM/NET_SET_SCENECHANGEALARM
 */
typedef struct tagNET_SceneChangeAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       nSensitivity;                        /* 灵敏度 [1,100] */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_SceneChangeAlarmInfo_S;

typedef NET_SceneChangeAlarmInfo_S* pNET_SceneChangeAlarmInfo_S;

/**
 * @struct tagNET_CrowdGatheringRule
 * @brief 人员聚集侦测区域规则参数 Crowd gathering detection region rule
 */
typedef struct tagNET_CrowdGatheringRule
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uPointCount;                         /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nObjectOccup;                        /* 人体面积占用户设定区域面积的比例阈值[1,100] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_CrowdGatheringRule_S;

typedef NET_CrowdGatheringRule_S* pNET_CrowdGatheringRule_S;

/**
 * @struct tagNET_CrowdGatheringAlarmInfo
 * @brief 人员聚集侦测告警配置信息 Crowd gathering detection alarm configuration
 * @note 用于NET_GET_CROWDGATHERINGALARM/NET_SET_CROWDGATHERINGALARM
 */
typedef struct tagNET_CrowdGatheringAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多4个 */
    NET_CrowdGatheringRule_S astRule[4];             /* 人员聚集侦测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_CrowdGatheringAlarmInfo_S;

typedef NET_CrowdGatheringAlarmInfo_S* pNET_CrowdGatheringAlarmInfo_S;

/**
 * @struct tagNET_ParkingRule
 * @brief 停车侦测区域规则参数 Parking detection region rule
 */
typedef struct tagNET_ParkingRule
{
    INT32       uPointCount;                         /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，单位秒 [0,100] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_ParkingRule_S;

typedef NET_ParkingRule_S* pNET_ParkingRule_S;

/**
 * @struct tagNET_ParkingAlarmInfo
 * @brief 停车侦测告警配置信息 Parking detection alarm configuration
 * @note 用于NET_GET_PARKINGALARM/NET_SET_PARKINGALARM
 */
typedef struct tagNET_ParkingAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多8个 */
    NET_ParkingRule_S astRule[8];                   /* 停车侦测规则，最多8个 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_ParkingAlarmInfo_S;

typedef NET_ParkingAlarmInfo_S* pNET_ParkingAlarmInfo_S;

/* ==================== 垃圾暴露检测相关结构体 ==================== */

/**
 * @struct tagNETTVGarbageExposureRule
 * @brief 垃圾暴露检测规则参数 Garbage exposure detection region rule
 */
typedef struct tagNET_GarbageExposureRule
{
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       uPointCount;                          /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                          /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                          /* 区域顶点Y坐标数组 [0.0-1.0] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_GarbageExposureRule_S;

typedef NET_GarbageExposureRule_S* pNET_GarbageExposureRule_S;

/**
 * @struct tagNET_GarbageExposureCfg
 * @brief 垃圾暴露检测配置信息 Garbage exposure detection configuration
 * @note 用于NET_GET_GARBAGE_EXPOSURE_CFG/NET_SET_GARBAGE_EXPOSURE_CFG
 */
typedef struct tagNET_GarbageExposureCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_GarbageExposureRule_S stRule;             /* 垃圾暴露检测规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_GarbageExposureCfg_S;

typedef NET_GarbageExposureCfg_S* pNET_GarbageExposureCfg_S;

/* ==================== 垃圾满溢检测相关结构体 ==================== */

/**
 * @struct tagNET_GarbageOverflowRule
 * @brief 垃圾满溢检测规则参数 Garbage overflow detection region rule
 */
typedef struct tagNET_GarbageOverflowRule
{
    INT32       nSensitivity;                          /* 灵敏度[1,100] */
    INT32       uPointCount;                          /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                          /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                          /* 区域顶点Y坐标数组 [0.0-1.0] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_GarbageOverflowRule_S;

typedef NET_GarbageOverflowRule_S* pNET_GarbageOverflowRule_S;

/**
 * @struct tagNET_GarbageOverflowCfg
 * @brief 垃圾满溢检测配置信息 Garbage overflow detection configuration
 * @note 用于NET_GET_GARBAGE_OVERFLOW_CFG/NET_SET_GARBAGE_OVERFLOW_CFG
 */
typedef struct tagNET_GarbageOverflowCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_GarbageOverflowRule_S stRule;             /* 垃圾满溢检测规则 */
    INT32       nTimeThreshold;                        /* 时间阈值(秒) */
    NET_AlarmSchedule_S stAlarmSchedule;          /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_GarbageOverflowCfg_S;

typedef NET_GarbageOverflowCfg_S* pNET_GarbageOverflowCfg_S;

/* ==================== 单规则智能检测配置结构体 ==================== */

/**
 * @struct tagNETTVAiSimpleRule
 * @brief 单规则智能检测通用规则参数
 */
typedef struct tagNET_AiSimpleRule
{
    INT32       nSensitivity;                          /* 灵敏度[1,100] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_AiSimpleRule_S;

typedef NET_AiSimpleRule_S* pNET_AiSimpleRule_S;

/**
 * @struct tagNETTVManholeCoverAbnormalCfg
 * @brief 井盖异常检测配置信息 Manhole cover abnormal detection configuration
 * @note 用于NET_GET_MANHOLE_COVER_ABNORMAL_CFG/NET_SET_MANHOLE_COVER_ABNORMAL_CFG
 */
typedef struct tagNET_ManholeCoverAbnormalCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 井盖异常检测规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_ManholeCoverAbnormalCfg_S;

typedef NET_ManholeCoverAbnormalCfg_S* pNET_ManholeCoverAbnormalCfg_S;

/**
 * @struct tagNET_SleepOnDutyCfg
 * @brief 睡岗识别配置信息 Sleep on duty detection configuration
 * @note 用于NET_GET_SLEEP_ON_DUTY_CFG/NET_SET_SLEEP_ON_DUTY_CFG
 */
typedef struct tagNET_SleepOnDutyCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 睡岗识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_SleepOnDutyCfg_S;

typedef NET_SleepOnDutyCfg_S* pNET_SleepOnDutyCfg_S;

/**
 * @struct tagNET_ElectricVehicleInElevatorCfg
 * @brief 电瓶车进电梯识别配置信息 Electric vehicle in elevator detection configuration
 * @note 用于NET_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG/NET_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG
 */
typedef struct tagNET_ElectricVehicleInElevatorCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 电瓶车进电梯识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_ElectricVehicleInElevatorCfg_S;

typedef NET_ElectricVehicleInElevatorCfg_S* pNET_ElectricVehicleInElevatorCfg_S;

/**
 * @struct tagNET_PersonFallDownCfg
 * @brief 人员倒地识别配置信息 Person fall down detection configuration
 * @note 用于NET_GET_PERSON_FALL_DOWN_CFG/NET_SET_PERSON_FALL_DOWN_CFG
 */
typedef struct tagNET_PersonFallDownCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 人员倒地识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_PersonFallDownCfg_S;

typedef NET_PersonFallDownCfg_S* pNET_PersonFallDownCfg_S;

/**
 * @struct tagNET_ConstructionOccupyRoadCfg
 * @brief 施工占道识别配置信息 Construction occupy road detection configuration
 * @note 用于NET_GET_CONSTRUCTION_OCCUPY_ROAD_CFG/NET_SET_CONSTRUCTION_OCCUPY_ROAD_CFG
 */
typedef struct tagNET_ConstructionOccupyRoadCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 施工占道识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_ConstructionOccupyRoadCfg_S;

typedef NET_ConstructionOccupyRoadCfg_S* pNET_ConstructionOccupyRoadCfg_S;

/**
 * @struct tagNET_CongestionCfg
 * @brief 拥堵识别配置信息 Congestion detection configuration
 * @note 用于NET_GET_CONGESTION_CFG/NET_SET_CONGESTION_CFG
 */
typedef struct tagNET_CongestionCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 拥堵识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_CongestionCfg_S;

typedef NET_CongestionCfg_S* pNET_CongestionCfg_S;

/**
 * @struct tagNETTVLicensePlateRecognitionCfg
 * @brief 车牌识别配置信息 License plate recognition configuration
 * @note 用于NET_GET_LICENSE_PLATE_RECOGNITION_CFG/NET_SET_LICENSE_PLATE_RECOGNITION_CFG
 */
typedef struct tagNET_LicensePlateRecognitionCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 车牌识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_LicensePlateRecognitionCfg_S;

typedef NET_LicensePlateRecognitionCfg_S* pNET_LicensePlateRecognitionCfg_S;

/**
 * @struct tagNET_HighAltitudeSeatbeltCfg
 * @brief 高空安全带识别配置信息 High altitude seatbelt detection configuration
 * @note 用于NET_GET_HIGH_ALTITUDE_SEATBELT_CFG/NET_SET_HIGH_ALTITUDE_SEATBELT_CFG
 */
typedef struct tagNET_HighAltitudeSeatbeltCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 高空安全带识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_HighAltitudeSeatbeltCfg_S;

typedef NET_HighAltitudeSeatbeltCfg_S* pNET_HighAltitudeSeatbeltCfg_S;

/**
 * @struct tagNET_SafetyHelmetCfg
 * @brief 安全帽识别配置信息 Safety helmet detection configuration
 * @note 用于NET_GET_SAFETY_HELMET_CFG/NET_SET_SAFETY_HELMET_CFG
 */
typedef struct tagNET_SafetyHelmetCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 安全帽识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_SafetyHelmetCfg_S;

typedef NET_SafetyHelmetCfg_S* pNET_SafetyHelmetCfg_S;

/**
 * @struct tagNET_PersonFallCfg
 * @brief 摔倒识别配置信息 Person fall detection configuration
 * @note 用于NET_GET_PERSON_FALL_CFG/NET_SET_PERSON_FALL_CFG
 */
typedef struct tagNET_PersonFallCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 摔倒识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_PersonFallCfg_S;

typedef NET_PersonFallCfg_S* pNET_PersonFallCfg_S;

/**
 * @struct tagNET_PhoneUsageCfg
 * @brief 玩手机识别配置信息 Phone usage detection configuration
 * @note 用于NET_GET_PHONE_USAGE_CFG/NET_SET_PHONE_USAGE_CFG
 */
typedef struct tagNET_PhoneUsageCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 玩手机识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_PhoneUsageCfg_S;

typedef NET_PhoneUsageCfg_S* pNET_PhoneUsageCfg_S;

/**
 * @struct tagNET_SmokingCfg
 * @brief 抽烟识别配置信息 Smoking detection configuration
 * @note 用于NET_GET_SMOKING_CFG/NET_SET_SMOKING_CFG
 */
typedef struct tagNET_SmokingCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 抽烟识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_SmokingCfg_S;

typedef NET_SmokingCfg_S* pNET_SmokingCfg_S;

/**
 * @struct tagNET_OpenFlameCfg
 * @brief 明火识别配置信息 Open flame detection configuration
 * @note 用于NET_GET_OPEN_FLAME_CFG/NET_SET_OPEN_FLAME_CFG
 */
typedef struct tagNET_OpenFlameCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 明火识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_OpenFlameCfg_S;

typedef NET_OpenFlameCfg_S* pNET_OpenFlameCfg_S;

/**
 * @struct tagNET_BareSoilCfg
 * @brief 黄土裸露识别配置信息 Bare soil detection configuration
 * @note 用于NET_GET_BARE_SOIL_CFG/NET_SET_BARE_SOIL_CFG
 */
typedef struct tagNET_BareSoilCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 黄土裸露识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_BareSoilCfg_S;

typedef NET_BareSoilCfg_S* pNET_BareSoilCfg_S;

/**
 * @struct tagNET_HoleProtectionBarCfg
 * @brief 洞口防护栏识别配置信息 Hole protection bar detection configuration
 * @note 用于NET_GET_HOLE_PROTECTION_BAR_CFG/NET_SET_HOLE_PROTECTION_BAR_CFG
 */
typedef struct tagNET_HoleProtectionBarCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 洞口防护栏识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_HoleProtectionBarCfg_S;

typedef NET_HoleProtectionBarCfg_S* pNET_HoleProtectionBarCfg_S;

/**
 * @struct tagNET_ReflectiveClothingCfg
 * @brief 反光衣识别配置信息 Reflective clothing detection configuration
 * @note 用于NET_GET_REFLECTIVE_CLOTHING_CFG/NET_SET_REFLECTIVE_CLOTHING_CFG
 */
typedef struct tagNET_ReflectiveClothingCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 反光衣识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_ReflectiveClothingCfg_S;

typedef NET_ReflectiveClothingCfg_S* pNET_ReflectiveClothingCfg_S;

/* ==================== 智能事件配置相关结构体 ==================== */

/**
 * @struct tagNETTVSmartRegion
 * @brief 智能事件检测区域 Smart event detection region
 */
typedef struct tagNET_SmartRegion
{
    INT32       uPointCount;                          /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                          /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                          /* 区域顶点Y坐标数组 [0.0-1.0] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_SmartRegion_S;

typedef NET_SmartRegion_S* pNET_SmartRegion_S;

/**
 * @struct tagNET_SmartRegionRule
 * @brief 智能事件区域规则参数 Smart event region rule
 */
typedef struct tagNET_SmartRegionRule
{
    BOOL        bEnable;                               /* 是否启用 */
    INT32       uPointCount;                          /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                          /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                          /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nTimeThreshold;                        /* 行为事件触发时间阈值，判断有效报警的时间[0,100] 单位秒 */
    INT32       nSensitivity;                          /* 灵敏度[1,100] */
    INT32       uDetectionTargetCount;                /* 检测目标数量 */
    INT32       auDetectionTarget[8];                 /* 检测目标数组 NET_DETECTION_TARGET_E，最多8个 */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_SmartRegionRule_S;

typedef NET_SmartRegionRule_S* pNET_SmartRegionRule_S;

/**
 * @struct tagNET_SmartLineRule
 * @brief 智能事件警戒线规则参数 Smart event line rule
 */
typedef struct tagNET_SmartLineRule
{
    BOOL        bEnable;                               /* 是否启用 */
    FLOAT       fStartPosX;                            /* 警戒线起始点X坐标 [0.0-1.0] */
    FLOAT       fStartPosY;                            /* 警戒线起始点Y坐标 [0.0-1.0] */
    FLOAT       fEndPosX;                              /* 警戒线终止点X坐标 [0.0-1.0] */
    FLOAT       fEndPosY;                              /* 警戒线终止点Y坐标 [0.0-1.0] */
    INT32       enCrossDirection;                      /* 警戒线的穿越方向 NET_CROSS_DIRECTION_E */
    INT32       nSensitivity;                          /* 警戒线灵敏度[1,100] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_SmartLineRule_S;

typedef NET_SmartLineRule_S* pNET_SmartLineRule_S;

/**
 * @struct tagNETTVPetRecognitionInfo
 * @brief 宠物识别配置信息 Pet recognition configuration
 * @note 用于NET_GET_PET_RECOGNITION_INFO/NET_SET_PET_RECOGNITION_INFO
 */
typedef struct tagNET_PetRecognitionInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    BOOL        bDynamicAnalysisEnable;                /* 是否启用动态分析 */
    INT32       nSensitivity;                          /* 灵敏度[1,100] */
    NET_SmartRegion_S stRegion;                    /* 宠物识别检测区域 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_PetRecognitionInfo_S;

typedef NET_PetRecognitionInfo_S* pNET_PetRecognitionInfo_S;

/**
 * @struct tagNET_ClimbFenceInfo
 * @brief 翻越围栏配置信息 Climb fence detection configuration
 * @note 用于NET_GET_CLIMB_FENCE_INFO/NET_SET_CLIMB_FENCE_INFO
 */
typedef struct tagNET_ClimbFenceInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                           /* 规则数量，最多4个 */
    NET_SmartRegionRule_S stRule[4];             /* 翻越围栏检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_ClimbFenceInfo_S;

typedef NET_ClimbFenceInfo_S* pNET_ClimbFenceInfo_S;

/**
 * @struct tagNET_DimissionInfo
 * @brief 离岗配置信息 Leave post detection configuration
 * @note 用于NET_GET_DIMISSION_INFO/NET_SET_DIMISSION_INFO
 */
typedef struct tagNET_DimissionInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                           /* 规则数量，最多4个 */
    NET_SmartRegionRule_S stRule[4];             /* 离岗检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_DimissionInfo_S;

typedef NET_DimissionInfo_S* pNET_DimissionInfo_S;

/**
 * @struct tagNET_IllegalLaneInfo
 * @brief 违规变道配置信息 Illegal lane change detection configuration
 * @note 用于NET_GET_ILLEGAL_LANE_INFO/NET_SET_ILLEGAL_LANE_INFO
 */
typedef struct tagNET_IllegalLaneInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                           /* 规则数量，最多4个 */
    NET_SmartLineRule_S stRule[4];               /* 违规变道检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_IllegalLaneInfo_S;

typedef NET_IllegalLaneInfo_S* pNET_IllegalLaneInfo_S;

/**
 * @struct tagNET_RetrogradeInfo
 * @brief 逆行配置信息 Retrograde detection configuration
 * @note 用于NET_GET_RETROGRADE_INFO/NET_SET_RETROGRADE_INFO
 */
typedef struct tagNET_RetrogradeInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                           /* 规则数量，最多4个 */
    NET_SmartLineRule_S stRule[4];               /* 逆行检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_RetrogradeInfo_S;

typedef NET_RetrogradeInfo_S* pNET_RetrogradeInfo_S;

/**
 * @struct tagNET_NonmotorVehicleIntrusionInfo
 * @brief 非机动车闯入配置信息 Non-motor vehicle intrusion detection configuration
 * @note 用于NET_GET_NONMOTOR_VEHICLE_INTRUSION_INFO/NET_SET_NONMOTOR_VEHICLE_INTRUSION_INFO
 */
typedef struct tagNET_NonmotorVehicleIntrusionInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                           /* 规则数量，最多4个 */
    NET_SmartRegionRule_S stRule[4];             /* 非机动车闯入检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_NonmotorVehicleIntrusionInfo_S;

typedef NET_NonmotorVehicleIntrusionInfo_S* pNET_NonmotorVehicleIntrusionInfo_S;

/**
 * @struct tagNET_OccupationEmergencyInfo
 * @brief 应急车道占用识别配置信息 Emergency lane occupancy detection configuration
 * @note 用于NET_GET_OCCUPATION_EMERGENCY_INFO/NET_SET_OCCUPATION_EMERGENCY_INFO
 */
typedef struct tagNET_OccupationEmergencyInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                           /* 规则数量，最多4个 */
    NET_SmartRegionRule_S stRule[4];             /* 应急车道占用检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_OccupationEmergencyInfo_S;

typedef NET_OccupationEmergencyInfo_S* pNET_OccupationEmergencyInfo_S;

/**
 * @struct tagNET_PedestrianIntrusionInfo
 * @brief 行人闯入配置信息 Pedestrian intrusion detection configuration
 * @note 用于NET_GET_PEDESTRIAN_INTRUSION_INFO/NET_SET_PEDESTRIAN_INTRUSION_INFO
 */
typedef struct tagNET_PedestrianIntrusionInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                           /* 规则数量，最多4个 */
    NET_SmartRegionRule_S stRule[4];             /* 行人闯入检测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_PedestrianIntrusionInfo_S;

typedef NET_PedestrianIntrusionInfo_S* pNET_PedestrianIntrusionInfo_S;

/**
 * @struct tagNET_SmokeFireCfg
 * @brief 烟火识别配置信息 Smoke fire detection configuration
 * @note 用于NET_GET_SMOKE_FIRE_CFG/NET_SET_SMOKE_FIRE_CFG
 */
typedef struct tagNET_SmokeFireCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 烟火识别规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_SmokeFireCfg_S;

typedef NET_SmokeFireCfg_S* pNET_SmokeFireCfg_S;

/**
 * @struct tagNET_RoadPondingCfg
 * @brief 道路积水检测配置信息 Road ponding detection configuration
 * @note 用于NET_GET_ROAD_PONDING_CFG/NET_SET_ROAD_PONDING_CFG
 */
typedef struct tagNET_RoadPondingCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_AiSimpleRule_S stRule;                    /* 道路积水检测规则 */
    NET_AlarmSchedule_S stAlarmSchedule;           /* 布防时间 */
    NET_LinkageList_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_RoadPondingCfg_S;

typedef NET_RoadPondingCfg_S* pNET_RoadPondingCfg_S;

/**
 * @struct tagNETTVUnattendedObjectRule
 * @brief 物品遗留侦测区域规则参数 Unattended object detection region rule
 */
typedef struct tagNET_UnattendedObjectRule
{
    INT32       uPointCount;                         /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，单位秒 [12,100] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_UnattendedObjectRule_S;

typedef NET_UnattendedObjectRule_S* pNET_UnattendedObjectRule_S;

/**
 * @struct tagNETTVUnattendedObjectAlarmInfo
 * @brief 物品遗留侦测告警配置信息 Unattended object detection alarm configuration
 * @note 用于NET_GET_UNATTENDEDOBJECTALARM/NET_SET_UNATTENDEDOBJECTALARM
 */
typedef struct tagNET_UnattendedObjectAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多4个 */
    NET_UnattendedObjectRule_S stRule[4];            /* 物品遗留侦测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;             /* 布防时间 */
    NET_LinkageList_S stLinkageList;                 /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_UnattendedObjectAlarmInfo_S;

typedef NET_UnattendedObjectAlarmInfo_S* pNET_UnattendedObjectAlarmInfo_S;

/**
 * @struct tagNETTVObjectRemovalRule
 * @brief 物品拿取侦测区域规则参数 Object removal detection region rule
 */
typedef struct tagNET_ObjectRemovalRule
{
    INT32       uPointCount;                         /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，单位秒 [12,100] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_ObjectRemovalRule_S;

typedef NET_ObjectRemovalRule_S* pNET_ObjectRemovalRule_S;

/**
 * @struct tagNETTVObjectRemovalAlarmInfo
 * @brief 物品拿取侦测告警配置信息 Object removal detection alarm configuration
 * @note 用于NET_GET_OBJECTREMOVALALARM/NET_SET_OBJECTREMOVALALARM
 */
typedef struct tagNET_ObjectRemovalAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       uRuleCount;                          /* 规则数量，最多4个 */
    NET_ObjectRemovalRule_S stRule[4];               /* 物品拿取侦测规则，最多4个 */
    NET_AlarmSchedule_S stAlarmSchedule;             /* 布防时间 */
    NET_LinkageList_S stLinkageList;                 /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_ObjectRemovalAlarmInfo_S;

typedef NET_ObjectRemovalAlarmInfo_S* pNET_ObjectRemovalAlarmInfo_S;

/**
 * @struct tagNETTVAudioAnomalyAlarmInfo
 * @brief 音频异常侦测告警配置信息 Audio anomaly detection alarm configuration
 * @note 用于NET_GET_AUDIOANOMALYALARM/NET_SET_AUDIOANOMALYALARM
 */
typedef struct tagNET_AudioAnomalyAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    BOOL        bAudioInputAnomaly;                  /* 音频输入异常检测是否启用 */
    BOOL        bUpEnable;                           /* 音量突升检测是否启用 */
    INT32       nUpSensitivity;                      /* 音量突升灵敏度 [1,100] */
    INT32       nUpThreshold;                        /* 音量突升阈值 [1,100] */
    BOOL        bDownEnable;                         /* 音量突降检测是否启用 */
    INT32       nDownSensitivity;                    /* 音量突降灵敏度 [1,100] */
    NET_AlarmSchedule_S stAlarmSchedule;             /* 布防时间 */
    NET_LinkageList_S stLinkageList;                 /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_AudioAnomalyAlarmInfo_S;

typedef NET_AudioAnomalyAlarmInfo_S* pNET_AudioAnomalyAlarmInfo_S;

/* 音频异常侦测实时音量结构体预留字段长度。 */
#define NET_AUDIO_ANOMALY_CURRENT_DB_RESERVED_LEN (120)

/**
 * @brief 音频异常侦测实时音量信息
 * @author ITC
 * @note 用于 NET_GET_AUDIO_ANOMALY_CURRENT_DB，仅表示本次查询时的音量快照
 */
typedef struct tagNET_AudioAnomalyCurrentDb
{
    BOOL        bValid;                              /* 实时音量是否有效 */
    FLOAT       fCurrentDb;                          /* 当前实时音量，单位：dB */
    BYTE        abyReserved[NET_AUDIO_ANOMALY_CURRENT_DB_RESERVED_LEN]; /* 预留字段 */
} NET_AudioAnomalyCurrentDb_S;

typedef NET_AudioAnomalyCurrentDb_S* pNET_AudioAnomalyCurrentDb_S;

/* ==================== FaceCapture人脸抓拍相关结构体 =================== */

/**
 * @struct tagNETTVFaceCaptureRegion
 * @brief 人脸抓拍区域（多边形）
 */
typedef struct tagNET_FaceCaptureRegion
{
    INT32       uPointCount;                        /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_FaceCaptureRegion_S;

typedef NET_FaceCaptureRegion_S* pNET_FaceCaptureRegion_S;

/**
 * @struct tagNET_FaceCaptureRule
 * @brief 人脸抓拍规则参数 Face capture rule
 */
typedef struct tagNET_FaceCaptureRule
{
    INT32                           nSensitivity;                    /* 灵敏度[1,100] */
    NET_FaceCaptureRegion_S    stRegion;                        /* 规则区域 */
    INT32                           uShieldRegionCount;             /* 屏蔽区域数量，最多4个 */
    NET_FaceCaptureRegion_S    astShieldRegion[4];              /* 屏蔽区域，最多4个 */
    INT32                           nMinIpdRectLeft;                 /* 最小瞳距区域左坐标 */
    INT32                           nMinIpdRectTop;                  /* 最小瞳距区域上坐标 */
    INT32                           nMinIpdRectRight;                /* 最小瞳距区域右坐标 */
    INT32                           nMinIpdRectBottom;               /* 最小瞳距区域下坐标 */
    INT32                           nMinWidth;                       /* 最小瞳距宽度 */
    INT32                           nMinHeight;                      /* 最小瞳距高度 */
    INT32                           nMaxWidth;                       /* 最大瞳距宽度 */
    INT32                           nMaxHeight;                      /* 最大瞳距高度 */
    INT32                           nInterval;                       /* 抓拍间隔 */
    BYTE                            byRes[128];                      /* 保留字段 */
}NET_FaceCaptureRule_S;

typedef NET_FaceCaptureRule_S* pNET_FaceCaptureRule_S;

/**
 * @struct tagNET_FaceCaptureInfo
 * @brief 人脸抓拍配置信息 Face capture configuration
 * @note 用于NET_GET_FACECAPTUREINFO/NET_SET_FACECAPTUREINFO
 */
typedef struct tagNET_FaceCaptureInfo
{
    BOOL                        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    NET_FaceCaptureRule_S  stRule;                             /* 人脸抓拍规则 */
    NET_AlarmSchedule_S     stAlarmSchedule;                    /* 布防时间 */
    NET_LinkageList_S       stLinkageList;                      /* 联动配置 */
    BYTE                        byRes[256];                         /* 保留字段 */
}NET_FaceCaptureInfo_S;

typedef NET_FaceCaptureInfo_S* pNET_FaceCaptureInfo_S;

/** @brief 人脸抓拍图片叠加配置。 */
typedef struct tagNET_FaceCaptureOverlayInfo
{
    INT32 nDeviceID;
    CHAR strMonitoryPointInfo[NET_LEN_256];
    BOOL bOverlayDeviceID;
    BOOL bOverlayCaptureTime;
    BOOL bOverlayMonitoryPointInfo;
    NET_OSD_COLOR_E enFontColor;
    CHAR strFontColor[NET_LEN_16];
    BYTE byRes[128];
} NET_FaceCaptureOverlayInfo_S;

typedef NET_FaceCaptureOverlayInfo_S* pNET_FaceCaptureOverlayInfo_S;

/**
 * @struct tagNET_FaceCompareInfo
 * @brief 人脸比对配置信息 Face compare configuration
 * @note 用于NET_SET_FACE_COMPARE_INFO
 */
typedef struct tagNET_FaceCompareInfo
{
    BOOL                    bEnable;                    /* 是否启用 0-不启用 1-启用 */
    NET_AlarmSchedule_S stAlarmSchedule;            /* 布防时间 */
    NET_LinkageList_S   stLinkageListSuccess;       /* 比对成功联动配置 */
    NET_LinkageList_S   stLinkageListFail;          /* 比对失败联动配置 */
    BYTE                    byRes[256];                 /* 保留字段 */
}NET_FaceCompareInfo_S;

typedef NET_FaceCompareInfo_S* pNET_FaceCompareInfo_S;

/**
 * @struct tagNETTVFaceLibInfo
 * @brief 目标库信息 Face library information
 * @note 用于NET_ADD_TARGET_LIB/NET_DEL_TARGET_LIB/NET_SET_TARGET_LIB
 */
typedef struct tagNET_FaceLibInfo
{
    CHAR    szFaceLibName[NET_FACE_DB_NAME_LEN];     /* 目标库名称 */
    INT32   nTotalFace;                                 /* 总人脸数 */
    INT32   nNormalNum;                                 /* 正常个数 */
    INT32   nAbnormalNum;                               /* 异常个数 */
    BYTE    byRes[256];                                 /* 保留字段 */
}NET_FaceLibInfo_S;

typedef NET_FaceLibInfo_S* pNET_FaceLibInfo_S;

/**
 * @struct tagNETTVFaceLibList
 * @brief 目标库列表 Face library list
 * @note 用于NET_GET_TARGET_LIB
 */
typedef struct tagNET_FaceLibList
{
    INT32                   nTargetLibCount;                            /* 目标库数量 */
    NET_FaceLibInfo_S  astTargetLibInfos[NET_FACE_LIB_MAX_NUM]; /* 目标库信息列表 */
    BYTE                    byRes[256];                                 /* 保留字段 */
}NET_FaceLibList_S;

typedef NET_FaceLibList_S* pNET_FaceLibList_S;

/**
 * @struct tagNETTVFaceIdInfo
 * @brief 人脸ID信息 Face id information
 * @note 用于NET_DEL_FACE_INFO
 */
typedef struct tagNET_FaceIdInfo
{
    INT32   nIdCount;                                   /* 人脸ID数量 */
    INT32   anIds[NET_FACE_ID_MAX_NUM];              /* 人脸ID列表 */
    BYTE    byRes[256];                                 /* 保留字段 */
}NET_FaceIdInfo_S;

typedef NET_FaceIdInfo_S* pNET_FaceIdInfo_S;

/**
 * @struct tagNETTVFaceInfo
 * @brief 人脸信息 Face information
 * @note 用于NET_ADD_FACE_INFO/NET_SET_FACE_INFO
 */
typedef struct tagNET_FaceInfo
{
    INT32   nId;                                        /* 人脸ID */
    CHAR    szFaceLibName[NET_FACE_DB_NAME_LEN];     /* 名单组名称 */
    CHAR    szName[NET_FACE_MEMBER_NAME_LEN];        /* 名字 */
    CHAR    szPhoneNum[NET_FACE_IDNUMBER_LEN];       /* 联系方式 */
    CHAR    szPicPath[NET_LEN_260];                  /* 图片完整路径/名称 */
    CHAR    szBinPath[NET_LEN_260];                  /* 图片二进制完整路径/名称 */
    CHAR    szPicType[NET_LEN_64];                   /* 图片类型 */
    INT32   nPicSize;                                   /* 图片大小 */
    CHAR    szPicDate[NET_LEN_64];                   /* 图片日期 */
    INT32   nModelState;                                /* 模型状态, 0未处理，1成功，-1失败 */
    INT32   nRatingLevel;                               /* 评估等级, 0全部，1评分未知，2低，3高 */
    BYTE    byRes[256];                                 /* 保留字段 */
}NET_FaceInfo_S;

typedef NET_FaceInfo_S* pNET_FaceInfo_S;

/**
 * @struct tagNETTVFaceInfoList
 * @brief 人脸信息列表 Face information list
 * @note 用于NET_GET_FACE_INFO
 */
typedef struct tagNET_FaceInfoList
{
    INT32               nFaceInfoCount;                         /* 人脸信息数量 */
    NET_FaceInfo_S  astFaceInfos[NET_FACE_INFO_MAX_NUM]; /* 人脸信息列表 */
    BYTE                byRes[256];                             /* 保留字段 */
}NET_FaceInfoList_S;

typedef NET_FaceInfoList_S* pNET_FaceInfoList_S;

/**
 * @brief 报警设备信息结构体
 * @note  用于报警推送，包含设备的序列号、名称、IP地址等信息
 */
typedef struct tagNET_Alarmer
{
    LPVOID lpUserID;                            /* NET_Login()返回值, 布防时有效 */
    BYTE    strSerialNumber[NET_LEN_64];     /* 序列号 */
    CHAR    strDeviceName[NET_LEN_32];       /* 设备名字 */
    BYTE    byMacAddr[NET_LEN_6];           /* MAC地址 */
    CHAR    strDeviceIP[128];                   /* IP地址 */
    BYTE    byReserved[12];                     /* 预留字段 */
} NET_Alarmer_S;

/**
 * @brief 报警设备信息结构体指针类型
 */
typedef NET_Alarmer_S* pNET_Alarmer_S;

/**
 * @struct tagNETTVVideoResolution
 * @brief 视频源分辨率信息 Video resolution
 * @attention 无
*/
typedef struct tagNET_VideoResolution
{
    CHAR  szName[NET_LEN_32];                              /* 分辨率名称, 如 1920*1080 */
    INT32 uWidth;                                             /*  视频编码分辨率 */
    INT32 uHeight;                                            /*  视频编码分辨率 */
    FLOAT fFrameRateMin;                                      /*  该分辨率支持的最小帧率fps */
    FLOAT fFrameRateMax;                                      /*  该分辨率支持的最大帧率fps */
    INT32 uFrameRateNum;                                      /*  该分辨率支持的帧率数量 */
    FLOAT afFrameRate[NET_VIDEO_FRAME_RATE_MAX_NUM];       /*  该分辨率支持的帧率fps数组 */
    INT32 uBitRateMin;                                        /*  该分辨率支持的最小码率kbps */
    INT32 uBitRateMax;                                        /*  该分辨率支持的最大码率kbps */
}NET_VideoResolution_S;

typedef NET_VideoResolution_S* pNET_VideoResolution_S;

/**
 * @struct tagNETTVRange
 * @brief 取值范围 Range
 * @attention 无
*/
typedef struct tagNET_Range
{
    INT32   uMin;                         /* 最小值 */
    INT32   uMax;                         /* 最大值 */
}NET_Range_S;

typedef NET_Range_S* pNET_Range_S;

/**
 * @struct tagNET_TVVideoEncodeOption
 * @brief 视频编码参数选项 Video encode option
 * @attention 无
*/
typedef struct tagNET_VideoEncodeOption
{
    INT32                       nId;                                                /* 视频码流ID 0-主码流 1-子码流 2-JPEG */
    INT32                       enVideoType;                                       /* 视频类型 0-复合流 1-视频流 */
    NET_VideoResolution_S   stVideoResolution;                                 /* 视频分辨率 */
    INT32                       enBitrateType;                                     /* 码率类型 */
    INT32                       enImageQuality;                                    /* 图像质量 */
    INT32                       enFrameRate;                                       /* 视频帧率fps */
    INT32                       nBitrateUpperLimit;                                /* 码率上限kbps */
    INT32                       nAverageBitrate;                                   /* 平均码率kbps */
    INT32                       enVideoCodec;                                      /* 视频编码 NET_VIDEO_CODE_TYPE_E */
    BOOL                        bSmartEnable;                                      /* 智能编码 */
    INT32                       enEncodingComplexity;                              /* 编码复杂度 */
    INT32                       nIFrameInterval;                                   /* I帧间隔 */
    INT32                       enSvcEnable;                                       /* SVC智能编码 */
    INT32                       nBitrateSmoothing;                                 /* 码流平滑 */
    BYTE                        byRes[256];                                        /* 保留字段 */
}NET_VideoEncodeOption_S;

typedef NET_VideoEncodeOption_S* pNET_VideoEncodeOption_S;

/**
 * @struct tagNET_TVVideoEncodeAbility
 * @brief 单个编码格式能力 Video encode ability
 * @attention 对应 Video_NS::EncodeAbility_S
*/
typedef struct tagNET_VideoEncodeAbility
{
    CHAR                        szVideoCodec[NET_LEN_32];                       /* 视频编码字符串, 如 H.264/H.265 */
    INT32                       enVideoCodec;                                      /* 视频编码 NET_VIDEO_CODE_TYPE_E */
    INT32                       nSupportAdjustComplexity;                          /* 是否支持调整编码复杂度 */
    INT32                       anEncodeComplexity[NET_VIDEO_ENCODE_COMPLEXITY_MAX_NUM]; /* 支持的编码复杂度 */
    INT32                       nEncodeComplexityNum;                              /* 编码复杂度有效个数 */
    UINT32                      nDefaultComplexity;                                /* 默认编码复杂度 */
    INT32                       bSupportSVC;                                       /* 是否支持 SVC */
    INT32                       bSupportStreamSmooth;                              /* 是否支持码流平滑 */
    BYTE                        byRes[64];                                         /* 保留字段 */
}NET_VideoEncodeAbility_S;

typedef NET_VideoEncodeAbility_S* pNET_VideoEncodeAbility_S;

/**
 * @struct tagNET_TVVideoStreamCap
 * @brief 视频码流参数能力集 Video stream CapNET_CAP_OSD
 * @attention 无
*/
typedef struct tagNET_VideoStreamCap
{
    INT32                           uStreamType;                                   /* 码流类型 入参 参见 NET_LIVE_STREAM_INDEX_E */
    INT32                           bSupportMultiStream;                            /* 是否支持复合流(包含音频) Support multi stream */
    INT32                           uEncodeCapSize;                                /* 编码能力集个数 Encode capability size */
    NET_VideoEncodeOption_S    astEncodeCap[NET_VIDEO_ENCODE_TYPE_MAX];     /* 编码能力 Encode capability */
    NET_Range_S                  stQuality;                                      /* 图像质量范围 Quality range */
    NET_Range_S                  stStreamSmooth;                                 /* 码流平滑范围 Stream smooth range */
    INT32                           uResolutionNum;                                /* 支持的分辨率个数 Number of supported resolutions */
    NET_VideoResolution_S       astResolution[NET_RESOLUTION_NUM_MAX];       /* 支持的分辨率列表 Supported resolution list */
    INT32                           uEncodeTypeNum;                                /* 编码格式有效个数 */
    INT32                           uEncodeAbilityNum;                             /* 编码能力有效个数 */
    NET_VideoEncodeAbility_S   astEncodeAbility[NET_VIDEO_ENCODE_TYPE_MAX]; /* 编码格式能力列表 */
    INT32                           uIFrameIntervalMin;                            /* I帧间隔最小值 */
    INT32                           uIFrameIntervalMax;                            /* I帧间隔最大值 */
}NET_VideoStreamCap_S;

typedef NET_VideoStreamCap_S* pNET_VideoStreamCap_S;

#define NET_VIDEO_STREAM_MAX         4             /* 最大码流数量 */

/**
 * @struct tagNET_VideoEncodeCap
 * @brief 视频编码能力集(多码流) Video Encode Capability
 * @attention 包含所有码流的能力集信息
*/
typedef struct tagNET_VideoEncodeCap
{
    INT32                       uStreamCount;                                  /* 码流数量 Stream count */
    NET_VideoStreamCap_S   astStreamCap[NET_VIDEO_STREAM_MAX];          /* 各码流能力 Stream capabilities */
}NET_VideoEncodeCap_S;

typedef NET_VideoEncodeCap_S* pNET_VideoEncodeCap_S;

/* ==================== AUDIO能力结构体Start =================== */

/**
 * @struct tagNETTVAudioRange
 * @brief 音频数值范围 Audio range
 * @attention 无
 */
typedef struct tagNET_AudioRange
{
    INT32   bEnable;    /* 是否启用范围约束，0-不校验，1-校验 */
    INT32   uMin;       /* 最小值 */
    INT32   uMax;       /* 最大值 */
    INT32   uStep;      /* 步长，<=0 表示不做步长校验 */
}NET_AudioRange_S;

typedef NET_AudioRange_S* pNET_AudioRange_S;

/**
 * @struct tagNETTVAudioFormatCap
 * @brief 音频格式能力集 Audio format capability
 * @attention 无
 */
typedef struct tagNET_AudioFormatCap
{
    INT32                   uFormat;                               /* 音频格式 参见 NET_AudioFormat_E */
    INT32                   uSampleRateSize;                       /* 采样率数量 */
    INT32                   auSampleRate[NET_AUDIO_SAMPRATE_MAX]; /* 采样率列表 参见 NET_AudioSampRate_E */

    INT32                   uBitRateSize;                          /* 码率数量 */
    INT32                   auBitRate[NET_AUDIO_BITRATE_MAX];  /* 码率列表 参见 NET_AudioBitRate_E */

    NET_AudioRange_S    stSampleRateRange;                      /* 采样率范围（预留） */
    NET_AudioRange_S    stBitRateRange;                         /* 码率范围（预留） */
}NET_AudioFormatCap_S;

typedef NET_AudioFormatCap_S* pNET_AudioFormatCap_S;

/**
 * @struct tagNETTVAudioEncodeCap
 * @brief 音频编码能力集 Audio encode capability
 * @attention 无
 */
typedef struct tagNET_AudioCap
{
    INT32                       uInputTypeSize;                           /* 输入类型数量 */
    INT32                       auInputType[NET_AUDIO_INPUT_TYPE_MAX];   /* 输入类型 参见 NET_AudioInputType_E */

    INT32                       uOutputTypeSize;                          /* 输出类型数量 */
    INT32                       auOutputType[NET_AUDIO_OUTPUT_TYPE_MAX]; /* 输出类型 参见 NET_AudioOutputType_E */

    INT32                       uFormatSize;                              /* 音频格式数量 */
    INT32                       auFormat[NET_AUDIO_FORMAT_MAX];        /* 音频格式 参见 NET_AudioFormat_E */

    INT32                       uFormatDetailSize;                        /* 音频格式详细能力数量 */
    NET_AudioFormatCap_S   astFormatDetail[NET_AUDIO_FORMAT_MAX];  /* 各音频格式详细能力 */
}NET_AudioCap_S;

typedef NET_AudioCap_S* pNET_AudioCap_S;

/* ==================== OSD相关结构体Start =================== */

/* OSD对齐方式 */
typedef enum
{
    OSD_ALIFN_CUSTOMIZE = 0,   /* 自定义 */
    OSD_ALIFN_CHARACTER_LEFT,  /* 字符左对齐 */
    OSD_ALIFN_CHARACTER_RIGHT, /* 字符右对齐 */
    OSD_ALIFN_ALL_LEFT,        /* 全部左对齐 */
    OSD_ALIFN_ALL_RIGHT,       /* 全部右对齐 */
    OSD_ALIFN_GB_MODE          /* 国标模式 */
} OSD_ALIGN_E;
/* OSD颜色 */
typedef enum
{
    OSD_COLOR_BLACK = 0, /* 黑色 */
    OSD_COLOR_WHITE,     /* 白色 */
    OSD_COLOR_CUSTOMIZE  /* 自定义 */
} OSD_COLOR_E;
/* OSD字体大小 */
typedef enum
{
    OSD_FONT_SIZE_ADAPTIVE = 0, /* 自适应 */
    OSD_FONT_SIZE_16,           /* 16 * 16 */
    OSD_FONT_SIZE_32,           /* 32 * 32 */
    OSD_FONT_SIZE_48,           /* 48 * 48 */
    OSD_FONT_SIZE_64            /* 64 * 64 */
} OSD_FONT_SIZE_E;
/* OSD属性 */
typedef enum
{
    OSD_ATTR_ALPHA_N_FLASH_N = 0, /* 不透明，不闪烁 */
    OSD_ATTR_ALPHA_N_FLASH_Y,     /* 不透明，  闪烁 */
    OSD_ATTR_ALPHA_Y_FLASH_N,     /*   透明，不闪烁 */
    OSD_ATTR_ALPHA_Y_FLASH_Y      /*   透明，  闪烁 */
} OSD_ATTRIBUTE_E;
/* OSD时间格式 */
typedef enum
{
    OSD_TIME_FORMAT_24 = 0, /* 24小时制 */
    OSD_TIME_FORMAT_12      /* 12小时制 */
} OSD_TIME_FORMAT_E;

/* OSD日期格式 */
typedef enum
{
    ENGLISH_YYYY_MM_DD = 0, /* YYYY-MM-DD(年月日), 例如 2024-10-01 */
    ENGLISH_MM_DD_YYYY,     /* MM-DD-YYYY(月日年), 例如 10-01-2024 */
    ENGLISH_DD_MM_YYYY,     /* DD-MM-YYYY(日月年), 例如 01-10-2024 */
    CHINESE_YYYYMMDD,       /* YYYY年MM月DD日,     例如 2024年10月01日 */
    CHINESE_MMDDYYYY,       /* MM月DD日YYYY年,     例如 10月01日2024年 */
    CHINESE_DDMMYYYY,       /* DD日MM月YYYY年,     例如 01日10月2024年 */
    ENGLISH_YYYYMMDD,       /* YYYY/MM/DD(年月日), 例如 2024/10/01 */
    ENGLISH_MMDDYYYY,       /* MM/DD/YYYY(月日年), 例如 10/01/2024 */
    ENGLISH_DDMMYYYY        /* DD/MM/YYYY(日月年), 例如 01/10/2024 */
} OSD_DATE_FORMAT_E;
 /* OSD状态信息 */
typedef struct _OsdAttribute_S_
{
    INT32 nX;                               /* 横坐标 */
    INT32 nY;                               /* 纵坐标 */
    INT32 nW;                               /*插件小窗口中osd显示框的宽 <=0表示运行时自适应 */
    INT32 nH;                               /* 插件小窗口中osd显示框的高 */
    OSD_ATTRIBUTE_E enAttribute;            /* OSD属性 */
    OSD_FONT_SIZE_E enFontSize;             /* OSD字体大小 */
    OSD_COLOR_E enFontColor;                /* OSD颜色 */
    CHAR strFontColor[NET_LEN_16];       /* OSD颜色为自定义时，使用，记录RGB颜色 格式:"#000000"" */
    CHAR strToken[NET_LEN_512];          /* OSD token Onvif使用 */
} OsdAttribute_S;
 /* OSD时间信息 */
typedef struct _OsdTimeInfo_S_
{
    BOOL bEnable;                           /* 是否启用 */
    BOOL bEnableWeek;                       /* 是否启用星期 */
    OSD_TIME_FORMAT_E enTimeFormat;         /* OSD时间格式 */
    OSD_DATE_FORMAT_E enDateFormat;         /* OSD日期格式 */
    OsdAttribute_S stOsdAttr;               /* OSD状态信息 */
} OsdTimeInfo_S;
/* OSD名称信息 */
typedef struct _OsdNameInfo_S_
{
    BOOL bEnable;                           /* 是否启用 */
    CHAR strName[NET_LEN_128];           /* 名称 */
    OsdAttribute_S stOsdAttr;               /* OSD状态信息 */
} OsdNameInfo_S;
/* OSD字符叠加信息 */
typedef struct _OsdInfo_S_
{
    INT32 nId;                              /* 字符ID */
    BOOL bEnable;                           /* 是否启用 */
    CHAR strName[NET_LEN_128];           /* 字符串名称 */
    OsdAttribute_S stOsdAttr;               /* OSD状态信息 */
} OsdInfo_S;
/* OSD配置信息 */
typedef struct tagNET_VideoOsdCfg
{
    OSD_ALIGN_E enAlign;                            /* OSD对齐方式 */
    OsdNameInfo_S stOsdNameInfo;                    /* OSD名称信息 */
    OsdTimeInfo_S stOsdTimeInfo;                    /* OSD时间信息 */
    OsdInfo_S OsdInfo[32];                          /* OSD字符叠加信息 */
    BYTE        byRes[64];                          /* 保留字段 */
}NET_VideoOsdCfg_S;

typedef NET_VideoOsdCfg_S* pNET_VideoOsdCfg_S;

/* ==================== OSD相关结构体End ===================== */

/**
 * @struct tagNETTVOsdCap
 * @brief 通道OSD的能力集 OSD Capabilities (简化版，对应OsdConfig_S)
 * @attention
 */
typedef struct tagNET_OsdCap
{
    /* 基础能力 */
    BOOL     bSupportOsd;                                                       /* 是否支持OSD配置 */
    BOOL     bSupportName;                                                      /* 是否支持名称OSD */
    BOOL     bSupportTime;                                                      /* 是否支持时间OSD */
    BOOL     bSupportWeek;                                                      /* 是否支持星期显示 */
    BOOL     bSupportCustomColor;                                               /* 是否支持自定义颜色 */

    /* 字符叠加能力 */
    UINT32   udwMaxOsdNum;                                                      /* 最大字符叠加数量 */

    /* 字体大小能力 */
    UINT32   udwSupportedFontSizeNum;                                           /* 支持的字体大小个数 */
    UINT32   audwSupportedFontSizeList[NET_OSD_FONT_SIZE_TYPE_MAX_NUM];      /* 支持的字体大小列表 NET_OSD_FONT_SIZE_E */

    /* 日期格式能力 */
    UINT32   udwSupportedDateFormatNum;                                         /* 支持的日期格式数量 */
    UINT32   audwSupportedDateFormatList[NET_OSD_DATE_FORMAT_MAX_NUM];       /* 支持的日期格式列表 NET_OSD_DATE_FORMAT_E */

    /* 时间格式能力 */
    UINT32   udwSupportedTimeFormatNum;                                         /* 支持的时间格式数量 */
    UINT32   audwSupportedTimeFormatList[NET_OSD_TIME_FORMAT_MAX_NUM];       /* 支持的时间格式列表 NET_OSD_TIME_FORMAT_E */

    /* 对齐方式能力 */
    UINT32   udwSupportedAlignNum;                                              /* 支持的对齐方式数量 */
    UINT32   audwSupportedAlignList[8];                                         /* 支持的对齐方式列表 NET_OSD_ALIGN_E */

    BYTE     byRes[256];                                                        /* 保留字段  Reserved */
} NET_OsdCap_S;

typedef NET_OsdCap_S* pNET_OsdCap_S;

/******************** 智能能力集结构体定义 Smart Capability Structures ********************/

/**
 * @struct tagNETTVMotionDetectCap
 * @brief 移动侦测能力  Motion detection capability
 * @attention
 */
typedef struct tagNET_MotionDetectCap
{
    BOOL    bSupport;                           /* 是否支持移动侦测  Support motion detection */
    BOOL    bSupportExpertMode;                 /* 是否支持专家模式  Support expert mode */
    BOOL    bSupportDynamicAnalysis;            /* 是否支持动态分析  Support dynamic analysis */
    UINT32  udwGridMaxWidth;                    /* 网格最大宽度  Grid max width */
    UINT32  udwGridMaxHeight;                   /* 网格最大高度  Grid max height */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportRectRegion;                 /* 是否支持矩形区域  Support rectangle region */
    BOOL    bSupportGridRegion;                 /* 是否支持网格区域  Support grid region */
    UINT32  udwExpertMaxAreas;                  /* 专家模式最大区域数  Expert mode max areas */
    BOOL    bSupportDayNightSwitch;             /* 是否支持日夜切换  Support day night switch */
    BOOL    bSupportAutoSwitch;                 /* 是否支持自动切换  Support auto switch */
    BOOL    bSupportTimedSwitch;                /* 是否支持定时切换  Support timed switch */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_MotionDetectCap_S;

typedef NET_MotionDetectCap_S* pNET_MotionDetectCap_S;

/**
 * @struct tagNETTVTamperDetectCap
 * @brief 遮挡检测能力  Tamper detection capability
 * @attention
 */
typedef struct tagNET_TamperDetectCap
{
    BOOL    bSupport;                           /* 是否支持遮挡检测  Support tamper detection */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportCustomRegion;               /* 是否支持自定义区域  Support custom region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TamperDetectCap_S;

typedef NET_TamperDetectCap_S* pNET_TamperDetectCap_S;

/**
 * @struct tagNETTVBoundaryDetectCap
 * @brief 越界检测能力  Boundary detection capability
 * @attention
 */
typedef struct tagNET_BoundaryDetectCap
{
    BOOL    bSupport;                           /* 是否支持越界检测  Support boundary detection */
    UINT32  udwMaxLines;                        /* 最大警戒线数  Max lines */
    UINT32  udwMaxPointsPerLine;                /* 每条线最大顶点数  Max points per line */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportDirection;                  /* 是否支持方向检测  Support direction */
    BOOL    bSupportTargetFilter;               /* 是否支持目标过滤  Support target filter */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_BoundaryDetectCap_S;

typedef NET_BoundaryDetectCap_S* pNET_BoundaryDetectCap_S;

/**
 * @struct tagNETTVIntrusionDetectCap
 * @brief 区域入侵检测能力  Intrusion detection capability
 * @attention
 */
typedef struct tagNET_IntrusionDetectCap
{
    BOOL    bSupport;                           /* 是否支持区域入侵检测  Support intrusion detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    UINT32  udwMinPointsPerRegion;              /* 每个区域最小顶点数  Min points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_Range_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BOOL    bSupportTargetFilter;               /* 是否支持目标过滤  Support target filter */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_IntrusionDetectCap_S;

typedef NET_IntrusionDetectCap_S* pNET_IntrusionDetectCap_S;

/**
 * @struct tagNETTVEnterExitDetectCap
 * @brief 进入/离开区域检测能力  Enter/Exit detection capability
 * @attention
 */
typedef struct tagNET_EnterExitDetectCap
{
    BOOL    bSupportEnter;                      /* 是否支持进入区域检测  Support enter detection */
    BOOL    bSupportExit;                       /* 是否支持离开区域检测  Support exit detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    UINT32  udwMinPointsPerRegion;              /* 每个区域最小顶点数  Min points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_Range_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BOOL    bSupportTargetFilter;               /* 是否支持目标过滤  Support target filter */
    BOOL    bSupportConfidence;                 /* 是否支持可信度设置  Support confidence */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_EnterExitDetectCap_S;

typedef NET_EnterExitDetectCap_S* pNET_EnterExitDetectCap_S;

/**
 * @struct tagNETTVLoiteringDetectCap
 * @brief 徘徊检测能力  Loitering detection capability
 * @attention
 */
typedef struct tagNET_LoiteringDetectCap
{
    BOOL    bSupport;                           /* 是否支持徘徊检测  Support loitering detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_Range_S stTimeThreshold;             /* 徘徊时长范围  Loitering time range */
    BOOL    bSupportTargetFilter;               /* 是否支持目标过滤  Support target filter */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_LoiteringDetectCap_S;

typedef NET_LoiteringDetectCap_S* pNET_LoiteringDetectCap_S;

/**
 * @struct tagNETTVCrowdGatheringCap
 * @brief 人群聚集检测能力  Crowd gathering detection capability
 * @attention
 */
typedef struct tagNET_CrowdGatheringCap
{
    BOOL    bSupport;                           /* 是否支持人群聚集检测  Support crowd gathering detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stObjectOccupancy;           /* 人体面积占比阈值  Object occupancy threshold */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_CrowdGatheringCap_S;

typedef NET_CrowdGatheringCap_S* pNET_CrowdGatheringCap_S;

/**
 * @struct tagNETTVParkingDetectCap
 * @brief 停车检测能力  Parking detection capability
 * @attention
 */
typedef struct tagNET_ParkingDetectCap
{
    BOOL    bSupport;                           /* 是否支持停车检测  Support parking detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_Range_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_ParkingDetectCap_S;

typedef NET_ParkingDetectCap_S* pNET_ParkingDetectCap_S;

/**
 * @struct tagNETTVObjectChangeDetectCap
 * @brief 物品遗留/移走检测能力  Object left/removal detection capability
 * @attention
 */
typedef struct tagNET_ObjectChangeDetectCap
{
    BOOL    bSupportUnattendedObject;           /* 是否支持物品遗留检测  Support unattended object detection */
    BOOL    bSupportObjectRemoval;              /* 是否支持物品移走检测  Support object removal detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_Range_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_ObjectChangeDetectCap_S;

typedef NET_ObjectChangeDetectCap_S* pNET_ObjectChangeDetectCap_S;

/**
 * @struct tagNETTVFaceDetectCap
 * @brief 人脸检测能力  Face detection capability
 * @attention
 */
typedef struct tagNET_FaceDetectCap
{
    BOOL    bSupport;                           /* 是否支持人脸检测  Support face detection */
    BOOL    bSupportDynamicAnalysis;            /* 是否支持动态分析  Support dynamic analysis */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportDetectionRegion;            /* 是否支持检测区域设置  Support detection region */
    UINT32  udwMaxPointsPerRegion;              /* 检测区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_FaceDetectCap_S;

typedef NET_FaceDetectCap_S* pNET_FaceDetectCap_S;

/**
 * @struct tagNETTVFaceCaptureCap
 * @brief 人脸抓拍能力  Face capture capability
 * @attention
 */
typedef struct tagNET_FaceCaptureCap
{
    BOOL    bSupport;                           /* 是否支持人脸抓拍  Support face capture */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportDetectionRegion;            /* 是否支持检测区域  Support detection region */
    BOOL    bSupportShieldedRegion;             /* 是否支持屏蔽区域  Support shielded region */
    UINT32  udwMaxShieldedRegions;              /* 最大屏蔽区域数  Max shielded regions */
    BOOL    bSupportIPD;                        /* 是否支持瞳距设置  Support IPD setting */
    NET_Range_S stMinIPD;                    /* 最小瞳距范围  Min IPD range */
    NET_Range_S stMaxIPD;                    /* 最大瞳距范围  Max IPD range */
    NET_Range_S stCaptureInterval;           /* 抓拍间隔范围  Capture interval range */
    BOOL    bSupportOverlay;                    /* 是否支持叠加信息  Support overlay */
    BOOL    bSupportFaceAttribute;              /* 是否支持人脸属性  Support face attribute */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_FaceCaptureCap_S;

typedef NET_FaceCaptureCap_S* pNET_FaceCaptureCap_S;

/**
 * @struct tagNETTVPetRecognitionCap
 * @brief 宠物识别能力  Pet recognition capability
 * @attention
 */
typedef struct tagNET_PetRecognitionCap
{
    BOOL    bSupport;                           /* 是否支持宠物识别  Support pet recognition */
    BOOL    bSupportDynamicAnalysis;            /* 是否支持动态分析  Support dynamic analysis */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportDetectionRegion;            /* 是否支持检测区域设置  Support detection region */
    UINT32  udwMaxPointsPerRegion;              /* 检测区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_PetRecognitionCap_S;

typedef NET_PetRecognitionCap_S* pNET_PetRecognitionCap_S;

/**
 * @struct tagNETTVAudioAnomalyCap
 * @brief 音频异常检测能力  Audio anomaly detection capability
 * @attention
 */
typedef struct tagNET_AudioAnomalyCap
{
    BOOL    bSupport;                           /* 是否支持音频异常检测  Support audio anomaly detection */
    BOOL    bSupportInputAnomaly;               /* 是否支持输入异常检测  Support input anomaly */
    BOOL    bSupportRise;                       /* 是否支持音量突升检测  Support rise detection */
    BOOL    bSupportFall;                       /* 是否支持音量突降检测  Support fall detection */
    NET_Range_S stRiseSensitivity;           /* 突升灵敏度范围  Rise sensitivity range */
    NET_Range_S stRiseThreshold;             /* 突升阈值范围  Rise threshold range */
    NET_Range_S stFallSensitivity;           /* 突降灵敏度范围  Fall sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_AudioAnomalyCap_S;

typedef NET_AudioAnomalyCap_S* pNET_AudioAnomalyCap_S;

/**
 * @struct tagNETTVSceneChangeCap
 * @brief 场景变更检测能力  Scene change detection capability
 * @attention
 */
typedef struct tagNET_SceneChangeCap
{
    BOOL    bSupport;                           /* 是否支持场景变更检测  Support scene change detection */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_SceneChangeCap_S;

typedef NET_SceneChangeCap_S* pNET_SceneChangeCap_S;

/**
 * @struct tagNETTVFireDetectCap
 * @brief 火灾检测能力  Fire detection capability
 * @attention
 */
typedef struct tagNET_FireDetectCap
{
    BOOL    bSupport;                           /* 是否支持火灾检测  Support fire detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_FireDetectCap_S;

typedef NET_FireDetectCap_S* pNET_FireDetectCap_S;

/**
 * @struct tagNETTVSmokeDetectCap
 * @brief 烟雾检测能力  Smoke detection capability
 * @attention
 */
typedef struct tagNET_SmokeDetectCap
{
    BOOL    bSupport;                           /* 是否支持烟雾检测  Support smoke detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_SmokeDetectCap_S;

typedef NET_SmokeDetectCap_S* pNET_SmokeDetectCap_S;

/**
 * @struct tagNETTVWaterAccumulationCap
 * @brief 积水检测能力  Water accumulation detection capability
 * @attention
 */
typedef struct tagNET_WaterAccumulationCap
{
    BOOL    bSupport;                           /* 是否支持积水检测  Support water accumulation detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_WaterAccumulationCap_S;

typedef NET_WaterAccumulationCap_S* pNET_WaterAccumulationCap_S;

/**
 * @struct tagNETTVTrashOverflowCap
 * @brief 垃圾满溢检测能力  Trash overflow detection capability
 * @attention
 */
typedef struct tagNET_TrashOverflowCap
{
    BOOL    bSupport;                           /* 是否支持垃圾满溢检测  Support trash overflow detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TrashOverflowCap_S;

typedef NET_TrashOverflowCap_S* pNET_TrashOverflowCap_S;

/**
 * @struct tagNETTVBehaviorDetectCap
 * @brief 人员行为检测能力  Personnel behavior detection capability
 * @attention
 */
typedef struct tagNET_BehaviorDetectCap
{
    BOOL    bSupport;                           /* 是否支持人员行为检测  Support behavior detection */
    BOOL    bSupportSleepOnDuty;                /* 是否支持睡岗检测  Support sleep on duty detection */
    BOOL    bSupportLeavePost;                  /* 是否支持离岗检测  Support leave post detection */
    BOOL    bSupportSmoking;                    /* 是否支持抽烟检测  Support smoking detection */
    BOOL    bSupportPhoneUsage;                 /* 是否支持玩手机检测  Support phone usage detection */
    BOOL    bSupportFenceClimbing;              /* 是否支持翻越围栏检测  Support fence climbing detection */
    BOOL    bSupportFallDown;                   /* 是否支持人员倒地检测  Support fall down detection */
    BOOL    bSupportTrip;                       /* 是否支持摔倒检测  Support trip detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_Range_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_BehaviorDetectCap_S;

typedef NET_BehaviorDetectCap_S* pNET_BehaviorDetectCap_S;

/**
 * @struct tagNETTVEnvironmentAnomalyCap
 * @brief 环境异常检测能力  Environment anomaly detection capability
 * @attention
 */
typedef struct tagNET_EnvironmentAnomalyCap
{
    BOOL    bSupport;                           /* 是否支持环境异常检测  Support environment anomaly detection */
    BOOL    bSupportElectricVehicleInElevator;  /* 是否支持电动车入梯检测  Support electric vehicle in elevator */
    BOOL    bSupportGarbageExposure;            /* 是否支持垃圾暴露检测  Support garbage exposure */
    BOOL    bSupportOpenFlame;                  /* 是否支持明火检测  Support open flame */
    BOOL    bSupportManholeCoverAbnormal;       /* 是否支持井盖异常检测  Support manhole cover abnormal */
    BOOL    bSupportBareSoil;                   /* 是否支持裸土检测  Support bare soil */
    BOOL    bSupportHoleProtectionBar;          /* 是否支持坑洞防护栏检测  Support hole protection bar */
    BOOL    bSupportPedestrianIntrusion;        /* 是否支持行人入侵检测  Support pedestrian intrusion */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_EnvironmentAnomalyCap_S;

typedef NET_EnvironmentAnomalyCap_S* pNET_EnvironmentAnomalyCap_S;

/**
 * @struct tagNETTVSafetyEquipmentCap
 * @brief 穿戴规范检测能力  Safety equipment detection capability
 * @attention
 */
typedef struct tagNET_SafetyEquipmentCap
{
    BOOL    bSupport;                           /* 是否支持穿戴规范检测  Support safety equipment detection */
    BOOL    bSupportHelmet;                     /* 是否支持安全帽检测  Support helmet detection */
    BOOL    bSupportReflectiveClothing;         /* 是否支持反光衣检测  Support reflective clothing detection */
    BOOL    bSupportSeatbelt;                   /* 是否支持高空安全带检测  Support seatbelt detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportColorDetect;                /* 是否支持颜色检测  Support color detection */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_SafetyEquipmentCap_S;

typedef NET_SafetyEquipmentCap_S* pNET_SafetyEquipmentCap_S;

/**
 * @struct tagNETTVLicensePlateCap
 * @brief 车牌识别能力  License plate recognition capability
 * @attention
 */
typedef struct tagNET_LicensePlateCap
{
    BOOL    bSupport;                           /* 是否支持车牌识别  Support license plate recognition */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BOOL    bSupportMultiPlate;                 /* 是否支持多车牌识别  Support multi-plate recognition */
    UINT32  udwMaxPlatesPerFrame;               /* 单帧最大车牌数  Max plates per frame */
    NET_Range_S stConfidence;                /* 置信度范围  Confidence range */
    BOOL    bSupportPlateColor;                 /* 是否支持车牌颜色识别  Support plate color recognition */
    BOOL    bSupportPlateType;                  /* 是否支持车牌类型识别  Support plate type recognition */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_LicensePlateCap_S;

typedef NET_LicensePlateCap_S* pNET_LicensePlateCap_S;

/**
 * @struct tagNETTVWrongWayDrivingCap
 * @brief 逆行检测能力  Wrong way driving detection capability
 * @attention
 */
typedef struct tagNET_WrongWayDrivingCap
{
    BOOL    bSupport;                           /* 是否支持逆行检测  Support wrong way driving detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_WrongWayDrivingCap_S;

typedef NET_WrongWayDrivingCap_S* pNET_WrongWayDrivingCap_S;

/**
 * @struct tagNET_IllegalLaneChangeCap
 * @brief 违规变道检测能力  Illegal lane change detection capability
 * @attention
 */
typedef struct tagNET_IllegalLaneChangeCap
{
    BOOL    bSupport;                           /* 是否支持违规变道检测  Support illegal lane change detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_IllegalLaneChangeCap_S;

typedef NET_IllegalLaneChangeCap_S* pNET_IllegalLaneChangeCap_S;

/**
 * @struct tagNET_EmergencyLaneOccupancyCap
 * @brief 应急车道占用检测能力  Emergency lane occupancy detection capability
 * @attention
 */
typedef struct tagNET_EmergencyLaneOccupancyCap
{
    BOOL    bSupport;                           /* 是否支持应急车道占用检测  Support emergency lane occupancy detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_EmergencyLaneOccupancyCap_S;

typedef NET_EmergencyLaneOccupancyCap_S* pNET_EmergencyLaneOccupancyCap_S;

/**
 * @struct tagNET_NonMotorVehicleIntrusionCap
 * @brief 非机动车入侵检测能力  Non-motor vehicle intrusion detection capability
 * @attention
 */
typedef struct tagNET_NonMotorVehicleIntrusionCap
{
    BOOL    bSupport;                           /* 是否支持非机动车入侵检测  Support non-motor vehicle intrusion detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_NonMotorVehicleIntrusionCap_S;

typedef NET_NonMotorVehicleIntrusionCap_S* pNET_NonMotorVehicleIntrusionCap_S;

/**
 * @struct tagNET_ConstructionOccupancyCap
 * @brief 施工占道检测能力  Construction occupancy detection capability
 * @attention
 */
typedef struct tagNET_ConstructionOccupancyCap
{
    BOOL    bSupport;                           /* 是否支持施工占道检测  Support construction occupancy detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_ConstructionOccupancyCap_S;

typedef NET_ConstructionOccupancyCap_S* pNET_ConstructionOccupancyCap_S;

/**
 * @struct tagNET_CongestionCap
 * @brief 拥堵检测能力  Congestion detection capability
 * @attention
 */
typedef struct tagNET_CongestionCap
{
    BOOL    bSupport;                           /* 是否支持拥堵检测  Support congestion detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stDensityThreshold;          /* 密度阈值范围  Density threshold range */
    NET_Range_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BOOL    bSupportVehicleCount;               /* 是否支持车辆计数  Support vehicle counting */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_CongestionCap_S;

typedef NET_CongestionCap_S* pNET_CongestionCap_S;

/**
 * @struct tagNET_IllegalParkingCap
 * @brief 违规停车检测能力  Illegal parking detection capability
 * @attention
 */
typedef struct tagNET_IllegalParkingCap
{
    BOOL    bSupport;                           /* 是否支持违规停车检测  Support illegal parking detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_Range_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BOOL    bSupportVehicleType;                /* 是否支持车辆类型识别  Support vehicle type recognition */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_IllegalParkingCap_S;

typedef NET_IllegalParkingCap_S* pNET_IllegalParkingCap_S;

/**
 * @struct tagNET_SmartCap
 * @brief 智能能力集  Smart capability set
 * @attention 对应 NET_CAP_SMART
 */
typedef struct tagNET_SmartCap
{
    NET_MotionDetectCap_S              stMotionDetect;             /* 移动侦测  Motion detection */
    NET_TamperDetectCap_S              stTamperDetect;             /* 遮挡检测  Tamper detection */
    NET_SceneChangeCap_S               stSceneChange;              /* 场景变更  Scene change */
    NET_AudioAnomalyCap_S              stAudioAnomaly;             /* 音频异常  Audio anomaly */
    NET_BoundaryDetectCap_S            stBoundaryDetect;           /* 越界检测  Boundary detection */
    NET_IntrusionDetectCap_S           stIntrusionDetect;          /* 区域入侵  Intrusion detection */
    NET_EnterExitDetectCap_S          stEnterExitDetect;          /* 进入/离开区域  Enter/Exit detection */
    NET_LoiteringDetectCap_S           stLoiteringDetect;          /* 徘徊检测  Loitering detection */
    NET_CrowdGatheringCap_S            stCrowdGathering;           /* 人群聚集  Crowd gathering */
    NET_ParkingDetectCap_S             stParkingDetect;            /* 停车检测  Parking detection */
    NET_ObjectChangeDetectCap_S       stObjectChange;             /* 物品遗留/移走  Object left/removal */
    NET_FaceDetectCap_S                stFaceDetect;               /* 人脸检测  Face detection */
    NET_FaceCaptureCap_S               stFaceCapture;              /* 人脸抓拍  Face capture */
    NET_PetRecognitionCap_S            stPetRecognition;           /* 宠物识别  Pet recognition */
    NET_FireDetectCap_S                stFireDetect;               /* 火灾检测  Fire detection */
    NET_SmokeDetectCap_S               stSmokeDetect;              /* 烟雾检测  Smoke detection */
    NET_WaterAccumulationCap_S         stWaterAccumulation;        /* 积水检测  Water accumulation */
    NET_TrashOverflowCap_S             stTrashOverflow;            /* 垃圾满溢检测  Trash overflow */
    NET_EnvironmentAnomalyCap_S        stEnvironmentAnomaly;       /* 环境异常检测  Environment anomaly */
    NET_BehaviorDetectCap_S            stBehaviorDetect;           /* 人员行为检测  Behavior detection */
    NET_SafetyEquipmentCap_S           stSafetyEquipment;          /* 穿戴规范检测  Safety equipment */
    NET_LicensePlateCap_S              stLicensePlate;             /* 车牌识别  License plate recognition */
    NET_WrongWayDrivingCap_S          stWrongWayDriving;          /* 逆行检测  Wrong way driving */
    NET_IllegalLaneChangeCap_S        stIllegalLaneChange;        /* 违规变道  Illegal lane change */
    NET_EmergencyLaneOccupancyCap_S   stEmergencyLaneOccupancy;   /* 应急车道占用  Emergency lane occupancy */
    NET_NonMotorVehicleIntrusionCap_S stNonMotorVehicleIntrusion;/* 非机动车入侵  Non-motor vehicle intrusion */
    NET_ConstructionOccupancyCap_S     stConstructionOccupancy;    /* 施工占道  Construction occupancy */
    NET_CongestionCap_S                 stCongestion;               /* 拥堵检测  Congestion detection */
    NET_IllegalParkingCap_S            stIllegalParking;           /* 违规停车  Illegal parking */
    BYTE    byRes[256];                                                 /* 保留字段  Reserved */
} NET_SmartCap_S;

typedef NET_SmartCap_S* pNET_SmartCap_S;

/************************************************************************/
/*              设备发现 Device Discovery                                */
/************************************************************************/
#define NET_DISCOVERY_MCAST_ADDR               "239.225.225.106"
#define NET_DISCOVERY_MCAST_PORT               39581
#define NET_DISCOVERY_TTL                      4

/**
 * @brief 设备发现响应信息结构体
 * @note  用于设备发现流程，接收设备上报的自身网络及固件信息
 */
typedef struct tagNET_DiscoveryDeviceInfo
{
    CHAR    strDeviceName[NET_LEN_64];               /* 设备名称 */
    CHAR    strDeviceID[NET_LEN_64];                 /* 设备唯一标识ID */
    CHAR    strDeviceType[NET_LEN_32];               /* 设备类型编码 */
    CHAR    strIPv4Address[NET_IPADDR_STR_MAX_LEN];  /* IPv4地址（点分十进制） */
    CHAR    strIPv4SubnetMask[NET_IPADDR_STR_MAX_LEN]; /* IPv4子网掩码 */
    CHAR    strIPv4Gateway[NET_IPADDR_STR_MAX_LEN];  /* IPv4默认网关 */
    CHAR    strMACAddress[NET_LEN_32];               /* MAC地址 */
    CHAR    strFirmwareVersion[NET_LEN_64];          /* 固件版本号 */
    UINT32  uHttpPort;                                  /* HTTP服务端口号 */
    CHAR    strManufacturer[NET_LEN_32];             /* 制造商名称 */
    BYTE    byReserved[128];                            /* 预留字段 */
} NET_DiscoveryDeviceInfo_S;

/**
 * @brief 设备发现响应信息结构体指针类型
 */
typedef NET_DiscoveryDeviceInfo_S* pNET_DiscoveryDeviceInfo_S;



/**
 * @file NetTVSDKServerInterface.h
 * @brief SDK服务端接口头文件，定义服务端初始化、配置回调注册、设备发现、语音对讲、录像帧流等核心接口
 * @note 服务端接口采用C风格API，供宿主程序（如NVR、IPC）调用，用于注册回调和推送消息
 */

/************************************************************************/
/*                          SDK服务端核心接口                           */
/************************************************************************/
/**
 * @brief SDK服务端初始化
 * @param [IN] dwPort 服务器端口号
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @param [IN] szDeviceName 设备名称（响应JSON中device_name字段值）
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 * @note
 */
NET_API BOOL STDCALL NET_serverInit(IN UINT32 udwPort,IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132],IN CHAR szDeviceName[NET_LEN_132]);

/**
* SDK 清理  SDK cleaning
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_API BOOL STDCALL NET_serverCleanup(void);

/**
 * @brief 设置日志
 * @param [IN] dwLogLevel   日志的等级（默认为0）：0-表示关闭日志，1-表示只输出ERROR错误日志，2-输出ERROR错误信息和DEBUG调试信息，3-输出ERROR错误信息、DEBUG调试信息和INFO普通信息等所有信息
 * @param [IN] strLogDir    日志路径
 * @param [IN] nLogFileSize 日志文件大小(单位：字节)
 * @param [IN] dwLogFileNum 日志文件个数
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_serverSetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 nLogFileSize,IN INT32 dwLogFileNum);

/**
* 获取SDK的版本信息 Get SDK version information
* @return SDK版本信息 SDK version information
* @note
* - 在两个高字节中高8位表示主版本,低八位表示次版本.两个低字节表示附加版本号如0x01080000：表示版本为1.8.0.0.
* - The two high bytes,The high-8-bit indicate the major version, and the low-8-bytes indicate the minor version.Two low bytes for additional version numbers For example, 0x01080000 means version 1.8.0.0
*/
NET_API INT32 STDCALL NET_serverGetSdkVersion(void);

/**
 * @brief 获取当前在线客户端数量（活跃会话数）
 * @return 客户端数量
 */
NET_API INT32 STDCALL NET_serverGetClientCount(void);

/**
 * @brief 设置用户名密码
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_serverSetUserPassword(IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132]);

/**
 * @brief 推送告警信息
 * @param [IN] pAlarmer    告警设备信息
 * @param [IN] lCommand    命令码(报警类型)，用于客户端按命令码反序列化结构体
 * @param [IN] pAlarmInfo  具体告警结构体指针（类型由 lCommand 决定）
 * @param [IN] dwBufLen    pAlarmInfo 长度（一般为 sizeof(对应结构体)）
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_serverPushAlarmInfo(IN NET_Alarmer_S *pAlarmer,
                                                    IN INT32 lCommand,
                                                    IN LPVOID pAlarmInfo,
                                                    IN INT32 dwBufLen);

/* 专用抓拍入口统一转换为 NET_AlarmCaptureInfo_S + NET_ALARM_CAPTURE_*。 */
NET_API BOOL STDCALL NET_serverPushFaceCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                    IN NET_FaceCapturePushInfo_S* pCaptureInfo);
NET_API BOOL STDCALL NET_serverPushPersonCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                      IN NET_PersonCapturePushInfo_S* pCaptureInfo);
NET_API BOOL STDCALL NET_serverPushMotorvehicleCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                            IN NET_MotorvehicleCapturePushInfo_S* pCaptureInfo);
NET_API BOOL STDCALL NET_serverPushNonMotorvehicleCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                               IN NET_NonMotorvehicleCapturePushInfo_S* pCaptureInfo);

/**
 * @brief 推送通道上下线状态
 * @param [IN] pChannelInfo 通道信息，byOnline/nDevState 表示当前状态
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_API BOOL STDCALL NET_serverPushChannelStatusInfo(IN NET_ChannelInfo_S *pChannelInfo);

/* 注册设备信息获取回调（NVR规模/能力数量：通道数/报警端口数等，BG6_ZHSJ/BU_SJCL专用） */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceInfoCb(NET_COMMON_ECODE_E (*CB)(pNET_DeviceInfo_S pInfo));

/**
 * @brief 设备基本信息回调类型（通用身份信息：型号/序列号/固件/MAC等）
 * @note  NET_DeviceBasicInfo_S 为通用设备身份属性，收口于通用设备回调
 * @param [OUT] pInfo 设备基本信息结构体指针，由回调填充
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetDeviceBasicInfo)(pNET_DeviceBasicInfo_S pInfo);

/**
 * @brief 注册获取设备基本信息回调 (NET_GET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceBasicInfoCb(NET_CB_GetDeviceBasicInfo pCb);

/**
 * @brief 设置设备基本信息回调类型（仅设备名strDeviceName可写，其余字段只读）
 * @note  身份字段(序列号/固件/MAC/型号/厂商)只读，宿主回调应仅应用strDeviceName
 * @param [IN] pInfo 设备基本信息结构体指针，含待设置字段
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_SetDeviceBasicInfo)(pNET_DeviceBasicInfo_S pInfo);

/**
 * @brief 注册设置设备基本信息回调 (NET_SET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterSetDeviceBasicInfoCb(NET_CB_SetDeviceBasicInfo pCb);

/**
 * @brief 设备存储信息回调类型（NVR/录播等有硬盘的设备专用）
 * @param [OUT] pInfo 设备存储信息结构体指针，由回调函数填充
 * @return NET_E_SUCCEED 成功，NET_E_NOT_SUPPORT 表示设备无存储，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetDeviceStorageInfo)(pNET_DeviceStorageInfo_S pInfo);

/**
 * @brief 注册获取设备存储信息回调 (NET_GET_STORAGE_INFO)
 * @details 只有具备存储能力的设备才需要注册此回调。
 *          编码器、矩阵等无存储设备不注册即可，SDK 返回 NET_E_NOT_SUPPORT。
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceStorageInfoCb(NET_CB_GetDeviceStorageInfo pCb);

/**
 * @brief 设备控制回调类型
 * @param [IN] pstCtrlInfo 设备硬件控制参数，参见 NET_DeviceControlInfo_S
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_DeviceControl)(pNET_DeviceControlInfo_S pstCtrlInfo);

/**
 * @brief 注册设备控制回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterDeviceControlCb(NET_CB_DeviceControl pCb);

/**
 * @brief 修改用户密码回调类型
 * @param [IN] pPasswordInfo 修改密码参数，包含用户名、旧密码、新密码
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_SetUserPassword)(pNET_UserPasswordInfo_S pPasswordInfo);

/**
 * @brief 注册修改用户密码回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterSetUserPasswordCb(NET_CB_SetUserPassword pCb);

/**
 * @brief 视频编码能力集回调类型 (NET_CAP_VIDEO_ENCODE)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetVideoEncodeCap)(INT32 dwChannelID,
                                                             pNET_VideoEncodeCap_S pCap);

/**
 * @brief 注册视频编码能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetVideoEncodeCapCb(NET_CB_GetVideoEncodeCap pCb);

/**
 * @brief 音频编码能力集回调类型 (NET_CAP_AUDIO)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         音频编码能力集结构体指针
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetAudioEncodeCap)(INT32 dwChannelID,
                                                             pNET_AudioCap_S pCap);

/**
 * @brief 注册音频编码能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetAudioEncodeCapCb(NET_CB_GetAudioEncodeCap pCb);

/**
 * @brief OSD能力集回调类型 (NET_CAP_OSD)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         OSD能力集结构体指针
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetOsdCap)(INT32 dwChannelID, pNET_OsdCap_S pCap);

/**
 * @brief 注册OSD能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetOsdCapCb(NET_CB_GetOsdCap pCb);

/**
 * @brief 通用配置回调类型（按命令码分发）
 * @param [IN] dwChannelID 通道号
 * @param [IN] dwCommand 命令码（标识配置类型）
 * @param [OUT] lpOutBuffer 输出缓冲区，存放配置数据
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpOutBuffer);

/**
 * @brief 通用配置设置回调类型（按命令码分发）
 * @param [IN] dwChannelID 通道号
 * @param [IN] dwCommand 命令码（标识配置类型）
 * @param [IN] lpInBuffer 输入缓冲区，包含要设置的配置数据
 * @return NET_E_SUCCEED 成功，其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_SetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpInBuffer);

/**
 * @brief 按命令码注册的配置回调类型（专用回调）
 * @note 回调参数由命令码对应结构体决定，比通用回调更具体
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpOutBuffer);
typedef NET_COMMON_ECODE_E (*NET_CB_SetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpInBuffer);

/**
 * @brief 获取RTSP流地址回调类型 (NET_GET_RTSPURLCFG)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pInfo        RTSP URL 信息结构体指针
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetRtspUrl)(INT32 dwChannelID, pNET_RtspUrlInfo_S pInfo);

/**
 * @brief 获取回放播放地址回调类型
 * @param [INOUT] pInfo 回放查询条件和播放URL返回信息
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetReplayUrl)(pNET_ReplayUrlInfo_S pInfo);

/**
 * @brief 回放控制回调类型
 * @param [INOUT] pInfo 回放控制输入输出参数
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_ControlReplay)(pNET_ReplayCtrlInfo_S pInfo);

/**
 * @brief 获取回放录像时间段回调类型
 * @param [INOUT] pInfo 查询条件及结果
 * @return NET_E_SUCCEED 成功, 其他值失败
 */
typedef NET_COMMON_ECODE_E (*NET_CB_GetReplayRecordList)(pNET_ReplayRecordList_S pInfo);

/**
 * @brief 注册通用配置获取回调（所有命令码统一处理）
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 * @note 当没有按命令码注册的专用回调时，会调用此通用回调
 */
NET_API BOOL STDCALL NET_serverRegisterGetDevConfigCb(NET_CB_GetDevConfig pCb);

/**
 * @brief 注册通用配置设置回调（所有命令码统一处理）
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 * @note 当没有按命令码注册的专用回调时，会调用此通用回调
 */
NET_API BOOL STDCALL NET_serverRegisterSetDevConfigCb(NET_CB_SetDevConfig pCb);
/************************************************************************/
/*                          录播的配置回调接口                     */
/************************************************************************/

NET_API BOOL STDCALL NET_serverRegisterGetRegisterInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRegisterInfoCb(NET_CB_SetDevConfigByCommand pCb);

/************************************************************************/
/*                          按命令码注册的配置回调接口                     */
/************************************************************************/
/**
 * @brief 注册设备基本信息获取回调 (NET_GET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterGetDeviceConfigCb(NET_CB_GetDevConfigByCommand pCb);

/**
 * @brief 注册设备基本信息设置回调 (NET_SET_DEVICECFG)
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_API BOOL STDCALL NET_serverRegisterSetDeviceConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetNtpConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetNtpConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetStreamConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetStreamConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRtspUrlCb(NET_CB_GetRtspUrl pCb);
NET_API BOOL STDCALL NET_serverRegisterGetReplayUrlCb(NET_CB_GetReplayUrl pCb);
NET_API BOOL STDCALL NET_serverRegisterControlReplayCb(NET_CB_ControlReplay pCb);
NET_API BOOL STDCALL NET_serverRegisterGetReplayRecordListCb(NET_CB_GetReplayRecordList pCb);
NET_API BOOL STDCALL NET_serverRegisterGetOsdCapConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetOsdCapConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetImageConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetImageConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetNetworkConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetNetworkConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetConfigWifiStaCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterConnectWifiStaCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterDisconnectWifiStaCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGet4GInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSet4GInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetHotspotInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetHotspotConnCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSecurityServicesInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSecurityServicesInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSshCountdownCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterFindLogCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterExportLogCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetLogServerCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetLogServerCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterTestLogServerCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterControlRecordInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRecordStatusCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRecordScheduleCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRecordScheduleCb(NET_CB_SetDevConfigByCommand pCb);
/**
 * @brief 注册获取 SD 卡物理状态的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_SdCardStatus_S 输出缓冲区的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetSdCardStatusCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取声音报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_AudibleAlarmInfo_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetAudibleAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置声音报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_AudibleAlarmInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetAudibleAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取报警输入配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_AlarmInputInfoList_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetAlarmInputInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置单个报警输入配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_AlarmInputInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetAlarmInputInfoCb(NET_CB_SetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取报警输出配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_AlarmOutputInfoList_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetAlarmOutputInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置单个报警输出配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_AlarmOutputInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetAlarmOutputInfoCb(NET_CB_SetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取闪光报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_FlashingLightAlarmInfo_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetFlashingLightAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置闪光报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_FlashingLightAlarmInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetFlashingLightAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb);
/*
 * 功能描述：注册获取人体红外（PIR）报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于填写 NET_PirAlarmInfo_S 输出数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetPirAlarmInfoCb(NET_CB_GetDevConfigByCommand pCb);
/*
 * 功能描述：注册设置人体红外（PIR）报警配置的回调函数。
 * 作者：ITC
 * @param [in] pCb 用于处理 NET_PirAlarmInfo_S 输入数据的回调函数。
 * @param [out] 无。注册成功后，回调函数将保存到内部配置回调表。
 * @return 注册成功返回 TRUE；回调函数为空或重复注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterSetPirAlarmInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRecordAdvancedParamCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRecordAdvancedParamCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterFindRecordFileInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterDownloadRecordFileCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPrivacyMaskConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPrivacyMaskConfigCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetTamperAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetTamperAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetMotionAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetMotionAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetCrossLineAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCrossLineAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetIntrusionAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetIntrusionAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetLoiteringAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetLoiteringAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetAudioAnomalyAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetAudioAnomalyAlarmCb(NET_CB_SetDevConfigByCommand pCb);
/**
 * @brief 注册获取音频异常侦测实时音量的回调函数。
 * @author ITC
 * @param [in] pCb 用于填充 NET_AudioAnomalyCurrentDb_S 输出缓冲区的回调函数。
 * @param [out] 无。SDK 将回调函数保存到配置回调表。
 * @return 注册成功返回 TRUE；回调函数非法或已注册时返回 FALSE。
 */
NET_API BOOL STDCALL NET_serverRegisterGetAudioAnomalyCurrentDbCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPreviewInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPreviewInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetChannelInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetCrowGatheringAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCrowGatheringAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetParkingAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetParkingAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetUnattendedObjectAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetUnattendedObjectAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetObjectRemovalAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetObjectRemovalAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSceneChangeAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSceneChangeAlarmCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetGarbageExposureConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetGarbageExposureConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetGarbageOverflowConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetGarbageOverflowConfigCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterSetTalkbackStateCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetTalkbackToStreamCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetTalkbackFromStreamCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetReplayTalkbackCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetUpgradeStatusCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetUpgradeCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetUpgradeVersionCb(NET_CB_GetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetCapturePlanInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCapturePlanInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetCaptureParamInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCaptureParamInfoCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetExposureInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetExposureInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetDayNightInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetDayNightInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetBackLightInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetBackLightInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetDenoiseInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetDenoiseInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetWhiteBalanceInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetWhiteBalanceInfoCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetAudioConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetAudioConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetEnterRegionAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetEnterRegionAlarmCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetLeaveRegionAlarmCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetLeaveRegionAlarmCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetFaceCaptureInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetFaceCaptureInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetFaceCaptureOverlayInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetFaceCaptureOverlayInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetFaceCompareInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterAddTargetLibCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterDelTargetLibCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetTargetLibCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetTargetLibCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterAddFaceInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterDelFaceInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetFaceInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetFaceInfoCb(NET_CB_GetDevConfigByCommand pCb);


NET_API BOOL STDCALL NET_serverRegisterGetPeopleFlowStatisticsConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPeopleFlowStatisticsConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterResetPeopleFlowStatisticsCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPeopleDensityDetectionConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPeopleDensityDetectionConfigCb(NET_CB_SetDevConfigByCommand pCb);

NET_API BOOL STDCALL NET_serverRegisterGetManholeCoverAbnormalConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetManholeCoverAbnormalConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSleepOnDutyConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSleepOnDutyConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetElectricVehicleInElevatorConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetElectricVehicleInElevatorConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPersonFallDownConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPersonFallDownConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetConstructionOccupyRoadConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetConstructionOccupyRoadConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetCongestionConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetCongestionConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetLicensePlateRecognitionConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetLicensePlateRecognitionConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetHighAltitudeSeatbeltConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetHighAltitudeSeatbeltConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSafetyHelmetConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSafetyHelmetConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPersonFallConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPersonFallConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPhoneUsageConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPhoneUsageConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSmokingConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSmokingConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetOpenFlameConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetOpenFlameConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetBareSoilConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetBareSoilConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetHoleProtectionBarConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetHoleProtectionBarConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetReflectiveClothingConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetReflectiveClothingConfigCb(NET_CB_SetDevConfigByCommand pCb);

/* 智能事件配置回调注册接口 */
NET_API BOOL STDCALL NET_serverRegisterGetPetRecognitionInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPetRecognitionInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetClimbFenceInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetClimbFenceInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetDimissionInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetDimissionInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetIllegalLaneInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetIllegalLaneInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRetrogradeInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRetrogradeInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetNonmotorVehicleIntrusionInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetNonmotorVehicleIntrusionInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetOccupationEmergencyInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetOccupationEmergencyInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetPedestrianIntrusionInfoCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetPedestrianIntrusionInfoCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetSmokeFireConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetSmokeFireConfigCb(NET_CB_SetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterGetRoadPondingConfigCb(NET_CB_GetDevConfigByCommand pCb);
NET_API BOOL STDCALL NET_serverRegisterSetRoadPondingConfigCb(NET_CB_SetDevConfigByCommand pCb);

/************************************************************************/
/*                    设备发现 Device Discovery                           */
/************************************************************************/
/**
 * @brief 获取设备发现信息的回调
 * @param [OUT] pDeviceInfo 由宿主应用填充设备信息
 */
typedef void(STDCALL *NET_CB_GetDiscoveryDeviceInfo)(
    OUT NET_DiscoveryDeviceInfo_S* pDeviceInfo);

typedef NET_COMMON_ECODE_E (STDCALL *NET_CB_SetNetwork)(
    IN const NET_PoeNetworkConfig_S* pConfig);

/**
 * @brief 注册设备发现信息回调（启动前必须调用）
 * @param [IN] cbFunc 回调函数指针
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverRegisterGetDiscoveryDeviceInfoCb(
    IN NET_CB_GetDiscoveryDeviceInfo cbFunc);

NET_API BOOL STDCALL
NET_serverRegisterSetNetworkCb(IN NET_CB_SetNetwork cbFunc);

/**
 * @brief 启动设备发现响应服务（阻塞线程中运行 AF_PACKET 接收循环）
 * @param [IN] szInterfaceName 网卡名称 (如 "eth0")
 * @return TRUE 成功，FALSE 失败
 * @note 需先调用 NET_serverRegisterGetDiscoveryDeviceInfoCb 注册回调
 */
NET_API BOOL STDCALL
NET_serverStartDiscovery(IN const CHAR* szInterfaceName);

/**
 * @brief 停止设备发现响应服务
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverStopDiscovery(void);

/************************************************************************/
/*                       语音对讲 VoiceCom (服务端)                       */
/************************************************************************/
/** @brief 语音对讲播放回调: 收到NVR端音频时调用, 推送到扬声器 */
typedef void (STDCALL *NET_serverVoiceComPlayCallBack)(const char* data, unsigned int size);
/**
 * @brief 语音对讲采集回调: SDK按协商参数主动拉取设备侧采集帧并发送到NVR
 * @param [IN]  pstAudioParam 当前 VoiceCom 会话协商的音频参数
 * @param [OUT] pBuffer       输出音频帧缓存
 * @param [IN]  dwBufferSize  输出缓存长度
 * @param [IN]  lpUserData    用户数据
 * @return 实际写入的音频字节数，返回 <=0 表示当前无可用音频帧
 * @note 回调内应写入与 pstAudioParam 匹配的裸音频帧；建议每次返回 dwFrameBytes 字节。
 */
typedef INT32 (STDCALL *NET_serverVoiceComCaptureCallBack)(
    IN const NET_VoiceComAudioParam_S* pstAudioParam,
    OUT CHAR* pBuffer,
    IN UINT32 dwBufferSize,
    IN LPVOID lpUserData);

/**
 * @brief 启动语音对讲TCP监听
 * @param [IN]  dwPort  监听端口, 默认9006
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverStartVoiceComServer(IN UINT32 dwPort);

/**
 * @brief 停止语音对讲TCP监听
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverStopVoiceComServer(void);

/**
 * @brief 注册播放回调 (收到NVR音频 → 扬声器)
 * @param [IN]  cb  播放回调
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverRegisterVoiceComPlayCb(IN NET_serverVoiceComPlayCallBack cb);

/**
 * @brief 注册采集回调 (麦克风/LineIn -> NVR)
 * @param [IN]  cb          采集回调，传 NULL 表示注销
 * @param [IN]  lpUserData  用户数据，回调时原样透传
 * @return TRUE 成功，FALSE 失败
 * @note SDK负责按当前 VoiceCom 会话参数定时拉帧并发送，业务侧只需要提供采集帧。
 */
NET_API BOOL STDCALL
NET_serverRegisterVoiceComCaptureCb(IN NET_serverVoiceComCaptureCallBack cb,
                                         IN LPVOID lpUserData);

/**
 * @brief 发送麦克风采集的音频到NVR
 * @param [IN]  pData  音频帧数据，格式需与当前 VoiceCom 会话协商参数一致
 * @param [IN]  dwSize 数据长度(字节)
 * @return TRUE 成功，FALSE 失败
 */
NET_API BOOL STDCALL
NET_serverSendVoiceComData(IN const CHAR* pData, IN UINT32 dwSize);

/**
 * @brief 获取当前 VoiceCom 会话协商的音频参数
 * @param [OUT] pstAudioParam  音频参数
 * @return TRUE 成功，FALSE 表示尚未建立会话或参数未协商
 */
NET_API BOOL STDCALL
NET_serverGetVoiceComAudioParam(OUT pNET_VoiceComAudioParam_S pstAudioParam);

/************************************************************************/
/*                       录像帧流 RecordFrame (服务端)                    */
/************************************************************************/
/**
 * @brief 录像帧流开始回调: 收到客户端起止时间查询后调用, 由宿主打开录像源并填充流信息
 */
typedef NET_COMMON_ECODE_E (STDCALL *NET_serverRecordFrameStartCallBack)(
    IN pNET_RecordFrameStreamCond_S pstCond,
    INOUT pNET_RecordFrameStreamInfo_S pstInfo,
    IN LPVOID lpUserData);

/**
 * @brief 录像帧读取回调: SDK在TCP连接建立后循环拉取帧并发送给客户端
 * @return 实际写入 pBuffer 的负载字节数；0 表示暂时无帧；<0 表示结束/失败
 */
typedef INT32 (STDCALL *NET_serverRecordFrameReadCallBack)(
    IN const CHAR* szStreamId,
    OUT pNET_RecordFrameInfo_S pstFrameInfo,
    OUT CHAR* pBuffer,
    IN UINT32 dwBufferSize,
    IN LPVOID lpUserData);

/** @brief 录像帧流停止回调: 客户端停止或流结束时调用 */
typedef NET_COMMON_ECODE_E (STDCALL *NET_serverRecordFrameStopCallBack)(
    IN const CHAR* szStreamId,
    IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_serverStartRecordFrameServer(IN UINT32 dwPort);

NET_API BOOL STDCALL
NET_serverStopRecordFrameServer(void);

NET_API BOOL STDCALL
NET_serverRegisterRecordFrameStartCb(IN NET_serverRecordFrameStartCallBack cb,
                                         IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_serverRegisterRecordFrameReadCb(IN NET_serverRecordFrameReadCallBack cb,
                                        IN LPVOID lpUserData);

NET_API BOOL STDCALL
NET_serverRegisterRecordFrameStopCb(IN NET_serverRecordFrameStopCallBack cb,
                                        IN LPVOID lpUserData);



#ifdef __cplusplus
}
#endif

#endif /* NETTVSDK_H */
