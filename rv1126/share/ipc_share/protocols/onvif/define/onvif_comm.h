/**
 * @file onvif_comm.h
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-28
 * 
 * @brief onvif 通用定义
 */
#pragma once

#define ONVIF_SEVICE_NUM 6      /* 当前可用sevice数量 */
/* onvif服务namespace */
#define ONVIF_DEVICE_URL "http://www.onvif.org/ver10/device/wsdl"           /* device的服务地址 */
#define ONVIF_MEDI_URL "http://www.onvif.org/ver10/media/wsdl"              /* media的服务地址 */
#define ONVIF_EVENT_URL "http://www.onvif.org/ver10/events/wsdl"            /* event的服务地址 */
#define ONVIF_PTZ_URL "http://www.onvif.org/ver20/ptz/wsdl"                 /* ptz的服务地址 */
#define ONVIF_IMGING_URL "http://www.onvif.org/ver20/imaging"               /* imging的服务地址 */
#define ONVIF_DEVICEIO_URL "http://www.onvif.org/ver10/deviceIO/wsdl"       /* deviceIO的服务地址 */
#define ONVIF_ANALYTICS_URL "http://www.onvif.org/ver20/analytics/wsdl"     /* analytics的服务地址 */
#define ONVIF_RECORDING_URL "http://www.onvif.org/ver10/recording/wsdl"     /* recording的服务地址 */
#define ONVIF_SEARCH_URL "http://www.onvif.org/ver10/search/wsdl"           /* search的服务地址 */
#define ONVIF_REPLAY_URL "http://www.onvif.org/ver10/replay/wsdl"           /* replay的服务地址 */
#define ONVIF_MEDIA2_URL "http://www.onvif.org/ver20/media/wsdl"            /* media2的服务地址 */

#define MANFACTURER "itc IPC"
// 认证信息
#define AUTHREALM "ITC_IPC"

/* 打印调试开关 */
#define ONVIF_LOG_SWITCH 0

#define ONVIF_REBOOT_MESSAGE "Rebooting in 90 seconds"

/* 超时时间 */
#define ONVIF_TIME_OUT 60000      //60s
/* onvif端口 */
#define ONVIF_UDP_PORT 3702
#define ONVIF_TCP_PORT 58080 
#define ONVIF_UPGRADE_PORT 58081
#define ONVIF_HTTP_PORT 80 

//ip
#define NETWORK_ADDR "0.0.0.0"
#define ONVIF_UDP_IP "239.255.255.250"
//frame size
#define ONVIF_FRAME_WIDTH 1920
#define ONVIF_FRAME_HEIGHT 1080  

//ROI裁剪区域的宽和高 默认值
#define ONVIF_BOUNDS_WIDTH 2560
#define ONVIF_BOUNDS_HEIGHT 1440  

/* 鉴权方式 */
#define ONVIF_DIGEST_MODE 0
#define ONVIF_AUTH_MODE 1

/*码流通道*/
#define ONVIF_RTSP_CHN_MAIN 0
#define ONVIF_RTSP_CHN_SUB 1

/* 最大支持帧率 */
#define VIDEO_FRAMERATE_MAX 30

/* 码率范围 */
#define VIDEO_BITRATE_MIN 256
#define VIDEO_BITRATE_MAX 16384


/* I帧间隔 */
#define VIDEO_I_FRAME_INTERVAL_MAX 400

/* 视频编码格式 */
#define VIDEO_CODEC_H264 "H264"
#define VIDEO_CODEC_H265 "H265"
#define VIDEO_CODEC_MJPEG "MJPEG"
#define VIDEO_CODEC_SVAC3 "SVAC3"

/* 音频采样率支持数量 */
#define AUDIO_SAMPRATE_LIST 11
/* 音频码率支持数量 */
#define AUDIO_BITRATE_LIST 6

// ONVIF DateFormat 
#define ONVIF_TT_DATE_FORMAT_MM_DD_YYYY       "MM/dd/yyyy"
#define ONVIF_TT_DATE_FORMAT_DD_MM_YYYY       "dd/MM/yyyy"
#define ONVIF_TT_DATE_FORMAT_YYYY_MM_DD_SLASH "yyyy/MM/dd"
#define ONVIF_TT_DATE_FORMAT_YYYY_MM_DD_DASH  "yyyy-MM-dd"

