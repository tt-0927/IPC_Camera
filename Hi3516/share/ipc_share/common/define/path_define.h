/**
 * @FilePath     : path_define.h
 * @Author       : zhouzirui
 * @Date         : 2025-04-08 15:35:27
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-06 09:58:40
 * @Description  : 文件、目录路径宏定义 
 */

#pragma once

#define USER_PATH                           "/opt"
#define CAM_PATH            USER_PATH       "/cam/"
#define COURSE_PATH         USER_PATH       "/course/"

/* sd卡挂载路径 */
#define SD_CARD_MOUNT_PATH  "/mnt"

/* 配置文件路径 */
#define USER_DATA_PATH      CAM_PATH        ".config/user_data/"
#define DESIGN_DATA_PATH    CAM_PATH        ".config/design_data/"
/*Unix域套接字路径*/
#define UDS_SOCKET_PATH     CAM_PATH        "bin/.uds_socket"
/* 数据库路径 */
#define DATABASE_PATH       CAM_PATH        "db/"
/* 证书路径 */
#define CA_PATH             CAM_PATH        "cert/"
/* 国密证书路径 */
#define GM_CA_PATH          CAM_PATH        "gm_cert/"
/* 第三方库路径 */
#define THIRD_PATRY_PATH    CAM_PATH        "third-party/"
/* 工具路径 */
#define TOOLS_PATH          CAM_PATH        "tools/"

/*freetype字体库文件*/
#define ITC_FONT_FILE       THIRD_PATRY_PATH "ttf/simhei.ttf"

/* 录制路径 */
#define RECORD_PATH         COURSE_PATH     "record"
/* 抓图路径 */
#define CAPTURE_PATH         COURSE_PATH     "capture"
/* 升级路径 */
#define UPLOAD_PATH         COURSE_PATH     "upload/"
/* 日志路径 */
#define LOG_PATH            COURSE_PATH     "log/"
/* 目标库-人脸图片存放路径 */
#define TARGETLIB_FACEIMAGE_PATH COURSE_PATH "SnapImage/FaceImage/"
/* 目标库1-人脸图片存放路径 */
#define TARGETLIB1_FACEIMAGE_PATH COURSE_PATH "SnapImage/FaceImage/Lib1"
/* 日志路径 */
#define STREAM_LOG_PATH     LOG_PATH    "stream/stream.log"
#define UPGRADE_LOG_PATH    LOG_PATH    "upgrade/upgrade.log"
#define RECORD_LOG_PATH     LOG_PATH    "record/record.log"
#define OPERATION_LOG_PATH  LOG_PATH    "operation/operation.log"

/* 证书路径 */
#define CA_DEVICE_PATH        CA_PATH "device/"                   /* 设备证书路径 */
#define CA_DEVICE_KEY         CA_PATH "device/device.key"         /* 设备证书私钥 */
#define CA_REQ_CSR            CA_PATH "request/request.csr"       /* 证书请求文件 */
#define CA_REQ_KEY            CA_PATH "request/request.key"       /* 证书请求私钥 */
#define CA_TRUST_PATH         CA_PATH "trust/"                    /* 受信任证书路径 */
#define CA_ROOT_CERT          CA_PATH "trust/root/rootCA.cer"     /* 根证书 */
#define CA_ROOT_KEY           CA_PATH "trust/root/rootCA.key"     /* 根证书私钥 */
#define CA_MIDDLE_CERT        CA_PATH "trust/middle/middleCA.cer" /* 中间证书 */
#define CA_MIDDLE_KEY         CA_PATH "trust/middle/middleCA.key" /* 中间证书私钥 */
#define CA_REQ_PATH           CA_PATH "requset/"                  /* 请求证书路径 */
#define UPLOAD_DEVICE_CA_PATH CA_PATH "device_ca"                 /* 导入设备证书路径 */
#define CA_UPLOAD_TRUST_PATH  CA_PATH "trust_ca"                  /* 导入受信任证书路径 */

