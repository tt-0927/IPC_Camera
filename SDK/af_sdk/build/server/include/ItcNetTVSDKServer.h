#ifndef NETTVSDK_H
#define NETTVSDK_H

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

#ifndef NEWINTERFACE
    #define NEWINTERFACE
#endif

#if defined(_WIN32)  /* windows */
#define NET_TV_API
#else
#define NET_TV_API
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
            #define STDCALL                 __attribute__((stdcall))__attribute__((visibility ("default")))
            #endif
        #else
            #ifndef STDCALL
            #define STDCALL
            #endif
        #endif
    #endif
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
#ifndef NET_TV_SDK_NO_MEDIA
#define NET_TV_WITH_MEDIA           1           /* 包含媒体模块 */
#endif

#ifndef NET_TV_SDK_NO_CLOUD
#define NET_TV_WITH_CLOUD           1           /* 包含云服务模块 */
#endif
#define NET_TV_WITH_XW              1           /* 包含电视墙、拼接模块 */
#define NET_TV_WITH_VMS             1           /* 包含VMS模块 */
#define NET_TV_WITH_PTZ             1           /* 包含云台模块 */
#define NET_TV_WITH_SMART           1           /* 包含智能业务模块 */
#define NET_TV_WITH_CONFIG          1           /* 包含配置业务模块 */
#define NET_TV_WITH_BASIC           1           /* 包含基础管理业务模块 */
#define NET_TV_ALARM_RECOVER_BASE   1           /* 告警恢复基数 Alarm recover base */
#define NET_TV_WITH_TRANS_CHANEL    1           /* 包含透明通道模块 */

/********************************** 常用数值宏  Commonly used numerical macros *************** */
#define NET_TV_STREAM_ID_LEN                    32          /* 流ID标识长度  Length of stream ID*/
#define NET_TV_FILE_NAME_LEN                    (256u)      /* 文件名长度  Length of filename */
#define NET_TV_USERNAME_LEN                     (128 + 4)   /* Maximum length of username */
#define NET_TV_PASSWORD_LEN                     128         /* Maximum length of password */
#define NET_TV_DESCRIBE_MAX_LEN                 (512 + 4)   /* 描述最大长度 ：128 * 4，任意字符达到长度128 */
#define NET_TV_DOMAIN_LEN                       64          /* 域名最大长度  Maximum length of domain name */
#define NET_TV_PATH_LEN                         128         /* 路径最大长度:包括文件名称  Maximum length of path, including filename */
#define NET_TV_MAX_URL_LEN                      512         /* URL 的最大长度  Maximum length of URL */
#define NET_TV_INVALID_CHANNEL_ID               (0XFFFFFF)  /* 无效的通道ID */
#define NET_TV_INVALID_ID                       0            /* 无效ID */

/* 通用长度  Common length */
#define NET_TV_LEN_2                            2
#define NET_TV_LEN_4                            4
#define NET_TV_LEN_6                            6
#define NET_TV_LEN_8                            8
#define NET_TV_LEN_16                           16
#define NET_TV_LEN_18                           18
#define NET_TV_LEN_32                           32
#define NET_TV_LEN_40                           40
#define NET_TV_LEN_64                           64
#define NET_TV_LEN_128                          128
#define NET_TV_LEN_132                          132
#define NET_TV_LEN_256                          256
#define NET_TV_LEN_260                          260
#define NET_TV_LEN_480                          480
#define NET_TV_LEN_512                          512
#define NET_TV_LEN_1024                         1024
#define NET_TV_LEN_2000                         2000
#define NET_TV_LEN_4096                         4096
#define NET_TV_LEN_8192                         8192

#define NET_TV_IPADDR_STR_MAX_LEN              (64u)        /* IP 地址信息字符串长度  Length of IP address string */
#define NET_TV_IPV4_LEN_MAX                     16          /* IPV4地址字符串长度 Length of IPV4 address string */
#define NET_TV_IPV6_LEN_MAX                     128         /* IPV6地址字符串长度 Length of IPV6 address string */
#define NET_TV_NAME_MAX_LEN                    (256u)       /* 通用名称字符串长度  Length of common name string */

#define NET_TV_CODE_STR_MAX_LEN                (256u)       /* 通用CODE 长度  Length of common code */
#define NET_TV_MAX_DATE_STRING_LEN             (64u)        /* 最大日期字符长度 Maximum length of date string "2008-10-02 09:25:33.001 GMT" */
#define NET_TV_MAX_ALARM_IN_NUM                 64          /* 告警输入最大数量  Maximum number of alarm inputs */
#define NET_TV_MAX_ALARM_OUT_NUM                64          /* 告警输出最大数量  Maximum number of alarm outputs */
#define NET_TV_PLAN_SECTION_NUM                 8           /* 一天中的计划时间段  Number of scheduled time sections in a day */
#define NET_TV_PLAN_NUM_AWEEK                   8           /* 一周总共可配置的计划个数,包括周一至周日和假日  Total number of plans allowed in a week, including Monday to Sunday, and holidays */

#define NET_TV_MAX_PRESET_NUM                   256         /* 预置位最大数  Maximum number of presets */
#define NET_TV_MAX_CRUISEPOINT_NUM              32          /* 巡航路径中预置位点最大个数  Maximum number of presets for preset patrol */
#define NET_TV_MAX_CRUISEROUTE_NUM              16          /* 预置位巡航路径最大条数  Maximum number of routes for preset patrol */
#define NET_TV_MIN_PTZ_SPEED_LEVEL              1           /* 云台移动最小速度  Maximum PTZ rotating speed */
#define NET_TV_MAX_PTZ_SPEED_LEVEL              9           /* 云台移动最大速度  MinimumPTZ rotating speed */
#define NET_TV_MAX_VIDEO_EFFECT_VALUE           255         /* 图像参数（亮度 对比度 色度 饱和度）最大值  Maximum values for image parameters (brightness, contrast, hue, saturation) */
#define NET_TV_MIN_VIDEO_EFFECT_VALUE           0           /* 图像参数（亮度 对比度 色度 饱和度）最小值  Minimum values for image parameters (brightness, contrast, hue, saturation) */
#define NET_TV_MAX_VIDEO_EFFECT_GAMMA_VALUE     10          /* 图像参数（伽马值）最大值 Minimum values for image parameters (Gama) */

#define NET_TV_MAX_PRIVACY_MASK_AREA_NUM        8           /* 最大可配置遮盖区域个数  Maximum number of privacy mask areas allowed */
#define NET_TV_OSD_TEXTOVERLAY_NUM              6           /* 通道 OSD 字符叠加数量  Number of OSD text overlays */
#define NET_TV_OSD_TEXT_MAX_LEN                 (64 + 4)    /* 通道 OSD 字符长度  Length of OSD texts */
#define NET_TV_OSD_TEXT_MAX_LEN_EX              (512 + 4)   /* 通道 OSD 字符长度(扩展)  Length of OSD texts */
#define NET_TV_OSD_TYPE_MAX_NUM                 32          /* 通道 OSD 最大类型个数  Maximum number of OSD type */
#define NET_TV_OSD_CUSTOM_MAX_NUM               4           /* 通道 OSD 自定义字符叠加最大个数  Maximum number of custom OSD texts */
#define NET_TV_OSD_FONT_SIZE_TYPE_MAX_NUM       4           /* 通道 OSD 字体最大类型个数  Maximum number of OSD font size type */
#define NET_TV_OSD_FONT_STYLE_TYPE_MAX_NUM      4           /* 通道 OSD 样式最大类型个数  Maximum number of OSD font style type */
#define NET_TV_OSD_TIME_FORMAT_MAX_NUM          7           /* 通道 OSD 最大时间格式个数  Maximum number of OSD time format type */
#define NET_TV_OSD_DATE_FORMAT_MAX_NUM          15          /* 通道 OSD 最大日期格式个数  Maximum number of OSD date format type */
#define NET_TV_PULL_ALARM_MAX_NUM               8           /* 拉告警最大告警个数  Maximum number of alarms a user can get */
#define NET_TV_TRACK_CRUISE_MAXNUM              1           /* 支持的轨迹巡航的最大条数  Maximum number of patrol routes allowed  */
#define NET_TV_AUDIO_INPUT_TYPE_MAX             2           /* 最大输入类型数量 */
#define NET_TV_AUDIO_OUTPUT_TYPE_MAX            3           /* 最大输出类型数量 */
#define NET_TV_AUDIO_FORMAT_MAX                 8           /* 最大音频格式数量 */
#define NET_TV_AUDIO_SAMPRATE_MAX               11          /* 最大采样率数量 */
#define NET_TV_AUDIO_BITRATE_MAX                7           /* 最大码率数量 */
#define NET_TV_AUDIO_SOUND_MIN_VALUE            0           /* 音量调节最小值  Minimum volume */
#define NET_TV_AUDIO_SOUND_MAX_VALUE            255         /* 音量调节最大值  Maximum volume */
#define NET_TV_MIC_SOUND_MIN_VALUE              0           /* 麦克风音量调节最小值  Minimum volume */
#define NET_TV_MIC_SOUND_MAX_VALUE              255         /* 麦克风音量调节最大值  Maximum volume */
#define NET_TV_SCREEN_INFO_ROW                  18          /* 屏幕信息行数  Screen Info Row */
#define NET_TV_SCREEN_INFO_COLUMN               22          /* 屏幕信息列数  Screen Info Column */
#define NET_TV_CHANNEL_MAX                      512         /* 最大通道数 Maximum number of channel */
#define NET_TV_RESOLUTION_NUM_MAX               32          /* 分辨率总个数 Maximum number of resolution */
#define NET_TV_MONTH_DAY_MAX                    31          /* 每月天数最大值 Maximum number of days in a month */
#define NET_TV_VIDEO_ENCODE_TYPE_MAX            16          /* 编码格式类型总个数 Maximum number of encode type */
#define NET_TV_PEOPLE_CNT_MAX_NUM               60          /* 客流量统计数组最大值（分报表） Maximum number of people count */
#define NET_TV_WIFISNIFFER_MAC_MAX_NUM          64          /* wifi sniffer MAC地址最大长度  Length of wifi sniffer MAC */
#define NET_TV_WIFISNIFFER_MAC_ARRY_MAX_NUM     128         /* wifi sniffer MAC地址数组最大值 Maximum number of wifi sniffer MAC array */
#define NET_TV_HOTSPOT_CONN_MAX_NUM             128         /* 热点连接设备最大数量 Maximum number of hotspot connected devices */
#define NET_TV_DISK_MAX_NUM                     256         /* 磁盘最大数量 Maximum number of Disk */
#define NET_TV_LOCAL_DISK_MAX_NUM               32          /* 本地磁盘最大数量 local Maximum number of Disk */
#define NET_TV_SD_CARD_DISK_MAX_NUM             16          /* SD卡最大数量 SD Maximum number of Disk */
#define NET_TV_ARRAY_MAX_NUM                    16          /* 阵列最大数量 array Maximum number of Disk */
#define NET_TV_EXTEND_CABINET_DISK_MAX_NUM      32          /* 扩展柜硬盘最大数量 extend cabinet Maximum number of Disk */
#define NET_TV_NAS_MAX_NUM                      16          /* NAS最大数量 NAS Maximum number of Disk */
#define NET_TV_ESATA_MAX_NUM                    4           /* ESATA最大数量 eSATA Maximum number of Disk */
#define NET_TV_DISK_SMART_MAX_NUM               128         /* 硬盘SMART信息最大数量 Maximum number of Disk Smart Info */
#define NET_TV_ENCODE_FORMAT_MAX_NUM            3           /* 最大视频编码格式数 Maximum number of video compression */
#define NET_TV_SMART_ENCODE_MODEL_MAX_NUM       3           /* 最大智能图像扩展编码模式数 Maximum number of smart image encoding mode */
#define NET_TV_GOP_TYPE_MAX_NUM                 4           /* 最大GOP类型数量 Maximum number of GOP type */
#define NET_TV_IPSAN_MAX_NUM                    4           /* IPSAN最大数量 IPSAN Maximum number of Disk */

#define NET_TV_PHOTO_SERVER_MAX_NUM             4           /* 照片服务器数量上限 Maximum number of Photo Server */

#define NET_TV_INTELLIGENT_SERVER_MAX_NUM       4           /* 智能服务器数量上限 Maximum number of Intelligent Server */

#define NET_TV_MANAGER_SERVER_MAX_NUM           4           /* 管理服务器数量上限 Maximum number of Manager Server */

#define NET_TV_DEV_OTHER_LEN_MAX                32          /* 其他字段 */
#define NET_TV_DEV_NAME_LEN_MAX                 64          /* 设备名称长度 */

#define NET_TV_DEV_PASSWORD_LEN_MAX             64          /* 设备密码长度 */
#define NET_TV_CLOUD_DEV_USER_NAME_LEN          260         /* 云端设备ID长度 */
#define NET_TV_CLOUD_USER_NAME_LEN              260         /* 云端用户名长度 */
#define NET_TV_CLOUD_DEV_USER_AUTH_LEN          260         /* 云端设备用户权限名称长度 */
#define NET_TV_CLOUD_SHARE_TARGET_NAME_LEN      64          /* 云端设备共享对象名称长度 */
#define NET_TV_CLOUD_SHARE_DESCRIBE_LEN         260         /* 云端设备共享描述长度 */
#define NET_TV_CLOUD_DEV_NAME_LEN               260         /* 云端设备名称长度 */
#define NET_TV_XW_MAX_PANE_NUM                  64          /* 窗口最大分屏数量 */
#define NET_TV_NTP_SERVER_LIST_NUM              5           /* NTP服务列表数量 */

#define NET_TV_TMS_FACE_RECORD_ID_LEN           32          /* 记录ID缓存长度 */
#define NET_TV_TMS_CAMER_ID_LEN                 32          /* 相机ID缓存长度 */
#define NET_TV_TMS_PASSTIME_LEN                 32          /* 通过时间字符串缓存长度 */
#define NET_TV_TMS_FACE_TOLLGATE_ID_LEN         32          /* 卡口编号缓存长度 */
#define NET_TV_TMS_HEAT_MAP_DEVID_LEN           32          /* 热度图DevID字段长度 */
#define NET_TV_TMS_HEAT_MAP_RECORD_ID_LEN       16          /* 热度图RecordID字段长度 */
#define NET_TV_TMS_HEAT_MAP_COllECT_TIME_LEN    18          /* 热度图CollectTime 字段长度 */
#define NET_TV_TMS_PIC_COMMON_NUM               10          /* 图片或区域上限个数 */
#define NET_TV_TMS_CAR_PLATE_CAMID_LEN          32          /* 车牌识别CamID字段长度 */
#define NET_TV_TMS_CAR_PLATE_RECORDID_LEN       32          /* 车牌识别RecordID字段长度 */
#define NET_TV_TMS_CAR_PLATE_TOLLGATE_LEN       32          /* 车牌识别TollgateID字段长度 */
#define NET_TV_TMS_CAR_PLATE_PASSTIME_LEN       18          /* 车牌识别PassTime字段长度 */
#define NET_TV_TMS_CAR_PLATE_LANEID_LEN         18          /* 车牌识别LaneID字段长度 */
#define NET_TV_TMS_CAR_PLATE_CARPLATE_LEN       32          /* 车牌识别CarPlate字段长度 */
#define NET_TV_USER_NAME_ENCRYPT_LEN            256         /* 加密后的用户名长度 */
#define NET_TV_PASSWORD_ENCRYPT_LEN             256         /* 加密后的密码长度 */
#define NET_TV_VIDEO_FORMAT_MAX                 32          /* 支持的视频输出制式最大数量 */
#define NET_TV_VIDEO_FORMAT_NAME_LEN            32          /* 支持的视频输出制式名称长度 */
#define NET_TV_TVWALL_NAME_LEN                  260         /* 电视墙名称长度 */
#define NET_TV_FORMAT_SPEC_MAX                  256         /* 特殊输出制式的最大个数量 */
#define NET_TV_LED_SPEC_MAX                     256         /* 特殊模组框的最大个数量 */
#define NET_TV_FORMAT_NAME_LEN                  32          /* 输出制式名称长度 */
#define NET_TV_VIDEO_OUT_MAX                    64          /* 物理输出端口的最大个数量 */
#define NET_TV_SCENE_NAME_LEN                   260         /* 场景名称长度 */
#define NET_TV_TIME_LEN                         16          /* 时间字符串长度 */
#define NET_TV_WND_NAME_LEN                     260         /* 窗口名称长度 */
#define NET_TV_SEQUENCE_SRC_MAX                 128         /* 轮巡时视频源最大个数 */
#define NET_TV_TEXT_LEN                         1024        /* 虚拟LED文字内容长度 */
#define NET_TV_BMAP_NAME_LEN                    256         /* 底图名称长度 */
#define NET_TV_SEQ_RES_WIN_MAX                  288         /* 轮巡资源中窗口的最大个数 */
#define NET_TV_MAX_DAY_NUM                      8           /* 最大天数 */
#define NET_TV_MAX_TIME_SECTION_NUM             8           /* 时间段数量 */

#define NET_TV_ALARM_SOURCE_MAX_LEN                 (64 + 4)    /* 告警资源字符描述长度 */
#define NET_TV_MAX_EVENT_RES_SIZE                   1024        /* 事件上报最大资源数 */

#define NET_TV_VIID_CODE_LEN                        48          /* 视图编码长度 */
#define NET_TV_VIDEO_FORMAT_CAP_NUM                 64          /* 编码制式能力集 */
#define NET_TV_VIDEO_FRAME_RATE_MAX_NUM             64          /* 视频能力集支持帧率最大数量 */
#define NET_TV_VIDEO_ENCODE_COMPLEXITY_MAX_NUM      3           /* 编码复杂度最大数量 */
#define NET_TV_LAYOUT_CAP_NUM                       64          /* 分屏能力集 */

#define NET_TV_DA_POINT_CODE_LEN                    48          /* 代理设备 点位 编码长度 */
#define NET_TV_DA_AREA_CODE_LEN                     48          /* 代理设备 区域 编码长度 */
#define NET_TV_VIRTUAL_MEM_TABLE_MAX                32          /* 虚拟内存表元素最大数量 */
#define NET_TV_EVENT_STORE_TYPE_NUM                 128         /* 时间存储类型数量 */
#define NET_TV_MAX_PANE_NUM                         36          /* DC业务分屏数量最大为36分屏 */
#define NET_TV_OSD_MAX_NUM_EX                       8           /* 通道 OSD 最大数量  Maximum Number of OSD */
#define NET_TV_RSA_MAX_VALUE                        3           /* 表示最多尝试密钥生成次数 */

#define NET_TV_MAX_VIDEO_BRIGHT_EFFECT_VALUE        199             /* 图像参数（亮度）最大值 */
#define NET_TV_MAX_VIDEO_CONTRAST_EFFECT_VALUE      199             /* 图像参数（对比度）最大值 */
#define NET_TV_MAX_VIDEO_SATURATION_EFFECT_VALUE    359             /* 图像参数（饱和度）最大值 */
#define NET_TV_MAX_VIDEO_HUE_EFFECT_VALUE           359             /* 图像参数（色度）最大值 */
#define NET_TV_MAX_VIDEO_GAMMA_EFFECT_VALUE         99              /* 图像参数（伽玛）最大值 */
#define NET_TV_PIXEL_CONVERT_RATIO                  5000            /* 像素转换比例 用于区域设置,如拉框放大区域 */
#define NET_TV_PANES_NUM                            16              /* 单通道最大分屏数 */

#define NET_TV_XW_AUDIO_NUM                         16              /* 音频输出通道最大个数 */
#define NET_TV_IP_ADDRESS_LEN                       64              /* IP地址长度 */

#define NET_TV_TVWALLPLAN_NUM                       4               /* 电视墙预案个数 */
#define NET_TV_PLAN_MAX_TVWALL_NUM                  4               /* 预案下电视墙最大数量 */
#define NET_TV_TVWALL_MAX_WIN_NUM                   81              /* 电视墙最大的窗口数量 */
#define NET_TV_TVWALL_MAX_LAYOUT_NUM                64              /* 电视墙最大的分屏数量,即为子窗口数量 */
#define NET_TV_ALARM_LINK_PRESET_NUM                16              /* 告警预案联动预置位数量  */
#define NET_TV_ALARM_LINK_SWITCHOUT_NUM             16              /* 告警预案联动告警输出通道数量 */
#define NET_TV_ALARM_LINK_MONITOR_NUM               16              /* 告警预案联动实况数量 */
#define NET_TV_ALARM_LINK_TVWALL_NUM                32              /* 告警预案联动电视墙最大数量*/
#define NET_TV_ALARM_LINK_SOUND_LEN                 512             /* 告警预案联动声音信息最大长度 */
#define NET_TV_ALARM_SOURCE_NUM                     1               /* 告警源数量 */
#define NET_TV_ALARM_LINK_NUM                       128             /* 告警预案数量 */

#define NET_TV_TIME_TEMPLATE_NUM                    32              /* 时间模板数量 */
#define NET_TV_DC_SCHEME_RES_CHN_MAX_NUM            256             /* DC轮巡最大资源数 */
#define NET_TV_VIEW_MAX_WIN_NUM                     100             /* 视图最大的窗口数量 */
#define NET_TV_MAX_ROLE_RIGHT_SIZE                  256             /* 用户权限菜单项数量 */
#define NET_TV_MAX_QUERY_CHANNEL_NUM                500             /* 单次查询最大通道个数 */
#define NET_TV_MAX_QUERY_DEV_NUM                    500             /* 单次查询最大设备个数 */
#define NET_TV_GRID_AREAS_LEN                       256             /* 宏块值数组长度 */
#define NET_TV_MAX_ORG_ROOT_ID_NUM                  32              /* 组织树根节点最大个数 */
#define NET_TV_VOICE_BROADCAST_CHANNEL_NUM_MAX      128             /* 一个语音广播组支持最大通道个数 */
#define NET_TV_RECORD_LOCK_ID_LEN                   64              /* 录像锁定ID最大长度 */
#define NET_TV_RECORD_LOCK_DESC_LEN                 64              /* 锁定录像段的描述最大长度 */
#define NET_TV_REPLAY_SESSION_ID_LEN                64              /* 回放会话ID最大长度 */
#define NET_TV_REPLAY_RECORD_SEGMENT_MAX            128             /* 单次回放录像查询最多返回的时间段数量 */
#define NET_TV_NOTIME                               0               /* 时间无值 */
#define NET_TV_WHITE_BALANCE_MODE_MAX_NUM           16              /* 最多支持的白平衡模式个数 Maximum number of Image white balance mode count */
#define NET_TV_FOCUS_MODE_MAX_NUM                   16              /* 最多支持的对焦模式个数 Maximum number of Image focus mode count */
#define NET_TV_FOCUS_SCENE_MAX_NUM                  16              /* 最多支持的对焦场景个数 Maximum number of Image focus scene count */
#define NET_TV_IMAGE_ROTATION_MODE_MAX_NUM          16              /* 最多支持的图像镜像模式个数 Maximum number of Image rotation mode count */
#define NET_TV_LAMP_CTRL_TYPE_MAX_NUM               16              /* 最多支持的支持的补光灯类型个数 Maximum number of lamp ctrl type count */
#define NET_TV_LAMP_CTRL_MODE_MAX_NUM               16              /* 最多支持的补光灯空控制模式个数 Maximum number of lamp ctrl mode count */
#define NET_TV_EXPOSURE_MODE_MAX_NUM                16              /* 最多支持的曝光模式个数 Maximum number of exposure mode count */
#define NET_TV_IRIS_RANGE_MAX_NUM                   16              /* 最多支持的光圈取值个数  Maximum number of Iris Range count */
#define NET_TV_METERING_MODE_MAX_NUM                16              /* 最多支持的测光控制模式个数  Maximum number of Metering mode count */
#define NET_TV_SHUTTER_TIME_RANGE_MAX_NUM           28              /* 最多支持的快门时间的取值的个数  Maximum number of shutter Time Range count */
#define NET_TV_SLOW_SHUTTER_TIME_RANGE_MAX_NUM      16              /* 最多支持的慢快门时间的取值的个数 Maximum number of slow shutter Time Range count */
#define NET_TV_WIDE_DYNAMIC_MODE_MAX_NUM            16              /* 最多支持的宽动态模式个数 Maximum number of wide dynamic mode count */
#define NET_TV_DAY_NIGHT_MODE_MAX_NUM               16              /* 最多支持的昼夜模式类型个数 Maximum number of slow Day Night Mode count */
#define NET_TV_AUDIO_IN_MAX_NUM                     16              /* 最多支持的音频口输入个数 Maximum number of Audio input count */
#define NET_TV_AUDIO_IN_CHL_MODE_MAX_NUM            8               /* 最多支持的音频输入通道模式个数 Maximum number of Audio input mode count */
#define NET_TV_AUDIO_IN_ENCODE_FORMAT_MAX_NUM       16              /* 最多支持的音频输入编码格式个数 Maximum number of Audio input encode format count */
#define NET_TV_AUDIO_SAMPLING_RATE_MAX_NUM          8               /* 最多支持的音频采样率个数 Maximum number of Audio sampling rate count */
#define NET_TV_SERIAL_IN_MAX_NUM                    16              /* 最多支持的串口输入个数 Maximum number of serial input count */
#define NET_TV_SERIAL_IN_ENCODE_FORMAT_MAX_NUM      16              /* 最多支持的串口输入编码格式个数 Maximum number of serial input encode format count */
#define NET_TV_FACE_FEATURE_SIZE                    512             /* 人脸特征信息 512B */
#define NET_TV_FACE_FEATURE_VERSION_LEN             40              /* 人脸特征模型版本号最大长度 */
#define NET_TV_FACE_FEATURE_LIST_FILE_LEN           256             /* 人脸特征库文件名最大长度 */
#define NET_TV_FACE_FEATURE_FILE_MD5_LEN            16              /* 人脸特征库文件的MD5值长度 */
#define NET_TV_FACE_FEATURE_GALLEY_ID_LEN           20              /* 人脸半结构化特征名单库ID长度 */
#define NET_TV_FACE_FEATURE_MAX_NUM                 3               /* 人脸半结构化特征最大数目 */
#define NET_TV_OBJ_TRACK_MODE_NUM                   8               /* 设备支持的智能跟踪模式数量 */
#define NET_TV_STREAM_MAX_NUM                       3               /* 最大支持的码流数量 */
#define NET_TV_PLAN_DAY_NUM_AWEEK                   7               /* 一周总共可配置的计划天数，包含周一到周日 */
#define NET_TV_PLAN_TIME_SECTION_NUM_ADAY           4               /* 一天可配置的时间段数 Total number of plans allowed in a day*/
#define NET_TV_XW_SERIAL_NUM                        16              /* 串口数量 */
#define NET_TV_DNS_LIST_NUM                         2               /* DNS列表数量 */
#define NET_TV_NETWORK_MACNAME_LEN                  48              /* MAC地址名称长度 */
#define NET_TV_LOG_QUERY_COND_NUM                   48              /* 日志查询条件数量 */
#define NET_TV_RECORD_FILE_MAX_NUM                  48              /* 录像查询结果最大数量 */
#define NET_TV_RECORD_DATE_MAX_NUM                  64              /* 录像日期结果最大数量 */
#define NET_TV_RECORD_DOWNLOAD_MAX_NUM              16              /* 录像下载任务最大数量 */
#define NET_TV_FACE_DB_NAME_LEN                     256             /* 人脸库名称长度最大值 */
#define NET_TV_FACE_MEMBER_NAME_LEN                 256             /* 人脸库成员名称长度最大值 */
#define NET_TV_FACE_MEMBER_REGION_LEN               256             /* 人脸库成员所在地区名称最大值 */
#define NET_TV_FACE_MEMBER_CUSTOM_NUM               5               /* 自定义属性列表个数 */
#define NET_TV_FACE_MEMBER_CUSTOM_LEN               255             /* 自定义属性值长度 */
#define NET_TV_FACE_IMAGE_MAX_LEN                   (2*1024*1024)   /* 人脸图片数据的最大长度，2M   2097152字节*/
#define NET_TV_FACE_DB_TITLE_NAME_LEN               508             /* 人脸库自定义属性名称最大长度 */
#define NET_TV_FACE_MONITOR_RULE_NAME_LEN           508             /* 人脸布控任务的布控名称最大值 */
#define NET_TV_FACE_MONITOR_RULE_REASON_LEN         508             /* 人脸布控的布控原因最大值 */
#define NET_TV_FACE_ALARM_SRC_LEN                   256             /* 抓拍通道名称长度 */
#define NET_TV_FACE_ANALYSIS_SKILL_NUM              16              /* 设备支持的人脸分析能力数量 */
#define NET_TV_FACE_MEMBER_BIRTHDAY_LEN             31              /* 成员出生日期字符串最大长度 */
#define NET_TV_FACE_IDNUMBER_LEN                    128             /* 证件号最大范围*/
#define NET_TV_FACE_LIB_MAX_NUM                     64              /* 目标库最大数量 */
#define NET_TV_FACE_INFO_MAX_NUM                    128             /* 人脸信息最大数量 */
#define NET_TV_FACE_ID_MAX_NUM                      128             /* 人脸ID最大数量 */
#define NET_TV_LABEL_ID_MAX_LEN                     32
#define NET_TV_TIME_RANGE_NUM                       8               /* 时间模板时间范围个数(周一到周日再加假日) */
#define NET_TV_TIME_DURATION_NUM                    8               /* 时间模板中一天最多8个片段 */
#define NET_TV_HOLIDAY_INFO_NUM                     32              /* 假日配置数量 */
#define NET_TV_AUDIO_MAX_NUM                        18              /* 音频输出业务数量 */
#define NET_TV_CREATE_CONNECT_NUMBER                1               /* 创建连接数量 */
#define NET_TV_EMERGENCY_BRLL_NAME_LEN              128             /* 紧急铃名称最大长度*/
#define NET_TV_EMERGENCY_BRLL_MAX_NUM               120             /* 紧急铃信息最大数量*/
#define NET_TV_VEHICLE_COMP_IMAGE_MAX_LEN           2097152         /* 车辆布控比对图片的最大长度 2M*/
#define NET_TV_VEHICLE_IMAGE_MAX_LEN                4194304         /* 车辆图片数据最大字节数 4M */
#define NET_TV_PIC_DATA_MAX_LEN                     (1024*1024)     /* 图片数据信息加密后最大大小 */

#define NET_TV_RES_CHANGE_INFO_LIST_NUM             64              /* 定义LAPI事件上报信息结构体 */

#define NET_TV_OUTPUT_NI_RECV_CARD_MAX_NUM          64              /* 输出网口下的接收卡最大数量 */
#define NET_TV_IMG_IN_MODE_LIST_MAX_NUM             16              /* 图像输入模式最大数量 */
#define NET_TV_GAMMA_INFO_LIST_MAX_NUM              1024            /* 伽马表最大数量值 */

#define NET_TV_MAX_SERIAL_PROT_NUM                  1               /* 设备当前只有一个485串口 */  
#define NET_TV_MAX_TRANS_CHANEL_NUM                 1               /* 一个485串口目前只支持一个透明通道 */

#define NET_TV_MAX_NIC_WORK_MODE_NUM                8               /* 最大网卡支持的工作模式数量 */  
#define NET_TV_MAX_PORT_WORK_MODE_NUM               24              /* 最大网口工作模式数量 */    
#define NET_TV_MAX_NET_WORK_CARD_NUM                8               /* 最大网卡数量 */
#define NET_TV_MAX_LINK_ACTION_NUM                  9               /* 最大联动动作数量 */

#define NET_TV_IVA_REPORT_COORD_NUM                 16              /* 一条规则的最大坐标点数 */

#define NET_TV_MAX_SCENE_INFO_NUM                   5               /* 最大场景信息数量 */
#define NET_TV_MAX_TRIGGER_DETAIL_INFO_NUM          4               /* 最大场景自动切换触发条件数量 */
#define NET_TV_MAX_ENV_PARAM_NUM                    2               /* 最大环境参数数量 */
#define NET_TV_MAX_SCENE_TYPE_NUM                   16              /* 最大支持的场景类型数量 */
#define NET_TV_MAX_ENV_TYPE_NUM                     2               /* 最大支持的环境类型数量 */

#define NET_TV_INVALID_PARAM                        (0xffffff)

/* 停车场车牌、车辆图片大小 单位：字节*/
#define NET_TV_VEH_PLATE_IMAGE_LEN              (1024*1024)

/* 停车场车牌、车辆加密后图片大小 单位：字节*/
#define NET_TV_VEH_PLATE_ENCODE_IMAGE_LEN       1400000

#define NET_TV_UINT32_INVALID                   0xFFFFFFFF               /* UINT32类型无效值定义 */

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
 * @struct NET_TV_ALARM_BASIC_INFO
 */
#define NET_TV_ALARM_BASE_BASIC         0x1000
#define NET_TV_ALARM_MOTION_DETECT      (NET_TV_ALARM_BASE_BASIC + 0x01) // 移动侦测
#define NET_TV_ALARM_OCCLUSION          (NET_TV_ALARM_BASE_BASIC + 0x02) // 视频遮挡
#define NET_TV_ALARM_ANOMALY            (NET_TV_ALARM_BASE_BASIC + 0x03) // 异常报警(通用)
#define NET_TV_ALARM_AUDIO              (NET_TV_ALARM_BASE_BASIC + 0x04) // 声音报警(分贝阈值)
#define NET_TV_ALARM_INPUT              (NET_TV_ALARM_BASE_BASIC + 0x05) // 报警输入(IO)
#define NET_TV_ALARM_OUTPUT             (NET_TV_ALARM_BASE_BASIC + 0x06) // 报警输出(IO)
#define NET_TV_ALARM_FLASH              (NET_TV_ALARM_BASE_BASIC + 0x07) // 闪光灯报警
#define NET_TV_ALARM_PIR                (NET_TV_ALARM_BASE_BASIC + 0x08) // PIR红外报警

/**
 * @brief 区域/周界规则报警 (0x2000 - 0x20FF)
 * @struct NET_TV_ALARM_RULE_INFO
 */