// ONVIF TimeFormat 
#define ONVIF_TT_TIME_FORMAT_HH_MM_SS_12H     "hh:mm:ss tt" // 12小时制
#define ONVIF_TT_TIME_FORMAT_HH_MM_SS_24H     "HH:mm:ss"    // 24小时制 

// ONVIF 位置选项宏定义
#define ONVIF_TT_POSITION_UPPER_LEFT     "UpperLeft"     // 左上
#define ONVIF_TT_POSITION_UPPER_RIGHT    "UpperRight"    // 右上
#define ONVIF_TT_POSITION_LOWER_LEFT     "LowerLeft"     // 左下
#define ONVIF_TT_POSITION_LOWER_RIGHT    "LowerRight"    // 右下
#define ONVIF_TT_POSITION_CUSTOM         "Custom"        // 自定义

// ONVIF tt:Type 宏定义
#define ONVIF_TT_TYPE_PLAIN        "Plain"
#define ONVIF_TT_TYPE_DATE         "Date"
#define ONVIF_TT_TYPE_TIME         "Time"
#define ONVIF_TT_TYPE_DATEANDTIME  "DateAndTime"

#define ONVIF_TT_OSD_COLORSPACE     "http://www.onvif.org/ver10/colorspace/YCbCr"
//ONVIF OSD文本拓展定义 代表是否是通道名称OSD
#define ONVIF_TT_OSD_TXET_EXTEN_CHANNEL_TRUE    "<tt:ChannelName>true</tt:ChannelName>"
#define ONVIF_TT_OSD_TXET_EXTEN_CHANNEL_FALSE   "<tt:ChannelName>false</tt:ChannelName>"


#define ONVIF_EVENT_MAX_PER_SUB 10  // 事件缓存 每个订阅最多缓存10个事件
#define ONVIF_MAX_SUBSCRIPTIONS 20 // 事件订阅 最多20个订阅
#define PULLMSG_TIMEOUT_UNIT    (3*1000) //事件订阅超时    

#define TAMPER_NAME             "IsTamper"      /* 遮挡报警 */
#define MOTION_NAME             "IsMotion"      /* 移动报警 */
#define LINE_NAME               "IsLineCross"   /* 拌线检测报警 */
#define FIELD_NAME              "IsInside"      /* 区域检测报警 */

/* 区域相关事件名称 */
#define ENTER_REGION_NAME        "IsEnterRegion"      /* 进入区域报警 */
#define LEAVE_REGION_NAME        "IsLeaveRegion"      /* 离开区域报警 */

/* 智能事件名称 */
#define AUDIO_ANOMALY_NAME       "IsAudioAnomaly"     /* 音频异常侦测报警 */
#define AUDIO_SUDDEN_RISE_NAME   "IsAudioSuddenRise"  /* 音频异常-声强陡升报警 */
#define AUDIO_SUDDEN_DROP_NAME   "IsAudioSuddenDrop"  /* 音频异常-声强陡降报警 */
#define SCENE_CHANGE_NAME        "IsSceneChange"      /* 场景变更报警 */
#define FACE_DETECT_NAME         "IsFaceDetect"       /* 人脸侦测报警 */
#define LOITERING_DETECT_NAME    "IsLoiteringDetect"  /* 徘徊侦测报警 */
#define CROWD_GATHERING_NAME     "IsCrowdGathering"   /* 人员聚集报警 */
#define PARKING_DETECT_NAME      "IsParkingDetect"    /* 停车侦测报警 */
#define UNATTENDED_OBJECT_NAME   "IsUnattendedObject" /* 物品遗留报警 */
#define OBJECT_REMOVAL_NAME      "IsObjectRemoval"    /* 物品拿取报警 */
#define PET_RECOGNITION_NAME     "IsPetRecognition"   /* 宠物识别报警 */
#define FACE_CAPTURE_NAME        "IsFaceCapture"      /* 人脸抓拍报警 */