/* 国密证书路径 */
#define GM_CA_DEVICE_PATH       GM_CA_PATH          "device/"           /* 设备证书路径 */
#define GM_CA_DEVICE_KEY        GM_CA_DEVICE_PATH   "deviceKey.pem"     /* 设备证书私钥 */
#define GM_CA_DEVICE_CERT       GM_CA_DEVICE_PATH   "deviceCert.pem"    /* 设备证书文件 */
#define GM_CA_REQ_PATH          GM_CA_PATH          "request/"          /* GM证书请求路径 */
#define GM_CA_REQ_CSR           GM_CA_REQ_PATH      "deviceReq.pem"     /* GM证书请求路径文件 */
#define GM_CA_TRUST_PATH        GM_CA_PATH          "trust/"            /* GM受信任证书路径 */
#define GM_CA_TRUST_CERT        GM_CA_TRUST_PATH    "caCert.pem"        /* CA证书文件 */
#define GM_PLATFORM_CERT        GM_CA_TRUST_PATH    "platformCert.pem"  /* 平台证书文件 */
#define GM_CA_TRUST_CRL         GM_CA_TRUST_PATH    "caCrl.der"         /* CA证书吊销列表 */

/* isp 配置 */
#ifdef SENSOR_SC533HAI 
#define ISP_CONFIG_PATH                                  CAM_PATH ".config/sensor_sc533hai"
#elif defined SENSOR_SC500AI 
#define ISP_CONFIG_PATH                                  CAM_PATH ".config/sensor_sc500ai"
#else 
#define ISP_CONFIG_PATH                                  CAM_PATH ".config/sensor_sc500ai"
#endif
/* 音频文件地址 */
#define AUDIO_CONFIG_PATH                                CAM_PATH ".config/audio/"
/* 自定义音频文件地址 */
#define CUSTOM_AUDIO_CONFIG_PATH                         AUDIO_CONFIG_PATH "custom/"

/* 事件配置 */
/* 算法配置 */
#define EVENT_ALGORITHM_CONFIG_FILE                     USER_DATA_PATH "event_algorithm_config.json"
/* 所有算法事件计划 */
#define EVENT_SCHEDULE_CONFIG_FILE                      USER_DATA_PATH "event_schedule_config.json"
/* 事件联动扩展策略 */
#define EVENT_LINKAGE_POLICY_CONFIG_FILE                USER_DATA_PATH "event_linkage_policy.json"
/* 规则信息 */
#define EVENT_RULE_INFOS_CONFIG_FILE                    USER_DATA_PATH "event_rule_infos.json"
/* 智能资源分配-智能事件启用情况 */
#define SMART_EVENT_ENABLE_STATUS_FILE                  USER_DATA_PATH "smart_event_enable_status.json"
/* Metadata配置 */
#define METADATA_CONFIG_FILE                            USER_DATA_PATH "metadata_config.json"

/**
 * @brief   : 普通事件
 */
/* 移动侦测 */
#define EVENT_MOTION_DETECTION_CONFIG_FILE              USER_DATA_PATH "event_motion_detection.json"
/* 遮挡报警 */
#define EVENT_HIDE_ALARM_CONFIG_FILE                    USER_DATA_PATH "event_hide_alarm.json"
/* 异常报警 */
#define EVENT_ABNORMAL_ALARM_CONFIG_FILE                USER_DATA_PATH "event_abnormal_alarm.json"
/* 声音报警输出 */
#define EVENT_SOUND_ALARM_OUTPUT_CONFIG_FILE            USER_DATA_PATH "event_sound_alarm_output.json"
/* 报警输入 */
#define EVENT_ALARM_INPUT_CONFIG_FILE                   USER_DATA_PATH "event_alarm_input.json"
/* 报警输出 */
#define EVENT_ALARM_OUTPUT_CONFIG_FILE                  USER_DATA_PATH "event_alarm_output.json"
/* 闪光报警灯输出 */
#define EVENT_FLASHING_LIGHT_ALARM_OUTPUT_CONFIG_FILE   USER_DATA_PATH "event_flashing_light_alarm_output.json"
/* 手动声光报警联动 */
#define EVENT_MANUAL_SOUND_LIGHT_ALARM_CONFIG_FILE      USER_DATA_PATH "event_manual_sound_light_alarm.json"
/* PIR报警 */
#define EVENT_PIR_ALARM_CONFIG_FILE                     USER_DATA_PATH "event_pir_alarm.json"
/**
 * @brief   : 周界事件
 */