#define NET_TV_ALARM_BASE_RULE          0x2000
#define NET_TV_ALARM_LINE_CROSSING      (NET_TV_ALARM_BASE_RULE + 0x01)  // 越界侦测
#define NET_TV_ALARM_INTRUSION          (NET_TV_ALARM_BASE_RULE + 0x02)  // 区域入侵
#define NET_TV_ALARM_ENTER_REGION       (NET_TV_ALARM_BASE_RULE + 0x03)  // 进入区域
#define NET_TV_ALARM_LEAVE_REGION       (NET_TV_ALARM_BASE_RULE + 0x04)  // 离开区域
#define NET_TV_ALARM_OBJECT_REMOVAL     (NET_TV_ALARM_BASE_RULE + 0x05)  // 物品拿取
#define NET_TV_ALARM_UNATTENDED_OBJECT  (NET_TV_ALARM_BASE_RULE + 0x06)  // 物品遗留

/**
 * @brief Smart/AI 行为分析 (0x3000 - 0x3FFF)
 * @struct NET_TV_ALARM_AI_OBJECT_INFO
 */
// ------------------------------------------
#define NET_TV_ALARM_BASE_AI            0x3000

// > 人脸/人员相关
#define NET_TV_ALARM_FACE_DETECT        (NET_TV_ALARM_BASE_AI + 0x01)    // 人脸侦测
#define NET_TV_ALARM_FACE_CAPTURE       (NET_TV_ALARM_BASE_AI + 0x02)    // 人脸抓拍
#define NET_TV_ALARM_CROWD_GATHERING    (NET_TV_ALARM_BASE_AI + 0x03)    // 人员聚集
#define NET_TV_ALARM_LOITERING          (NET_TV_ALARM_BASE_AI + 0x04)    // 徘徊侦测
#define NET_TV_ALARM_PERSON_FALL        (NET_TV_ALARM_BASE_AI + 0x05)    // 人员倒地
#define NET_TV_ALARM_RUNNING            (NET_TV_ALARM_BASE_AI + 0x06)    // 快速奔跑
#define NET_TV_ALARM_FACE_COMPARE       (NET_TV_ALARM_BASE_AI + 0x07)    // 人脸比对

// > 行为监管/安防
#define NET_TV_ALARM_SLEEP_ON_DUTY      (NET_TV_ALARM_BASE_AI + 0x20)    // 睡岗
#define NET_TV_ALARM_LEAVE_POST         (NET_TV_ALARM_BASE_AI + 0x21)    // 离岗
#define NET_TV_ALARM_SMOKING            (NET_TV_ALARM_BASE_AI + 0x22)    // 抽烟
#define NET_TV_ALARM_PHONE_USAGE        (NET_TV_ALARM_BASE_AI + 0x23)    // 玩手机
#define NET_TV_ALARM_HELMET_MISSING     (NET_TV_ALARM_BASE_AI + 0x24)    // 未戴安全帽 (SAFETY_HELMET)
#define NET_TV_ALARM_NO_REFLECTIVE_VEST (NET_TV_ALARM_BASE_AI + 0x25)    // 未穿反光衣 (REFLECTIVE_CLOTHING)
#define NET_TV_ALARM_SMOKE_FIRE         (NET_TV_ALARM_BASE_AI + 0x26)    // 烟火检测

// > 音频智能
#define NET_TV_ALARM_AUDIO_ANOMALY      (NET_TV_ALARM_BASE_AI + 0x50)    // 音频异常

/**
 * @brief 交通/车辆相关 (0x4000 - 0x40FF)
 * @struct NET_TV_ALARM_PLATE_INFO
 */
#define NET_TV_ALARM_BASE_TRAFFIC       0x4000
#define NET_TV_ALARM_PLATE_RECOGNITION  (NET_TV_ALARM_BASE_TRAFFIC + 0x01) // 车牌识别
#define NET_TV_ALARM_ILLEGAL_PARKING    (NET_TV_ALARM_BASE_TRAFFIC + 0x02) // 违章停车
#define NET_TV_ALARM_TRAFFIC_CONGESTION (NET_TV_ALARM_BASE_TRAFFIC + 0x03) // 交通拥堵

/**
 * 设备异常/状态事件 (0x5000 - 0x50FF)
 * @struct NET_TV_ALARM_EXCEPTION_INFO
*/
#define NET_TV_ALARM_BASE_EXCEPTION     0x5000
#define NET_TV_ALARM_DISK_FULL          (NET_TV_ALARM_BASE_EXCEPTION + 0x01) // 硬盘满
#define NET_TV_ALARM_DISK_ERROR         (NET_TV_ALARM_BASE_EXCEPTION + 0x02) // 硬盘坏
#define NET_TV_ALARM_NET_BROKEN         (NET_TV_ALARM_BASE_EXCEPTION + 0x03) // 网络断开
#define NET_TV_ALARM_IP_CONFLICT        (NET_TV_ALARM_BASE_EXCEPTION + 0x04) // IP冲突
#define NET_TV_ALARM_VIDEO_LOSS         (NET_TV_ALARM_BASE_EXCEPTION + 0x05) // 视频丢失

/**
 * 统计类告警 (0x6000 - 0x60FF)
 * @struct NET_TV_ALARM_STATISTICS_INFO
 */
#define NET_TV_ALARM_BASE_STATISTICS             0x6000
#define NET_TV_ALARM_PEOPLE_FLOW_STATISTICS      (NET_TV_ALARM_BASE_STATISTICS + 0x01) // 人流统计
#define NET_TV_ALARM_PEOPLE_DENSITY_STATISTICS   (NET_TV_ALARM_BASE_STATISTICS + 0x02) // 人员密度统计
#define NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM   2

/**
 * SDK事件通知 (0x7000 - 0x70FF)
 */
#define NET_TV_NOTIFY_BASE_EVENT                  0x7000
#define NET_TV_NOTIFY_CHANNEL_STATUS              (NET_TV_NOTIFY_BASE_EVENT + 0x01) // 通道上下线状态变更
/* END****************************  Alarm type ************************************************************/

 
/* BEGIN****************************  Enum ************************************************************/
/**
* @enum tagNETTVCommonErrCode
* @brief 通用错误码
* @attention 无 None
*/
typedef enum tagNETTVCommonErrCode
{
    NET_TV_E_FAILED                         = -1,           /* 失败  Failed*/
    NET_TV_E_SUCCEED                        = 0,            /* 成功  Succeeded*/
    NET_TV_E_SVC_FAILED                     = 1,            /* 服务器失败错误码  Common Failed*/
    NET_TV_E_NOT_AUTHORIZED                 = 3,            /* 用户无权限 User Not Authorized*/
    NET_TV_E_NO_USER                        = 5,            /* 用户不存在 User does not exist*/
    NET_TV_E_SDK_NOT_INIT                   = 6,            /* 未初始化SDK Not Init SDK*/

    NET_TV_E_NO_RESULT                      = 11,           /* 查无结果 No result */
    NET_TV_E_NOENOUGH_BUF                   = 12,           /* 缓冲区太小: 接收设备数据的缓冲区  Buffer is too small for receiving device data */
    NET_TV_E_SDK_SOCKET_LSN_FAIL            = 13,           /* 创建socket listen失败  Failed to create socket listen */
    NET_TV_E_INIT_MUTEX_FAIL                = 14,           /* 初始化锁失败  Failed to initialize lock */
    NET_TV_E_INIT_SEMA_FAIL                 = 15,           /* 初始化信号量失败  Failed to initialize semaphore */
    NET_TV_E_ALLOC_RESOURCE_ERROR           = 16,           /* SDK资源分配错误  Error occurred during SDK resource allocation */
    NET_TV_E_HAVEDATA                       = 17,           /* 还有数据   Data not all sent */
    NET_TV_E_NEEDMOREDATA                   = 18,           /* 需要更多数据  More data required  */
    NET_TV_E_TRANSFILE_FAIL                 = 19,           /* 文件传输失败  File transmission failed */
    NET_TV_E_DEVICE_TYPE_ERR                = 20,           /* 不支持的设备类型 Device type that are not supported */
    NET_TV_E_NONCE_TIMEOUT                  = 21,           /* nonce过期 Nonce expired */
    NET_TV_E_INNER_ERR                      = 22,           /* 系统内部错误 System internal error */
    NET_TV_E_BINDNOTIFY_FAIL                = 24,           /* 绑定告警失败  Failed to bind alarms*/

    NET_TV_E_SYSCALL_FALIED                 = 100,          /* 系统函数调用失败，请查看errno  Failed to call system function. See errno */
    NET_TV_E_NULL_POINT                     = 101,          /* 空指针  Null pointer */
    NET_TV_E_INVALID_PARAM                  = 102,          /* 无效参数  Invalid parameter */
    NET_TV_E_INVALID_MODULEID               = 103,          /* 无效模块ID  Invalid module ID */
    NET_TV_E_INVALID_HANDLE                 = 104,          /* 无效的句柄 Invalid handle */
    NET_TV_E_NO_MEMORY                      = 105,          /* 内存分配失败 Memory allocation failed */
    NET_TV_E_FILE_NO_EXIST                  = 106,          /* 文件等不存在 File does not exist */
    NET_TV_E_NO_DEV                         = 107,          /* 设备不存在 Device does not exist*/
    NET_TV_E_NO_FIT_LOG                     = 108,          /* 符合条件的日志不存在 Qualified logs do not exist*/
    NET_TV_E_BUSY                           = 109,          /* busy状态 busy status */
    NET_TV_E_TIMER_REG_FAILED               = 110,          /* 注册定时器失败 Register timer failed */
    NET_TV_E_COMMON_FAILED                  = 111,          /* 通用错误 General error */
    NET_TV_E_CMD_NOT_SUPPORT                = 112,          /* 命令不支持 Command does not support */
    NET_TV_E_NOT_SUPPORT                    = 113,          /* 设备不支持该功能 The device does not support this function */
    NET_TV_E_TIMEOUT                        = 114,          /* 超时 Overtime */
    NET_TV_E_MSG_ERR                        = 115,          /* 消息不匹配 Message mismatch */
    NET_TV_E_MODULE_INEXIST                 = 116,          /* 模块不存在 Module does not exist */
    NET_TV_E_SOCKET_RECV_ERR                = 117,          /* 消息接收失败 Message acceptance failure */
    NET_TV_E_DECODE_IE_FAILED               = 118,          /* 获取消息IE失败 Failure to get message IE */
    NET_TV_E_ENCODE_IE_FAILED               = 119,          /* 添加消息IE失败 Failed to add message IE */
    NET_TV_E_SDK_NOINTE_ERROR               = 120,          /* SDK未初始化 SDK uninitialized */
    NET_TV_E_ALREDY_INIT_ERROR              = 121,          /* SDK已经初始化 SDK has been initialized */
    NET_TV_E_DEVICE_FACTURER_ERR            = 122,          /* 不支持的设备厂商 Unsupported equipment manufacturer */
    NET_TV_E_NAME_EXIST                     = 123,          /* 名称已存在 Name already exists */
    NET_TV_E_GET_CFG_FAILED                 = 124,          /* 获取配置信息出错 Error acquiring configuration information */
    NET_TV_E_SET_CFG_FAILED                 = 125,          /* 设置配置信息出错 Error setting configuration information */
    NET_TV_E_CHANNEL_OVER_SPEC              = 126,          /* 通道数超规格 Channel number exceeding specification */
    NET_TV_E_CALL_DRV_COMMON                = 127,          /* 调用驱动通用失败 Call driver universal failure */
    NET_TV_E_TOTAL_QUOTA_FULL               = 128,          /* 可分配的配额空间不足 Insufficient allocatable quota space */
    NET_TV_E_CALL_DB_COMMON                 = 129,          /* 调用数据库通用失败 Failure to invoke database universality */
    NET_TV_E_NEED_MORE_MEMORY               = 130,          /* 内存分配不足 Insufficient memory allocation */
    NET_TV_E_T2U_CONNECT_FAILED             = 131,          /* T2U连接失败 Failure of T2U connection */
    NET_TV_E_FUNC_IS_INITIALIZING           = 132,          /* 功能正在初始化中 Functions are being initialized */

    NET_TV_E_CONNECT_ERROR                  = 200,          /* 创建连接失败  Failed to create connection */
    NET_TV_E_SEND_MSG_ERROR                 = 201,          /* 发送消息失败 Failed to send message */
    NET_TV_E_DECODE_RSP_ERROR               = 202,          /* 解析响应消息失败  Failed to decode response message */
    NET_TV_E_NONSUPPORT                     = 203,          /* 该功能函数未实现  Function not supported */
    NET_TV_E_JSON_ERROR                     = 204,          /* Json 通用错误  Json common error */
    NET_TV_E_NORESULT                       = 205,          /* 查询结果为空 The query result is empty */
    NET_TV_E_SOCKET_RECV_ERROR              = 206,          /* Socket接收消息失败  Socket failed to receive message */
    NET_TV_E_CREATE_THREAD_FAIL             = 207,          /* 创建线程失败  Failed to create thread */
    NET_TV_E_RESCODE_NO_EXIST               = 208,          /* 资源编码不存在  Resource code not exist */
    NET_TV_E_MSG_DATA_INVALID               = 209,          /* 消息内容错误  Incorrect message content */
    NET_TV_E_JSON_NO_IMAGE                  = 210,          /* 图片数据为空 Picture data is empty */
    NET_TV_E_IMAGE_SIZE_BEYOND_THE_LIMIT    = 211,          /* 图片大小超出限制 Image size beyond the limit*/

}NET_TV_COMMON_ECODE_E;

/**
* @enum tagNETTVMediaErrCode
* @brief 媒体相关错误码
* @attention 无 None
*/
typedef enum tagNETTVMediaErrCode
{
    NET_TV_E_PLAYER_FAIL                        = 1001,     /* 执行失败 Execution failure */
    NET_TV_E_PLAYER_INVALID_PARAM               = 1002,     /* 输入参数非法 Illegal input parameters */
    NET_TV_E_PLAYER_NO_MEMORY                   = 1003,     /* 系统内存不足 Insufficient system memory */
    NET_TV_E_PLAYER_SOCKET_FAIL                 = 1004,     /* 创建SOCKET失败 Failure to create SOCKET */
    NET_TV_E_PLAYER_RECV_FAIL                   = 1005,     /* 接收失败 Failure to receive */
    NET_TV_E_PLAYER_RECV_ZERO                   = 1006,     /* 接收为零 Receive zero */
    NET_TV_E_PLAYER_NOT_SUPPORT                 = 1007,     /* 功能暂不支持 Function not supported for the time being */
    NET_TV_E_PLAYER_CREATETHREAD_FAILED         = 1008,     /* 创建线程失败 Failed to create thread */
    NET_TV_E_PLAYER_OPENDL_FAILED               = 1009,     /* 加载动态库失败 Failure to load dynamic library */
    NET_TV_E_PLAYER_SYMDL_FAILED                = 1010,     /* 获取动态方法失败 Failure to obtain dynamic methods */
    NET_TV_E_PLAYER_SEND_FAILED                 = 1011,     /* 发送失败 Fail in send */
    NET_TV_E_PLAYER_EACCES                      = 1012,     /* 创建文件权限不足 Insufficient permission to create files */
    NET_TV_E_PLAYER_FILE_NOT_FIND               = 1013,     /* 读文件未找到 Reading file not found */
    NET_TV_E_PLAYER_LOG_CLOSE                   = 1014,     /* 日志关闭 Log closes */
    NET_TV_E_PLAYER_MEDIA_EXCEPTION             = 1017,     /* 内部处理异常 Media exception  */
    NET_TV_E_PLAYER_SYS_FAIL                    = 1018,     /* 系统通用错误 */
    NET_TV_E_PLAYER_INIT_DONE                   = 1019,     /* 已经初始化 */
    NET_TV_E_PLAYER_SYS_RES_FAILED              = 1020,     /* 创建系统资源失败 */
    NET_TV_E_PLAYER_INVALID_IP                  = 1021,     /* IP有误 */
    NET_TV_E_PLAYER_EZSTREAMER_FULL             = 1022,     /* EZStreamer 规格满 */
    NET_TV_E_PLAYER_VOD_OVER_ABILITY            = 1023,     /* 一体机流规格满 */
    NET_TV_E_PLAYER_STREAM_IN_PROCESS           = 1024,     /* 流存储已经处理中 */
    NET_TV_E_PLAYER_NO_SPARE_SESSION            = 1025,     /* 无空闲会话 */
    NET_TV_E_PLAYER_NEED_AUTHENTICATE           = 1026,     /* 需要鉴权 */
    NET_TV_E_PLAYER_GET_AUTHENTICATE_FAID       = 1027,     /* 获取鉴权失败 */
    NET_TV_E_PLAYER_MAKE_AUTHENTICATE_FAID      = 1028,     /* 制作鉴权失败 */
    NET_TV_E_PLAYER_AUTHENTICATEINFO_DIFF       = 1029,     /* 鉴权信息不一致 */
    NET_TV_E_PLAYER_SESSION_CLOSED              = 1030,     /* 会话已关闭 */

    /*********************** Player 资源相关********************************/
    NET_TV_E_FAIL_TO_INIT_EZPLAYER              = 1257,     /* 初始化播放器失败 Initialization player failed */
    NET_TV_E_FAIL_TO_ALLOC_PORT_RES             = 1258,     /* 分配播放通道资源失败 Failed to allocate playback channel resources */
    NET_TV_E_FAIL_TO_GET_PORT_RES               = 1259,     /* 获取播放通道资源失败 Failed to obtain playback channel resources*/
    NET_TV_E_BUFFER_QUEUE_FULL                  = 1260,     /* 缓存队列已满 Cache queue full */
    NET_TV_E_BUFFER_QUEUE_EMPTY                 = 1261,     /* 缓存队列空 Cache queue empty */
    NET_TV_E_OPEN_FILE_FAILED                   = 1262,     /* 打开文件失败 Fail to open file */
    NET_TV_E_FILE_READ_END                      = 1263,     /* 文件已经读取完毕 The file has been read out */
    NET_TV_E_FILE_DISKSPACE_FULL                = 1264,     /* 磁盘空间满 Disk space is full */
    NET_TV_E_FILE_READ_FAIL                     = 1265,     /* 读取失败 Read failure */
    NET_TV_E_MCM_MIC_NOT_EXIST                  = 1266,     /* 麦克风不存在 Microphone does not exist */
    NET_TV_E_TS_PACKET_IN_THE_ROUGH             = 1267,     /* TS打包未完成 TS packaging not completed */
    NET_TV_E_FILE_RECORD_FINISH                 = 1268,     /* 录像保存完毕 The video has been saved.*/
    NET_TV_E_VIDEO_RESOLUTION_CHANGE            = 1269,     /* 分辨率发生变化 Resolution changed */

    NET_TV_E_FAIL_TO_OPEN_STREAM                = 1513,     /* 启动媒体流播放失败 Failed to start media stream playback */
    NET_TV_E_FAIL_TO_CLOSE_STREAM               = 1514,     /* 关闭媒体流播放失败 Failed to shut down media stream playback */
    NET_TV_E_FAIL_TO_RECV_DATA                  = 1515,     /* 网络故障导致接收数据失败 Failure of receiving data due to network failure */
    NET_TV_E_FAIL_TO_PROCESS_MEDIA_DATA         = 1516,     /* 媒体数据处理失败 Media data processing failure */
    NET_TV_E_NOT_START_PLAY                     = 1517,     /* 播放通道未开始播放操作 Play channel did not start playback operation */
    NET_TV_E_FAIL_TO_INPUT_DATA                 = 1518,     /* 输入媒体流数据失败 Input media streaming data failed */
    NET_TV_E_INPUTDATA_BUFFER_FULL              = 1519,     /* 输入数据缓存满 Input data cached full */
    NET_TV_E_FAIL_TO_SET_PROCESS_DATA_CB        = 1520,     /* 设置媒体流数据回调函数失败 Failed to set media stream data callback function */
    NET_TV_E_VOICE_RUNNING                      = 1521,     /* 语音业务运行过程中出错 Errors in Voice Service Operation */
    NET_TV_E_FAIL_TO_OPEN_VOICE_SVC             = 1522,     /* 启动语音业务失败 Failure to start voice service */
    NET_TV_E_FAIL_TO_CLOSE_VOICE_SVC            = 1523,     /* 关闭语音业务失败 Failed to shut down voice service */
    NET_TV_E_UNKNOWN_STREAM_TYPE                = 1524,     /* 未知媒体流 Unknown media stream */
    NET_TV_E_PACKET_LOSE                        = 1525,     /* 丢包 Packet loss */
    NET_TV_E_NEED_MORE_PACKET                   = 1526,     /* 拼包未完成，需要更多包 Packing is not completed, more packages are needed */
    NET_TV_E_FAIL_TO_CREATE_DECODE              = 1527,     /* 创建解码器失败 Failed to create decoder */
    NET_TV_E_FAIL_TO_DECODE                     = 1528,     /* 解码失败 Decoding failure */
    NET_TV_E_RECV_DATA_NOTENOUGH                = 1529,     /* 接收数据不足 Insufficient data received */
    NET_TV_E_RENDER_RES_FULL                    = 1530,     /* 显示资源满 Display full resources */
    NET_TV_E_RENDER_RES_NOT_EXIST               = 1531,     /* 显示资源不存在 Show that resources do not exist */
    NET_TV_E_CREATE_DEV_FAILED                  = 1532,     /* 资源创建失败 Resource creation failed */
    NET_TV_E_AUDIO_RES_NOT_EXIST                = 1533,     /* 音频资源不存在 Audio resources do not exist */
    NET_TV_E_IHW265D_NEED_MORE_BITS             = 1534,     /* 解码器需要更多数据 Decoder needs more data */
    NET_TV_E_FAIL_TO_CREATE_ENCODE              = 1535,     /* 创建编码器失败 Failure to create encoder */
    NET_TV_E_CAPTURE_RES_EXIST                  = 1536,     /* 采集资源不存在 Collection resources do not exist */
    NET_TV_E_RECORD_STARTED                     = 1537,     /* 录像已打开 The video has been turned on */
    NET_TV_E_NEED_WAIT_DECODEC                  = 1538,     /* 未解码完成，需要等待 Undecoded, need to wait */
    NET_TV_E_MORE_DATA_NEED_PACKET              = 1539,     /* 数据过多，还需要继续打包 There's too much data to pack. */
    NET_TV_E_AAC_LC_DECODE_FAIL                 = 1540,     /* AAC_LC解码失败 AAC_LC decode failure*/
    NET_TV_E_RENDER_SURFACELOST                 = 1541,     /* 显示表面丢失 */
    NET_TV_E_FILE_ENCRYPED                      = 1543,     /* 文件已加密 */
    NET_TV_E_SCRAMBLING_INFO_FAILED             = 1544,     /* 加扰信息异常 */

    /* 媒体会话业务异常上报错误码 */
    NET_TV_E_LIVE_EXISTED                       = 2000,     /* 实况业务已经建立 Live business has been established */
    NET_TV_E_LIVE_INPUT_NOT_READY               = 2001,     /* 媒体流未准备就绪 Media streaming is not ready */
    NET_TV_E_LIVE_OUTPUT_BUSY                   = 2002,     /* 实况业务显示资源忙 Live business display resources busy */
    NET_TV_E_LIVE_CB_NOTEXIST                   = 2003,     /* 实况控制块不存在 Real-time control block does not exist */
    NET_TV_E_LIVE_STREAM_FULL                   = 2004,     /* 实况流资源已满 Real-time flow resources are full */
    NET_TV_E_LIVE_NET_FAILED                    = 2005,     /* 会话网络错误 */
    NET_TV_E_LIVE_NET_TIMEOUT                   = 2006,     /* 会话网络超时 */
    NET_TV_E_LIVE_SHAKE_FAILED                  = 2007,     /* 会话交互错误 */
    NET_TV_E_LIVE_AUTH_FAILED                   = 2008,     /* 鉴权失败 */
    NET_TV_E_LIVE_INNER_ERROR                   = 2009,     /* 设备侧内部处理错误 */
    NET_TV_E_LIVE_INNER_TIMEOUT                 = 2010,     /* 内部处理超时 */
    NET_TV_E_LIVE_KEEP_ALIVE_FAILED             = 2011,     /* 保活失败 */
    NET_TV_E_LIVE_SESSION_NOT_EXIST             = 2012,     /* 会话不存在 */
    NET_TV_E_LIVE_NOT_ENOUGH_BANDWIDTH2         = 2013,     /* 带宽不足 */
    NET_TV_E_LIVE_REALPLAY_ESTABLISHED          = 2014,     /* 实况业务已经建立 */
    NET_TV_E_LIVE_REALPLAY_RES_BUSY             = 2015,     /* 实况业务显示资源忙 */
    NET_TV_E_LIVE_MULTICAST_DISABLED            = 2016,     /* 组播使能关闭 */
    NET_TV_E_LIVE_MULTICAST_PORT_OCCUPIED       = 2017,     /* 组播端口已被占用 */
    NET_TV_E_LIVE_MULTICAST_PORT_EXHAUSTED      = 2018,     /* 组播端口已耗尽 */
    NET_TV_E_LIVE_MULTICAST_USER_NOT_EXIST      = 2019,     /* 组播用户不存在 */
    NET_TV_E_LIVE_CHANNEL_NOT_ONLINE            = 2020,     /* 通道不在线 */
    NET_TV_E_LIVE_TALKBACK_ENCODED_INVALID      = 2021,     /* 语音对讲资源编码无效 */
    NET_TV_E_LIVE_VOICE_RES_USED_BY_TALKBACK    = 2022,     /* 语音资源已被对讲使用 */
    NET_TV_E_LIVE_TALKBACK_EXISTS               = 2023,     /* 语音对讲已存在 */
    NET_TV_E_LIVE_VOICE_WORK_NOT_EXIST          = 2024,     /* 语音业务不存在 */
    NET_TV_E_LIVE_TALKBACK_TIMEOUT              = 2025,     /* 建立语音对讲业务超时 */
    NET_TV_E_LIVE_TALKBACK_ERROR                = 2026,     /* 语音对讲失败 */
    NET_TV_E_LIVE_UNDEFINED_ERROR               = 2027,     /* 未定义错误 */
    NET_TV_E_LIVE_BAD_REQUEST                   = 2028,     /* 错误的请求 */
    NET_TV_E_LIVE_UNAUTHORIZED                  = 2029,     /* 未通过认证 */
    NET_TV_E_LIVE_PAYMENT_REQUIRED              = 2030,     /* 需要付费 */
    NET_TV_E_LIVE_FORIBIDDEN                    = 2031,     /* 禁止 */
    NET_TV_E_LIVE_METHOD_NOT_ALLOWED            = 2032,     /* 不允许该方法 */
    NET_TV_E_LIVE_NOT_ACCEPTABLE                = 2033,     /* 不接受 */
    NET_TV_E_LIVE_PROXY_REQUIRED                = 2034,     /* 代理需要认证 */
    NET_TV_E_LIVE_REQUEST_TIMEOUT               = 2035,     /* 请求超时 */
    NET_TV_E_LIVE_GONE                          = 2036,     /* 不在服务器 */
    NET_TV_E_LIVE_LENGTH_REQUIRED               = 2037,     /* 需要长度 */
    NET_TV_E_LIVE_PRECONDITION_FAILED           = 2038,     /* 预处理失败 */
    NET_TV_E_LIVE_ENTITY_TOO_LARGE              = 2039,     /* 请求实体过长 */
    NET_TV_E_LIVE_URI_TOO_LARGE                 = 2040,     /* 请求-URI过长 */
    NET_TV_E_LIVE_UNSUPPORTED_TYPE              = 2041,     /* 媒体类型不支持 */
    NET_TV_E_LIVE_NOT_UNDERSTOOD                = 2042,     /* 不理解此参数 */
    NET_TV_E_LIVE_CONFERENCE_NOT_FOUND          = 2043,     /* 找不到会议 */
    NET_TV_E_LIVE_NOT_ENOUGH_BANDWIDTH          = 2044,     /* 带宽不足 */
    NET_TV_E_LIVE_SESSION_NOT_FOUND             = 2045,     /* 找不到会话 */
    NET_TV_E_LIVE_METHOD_NOT_VALID              = 2046,     /* 此状态下此方法无效 */
    NET_TV_E_LIVE_HEADER_NOT_VALID              = 2047,     /* 此头部域对该资源无效 */
    NET_TV_E_LIVE_INVALID_RANGE                 = 2048,     /* 无效范围 */
    NET_TV_E_LIVE_PARAMETER_READ_ONLY           = 2049,     /* 参数是只读的 */
    NET_TV_E_LIVE_AO_NOT_ALLOWED                = 2050,     /* 不允许合控制 */
    NET_TV_E_LIVE_ONLY_AO_ALLOWED               = 2051,     /* 只允许合控制 */
    NET_TV_E_LIVE_UNSUPPORTED_TRANSPORT         = 2052,     /* 传输方式不支持 */
    NET_TV_E_LIVE_DESTINATION_UNREACHABLE       = 2053,     /* 无法到达目的地址 */
    NET_TV_E_LIVE_INTERNAL_SERVER_ERROR         = 2054,     /* 服务器内部错误 */
    NET_TV_E_LIVE_NOT_IMPLEMENTED               = 2055,     /* 未实现 */
    NET_TV_E_LIVE_BAD_GATEWAY                   = 2056,     /* 网关错误 */
    NET_TV_E_LIVE_SERVICE_UNAVAILABLE           = 2057,     /* 无法得到服务 */
    NET_TV_E_LIVE_VERSION_NOT_SUPPORTED         = 2058,     /* 不支持此RTSP版本 */
    NET_TV_E_LIVE_GATEWAY_TIMEOUT               = 2059,     /* 网关超时 */
    NET_TV_E_LIVE_OPTION_NOT_SUPPORTED          = 2060,     /* 不支持选项 */
    NET_TV_E_LIVE_MALLOC_FAIL                   = 2061,     /* 内存分配失败 */
    NET_TV_E_LIVE_REALLOC_FAIL                  = 2062,     /* 内存再分配失败 */
    NET_TV_E_LIVE_DESCRIBE_TIMEOUT              = 2063,     /* describe超时（大GOP、磁盘读取数据慢导致） */
    NET_TV_E_LIVE_IPC_NOTBIND                   = 2064,     /* 通道未绑定，POE通道未接入IPC，非POE通道未添加IPC */
    NET_TV_E_LIVE_DISK_ABNOMAL                  = 2065,     /* 磁盘异常 */

    NET_TV_E_AUDIO_EXISTED                      = 2100,     /* 语音对讲已存在 Speech intercom already exists */
    NET_TV_E_AUDIO_NO_EXISTED                   = 2101,     /* 语音业务不存在 Voice service does not exist */
    NET_TV_E_AUDIO_RESCODE_INVALID              = 2102,     /* 语音对讲资源编码无效 Invalid encoding of voice intercom resources */
    NET_TV_E_AUDIO_RES_USED_BY_TALK             = 2103,     /* 语音资源已被对讲使用  Audio resource is being used by two-way audio */
    NET_TV_E_AUDIO_FAILED                       = 2104,     /* 语音对讲失败 Speech intercom failure */
    NET_TV_E_AUDIO_AUDIOBCAST_FULL              = 2205,     /* 语音业务已满  No more audio service allowed */

    NET_TV_E_CAPTURE_NO_SUPPORT_FORMAT          = 2200,     /* 抓拍格式不支持 Snapshot format does not support  */
    NET_TV_E_CAPTURE_NO_ENOUGH_CAPACITY         = 2201,     /* 硬盘空间不足 Insufficient hard disk space */
    NET_TV_E_CAPTURE_NO_DECODED_PICTURE         = 2202,     /* 没有解码过的图片可供抓拍 Undecoded pictures can be captured */
    NET_TV_E_CAPTURE_SINGLE_FAILED              = 2203,     /* 单次抓拍操作失败 Single snap operation failed */

    NET_TV_E_OVER_ABILITY                       = 2301,     /* 码流超出能力集 Bit stream excess capability set */

    /* 云媒体业务异常上报  Cloud media view exception report 2793~2809 */
    NET_TV_E_CLOUD_DOWNLOAD_FINISH              = 2793,     /* 下载完成 */
    NET_TV_E_CLOUD_PARSE_DOMAIN_FAIL            = 2794,     /* 解析域名失败 */
    NET_TV_E_CLOUD_CONNECT_FAIL                 = 2795,     /* 连接失败 */
    NET_TV_E_CLOUD_CONNECT_TIMEOUT              = 2796,     /* 连接超时 */
    NET_TV_E_CLOUD_DOWNLOAD_TIMEOUT             = 2797,     /* 下载超时 */
    NET_TV_E_CLOUD_DOWNLOAD_FAIL                = 2798,     /* 下载失败 */
    NET_TV_E_CLOUD_NETWORK_POOR                 = 2799,     /* 网络较差 */
    NET_TV_E_CLOUD_PLAY_FINISH                  = 2800,     /* 播放完成 */
    NET_TV_E_CLOUD_DISK_FULL                    = 2801,     /* 磁盘空间满 */
    NET_TV_E_CLOUD_AUTH_FAIL                    = 2802,     /* 鉴权失败 */
    NET_TV_E_CLOUD_CURRENT_TIME                 = 2803,     /* 当前播放时间，仅用于上报 */
    NET_TV_E_CLOUD_PRIOR_DISK_FULL              = 2804,     /* 磁盘预值满 */
    NET_TV_E_CLOUD_NODE_NOT_EXIST               = 2805,     /* 时间节点不存在 */
    NET_TV_E_CLOUD_NO_CACHE_PATH                = 2806,     /* 未设置缓存路径 */
    NET_TV_E_CLOUD_MSG_SEND_FAIL                = 2807,     /* 消息发送失败 */
    NET_TV_E_CLOUD_TASK_CANCELLED               = 2808,     /* 任务已取消 */
    NET_TV_E_CLOUD_TASK_STREAM_CONTINUE         = 2809,     /* 流继续播放 */

    NET_TV_E_MEDIA_INPUT_NOT_READY              = 10000,    /* 媒体流未准备就绪 Media streaming is not ready */
    NET_TV_E_CCB_STATR_INVALID                  = 10001,    /* 控制块状态不可用 Control block state unavailable */
    NET_TV_E_MEDIA_OUTPUT_BUSY                  = 10002,    /* 实况业务显示资源繁忙 Live business display resource busy */
    NET_TV_E_MEDIA_START_LOCAL_LIVE_ERR         = 10003,    /* 实况媒体流未准备就绪 Live media streams are not ready */
    NET_TV_E_MEDIA_START_LOCAL_REPLAY_ERR       = 10004,    /* 回放媒体流未准备就绪 Playback media streams are not ready */

    NET_TV_E_MEDIA_BW_RECV_NOT_ENOUGH           = 10007,    /* 网络接收带宽不足 Insufficient network reception bandwidth */
    NET_TV_E_MEDIA_BW_SEND_NOT_ENOUGH           = 10008,    /* 网络发送带宽不足 Insufficient network transmission bandwidth */
    NET_TV_E_MEDIA_AUDIO_BROADCAST_TO_LIMIT     = 10009,    /* 语音广播业务已达上限 Voice broadcasting service has reached the upper limit */
    NET_TV_E_MEDIA_AUDIO_CHL_BING_USED          = 10010,    /* 音频通道已被占用 Audio channel has been occupied */
    
    NET_TV_E_MEDIA_NOT_SUPPORT_ENCODETYPE       = 10012,    /* 码流格式不支持 Encode type Not supported */

    NET_TV_E_MEDIA_MAX                          = 10399     /* 媒体相关错误码最大值 Maximum Media Related Error Code */
}NET_TV_MEDIA_ECODE_E;