/* onvif订阅事件主题 */
#define TAMPER_EVENT_THEME              "tns1:RuleEngine/TamperDetector/Tamper"         /* 遮挡报警主题 */
#define MOTION_EVENT_THEME              "tns1:RuleEngine/CellMotionDetector/Motion"     /* 移动报警主题 */
#define LINE_EVENT_THEME                "tns1:RuleEngine/LineDetector/Crossed"          /* 拌线报警主题 */
#define FIELD_EVENT_THEME               "tns1:RuleEngine/FieldDetector/ObjectsInside"   /* 区域报警主题 */

/* 区域相关事件主题 */
#define ENTER_REGION_EVENT_THEME  "tns1:RuleEngine/RegionDetector/Enter"    /* 进入区域事件主题 */
#define LEAVE_REGION_EVENT_THEME  "tns1:RuleEngine/RegionDetector/Leave"    /* 离开区域事件主题 */

/* 智能事件主题 */
#define AUDIO_ANOMALY_EVENT_THEME      "tns1:RuleEngine/AudioDetector/Anomaly"        /* 音频异常侦测事件主题 */
#define AUDIO_SUDDEN_RISE_EVENT_THEME  "tns1:RuleEngine/AudioDetector/SuddenRise"     /* 音频异常-声强陡升事件主题 */
#define AUDIO_SUDDEN_DROP_EVENT_THEME  "tns1:RuleEngine/AudioDetector/SuddenDrop"     /* 音频异常-声强陡降事件主题 */
#define SCENE_CHANGE_EVENT_THEME       "tns1:RuleEngine/SceneDetector/Change"         /* 场景变更事件主题 */
#define FACE_DETECT_EVENT_THEME        "tns1:RuleEngine/FaceDetector/Detect"          /* 人脸侦测事件主题 */
#define LOITERING_DETECT_EVENT_THEME   "tns1:RuleEngine/LoiteringDetector/Detect"     /* 徘徊侦测事件主题 */
#define CROWD_GATHERING_EVENT_THEME    "tns1:RuleEngine/CrowdDetector/Gathering"      /* 人员聚集事件主题 */
#define PARKING_DETECT_EVENT_THEME     "tns1:RuleEngine/ParkingDetector/Detect"        /* 停车侦测事件主题 */
#define UNATTENDED_OBJECT_EVENT_THEME  "tns1:RuleEngine/ObjectDetector/Unattended"     /* 物品遗留事件主题 */
#define OBJECT_REMOVAL_EVENT_THEME     "tns1:RuleEngine/ObjectDetector/Removal"        /* 物品拿取事件主题 */
#define PET_RECOGNITION_EVENT_THEME    "tns1:RuleEngine/PetDetector/Recognition"       /* 宠物识别事件主题 */
#define FACE_CAPTURE_EVENT_THEME       "tns1:RuleEngine/FaceDetector/Capture"         /* 人脸抓拍事件主题 */

#define ONVIF_ANALYTICS_SENS_NAME   "Sensitivity"           //分析灵敏度定义名称
#define ONVIF_ANALYTICS_LAYOUT_NAME  "Layout"               //分析布局规则定义名称
#define ONVIF_ANALYTICS_TRANS_NAME  "Transformation"        //分析坐标转换信息定义名称
#define ONVIF_ANALYTICS_FIELD_NAME  "Field"                 //分析区域定义名称