/* 越界侦测 */
#define EVENT_LINE_CROSSING_DETECTION_CONFIG_FILE       USER_DATA_PATH "event_line_crossing_detection.json"
/* 区域入侵侦测 */
#define EVENT_REGIONAL_INTRUSION_DETECTION_CONFIG_FILE  USER_DATA_PATH "event_regional_intrusion_detection.json"
/* 进入区域侦测 */
#define EVENT_ENTER_REGION_DETECTION_CONFIG_FILE        USER_DATA_PATH "event_enter_region_detection.json"
/* 离开区域侦测 */
#define EVENT_LEAVE_REGION_DETECTION_CONFIG_FILE        USER_DATA_PATH "event_leave_region_detection.json"

/**
 * @brief   : smart事件
 */
/* 音频异常侦测 */
#define EVENT_AUDIO_ANOMALY_DETECTION_CONFIG_FILE       USER_DATA_PATH "event_audio_anomaly_detection.json"
/* 场景变更侦测 */
#define EVENT_SCENE_CHANGE_DETECTION_CONFIG_FILE        USER_DATA_PATH "event_scene_change_detection.json"
/* 人脸侦测 */
#define EVENT_FACE_DETECTION_CONFIG_FILE                USER_DATA_PATH "event_face_detection.json"
/* 徘徊侦测 */
#define EVENT_LOITERING_DETECTION_CONFIG_FILE           USER_DATA_PATH "event_loitering_detection.json"
/* 人员聚集侦测 */
#define EVENT_CROWD_GATHERING_DETECTION_CONFIG_FILE     USER_DATA_PATH "event_crowd_gathering_detection.json"
/* 停车侦测 */
#define EVENT_PARKING_DETECTION_CONFIG_FILE             USER_DATA_PATH "event_parking_detection.json"
/* 物品遗留侦测 */
#define EVENT_UNATTENDED_OBJECT_DETECTION_CONFIG_FILE   USER_DATA_PATH "event_unattended_object_detection.json"
/* 物品拿取侦测 */
#define EVENT_OBJECT_REMOVAL_DETECTION_CONFIG_FILE      USER_DATA_PATH "event_object_removal_detection.json"
/* 宠物识别 */
#define EVENT_PET_RECOGNITION_CONFIG_FILE               USER_DATA_PATH "event_pet_recognition.json"
/* 人脸抓拍 */
#define EVENT_FACE_CAPTURE_CONFIG_FILE                  USER_DATA_PATH "event_face_capture.json"
/* 人脸比对 */
#define EVENT_FACE_COMPARE_CONFIG_FILE                  USER_DATA_PATH "event_face_compare.json"
/* 人脸抓拍叠加信息 */
#define EVENT_FACE_CAPTURE_OVERLAY_INFO_FILE            USER_DATA_PATH "event_face_capture_overlay_info.json"

/**
 * @brief   : 场景智能分析事件
 */