/**
 * enum tagNETTVDeviceType
 * @brief 设备类型
 * @attention 无 None
 */
typedef enum tagNETTVDeviceType
{
    NET_TV_DTYPE_UNKNOWN                        = 0,            /* Unknown type */
    NET_TV_DTYPE_IPC                            = 1,            /* IPC range */
    NET_TV_DTYPE_NVR                            = 2,            /* NVR range */
    NET_TV_DTYPE_INVALID                        = 0xFFFF        /* 无效值  Invalid value */
}NET_TV_DEVICE_TYPE_E;

/**
 * @enum tagNETTVException
 * @brief 异常回调的消息类型 枚举定义 Exception callback message types Enumeration definition 
 * @attention 无 None
 */
typedef enum tagNETTVException
{
   NET_TV_EXCEPTION_REPORT_REMUXING_FINISH     = 284,          /* 转封装完成 */

    /* 回放业务异常上报  Playback exceptions report 300~399 */
   NET_TV_EXCEPTION_REPORT_VOD_END             = 300,          /* 回放结束  Playback ended*/
   NET_TV_EXCEPTION_REPORT_VOD_ABEND           = 301,          /* 回放异常  Playback exception occured */
   NET_TV_EXCEPTION_REPORT_BACKUP_END          = 302,          /* 备份结束  Backup ended */
   NET_TV_EXCEPTION_REPORT_BACKUP_DISC_OUT     = 303,          /* 磁盘被拔出  Disk removed */
   NET_TV_EXCEPTION_REPORT_BACKUP_DISC_FULL    = 304,          /* 磁盘已满  Disk full */
   NET_TV_EXCEPTION_REPORT_BACKUP_ABEND        = 305,          /* 其他原因导致备份失败   Backup failure caused by other reasons */

   NET_TV_EXCEPTION_EXCHANGE                   = 0x8000,       /* 用户交互时异常（用户保活超时）  Exception occurred during user interaction (keep-alive timeout) */
   NET_TV_EXCEPTION_REPORT_ALARM_INTERRUPT     = 0x8001,       /* 告警上报异常结束 保活失败或者长连接断开重订阅成功上报新的订阅ID Failure to report abnormal termination of life preservation or disconnection of long connection */
   NET_TV_EXCEPTION_ALARM_SUBSCRIBE_FAILED     = 0x8002,       /* 告警重订阅失败异常上报 */
   NET_TV_EXCEPTION_COMMON_ALARM_RENEW_FAIL    = 0x8003,       /* 快速告警对接刷新失败异常上报 */


   NET_TV_EXCEPTION_REPORT_MAX,                                /* 最大值  Maximum value */
   NET_TV_EXCEPTION_REPORT_NOT_VALID_PERIOD,                   /* 不在有效期内 Not Valid period */
   NET_TV_EXCEPTION_REPORT_NOT_VALID_TIME,                     /* 不在有效时段内 Not Valid Time */

   NET_TV_EXCEPTION_REPORT_INVALID             = 0xFFFF        /* 无效值  Invalid value */
}NET_TV_EXCEPTION_TYPE_E;

/**
 * @enum tagNETTVCapabilityCommond
 * @brief 能力集命令 Device capability commond
 * @attention 无
*/
typedef enum tagNETTVCapabilityCommond
{
    NET_TV_CAP_VIDEO_ENCODE             = 1,            /* 视频编码能力集 参见# NET_TV_VIDEO_STREAM_CAP_S。 Video encoding capability. See # NET_TV_VIDEO_STREAM_CAP_S for reference*/
    NET_TV_CAP_OSD                      = 2,            /* OSD参数能力集 参见# NET_TV_OSD_CAP_S。 OSD parameter capability. See # NET_TV_OSD_CAP_S for reference*/
    NET_TV_CAP_SMART                    = 3,            /* 智能能力集 参见# NET_TV_SMART_CAP_S。 Smart capability. See # NET_TV_SMART_CAP_S for reference */
    NET_TV_CAP_IMAGE                    = 5,            /* 图像参数能力集 参见#NET_TV_IMAGE_CAP_S。 Image capability See # NET_TV_IMAGE_CAP_S for reference*/
    NET_TV_CAP_AUDIO                    = 6,            /* 音频能力集 参见 NET_TV_AUDIO_CAP_S */
    NET_TV_CAP_CHANNELS_ALARM           = 13,           /* 通道告警能力集, 参见 NET_TV_CHN_ALARM_CAP_INFO_S （单通道IPC对应SDK通道号传入参数1；多通道IPC对应SDK通道号传入参数1+IPC实际通道号；NVR下对应通道号传入实际通道号） */
    NET_TV_CAP_SYS                      = 14,           /* 系统能力集 参见 NET_TV_SYS_CAPABILITY_S  */
    NET_TV_CAP_USER_MANAGE              = 23,           /* 用户管理能力集, 参见 NET_TV_USER_MANAGE_CAP_INFO_S */
    NET_TV_CAP_MEDIA                    = 28,           /* 视频通道媒体能力集，详见 NET_TV_MEDIA_CAP_INFO_S */
    NET_TV_CAP_INVALID                  = 0Xff
}NET_TV_CAPABILITY_COMMOND_E;

typedef enum tagNETTVCfgCmd
{
    NET_TV_GET_DEVICECFG                = 100,              /* 获取设备信息,参见#NET_TV_DEVICE_BASICINFO_S  Get device information, see #NET_TV_DEVICE_BASICINFO_S */
    NET_TV_SET_DEVICECFG                = 101,              /* 保留 Reserved */

    NET_TV_GET_UPGRADESTATUS            = 102,              /* 获取设备升级状态信息 */
    NET_TV_SET_UPGRADE                  = 103,              /* 设置设备升级信息 */
    NET_TV_GET_UPGRADEVERSION           = 104,              /* 获取设备升级版本信息 */  

    NET_TV_GET_NTPCFG                   = 110,              /* 获取NTP参数,参见#NET_TV_SYSTEM_NTP_INFO_S  Get NTP parameter, see #NET_TV_SYSTEM_NTP_INFO_S */
    NET_TV_SET_NTPCFG                   = 111,              /* 设置NTP参数,参见#NET_TV_SYSTEM_NTP_INFO_S  Set NTP parameter, see #NET_TV_SYSTEM_NTP_INFO_S */

    NET_TV_GET_STREAMCFG                = 120,              /* 获取视频编码参数,参见#NET_TV_VIDEO_ENCODE_OPTION_S  Get video encoding parameter, see #NET_TV_VIDEO_ENCODE_OPTION_S */
    NET_TV_SET_STREAMCFG                = 121,              /* 设置视频编码参数,参见#NET_TV_VIDEO_ENCODE_OPTION_S  Set video encoding parameter, see #NET_TV_VIDEO_ENCODE_OPTION_S */
    NET_TV_GET_RTSPURLCFG               = 122,              /* 获取RTSP流地址,参见#NET_TV_RTSP_URL_INFO_S  Get RTSP URL, see #NET_TV_RTSP_URL_INFO_S */
    NET_TV_GET_REPLAY_URLCFG            = 123,              /* 获取回放播放地址,参见#NET_TV_REPLAY_URL_INFO_S  Get playback URL */
    NET_TV_GET_REPLAY_RECORD_LIST       = 124,              /* 获取NVR回放录像时间段,参见#NET_TV_REPLAY_RECORD_LIST_S */
    NET_TV_SET_REPLAY_CTRL              = 125,              /* 控制回放开始/停止/倍速,参见#NET_TV_REPLAY_CTRL_INFO_S */

    NET_TV_GET_AUDIOCFG                 = 130,              /* 获取音频编码参数,参见#NET_TV_AUDIO_CFG_S  Get audio encoding parameter, see #NET_TV_AUDIO_CFG_S */
    NET_TV_SET_AUDIOCFG                 = 131,              /* 设置音频编码参数,参见#NET_TV_AUDIO_CFG_S  Set audio encoding parameter, see #NET_TV_AUDIO_CFG_S */

    NET_TV_GET_OSDCAPCFG                = 140,              /* 获取OSD能力集配置信息,参见#NET_TV_VIDEO_OSD_CFG_S  Get OSD configuration information, see #NET_TV_VIDEO_OSD_CFG_S */
    NET_TV_SET_OSDCAPCFG                = 141,              /* 设置OSD能力集配置信息,参见#NET_TV_VIDEO_OSD_CFG_S  Set OSD configuration information, see #NET_TV_VIDEO_OSD_CFG_S */

    NET_TV_GET_IMAGECFG                 = 160,              /* 获取图像配置信息,参见#NET_TV_IMAGE_SETTING_S  Get image configuration information, see #NET_TV_IMAGE_SETTING_S */
    NET_TV_SET_IMAGECFG                 = 161,              /* 设置图像配置信息,参见#NET_TV_IMAGE_SETTING_S  Set image configuration information, see #NET_TV_IMAGE_SETTING_S */

    NET_TV_GET_NETWORKCFG               = 170,              /* 获取网络配置信息,参见#NET_TV_NETWORKCFG_S  Get network configuration information, see #NET_TV_NETWORKCFG_S */
    NET_TV_SET_NETWORKCFG               = 171,              /* 设置网络配置信息,参见#NET_TV_NETWORKCFG_S  Set network configuration information, see #NET_TV_NETWORKCFG_S */

    NET_TV_GET_PRIVACYMASKCFG           = 180,              /* 获取隐私遮盖配置信息,参见#NET_TV_PRIVACY_MASK_CFG_S  Get privacy mask configuration information, see #NET_TV_PRIVACY_MASK_CFG_S */
    NET_TV_SET_PRIVACYMASKCFG           = 181,              /* 设置隐私遮盖配置信息,参见#NET_TV_PRIVACY_MASK_CFG_S  Set privacy mask configuration information, see #NET_TV_PRIVACY_MASK_CFG_S */
    
    NET_TV_GET_TAMPERALARM              = 190,              /* 获取遮挡检测告警信息  参见#NET_TV_TAMPER_ALARM_INFO_S  Get tamper alarm configuration information, see #NET_TV_TAMPER_ALARM_INFO_S */
    NET_TV_SET_TAMPERALARM              = 191,              /* 设置遮挡检测告警信息  参见#NET_TV_TAMPER_ALARM_INFO_S  Set tamper alarm configuration information, see #NET_TV_TAMPER_ALARM_INFO_S */

    NET_TV_GET_MOTIONALARM              = 200,              /* 获取运动检测告警信息 参见#NET_TV_MOTION_ALARM_INFO_S  Get motion alarm configuration information, see #NET_TV_MOTION_ALARM_INFO_S */
    NET_TV_SET_MOTIONALARM              = 201,              /* 设置运动检测告警信息 参见#NET_TV_MOTION_ALARM_INFO_S  Set motion alarm configuration information, see #NET_TV_MOTION_ALARM_INFO_S */

    NET_TV_GET_CROSSLINEALARM           = 202,              /* 获取越界检测告警信息 参见NET_TV_CROSS_LINE_ALARM_INFO_S Get Cross Line alarm configuration information, see #NET_TV_CROSS_LINE_ALARM_INFO_S*/
    NET_TV_SET_CROSSLINEALARM           = 203,              /* 设置越界检测告警信息 参见NET_TV_CROSS_LINE_ALARM_INFO_S Set Cross Line alarm configuration information, see #NET_TV_CROSS_LINE_ALARM_INFO_S*/

    NET_TV_GET_INTRUSIONALARM           = 204,              /* 获取入侵检测告警信息 参见NET_TV_INTRUSION_ALARM_INFO_S Get intrusion alarm configuration information, see #NET_TV_INTRUSION_ALARM_INFO_S*/
    NET_TV_SET_INTRUSIONALARM           = 205,              /* 设置入侵检测告警信息 参见NET_TV_INTRUSION_ALARM_INFO_S Set intrusion alarm configuration information, see #NET_TV_INTRUSION_ALARM_INFO_S*/

    NET_TV_GET_LOITERINGALARM           = 206,              /* 获取徘徊侦测告警信息 参见NET_TV_LOITERING_ALARM_INFO_S Get loitering alarm configuration information, see #NET_TV_LOITERING_ALARM_INFO_S*/
    NET_TV_SET_LOITERINGALARM           = 207,              /* 设置徘徊侦测告警信息 参见NET_TV_LOITERING_ALARM_INFO_S Set loitering alarm configuration information, see #NET_TV_LOITERING_ALARM_INFO_S*/

    NET_TV_GET_CAPTURE_PLAN_INFO        = 208,              /* 获取抓图计划信息 */
    NET_TV_SET_CAPTURE_PLAN_INFO        = 209,              /* 设置抓图计划信息 */
    NET_TV_GET_CAPTURE_PARAM_INFO       = 210,              /* 获取抓图参数信息 */
    NET_TV_SET_CAPTURE_PARAM_INFO       = 211,              /* 设置抓图参数信息 */

    NET_TV_GET_EXPOSURE_INFO            = 212,              /* 获取曝光信息 */
    NET_TV_SET_EXPOSURE_INFO            = 213,              /* 设置曝光信息 */
    NET_TV_GET_DAYNIGHT_INFO            = 214,              /* 获取日夜转换信息 */
    NET_TV_SET_DAYNIGHT_INFO            = 215,              /* 设置日夜转换信息 */
    NET_TV_GET_BACKLIGHT_INFO           = 216,              /* 获取背光信息 */
    NET_TV_SET_BACKLIGHT_INFO           = 217,              /* 设置背光信息 */
    NET_TV_GET_DENOISE_INFO             = 218,              /* 获取降噪信息 */
    NET_TV_SET_DENOISE_INFO             = 219,              /* 设置降噪信息 */
    NET_TV_GET_WHITEBALANCE_INFO        = 220,              /* 获取白平衡信息 */
    NET_TV_SET_WHITEBALANCE_INFO        = 221,              /* 设置白平衡信息 */

    NET_TV_GET_AUDIOANOMALYALARM        = 222,              /* 获取音频异常侦测告警信息 参见NET_TV_AUDIO_ANOMALY_ALARM_INFO_S */
    NET_TV_SET_AUDIOANOMALYALARM        = 223,              /* 设置音频异常侦测告警信息 参见NET_TV_AUDIO_ANOMALY_ALARM_INFO_S */
    NET_TV_GET_PREVIEW_INFO             = 224,              /* 获取预览信息 参见NET_TV_PREVIEW_INFO_S */
    NET_TV_SET_PREVIEW_INFO             = 225,              /* 设置预览信息 参见NET_TV_PREVIEW_INFO_S */
    NET_TV_GET_SCENECHANGEALARM         = 226,              /* 获取场景变更侦测告警信息 参见NET_TV_SCENE_CHANGE_ALARM_INFO_S */
    NET_TV_SET_SCENECHANGEALARM         = 227,              /* 设置场景变更侦测告警信息 参见NET_TV_SCENE_CHANGE_ALARM_INFO_S */
    NET_TV_GET_CROWDGATHERINGALARM      = 228,              /* 获取人员聚集侦测告警信息 参见NET_TV_CROWD_GATHERING_ALARM_INFO_S */
    NET_TV_SET_CROWDGATHERINGALARM      = 229,              /* 设置人员聚集侦测告警信息 参见NET_TV_CROWD_GATHERING_ALARM_INFO_S */
    NET_TV_GET_PARKINGALARM             = 230,              /* 获取停车侦测告警信息 参见NET_TV_PARKING_ALARM_INFO_S */
    NET_TV_SET_PARKINGALARM             = 231,              /* 设置停车侦测告警信息 参见NET_TV_PARKING_ALARM_INFO_S */
    NET_TV_GET_UNATTENDEDOBJECTALARM    = 232,              /* 获取物品遗留侦测告警信息 参见NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S */
    NET_TV_SET_UNATTENDEDOBJECTALARM    = 233,              /* 设置物品遗留侦测告警信息 参见NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S */
    NET_TV_GET_OBJECTREMOVALALARM       = 234,              /* 获取物品拿取侦测告警信息 参见NET_TV_OBJECT_REMOVAL_ALARM_INFO_S */
    NET_TV_SET_OBJECTREMOVALALARM       = 235,              /* 设置物品拿取侦测告警信息 参见NET_TV_OBJECT_REMOVAL_ALARM_INFO_S */
    NET_TV_SET_CONFIG_WIFI_STA          = 236,              /* WIFI配置（STA） 参见 NET_TV_WIFI_STA_CFG_S */
    NET_TV_CONNECT_WIFI_STA             = 237,              /* WIFI连接（STA） 参见 NET_TV_WIFI_STA_CONNECT_S */
    NET_TV_DISCONNECT_WIFI_STA          = 238,              /* WIFI断开（STA） */
    NET_TV_GET_4G_INFO                  = 239,              /* 获取4G配置 参见 NET_TV_4G_INFO_S */
    NET_TV_SET_4G_INFO                  = 240,              /* 设置4G配置 参见 NET_TV_4G_INFO_S */
    NET_TV_SET_HOTSPOT_INFO             = 241,              /* 设置热点配置 参见 NET_TV_HOTSPOT_INFO_S */
    
    NET_TV_GET_ENTERREGIONALARM         = 242,              /* 获取进入区域侦测告警信息 参见NET_TV_ENTER_REGION_ALARM_INFO_S */
    NET_TV_SET_ENTERREGIONALARM         = 243,              /* 设置进入区域侦测告警信息 参见NET_TV_ENTER_REGION_ALARM_INFO_S */
    NET_TV_GET_LEAVEREGIONALARM         = 244,              /* 获取离开区域侦测告警信息 参见NET_TV_LEAVE_REGION_ALARM_INFO_S */
    NET_TV_SET_LEAVEREGIONALARM         = 245,              /* 设置离开区域侦测告警信息 参见NET_TV_LEAVE_REGION_ALARM_INFO_S */
    NET_TV_GET_FACECAPTUREINFO          = 246,              /* 获取人脸抓拍配置信息 参见NET_TV_FACE_CAPTURE_INFO_S */
    NET_TV_SET_FACECAPTUREINFO          = 247,              /* 设置人脸抓拍配置信息 参见NET_TV_FACE_CAPTURE_INFO_S */
    NET_TV_GET_HOTSPOT_CONN             = 248,              /* 获取热点连接设备 参见 NET_TV_HOTSPOT_CONN_INFO_S */

    NET_TV_GET_CHANNEL_INFO             = 300,              /* 获取通道信息 参见NET_TV_CHANNEL_INFO_S */
    NET_TV_GET_CHANNEL_LIST             = 301,              /* 获取全部通道信息 参见NET_TV_CHANNEL_LIST_S */

    NET_TV_STATE_TALKBACK               = 400,              /* 设置对讲状态信息 参见NET_TV_INTERCOM_INFO_S */
    NET_TV_TO_STREAM_TALKBACK           = 401,              /* 流媒体对讲：发送对讲数据 参见NET_TV_REPLAY_TALKBACK_INFO_S */
    NET_TV_FROM_STREAM_TALKBACK         = 402,              /* 流媒体对讲：接收对讲数据 参见NET_TV_REPLAY_TALKBACK_INFO_S */
    NET_TV_REPLAY_TALKBACK              = 403,              /* 流媒体对讲：回放对讲数据 参见NET_TV_REPLAY_TALKBACK_INFO_S */

    NET_TV_GET_GARBAGE_EXPOSURE_CFG     = 404,              /* 获取垃圾暴露配置 参见NET_TV_GARBAGE_EXPOSURE_CFG_S */
    NET_TV_SET_GARBAGE_EXPOSURE_CFG     = 405,              /* 设置垃圾暴露配置 参见NET_TV_GARBAGE_EXPOSURE_CFG_S */
    NET_TV_GET_GARBAGE_OVERFLOW_CFG     = 406,              /* 获取垃圾满溢配置 参见NET_TV_GARBAGE_OVERFLOW_CFG_S */
    NET_TV_SET_GARBAGE_OVERFLOW_CFG     = 407,              /* 设置垃圾满溢配置 参见NET_TV_GARBAGE_OVERFLOW_CFG_S */

    NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG = 408,           /* 获取人流统计配置 参见NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S */
    NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG = 409,           /* 设置人流统计配置 参见NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S */
    NET_TV_RESET_PEOPLE_FLOW_STATISTICS   = 410,           /* 立即清零人流统计结果 */
    NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG = 411,         /* 获取人员密度检测配置 参见NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S */
    NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG = 412,         /* 设置人员密度检测配置 参见NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S */

    NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG = 413,           /* 获取井盖异常检测配置 参见NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S */
    NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG = 414,           /* 设置井盖异常检测配置 参见NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S */
    NET_TV_GET_SLEEP_ON_DUTY_CFG          = 415,           /* 获取睡岗识别配置 参见NET_TV_SLEEP_ON_DUTY_CFG_S */
    NET_TV_SET_SLEEP_ON_DUTY_CFG          = 416,           /* 设置睡岗识别配置 参见NET_TV_SLEEP_ON_DUTY_CFG_S */
    NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG = 417,    /* 获取电瓶车进电梯识别配置 参见NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S */
    NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG = 418,    /* 设置电瓶车进电梯识别配置 参见NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S */
    NET_TV_GET_PERSON_FALL_DOWN_CFG       = 419,           /* 获取人员倒地识别配置 参见NET_TV_PERSON_FALL_DOWN_CFG_S */
    NET_TV_SET_PERSON_FALL_DOWN_CFG       = 420,           /* 设置人员倒地识别配置 参见NET_TV_PERSON_FALL_DOWN_CFG_S */
    NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG = 421,         /* 获取施工占道识别配置 参见NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S */
    NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG = 422,         /* 设置施工占道识别配置 参见NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S */
    NET_TV_GET_CONGESTION_CFG             = 423,           /* 获取拥堵识别配置 参见NET_TV_CONGESTION_CFG_S */
    NET_TV_SET_CONGESTION_CFG             = 424,           /* 设置拥堵识别配置 参见NET_TV_CONGESTION_CFG_S */
    NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG = 425,        /* 获取车牌识别配置 参见NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S */
    NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG = 426,        /* 设置车牌识别配置 参见NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S */
    NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG = 427,           /* 获取高空安全带识别配置 参见NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S */
    NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG = 428,           /* 设置高空安全带识别配置 参见NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S */
    NET_TV_GET_SAFETY_HELMET_CFG          = 429,           /* 获取安全帽识别配置 参见NET_TV_SAFETY_HELMET_CFG_S */
    NET_TV_SET_SAFETY_HELMET_CFG          = 430,           /* 设置安全帽识别配置 参见NET_TV_SAFETY_HELMET_CFG_S */
    NET_TV_GET_PERSON_FALL_CFG            = 431,           /* 获取摔倒识别配置 参见NET_TV_PERSON_FALL_CFG_S */
    NET_TV_SET_PERSON_FALL_CFG            = 432,           /* 设置摔倒识别配置 参见NET_TV_PERSON_FALL_CFG_S */
    NET_TV_GET_PHONE_USAGE_CFG            = 433,           /* 获取玩手机识别配置 参见NET_TV_PHONE_USAGE_CFG_S */
    NET_TV_SET_PHONE_USAGE_CFG            = 434,           /* 设置玩手机识别配置 参见NET_TV_PHONE_USAGE_CFG_S */
    NET_TV_GET_SMOKING_CFG                = 435,           /* 获取抽烟识别配置 参见NET_TV_SMOKING_CFG_S */
    NET_TV_SET_SMOKING_CFG                = 436,           /* 设置抽烟识别配置 参见NET_TV_SMOKING_CFG_S */
    NET_TV_GET_OPEN_FLAME_CFG             = 437,           /* 获取明火识别配置 参见NET_TV_OPEN_FLAME_CFG_S */
    NET_TV_SET_OPEN_FLAME_CFG             = 438,           /* 设置明火识别配置 参见NET_TV_OPEN_FLAME_CFG_S */
    NET_TV_GET_BARE_SOIL_CFG              = 439,           /* 获取黄土裸露识别配置 参见NET_TV_BARE_SOIL_CFG_S */
    NET_TV_SET_BARE_SOIL_CFG              = 440,           /* 设置黄土裸露识别配置 参见NET_TV_BARE_SOIL_CFG_S */
    NET_TV_GET_HOLE_PROTECTION_BAR_CFG    = 441,           /* 获取洞口防护栏识别配置 参见NET_TV_HOLE_PROTECTION_BAR_CFG_S */
    NET_TV_SET_HOLE_PROTECTION_BAR_CFG    = 442,           /* 设置洞口防护栏识别配置 参见NET_TV_HOLE_PROTECTION_BAR_CFG_S */
    NET_TV_GET_REFLECTIVE_CLOTHING_CFG    = 443,           /* 获取反光衣识别配置 参见NET_TV_REFLECTIVE_CLOTHING_CFG_S */
    NET_TV_SET_REFLECTIVE_CLOTHING_CFG    = 444,           /* 设置反光衣识别配置 参见NET_TV_REFLECTIVE_CLOTHING_CFG_S */

    NET_TV_GET_PET_RECOGNITION_INFO       = 445,           /* 获取宠物识别配置 参见NET_TV_PET_RECOGNITION_INFO_S */
    NET_TV_SET_PET_RECOGNITION_INFO       = 446,           /* 设置宠物识别配置 参见NET_TV_PET_RECOGNITION_INFO_S */
    NET_TV_GET_CLIMB_FENCE_INFO           = 447,           /* 获取翻越围栏配置 参见NET_TV_CLIMB_FENCE_INFO_S */
    NET_TV_SET_CLIMB_FENCE_INFO           = 448,           /* 设置翻越围栏配置 参见NET_TV_CLIMB_FENCE_INFO_S */
    NET_TV_GET_DIMISSION_INFO             = 449,           /* 获取离岗配置 参见NET_TV_DIMISSION_INFO_S */
    NET_TV_SET_DIMISSION_INFO             = 450,           /* 设置离岗配置 参见NET_TV_DIMISSION_INFO_S */
    NET_TV_GET_ILLEGAL_LANE_INFO          = 451,           /* 获取违规变道配置 参见NET_TV_ILLEGAL_LANE_INFO_S */
    NET_TV_SET_ILLEGAL_LANE_INFO          = 452,           /* 设置违规变道配置 参见NET_TV_ILLEGAL_LANE_INFO_S */
    NET_TV_GET_RETROGRADE_INFO            = 453,           /* 获取逆行配置 参见NET_TV_RETROGRADE_INFO_S */
    NET_TV_SET_RETROGRADE_INFO            = 454,           /* 设置逆行配置 参见NET_TV_RETROGRADE_INFO_S */
    NET_TV_GET_NONMOTOR_VEHICLE_INTRUSION_INFO = 455,     /* 获取非机动车闯入配置 参见NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S */
    NET_TV_SET_NONMOTOR_VEHICLE_INTRUSION_INFO = 456,     /* 设置非机动车闯入配置 参见NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S */
    NET_TV_GET_OCCUPATION_EMERGENCY_INFO  = 457,           /* 获取应急车道占用识别配置 参见NET_TV_OCCUPATION_EMERGENCY_INFO_S */
    NET_TV_SET_OCCUPATION_EMERGENCY_INFO  = 458,           /* 设置应急车道占用识别配置 参见NET_TV_OCCUPATION_EMERGENCY_INFO_S */
    NET_TV_GET_PEDESTRIAN_INTRUSION_INFO  = 459,           /* 获取行人闯入配置 参见NET_TV_PEDESTRIAN_INTRUSION_INFO_S */
    NET_TV_SET_PEDESTRIAN_INTRUSION_INFO  = 460,           /* 设置行人闯入配置 参见NET_TV_PEDESTRIAN_INTRUSION_INFO_S */
    NET_TV_GET_SMOKE_FIRE_CFG             = 461,           /* 获取烟火识别配置 参见NET_TV_SMOKE_FIRE_CFG_S */
    NET_TV_SET_SMOKE_FIRE_CFG             = 462,           /* 设置烟火识别配置 参见NET_TV_SMOKE_FIRE_CFG_S */
    NET_TV_GET_ROAD_PONDING_CFG           = 463,           /* 获取道路积水检测配置 参见NET_TV_ROAD_PONDING_CFG_S */
    NET_TV_SET_ROAD_PONDING_CFG           = 464,           /* 设置道路积水检测配置 参见NET_TV_ROAD_PONDING_CFG_S */
    NET_TV_GET_SECURITY_SERVICES_INFO     = 465,           /* 获取安全服务配置 参见NET_TV_SECURITY_SERVICES_INFO_S */
    NET_TV_SET_SECURITY_SERVICES_INFO     = 466,           /* 设置安全服务配置 参见NET_TV_SECURITY_SERVICES_INFO_S */
    NET_TV_GET_SSH_COUNTDOWN              = 467,           /* 获取SSH倒计时 参见NET_TV_SSH_COUNTDOWN_INFO_S */
    NET_TV_FIND_LOG                       = 468,           /* 查询日志 参见NET_TV_LOG_LIST_S */
    NET_TV_EXPORT_LOG                     = 469,           /* 导出日志 参见NET_TV_LOG_LIST_S */
    NET_TV_GET_LOG_SERVER                 = 470,           /* 获取日志服务器配置 参见NET_TV_LOG_SERVER_INFO_S */
    NET_TV_SET_LOG_SERVER                 = 471,           /* 设置日志服务器配置 参见NET_TV_LOG_SERVER_INFO_S */
    NET_TV_TEST_LOG_SERVER                = 472,           /* 测试日志服务器配置 参见NET_TV_LOG_SERVER_INFO_S */
    NET_TV_CONTROL_RECORD_INFO            = 473,           /* 手动录像/停止录像 参见NET_TV_RECORD_INFO_S，IPC实际接AC_SET_HUMAN_RECORD */
    NET_TV_GET_RECORD_STATUS              = 474,           /* 获取录像状态 参见NET_TV_RECORD_STATUS_INFO_S */
    NET_TV_GET_RECORD_SCHEDULE            = 475,           /* 获取录像计划 参见NET_TV_RECORD_SCHEDULE_S */
    NET_TV_SET_RECORD_SCHEDULE            = 476,           /* 设置录像计划 参见NET_TV_RECORD_SCHEDULE_S */
    NET_TV_GET_RECORD_ADVANCED_PARAM      = 477,           /* 获取录像高级参数 参见NET_TV_RECORD_ADVANCED_PARAM_S */
    NET_TV_SET_RECORD_ADVANCED_PARAM      = 478,           /* 设置录像高级参数 参见NET_TV_RECORD_ADVANCED_PARAM_S */
    NET_TV_FIND_RECORD_FILE_INFO          = 479,           /* 查找录像文件 参见NET_TV_RECORD_FILE_LIST_S */
    NET_TV_DOWNLOAD_RECORD_FILE           = 480,           /* 下载录像文件 参见NET_TV_RECORD_DOWNLOAD_LIST_S */
    NET_TV_NOTICE_DOWNLOAD_RECORD_PROGRESS = 481,          /* 录像下载进度通知 参见NET_TV_RECORD_DOWNLOAD_PROGRESS_S */
    NET_TV_SET_FACE_COMPARE_INFO          = 482,           /* 设置人脸比对配置 参见NET_TV_FACE_COMPARE_INFO_S */
    NET_TV_ADD_TARGET_LIB                 = 483,           /* 添加目标库 参见NET_TV_FACE_LIB_INFO_S */
    NET_TV_DEL_TARGET_LIB                 = 484,           /* 删除目标库 参见NET_TV_FACE_LIB_INFO_S */
    NET_TV_SET_TARGET_LIB                 = 485,           /* 修改目标库 参见NET_TV_FACE_LIB_INFO_S */
    NET_TV_GET_TARGET_LIB                 = 486,           /* 获取目标库 参见NET_TV_FACE_LIB_LIST_S */
    NET_TV_ADD_FACE_INFO                  = 487,           /* 添加人脸 参见NET_TV_FACE_INFO_S */
    NET_TV_DEL_FACE_INFO                  = 488,           /* 删除人脸 参见NET_TV_FACE_ID_INFO_S */
    NET_TV_SET_FACE_INFO                  = 489,           /* 修改人脸 参见NET_TV_FACE_INFO_S */
    NET_TV_GET_FACE_INFO                  = 490,           /* 获取人脸 参见NET_TV_FACE_INFO_LIST_S */
 
    NET_TV_CFG_INVALID                  = 0xFFFF            /* 无效值  Invalid value */

}NET_TV_CONFIG_COMMAND_E;