#define ONVIF_ANALYTICS_INTEGER_TYPE    "xs:integer"            //分析模块规则参数类型,整数
#define ONVIF_ANALYTICS_BASE64_TYPE     "xs:base64Binary"       //分析模块规则参数类型,Base编码
#define ONVIF_ANALYTICS_STRING_TYPE     "xs:string"             //分析模块规则参数类型,字符串
#define ONVIF_ANALYTICS_TOKEN_TYPE      "xs:ReferenceToken"     //分析模块规则参数类型,TOKEN
#define ONVIF_ANALYTICS_BOOL_TYPE       "xs:boolean"            //分析模块规则参数类型,bool
#define ONVIF_ANALYTICS_POLYGON_TYPE       "tt:PolygonConfiguration"        //分析模块规则参数类型,区域
#define ONVIF_ANALYTICS_DIRECTION_TYPE       "tt:Direction"                 //分析模块规则参数类型,方向
#define ONVIF_ANALYTICS_POLYLINE_TYPE       "tt:Polyline"                   //分析模块规则参数类型,画线区域
#define ONVIF_ANALYTICS_POLYON_TYPE       "tt:Polygon"                   //分析模块规则参数类型,画线区域
#define ONVIF_ANALYTICS_CELL_TYPE    "xs:CellLayout"            //分析模块规则参数类型,网格
#define ONVIF_ANALYTICS_TRANS_TYPE    "xs:Transformation"            //分析模块规则参数类型,缩放
/* onvif分析规则名称 */
#define MOTION_EVENT_RULE              "MyMotionDetectorRule"               /* 移动报警规则 */
#define TAMPEREVENT_RULE                "MyTamperDetectorRule"              /* 遮挡报警规则 */
#define LINE_EVENT_RULE                 "MyCrossedDetectorRule"             /* 拌线报警规则 */
#define FIELD_EVENT_RULE                "MyObjectsDetectorRule"             /* 区域报警规则 */

/* 区域相关事件规则 */
#define ENTER_REGION_EVENT_RULE        "MyRegionEnterDetectorRule"          /* 进入区域报警规则 */
#define LEAVE_REGION_EVENT_RULE        "MyRegionLeaveDetectorRule"          /* 离开区域报警规则 */

/* 智能事件规则 */
#define AUDIO_ANOMALY_EVENT_RULE       "MyAudioAnomalyDetectorRule"         /* 音频异常侦测规则 */
#define AUDIO_SUDDEN_RISE_EVENT_RULE   "MyAudioSuddenRiseDetectorRule"      /* 音频异常-声强陡升规则 */
#define AUDIO_SUDDEN_DROP_EVENT_RULE   "MyAudioSuddenDropDetectorRule"      /* 音频异常-声强陡降规则 */
#define SCENE_CHANGE_EVENT_RULE        "MySceneChangeDetectorRule"          /* 场景变更规则 */
#define FACE_DETECT_EVENT_RULE         "MyFaceDetectDetectorRule"           /* 人脸侦测规则 */
#define LOITERING_DETECT_EVENT_RULE    "MyLoiteringDetectorRule"            /* 徘徊侦测规则 */
#define CROWD_GATHERING_EVENT_RULE     "MyCrowdGatheringDetectorRule"       /* 人员聚集规则 */
#define PARKING_DETECT_EVENT_RULE      "MyParkingDetectorRule"              /* 停车侦测规则 */
#define UNATTENDED_OBJECT_EVENT_RULE   "MyUnattendedObjectDetectorRule"     /* 物品遗留规则 */
#define OBJECT_REMOVAL_EVENT_RULE      "MyObjectRemovalDetectorRule"        /* 物品拿取规则 */
#define PET_RECOGNITION_EVENT_RULE     "MyPetRecognitionDetectorRule"       /* 宠物识别规则 */
#define FACE_CAPTURE_EVENT_RULE        "MyFaceCaptureDetectorRule"          /* 人脸抓拍规则 */

/* onvif分析规则类型 */
#define MOTION_EVENT_RULE_TYPE              "tt:CellMotionDetector"             /* 移动报警规则 */
#define TAMPEREVENT_RULE_TYPE                "tt:TamperDetector"                /* 遮挡报警规则 */
#define LINE_EVENT_RULE_TYPE                 "tt:LineDetector"                  /* 拌线报警规则 */
#define FIELD_EVENT_RULE_TYPE                "tt:FieldDetector"                 /* 区域报警规则 */

/* 区域相关事件规则类型 */
#define ENTER_REGION_EVENT_RULE_TYPE        "tt:RegionEnterDetector"          /* 进入区域报警规则类型 */
#define LEAVE_REGION_EVENT_RULE_TYPE        "tt:RegionLeaveDetector"          /* 离开区域报警规则类型 */