/* AI LLM模型推理场景智能分析控制 */
#define EVENT_AI_LLM_AI_SCENE_ANALYSIS_INFO_FILE        USER_DATA_PATH "event_ai_scene_analysis_info.json"
/* AI LLM模型推理画面分析信息 */
#define EVENT_AI_LLMIMAGE_ANALYSIS_INFO_FILE            USER_DATA_PATH "event_ai_image_analysis_info.json"
/* AI LLM模型推理画面分析记录信息 */
#define EVENT_AI_LLMIMAGE_ANALYSIS_RECORD_FILE           USER_DATA_PATH "event_ai_image_analysis_record.json"
/* 文字预设任务 */
#define EVENT_TEXT_PRESET_ANALYSIS_FILE            USER_DATA_PATH "event_text_preset_analysis.json"
/* 文字预设管理 */
#define EVENT_TEXT_PRESET_TASK_MANAGER_FILE          USER_DATA_PATH "event_text_preset_task_manager.json"
/* 实时预警管理 */
#define EVENT_REAL_PUSH_ALARM_MANAGER_FILE          USER_DATA_PATH "event_real_push_alarm_manager.json"

/* 画面分析图片保存路径 */
#define EVENT_IMAGE_ANALYSIS_PIC_PATH          COURSE_PATH  "vlm_record/image/ImageAnalysis/image_"

/* 文字预设任务图片保存路径 */
#define EVENT_TEXT_ANALYSIS_PIC_PATH           COURSE_PATH  "vlm_record/image/TextPreset/"

/* 模型推理视频分片目录名 */
#define VLM_VIDEO "ai_video_backup"

/* 模型推理视频分片路径 */
#define AI_VIDEO_BACKUP_PATH                   COURSE_PATH  "vlm_record/video/ai_video_backup"

/**
 * @brief   : 场景智能
 */
/* 翻越围栏 */
#define EVENT_FENCE_CLIMBING_INFO_FILE            USER_DATA_PATH "event_fence_climbing_info.json"
/* 离岗识别 */
#define EVENT_LEAVE_POST_INFO_FILE                USER_DATA_PATH "event_leave_post_info.json"
/* 行人闯入 */
#define EVENT_PEDESTRIAN_INTRUSION_INFO_FILE      USER_DATA_PATH "event_pedestrian_intrusion_info.json"
/* 烟火识别 */
#define EVENT_SMOKE_FIRE_INFO_FILE                USER_DATA_PATH "event_smoke_fire_info.json"
/* 明火识别 */
#define EVENT_OPEN_FLAME_INFO_FILE                USER_DATA_PATH "event_open_flame_info.json"
/* 道路积水检测 */
#define EVENT_ROAD_PONDING_INFO_FILE              USER_DATA_PATH "event_road_ponding_info.json"
/* 井盖异常检测 */
#define EVENT_MANHOLE_COVER_ABNORMAL_INFO_FILE    USER_DATA_PATH "event_manhole_coverAbnormal_info.json"
/* 垃圾暴露检测 */
#define EVENT_GARBAGE_EXPOSURE_INFO_FILE          USER_DATA_PATH "event_garbage_exposure_info.json"
/* 垃圾满溢检测 */
#define EVENT_GARBAGE_OVERFLOW_INFO_FILE          USER_DATA_PATH "event_garbage_overflow_info.json"
/* 睡岗识别 */
#define EVENT_SLEEP_ON_DUTY_INFO_FILE             USER_DATA_PATH "event_sleep_onDuty_info.json"
/* 摔倒识别 */
#define EVENT_PERSON_TRIP_INFO_FILE               USER_DATA_PATH "event_trip_info.json"
/* 倒地识别 */
#define EVENT_PERSON_FALL_DOWN_INFO_FILE          USER_DATA_PATH "event_person_fallDown_info.json"
/* 玩手机识别 */
#define EVENT_PHONE_USAGE_INFO_FILE               USER_DATA_PATH "event_phone_usage_info.json"
/* 高空安全带识别 */
#define EVENT_HIGH_ALTITUDE_SEATBELT_INFO_FILE    USER_DATA_PATH "event_highAltitude_seatbelt_info.json"
/* 黄土裸露识别 */
#define EVENT_BARE_SOIL_INFO_FILE                 USER_DATA_PATH "event_bare_soil_info.json"
/* 安全帽识别 */
#define EVENT_SAFETY_HELMET_INFO_FILE             USER_DATA_PATH "event_safety_helmet_info.json"
/* 洞口防护栏识别 */
#define EVENT_HOLE_PROTECTION_BAR_INFO_FILE       USER_DATA_PATH "event_hole_protectionBar_info.json"
/* 反光衣识别 */
#define EVENT_REFLECTIVE_CLOTHING_INFO_FILE       USER_DATA_PATH "event_reflective_clothing_info.json"
/* 抽烟识别 */
#define EVENT_SMOKINGE_INFO_FILE                  USER_DATA_PATH "event_smoking_info.json"
/* 施工占道检测 */
#define EVENT_CONSTRUCTION_ENCROACHMENT_ROAD_INFO_FILE   USER_DATA_PATH "event_construction_Encroachment_Road_info.json"
/* 电瓶车检测 */
#define EVENT_ELECTRIC_SCOOTER_INFO_FILE          USER_DATA_PATH "event_electric_scooter_info.json"
/* 车牌识别检测 */
#define EVENT_LICENSE_PLATE_COGNITION_INFO_FILE   USER_DATA_PATH "event_license_plate_cognition_info.json"
/* 逆行识别检测 */
#define EVENT_DRIVING_AGAINST_TRAFFIC_INFO_FILE   USER_DATA_PATH "event_driving_against_traffic_info.json"
/* 违规变道识别检测 */
#define EVENT_ILLEGAL_LANE_CHANGE_INFO_FILE       USER_DATA_PATH "event_illegalLane_change_info.json"
/* 拥堵识别检测 */
#define EVENT_CONGESTION_INFO_FILE                USER_DATA_PATH "event_congestion_info.json"
/* 应急车道占用识别检测 */
#define EVENT_EMERGENCY_LANE_OCCUPANCY_INFO_FILE  USER_DATA_PATH "event_emergency_lane_occupancy_info.json"
/* 非机动车闯入识别检测 */
#define EVENT_NON_MOTOR_VEHICLE_INTRUSION_INFO_FILE  USER_DATA_PATH "event_nonmotor_vehicle_intrusion_info.json"