typedef enum tagNETTVReplayCtrlCmd
{
    NET_TV_REPLAY_CTRL_START            = 1,                /* 开始播放，同时返回回放URL和会话ID */
    NET_TV_REPLAY_CTRL_STOP             = 2,                /* 停止播放 */
    NET_TV_REPLAY_CTRL_SET_SPEED        = 3,                /* 倍速播放 */
    NET_TV_REPLAY_CTRL_INVALID          = 0xff
} NET_TV_REPLAY_CTRL_CMD_E;

/**
 * @enum tagNETTVVideoCodeType
 * @brief 视频编码格式 枚举定义  Video encoding format Enumeration definition
 * @attention 无 None
 */
typedef enum tagNETTVVideoCodeType
{
    NET_TV_VIDEO_CODE_MJPEG     = 0,          /* MJPEG */
    NET_TV_VIDEO_CODE_H264      = 1,          /* H.264 */
    NET_TV_VIDEO_CODE_H265      = 2,          /* H.265 */
    NET_TV_VIDEO_CODE_JPEG      = 3,          /* JPEG */
    NET_TV_VIDEO_CODE_SVAC3     = 4,          /* SVAC3 */
    NET_TV_VIDEO_CODE_MPEG4     = 5,          /* MPEG4 */
    NET_TV_VIDEO_CODE_INVALID
}NET_TV_VIDEO_CODE_TYPE_E;

/**
 * @enum tagNETTVAudioInputType
 * @brief 音频输入类型 枚举定义 Audio input type Enumeration definition
 * @attention 无 None
 */
typedef enum tagNETTVAudioInputType
{
    NET_TV_AUDIO_INPUT_MICIN   = 0,   /* 麦克风输入 */
    NET_TV_AUDIO_INPUT_LINEIN  = 1,   /* 线路输入  */
    NET_TV_AUDIO_INPUT_INVALID
}NET_TV_AUDIO_INPUT_TYPE_E;


/**
 * @enum tagNETTVAudioOutputType
 * @brief 音频输出类型 枚举定义 Audio output type Enumeration definition
 * @attention 无 None
 */
typedef enum tagNETTVAudioOutputType
{
    NET_TV_AUDIO_OUTPUT_SPEAKER  = 0, /* 扬声器输出 */
    NET_TV_AUDIO_OUTPUT_LINEOUT  = 1, /* 线路输出   */
    NET_TV_AUDIO_OUTPUT_MUTE     = 2, /* 静音       */
    NET_TV_AUDIO_OUTPUT_INVALID
}NET_TV_AUDIO_OUTPUT_TYPE_E;


/**
 * @enum tagNETTVAudioFormat
 * @brief 音频编码格式 枚举定义 Audio encoding format Enumeration definition
 * @attention 无 None
 */
typedef enum tagNETTVAudioFormat
{
    NET_TV_AUDIO_FORMAT_G722_1  = 0,  /* G722.1 */
    NET_TV_AUDIO_FORMAT_G711U   = 1,  /* G711U  */
    NET_TV_AUDIO_FORMAT_G711A   = 2,  /* G711A  */
    NET_TV_AUDIO_FORMAT_MP2L2   = 3,  /* MP2L2  */
    NET_TV_AUDIO_FORMAT_G726    = 4,  /* G726   */
    NET_TV_AUDIO_FORMAT_AAC     = 5,  /* AAC    */
    NET_TV_AUDIO_FORMAT_PCM     = 6,  /* PCM    */
    NET_TV_AUDIO_FORMAT_MP3     = 7,  /* MP3    */
    NET_TV_AUDIO_FORMAT_INVALID
}NET_TV_AUDIO_FORMAT_E;


/**
 * @enum tagNETTVAudioSampleRate
 * @brief 音频采样率 枚举定义 Audio sample rate Enumeration definition
 * @attention 无 None
 */
typedef enum tagNETTVAudioSampleRate
{
    NET_TV_AUDIO_SAMPRATE_8000   = 8000,   /* 8KHz   */
    NET_TV_AUDIO_SAMPRATE_11025  = 11025,  /* 11.025KHz */
    NET_TV_AUDIO_SAMPRATE_12000  = 12000,  /* 12KHz  */
    NET_TV_AUDIO_SAMPRATE_16000  = 16000,  /* 16KHz  */
    NET_TV_AUDIO_SAMPRATE_22050  = 22050,  /* 22.05KHz */
    NET_TV_AUDIO_SAMPRATE_24000  = 24000,  /* 24KHz  */
    NET_TV_AUDIO_SAMPRATE_32000  = 32000,  /* 32KHz  */
    NET_TV_AUDIO_SAMPRATE_44100  = 44100,  /* 44.1KHz */
    NET_TV_AUDIO_SAMPRATE_48000  = 48000,  /* 48KHz  */
    NET_TV_AUDIO_SAMPRATE_64000  = 64000,  /* 64KHz  */
    NET_TV_AUDIO_SAMPRATE_96000  = 96000,  /* 96KHz  */
    NET_TV_AUDIO_SAMPRATE_INVALID = -1
}NET_TV_AUDIO_SAMPRATE_E;


/**
 * @enum tagNETTVAudioBitRate
 * @brief 音频码率 枚举定义 Audio bit rate Enumeration definition
 * @attention 无 None
 */
typedef enum tagNETTVAudioBitRate
{
    NET_TV_AUDIO_BITRATE_16K    = 16000,   /* 16Kbps  */
    NET_TV_AUDIO_BITRATE_32K    = 32000,   /* 32Kbps  */
    NET_TV_AUDIO_BITRATE_48K    = 48000,   /* 48Kbps  */
    NET_TV_AUDIO_BITRATE_64K    = 64000,   /* 64Kbps  */
    NET_TV_AUDIO_BITRATE_96K    = 96000,   /* 96Kbps  */
    NET_TV_AUDIO_BITRATE_128K   = 128000,  /* 128Kbps */
    NET_TV_AUDIO_BITRATE_256K   = 256000,  /* 256Kbps */
    NET_TV_AUDIO_BITRATE_INVALID = -1
}NET_TV_AUDIO_BITRATE_E;

/**
 * @enum tagNETTVLiveStreamIndex
 * @brief 实况业务流索引 枚举定 义 Live stream index Enumeration definition
 * @attention 无 None
 */
typedef enum tagNET_TVLiveStreamIndex
{
    NET_TV_LIVE_STREAM_INDEX_MAIN       = 0,    /* 主流  Main stream */
    NET_TV_LIVE_STREAM_INDEX_AUX        = 1,    /* 辅流  Sub stream */
    NET_TV_LIVE_STREAM_INDEX_THIRD      = 2,    /* 第三流  Third stream */
    NET_TV_LIVE_STREAM_INDEX_ADAPTIVE   = 10,   /* 自适应  Adaptive stream */
    NET_TV_LIVE_STREAM_INDEX_INVALID    = 0xFF  /* 无效值  Invalid value */
}NET_TV_LIVE_STREAM_INDEX_E;


/**
 * @enum tagNETTVOsdDateFormat
 * @brief OSD日期格式 枚举定义 (对应 OSD_DATE_FORMAT_E)
 * @attention
 */
typedef enum tagNETTVOsdDateFormat 
{
    NET_TV_OSD_DATE_YYYY_MM_DD = 0,         /* YYYY-MM-DD(年月日), 例如 2024-10-01 */
    NET_TV_OSD_DATE_MM_DD_YYYY = 1,         /* MM-DD-YYYY(月日年), 例如 10-01-2024 */
    NET_TV_OSD_DATE_DD_MM_YYYY = 2,         /* DD-MM-YYYY(日月年), 例如 01-10-2024 */
    NET_TV_OSD_DATE_YYYY_MM_DD_CHN = 3,     /* YYYY年MM月DD日,     例如 2024年10月01日 */
    NET_TV_OSD_DATE_MM_DD_YYYY_CHN = 4,     /* MM月DD日YYYY年,     例如 10月01日2024年 */
    NET_TV_OSD_DATE_DD_MM_YYYY_CHN = 5,     /* DD日MM月YYYY年,     例如 01日10月2024年 */
    NET_TV_OSD_DATE_YYYY_MM_DD_SLASH = 6,   /* YYYY/MM/DD(年月日), 例如 2024/10/01 */
    NET_TV_OSD_DATE_MM_DD_YYYY_SLASH = 7,   /* MM/DD/YYYY(月日年), 例如 10/01/2024 */
    NET_TV_OSD_DATE_DD_MM_YYYY_SLASH = 8,   /* DD/MM/YYYY(日月年), 例如 01/10/2024 */
    NET_TV_OSD_DATE_FORMAT_INVALID = 0xFF
} NET_TV_OSD_DATE_FORMAT_E;

/**
 * @enum tagNETTVOsdTimeFormat
 * @brief OSD时间格式 枚举定义 (对应 OSD_TIME_FORMAT_E)
 * @attention
 */
typedef enum tagNETTVOsdTimeFormat
{
    NET_TV_OSD_TIME_FORMAT_24 = 0,          /* 24小时制 */
    NET_TV_OSD_TIME_FORMAT_12 = 1,          /* 12小时制 */
    NET_TV_OSD_TIME_FORMAT_INVALID = 0xFF
} NET_TV_OSD_TIME_FORMAT_E;

/**
 * @enum tagNETTVOsdFontSize
 * @brief OSD字体大小 枚举定义 (对应 OSD_FONT_SIZE_E)
 * @attention
 */
typedef enum tagNETTVOsdFontSize
{
    NET_TV_OSD_FONT_SIZE_ADAPTIVE = 0,      /* 自适应 */
    NET_TV_OSD_FONT_SIZE_16 = 1,            /* 16 * 16 */
    NET_TV_OSD_FONT_SIZE_32 = 2,            /* 32 * 32 */
    NET_TV_OSD_FONT_SIZE_48 = 3,            /* 48 * 48 */
    NET_TV_OSD_FONT_SIZE_64 = 4,            /* 64 * 64 */
    NET_TV_OSD_FONT_SIZE_INVALID = 0xFF
} NET_TV_OSD_FONT_SIZE_E;

/**
 * @enum tagNETTVOsdColor
 * @brief OSD颜色 枚举定义 (对应 OSD_COLOR_E)
 * @attention
 */
typedef enum tagNETTVOsdColor
{
    NET_TV_OSD_COLOR_BLACK = 0,             /* 黑色 */
    NET_TV_OSD_COLOR_WHITE = 1,             /* 白色 */
    NET_TV_OSD_COLOR_CUSTOM = 2,            /* 自定义 */
    NET_TV_OSD_COLOR_INVALID = 0xFF
} NET_TV_OSD_COLOR_E;

/**
 * @enum tagNETTVOsdAlign
 * @brief OSD对齐方式 枚举定义 (对应 OSD_ALIGN_E)
 * @attention
 */
typedef enum tagNETTVOsdAlign
{
    NET_TV_OSD_ALIGN_CUSTOMIZE = 0,         /* 自定义 */
    NET_TV_OSD_ALIGN_CHAR_LEFT = 1,         /* 字符左对齐 */
    NET_TV_OSD_ALIGN_CHAR_RIGHT = 2,        /* 字符右对齐 */
    NET_TV_OSD_ALIGN_ALL_LEFT = 3,          /* 全部左对齐 */
    NET_TV_OSD_ALIGN_ALL_RIGHT = 4,         /* 全部右对齐 */
    NET_TV_OSD_ALIGN_GB_MODE = 5,           /* 国标模式 */
    NET_TV_OSD_ALIGN_INVALID = 0xFF
} NET_TV_OSD_ALIGN_E;

/**
 * @enum tagNETTVMotionMode
 * @brief 移动侦测模式  Motion detection mode
 * @attention
 */
typedef enum tagNETTVMotionMode
{
    NET_TV_MOTION_MODE_NORMAL = 0,              /* 普通模式  Normal mode */
    NET_TV_MOTION_MODE_EXPERT = 1               /* 专家模式  Expert mode */
} NET_TV_MOTION_MODE_E;

/**
 * @enum tagNETTVCrossDirection
 * @brief 越界方向  Cross direction
 * @attention
 */
typedef enum tagNETTVCrossDirection
{
    NET_TV_CROSS_BOTH_WAYS = 0,                 /* 双向  Both ways */
    NET_TV_CROSS_A_TO_B = 1,                    /* A到B  A to B */
    NET_TV_CROSS_B_TO_A = 2                     /* B到A  B to A */
} NET_TV_CROSS_DIRECTION_E;

/**
 * @enum tagNETTVDetectionTarget
 * @brief 检测目标类型  Detection target type
 * @attention
 */
typedef enum tagNETTVDetectionTarget
{
    NET_TV_TARGET_ALL = 0,                      /* 所有目标  All targets */
    NET_TV_TARGET_HUMAN = 1,                    /* 人体  Human */
    NET_TV_TARGET_VEHICLE = 2,                  /* 车辆  Vehicle */
    NET_TV_TARGET_HUMAN_AND_VEHICLE = 3         /* 人和车  Human and vehicle */
} NET_TV_DETECTION_TARGET_E;

/**
 * @enum tagNETTVBehaviorType
 * @brief 人员行为类型  Personnel behavior type
 * @attention
 */
typedef enum tagNETTVBehaviorType
{
    NET_TV_BEHAVIOR_SLEEP_ON_DUTY = 0,          /* 睡岗  Sleep on duty */
    NET_TV_BEHAVIOR_LEAVE_POST = 1,             /* 离岗  Leave post */
    NET_TV_BEHAVIOR_SMOKING = 2,                /* 抽烟  Smoking */
    NET_TV_BEHAVIOR_PHONE_USAGE = 3,            /* 玩手机  Phone usage */
    NET_TV_BEHAVIOR_FENCE_CLIMBING = 4,         /* 翻越围栏  Fence climbing */
    NET_TV_BEHAVIOR_FALL_DOWN = 5,              /* 人员倒地  Person fall down */
    NET_TV_BEHAVIOR_TRIP = 6                    /* 摔倒  Trip */
} NET_TV_BEHAVIOR_TYPE_E;

/**
 * @enum tagNETTVSafetyEquipmentType
 * @brief 安全装备类型  Safety equipment type
 * @attention
 */
typedef enum tagNETTVSafetyEquipmentType
{
    NET_TV_EQUIPMENT_HELMET = 0,                /* 安全帽  Helmet */
    NET_TV_EQUIPMENT_REFLECTIVE_CLOTHING = 1,   /* 反光衣  Reflective clothing */
    NET_TV_EQUIPMENT_SEATBELT = 2               /* 高空安全带  High altitude seatbelt */
} NET_TV_SAFETY_EQUIPMENT_TYPE_E;

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
 * @brief 星期一日枚举
 */
typedef enum DayOfWeek
{
    Monday = 1,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday
} DayOfWeek_E;

/**
 * @struct tagNETTVCapturePictureFormat
 * @brief 抓图格式
 * @attention
 */
typedef enum tagNETTVCapturePictureFormat
{
    NET_TV_CAPTURE_PICTURE_FORMAT_JPEG = 0,
    NET_TV_CAPTURE_PICTURE_FORMAT_BMP  = 1
} NET_TV_CAPTURE_PICTURE_FORMAT_E;

/**
 * @struct tagNETTVCaptureImageQuality
 * @brief 抓图质量
 * @attention
 */
typedef enum tagNETTVCaptureImageQuality
{
    NET_TV_CAPTURE_IMAGE_QUALITY_LOW    = 0,
    NET_TV_CAPTURE_IMAGE_QUALITY_MEDIUM = 1,
    NET_TV_CAPTURE_IMAGE_QUALITY_HIGH   = 2
} NET_TV_CAPTURE_IMAGE_QUALITY_E;

/**
 * @struct tagNETTVCaptureTimeUnit
 * @brief 抓图时间单位
 * @attention
 */
typedef enum tagNETTVCaptureTimeUnit
{
    NET_TV_CAPTURE_TIME_UNIT_MILLISECONDS = 0,
    NET_TV_CAPTURE_TIME_UNIT_SECONDS      = 1,
    NET_TV_CAPTURE_TIME_UNIT_MINUTES      = 2,
    NET_TV_CAPTURE_TIME_UNIT_HOURS        = 3,
    NET_TV_CAPTURE_TIME_UNIT_DAYS         = 4
} NET_TV_CAPTURE_TIME_UNIT_E;

/**
 * @enum tagNETTVExposureTime
 * @brief 曝光时间  Exposure time
 */
typedef enum tagNETTVExposureTime
{
    NET_TV_EXPOSURE_TIME_1_3      = 0,
    NET_TV_EXPOSURE_TIME_1_6      = 1,
    NET_TV_EXPOSURE_TIME_1_12     = 2,
    NET_TV_EXPOSURE_TIME_1_25     = 3,
    NET_TV_EXPOSURE_TIME_1_50     = 4,
    NET_TV_EXPOSURE_TIME_1_100    = 5,
    NET_TV_EXPOSURE_TIME_1_150    = 6,
    NET_TV_EXPOSURE_TIME_1_200    = 7,
    NET_TV_EXPOSURE_TIME_1_250    = 8,
    NET_TV_EXPOSURE_TIME_1_500    = 9,
    NET_TV_EXPOSURE_TIME_1_750    = 10,
    NET_TV_EXPOSURE_TIME_1_1000   = 11,
    NET_TV_EXPOSURE_TIME_1_2000   = 12,
    NET_TV_EXPOSURE_TIME_1_4000   = 13,
    NET_TV_EXPOSURE_TIME_1_10000  = 14,
    NET_TV_EXPOSURE_TIME_1_100000 = 15,
    NET_TV_EXPOSURE_TIME_INVALID  = 0xFF
} NET_TV_EXPOSURE_TIME_E;

/**
 * @enum tagNETTVDayNightMode
 * @brief 日夜模式  Day/Night mode
 */
typedef enum tagNETTVDayNightMode
{
    NET_TV_DAYNIGHT_MODE_DAY     = 0,
    NET_TV_DAYNIGHT_MODE_NIGHT   = 1,
    NET_TV_DAYNIGHT_MODE_AUTO    = 2,
    NET_TV_DAYNIGHT_MODE_TIMING  = 3,
    NET_TV_DAYNIGHT_MODE_INVALID = 0xFF
} NET_TV_DAYNIGHT_MODE_E;

/**
 * @enum tagNETTVLightBrightMode
 * @brief 光照亮度模式  Light brightness mode
 */
typedef enum tagNETTVLightBrightMode
{
    NET_TV_LIGHT_BRIGHT_MANUAL  = 0,
    NET_TV_LIGHT_BRIGHT_AUTO    = 1,
    NET_TV_LIGHT_BRIGHT_INVALID = 0xFF
} NET_TV_LIGHT_BRIGHT_MODE_E;

/**
 * @enum tagNETTVLightType
 * @brief 灯光类型  Light type
 */
typedef enum tagNETTVLightType
{
    NET_TV_LIGHT_TYPE_WHITE            = 0,
    NET_TV_LIGHT_TYPE_RED              = 1,
    NET_TV_LIGHT_TYPE_SMART            = 2,
    NET_TV_LIGHT_TYPE_CLOSE            = 3,
    NET_TV_LIGHT_TYPE_BOTH             = 4,
    NET_TV_LIGHT_TYPE_RED_ON_WHITE_OFF = 5,
    NET_TV_LIGHT_TYPE_WHITE_ON_RED_OFF = 6,
    NET_TV_LIGHT_TYPE_INVALID          = 0xFF
} NET_TV_LIGHT_TYPE_E;

/**
 * @enum tagNETTVBackLightArea
 * @brief 背光区域  Backlight area
 */
typedef enum tagNETTVBackLightArea
{
    NET_TV_BACKLIGHT_AREA_CLOSE   = 0,
    NET_TV_BACKLIGHT_AREA_UP      = 1,
    NET_TV_BACKLIGHT_AREA_DOWN    = 2,
    NET_TV_BACKLIGHT_AREA_LEFT    = 3,
    NET_TV_BACKLIGHT_AREA_RIGHT   = 4,
    NET_TV_BACKLIGHT_AREA_CENTER  = 5,
    NET_TV_BACKLIGHT_AREA_INVALID = 0xFF
} NET_TV_BACKLIGHT_AREA_E;

/**
 * @enum tagNETTVDnrMode
 * @brief 数字降噪模式  Digital noise reduction mode
 */
typedef enum tagNETTVDnrMode
{
    NET_TV_DNR_MODE_CLOSE    = 0,
    NET_TV_DNR_MODE_NORMAL   = 1,
    NET_TV_DNR_MODE_ADVANCED = 2,
    NET_TV_DNR_MODE_INVALID  = 0xFF
} NET_TV_DNR_MODE_E;

/**
 * @enum tagNETTVAwbMode
 * @brief 自动白平衡模式  Auto white balance mode
 */
typedef enum tagNETTVAwbMode
{
    NET_TV_AWB_MODE_AUTO         = 0,
    NET_TV_AWB_MODE_MANUAL       = 1,
    NET_TV_AWB_MODE_LOCK         = 2,
    NET_TV_AWB_MODE_INCANDESCENT = 3,
    NET_TV_AWB_MODE_WARM         = 4,
    NET_TV_AWB_MODE_FLUORESCENT  = 5,
    NET_TV_AWB_MODE_DAYLIGHT     = 6,
    NET_TV_AWB_MODE_SEMI_AUTO    = 7,
    NET_TV_AWB_MODE_REG_MANUAL   = 8,
    NET_TV_AWB_MODE_INVAL        = 9,
    NET_TV_AWB_MODE_INVALID      = 0xFF
} NET_TV_AWB_MODE_E;

/**
 *  @enum tagNETTVWifiSecurityMode
 *  @brief WIFI 安全模式
 */
typedef enum tagNETTVWifiSecurityMode
{
    NET_TV_WIFI_SECURITY_WPA_PERSONAL = 0,
    NET_TV_WIFI_SECURITY_OPEN         = 1,
    NET_TV_WIFI_SECURITY_WEP          = 2,
    NET_TV_WIFI_SECURITY_EAP_PEAP     = 3,
    NET_TV_WIFI_SECURITY_EAP_TLS      = 4,
    NET_TV_WIFI_SECURITY_INVALID      = 0x7FFFFFFF
} NET_TV_WIFI_SECURITY_MODE_E;

/**
 * @enum tagNETTVPeopleFlowStatType
 * @brief 人流统计类型
 */
typedef enum tagNETTVPeopleFlowStatType
{
    NET_TV_PEOPLE_FLOW_STAT_TOTAL = 0,                /* 总人数 */
    NET_TV_PEOPLE_FLOW_STAT_ENTER = 1,                /* 进入人数 */
    NET_TV_PEOPLE_FLOW_STAT_LEAVE = 2                 /* 离开人数 */
} NET_TV_PEOPLE_FLOW_STAT_TYPE_E;

typedef enum tagNETTVStatisticsType
{
    NET_TV_STATISTICS_TYPE_PEOPLE_FLOW = 1,           /* 人流统计 */
    NET_TV_STATISTICS_TYPE_PEOPLE_DENSITY = 2         /* 人员密度统计 */
} NET_TV_STATISTICS_TYPE_E;

/**
 * @enum tagNETTVRecordStatus
 * @brief 录制状态
 */
typedef enum tagNETTVRecordStatus
{
    NET_TV_RECORD_STATUS_NO_OPERATION = 0,            /* 无操作 */
    NET_TV_RECORD_STATUS_RECORDING    = 1,            /* 正在录制 */
    NET_TV_RECORD_STATUS_PAUSE        = 2,            /* 暂停录制 */
    NET_TV_RECORD_STATUS_STOP         = 3,            /* 停止录制 */
    NET_TV_RECORD_STATUS_PRERECORD    = 4             /* 预览录制 */
} NET_TV_RECORD_STATUS_E;

/**
 * @enum tagNETTVPopulationAlarmSeverity
 * @brief 人数报警等级
 */
typedef enum tagNETTVPopulationAlarmSeverity
{
    NET_TV_POPULATION_ALARM_NORMAL = 0,               /* 普通报警 */
    NET_TV_POPULATION_ALARM_MEDIUM = 1,               /* 中度报警 */
    NET_TV_POPULATION_ALARM_SEVERE = 2              /* 严重报警 */
} NET_TV_POPULATION_ALARM_SEVERITY_E;

/* END************* 枚举值  Enumeration value *************************** */


/* BEGIN*********** 结构体  Structure *********************************** */

/**
 * @struct tagNETTVTalkbackStateInfo
 * @brief 对讲状态信息结构体
 * @note 对应命令：NET_TV_STATE_TALKBACK
 */
typedef struct tagNETTVTalkbackStateInfo
{
    BOOL    bEnable;                     /* 对讲功能使能标记 */
    CHAR    szSdp[NET_TV_MAX_URL_LEN];  /* SDP 协议描述地址 */
    CHAR    szUrl[NET_TV_MAX_URL_LEN];  /* 对讲服务 URL 地址 */
    CHAR    szLocalIP[NET_TV_LEN_64];    /* 本机 IP 地址 */
    BYTE    byRes[128];                 /* 保留字段，未使用 */
} NET_TV_TALKBACK_STATE_INFO_S, *LPNET_TV_TALKBACK_STATE_INFO_S;

/**
 * @struct tagNETTVTalkbackStreamInfo
 * @brief 对讲流信息结构体
 * @note 对应命令：NET_TV_TO_STREAM_TALKBACK / NET_TV_FROM_STREAM_TALKBACK
 */
typedef struct tagNETTVTalkbackStreamInfo
{
    CHAR    szHost[NET_TV_LEN_64];      /* 对讲服务器主机地址/IP */
    INT32   nPort;                      /* 对讲服务端口号 */
    INT32   nChnId;                     /* 通道 ID，对应对讲通道 */
    INT32   nUserID;                    /* 用户 ID，标识操作会话 */
    BOOL    bMainStream;                /* 主码流标记 (true/false) */
    CHAR    szProtocol[NET_TV_LEN_32]; /* 传输协议类型 (如 rtp/raw 等) */
    CHAR    szStartTime[NET_TV_LEN_64];/* 对讲开始时间戳 */
    CHAR    szEndTime[NET_TV_LEN_64];    /* 对讲结束时间戳 */
    CHAR    szFileName[NET_TV_LEN_260]; /* 对讲音频文件名/路径 */
    BYTE    byRes[128];                 /* 保留字段，未使用 */
} NET_TV_TALKBACK_STREAM_INFO_S, *LPNET_TV_TALKBACK_STREAM_INFO_S;

/**
 * @struct tagNETTVReplayTalkbackInfo
 * @brief 对讲回放信息结构体
 * @note 对应命令：NET_TV_REPLAY_TALKBACK
 */
typedef struct tagNETTVReplayTalkbackInfo
{
    CHAR    szNvrIp[NET_TV_LEN_64];     /* NVR 网络摄像机 IP 地址 */
    CHAR    szRemoteIp[NET_TV_LEN_64];  /* 远端设备 IP 地址 */
    NET_TV_TALKBACK_STREAM_INFO_S stIPCInfo; /* IPC 端对讲流详细信息 */
    BYTE    byRes[128];                 /* 保留字段，未使用 */
} NET_TV_REPLAY_TALKBACK_INFO_S, *LPNET_TV_REPLAY_TALKBACK_INFO_S;

/**
 * @struct tagNETTVExposureInfo
 * @brief 曝光信息结构体
 * @attention
 */
typedef struct tagNETTVExposureInfo
{
    INT32               enExpTime;          /* ISP::ExpTimeMode_E */
    BOOL                bAntiBanding;       /* TRUE/FALSE */
    BYTE                byRes[64];
} NET_TV_EXPOSURE_INFO_S, *LPNET_TV_EXPOSURE_INFO_S;

/**
 * @struct tagNETTVDayNightInfo
 * @brief 日夜切换信息结构体
 * @attention
 */
typedef struct tagNETTVDayNightInfo
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
} NET_TV_DAYNIGHT_INFO_S, *LPNET_TV_DAYNIGHT_INFO_S;

/**
 * @struct tagNETTVBackLightInfo
 * @brief 背光信息结构体
 * @attention
 */
typedef struct tagNETTVBackLightInfo
{
    INT32               enBackLightArea;    /* ISP::BackLightArea_E */
    BOOL                bWdrEnable;
    INT32               nWdrLevel;
    BOOL                bHlsEnable;
    INT32               nHlsLevel;
    BYTE                byRes[64];
} NET_TV_BACKLIGHT_INFO_S, *LPNET_TV_BACKLIGHT_INFO_S;

/**
 * @struct tagNETTVDenoiseInfo
 * @brief 降噪信息结构体
 * @attention
 */
typedef struct tagNETTVDenoiseInfo
{
    INT32               enDnrMode;          /* ISP::DnrMode_E */
    UINT32              nDnrLevel;
    UINT32              nSnrLevel;
    UINT32              nTnrLevel;
    BYTE                byRes[64];
} NET_TV_DENOISE_INFO_S, *LPNET_TV_DENOISE_INFO_S;

/**
 * @struct tagNETTVWhiteBalanceInfo
 * @brief 白平衡信息结构体
 * @attention
 */
typedef struct tagNETTVWhiteBalanceInfo
{
    INT32               enAwbMode;          /* ISP::AwbMode_E */
    UINT32              nRGain;
    UINT32              nBGain;
    BYTE                byRes[64];
} NET_TV_WHITEBALANCE_INFO_S, *LPNET_TV_WHITEBALANCE_INFO_S;

/**
 * @struct tagNETTVUpgradeInfo
 * @brief 升级文件信息
 * @attention
 */
typedef struct tagNETTVUpgradeInfo
{
    CHAR                szUpgradePath[NET_TV_FILE_NAME_LEN];
    BYTE                byRes[64];
} NET_TV_UPGRADE_INFO_S, *LPNET_TV_UPGRADE_INFO_S;

/**
 * @struct tagNETTVUpgradeStatus
 * @brief 系统升级状态
 * @attention
 */
typedef struct tagNETTVUpgradeStatus
{
    INT32               nUpgradeStatus;
    BYTE                byRes[64];
} NET_TV_UPGRADE_STATUS_S, *LPNET_TV_UPGRADE_STATUS_S;

/**
 * @struct tagNETTVUpgradeVersion
 * @brief 升级文件版本
 * @attention
 */
typedef struct tagNETTVUpgradeVersion
{
    CHAR                szVersion[NET_TV_LEN_64];
    BYTE                byRes[64];
} NET_TV_UPGRADE_VERSION_S, *LPNET_TV_UPGRADE_VERSION_S;

/**
 * @struct tagNETTVCaptureTime
 * @brief 抓图时间
 * @attention
 */
typedef struct tagNETTVCaptureTime
{
    INT32               nStartTime;
    INT32               nEndTime;
    BYTE                byRes[32];
} NET_TV_CAPTURE_TIME_S, *LPNET_TV_CAPTURE_TIME_S;

/**
 * @struct tagNETTVCaptureDaySchedule
 * @brief 抓图日程
 * @attention
 */
typedef struct tagNETTVCaptureDaySchedule
{
    INT32               nDayOfWeek; /* 1~7: Monday~Sunday */
    UINT32              udwTimeCount;
    NET_TV_CAPTURE_TIME_S astTimes[NET_TV_PLAN_TIME_SECTION_NUM_ADAY];
    BYTE                byRes[64];
} NET_TV_CAPTURE_DAY_SCHEDULE_S, *LPNET_TV_CAPTURE_DAY_SCHEDULE_S;

/**
 * @struct tagNETTVCapturePlanInfo
 * @brief 抓图计划信息
 * @attention
 */
typedef struct tagNETTVCapturePlanInfo
{
    NET_TV_CAPTURE_DAY_SCHEDULE_S astDaySchedules[NET_TV_PLAN_DAY_NUM_AWEEK];
    BYTE                byRes[256];
} NET_TV_CAPTURE_PLAN_INFO_S, *LPNET_TV_CAPTURE_PLAN_INFO_S;

/**
 * @struct tagNETTVCaptureConfig
 * @brief 抓图配置
 * @attention
 */
typedef struct tagNETTVCaptureConfig
{
    BOOL                bEnable;
    INT32               enPictureFormat; /* NET_TV_CAPTURE_PICTURE_FORMAT_E */
    INT32               nWidth;
    INT32               nHeight;
    INT32               enImageQuality;  /* NET_TV_CAPTURE_IMAGE_QUALITY_E */
    UINT32              unInterval;
    INT32               enTimeUnit;      /* NET_TV_CAPTURE_TIME_UNIT_E */
    UINT32              unNumber;
    BYTE                byRes[64];
} NET_TV_CAPTURE_CONFIG_S, *LPNET_TV_CAPTURE_CONFIG_S;

/**
 * @struct tagNETTVCaptureParamInfo
 * @brief 抓图参数信息
 * @attention
 */
typedef struct tagNETTVCaptureParamInfo
{
    NET_TV_CAPTURE_CONFIG_S stCaptureTimingConfig;
    NET_TV_CAPTURE_CONFIG_S stCaptureEventConfig;
    BYTE                byRes[128];
} NET_TV_CAPTURE_PARAM_INFO_S, *LPNET_TV_CAPTURE_PARAM_INFO_S;

typedef struct tagNETTVCaptureInfo
{
    INT32               nChnId;
    INT32               enType;
    CHAR                szStartTime[NET_TV_MAX_DATE_STRING_LEN];
    CHAR                szEndTime[NET_TV_MAX_DATE_STRING_LEN];
    CHAR                szImagePath[NET_TV_FILE_NAME_LEN];
    INT32               nImageSize;
    BYTE                byRes[128];
} NET_TV_CAPTURE_INFO_S, *LPNET_TV_CAPTURE_INFO_S;

/**
 * @struct tagstNETTVLoginInfo
 * @brief 设备登录信息
 * @attention
 */
typedef struct tagstNETTVDeviceLoginInfo
{    
    CHAR    szIPAddr[NET_TV_LEN_260];       /* IP地址/域名 */
    INT32   dwPort;                         /* 端口号 */
    CHAR    szUserName[NET_TV_LEN_132];     /* 用户名 */
    CHAR    szPassword[NET_TV_LEN_128];     /* 密码 */
    BYTE    byRes[256];                     /* 保留字段 */
}NET_TV_DEVICE_LOGIN_INFO_S, *LPNET_TV_DEVICE_LOGIN_INFO_S;