/* 智能事件规则类型 */
#define AUDIO_ANOMALY_EVENT_RULE_TYPE       "tt:AudioAnomalyDetector"         /* 音频异常侦测规则类型 */
#define AUDIO_SUDDEN_RISE_EVENT_RULE_TYPE   "tt:AudioSuddenRiseDetector"      /* 音频异常-声强陡升规则类型 */
#define AUDIO_SUDDEN_DROP_EVENT_RULE_TYPE   "tt:AudioSuddenDropDetector"      /* 音频异常-声强陡降规则类型 */
#define SCENE_CHANGE_EVENT_RULE_TYPE        "tt:SceneChangeDetector"          /* 场景变更规则类型 */
#define FACE_DETECT_EVENT_RULE_TYPE         "tt:FaceDetectDetector"           /* 人脸侦测规则类型 */
#define LOITERING_DETECT_EVENT_RULE_TYPE    "tt:LoiteringDetector"            /* 徘徊侦测规则类型 */
#define CROWD_GATHERING_EVENT_RULE_TYPE     "tt:CrowdGatheringDetector"       /* 人员聚集规则类型 */
#define PARKING_DETECT_EVENT_RULE_TYPE      "tt:ParkingDetector"              /* 停车侦测规则类型 */
#define UNATTENDED_OBJECT_EVENT_RULE_TYPE   "tt:UnattendedObjectDetector"     /* 物品遗留规则类型 */
#define OBJECT_REMOVAL_EVENT_RULE_TYPE      "tt:ObjectRemovalDetector"        /* 物品拿取规则类型 */
#define PET_RECOGNITION_EVENT_RULE_TYPE     "tt:PetRecognitionDetector"       /* 宠物识别规则类型 */
#define FACE_CAPTURE_EVENT_RULE_TYPE        "tt:FaceCaptureDetector"          /* 人脸抓拍规则类型 */

/* onvif分析模块名称 */
#define MOTION_EVENT_MODULE               "MyCellMotionModule"              /* 移动报警模块 */
#define TAMPEREVENT_MODULE                "MyTamperDetecModule"             /* 遮挡报警模块 */
#define LINE_EVENT_MODULE                 "MyLineDetectorModule"            /* 拌线报警模块 */
#define FIELD_EVENT_MODULE                "MyFieldDetectorModule"           /* 区域报警模块 */

/* 区域相关事件模块名称 */
#define ENTER_REGION_EVENT_MODULE        "MyRegionEnterDetectorModule"          /* 进入区域报警模块 */
#define LEAVE_REGION_EVENT_MODULE        "MyRegionLeaveDetectorModule"          /* 离开区域报警模块 */

/* 智能事件模块名称 */
#define AUDIO_ANOMALY_EVENT_MODULE       "MyAudioAnomalyDetectorModule"         /* 音频异常侦测模块 */
#define AUDIO_SUDDEN_RISE_EVENT_MODULE   "MyAudioSuddenRiseDetectorModule"      /* 音频异常-声强陡升模块 */
#define AUDIO_SUDDEN_DROP_EVENT_MODULE   "MyAudioSuddenDropDetectorModule"      /* 音频异常-声强陡降模块 */
#define SCENE_CHANGE_EVENT_MODULE        "MySceneChangeDetectorModule"          /* 场景变更模块 */
#define FACE_DETECT_EVENT_MODULE         "MyFaceDetectDetectorModule"           /* 人脸侦测模块 */
#define LOITERING_DETECT_EVENT_MODULE    "MyLoiteringDetectorModule"            /* 徘徊侦测模块 */
#define CROWD_GATHERING_EVENT_MODULE     "MyCrowdGatheringDetectorModule"       /* 人员聚集模块 */
#define PARKING_DETECT_EVENT_MODULE      "MyParkingDetectorModule"              /* 停车侦测模块 */
#define UNATTENDED_OBJECT_EVENT_MODULE   "MyUnattendedObjectDetectorModule"     /* 物品遗留模块 */
#define OBJECT_REMOVAL_EVENT_MODULE      "MyObjectRemovalDetectorModule"        /* 物品拿取模块 */
#define PET_RECOGNITION_EVENT_MODULE     "MyPetRecognitionDetectorModule"       /* 宠物识别模块 */
#define FACE_CAPTURE_EVENT_MODULE        "MyFaceCaptureDetectorModule"          /* 人脸抓拍模块 */