/* 属性识别检测 */
#define ATTRIBUTE_DETECT_INFO_FILE  USER_DATA_PATH "attribute_detect_info.json"

/* 人流统计 */
#define EVENT_PEOPLE_FLOW_STATISTICS_CONFIG_FILE        USER_DATA_PATH "event_people_flow_statistics.json"
/* 人员密度检测 */
#define EVENT_PEOPLE_DENSITY_DETECTION_CONFIG_FILE      USER_DATA_PATH "event_people_density_detection.json"

/* Ipc信息 */
#define IPC_INFO_CONFIG_FILE            USER_DATA_PATH "IpcInfo.json"
/* IPC通道排序 */
#define IPC_SORT_CONFIG_FILE            USER_DATA_PATH "IpcSortConfig.json"
/* IPC视频编码信息 */
#define IPC_VIDEO_INFO_CONFIG_FILE      USER_DATA_PATH "IpcVideoInfo.json"
/* IPC自定义协议 */
#define IPC_CUSTOM_PROTOCOL_CONFIG_FILE USER_DATA_PATH "IpcCustomProtocol.json"
/* IPC默认配置信息 */
#define IPC_DEFAULT_CONFIG_FILE         USER_DATA_PATH "IpcDefaultConfig.json"
/* IPC配置信息 */
#define IPC_CONFIG_INFO_FILE            USER_DATA_PATH "IpcConfigInfo.csv"