/**
 * @struct tagNET_TV_DeviceInfo
 * @brief 设备信息 结构体定义 Device information Structure definition
 * @attention 无 None
 */
typedef struct tagNETTVDeviceInfo
{
    INT32   dwDevType;                          /* 设备类型,参见枚举#NET_TV_DEVICE_TYPE_E  Device type, see enumeration # NET_TV_DEVICE_TYPE_E */
    INT16   wAlarmInPortNum;                    /* 报警输入个数  Number of alarm inputs */
    INT16   wAlarmOutPortNum;                   /* 报警输出个数  Number of alarm outputs */
    INT32   dwChannelNum;                       /* 通道个数  Number of Channels */
    BYTE    byRes[48];                          /* 保留字段  Reserved */
}NET_TV_DEVICE_INFO_S,*LPNET_TV_DEVICE_INFO_S;

/**
 * @struct tagNET_TVDeviceBasicInfo
 * @brief 设备基本信息 结构体定义 Basic device information Structure definition
 * @attention 无 None
 */
typedef struct tagNETTVDeviceBasicInfo
{ 
    CHAR szDevModel[NET_TV_LEN_64];                     /* 设备型号  Device model */
    CHAR szSerialNum[NET_TV_LEN_64];                    /* 硬件序列号  Hardware serial number */
    CHAR szFirmwareVersion[NET_TV_LEN_64];              /* 软件版本号  Software version */
    CHAR szMacAddress[NET_TV_LEN_64];                   /* IPv4的Mac地址  MAC address of IPv4 */
    CHAR szDeviceName[NET_TV_LEN_64];                   /* 设备名称  Device name */
    CHAR szManufacturer[NET_TV_LEN_64];                 /* 厂商信息  Manufacturer */
    CHAR szDeviceTypeV2[NET_TV_LEN_128];                /* 设备类型 */
    BYTE byRes[256];                                    /* 保留字段  Reserved */
}NET_TV_DEVICE_BASICINFO_S, *LPNET_TV_DEVICE_BASICINFO_S;

/**
 * @struct tagNETTVPageInfo
 * @brief 分页信息
 */
typedef struct tagNETTVPageInfo
{
    INT32   nCurPage;
    INT32   nPageSize;
    INT32   nDataTotal;
    INT32   nPageTotal;
    BYTE    byRes[64];
} NET_TV_PAGE_INFO_S, *LPNET_TV_PAGE_INFO_S;

/**
 * @struct tagNETTVLoginLockInfo
 * @brief 登录锁定信息
 */
typedef struct tagNETTVLoginLockInfo
{
    BOOL    bIllegalLoginEnable;
    INT32   nCheckInterval;
    INT32   nMaxErrorTimes;
    INT32   nLockDuration;
    BYTE    byRes[64];
} NET_TV_LOGIN_LOCK_INFO_S, *LPNET_TV_LOGIN_LOCK_INFO_S;

/**
 * @struct tagNETTVPwdPolicyInfo
 * @brief 密码策略信息
 */
typedef struct tagNETTVPwdPolicyInfo
{
    BOOL    bPwdSecurityLevelEnable;
    BOOL    bAllowLowLevelPwdLogin;
    BYTE    byRes[64];
} NET_TV_PWD_POLICY_INFO_S, *LPNET_TV_PWD_POLICY_INFO_S;

/**
 * @struct tagNETTVSshAdminInfo
 * @brief SSH管理信息
 */
typedef struct tagNETTVSshAdminInfo
{
    BOOL    bSshEnable;
    INT32   nSshPort;
    CHAR    szSshStartTime[NET_TV_LEN_64];
    CHAR    szSshCountdown[NET_TV_LEN_64];
    BYTE    byRes[64];
} NET_TV_SSH_ADMIN_INFO_S, *LPNET_TV_SSH_ADMIN_INFO_S;

/**
 * @struct tagNETTVSecurityServicesInfo
 * @brief 安全服务配置
 */
typedef struct tagNETTVSecurityServicesInfo
{
    NET_TV_LOGIN_LOCK_INFO_S stLoginLock;
    NET_TV_PWD_POLICY_INFO_S stPwdPolicy;
    NET_TV_SSH_ADMIN_INFO_S stSshAdmin;
    BYTE    byRes[128];
} NET_TV_SECURITY_SERVICES_INFO_S, *LPNET_TV_SECURITY_SERVICES_INFO_S;

/**
 * @struct tagNETTVSshCountdownInfo
 * @brief SSH倒计时信息
 */
typedef struct tagNETTVSshCountdownInfo
{
    CHAR    szCountdown[NET_TV_LEN_64];
    BYTE    byRes[64];
} NET_TV_SSH_COUNTDOWN_INFO_S, *LPNET_TV_SSH_COUNTDOWN_INFO_S;

/**
 * @struct tagNETTVLogServerInfo
 * @brief 日志服务器配置
 */
typedef struct tagNETTVLogServerInfo
{
    BOOL    bEnable;
    BOOL    bEnSsl;
    CHAR    szServerAddr[NET_TV_LEN_256];
    INT32   nPort;
    BYTE    byRes[128];
} NET_TV_LOG_SERVER_INFO_S, *LPNET_TV_LOG_SERVER_INFO_S;

/**
 * @struct tagNETTVLogRetrievalCond
 * @brief 日志检索条件
 */
typedef struct tagNETTVLogRetrievalCond
{
    INT32   nType;
    INT32   nAction;
    CHAR    szStartTime[NET_TV_LEN_64];
    CHAR    szEndTime[NET_TV_LEN_64];
    BYTE    byRes[64];
} NET_TV_LOG_RETRIEVAL_COND_S, *LPNET_TV_LOG_RETRIEVAL_COND_S;

/**
 * @struct tagNETTVLogInfo
 * @brief 日志信息
 */
typedef struct tagNETTVLogInfo
{
    CHAR    szStartTime[NET_TV_LEN_64];
    INT32   nType;
    INT32   nAction;
    CHAR    szChnName[NET_TV_LEN_64];
    CHAR    szUser[NET_TV_LEN_64];
    CHAR    szHost[NET_TV_LEN_64];
    CHAR    szContext[NET_TV_LEN_512];
    BYTE    byRes[64];
} NET_TV_LOG_INFO_S, *LPNET_TV_LOG_INFO_S;

/**
 * @struct tagNETTVLogList
 * @brief 日志查询结果列表
 */
typedef struct tagNETTVLogList
{
    NET_TV_LOG_RETRIEVAL_COND_S stCond;
    NET_TV_PAGE_INFO_S stPage;
    INT32   nLogCount;
    NET_TV_LOG_INFO_S astLogs[NET_TV_LOG_QUERY_COND_NUM];
    BYTE    byRes[128];
} NET_TV_LOG_LIST_S, *LPNET_TV_LOG_LIST_S;

/**
 * @struct tagNETTVRecordInfo
 * @brief 手动录像控制信息
 */
typedef struct tagNETTVRecordInfo
{
    INT32   nChnId;
    INT32   nVideoStatus;
    INT32   nAudioStatus;
    INT32   nRecordStatus;                      /* NET_TV_RECORD_STATUS_E */
    INT32   nRecordFormat;
    INT32   nEventType;
    CHAR    szPath[NET_TV_MAX_URL_LEN];
    CHAR    szRedunPath[NET_TV_MAX_URL_LEN];
    CHAR    szRecordName[NET_TV_FILE_NAME_LEN];
    CHAR    szRecordTime[NET_TV_LEN_64];
    INT32   nStreamType;
    BYTE    byRes[128];
} NET_TV_RECORD_INFO_S, *LPNET_TV_RECORD_INFO_S;

/**
 * @struct tagNETTVRecordStatusInfo
 * @brief 录像状态信息
 */
typedef struct tagNETTVRecordStatusInfo
{
    INT32   nStatus;                            /* NET_TV_RECORD_STATUS_E */
    BYTE    byRes[64];
} NET_TV_RECORD_STATUS_INFO_S, *LPNET_TV_RECORD_STATUS_INFO_S;

/**
 * @struct tagNETTVRecordTime
 * @brief 录像计划时间段
 */
typedef struct tagNETTVRecordTime
{
    INT32   nType;                              /* 1:定时录像, 2:事件录像 */
    INT32   nStartTime;                         /* 秒 */
    INT32   nEndTime;                           /* 秒 */
    BYTE    byRes[32];
} NET_TV_RECORD_TIME_S, *LPNET_TV_RECORD_TIME_S;

/**
 * @struct tagNETTVRecordDaySchedule
 * @brief 单日录像计划
 */
typedef struct tagNETTVRecordDaySchedule
{
    INT32   nDayOfWeek;                         /* 1:周一 ... 7:周日 */
    INT32   nRecordTimeCount;
    NET_TV_RECORD_TIME_S astRecordTimes[NET_TV_TIME_DURATION_NUM];
    BYTE    byRes[64];
} NET_TV_RECORD_DAY_SCHEDULE_S, *LPNET_TV_RECORD_DAY_SCHEDULE_S;

/**
 * @struct tagNETTVRecordSchedule
 * @brief 录像计划
 */
typedef struct tagNETTVRecordSchedule
{
    BOOL    bEnable;
    INT32   nDayScheduleCount;
    NET_TV_RECORD_DAY_SCHEDULE_S astDaySchedules[NET_TV_PLAN_DAY_NUM_AWEEK];
    BYTE    byRes[128];
} NET_TV_RECORD_SCHEDULE_S, *LPNET_TV_RECORD_SCHEDULE_S;

/**
 * @struct tagNETTVRecordAdvancedParam
 * @brief 录像高级参数
 */
typedef struct tagNETTVRecordAdvancedParam
{
    BOOL    bLoopWrite;
    INT32   nPreTime;
    INT32   nDelayTime;
    INT32   nStreamType;
    BYTE    byRes[128];
} NET_TV_RECORD_ADVANCED_PARAM_S, *LPNET_TV_RECORD_ADVANCED_PARAM_S;

/**
 * @struct tagNETTVRecordFindCond
 * @brief 录像文件查找条件
 */
typedef struct tagNETTVRecordFindCond
{
    INT32   nChnId;
    INT32   nType;
    CHAR    szYear[NET_TV_LEN_16];
    CHAR    szMonth[NET_TV_LEN_16];
    CHAR    szDate[NET_TV_LEN_64];
    CHAR    szStartTime[NET_TV_LEN_64];
    CHAR    szEndTime[NET_TV_LEN_64];
    CHAR    szFilename[NET_TV_FILE_NAME_LEN];
    BYTE    byRes[128];
} NET_TV_RECORD_FIND_COND_S, *LPNET_TV_RECORD_FIND_COND_S;

/**
 * @struct tagNETTVRecordVideoTime
 * @brief 指定日期录像时间段
 */
typedef struct tagNETTVRecordVideoTime
{
    INT32   nStartTime;
    INT32   nEndTime;
    BYTE    byRes[32];
} NET_TV_RECORD_VIDEO_TIME_S, *LPNET_TV_RECORD_VIDEO_TIME_S;

/**
 * @struct tagNETTVRecordFindResult
 * @brief 录像查找结果项
 */
typedef struct tagNETTVRecordFindResult
{
    INT32   nChnId;
    INT32   nDateCount;
    CHAR    aszDates[NET_TV_RECORD_DATE_MAX_NUM][NET_TV_LEN_64];
    CHAR    szFilename[NET_TV_FILE_NAME_LEN];
    INT32   nVideoTimeCount;
    NET_TV_RECORD_VIDEO_TIME_S astVideoTimes[NET_TV_TIME_DURATION_NUM];
    BYTE    byRes[128];
} NET_TV_RECORD_FIND_RESULT_S, *LPNET_TV_RECORD_FIND_RESULT_S;

/**
 * @struct tagNETTVRecordFileList
 * @brief 录像查找条件与结果
 */
typedef struct tagNETTVRecordFileList
{
    NET_TV_RECORD_FIND_COND_S stFind;
    INT32   nResultCount;
    NET_TV_RECORD_FIND_RESULT_S astResults[NET_TV_RECORD_FILE_MAX_NUM];
    BYTE    byRes[128];
} NET_TV_RECORD_FILE_LIST_S, *LPNET_TV_RECORD_FILE_LIST_S;

/**
 * @struct tagNETTVRecordDownloadInfo
 * @brief 录像下载任务
 */
typedef struct tagNETTVRecordDownloadInfo
{
    INT32   nChnId;
    CHAR    szPath[NET_TV_MAX_URL_LEN];
    CHAR    szStartTime[NET_TV_LEN_64];
    CHAR    szEndTime[NET_TV_LEN_64];
    BYTE    byRes[128];
} NET_TV_RECORD_DOWNLOAD_INFO_S, *LPNET_TV_RECORD_DOWNLOAD_INFO_S;

/**
 * @struct tagNETTVRecordDownloadProgress
 * @brief 录像下载进度
 */
typedef struct tagNETTVRecordDownloadProgress
{
    CHAR    szFilename[NET_TV_FILE_NAME_LEN];
    INT32   nProgress;
    BYTE    byRes[64];
} NET_TV_RECORD_DOWNLOAD_PROGRESS_S, *LPNET_TV_RECORD_DOWNLOAD_PROGRESS_S;

/**
 * @struct tagNETTVRecordDownloadList
 * @brief 录像下载任务列表
 */
typedef struct tagNETTVRecordDownloadList
{
    INT32   nDownloadCount;
    NET_TV_RECORD_DOWNLOAD_INFO_S astDownloads[NET_TV_RECORD_DOWNLOAD_MAX_NUM];
    INT32   nProgressCount;
    NET_TV_RECORD_DOWNLOAD_PROGRESS_S astProgress[NET_TV_RECORD_DOWNLOAD_MAX_NUM];
    BYTE    byRes[128];
} NET_TV_RECORD_DOWNLOAD_LIST_S, *LPNET_TV_RECORD_DOWNLOAD_LIST_S;

/**
 * @struct tagNETTVAudioCfg
 * @brief 音频配置参数
 * @note 对应 Audio_NS::AudioConfig_S
 */
typedef struct tagNETTVAudioCfg
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
} NET_TV_AUDIO_CFG_S, *LPNET_TV_AUDIO_CFG_S;

/**
 * @struct tagNETTVNetworkInterfaces
 * @brief 网络配置信息 结构体定义 Network configuration information
 * @attention 无 None
 */
typedef struct tagNETTVNetworkInterfaces
{
    INT32   dwMTU;                              /* MTU值  MTU value */
    BOOL    bIPv4DHCP;                          /* IPv4的DHCP  DHCP of IPv4 */
    CHAR    szIpv4Address[NET_TV_LEN_32];       /* IPv4的IP地址  IP address of IPv4 */
    CHAR    szIPv4GateWay[NET_TV_LEN_32];       /* IPv4的网关地址  Gateway of IPv4 */
    CHAR    szIPv4SubnetMask[NET_TV_LEN_32];    /* IPv4的子网掩码  Subnet mask of IPv4 */
    BYTE    byRes[480];                         /* 保留字段  Reserved */
}NET_TV_NETWORKCFG_S, *LPNET_TV_NETWORKCFG_S;

/**
 * @struct tagNETTVRtspUrlInfo
 * @brief RTSP流地址信息  RTSP URL information
 * @note
 * - szRtspUrl 由设备端生成并返回（可包含鉴权信息或 token，按项目约定）
 * - dwStreamIndex 参考 #NET_TV_LIVE_STREAM_INDEX_E（主/辅/第三流等）
 */
typedef struct tagNETTVRtspUrlInfo
{
    INT32   dwChannel;                           /* 通道号  Channel ID */
    INT32   dwStreamIndex;                       /* 码流索引  Stream index */
    CHAR    szRtspUrl[NET_TV_LEN_260];           /* RTSP URL (e.g. rtsp://ip:port/...) */
    BYTE    byRes[256];                          /* 保留字段  Reserved */
}NET_TV_RTSP_URL_INFO_S, *LPNET_TV_RTSP_URL_INFO_S;

/**
 * @struct tagNETTVReplayUrlInfo
 * @brief 回放播放地址信息  Playback URL information
 * @note
 * - 调用方填充通道和起止时间。
 * - 设备端/服务端申请回放后填充 szUrl。
 */
typedef struct tagNETTVReplayUrlInfo
{
    INT32   dwChannel;                           /* 通道号 Channel ID */
    CHAR    szStartTime[NET_TV_LEN_64];          /* 开始时间 "YYYY-MM-DD HH:MM:SS" */
    CHAR    szEndTime[NET_TV_LEN_64];            /* 结束时间 "YYYY-MM-DD HH:MM:SS" */
    CHAR    szUrl[NET_TV_MAX_URL_LEN];           /* 回放播放地址 */
    BYTE    byRes[256];                          /* 保留字段 Reserved */
}NET_TV_REPLAY_URL_INFO_S, *LPNET_TV_REPLAY_URL_INFO_S;

/**
 * @struct tagNETTVReplayCtrlInfo
 * @brief 回放控制信息
 * @note
 * - `dwCtrlType=NET_TV_REPLAY_CTRL_START` 时，调用方填写通道/起止时间，服务端返回 `szSessionId` 与 `szUrl`
 * - `dwCtrlType=NET_TV_REPLAY_CTRL_STOP` 或 `NET_TV_REPLAY_CTRL_SET_SPEED` 时，优先使用 `szSessionId` 标识会话
 * - `fSpeed` 仅在 `NET_TV_REPLAY_CTRL_SET_SPEED` 时有效，例如 0.5 / 1.0 / 2.0 / 4.0
 */
typedef struct tagNETTVReplayCtrlInfo
{
    INT32   dwChannel;                                   /* 通道号 Channel ID */
    INT32   dwCtrlType;                                  /* 控制类型，参见 NET_TV_REPLAY_CTRL_CMD_E */
    FLOAT   fSpeed;                                      /* 播放倍速 */
    CHAR    szSessionId[NET_TV_REPLAY_SESSION_ID_LEN];   /* 回放会话ID */
    CHAR    szStartTime[NET_TV_LEN_64];                  /* 开始时间 "YYYY-MM-DD HH:MM:SS" */
    CHAR    szEndTime[NET_TV_LEN_64];                    /* 结束时间 "YYYY-MM-DD HH:MM:SS" */
    CHAR    szUrl[NET_TV_MAX_URL_LEN];                   /* 当前回放播放地址 */
    BYTE    byRes[128];                                  /* 保留字段 Reserved */
} NET_TV_REPLAY_CTRL_INFO_S, *LPNET_TV_REPLAY_CTRL_INFO_S;

/**
 * @struct tagNETTVReplayRecordTime
 * @brief 单个录像时间段
 */
typedef struct tagNETTVReplayRecordTime
{
    INT32   nStartTime;                          /* 开始秒数，按当天 00:00:00 起算 */
    INT32   nEndTime;                            /* 结束秒数，按当天 00:00:00 起算 */
    BYTE    byRes[16];                           /* 保留字段 */
} NET_TV_REPLAY_RECORD_TIME_S, *LPNET_TV_REPLAY_RECORD_TIME_S;

/**
 * @struct tagNETTVReplayRecordList
 * @brief NVR回放录像时间段查询结果
 * @note
 * - 调用方至少填写 `dwChannel`
 * - 兼容旧方式：仅填写 `szDate`
 * - 新方式：可额外填写 `bFilterByEventType`、`dwEventType`、`szStartTime`、`szEndTime`
 * - 返回普通录像、人员事件、车辆事件、其他事件四类时间段
 */
typedef struct tagNETTVReplayRecordList
{
    INT32   dwChannel;                                                   /* 通道号 Channel ID */
    BOOL    bFilterByEventType;                                          /* 是否按事件类型过滤 FALSE:不过滤 TRUE:按 dwEventType 过滤 */
    INT32   dwEventType;                                                 /* 事件类型，取值可对齐 NVR 事件类型定义；兼容旧请求时忽略 */
    CHAR    szDate[NET_TV_LEN_32];                                       /* 日期 "YYYY-MM-DD" */
    CHAR    szStartTime[NET_TV_MAX_DATE_STRING_LEN];                     /* 开始时间 "YYYY-MM-DD HH:MM:SS"，为空则按整天开始 */
    CHAR    szEndTime[NET_TV_MAX_DATE_STRING_LEN];                       /* 结束时间 "YYYY-MM-DD HH:MM:SS"，为空则按整天结束 */
    INT32   nVideoCount;                                                 /* 普通录像时间段数量 */
    NET_TV_REPLAY_RECORD_TIME_S astVideoTimes[NET_TV_REPLAY_RECORD_SEGMENT_MAX];
    INT32   nPersonEventCount;                                           /* 人员事件时间段数量 */
    NET_TV_REPLAY_RECORD_TIME_S astPersonEventTimes[NET_TV_REPLAY_RECORD_SEGMENT_MAX];
    INT32   nVehicleEventCount;                                          /* 车辆事件时间段数量 */
    NET_TV_REPLAY_RECORD_TIME_S astVehicleEventTimes[NET_TV_REPLAY_RECORD_SEGMENT_MAX];
    INT32   nOtherEventCount;                                            /* 其他事件时间段数量 */
    NET_TV_REPLAY_RECORD_TIME_S astOtherEventTimes[NET_TV_REPLAY_RECORD_SEGMENT_MAX];
    BYTE    byRes[64];                                                   /* 保留字段 */
} NET_TV_REPLAY_RECORD_LIST_S, *LPNET_TV_REPLAY_RECORD_LIST_S;

/**
 * @struct tagNETTVPreviewRtspUrlInfo
 * @brief 预览RTSP地址信息 Preview RTSP URL information
 */
typedef struct tagNETTVPreviewRtspUrlInfo
{
    CHAR    szRtspMainUrl[NET_TV_MAX_URL_LEN];   /* 主码流RTSP地址 */
    CHAR    szRtspSubUrl[NET_TV_MAX_URL_LEN];    /* 子码流RTSP地址 */
    BYTE    byRes[64];                           /* 保留字段 */
}NET_TV_PREVIEW_RTSP_URL_S, *LPNET_TV_PREVIEW_RTSP_URL_S;

/**
 *  @struct tagNETTVWifiStaCfg
 *  @brief WIFI STA基础配置
 */
typedef struct tagNETTVWifiStaCfg
{
    BOOL    bEnableWifi;                        /* 是否开启WiFi */
    BOOL    bEnableBoost;                       /* 是否开启增强功能 */
    BYTE    byRes[64];                          /* 保留字段 */
} NET_TV_WIFI_STA_CFG_S, *LPNET_TV_WIFI_STA_CFG_S;

/**
 *  @struct tagNETTVWifiWepKey
 *  @brief WIFI WEP 密码项
 */
typedef struct tagNETTVWifiWepKey
{
    INT32   nIndex;                             /* 密钥索引 1-4 */
    CHAR    szValue[NET_TV_LEN_132];            /* 密钥内容 */
    BYTE    byRes[32];                          /* 保留字段 */
} NET_TV_WIFI_WEP_KEY_S, *LPNET_TV_WIFI_WEP_KEY_S;


/**
 * @struct tagNETTVWifiStaConnect
 * @brief WIFI STA连接参数
 */
typedef struct tagNETTVWifiStaConnect
{
    CHAR    szSsid[NET_TV_NAME_MAX_LEN];        /* SSID */
    INT32   nSecurityMode;                      /* NET_TV_WIFI_SECURITY_MODE_E */
    CHAR    szIpAddress[NET_TV_IPADDR_STR_MAX_LEN];
    CHAR    szPassword[NET_TV_LEN_132];
    CHAR    szPairwise[NET_TV_LEN_32];          /* TKIP/CCMP */
    INT32   nWepKeyLen;                         /* 64/128 */
    BOOL    bWepIsHex;                          /* true=hex,false=ascii */
    CHAR    szAuthAlg[NET_TV_LEN_32];           /* OPEN/SHARED */
    INT32   nWepKeyCount;
    NET_TV_WIFI_WEP_KEY_S astWepKeys[4];

    CHAR    szEapIdentity[NET_TV_LEN_132];
    CHAR    szEapPassword[NET_TV_LEN_132];
    CHAR    szPeapVersion[NET_TV_LEN_16];
    CHAR    szPhase2[NET_TV_LEN_64];
    CHAR    szAnonymousIdentity[NET_TV_LEN_132];
    CHAR    szCaCertPath[NET_TV_LEN_260];
    CHAR    szPeapLabel[NET_TV_LEN_32];

    CHAR    szTlsIdentity[NET_TV_LEN_132];
    CHAR    szPrivateKeyPasswd[NET_TV_LEN_132];
    CHAR    szEapolVersion[NET_TV_LEN_16];
    CHAR    szClientCertPath[NET_TV_LEN_260];
    CHAR    szPrivateKeyPath[NET_TV_LEN_260];
    CHAR    szCtrlInterface[NET_TV_LEN_260];
    CHAR    szInterfaceName[NET_TV_LEN_64];
    BYTE    byRes[256];
} NET_TV_WIFI_STA_CONNECT_S, *LPNET_TV_WIFI_STA_CONNECT_S;

/**
 * @struct tagNETTV4GInfo
 * @brief 4G配置
 */
typedef struct tagNETTV4GInfo
{
    CHAR    szApn[NET_TV_LEN_64];
    CHAR    szUserName[NET_TV_LEN_132];
    CHAR    szPassword[NET_TV_LEN_132];
    CHAR    szCallNumber[NET_TV_LEN_32];
    INT32   nMtu;
    INT32   nAuthMode;                          /* 0:None,1:PAP,2:CHAP,3:PAP&CHAP */
    INT32   nNetworkMode;                       /* 0:Auto,1:4G,2:3G,3:2G */
    INT32   nDialMode;                          /* 0:Auto,1:Manual */
    BYTE    byRes[128];
} NET_TV_4G_INFO_S, *LPNET_TV_4G_INFO_S;

/**
 * @struct tagNETTVHotspotInfo
 * @brief 热点配置
 */
typedef struct tagNETTVHotspotInfo
{
    BOOL    bEnabled;
    CHAR    szSsid[NET_TV_NAME_MAX_LEN];
    CHAR    szSecurityMode[NET_TV_LEN_64];
    CHAR    szEncryptionType[NET_TV_LEN_64];
    CHAR    szPassword[NET_TV_LEN_132];
    CHAR    szConfirmPassword[NET_TV_LEN_132];
    BYTE    byRes[128];
} NET_TV_HOTSPOT_INFO_S, *LPNET_TV_HOTSPOT_INFO_S;

/**
 * @struct tagNETTVHotspotConnDevice
 * @brief 热点连接设备项
 */
typedef struct tagNETTVHotspotConnDevice
{
    INT32   nIndex;
    CHAR    szMac[NET_TV_LEN_64];
    CHAR    szIp[NET_TV_IPADDR_STR_MAX_LEN];
    CHAR    szConnTime[NET_TV_LEN_64];
} NET_TV_HOTSPOT_CONN_DEVICE_S, *LPNET_TV_HOTSPOT_CONN_DEVICE_S;

/**
 * @struct tagNETTVHotspotConnInfo
 * @brief 热点连接设备列表
 */
typedef struct tagNETTVHotspotConnInfo
{
    CHAR    szStatus[NET_TV_LEN_32];
    INT32   nTotal;
    INT32   nDeviceCount;
    NET_TV_HOTSPOT_CONN_DEVICE_S astDevices[NET_TV_HOTSPOT_CONN_MAX_NUM];
} NET_TV_HOTSPOT_CONN_INFO_S, *LPNET_TV_HOTSPOT_CONN_INFO_S;


/**
 * @struct tagNETTVPreviewImageParam
 * @brief 预览图像参数 Preview image parameters
 */
typedef struct tagNETTVPreviewImageParam
{
    INT32   nBrightness;                         /* 亮度 [0,100] */
    INT32   nContrast;                           /* 对比度 [0,100] */
    INT32   nSaturation;                         /* 饱和度 [0,100] */
    INT32   nSharpness;                          /* 锐度 [0,100] */
    BYTE    byRes[64];                           /* 保留字段 */
}NET_TV_PREVIEW_IMAGE_PARAM_S, *LPNET_TV_PREVIEW_IMAGE_PARAM_S;

/**
 * @struct tagNETTVPreviewInfo
 * @brief 预览信息 Preview information
 * @note 用于NET_TV_GET_PREVIEW_INFO/NET_TV_SET_PREVIEW_INFO
 */
typedef struct tagNETTVPreviewInfo
{
    NET_TV_PREVIEW_RTSP_URL_S stRtspUrl;         /* 预览流地址 */
    NET_TV_PREVIEW_IMAGE_PARAM_S stImageParam;   /* 图像参数 */
    BYTE    byRes[256];                          /* 保留字段 */
}NET_TV_PREVIEW_INFO_S, *LPNET_TV_PREVIEW_INFO_S;

/**
 * @struct tagNETTVChannelInfo
 * @brief 通用通道信息 Channel information
 * @note 可用于单通道查询，也可作为通道列表项。
 */
typedef struct tagNETTVChannelInfo
{
    UINT32  dwSize;                              /* 结构体大小 */

    UINT32  dwChannel;                           /* 通道号 */
    BYTE    byEnable;                            /* 是否启用 */
    BYTE    byOnline;                            /* 是否在线 */
    BYTE    byStreamType;                        /* 默认码流：0主码流 1子码流 */
    BYTE    byHasRecord;                         /* 是否有录像 */
    INT32   nRecordStatus;                       /* 当前录制状态 NET_TV_RECORD_STATUS_E */


    INT32   nDevState;                           /* 设备状态 */
    INT32   nAppProto;                           /* 接入协议 */
    INT32   nTransProto;                         /* 传输协议 */
    INT32   nMfrsType;                           /* 厂商类型 */

    INT32   nCtrlPort;                           /* 管理端口 */
    INT32   nReserved[3];                        /* 保留字段 */

    CHAR    szChannelName[NET_TV_LEN_64];        /* 通道名称 */
    CHAR    szDevName[NET_TV_LEN_64];            /* 设备名称 */
    CHAR    szDevType[NET_TV_LEN_64];            /* 设备类型 */
    CHAR    szSerialNum[NET_TV_LEN_64];          /* 序列号 */

    CHAR    szFirmwareVersion[NET_TV_LEN_64];    /* 固件版本 */
    CHAR    szDeviceIP[NET_TV_IPADDR_STR_MAX_LEN]; /* IP地址 */
    CHAR    szMac[NET_TV_LEN_64];                /* MAC地址 */
    CHAR    szSubnetMask[NET_TV_IPADDR_STR_MAX_LEN]; /* 子网掩码 */

    CHAR    szMfrsName[NET_TV_LEN_64];           /* 厂商名称 */
    CHAR    szAppProtoName[NET_TV_LEN_64];       /* 协议名称 */
    CHAR    szOnvifDeviceUrl[NET_TV_MAX_URL_LEN]; /* ONVIF URL */

    CHAR    szPreviewMainUrl[NET_TV_MAX_URL_LEN]; /* 主码流预览地址 */
    CHAR    szPreviewSubUrl[NET_TV_MAX_URL_LEN];  /* 子码流预览地址 */
    CHAR    szRtspMainUrl[NET_TV_MAX_URL_LEN];    /* 直连主码流地址 */
    CHAR    szRtspSubUrl[NET_TV_MAX_URL_LEN];     /* 直连子码流地址 */

    BYTE    byRes[512];                          /* 保留字段 */
}NET_TV_CHANNEL_INFO_S, *LPNET_TV_CHANNEL_INFO_S;

#ifndef NET_TV_MAX_CHANNEL_NUM
#define NET_TV_MAX_CHANNEL_NUM 128
#endif

typedef struct tagNETTVChannelList
{
    UINT32  dwSize;
    UINT32  dwChannelCount;                      /* 实际通道数量 */
    NET_TV_CHANNEL_INFO_S stChannels[NET_TV_MAX_CHANNEL_NUM];
    BYTE    byRes[512];
}NET_TV_CHANNEL_LIST_S, *LPNET_TV_CHANNEL_LIST_S;

typedef NET_TV_CHANNEL_INFO_S NET_TV_DIRECT_CONNECT_CHAN_INFO_S, *LPNET_TV_DIRECT_CONNECT_CHAN_INFO_S;

/**
 * @struct tagNETTVRevTimeout
 * @brief 超时时间 结构体定义  Timeout structure definition
 * @attention
*/
typedef struct tagNETTVRevTimeout
{
    INT32   dwRevTimeOut;                 /* 设置接收超时时间 Set timeout for receiving */
    INT32   dwFileReportTimeOut;          /* 设置文件操作超时时间 Set timeout for file operation */
    BYTE    byRes[128];                   /* 保留字段  Reserved */
}NET_TV_REV_TIMEOUT_S, *LPNET_TV_REV_TIMEOUT_S;

/**
 * @struct tagNETTVAlarmBasicInfo
 * @brief 基础/常规报警 (0x1000 - 0x10FF)
 */
typedef struct tagNETTVAlarmBasicInfo
{
    UINT32      dwAlarmType;                                    /* 报警类型 */
    UINT32      dwAlarmInputNumber;                             /* 报警输入口 */
    BYTE        byAlarmOutputNumber[NET_TV_MAX_ALARM_OUT_NUM]; /* 触发的报警输出口，为1表示触发该口 */
    BYTE        byAlarmRelateChannel[NET_TV_MAX_ALARM_IN_NUM]; /* 触发的录像通道，为1表示触发该通道 */
    BYTE        byChannel[NET_TV_MAX_ALARM_IN_NUM];            /* 报警通道，为1表示触发该通道 */
    BYTE        byDiskNumber[NET_TV_LOCAL_DISK_MAX_NUM];       /* 发生报警的硬盘，为1表示该硬盘异常 */
    BYTE        byPanoramaImg[NET_TV_PIC_DATA_MAX_LEN];        /* 全景 JPEG 二进制图片 */
    UINT32      dwPanoramaImgLen;                              /* 全景 JPEG 图片长度 */
    BYTE        byRes[128];                                    /* 保留字段 */
}NET_TV_ALARM_BASIC_INFO_S, *LPNET_TV_ALARM_BASIC_INFO_S;