/* onvif分析模块类型 */
#define MOTION_EVENT_MODULE_TYPE               "tt:CellMotionEngine"                /* 移动报警模块 */
#define TAMPEREVENT_MODULE_TYPE                "tt:TamperEngine"                    /* 遮挡报警模块 */
#define LINE_EVENT_MODULE_TYPE                 "tt:LineDetectorEngine"              /* 拌线报警模块 */
#define FIELD_EVENT_MODULE_TYPE                "tt:FieldDetectorEngine"             /* 区域报警模块 */

/* 区域相关事件模块类型 */
#define ENTER_REGION_EVENT_MODULE_TYPE        "tt:RegionEnterEngine"          /* 进入区域报警模块类型 */
#define LEAVE_REGION_EVENT_MODULE_TYPE        "tt:RegionLeaveEngine"          /* 离开区域报警模块类型 */

/* 智能事件模块类型 */
#define AUDIO_ANOMALY_EVENT_MODULE_TYPE       "tt:AudioAnomalyEngine"         /* 音频异常侦测模块类型 */
#define AUDIO_SUDDEN_RISE_EVENT_MODULE_TYPE   "tt:AudioSuddenRiseEngine"      /* 音频异常-声强陡升模块类型 */
#define AUDIO_SUDDEN_DROP_EVENT_MODULE_TYPE   "tt:AudioSuddenDropEngine"      /* 音频异常-声强陡降模块类型 */
#define SCENE_CHANGE_EVENT_MODULE_TYPE        "tt:SceneChangeEngine"          /* 场景变更模块类型 */
#define FACE_DETECT_EVENT_MODULE_TYPE         "tt:FaceDetectEngine"           /* 人脸侦测模块类型 */
#define LOITERING_DETECT_EVENT_MODULE_TYPE    "tt:LoiteringEngine"            /* 徘徊侦测模块类型 */
#define CROWD_GATHERING_EVENT_MODULE_TYPE     "tt:CrowdGatheringEngine"       /* 人员聚集模块类型 */
#define PARKING_DETECT_EVENT_MODULE_TYPE      "tt:ParkingEngine"              /* 停车侦测模块类型 */
#define UNATTENDED_OBJECT_EVENT_MODULE_TYPE   "tt:UnattendedObjectEngine"     /* 物品遗留模块类型 */
#define OBJECT_REMOVAL_EVENT_MODULE_TYPE      "tt:ObjectRemovalEngine"        /* 物品拿取模块类型 */
#define PET_RECOGNITION_EVENT_MODULE_TYPE     "tt:PetRecognitionEngine"       /* 宠物识别模块类型 */
#define FACE_CAPTURE_EVENT_MODULE_TYPE        "tt:FaceCaptureEngine"          /* 人脸抓拍模块类型 */

/* 移动侦测网格布局 */
#define CELL_MOTION_COLUMNS     22  //网格列数
#define CELL_MOTION_ROWS        18  //网格行数
/* onvif移动侦测布局定义 */
#define ONVIF_CELL_MOTION_LAYOUT_MACRO \
    "<tt:CellLayout Columns=\"22\" Rows=\"18\">\r\n" \
      "<tt:Transformation>\r\n" \
        "<tt:Translate x=\"-1.000000\" y=\"-1.000000\" />\r\n" \
        "<tt:Scale x=\"0.090909\" y=\"0.111111\" />\r\n" \
      "</tt:Transformation>\r\n" \
    "</tt:CellLayout>\r\n" \