/* 预览基本配置 */
#define PREVIEW_CONFIG_FILE                USER_DATA_PATH "preview.json"
/* 预览视图配置 */
#define PREVIEW_VIEW_CONFIG_FILE           USER_DATA_PATH "ViewConf.json"
/* 零通道配置 */
#define ZERO_CHANNEL_CONFIG_FILE           USER_DATA_PATH "ZeroChannelConf.json"
/* 预览通道 */
#define PREVIEW_CHANNEL_CONFIG_FILE        USER_DATA_PATH "PreviewChannle.json"
/* 更多配置 */
#define PREVIEW_MORE_CONFIGURA_CONFIG_FILE USER_DATA_PATH "MoreConfiguration.json"

/* 录制配置 */
/* 录制计划配置 */
#define RECORD_SCHEDULE_CONFIG_FILE       USER_DATA_PATH "record_schedule.json"
/* 录制高级参数 */
#define RECORD_ADVANCED_PARAM_CONFIG_FILE USER_DATA_PATH "record_advanced_param.json"
/* 录制配额配置 */
#define RECORD_QUOTA_CONFIG_FILE          USER_DATA_PATH "record_quota_config.json"
/* 录制盘组配置 */
#define RECORD_DISK_GROUP_CONFIG_FILE     USER_DATA_PATH "record_disk_group_config.json"
/* 录制存储模式配置 */
#define RECORD_STORAGE_MODE_CONFIG_FILE   USER_DATA_PATH "storage_mode_info.json"
/* 假日信息配置 */
#define HOLIDAY_INFO_CONFIG_FILE USER_DATA_PATH "holiday_info.json"
/* 录制其他配置 */
#define  RECORD_OTHER_CONFIG_FILE USER_DATA_PATH "record_other_info.json"

/* Bonjour配置 */
#define BONJOUR_CONFIG_FILE        USER_DATA_PATH "bonjour.json"
/* DDNS配置 */
#define DDNS_CONFIG_FILE           USER_DATA_PATH "ddns.json"
/* Email配置 */
#define EMAIL_CONFIG_FILE          USER_DATA_PATH "email.json"
/* SNMP配置 */
#define SNMP_CONFIG_FILE           USER_DATA_PATH "snmp.json"
/* PPPOE配置 */
#define PPPOE_CONFIG_FILE          USER_DATA_PATH "pppoe.json"
/* QOS配置 */
#define QOS_CONFIG_FILE            USER_DATA_PATH "qos.json"
/* 端口配置 */
#define PORT_CONFIG_FILE           USER_DATA_PATH "port.json"
/* UPNP端口映射配置 */
#define UPNP_CONFIG_FILE           USER_DATA_PATH "upnp.json"
/* 认证方式 */
#define SECURITY_CERT_CONFIG_FILE  USER_DATA_PATH "security_cert.json"
/* ONVIF配置 */
#define ONVIF_CONFIG_FILE          USER_DATA_PATH "onvif.json"
/* GB28181客户端配置 */
#define GB28181_CLIENT_CONFIG_FILE USER_DATA_PATH "gb28181_client.json"
/* HTTPS配置 */
#define HTTPS_CONFIG_FILE          USER_DATA_PATH "https.json"
/* IP地址过滤配置 */
#define IP_FILTER_CONFIG_FILE      USER_DATA_PATH "ip_filter.json"
/* wifi配置 */
#define WIFI_CONFIG_FILE           USER_DATA_PATH "wifi.json"
/* 4G配置 */
#define NETWORK_4G_CONFIG_FILE     USER_DATA_PATH "network_4g.json"
/*热点功能配置*/
#define HOSTAPD_CONFIG_FILE        USER_DATA_PATH "hostapd.json"
/* 系统配置 */
/* 设备基本配置 */
#define DEVICE_CONFIG_FILE          USER_DATA_PATH "device.json"
/* 设备信息配置 */
#define DEVICE_INFO_CONFIG_FILE     USER_DATA_PATH "device_info.json"
/* 安全服务配置 */
#define SECURITY_SERVICES_FILE      USER_DATA_PATH "security_services.json"
/* 升级维护配置 */
#define UPGRADE_MAINTAIN_FILE       USER_DATA_PATH "upgrade_maintain.json"
/* 时间配置 */
#define TIME_CONFIG_FILE            USER_DATA_PATH "time.json"
/* 时间配置保存文件 */
#define NTP_CONFIG_FILE             USER_DATA_PATH "ntp.json"
/* RS232配置 */
#define RS232_CONFIG_FILE           USER_DATA_PATH "RS232Info.json"
/* RS485配置 */
#define RS485_CONFIG_FILE           USER_DATA_PATH "RS485Info.json"
/* smart检测信息 */
#define SMART_DETECTION_CONFIG_FILE USER_DATA_PATH "SmartInfo.json"
/* 外设配置信息 */
#define PERIPHERAL_CONFIG_FILE      USER_DATA_PATH "peripheral_info.json"