/**
 * @struct tagNETTVAlarmRuleInfo
 * @brief 区域/周界规则报警 (0x2000 - 0x20FF)
 * @note 约定：dwAlarmType 填写命令码(如 NET_TV_ALARM_LINE_CROSSING)
 */
typedef struct tagNETTVAlarmRuleInfo
{
    UINT32      dwAlarmType;                         /* 报警类型/命令码 */
    UINT32      dwChannel;                           /* 通道号 */
    UINT32      dwRuleID;                            /* 规则ID */
    UINT32      dwRuleType;                          /* 规则类型(可与dwAlarmType对应) */
    CHAR        szRuleName[NET_TV_LEN_64];           /* 规则名称(可选) */
    UINT32      dwTargetID;                          /* 目标ID(可选) */
    BYTE        byPanoramaImg[NET_TV_PIC_DATA_MAX_LEN]; /* 全景 JPEG 二进制图片 */
    UINT32      dwPanoramaImgLen;                    /* 全景 JPEG 图片长度 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_ALARM_RULE_INFO_S, *LPNET_TV_ALARM_RULE_INFO_S;

/**
 * @struct tagNETTVAlarmAiObjectInfo
 * @brief Smart/AI 行为分析 (0x3000 - 0x3FFF)
 * @note 约定：dwAlarmType 填写命令码(如 NET_TV_ALARM_FACE_CAPTURE)
 */
typedef struct tagNETTVAlarmAiObjectInfo
{
    UINT32      dwAlarmType;                         /* 报警类型/命令码 */
    UINT32      dwChannel;                           /* 通道号 */
    UINT32      dwObjectType;                        /* 目标类型(0:未知 1:人 2:车 ... 可扩展) */
    float       fConfidence;                         /* 置信度 0~1 */
    INT32       nLeft;                               /* 目标框 left */
    INT32       nTop;                                /* 目标框 top */
    INT32       nRight;                              /* 目标框 right */
    INT32       nBottom;                             /* 目标框 bottom */
    CHAR        szObjectID[NET_TV_LEN_64];           /* 目标ID(可选) */
    BYTE        byImgData[NET_TV_PIC_DATA_MAX_LEN];  /* 报警图片数据 */
    UINT32      dwImgLen;                            /* 图片长度 */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_ALARM_AI_OBJECT_INFO_S, *LPNET_TV_ALARM_AI_OBJECT_INFO_S;

/**
 * @struct tagNETTVAlarmFaceCompareInfo
 * @brief 人脸比对结果告警
 * @note 约定：dwAlarmType 填写 NET_TV_ALARM_FACE_COMPARE
 */
typedef struct tagNETTVAlarmFaceCompareInfo
{
    UINT32      dwAlarmType;                         /* 报警类型/命令码 */
    UINT32      dwChannel;                           /* 通道号 */
    INT64       llTimestampMs;                       /* 报警时间戳，单位毫秒 */
    INT32       nEventId;                            /* 事件ID */
    INT32       nCompResult;                         /* 比对结果：0-不匹配 1-匹配 */
    INT32       nSimilarity;                         /* 相似度 0-100 */
    INT32       nFaceId;                             /* 人脸ID */
    CHAR        szFaceLibName[NET_TV_FACE_DB_NAME_LEN];      /* 目标库名称 */
    CHAR        szFaceName[NET_TV_FACE_MEMBER_NAME_LEN];     /* 人脸名称 */
    CHAR        szLibFacePath[NET_TV_LEN_260];       /* 目标库人脸图片路径 */
    CHAR        szCapFacePath[NET_TV_LEN_260];       /* 抓拍人脸图片路径 */
    CHAR        szCapImagePath[NET_TV_LEN_260];      /* 抓拍原图路径 */
    BYTE        byLibFaceImg[NET_TV_FACE_IMAGE_MAX_LEN];     /* 目标库人脸 JPEG 二进制图片 */
    UINT32      dwLibFaceImgLen;                     /* 目标库人脸 JPEG 图片长度 */
    BYTE        byCapFaceImg[NET_TV_FACE_IMAGE_MAX_LEN];     /* 抓拍人脸 JPEG 二进制图片 */
    UINT32      dwCapFaceImgLen;                     /* 抓拍人脸 JPEG 图片长度 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_ALARM_FACE_COMPARE_INFO_S, *LPNET_TV_ALARM_FACE_COMPARE_INFO_S;

/**
 * @struct tagNETTVAlarmPlateInfo
 * @brief 交通/车辆相关 (0x4000 - 0x40FF)
 * @note 约定：dwAlarmType 填写命令码(如 NET_TV_ALARM_PLATE_RECOGNITION)
 */
typedef struct tagNETTVAlarmPlateInfo
{
    UINT32      dwAlarmType;                         /* 报警类型/命令码 */
    UINT32      dwChannel;                           /* 通道号 */
    CHAR        szPlateNumber[NET_TV_LEN_32];        /* 车牌号 */
    UINT32      dwPlateColor;                        /* 车牌颜色(枚举可扩展) */
    UINT32      dwVehicleType;                       /* 车辆类型(枚举可扩展) */
    float       fConfidence;                         /* 置信度 0~1 */
    UINT32      dwSpeed;                             /* 速度(km/h，可选) */
    UINT32      dwLaneNo;                            /* 车道号(可选) */
    BYTE        byPlateImg[NET_TV_VEH_PLATE_IMAGE_LEN]; /* 车牌图片 */
    UINT32      dwPlateImgLen;                       /* 车牌图片长度 */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_ALARM_PLATE_INFO_S, *LPNET_TV_ALARM_PLATE_INFO_S;

/**
 * @struct tagNETTVAlarmExceptionInfo
 * @brief 设备异常/状态事件 (0x5000 - 0x50FF)
 * @note 约定：dwAlarmType 填写命令码(如 NET_TV_ALARM_DISK_FULL)
 */
typedef struct tagNETTVAlarmExceptionInfo
{
    UINT32      dwAlarmType;                         /* 报警类型/命令码 */
    UINT32      dwChannel;                           /* 通道号(若无则填0) */
    UINT32      dwDiskNo;                            /* 硬盘号(若无则填0) */
    UINT32      dwStatus;                            /* 状态(0:恢复 1:触发) */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_ALARM_EXCEPTION_INFO_S, *LPNET_TV_ALARM_EXCEPTION_INFO_S;

/**
 * @struct tagNETTVAlarmStatisticsTarget
 * @brief 统计类告警目标快照
 */
typedef struct tagNETTVAlarmStatisticsTarget
{
    INT32       nTrackID;                             /* 目标跟踪 ID */
    UINT32      dwRuleID;                             /* 规则 ID */
    UINT32      dwSnapshotType;                       /* 快照类型：进入/离开/区域当前目标 */
    INT32       nLeft;                                /* 目标框 left */
    INT32       nTop;                                 /* 目标框 top */
    INT32       nRight;                               /* 目标框 right */
    INT32       nBottom;                              /* 目标框 bottom */
    INT64       llTimestampMs;                        /* 快照时间戳，单位毫秒 */
    INT32       nDirection;                           /* 目标方向，跨线类事件填业务方向枚举值 */
    BYTE        byRes[64];                            /* 保留字段 */
}NET_TV_ALARM_STATISTICS_TARGET_S, *LPNET_TV_ALARM_STATISTICS_TARGET_S;

/**
 * @struct tagNETTVAlarmStatisticsInfo
 * @brief 统计类通用告警，优先用于人流统计和人员密度统计
 */
typedef struct tagNETTVAlarmStatisticsInfo
{
    UINT32      dwAlarmType;                          /* 报警类型/命令码 */
    UINT32      dwChannel;                            /* 通道号 */
    UINT32      dwStatisticsType;                     /* 统计子类型 NET_TV_STATISTICS_TYPE_E */
    UINT32      dwRuleID;                             /* 规则 ID */
    INT64       llTimestampMs;                        /* 报告时间戳，单位毫秒 */
    UINT32      dwReportSeq;                          /* 统计报告序号 */
    UINT32      dwEnterCount;                         /* 累计进入人数 */
    UINT32      dwLeaveCount;                         /* 累计离开人数 */
    UINT32      dwTotalCount;                         /* 累计通行总人数 */
    UINT32      dwCurrentPeopleCount;                 /* 当前区域人数 */
    UINT32      dwAverageStayTimeSec;                 /* 平均停留时间，单位秒 */
    UINT32      dwTargetCount;                        /* 当前目标快照数量 */
    NET_TV_ALARM_STATISTICS_TARGET_S stTargets[NET_TV_ALARM_STATISTICS_TARGET_MAX_NUM]; /* 目标快照列表 */
    BYTE        byPanoramaImg[NET_TV_PIC_DATA_MAX_LEN]; /* 全景 JPEG 二进制图片 */
    UINT32      dwPanoramaImgLen;                     /* 全景 JPEG 图片长度 */
    BYTE        byRes[256];                           /* 保留字段 */
}NET_TV_ALARM_STATISTICS_INFO_S, *LPNET_TV_ALARM_STATISTICS_INFO_S;

/* ==================== 布防时间和联动相关结构体 ==================== */

/**
 * @struct tagNETTVSchedTime
 * @brief 时间段结构体 Schedule time structure
 * @note 用于布防时间配置
 */
typedef struct tagNETTVSchedTime
{
    INT32       nStartHour;                          /* 开始小时 [0-23] */
    INT32       nStartMinute;                       /* 开始分钟 [0-59] */
    INT32       nEndHour;                           /* 结束小时 [0-23] */
    INT32       nEndMinute;                         /* 结束分钟 [0-59] */
    BYTE        byRes[16];                          /* 保留字段 */
}NET_TV_SCHED_TIME_S, *LPNET_TV_SCHED_TIME_S;

/**
 * @struct tagNETTVAlarmSchedule
 * @brief 布防时间配置 Alarm schedule configuration
 * @note 一周7天，每天最多8个时间段
 */
typedef struct tagNETTVAlarmSchedule
{
    INT32       dwTimeSectionCount[7];               /* 每天的时间段数量 [0-8] */
    NET_TV_SCHED_TIME_S astTimeSection[7][NET_TV_PLAN_SECTION_NUM]; /* 一周7天，每天最多8个时间段 */
    BYTE        byRes[64];                          /* 保留字段 */
}NET_TV_ALARM_SCHEDULE_S, *LPNET_TV_ALARM_SCHEDULE_S;

/**
 * @struct tagNETTVLinkageList
 * @brief 联动配置列表 Linkage configuration list
 */
typedef struct tagNETTVLinkageList
{
    INT32       dwAlarmOutputCount;                  /* 报警输出数量 */
    INT32       adwAlarmOutput[NET_TV_MAX_ALARM_OUT_NUM]; /* 报警输出通道号数组 */
    INT32       dwRecordChannelCount;                /* 录像通道数量 */
    INT32       adwRecordChannel[NET_TV_CHANNEL_MAX]; /* 录像通道号数组 */
    INT32       dwSnapshotChannelCount;              /* 抓拍通道数量 */
    INT32       adwSnapshotChannel[NET_TV_CHANNEL_MAX]; /* 抓拍通道号数组 */
    BYTE        byRes[256];                         /* 保留字段 */
}NET_TV_LINKAGE_LIST_S, *LPNET_TV_LINKAGE_LIST_S;


/**
 * @struct tagNETTVPeopleFlowRuleLine
 * @brief 人流统计规则线
 */
typedef struct tagNETTVPeopleFlowRuleLine
{
    FLOAT       fStartPointX;                         /* 规则线起点X坐标 [0.0-1.0] */
    FLOAT       fStartPointY;                         /* 规则线起点Y坐标 [0.0-1.0] */
    FLOAT       fEndPointX;                           /* 规则线终点X坐标 [0.0-1.0] */
    FLOAT       fEndPointY;                           /* 规则线终点Y坐标 [0.0-1.0] */
    INT32       nDirection;                           /* 统计方向 1-A到B 2-B到A */
    BYTE        byRes[60];                            /* 保留字段 */
} NET_TV_PEOPLE_FLOW_RULE_LINE_S;

/**
 * @struct tagNETTVPeopleAlarmRule
 * @brief 单档人数报警配置
 */
typedef struct tagNETTVPeopleAlarmRule
{
    BOOL        bEnable;                               /* 是否启用 */
    INT32       nThreshold;                           /* 人数触发阈值 */
    NET_TV_LINKAGE_LIST_S stLinkageList;              /* 联动配置 */
    BYTE        byRes[128];                           /* 保留字段 */
} NET_TV_PEOPLE_ALARM_RULE_S;

/**
 * @struct tagNETTVPeopleAlarmConfig
 * @brief 三级人数报警配置
 */
typedef struct tagNETTVPeopleAlarmConfig
{
    NET_TV_PEOPLE_ALARM_RULE_S stNormal;              /* 普通报警 */
    NET_TV_PEOPLE_ALARM_RULE_S stMedium;              /* 中度报警 */
    NET_TV_PEOPLE_ALARM_RULE_S stSevere;              /* 严重报警 */
    BYTE        byRes[256];                           /* 保留字段 */
} NET_TV_PEOPLE_ALARM_CONFIG_S;

/**
 * @struct tagNETTVStatisticsResetConfig
 * @brief 定时清零配置
 */
typedef struct tagNETTVStatisticsResetConfig
{
    BOOL        bEnable;                               /* 是否启用 */
    INT32       nHour;                                /* 执行小时 [0-23] */
    INT32       nMinute;                              /* 执行分钟 [0-59] */
    BYTE        byRes[16];                            /* 保留字段 */
} NET_TV_STATISTICS_RESET_CONFIG_S;

/**
 * @struct tagNETTVPeopleFlowStatisticsCfg
 * @brief 人流统计配置信息
 * @note 用于NET_TV_GET_PEOPLE_FLOW_STATISTICS_CFG/NET_TV_SET_PEOPLE_FLOW_STATISTICS_CFG
 */
typedef struct tagNETTVPeopleFlowStatisticsCfg
{
    BOOL        bEnable;                               /* 是否启用人流统计 */
    INT32       nSensitivity;                           /* 灵敏度[1-100] */
    NET_TV_PEOPLE_FLOW_RULE_LINE_S stRuleLine;        /* 规则线 */
    INT32       dwPointCount;                         /* 检测区域顶点数，最多32个 */
    FLOAT       afPointX[32];                         /* 检测区域X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                         /* 检测区域Y坐标数组 [0.0-1.0] */
    INT32       nReportInterval;                        /* 数据上报间隔(秒) */
    INT32       enStatisticsType;                      /* 统计类型 NET_TV_PEOPLE_FLOW_STAT_TYPE_E */
    NET_TV_STATISTICS_RESET_CONFIG_S stTimedReset;     /* 定时清零 */
    NET_TV_PEOPLE_ALARM_CONFIG_S stStayAlarm;          /* 滞留人数报警 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    BYTE        byRes[256];                           /* 保留字段 */
} NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S, *LPNET_TV_PEOPLE_FLOW_STATISTICS_CFG_S;

/**
 * @struct tagNETTVPeopleDensityDetectionCfg
 * @brief 人员密度检测配置信息
 * @note 用于NET_TV_GET_PEOPLE_DENSITY_DETECTION_CFG/NET_TV_SET_PEOPLE_DENSITY_DETECTION_CFG
 */
typedef struct tagNETTVPeopleDensityDetectionCfg
{
    BOOL        bEnable;                               /* 是否启用人员密度检测 */
    INT32       nSensitivity;                           /* 灵敏度[1-100] */
    INT32       dwPointCount;                         /* 检测区域顶点数，最多32个 */
    FLOAT       afPointX[32];                         /* 检测区域X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                         /* 检测区域Y坐标数组 [0.0-1.0] */
    INT32       nReportInterval;                        /* 数据上报间隔(秒) */
    NET_TV_PEOPLE_ALARM_CONFIG_S stDensityAlarm;      /* 密度报警 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    BYTE        byRes[256];                           /* 保留字段 */
} NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S, *LPNET_TV_PEOPLE_DENSITY_DETECTION_CFG_S;


/* ==================== 移动侦测相关结构体 ==================== */

/**
 * @struct tagNETTVMotionRegion
 * @brief 移动侦测专家模式区域参数 Motion detection expert mode region
 */
typedef struct tagNETTVMotionRegion
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
}NET_TV_MOTION_REGION_S, *LPNET_TV_MOTION_REGION_S;

/**
 * @struct tagNETTVMotionExpertMode
 * @brief 移动侦测专家模式参数 Motion detection expert mode
 */
typedef struct tagNETTVMotionExpertMode
{
    INT32       nExpertDayNightCtrl;                /* 日夜控制：0-关闭(默认)，1-自动切换，2-定时切换 */
    NET_TV_SCHED_TIME_S stDayTime;                  /* 日夜切换时间 定时切换时有效 */
    INT32       dwRegionCount;                      /* 区域数量 */
    NET_TV_MOTION_REGION_S astRegion[16];            /* 移动侦测专家模式区域参数，最多16个 */
    BYTE        byRes[128];                         /* 保留字段 */
}NET_TV_MOTION_EXPERT_MODE_S, *LPNET_TV_MOTION_EXPERT_MODE_S;

/**
 * @struct tagNETTVMotionNormalMode
 * @brief 移动侦测普通模式参数 Motion detection normal mode
 */
typedef struct tagNETTVMotionNormalMode
{
    INT32       nSensitivity;                        /* 灵敏度 [0,100] */
    INT32       nRegionType;                         /* 区域类型 0：筒型 1：网格 */
    INT32       nRectLeft;                           /* 筒型区域左坐标 (当nRegionType=0时有效) */
    INT32       nRectTop;                            /* 筒型区域上坐标 */
    INT32       nRectRight;                          /* 筒型区域右坐标 */
    INT32       nRectBottom;                         /* 筒型区域下坐标 */
    INT32       dwGridWidth;                         /* 网格宽度 (当nRegionType=1时有效，通常22) */
    INT32       dwGridHeight;                        /* 网格高度 (当nRegionType=1时有效，通常18) */
    BYTE        abyGridArea[18][22];                /* 网格区域标记，1表示移动侦测区域 (18x22) */
    BYTE        byRes[128];                          /* 保留字段 */
}NET_TV_MOTION_NORMAL_MODE_S, *LPNET_TV_MOTION_NORMAL_MODE_S;

/**
 * @struct tagNETTVMotionAlarmInfo
 * @brief 移动侦测告警配置信息 Motion detection alarm configuration
 * @note 用于NET_TV_GET_MOTIONALARM/NET_TV_SET_MOTIONALARM
 */
typedef struct tagNETTVMotionAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    BOOL        bDynamicAnalysisEnable;              /* 是否启用动态分析 */
    INT32       dwMode;                              /* 模式 0-普通模式 1-专家模式 NET_TV_MOTION_MODE_E */
    NET_TV_MOTION_NORMAL_MODE_S stNormalMode;       /* 普通模式参数 */
    NET_TV_MOTION_EXPERT_MODE_S stExpertMode;       /* 专家模式参数 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;        /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;            /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_MOTION_ALARM_INFO_S, *LPNET_TV_MOTION_ALARM_INFO_S;

/* ==================== 隐私遮盖配置相关结构体 ==================== */

/**
 * @struct tagNETTVPrivacyMaskArea
 * @brief 单个隐私遮盖区域 Privacy mask area
 * @note 用于NET_TV_PRIVACY_MASK_CFG_S中配置单个遮盖区域
 */
typedef struct tagNETTVPrivacyMaskArea
{
    INT32       nAreaID;                              /* 遮盖区域ID [0, NET_TV_MAX_PRIVACY_MASK_AREA_NUM) */
    BOOL        bEnable;                              /* 是否启用 0-不启用 1-启用 */
    INT32       nRectLeft;                            /* 遮盖区域左坐标 [0, 8191] */
    INT32       nRectTop;                             /* 遮盖区域上坐标 [0, 8191] */
    INT32       nRectRight;                           /* 遮盖区域右坐标 [0, 8191] */
    INT32       nRectBottom;                          /* 遮盖区域下坐标 [0, 8191] */
    BYTE        byRes[32];                            /* 保留字段 */
}NET_TV_PRIVACY_MASK_AREA_S, *LPNET_TV_PRIVACY_MASK_AREA_S;

/**
 * @struct tagNETTVPrivacyMaskCfg
 * @brief 隐私遮盖配置信息 Privacy mask configuration
 * @note 用于NET_TV_GET_PRIVACYMASKCFG/NET_TV_SET_PRIVACYMASKCFG
 */
typedef struct tagNETTVPrivacyMaskCfg
{
    BOOL        bEnable;                              /* 是否启用隐私遮盖 0-不启用 1-启用 */
    INT32       dwAreaCount;                          /* 遮盖区域数量 [0, NET_TV_MAX_PRIVACY_MASK_AREA_NUM] */
    NET_TV_PRIVACY_MASK_AREA_S astArea[NET_TV_MAX_PRIVACY_MASK_AREA_NUM]; /* 遮盖区域数组 */
    BYTE        byRes[256];                           /* 保留字段 */
}NET_TV_PRIVACY_MASK_CFG_S, *LPNET_TV_PRIVACY_MASK_CFG_S;

/* ==================== 遮挡报警相关结构体 ==================== */

/**
 * @struct tagNETTVTamperAlarmInfo
 * @brief 遮挡检测告警配置信息 Tamper detection alarm configuration
 * @note 用于NET_TV_GET_TAMPERALARM/NET_TV_SET_TAMPERALARM
 */
typedef struct tagNETTVTamperAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwSensitivity;                        /* 遮挡报警灵敏度[0,3]，值越大越灵敏 */
    INT32       nRectLeft;                           /* 遮挡区域左坐标 */
    INT32       nRectTop;                            /* 遮挡区域上坐标 */
    INT32       nRectRight;                          /* 遮挡区域右坐标 */
    INT32       nRectBottom;                         /* 遮挡区域下坐标 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;        /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;            /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_TAMPER_ALARM_INFO_S, *LPNET_TV_TAMPER_ALARM_INFO_S;

/* ==================== 越界检测相关结构体 ==================== */

/**
 * @struct tagNETTVBoundaryPlane
 * @brief 越界检测警戒线规则 Boundary detection plane rule
 */
typedef struct tagNETTVBoundaryPlane
{
    BOOL        bEnable;                             /* 是否启用 */
    FLOAT       fStartPosX;                          /* 警戒线起始点X坐标 [0.0-1.0] */
    FLOAT       fStartPosY;                          /* 警戒线起始点Y坐标 [0.0-1.0] */
    FLOAT       fEndPosX;                            /* 警戒线终止点X坐标 [0.0-1.0] */
    FLOAT       fEndPosY;                            /* 警戒线终止点Y坐标 [0.0-1.0] */
    INT32       enCrossDirection;                    /* 警戒线的穿越方向 NET_TV_CROSS_DIRECTION_E */
    INT32       nSensitivity;                        /* 警戒线灵敏度[1,100] */
    INT32       dwDetectionTargetCount;              /* 检测目标数量 */
    INT32       adwDetectionTarget[8];               /* 检测目标数组 NET_TV_DETECTION_TARGET_E，最多8个 */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_BOUNDARY_PLANE_S, *LPNET_TV_BOUNDARY_PLANE_S;

/**
 * @struct tagNETTVCrossLineAlarmInfo
 * @brief 越界检测告警配置信息 Cross line detection alarm configuration
 * @note 用于NET_TV_GET_CROSSLINEALARM/NET_TV_SET_CROSSLINEALARM
 */
typedef struct tagNETTVCrossLineAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多4个 */
    NET_TV_BOUNDARY_PLANE_S astRule[4];              /* 越界检测区域规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;        /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;            /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_CROSS_LINE_ALARM_INFO_S, *LPNET_TV_CROSS_LINE_ALARM_INFO_S;

/* ==================== 入侵检测相关结构体 ==================== */

/**
 * @struct tagNETTVIntrusionRule
 * @brief 入侵检测区域规则参数 Intrusion detection region rule
 */
typedef struct tagNETTVIntrusionRule
{
    BOOL        bEnable;                             /* 是否启用 */
    INT32       dwPointCount;                        /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，判断有效报警的时间[0,100] 单位秒 */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       dwDetectionTargetCount;              /* 检测目标数量 */
    INT32       adwDetectionTarget[8];               /* 检测目标数组 NET_TV_DETECTION_TARGET_E，最多8个 */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_INTRUSION_RULE_S, *LPNET_TV_INTRUSION_RULE_S;

/**
 * @struct tagNETTVIntrusionAlarmInfo
 * @brief 入侵检测告警配置信息 Intrusion detection alarm configuration
 * @note 用于NET_TV_GET_INTRUSIONALARM/NET_TV_SET_INTRUSIONALARM
 */
typedef struct tagNETTVIntrusionAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多4个 */
    NET_TV_INTRUSION_RULE_S astRule[4];              /* 区域入侵检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;        /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;            /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_INTRUSION_ALARM_INFO_S, *LPNET_TV_INTRUSION_ALARM_INFO_S;

/**
 * @struct tagNETTVEnterRegionAlarmInfo
 * @brief 进入区域侦测告警配置信息 Enter region detection alarm configuration
 * @note 用于NET_TV_GET_ENTERREGIONALARM/NET_TV_SET_ENTERREGIONALARM
 */
typedef struct tagNETTVEnterRegionAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多4个 */
    NET_TV_INTRUSION_RULE_S astRule[4];              /* 进入区域检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;        /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;            /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_ENTER_REGION_ALARM_INFO_S, *LPNET_TV_ENTER_REGION_ALARM_INFO_S;

/**
 * @struct tagNETTVLeaveRegionAlarmInfo
 * @brief 离开区域侦测告警配置信息 Leave region detection alarm configuration
 * @note 用于NET_TV_GET_LEAVEREGIONALARM/NET_TV_SET_LEAVEREGIONALARM
 */
typedef struct tagNETTVLeaveRegionAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多4个 */
    NET_TV_INTRUSION_RULE_S astRule[4];              /* 离开区域检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;         /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;             /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_LEAVE_REGION_ALARM_INFO_S, *LPNET_TV_LEAVE_REGION_ALARM_INFO_S;

/* ==================== 徘徊侦测相关结构体 ==================== */

/**
 * @struct tagNETTVLoiteringRule
 * @brief 徘徊侦测区域规则参数 Loitering detection region rule
 */
typedef struct tagNETTVLoiteringRule
{
    BOOL        bEnable;                             /* 是否启用 */
    INT32       dwPointCount;                        /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，判断有效报警的时间[0,100] 单位秒 */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       dwDetectionTargetCount;              /* 检测目标数量 */
    INT32       adwDetectionTarget[8];               /* 检测目标数组 NET_TV_DETECTION_TARGET_E，最多8个 */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_LOITERING_RULE_S, *LPNET_TV_LOITERING_RULE_S;

/**
 * @struct tagNETTVLoiteringAlarmInfo
 * @brief 徘徊告警配置信息 Loitering detection alarm configuration
 * @note 用于NET_TV_GET_LOITERINGALARM/NET_TV_SET_LOITERINGALARM
 */
typedef struct tagNETTVLoiteringAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多4个 */
    NET_TV_LOITERING_RULE_S astRule[4];              /* 区域入侵检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;        /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;            /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_LOITERING_ALARM_INFO_S, *LPNET_TV_LOITERING_ALARM_INFO_S;

/**
 * @struct tagNETTVSceneChangeAlarmInfo
 * @brief 场景变更侦测告警配置信息 Scene change detection alarm configuration
 * @note 用于NET_TV_GET_SCENECHANGEALARM/NET_TV_SET_SCENECHANGEALARM
 */
typedef struct tagNETTVSceneChangeAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       nSensitivity;                        /* 灵敏度 [1,100] */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;         /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;             /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_SCENE_CHANGE_ALARM_INFO_S, *LPNET_TV_SCENE_CHANGE_ALARM_INFO_S;

/**
 * @struct tagNETTVCrowdGatheringRule
 * @brief 人员聚集侦测区域规则参数 Crowd gathering detection region rule
 */
typedef struct tagNETTVCrowdGatheringRule
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwPointCount;                        /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nObjectOccup;                        /* 人体面积占用户设定区域面积的比例阈值[1,100] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_CROWD_GATHERING_RULE_S, *LPNET_TV_CROWD_GATHERING_RULE_S;

/**
 * @struct tagNETTVCrowdGatheringAlarmInfo
 * @brief 人员聚集侦测告警配置信息 Crowd gathering detection alarm configuration
 * @note 用于NET_TV_GET_CROWDGATHERINGALARM/NET_TV_SET_CROWDGATHERINGALARM
 */
typedef struct tagNETTVCrowdGatheringAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多4个 */
    NET_TV_CROWD_GATHERING_RULE_S astRule[4];        /* 人员聚集侦测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;         /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;             /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_CROWD_GATHERING_ALARM_INFO_S, *LPNET_TV_CROWD_GATHERING_ALARM_INFO_S;

/**
 * @struct tagNETTVParkingRule
 * @brief 停车侦测区域规则参数 Parking detection region rule
 */
typedef struct tagNETTVParkingRule
{
    INT32       dwPointCount;                        /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，单位秒 [0,100] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_PARKING_RULE_S, *LPNET_TV_PARKING_RULE_S;

/**
 * @struct tagNETTVParkingAlarmInfo
 * @brief 停车侦测告警配置信息 Parking detection alarm configuration
 * @note 用于NET_TV_GET_PARKINGALARM/NET_TV_SET_PARKINGALARM
 */
typedef struct tagNETTVParkingAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多8个 */
    NET_TV_PARKING_RULE_S astRule[8];                /* 停车侦测规则，最多8个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;         /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;             /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_PARKING_ALARM_INFO_S, *LPNET_TV_PARKING_ALARM_INFO_S;

/* ==================== 垃圾暴露检测相关结构体 ==================== */

/**
 * @struct tagNETTVGarbageExposureRule
 * @brief 垃圾暴露检测规则参数 Garbage exposure detection region rule
 */
typedef struct tagNETTVGarbageExposureRule
{
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       dwPointCount;                          /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                          /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                          /* 区域顶点Y坐标数组 [0.0-1.0] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_TV_GARBAGE_EXPOSURE_RULE_S, *LPNET_TV_GARBAGE_EXPOSURE_RULE_S;

/**
 * @struct tagNETTVGarbageExposureCfg
 * @brief 垃圾暴露检测配置信息 Garbage exposure detection configuration
 * @note 用于NET_TV_GET_GARBAGE_EXPOSURE_CFG/NET_TV_SET_GARBAGE_EXPOSURE_CFG
 */
typedef struct tagNETTVGarbageExposureCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_GARBAGE_EXPOSURE_RULE_S stRule;             /* 垃圾暴露检测规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;                /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_GARBAGE_EXPOSURE_CFG_S, *LPNET_TV_GARBAGE_EXPOSURE_CFG_S;

/* ==================== 垃圾满溢检测相关结构体 ==================== */

/**
 * @struct tagNETTVGarbageOverflowRule
 * @brief 垃圾满溢检测规则参数 Garbage overflow detection region rule
 */
typedef struct tagNETTVGarbageOverflowRule
{
    INT32       nSensitivity;                          /* 灵敏度[1,100] */
    INT32       dwPointCount;                          /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                          /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                          /* 区域顶点Y坐标数组 [0.0-1.0] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_TV_GARBAGE_OVERFLOW_RULE_S, *LPNET_TV_GARBAGE_OVERFLOW_RULE_S;

/**
 * @struct tagNETTVGarbageOverflowCfg
 * @brief 垃圾满溢检测配置信息 Garbage overflow detection configuration
 * @note 用于NET_TV_GET_GARBAGE_OVERFLOW_CFG/NET_TV_SET_GARBAGE_OVERFLOW_CFG
 */
typedef struct tagNETTVGarbageOverflowCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_GARBAGE_OVERFLOW_RULE_S stRule;             /* 垃圾满溢检测规则 */
    INT32       nTimeThreshold;                        /* 时间阈值(秒) */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;          /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_GARBAGE_OVERFLOW_CFG_S, *LPNET_TV_GARBAGE_OVERFLOW_CFG_S;

/* ==================== 单规则智能检测配置结构体 ==================== */

/**
 * @struct tagNETTVAiSimpleRule
 * @brief 单规则智能检测通用规则参数
 */
typedef struct tagNETTVAiSimpleRule
{
    INT32       nSensitivity;                          /* 灵敏度[1,100] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_TV_AI_SIMPLE_RULE_S, *LPNET_TV_AI_SIMPLE_RULE_S;

/**
 * @struct tagNETTVManholeCoverAbnormalCfg
 * @brief 井盖异常检测配置信息 Manhole cover abnormal detection configuration
 * @note 用于NET_TV_GET_MANHOLE_COVER_ABNORMAL_CFG/NET_TV_SET_MANHOLE_COVER_ABNORMAL_CFG
 */
typedef struct tagNETTVManholeCoverAbnormalCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 井盖异常检测规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S, *LPNET_TV_MANHOLE_COVER_ABNORMAL_CFG_S;

/**
 * @struct tagNETTVSleepOnDutyCfg
 * @brief 睡岗识别配置信息 Sleep on duty detection configuration
 * @note 用于NET_TV_GET_SLEEP_ON_DUTY_CFG/NET_TV_SET_SLEEP_ON_DUTY_CFG
 */
typedef struct tagNETTVSleepOnDutyCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 睡岗识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_SLEEP_ON_DUTY_CFG_S, *LPNET_TV_SLEEP_ON_DUTY_CFG_S;

/**
 * @struct tagNETTVElectricVehicleInElevatorCfg
 * @brief 电瓶车进电梯识别配置信息 Electric vehicle in elevator detection configuration
 * @note 用于NET_TV_GET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG/NET_TV_SET_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG
 */
typedef struct tagNETTVElectricVehicleInElevatorCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 电瓶车进电梯识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S, *LPNET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S;

/**
 * @struct tagNETTVPersonFallDownCfg
 * @brief 人员倒地识别配置信息 Person fall down detection configuration
 * @note 用于NET_TV_GET_PERSON_FALL_DOWN_CFG/NET_TV_SET_PERSON_FALL_DOWN_CFG
 */
typedef struct tagNETTVPersonFallDownCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 人员倒地识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_PERSON_FALL_DOWN_CFG_S, *LPNET_TV_PERSON_FALL_DOWN_CFG_S;

/**
 * @struct tagNETTVConstructionOccupyRoadCfg
 * @brief 施工占道识别配置信息 Construction occupy road detection configuration
 * @note 用于NET_TV_GET_CONSTRUCTION_OCCUPY_ROAD_CFG/NET_TV_SET_CONSTRUCTION_OCCUPY_ROAD_CFG
 */
typedef struct tagNETTVConstructionOccupyRoadCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 施工占道识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S, *LPNET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S;

/**
 * @struct tagNETTVCongestionCfg
 * @brief 拥堵识别配置信息 Congestion detection configuration
 * @note 用于NET_TV_GET_CONGESTION_CFG/NET_TV_SET_CONGESTION_CFG
 */
typedef struct tagNETTVCongestionCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 拥堵识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_CONGESTION_CFG_S, *LPNET_TV_CONGESTION_CFG_S;

/**
 * @struct tagNETTVLicensePlateRecognitionCfg
 * @brief 车牌识别配置信息 License plate recognition configuration
 * @note 用于NET_TV_GET_LICENSE_PLATE_RECOGNITION_CFG/NET_TV_SET_LICENSE_PLATE_RECOGNITION_CFG
 */
typedef struct tagNETTVLicensePlateRecognitionCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 车牌识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S, *LPNET_TV_LICENSE_PLATE_RECOGNITION_CFG_S;

/**
 * @struct tagNETTVHighAltitudeSeatbeltCfg
 * @brief 高空安全带识别配置信息 High altitude seatbelt detection configuration
 * @note 用于NET_TV_GET_HIGH_ALTITUDE_SEATBELT_CFG/NET_TV_SET_HIGH_ALTITUDE_SEATBELT_CFG
 */
typedef struct tagNETTVHighAltitudeSeatbeltCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 高空安全带识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S, *LPNET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S;

/**
 * @struct tagNETTVSafetyHelmetCfg
 * @brief 安全帽识别配置信息 Safety helmet detection configuration
 * @note 用于NET_TV_GET_SAFETY_HELMET_CFG/NET_TV_SET_SAFETY_HELMET_CFG
 */
typedef struct tagNETTVSafetyHelmetCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 安全帽识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_SAFETY_HELMET_CFG_S, *LPNET_TV_SAFETY_HELMET_CFG_S;

/**
 * @struct tagNETTVPersonFallCfg
 * @brief 摔倒识别配置信息 Person fall detection configuration
 * @note 用于NET_TV_GET_PERSON_FALL_CFG/NET_TV_SET_PERSON_FALL_CFG
 */
typedef struct tagNETTVPersonFallCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 摔倒识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_PERSON_FALL_CFG_S, *LPNET_TV_PERSON_FALL_CFG_S;

/**
 * @struct tagNETTVPhoneUsageCfg
 * @brief 玩手机识别配置信息 Phone usage detection configuration
 * @note 用于NET_TV_GET_PHONE_USAGE_CFG/NET_TV_SET_PHONE_USAGE_CFG
 */
typedef struct tagNETTVPhoneUsageCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 玩手机识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_PHONE_USAGE_CFG_S, *LPNET_TV_PHONE_USAGE_CFG_S;

/**
 * @struct tagNETTVSmokingCfg
 * @brief 抽烟识别配置信息 Smoking detection configuration
 * @note 用于NET_TV_GET_SMOKING_CFG/NET_TV_SET_SMOKING_CFG
 */
typedef struct tagNETTVSmokingCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 抽烟识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_SMOKING_CFG_S, *LPNET_TV_SMOKING_CFG_S;

/**
 * @struct tagNETTVOpenFlameCfg
 * @brief 明火识别配置信息 Open flame detection configuration
 * @note 用于NET_TV_GET_OPEN_FLAME_CFG/NET_TV_SET_OPEN_FLAME_CFG
 */
typedef struct tagNETTVOpenFlameCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 明火识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_OPEN_FLAME_CFG_S, *LPNET_TV_OPEN_FLAME_CFG_S;

/**
 * @struct tagNETTVBareSoilCfg
 * @brief 黄土裸露识别配置信息 Bare soil detection configuration
 * @note 用于NET_TV_GET_BARE_SOIL_CFG/NET_TV_SET_BARE_SOIL_CFG
 */
typedef struct tagNETTVBareSoilCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 黄土裸露识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_BARE_SOIL_CFG_S, *LPNET_TV_BARE_SOIL_CFG_S;

/**
 * @struct tagNETTVHoleProtectionBarCfg
 * @brief 洞口防护栏识别配置信息 Hole protection bar detection configuration
 * @note 用于NET_TV_GET_HOLE_PROTECTION_BAR_CFG/NET_TV_SET_HOLE_PROTECTION_BAR_CFG
 */
typedef struct tagNETTVHoleProtectionBarCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 洞口防护栏识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_HOLE_PROTECTION_BAR_CFG_S, *LPNET_TV_HOLE_PROTECTION_BAR_CFG_S;

/**
 * @struct tagNETTVReflectiveClothingCfg
 * @brief 反光衣识别配置信息 Reflective clothing detection configuration
 * @note 用于NET_TV_GET_REFLECTIVE_CLOTHING_CFG/NET_TV_SET_REFLECTIVE_CLOTHING_CFG
 */
typedef struct tagNETTVReflectiveClothingCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 反光衣识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_REFLECTIVE_CLOTHING_CFG_S, *LPNET_TV_REFLECTIVE_CLOTHING_CFG_S;

/* ==================== 智能事件配置相关结构体 ==================== */

/**
 * @struct tagNETTVSmartRegion
 * @brief 智能事件检测区域 Smart event detection region
 */
typedef struct tagNETTVSmartRegion
{
    INT32       dwPointCount;                          /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                          /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                          /* 区域顶点Y坐标数组 [0.0-1.0] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_TV_SMART_REGION_S, *LPNET_TV_SMART_REGION_S;

/**
 * @struct tagNETTVSmartRegionRule
 * @brief 智能事件区域规则参数 Smart event region rule
 */
typedef struct tagNETTVSmartRegionRule
{
    BOOL        bEnable;                               /* 是否启用 */
    INT32       dwPointCount;                          /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                          /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                          /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nTimeThreshold;                        /* 行为事件触发时间阈值，判断有效报警的时间[0,100] 单位秒 */
    INT32       nSensitivity;                          /* 灵敏度[1,100] */
    INT32       dwDetectionTargetCount;                /* 检测目标数量 */
    INT32       adwDetectionTarget[8];                 /* 检测目标数组 NET_TV_DETECTION_TARGET_E，最多8个 */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_TV_SMART_REGION_RULE_S, *LPNET_TV_SMART_REGION_RULE_S;

/**
 * @struct tagNETTVSmartLineRule
 * @brief 智能事件警戒线规则参数 Smart event line rule
 */
typedef struct tagNETTVSmartLineRule
{
    BOOL        bEnable;                               /* 是否启用 */
    FLOAT       fStartPosX;                            /* 警戒线起始点X坐标 [0.0-1.0] */
    FLOAT       fStartPosY;                            /* 警戒线起始点Y坐标 [0.0-1.0] */
    FLOAT       fEndPosX;                              /* 警戒线终止点X坐标 [0.0-1.0] */
    FLOAT       fEndPosY;                              /* 警戒线终止点Y坐标 [0.0-1.0] */
    INT32       enCrossDirection;                      /* 警戒线的穿越方向 NET_TV_CROSS_DIRECTION_E */
    INT32       nSensitivity;                          /* 警戒线灵敏度[1,100] */
    BYTE        byRes[64];                             /* 保留字段 */
}NET_TV_SMART_LINE_RULE_S, *LPNET_TV_SMART_LINE_RULE_S;

/**
 * @struct tagNETTVPetRecognitionInfo
 * @brief 宠物识别配置信息 Pet recognition configuration
 * @note 用于NET_TV_GET_PET_RECOGNITION_INFO/NET_TV_SET_PET_RECOGNITION_INFO
 */
typedef struct tagNETTVPetRecognitionInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    BOOL        bDynamicAnalysisEnable;                /* 是否启用动态分析 */
    INT32       nSensitivity;                          /* 灵敏度[1,100] */
    NET_TV_SMART_REGION_S stRegion;                    /* 宠物识别检测区域 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_PET_RECOGNITION_INFO_S, *LPNET_TV_PET_RECOGNITION_INFO_S;

/**
 * @struct tagNETTVClimbFenceInfo
 * @brief 翻越围栏配置信息 Climb fence detection configuration
 * @note 用于NET_TV_GET_CLIMB_FENCE_INFO/NET_TV_SET_CLIMB_FENCE_INFO
 */
typedef struct tagNETTVClimbFenceInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                           /* 规则数量，最多4个 */
    NET_TV_SMART_REGION_RULE_S astRule[4];             /* 翻越围栏检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_CLIMB_FENCE_INFO_S, *LPNET_TV_CLIMB_FENCE_INFO_S;

/**
 * @struct tagNETTVDimissionInfo
 * @brief 离岗配置信息 Leave post detection configuration
 * @note 用于NET_TV_GET_DIMISSION_INFO/NET_TV_SET_DIMISSION_INFO
 */
typedef struct tagNETTVDimissionInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                           /* 规则数量，最多4个 */
    NET_TV_SMART_REGION_RULE_S astRule[4];             /* 离岗检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_DIMISSION_INFO_S, *LPNET_TV_DIMISSION_INFO_S;

/**
 * @struct tagNETTVIllegalLaneInfo
 * @brief 违规变道配置信息 Illegal lane change detection configuration
 * @note 用于NET_TV_GET_ILLEGAL_LANE_INFO/NET_TV_SET_ILLEGAL_LANE_INFO
 */
typedef struct tagNETTVIllegalLaneInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                           /* 规则数量，最多4个 */
    NET_TV_SMART_LINE_RULE_S astRule[4];               /* 违规变道检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_ILLEGAL_LANE_INFO_S, *LPNET_TV_ILLEGAL_LANE_INFO_S;

/**
 * @struct tagNETTVRetrogradeInfo
 * @brief 逆行配置信息 Retrograde detection configuration
 * @note 用于NET_TV_GET_RETROGRADE_INFO/NET_TV_SET_RETROGRADE_INFO
 */
typedef struct tagNETTVRetrogradeInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                           /* 规则数量，最多4个 */
    NET_TV_SMART_LINE_RULE_S astRule[4];               /* 逆行检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_RETROGRADE_INFO_S, *LPNET_TV_RETROGRADE_INFO_S;

/**
 * @struct tagNETTVNonmotorVehicleIntrusionInfo
 * @brief 非机动车闯入配置信息 Non-motor vehicle intrusion detection configuration
 * @note 用于NET_TV_GET_NONMOTOR_VEHICLE_INTRUSION_INFO/NET_TV_SET_NONMOTOR_VEHICLE_INTRUSION_INFO
 */
typedef struct tagNETTVNonmotorVehicleIntrusionInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                           /* 规则数量，最多4个 */
    NET_TV_SMART_REGION_RULE_S astRule[4];             /* 非机动车闯入检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S, *LPNET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S;

/**
 * @struct tagNETTVOccupationEmergencyInfo
 * @brief 应急车道占用识别配置信息 Emergency lane occupancy detection configuration
 * @note 用于NET_TV_GET_OCCUPATION_EMERGENCY_INFO/NET_TV_SET_OCCUPATION_EMERGENCY_INFO
 */
typedef struct tagNETTVOccupationEmergencyInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                           /* 规则数量，最多4个 */
    NET_TV_SMART_REGION_RULE_S astRule[4];             /* 应急车道占用检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_OCCUPATION_EMERGENCY_INFO_S, *LPNET_TV_OCCUPATION_EMERGENCY_INFO_S;

/**
 * @struct tagNETTVPedestrianIntrusionInfo
 * @brief 行人闯入配置信息 Pedestrian intrusion detection configuration
 * @note 用于NET_TV_GET_PEDESTRIAN_INTRUSION_INFO/NET_TV_SET_PEDESTRIAN_INTRUSION_INFO
 */
typedef struct tagNETTVPedestrianIntrusionInfo
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                           /* 规则数量，最多4个 */
    NET_TV_SMART_REGION_RULE_S astRule[4];             /* 行人闯入检测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_PEDESTRIAN_INTRUSION_INFO_S, *LPNET_TV_PEDESTRIAN_INTRUSION_INFO_S;

/**
 * @struct tagNETTVSmokeFireCfg
 * @brief 烟火识别配置信息 Smoke fire detection configuration
 * @note 用于NET_TV_GET_SMOKE_FIRE_CFG/NET_TV_SET_SMOKE_FIRE_CFG
 */
typedef struct tagNETTVSmokeFireCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 烟火识别规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_SMOKE_FIRE_CFG_S, *LPNET_TV_SMOKE_FIRE_CFG_S;

/**
 * @struct tagNETTVRoadPondingCfg
 * @brief 道路积水检测配置信息 Road ponding detection configuration
 * @note 用于NET_TV_GET_ROAD_PONDING_CFG/NET_TV_SET_ROAD_PONDING_CFG
 */
typedef struct tagNETTVRoadPondingCfg
{
    BOOL        bEnable;                               /* 是否启用 0-不启用 1-启用 */
    NET_TV_AI_SIMPLE_RULE_S stRule;                    /* 道路积水检测规则 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;           /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;               /* 联动配置 */
    BYTE        byRes[256];                            /* 保留字段 */
}NET_TV_ROAD_PONDING_CFG_S, *LPNET_TV_ROAD_PONDING_CFG_S;

/**
 * @struct tagNETTVUnattendedObjectRule
 * @brief 物品遗留侦测区域规则参数 Unattended object detection region rule
 */
typedef struct tagNETTVUnattendedObjectRule
{
    INT32       dwPointCount;                        /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，单位秒 [12,100] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_UNATTENDED_OBJECT_RULE_S, *LPNET_TV_UNATTENDED_OBJECT_RULE_S;

/**
 * @struct tagNETTVUnattendedObjectAlarmInfo
 * @brief 物品遗留侦测告警配置信息 Unattended object detection alarm configuration
 * @note 用于NET_TV_GET_UNATTENDEDOBJECTALARM/NET_TV_SET_UNATTENDEDOBJECTALARM
 */
typedef struct tagNETTVUnattendedObjectAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多4个 */
    NET_TV_UNATTENDED_OBJECT_RULE_S astRule[4];      /* 物品遗留侦测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;         /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;             /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S, *LPNET_TV_UNATTENDED_OBJECT_ALARM_INFO_S;

/**
 * @struct tagNETTVObjectRemovalRule
 * @brief 物品拿取侦测区域规则参数 Object removal detection region rule
 */
typedef struct tagNETTVObjectRemovalRule
{
    INT32       dwPointCount;                        /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    INT32       nSensitivity;                        /* 灵敏度[1,100] */
    INT32       nTimeThreshold;                      /* 行为事件触发时间阈值，单位秒 [12,100] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_OBJECT_REMOVAL_RULE_S, *LPNET_TV_OBJECT_REMOVAL_RULE_S;

/**
 * @struct tagNETTVObjectRemovalAlarmInfo
 * @brief 物品拿取侦测告警配置信息 Object removal detection alarm configuration
 * @note 用于NET_TV_GET_OBJECTREMOVALALARM/NET_TV_SET_OBJECTREMOVALALARM
 */
typedef struct tagNETTVObjectRemovalAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    INT32       dwRuleCount;                         /* 规则数量，最多4个 */
    NET_TV_OBJECT_REMOVAL_RULE_S astRule[4];         /* 物品拿取侦测规则，最多4个 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;         /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;             /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_OBJECT_REMOVAL_ALARM_INFO_S, *LPNET_TV_OBJECT_REMOVAL_ALARM_INFO_S;

/**
 * @struct tagNETTVAudioAnomalyAlarmInfo
 * @brief 音频异常侦测告警配置信息 Audio anomaly detection alarm configuration
 * @note 用于NET_TV_GET_AUDIOANOMALYALARM/NET_TV_SET_AUDIOANOMALYALARM
 */
typedef struct tagNETTVAudioAnomalyAlarmInfo
{
    BOOL        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    BOOL        bAudioInputAnomaly;                  /* 音频输入异常检测是否启用 */
    BOOL        bUpEnable;                           /* 音量突升检测是否启用 */
    INT32       nUpSensitivity;                      /* 音量突升灵敏度 [1,100] */
    INT32       nUpThreshold;                        /* 音量突升阈值 [1,100] */
    BOOL        bDownEnable;                         /* 音量突降检测是否启用 */
    INT32       nDownSensitivity;                    /* 音量突降灵敏度 [1,100] */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;         /* 布防时间 */
    NET_TV_LINKAGE_LIST_S stLinkageList;             /* 联动配置 */
    BYTE        byRes[256];                          /* 保留字段 */
}NET_TV_AUDIO_ANOMALY_ALARM_INFO_S, *LPNET_TV_AUDIO_ANOMALY_ALARM_INFO_S;

/* ==================== FaceCapture人脸抓拍相关结构体 =================== */

/**
 * @struct tagNETTVFaceCaptureRegion
 * @brief 人脸抓拍区域（多边形）
 */
typedef struct tagNETTVFaceCaptureRegion
{
    INT32       dwPointCount;                        /* 区域顶点数量，最多32个 */
    FLOAT       afPointX[32];                        /* 区域顶点X坐标数组 [0.0-1.0] */
    FLOAT       afPointY[32];                        /* 区域顶点Y坐标数组 [0.0-1.0] */
    BYTE        byRes[64];                           /* 保留字段 */
}NET_TV_FACE_CAPTURE_REGION_S, *LPNET_TV_FACE_CAPTURE_REGION_S;

/**
 * @struct tagNETTVFaceCaptureRule
 * @brief 人脸抓拍规则参数 Face capture rule
 */
typedef struct tagNETTVFaceCaptureRule
{
    INT32                           nSensitivity;                    /* 灵敏度[1,100] */
    NET_TV_FACE_CAPTURE_REGION_S    stRegion;                        /* 规则区域 */
    INT32                           dwShieldRegionCount;             /* 屏蔽区域数量，最多4个 */
    NET_TV_FACE_CAPTURE_REGION_S    astShieldRegion[4];              /* 屏蔽区域，最多4个 */
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
}NET_TV_FACE_CAPTURE_RULE_S, *LPNET_TV_FACE_CAPTURE_RULE_S;

/**
 * @struct tagNETTVFaceCaptureInfo
 * @brief 人脸抓拍配置信息 Face capture configuration
 * @note 用于NET_TV_GET_FACECAPTUREINFO/NET_TV_SET_FACECAPTUREINFO
 */
typedef struct tagNETTVFaceCaptureInfo
{
    BOOL                        bEnable;                             /* 是否启用 0-不启用 1-启用 */
    NET_TV_FACE_CAPTURE_RULE_S  stRule;                             /* 人脸抓拍规则 */
    NET_TV_ALARM_SCHEDULE_S     stAlarmSchedule;                    /* 布防时间 */
    NET_TV_LINKAGE_LIST_S       stLinkageList;                      /* 联动配置 */
    BYTE                        byRes[256];                         /* 保留字段 */
}NET_TV_FACE_CAPTURE_INFO_S, *LPNET_TV_FACE_CAPTURE_INFO_S;

/**
 * @struct tagNETTVFaceCompareInfo
 * @brief 人脸比对配置信息 Face compare configuration
 * @note 用于NET_TV_SET_FACE_COMPARE_INFO
 */
typedef struct tagNETTVFaceCompareInfo
{
    BOOL                    bEnable;                    /* 是否启用 0-不启用 1-启用 */
    NET_TV_ALARM_SCHEDULE_S stAlarmSchedule;            /* 布防时间 */
    NET_TV_LINKAGE_LIST_S   stLinkageListSuccess;       /* 比对成功联动配置 */
    NET_TV_LINKAGE_LIST_S   stLinkageListFail;          /* 比对失败联动配置 */
    BYTE                    byRes[256];                 /* 保留字段 */
}NET_TV_FACE_COMPARE_INFO_S, *LPNET_TV_FACE_COMPARE_INFO_S;

/**
 * @struct tagNETTVFaceLibInfo
 * @brief 目标库信息 Face library information
 * @note 用于NET_TV_ADD_TARGET_LIB/NET_TV_DEL_TARGET_LIB/NET_TV_SET_TARGET_LIB
 */
typedef struct tagNETTVFaceLibInfo
{
    CHAR    szFaceLibName[NET_TV_FACE_DB_NAME_LEN];     /* 目标库名称 */
    INT32   nTotalFace;                                 /* 总人脸数 */
    INT32   nNormalNum;                                 /* 正常个数 */
    INT32   nAbnormalNum;                               /* 异常个数 */
    BYTE    byRes[256];                                 /* 保留字段 */
}NET_TV_FACE_LIB_INFO_S, *LPNET_TV_FACE_LIB_INFO_S;

/**
 * @struct tagNETTVFaceLibList
 * @brief 目标库列表 Face library list
 * @note 用于NET_TV_GET_TARGET_LIB
 */
typedef struct tagNETTVFaceLibList
{
    INT32                   nTargetLibCount;                            /* 目标库数量 */
    NET_TV_FACE_LIB_INFO_S  astTargetLibInfos[NET_TV_FACE_LIB_MAX_NUM]; /* 目标库信息列表 */
    BYTE                    byRes[256];                                 /* 保留字段 */
}NET_TV_FACE_LIB_LIST_S, *LPNET_TV_FACE_LIB_LIST_S;

/**
 * @struct tagNETTVFaceIdInfo
 * @brief 人脸ID信息 Face id information
 * @note 用于NET_TV_DEL_FACE_INFO
 */
typedef struct tagNETTVFaceIdInfo
{
    INT32   nIdCount;                                   /* 人脸ID数量 */
    INT32   anIds[NET_TV_FACE_ID_MAX_NUM];              /* 人脸ID列表 */
    BYTE    byRes[256];                                 /* 保留字段 */
}NET_TV_FACE_ID_INFO_S, *LPNET_TV_FACE_ID_INFO_S;

/**
 * @struct tagNETTVFaceInfo
 * @brief 人脸信息 Face information
 * @note 用于NET_TV_ADD_FACE_INFO/NET_TV_SET_FACE_INFO
 */
typedef struct tagNETTVFaceInfo
{
    INT32   nId;                                        /* 人脸ID */
    CHAR    szFaceLibName[NET_TV_FACE_DB_NAME_LEN];     /* 名单组名称 */
    CHAR    szName[NET_TV_FACE_MEMBER_NAME_LEN];        /* 名字 */
    CHAR    szPhoneNum[NET_TV_FACE_IDNUMBER_LEN];       /* 联系方式 */
    CHAR    szPicPath[NET_TV_LEN_260];                  /* 图片完整路径/名称 */
    CHAR    szBinPath[NET_TV_LEN_260];                  /* 图片二进制完整路径/名称 */
    CHAR    szPicType[NET_TV_LEN_64];                   /* 图片类型 */
    INT32   nPicSize;                                   /* 图片大小 */
    CHAR    szPicDate[NET_TV_LEN_64];                   /* 图片日期 */
    INT32   nModelState;                                /* 模型状态, 0未处理，1成功，-1失败 */
    INT32   nRatingLevel;                               /* 评估等级, 0全部，1评分未知，2低，3高 */
    BYTE    byRes[256];                                 /* 保留字段 */
}NET_TV_FACE_INFO_S, *LPNET_TV_FACE_INFO_S;

/**
 * @struct tagNETTVFaceInfoList
 * @brief 人脸信息列表 Face information list
 * @note 用于NET_TV_GET_FACE_INFO
 */
typedef struct tagNETTVFaceInfoList
{
    INT32               nFaceInfoCount;                         /* 人脸信息数量 */
    NET_TV_FACE_INFO_S  astFaceInfos[NET_TV_FACE_INFO_MAX_NUM]; /* 人脸信息列表 */
    BYTE                byRes[256];                             /* 保留字段 */
}NET_TV_FACE_INFO_LIST_S, *LPNET_TV_FACE_INFO_LIST_S;

/**
 * @struct tagNET_TV_ALARMER
 * @brief 报警设备信息
 */
typedef struct tagNET_TV_ALARMER
{
    LPVOID lpUserID;                            /* NET_TV_Login()返回值, 布防时有效 */
    BYTE szSerialNumber[NET_TV_LEN_64];         /* 序列号 */
    CHAR szDeviceName[NET_TV_LEN_32];           /* 设备名字 */
    BYTE byMacAddr[NET_TV_LEN_6];               /* MAC地址 */    
    CHAR szDeviceIP[128];                       /* IP地址 */
    BYTE byRes1[12];
}NET_TV_ALARMER_S,*LPNET_TV_ALARMER_S;

/**
 * @struct tagNETTVVideoResolution
 * @brief 视频源分辨率信息 Video resolution
 * @attention 无
*/
typedef struct tagNETTVVideoResolution
{
    CHAR  szName[NET_TV_LEN_32];                              /* 分辨率名称, 如 1920*1080 */
    INT32 dwWidth;                                             /*  视频编码分辨率 */
    INT32 dwHeight;                                            /*  视频编码分辨率 */
    FLOAT dwFrameRateMin;                                      /*  该分辨率支持的最小帧率fps */
    FLOAT dwFrameRateMax;                                      /*  该分辨率支持的最大帧率fps */
    INT32 dwFrameRateNum;                                      /*  该分辨率支持的帧率数量 */
    FLOAT adwFrameRate[NET_TV_VIDEO_FRAME_RATE_MAX_NUM];       /*  该分辨率支持的帧率fps数组 */
    INT32 dwBitRateMin;                                        /*  该分辨率支持的最小码率kbps */
    INT32 dwBitRateMax;                                        /*  该分辨率支持的最大码率kbps */
}NET_TV_VIDEO_RESOLUTION_S, *LPNET_TV_VIDEO_RESOLUTION_S;

/**
 * @struct tagNETTVRange
 * @brief 取值范围 Range
 * @attention 无
*/
typedef struct tagNETTVRange
{
    INT32   dwMin;                         /* 最小值 */
    INT32   dwMax;                         /* 最大值 */
}NET_TV_RANGE_S, *LPNET_TV_RANGE_S;

/**
 * @struct tagNET_TVVideoEncodeOption
 * @brief 视频编码参数选项 Video encode option
 * @attention 无
*/
typedef struct tagNET_TVVideoEncodeOption
{
    INT32                       nId;                                                /* 视频码流ID 0-主码流 1-子码流 2-JPEG */
    INT32                       enVideoType;                                       /* 视频类型 0-复合流 1-视频流 */
    NET_TV_VIDEO_RESOLUTION_S   stVideoResolution;                                 /* 视频分辨率 */
    INT32                       enBitrateType;                                     /* 码率类型 */
    INT32                       enImageQuality;                                    /* 图像质量 */
    INT32                       enFrameRate;                                       /* 视频帧率fps */
    INT32                       nBitrateUpperLimit;                                /* 码率上限kbps */
    INT32                       nAverageBitrate;                                   /* 平均码率kbps */
    INT32                       enVideoCodec;                                      /* 视频编码 NET_TV_VIDEO_CODE_TYPE_E */
    BOOL                        bSmartEnable;                                      /* 智能编码 */
    INT32                       enEncodingComplexity;                              /* 编码复杂度 */
    INT32                       nIFrameInterval;                                   /* I帧间隔 */
    INT32                       enSvcEnable;                                       /* SVC智能编码 */
    INT32                       nBitrateSmoothing;                                 /* 码流平滑 */
    BYTE                        byRes[256];                                        /* 保留字段 */
}NET_TV_VIDEO_ENCODE_OPTION_S, *LPNET_TV_VIDEO_ENCODE_OPTION_S;

/**
 * @struct tagNET_TVVideoEncodeAbility
 * @brief 单个编码格式能力 Video encode ability
 * @attention 对应 Video_NS::EncodeAbility_S
*/
typedef struct tagNET_TVVideoEncodeAbility
{
    CHAR                        szVideoCodec[NET_TV_LEN_32];                       /* 视频编码字符串, 如 H.264/H.265 */
    INT32                       enVideoCodec;                                      /* 视频编码 NET_TV_VIDEO_CODE_TYPE_E */
    INT32                       nSupportAdjustComplexity;                          /* 是否支持调整编码复杂度 */
    INT32                       anEncodeComplexity[NET_TV_VIDEO_ENCODE_COMPLEXITY_MAX_NUM]; /* 支持的编码复杂度 */
    INT32                       nEncodeComplexityNum;                              /* 编码复杂度有效个数 */
    UINT32                      nDefaultComplexity;                                /* 默认编码复杂度 */
    INT32                       bSupportSVC;                                       /* 是否支持 SVC */
    INT32                       bSupportStreamSmooth;                              /* 是否支持码流平滑 */
    BYTE                        byRes[64];                                         /* 保留字段 */
}NET_TV_VIDEO_ENCODE_ABILITY_S, *LPNET_TV_VIDEO_ENCODE_ABILITY_S;

/**
 * @struct tagNET_TVVideoStreamCap
 * @brief 视频码流参数能力集 Video stream CapNET_TV_CAP_OSD
 * @attention 无
*/
typedef struct tagNET_TVVideoStreamCap
{
    INT32                           dwStreamType;                                   /* 码流类型 入参 参见 NET_TV_LIVES_TREAM_INDEX_E */
    INT32                           bSupportMultiStream;                            /* 是否支持复合流(包含音频) Support multi stream */
    INT32                           dwEncodeCapSize;                                /* 编码能力集个数 Encode capability size */
    NET_TV_VIDEO_ENCODE_OPTION_S    astEncodeCap[NET_TV_VIDEO_ENCODE_TYPE_MAX];     /* 编码能力 Encode capability */
    NET_TV_RANGE_S                  stQuality;                                      /* 图像质量范围 Quality range */
    NET_TV_RANGE_S                  stStreamSmooth;                                 /* 码流平滑范围 Stream smooth range */
    INT32                           dwResolutionNum;                                /* 支持的分辨率个数 Number of supported resolutions */
    NET_TV_VIDEO_RESOLUTION_S       astResolution[NET_TV_RESOLUTION_NUM_MAX];       /* 支持的分辨率列表 Supported resolution list */
    INT32                           dwEncodeTypeNum;                                /* 编码格式有效个数 */
    INT32                           dwEncodeAbilityNum;                             /* 编码能力有效个数 */
    NET_TV_VIDEO_ENCODE_ABILITY_S   astEncodeAbility[NET_TV_VIDEO_ENCODE_TYPE_MAX]; /* 编码格式能力列表 */
    INT32                           dwIFrameIntervalMin;                            /* I帧间隔最小值 */
    INT32                           dwIFrameIntervalMax;                            /* I帧间隔最大值 */
}NET_TV_VIDEO_STREAM_CAP_S, *LPNET_TV_VIDEO_STREAM_CAP_S;

#define NET_TV_VIDEO_STREAM_MAX         4             /* 最大码流数量 */

/**
 * @struct tagNETTVVideoEncodeCap
 * @brief 视频编码能力集(多码流) Video Encode Capability
 * @attention 包含所有码流的能力集信息
*/
typedef struct tagNETTVVideoEncodeCap
{
    INT32                       dwStreamCount;                                  /* 码流数量 Stream count */
    NET_TV_VIDEO_STREAM_CAP_S   astStreamCap[NET_TV_VIDEO_STREAM_MAX];          /* 各码流能力 Stream capabilities */
}NET_TV_VIDEO_ENCODE_CAP_S, *LPNET_TV_VIDEO_ENCODE_CAP_S;

/* ==================== AUDIO能力结构体Start =================== */

/**
 * @struct tagNETTVAudioRange
 * @brief 音频数值范围 Audio range
 * @attention 无
 */
typedef struct tagNETTVAudioRange
{
    INT32   bEnable;    /* 是否启用范围约束，0-不校验，1-校验 */
    INT32   dwMin;      /* 最小值 */
    INT32   dwMax;      /* 最大值 */
    INT32   dwStep;     /* 步长，<=0 表示不做步长校验 */
}NET_TV_AUDIO_RANGE_S, *LPNET_TV_AUDIO_RANGE_S;

/**
 * @struct tagNETTVAudioFormatCap
 * @brief 音频格式能力集 Audio format capability
 * @attention 无
 */
typedef struct tagNETTVAudioFormatCap
{
    INT32                   dwFormat;                               /* 音频格式 参见 NET_TV_AUDIO_FORMAT_E */
    INT32                   dwSampleRateSize;                       /* 采样率数量 */
    INT32                   adwSampleRate[NET_TV_AUDIO_SAMPRATE_MAX]; /* 采样率列表 参见 NET_TV_AUDIO_SAMPRATE_E */

    INT32                   dwBitRateSize;                          /* 码率数量 */
    INT32                   adwBitRate[NET_TV_AUDIO_BITRATE_MAX];  /* 码率列表 参见 NET_TV_AUDIO_BITRATE_E */

    NET_TV_AUDIO_RANGE_S    stSampleRateRange;                      /* 采样率范围（预留） */
    NET_TV_AUDIO_RANGE_S    stBitRateRange;                         /* 码率范围（预留） */
}NET_TV_AUDIO_FORMAT_CAP_S, *LPNET_TV_AUDIO_FORMAT_CAP_S;

/**
 * @struct tagNETTVAudioEncodeCap
 * @brief 音频编码能力集 Audio encode capability
 * @attention 无
 */
typedef struct tagNETTVAudioEncodeCap
{
    INT32                       dwInputTypeSize;                           /* 输入类型数量 */
    INT32                       adwInputType[NET_TV_AUDIO_INPUT_TYPE_MAX];   /* 输入类型 参见 NET_TV_AUDIO_INPUT_TYPE_E */

    INT32                       dwOutputTypeSize;                          /* 输出类型数量 */
    INT32                       adwOutputType[NET_TV_AUDIO_OUTPUT_TYPE_MAX]; /* 输出类型 参见 NET_TV_AUDIO_OUTPUT_TYPE_E */

    INT32                       dwFormatSize;                              /* 音频格式数量 */
    INT32                       adwFormat[NET_TV_AUDIO_FORMAT_MAX];        /* 音频格式 参见 NET_TV_AUDIO_FORMAT_E */

    INT32                       dwFormatDetailSize;                        /* 音频格式详细能力数量 */
    NET_TV_AUDIO_FORMAT_CAP_S   astFormatDetail[NET_TV_AUDIO_FORMAT_MAX];  /* 各音频格式详细能力 */
}NET_TV_AUDIO_CAP_S, *LPNET_TV_AUDIO_CAP_S;

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
    CHAR strFontColor[NET_TV_LEN_16];       /* OSD颜色为自定义时，使用，记录RGB颜色 格式:"#000000"" */
    CHAR strToken[NET_TV_LEN_512];          /* OSD token Onvif使用 */
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
    CHAR strName[NET_TV_LEN_128];           /* 名称 */
    OsdAttribute_S stOsdAttr;               /* OSD状态信息 */
} OsdNameInfo_S;
/* OSD字符叠加信息 */
typedef struct _OsdInfo_S_
{
    INT32 nId;                              /* 字符ID */
    BOOL bEnable;                           /* 是否启用 */
    CHAR strName[NET_TV_LEN_128];           /* 字符串名称 */
    OsdAttribute_S stOsdAttr;               /* OSD状态信息 */
} OsdInfo_S;
/* OSD配置信息 */
typedef struct tagNETTVObsConfigInfo
{
    OSD_ALIGN_E enAlign;                            /* OSD对齐方式 */
    OsdNameInfo_S stOsdNameInfo;                    /* OSD名称信息 */
    OsdTimeInfo_S stOsdTimeInfo;                    /* OSD时间信息 */
    OsdInfo_S OsdInfo[32];                          /* OSD字符叠加信息 */
    BYTE        byRes[64];                          /* 保留字段 */
}NET_TV_VIDEO_OSD_CFG_S, *LPNET_TV_VIDEO_OSD_CFG_S;

/* ==================== OSD相关结构体End ===================== */

/**
 * @struct tagNETTVOsdCap
 * @brief 通道OSD的能力集 OSD Capabilities (简化版，对应OsdConfig_S)
 * @attention
 */
typedef struct tagNETTVOsdCap
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
    UINT32   audwSupportedFontSizeList[NET_TV_OSD_FONT_SIZE_TYPE_MAX_NUM];      /* 支持的字体大小列表 NET_TV_OSD_FONT_SIZE_E */
    
    /* 日期格式能力 */
    UINT32   udwSupportedDateFormatNum;                                         /* 支持的日期格式数量 */
    UINT32   audwSupportedDateFormatList[NET_TV_OSD_DATE_FORMAT_MAX_NUM];       /* 支持的日期格式列表 NET_TV_OSD_DATE_FORMAT_E */
    
    /* 时间格式能力 */
    UINT32   udwSupportedTimeFormatNum;                                         /* 支持的时间格式数量 */
    UINT32   audwSupportedTimeFormatList[NET_TV_OSD_TIME_FORMAT_MAX_NUM];       /* 支持的时间格式列表 NET_TV_OSD_TIME_FORMAT_E */
    
    /* 对齐方式能力 */
    UINT32   udwSupportedAlignNum;                                              /* 支持的对齐方式数量 */
    UINT32   audwSupportedAlignList[8];                                         /* 支持的对齐方式列表 NET_TV_OSD_ALIGN_E */
    
    BYTE     byRes[256];                                                        /* 保留字段  Reserved */
} NET_TV_OSD_CAP_S, *LPNET_TV_OSD_CAP_S;

/******************** 智能能力集结构体定义 Smart Capability Structures ********************/

/**
 * @struct tagNETTVMotionDetectCap
 * @brief 移动侦测能力  Motion detection capability
 * @attention
 */
typedef struct tagNETTVMotionDetectCap
{
    BOOL    bSupport;                           /* 是否支持移动侦测  Support motion detection */
    BOOL    bSupportExpertMode;                 /* 是否支持专家模式  Support expert mode */
    BOOL    bSupportDynamicAnalysis;            /* 是否支持动态分析  Support dynamic analysis */
    UINT32  udwGridMaxWidth;                    /* 网格最大宽度  Grid max width */
    UINT32  udwGridMaxHeight;                   /* 网格最大高度  Grid max height */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportRectRegion;                 /* 是否支持矩形区域  Support rectangle region */
    BOOL    bSupportGridRegion;                 /* 是否支持网格区域  Support grid region */
    UINT32  udwExpertMaxAreas;                  /* 专家模式最大区域数  Expert mode max areas */
    BOOL    bSupportDayNightSwitch;             /* 是否支持日夜切换  Support day night switch */
    BOOL    bSupportAutoSwitch;                 /* 是否支持自动切换  Support auto switch */
    BOOL    bSupportTimedSwitch;                /* 是否支持定时切换  Support timed switch */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_MOTION_DETECT_CAP_S, *LPNET_TV_MOTION_DETECT_CAP_S;

/**
 * @struct tagNETTVTamperDetectCap
 * @brief 遮挡检测能力  Tamper detection capability
 * @attention
 */
typedef struct tagNETTVTamperDetectCap
{
    BOOL    bSupport;                           /* 是否支持遮挡检测  Support tamper detection */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportCustomRegion;               /* 是否支持自定义区域  Support custom region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_TAMPER_DETECT_CAP_S, *LPNET_TV_TAMPER_DETECT_CAP_S;

/**
 * @struct tagNETTVBoundaryDetectCap
 * @brief 越界检测能力  Boundary detection capability
 * @attention
 */
typedef struct tagNETTVBoundaryDetectCap
{
    BOOL    bSupport;                           /* 是否支持越界检测  Support boundary detection */
    UINT32  udwMaxLines;                        /* 最大警戒线数  Max lines */
    UINT32  udwMaxPointsPerLine;                /* 每条线最大顶点数  Max points per line */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportDirection;                  /* 是否支持方向检测  Support direction */
    BOOL    bSupportTargetFilter;               /* 是否支持目标过滤  Support target filter */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_BOUNDARY_DETECT_CAP_S, *LPNET_TV_BOUNDARY_DETECT_CAP_S;

/**
 * @struct tagNETTVIntrusionDetectCap
 * @brief 区域入侵检测能力  Intrusion detection capability
 * @attention
 */
typedef struct tagNETTVIntrusionDetectCap
{
    BOOL    bSupport;                           /* 是否支持区域入侵检测  Support intrusion detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    UINT32  udwMinPointsPerRegion;              /* 每个区域最小顶点数  Min points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_TV_RANGE_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BOOL    bSupportTargetFilter;               /* 是否支持目标过滤  Support target filter */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_INTRUSION_DETECT_CAP_S, *LPNET_TV_INTRUSION_DETECT_CAP_S;

/**
 * @struct tagNETTVEnterExitDetectCap
 * @brief 进入/离开区域检测能力  Enter/Exit detection capability
 * @attention
 */
typedef struct tagNETTVEnterExitDetectCap
{
    BOOL    bSupportEnter;                      /* 是否支持进入区域检测  Support enter detection */
    BOOL    bSupportExit;                       /* 是否支持离开区域检测  Support exit detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    UINT32  udwMinPointsPerRegion;              /* 每个区域最小顶点数  Min points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_TV_RANGE_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BOOL    bSupportTargetFilter;               /* 是否支持目标过滤  Support target filter */
    BOOL    bSupportConfidence;                 /* 是否支持可信度设置  Support confidence */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_ENTER_EXIT_DETECT_CAP_S, *LPNET_TV_ENTER_EXIT_DETECT_CAP_S;

/**
 * @struct tagNETTVLoiteringDetectCap
 * @brief 徘徊检测能力  Loitering detection capability
 * @attention
 */
typedef struct tagNETTVLoiteringDetectCap
{
    BOOL    bSupport;                           /* 是否支持徘徊检测  Support loitering detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_TV_RANGE_S stTimeThreshold;             /* 徘徊时长范围  Loitering time range */
    BOOL    bSupportTargetFilter;               /* 是否支持目标过滤  Support target filter */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_LOITERING_DETECT_CAP_S, *LPNET_TV_LOITERING_DETECT_CAP_S;

/**
 * @struct tagNETTVCrowdGatheringCap
 * @brief 人群聚集检测能力  Crowd gathering detection capability
 * @attention
 */
typedef struct tagNETTVCrowdGatheringCap
{
    BOOL    bSupport;                           /* 是否支持人群聚集检测  Support crowd gathering detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stObjectOccupancy;           /* 人体面积占比阈值  Object occupancy threshold */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_CROWD_GATHERING_CAP_S, *LPNET_TV_CROWD_GATHERING_CAP_S;

/**
 * @struct tagNETTVParkingDetectCap
 * @brief 停车检测能力  Parking detection capability
 * @attention
 */
typedef struct tagNETTVParkingDetectCap
{
    BOOL    bSupport;                           /* 是否支持停车检测  Support parking detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_TV_RANGE_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_PARKING_DETECT_CAP_S, *LPNET_TV_PARKING_DETECT_CAP_S;

/**
 * @struct tagNETTVObjectChangeDetectCap
 * @brief 物品遗留/移走检测能力  Object left/removal detection capability
 * @attention
 */
typedef struct tagNETTVObjectChangeDetectCap
{
    BOOL    bSupportUnattendedObject;           /* 是否支持物品遗留检测  Support unattended object detection */
    BOOL    bSupportObjectRemoval;              /* 是否支持物品移走检测  Support object removal detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_TV_RANGE_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_OBJECT_CHANGE_DETECT_CAP_S, *LPNET_TV_OBJECT_CHANGE_DETECT_CAP_S;

/**
 * @struct tagNETTVFaceDetectCap
 * @brief 人脸检测能力  Face detection capability
 * @attention
 */
typedef struct tagNETTVFaceDetectCap
{
    BOOL    bSupport;                           /* 是否支持人脸检测  Support face detection */
    BOOL    bSupportDynamicAnalysis;            /* 是否支持动态分析  Support dynamic analysis */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportDetectionRegion;            /* 是否支持检测区域设置  Support detection region */
    UINT32  udwMaxPointsPerRegion;              /* 检测区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_FACE_DETECT_CAP_S, *LPNET_TV_FACE_DETECT_CAP_S;

/**
 * @struct tagNETTVFaceCaptureCap
 * @brief 人脸抓拍能力  Face capture capability
 * @attention
 */
typedef struct tagNETTVFaceCaptureCap
{
    BOOL    bSupport;                           /* 是否支持人脸抓拍  Support face capture */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportDetectionRegion;            /* 是否支持检测区域  Support detection region */
    BOOL    bSupportShieldedRegion;             /* 是否支持屏蔽区域  Support shielded region */
    UINT32  udwMaxShieldedRegions;              /* 最大屏蔽区域数  Max shielded regions */
    BOOL    bSupportIPD;                        /* 是否支持瞳距设置  Support IPD setting */
    NET_TV_RANGE_S stMinIPD;                    /* 最小瞳距范围  Min IPD range */
    NET_TV_RANGE_S stMaxIPD;                    /* 最大瞳距范围  Max IPD range */
    NET_TV_RANGE_S stCaptureInterval;           /* 抓拍间隔范围  Capture interval range */
    BOOL    bSupportOverlay;                    /* 是否支持叠加信息  Support overlay */
    BOOL    bSupportFaceAttribute;              /* 是否支持人脸属性  Support face attribute */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_FACE_CAPTURE_CAP_S, *LPNET_TV_FACE_CAPTURE_CAP_S;

/**
 * @struct tagNETTVPetRecognitionCap
 * @brief 宠物识别能力  Pet recognition capability
 * @attention
 */
typedef struct tagNETTVPetRecognitionCap
{
    BOOL    bSupport;                           /* 是否支持宠物识别  Support pet recognition */
    BOOL    bSupportDynamicAnalysis;            /* 是否支持动态分析  Support dynamic analysis */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportDetectionRegion;            /* 是否支持检测区域设置  Support detection region */
    UINT32  udwMaxPointsPerRegion;              /* 检测区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_PET_RECOGNITION_CAP_S, *LPNET_TV_PET_RECOGNITION_CAP_S;

/**
 * @struct tagNETTVAudioAnomalyCap
 * @brief 音频异常检测能力  Audio anomaly detection capability
 * @attention
 */
typedef struct tagNETTVAudioAnomalyCap
{
    BOOL    bSupport;                           /* 是否支持音频异常检测  Support audio anomaly detection */
    BOOL    bSupportInputAnomaly;               /* 是否支持输入异常检测  Support input anomaly */
    BOOL    bSupportRise;                       /* 是否支持音量突升检测  Support rise detection */
    BOOL    bSupportFall;                       /* 是否支持音量突降检测  Support fall detection */
    NET_TV_RANGE_S stRiseSensitivity;           /* 突升灵敏度范围  Rise sensitivity range */
    NET_TV_RANGE_S stRiseThreshold;             /* 突升阈值范围  Rise threshold range */
    NET_TV_RANGE_S stFallSensitivity;           /* 突降灵敏度范围  Fall sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_AUDIO_ANOMALY_CAP_S, *LPNET_TV_AUDIO_ANOMALY_CAP_S;

/**
 * @struct tagNETTVSceneChangeCap
 * @brief 场景变更检测能力  Scene change detection capability
 * @attention
 */
typedef struct tagNETTVSceneChangeCap
{
    BOOL    bSupport;                           /* 是否支持场景变更检测  Support scene change detection */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_SCENE_CHANGE_CAP_S, *LPNET_TV_SCENE_CHANGE_CAP_S;

/**
 * @struct tagNETTVFireDetectCap
 * @brief 火灾检测能力  Fire detection capability
 * @attention
 */
typedef struct tagNETTVFireDetectCap
{
    BOOL    bSupport;                           /* 是否支持火灾检测  Support fire detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_FIRE_DETECT_CAP_S, *LPNET_TV_FIRE_DETECT_CAP_S;

/**
 * @struct tagNETTVSmokeDetectCap
 * @brief 烟雾检测能力  Smoke detection capability
 * @attention
 */
typedef struct tagNETTVSmokeDetectCap
{
    BOOL    bSupport;                           /* 是否支持烟雾检测  Support smoke detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_SMOKE_DETECT_CAP_S, *LPNET_TV_SMOKE_DETECT_CAP_S;

/**
 * @struct tagNETTVWaterAccumulationCap
 * @brief 积水检测能力  Water accumulation detection capability
 * @attention
 */
typedef struct tagNETTVWaterAccumulationCap
{
    BOOL    bSupport;                           /* 是否支持积水检测  Support water accumulation detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_WATER_ACCUMULATION_CAP_S, *LPNET_TV_WATER_ACCUMULATION_CAP_S;

/**
 * @struct tagNETTVTrashOverflowCap
 * @brief 垃圾满溢检测能力  Trash overflow detection capability
 * @attention
 */
typedef struct tagNETTVTrashOverflowCap
{
    BOOL    bSupport;                           /* 是否支持垃圾满溢检测  Support trash overflow detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_TRASH_OVERFLOW_CAP_S, *LPNET_TV_TRASH_OVERFLOW_CAP_S;

/**
 * @struct tagNETTVBehaviorDetectCap
 * @brief 人员行为检测能力  Personnel behavior detection capability
 * @attention
 */
typedef struct tagNETTVBehaviorDetectCap
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
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    NET_TV_RANGE_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_BEHAVIOR_DETECT_CAP_S, *LPNET_TV_BEHAVIOR_DETECT_CAP_S;

/**
 * @struct tagNETTVEnvironmentAnomalyCap
 * @brief 环境异常检测能力  Environment anomaly detection capability
 * @attention
 */
typedef struct tagNETTVEnvironmentAnomalyCap
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
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_ENVIRONMENT_ANOMALY_CAP_S, *LPNET_TV_ENVIRONMENT_ANOMALY_CAP_S;

/**
 * @struct tagNETTVSafetyEquipmentCap
 * @brief 穿戴规范检测能力  Safety equipment detection capability
 * @attention
 */
typedef struct tagNETTVSafetyEquipmentCap
{
    BOOL    bSupport;                           /* 是否支持穿戴规范检测  Support safety equipment detection */
    BOOL    bSupportHelmet;                     /* 是否支持安全帽检测  Support helmet detection */
    BOOL    bSupportReflectiveClothing;         /* 是否支持反光衣检测  Support reflective clothing detection */
    BOOL    bSupportSeatbelt;                   /* 是否支持高空安全带检测  Support seatbelt detection */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stSensitivity;               /* 灵敏度范围  Sensitivity range */
    BOOL    bSupportColorDetect;                /* 是否支持颜色检测  Support color detection */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_SAFETY_EQUIPMENT_CAP_S, *LPNET_TV_SAFETY_EQUIPMENT_CAP_S;

/**
 * @struct tagNETTVLicensePlateCap
 * @brief 车牌识别能力  License plate recognition capability
 * @attention
 */
typedef struct tagNETTVLicensePlateCap
{
    BOOL    bSupport;                           /* 是否支持车牌识别  Support license plate recognition */
    BOOL    bSupportRegion;                     /* 是否支持区域设置  Support region setting */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BOOL    bSupportMultiPlate;                 /* 是否支持多车牌识别  Support multi-plate recognition */
    UINT32  udwMaxPlatesPerFrame;               /* 单帧最大车牌数  Max plates per frame */
    NET_TV_RANGE_S stConfidence;                /* 置信度范围  Confidence range */
    BOOL    bSupportPlateColor;                 /* 是否支持车牌颜色识别  Support plate color recognition */
    BOOL    bSupportPlateType;                  /* 是否支持车牌类型识别  Support plate type recognition */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_LICENSE_PLATE_CAP_S, *LPNET_TV_LICENSE_PLATE_CAP_S;

/**
 * @struct tagNETTVWrongWayDrivingCap
 * @brief 逆行检测能力  Wrong way driving detection capability
 * @attention
 */
typedef struct tagNETTVWrongWayDrivingCap
{
    BOOL    bSupport;                           /* 是否支持逆行检测  Support wrong way driving detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_WRONG_WAY_DRIVING_CAP_S, *LPNET_TV_WRONG_WAY_DRIVING_CAP_S;

/**
 * @struct tagNETTVIllegalLaneChangeCap
 * @brief 违规变道检测能力  Illegal lane change detection capability
 * @attention
 */
typedef struct tagNETTVIllegalLaneChangeCap
{
    BOOL    bSupport;                           /* 是否支持违规变道检测  Support illegal lane change detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_ILLEGAL_LANE_CHANGE_CAP_S, *LPNET_TV_ILLEGAL_LANE_CHANGE_CAP_S;

/**
 * @struct tagNETTVEmergencyLaneOccupancyCap
 * @brief 应急车道占用检测能力  Emergency lane occupancy detection capability
 * @attention
 */
typedef struct tagNETTVEmergencyLaneOccupancyCap
{
    BOOL    bSupport;                           /* 是否支持应急车道占用检测  Support emergency lane occupancy detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S, *LPNET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S;

/**
 * @struct tagNETTVNonMotorVehicleIntrusionCap
 * @brief 非机动车入侵检测能力  Non-motor vehicle intrusion detection capability
 * @attention
 */
typedef struct tagNETTVNonMotorVehicleIntrusionCap
{
    BOOL    bSupport;                           /* 是否支持非机动车入侵检测  Support non-motor vehicle intrusion detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S, *LPNET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S;

/**
 * @struct tagNETTVConstructionOccupancyCap
 * @brief 施工占道检测能力  Construction occupancy detection capability
 * @attention
 */
typedef struct tagNETTVConstructionOccupancyCap
{
    BOOL    bSupport;                           /* 是否支持施工占道检测  Support construction occupancy detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_CONSTRUCTION_OCCUPANCY_CAP_S, *LPNET_TV_CONSTRUCTION_OCCUPANCY_CAP_S;

/**
 * @struct tagNETTVCongestionCap
 * @brief 拥堵检测能力  Congestion detection capability
 * @attention
 */
typedef struct tagNETTVCongestionCap
{
    BOOL    bSupport;                           /* 是否支持拥堵检测  Support congestion detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stDensityThreshold;          /* 密度阈值范围  Density threshold range */
    NET_TV_RANGE_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BOOL    bSupportVehicleCount;               /* 是否支持车辆计数  Support vehicle counting */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_CONGESTION_CAP_S, *LPNET_TV_CONGESTION_CAP_S;

/**
 * @struct tagNETTVIllegalParkingCap
 * @brief 违规停车检测能力  Illegal parking detection capability
 * @attention
 */
typedef struct tagNETTVIllegalParkingCap
{
    BOOL    bSupport;                           /* 是否支持违规停车检测  Support illegal parking detection */
    UINT32  udwMaxRegions;                      /* 最大区域数  Max regions */
    UINT32  udwMaxPointsPerRegion;              /* 每个区域最大顶点数  Max points per region */
    NET_TV_RANGE_S stTimeThreshold;             /* 时间阈值范围  Time threshold range */
    BOOL    bSupportVehicleType;                /* 是否支持车辆类型识别  Support vehicle type recognition */
    BYTE    byRes[64];                          /* 保留字段  Reserved */
} NET_TV_ILLEGAL_PARKING_CAP_S, *LPNET_TV_ILLEGAL_PARKING_CAP_S;

/**
 * @struct tagNETTVSmartCap
 * @brief 智能能力集  Smart capability set
 * @attention 对应 NET_TV_CAP_SMART
 */
typedef struct tagNETTVSmartCap
{
    NET_TV_MOTION_DETECT_CAP_S              stMotionDetect;             /* 移动侦测  Motion detection */
    NET_TV_TAMPER_DETECT_CAP_S              stTamperDetect;             /* 遮挡检测  Tamper detection */
    NET_TV_SCENE_CHANGE_CAP_S               stSceneChange;              /* 场景变更  Scene change */
    NET_TV_AUDIO_ANOMALY_CAP_S              stAudioAnomaly;             /* 音频异常  Audio anomaly */
    NET_TV_BOUNDARY_DETECT_CAP_S            stBoundaryDetect;           /* 越界检测  Boundary detection */
    NET_TV_INTRUSION_DETECT_CAP_S           stIntrusionDetect;          /* 区域入侵  Intrusion detection */
    NET_TV_ENTER_EXIT_DETECT_CAP_S          stEnterExitDetect;          /* 进入/离开区域  Enter/Exit detection */
    NET_TV_LOITERING_DETECT_CAP_S           stLoiteringDetect;          /* 徘徊检测  Loitering detection */
    NET_TV_CROWD_GATHERING_CAP_S            stCrowdGathering;           /* 人群聚集  Crowd gathering */
    NET_TV_PARKING_DETECT_CAP_S             stParkingDetect;            /* 停车检测  Parking detection */
    NET_TV_OBJECT_CHANGE_DETECT_CAP_S       stObjectChange;             /* 物品遗留/移走  Object left/removal */
    NET_TV_FACE_DETECT_CAP_S                stFaceDetect;               /* 人脸检测  Face detection */
    NET_TV_FACE_CAPTURE_CAP_S               stFaceCapture;              /* 人脸抓拍  Face capture */
    NET_TV_PET_RECOGNITION_CAP_S            stPetRecognition;           /* 宠物识别  Pet recognition */
    NET_TV_FIRE_DETECT_CAP_S                stFireDetect;               /* 火灾检测  Fire detection */
    NET_TV_SMOKE_DETECT_CAP_S               stSmokeDetect;              /* 烟雾检测  Smoke detection */
    NET_TV_WATER_ACCUMULATION_CAP_S         stWaterAccumulation;        /* 积水检测  Water accumulation */
    NET_TV_TRASH_OVERFLOW_CAP_S             stTrashOverflow;            /* 垃圾满溢检测  Trash overflow */
    NET_TV_ENVIRONMENT_ANOMALY_CAP_S        stEnvironmentAnomaly;       /* 环境异常检测  Environment anomaly */
    NET_TV_BEHAVIOR_DETECT_CAP_S            stBehaviorDetect;           /* 人员行为检测  Behavior detection */
    NET_TV_SAFETY_EQUIPMENT_CAP_S           stSafetyEquipment;          /* 穿戴规范检测  Safety equipment */
    NET_TV_LICENSE_PLATE_CAP_S              stLicensePlate;             /* 车牌识别  License plate recognition */
    NET_TV_WRONG_WAY_DRIVING_CAP_S          stWrongWayDriving;          /* 逆行检测  Wrong way driving */
    NET_TV_ILLEGAL_LANE_CHANGE_CAP_S        stIllegalLaneChange;        /* 违规变道  Illegal lane change */
    NET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S   stEmergencyLaneOccupancy;   /* 应急车道占用  Emergency lane occupancy */
    NET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S stNonMotorVehicleIntrusion;/* 非机动车入侵  Non-motor vehicle intrusion */
    NET_TV_CONSTRUCTION_OCCUPANCY_CAP_S     stConstructionOccupancy;    /* 施工占道  Construction occupancy */
    NET_TV_CONGESTION_CAP_S                 stCongestion;               /* 拥堵检测  Congestion detection */
    NET_TV_ILLEGAL_PARKING_CAP_S            stIllegalParking;           /* 违规停车  Illegal parking */
    BYTE    byRes[256];                                                 /* 保留字段  Reserved */
} NET_TV_SMART_CAP_S, *LPNET_TV_SMART_CAP_S;




/************************************************************************/
/*                          函数                                  */
/************************************************************************/
/**
 * @brief SDK服务端初始化
 * @param [IN] dwPort 服务器端口号
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 * @note
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_Init(IN UINT32 udwPort,IN CHAR szUserName[NET_TV_LEN_132],IN CHAR szPassword[NET_TV_LEN_132]);

/**
* SDK 清理  SDK cleaning
* @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
* @note
*/
NET_TV_API BOOL STDCALL NET_TV_SERVER_Cleanup(void);

/**
 * @brief 设置日志
 * @param [IN] dwLogLevel   日志的等级（默认为0）：0-表示关闭日志，1-表示只输出ERROR错误日志，2-输出ERROR错误信息和DEBUG调试信息，3-输出ERROR错误信息、DEBUG调试信息和INFO普通信息等所有信息 
 * @param [IN] strLogDir    日志路径
 * @param [IN] dwLogFileSize 日志文件大小(单位：字节)
 * @param [IN] dwLogFileNum 日志文件个数
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_SetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 dwLogFileSize,IN INT32 dwLogFileNum);

/**
* 获取SDK的版本信息 Get SDK version information 
* @return SDK版本信息 SDK version information
* @note
* - 在两个高字节中高8位表示主版本,低八位表示次版本.两个低字节表示附加版本号如0x01080000：表示版本为1.8.0.0.
* - The two high bytes,The high-8-bit indicate the major version, and the low-8-bytes indicate the minor version.Two low bytes for additional version numbers For example, 0x01080000 means version 1.8.0.0
*/
NET_TV_API INT32 STDCALL NET_TV_SERVER_GetSDKVersion(void);

/**
 * @brief 获取当前在线客户端数量（活跃会话数）
 * @return 客户端数量
 */
NET_TV_API INT32 STDCALL NET_TV_SERVER_GetClientCount(void);

/**
 * @brief 设置用户名密码
 * @param [IN] szUserName 用户名
 * @param [IN] szPassword 密码
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_SetUserPasswd(IN CHAR szUserName[NET_TV_LEN_132],IN CHAR szPassword[NET_TV_LEN_132]);

/**
 * @brief 推送告警信息
 * @param [IN] pAlarmer    告警设备信息
 * @param [IN] lCommand    命令码(报警类型)，用于客户端按命令码反序列化结构体
 * @param [IN] pAlarmInfo  具体告警结构体指针（类型由 lCommand 决定）
 * @param [IN] dwBufLen    pAlarmInfo 长度（一般为 sizeof(对应结构体)）
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_PushAlarmInfo(IN NET_TV_ALARMER_S *pAlarmer,
                                                    IN INT32 lCommand,
                                                    IN LPVOID pAlarmInfo,
                                                    IN INT32 dwBufLen);

/**
 * @brief 推送通道上下线状态
 * @param [IN] pChannelInfo 通道信息，byOnline/nDevState 表示当前状态
 * @return TRUE表示成功,其他表示失败 TRUE means success, and any other value means failure.
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_PushChannelStatusInfo(IN NET_TV_CHANNEL_INFO_S *pChannelInfo);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDeviceInfo(NET_TV_COMMON_ECODE_E (*CB)(LPNET_TV_DEVICE_INFO_S pInfo));

/**
 * @brief 视频编码能力集回调类型 (NET_TV_CAP_VIDEO_ENCODE)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         视频编码能力集结构体指针(多码流)
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetVideoEncodeCap)(INT32 dwChannelID, 
                                                             LPNET_TV_VIDEO_ENCODE_CAP_S pCap);

/**
 * @brief 注册视频编码能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetVideoEncodeCap(NET_TV_CB_GetVideoEncodeCap pCb);

/**
 * @brief 音频编码能力集回调类型 (NET_TV_CAP_AUDIO)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         音频编码能力集结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetAudioEncodeCap)(INT32 dwChannelID, 
                                                             LPNET_TV_AUDIO_CAP_S pCap);

/**
 * @brief 注册音频编码能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioEncodeCap(NET_TV_CB_GetAudioEncodeCap pCb);

/**
 * @brief OSD能力集回调类型 (NET_TV_CAP_OSD)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pCap         OSD能力集结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetOsdCap)(INT32 dwChannelID, LPNET_TV_OSD_CAP_S pCap);

/**
 * @brief 注册OSD能力集回调
 * @param [IN] pCb 回调函数指针
 * @return TRUE表示成功,其他表示失败
 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOsdCap(NET_TV_CB_GetOsdCap pCb);

typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpOutBuffer);
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_SetDevConfig)(INT32 dwChannelID,
                                                         INT32 dwCommand,
                                                         LPVOID lpInBuffer);

/* 按命令码注册配置回调，回调参数由命令码对应结构体决定 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpOutBuffer);
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_SetDevConfigByCommand)(INT32 dwChannelID, LPVOID lpInBuffer);

/**
 * @brief 获取RTSP流地址回调类型 (NET_TV_GET_RTSPURLCFG)
 * @param [IN]  dwChannelID  通道号
 * @param [OUT] pInfo        RTSP URL 信息结构体指针
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetRtspUrl)(INT32 dwChannelID, LPNET_TV_RTSP_URL_INFO_S pInfo);

/**
 * @brief 获取回放播放地址回调类型
 * @param [INOUT] pInfo 回放查询条件和播放URL返回信息
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetReplayUrl)(LPNET_TV_REPLAY_URL_INFO_S pInfo);

/**
 * @brief 回放控制回调类型
 * @param [INOUT] pInfo 回放控制输入输出参数
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_ControlReplay)(LPNET_TV_REPLAY_CTRL_INFO_S pInfo);

/**
 * @brief 获取回放录像时间段回调类型
 * @param [INOUT] pInfo 查询条件及结果
 * @return NET_TV_E_SUCCEED 成功, 其他值失败
 */
typedef NET_TV_COMMON_ECODE_E (*NET_TV_CB_GetReplayRecordList)(LPNET_TV_REPLAY_RECORD_LIST_S pInfo);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDevConfig(NET_TV_CB_GetDevConfig pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDevConfig(NET_TV_CB_SetDevConfig pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDeviceCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDeviceCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNtpCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNtpCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetStreamCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetStreamCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRtspUrl(NET_TV_CB_GetRtspUrl pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReplayUrl(NET_TV_CB_GetReplayUrl pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ControlReplay(NET_TV_CB_ControlReplay pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReplayRecordList(NET_TV_CB_GetReplayRecordList pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOsdCapCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOsdCapCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetImageCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetImageCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNetworkCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNetworkCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetConfigWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ConnectWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DisconnectWifiSta(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_Get4GInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_Set4GInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHotspotInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHotspotConn(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSshCountdown(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_FindLog(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ExportLog(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLogServer(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLogServer(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_TestLogServer(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ControlRecordInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordStatus(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordSchedule(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRecordSchedule(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_FindRecordFileInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DownloadRecordFile(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTamperAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTamperAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetMotionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetMotionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCrossLineAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCrossLineAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetIntrusionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetIntrusionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLoiteringAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLoiteringAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPreviewInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPreviewInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetChannelInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetChannelList(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetParkingAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetParkingAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTalkbackState(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTalkbackToStream(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTalkbackFromStream(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetReplayTalkback(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUpgradeStatus(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetUpgrade(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetUpgradeVersion(NET_TV_CB_GetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCapturePlanInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCapturePlanInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCaptureParamInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCaptureParamInfo(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetExposureInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetExposureInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDayNightInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDayNightInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetBackLightInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetBackLightInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDenoiseInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDenoiseInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetAudioCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetAudioCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceCompareInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_AddTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DelTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetTargetLib(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetTargetLib(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_AddFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_DelFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetFaceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetFaceInfo(NET_TV_CB_GetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg(NET_TV_CB_SetDevConfigByCommand pCb);

NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetCongestionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetCongestionCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPersonFallCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPersonFallCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSmokingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSmokingCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOpenFlameCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOpenFlameCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetBareSoilCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetBareSoilCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg(NET_TV_CB_SetDevConfigByCommand pCb);

/* 智能事件配置回调注册接口 */
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetClimbFenceInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetClimbFenceInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetDimissionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetDimissionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRetrogradeInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRetrogradeInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetSmokeFireCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetSmokeFireCfg(NET_TV_CB_SetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_GetRoadPondingCfg(NET_TV_CB_GetDevConfigByCommand pCb);
NET_TV_API BOOL STDCALL NET_TV_SERVER_RegisterCb_SetRoadPondingCfg(NET_TV_CB_SetDevConfigByCommand pCb);




#ifdef __cplusplus
}
#endif

#endif /* NETTVSDK_H */