#define  ONVIF_CELLMOTION_MODULEPARAM_NUM 2   //移动侦测分析模块参数个数
#define  ONVIF_CELLMOTION_RULEPARAM_NUM 4   //移动侦测规则参数个数
/* 移动侦测规则参数名称 */
#define  ONVIF_CELLMOTION_RULEPARAM_1       "MinCount"
#define  ONVIF_CELLMOTION_RULEPARAM_2       "AlarmOnDelay"
#define  ONVIF_CELLMOTION_RULEPARAM_3       "AlarmOffDelay"
#define  ONVIF_CELLMOTION_RULEPARAM_4       "ActiveCells"

/* 移动侦测规则参数默认值 */
#define  ONVIF_CELLMOTION_RULEPARAM_VALUE1       "5"
#define  ONVIF_CELLMOTION_RULEPARAM_VALUE2       "1000"
#define  ONVIF_CELLMOTION_RULEPARAM_VALUE3       "1000"

/* 遮挡报警坐标转换定义 */
#define ONVIF_TAMPER_TRANSFORMATION_MACRO \
    "<tt:Transformation>\r\n" \
      "<tt:Translate x=\"-1.000000\" y=\"-1.000000\" />\r\n" \
      "<tt:Scale x=\"0.002841\" y=\"0.003472\" />\r\n" \
    "</tt:Transformation>\r\n"
/* 遮挡报警区域定义:
(0,0)：左上角
(0,576)：左下角
(704,576)：右下角
(704,0)：右上角 */
#define ONVIF_TAMPER_POLYGON_CONFIG_MACRO \
    "<tt:PolygonConfiguration>\r\n" \
      "<tt:Polygon>\r\n" \
        "<tt:Point x=\"0\" y=\"0\" />\r\n" \
        "<tt:Point x=\"0\" y=\"576\" />\r\n" \
        "<tt:Point x=\"704\" y=\"576\" />\r\n" \
        "<tt:Point x=\"704\" y=\"0\" />\r\n" \
      "</tt:Polygon>\r\n" \
    "</tt:PolygonConfiguration>\r\n"
#define  ONVIF_TAMPER_RULEPARAM_NUM 1   //遮挡报警规则参数个数
#define  ONVIF_TAMPER_MODULEPARAM_NUM 3   //遮挡报警分析模块参数个数

/* 区域入侵坐标转换定义 */
#define ONVIF_FIELD_TRANSFORMATION_MACRO \
    "<tt:Transformation>\r\n" \
      "<tt:Translate x=\"-1.000000\" y=\"-1.000000\" />\r\n" \
      "<tt:Scale x=\"0.001042\" y=\"0.001852\" />\r\n" \
    "</tt:Transformation>\r\n"
/* 区域入侵报警区域定义 */
#define ONVIF_FIELD_POLYGON_CONFIG_MACRO \
    "<tt:PolygonConfiguration>\r\n" \
      "<tt:Polygon>\r\n" \
        "<tt:Point x=\"0\" y=\"0\" />\r\n" \
        "<tt:Point x=\"0\" y=\"1080\" />\r\n" \
        "<tt:Point x=\"1920\" y=\"1080\" />\r\n" \
        "<tt:Point x=\"1920\" y=\"0\" />\r\n" \
      "</tt:Polygon>\r\n" \
    "</tt:PolygonConfiguration>\r\n"

/* 拌线入侵坐标转换定义 */
#define ONVIF_LINE_TRANSFORMATION_MACRO \
    "<tt:Transformation>\r\n" \
      "<tt:Translate x=\"-1.000000\" y=\"-1.000000\" />\r\n" \
      "<tt:Scale x=\"0.001042\" y=\"0.001852\" />\r\n" \
    "</tt:Transformation>\r\n"
/* 拌线入侵报警区域定义 */
#define ONVIF_LINE_POLYGON_CONFIG_MACRO \
    "<tt:PolygonConfiguration>\r\n" \
      "<tt:Polygon>\r\n" \
        "<tt:Point x=\"0\" y=\"0\" />\r\n" \
        "<tt:Point x=\"0\" y=\"1080\" />\r\n" \
        "<tt:Point x=\"1920\" y=\"1080\" />\r\n" \
        "<tt:Point x=\"1920\" y=\"0\" />\r\n" \
      "</tt:Polygon>\r\n" \
    "</tt:PolygonConfiguration>\r\n"