/* 网络配置 */
#define NETWORK_INTERFACES          "/etc/network/interfaces"
#define NETWORK_INTERFACES_TMP      "/etc/network/interfaces.tmp"
#define NETWORK_INIT_SCRIPT         "/etc/init.d/S80network"
#define NETWORK_INIT_SCRIPT_TMP     "/etc/init.d/S80network.tmp"
#define NETWORK_CONFIG_FILE         USER_DATA_PATH  "network.json"
#define NETWORK_WIFI_CONFIG_FILE    USER_DATA_PATH  "network_wifi.json"
/* 垃圾站平台管理配置 */
#define PLATFORM_CONFIG_FILE        USER_DATA_PATH  "platform_config.json"
/* 日志服务器 */
#define LOG_SERVER_INFO_FILE        USER_DATA_PATH  "log_server.json"
/* 设备注册信息 */
#define REGISTER_INFO_FILE          DESIGN_DATA_PATH "register/register.json"
/* 设备注册 uuid 信息 */
#define UUID_FILE_PATH              CAM_PATH         ".uuid.txt"
/* 设备注册信息备份文件 */
#define REGISTER_BACKUP_FILE_PATH   DESIGN_DATA_PATH "register/.backup.register.json"
/* 激活码管理文件 */
#define REGISTER_CODE_MANAGE_PATH   DESIGN_DATA_PATH "register/registerCodeManage.ini"

/* 图案解锁 */
#define PATTERN_UNLOCK_FILE         DESIGN_DATA_PATH "register/pattern.json"
/* 激活信息 */
#define ACTIVATE_INFO_FILE          DESIGN_DATA_PATH "register/ActivationInfo.json"
/* web插件配置 */
#define WEB_PLUGIN_CONFIG_FILE      USER_DATA_PATH "web_plugin_config.json"
/* 产测配置 */
#define PRODUCTION_TEST_CONFIG_FILE USER_DATA_PATH "production_test.json"
/* 设备类型 */
#define DEVICE_PARAM_FILE           USER_DATA_PATH "IpcDeviceParams.txt"

/* 视频配置 */
#define VIDEO_CONFIG_FILE           USER_DATA_PATH "video_config.json"
/* ROI感兴趣区域配置 */
#define ROI_CONFIG_FILE             USER_DATA_PATH "roi_config.json"
/* 区域裁剪配置 */
#define AREA_CROP_CONFIG_FILE       USER_DATA_PATH "area_crop_config.json"
/* 视频能力集 */
#define VIDEO_CAPABILITY_SET_FILE   USER_DATA_PATH "video_capability_set.json"
/* 音频能力集 */
#define AUDIO_CAPABILITY_SET_FILE   USER_DATA_PATH "audio_capability_set.json"
/* 音频配置 */
#define AUDIO_CONFIG_FILE           USER_DATA_PATH "audio_config.json"
/* GB28181配置文件 */
#define GB28181_CONFIG_FILE         USER_DATA_PATH "gb28181_client.json"
/* 国密证书信息文件 */
#define GM_CERT_INFO_FILE           USER_DATA_PATH "gm_cert_info.json"
/* 国密CRL文件信息 */
#define GM_CRL_INFO_FILE            USER_DATA_PATH "gm_crl_info.json"

/* stream配置文件 */
#define STREAM_INI                  USER_DATA_PATH "stream_conf.ini"

/* OSD配置 */
#define OSD_CONFIG_FILE             USER_DATA_PATH "osd_config.json"
/* Cover配置 */
#define COVER_CONFIG_FILE           USER_DATA_PATH "cover_config.json"
/* Osd_overplay配置 */
#define OSD_OVERPLAY_CONFIG_FILE    USER_DATA_PATH "overplay.json"
/* Osd_cover配置 */
#define OSD_COVER_CONFIG_FILE       USER_DATA_PATH "cover.json"

/* 抓图计划 */
#define CAPTURE_PLAN_CONFIG_FILE       USER_DATA_PATH "capture_plan.json"
/* 抓图参数 */
#define CAPTURE_PARAM_CONFIG_FILE       USER_DATA_PATH "capture_param.json"

/* 储存管理参数 */
#define STORAGE_MANAGE_CONFIG_FILE       USER_DATA_PATH "storage_manage_config.json"

/* AI模型配置 */
/* 人脸侦测 */
#define AI_FACE_DETECTION_CONFIG_FILE       DESIGN_DATA_PATH "ai_face_detection.json"
/*人脸和垃圾检测*/
#define AI_FACE_RUBISH_DETECTION_CONFIG_FILE       DESIGN_DATA_PATH "aiRknnConfig.json"
/* 人脸特征 */
#define AI_FACE_FEATURE_CONFIG_FILE         DESIGN_DATA_PATH "ai_mobileface.json"
/* 人头侦测 */
#define AI_HEAD_DETECTION_CONFIG_FILE       DESIGN_DATA_PATH "ai_head_detection.json"
/* 停车侦测 */
#define AI_PARKING_DETECTION_CONFIG_FILE    DESIGN_DATA_PATH "ai_parking_detection.json"
/* 垃圾检测 */
#define AI_GARBAGE_DETECTION_CONFIG_FILE    DESIGN_DATA_PATH "ai_garbage_detection.json"
/* rknn模型 */
#define AI_RKNN_DETECTION_CONFIG_FILE    DESIGN_DATA_PATH "ai_rknn_detect.json"
/* rkllm模型 */
#define AI_RKLLM_CONFIG_FILE    DESIGN_DATA_PATH "ai_rkllm_analysis.json"

/* 录制文件数据库路径 */
#define RECORD_DATABASE_PATH        SD_CARD_MOUNT_PATH   "/record/record_file_manage.db"
/* 录制事件智能摘要文件数据库路径 */
#define RECORD_EVENT_DATABASE_PATH  SD_CARD_MOUNT_PATH   "/record/event_manage.db"
/* 抓图文件数据库路径 */
#define CAPTURE_DATABASE_PATH       SD_CARD_MOUNT_PATH   "/capture/capture_file_manage.db"

/********** 图像配置 *****************************/
/* 场景配置 */
#define PIC_SCENE_CONFIG_FILE    USER_DATA_PATH "pic_scene_config.json"
/* 镜像配置 */
#define PIC_MIRROR_CONFIG_FILE    USER_DATA_PATH "pic_mirror_config.json"
/* 图像计划配置 */
#define IMAGE_SCHEDULE_CONFIG_FILE     USER_DATA_PATH "image_schedule.json"
/* 图像参数配置 */
#define PIC_SCENEPARAM_CONFIG_FILE     USER_DATA_PATH "pic_scene_param.json"

#ifdef ENABLE_GAT1400_SRC
/* GAT1400配置文件 */
#define GAT1400_CONFIG_FILE     USER_DATA_PATH "gat1400_config.json"
#endif

#ifdef ENABLE_AI_STUDENT
/* AI STUDENT配置文件 */
#define AI_STUDENT_CLASS_CONFIG_FILE    USER_DATA_PATH "ai_student_config.json"
#endif
