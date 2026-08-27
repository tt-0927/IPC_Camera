/**
 * @FilePath     : share_data.h
 * @Author       : zhouzirui
 * @Date         : 2024-09-04 17:37:23
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-27 17:42:21
 * @Description  : 公共数据定义
 */

#ifndef _SHARE_DATA_H_
#define _SHARE_DATA_H_

/*向上字节对齐 x:字节长度 a: 以多少字节对齐/对齐位数*/
#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
/*向下字节对齐*/
#define ALIGN_BACK(x, a) (((x) / (a) * (a)))

/*一次性读取文件 *pSize为读取的数据长度*/
#define BL_READ_FILE( path, pData, pSize ) \
    {   \
        FILE *pFile = NULL; \
        *pSize = 0; \
        if( 0 == access( path, F_OK )) \
        {\
            /*判断文件大小*/\
            struct stat fileStat;\
            if (stat( path, &fileStat) == -1) \
            {\
                printf("Failed to get file information.\n");\
            }\
            else \
            {\
                *pSize = fileStat.st_size;\
                pFile = fopen( path, "r");\
                if (NULL == pFile)\
                {\
                    printf("fopen %s file \n", path );\
                    *pSize = 0;\
                }\
                else\
                {\
                    pData = (char*) malloc ( *pSize );\
                    fread( pData, 1, nSize, pFile);\
                }\
            }\
            if( pFile )\
                fclose(pFile);\
        }\
    }
/*一次性读取文件*/

/* 音频数据线程优先级 */
#define AUDIO_DATA_THREAD_PRIORITY (70)
/* 视频数据线程优先级 */
#define VIDEO_DATA_THREAD_PRIORITY (65)

/*超时时间*/
#define TIMEOUT_0_MS (0)
#define TIMEOUT_100_MS (100)
#define TIMEOUT_150_MS (150)
#define TIMEOUT_500_MS (500)
#define TIMEOUT_1000_MS (1000)
/*AI检测超时时间*/
#define AI_DETECT_TIMEOUTMS (2000)

/*压缩模式*/
#define COMPRESSMODE COMPRESS_MODE_NONE

/**********************VPSS*******************/

/*主码流/子码流的vpss*/
#define VPSS_MAIN_SUB           (0)
#define VPSS_MAIN_SUB_CHN       (3)
/*VPSS总数*/
#define VPSS_GROUP_SUM          (1)

#define VPSS_CHANNEL_MAIN       (0)
#define VPSS_CHANNEL_SUB        (1)
#define VPSS_CHANNEL_AI         (2)
#define VPSS_CHANNEL_SUM        (3)

/* AI 通道分辨率  */
#define PIXEL_WIDTH_AI          (1920)
#define PIXEL_HEIGHT_AI         (1080)
/**********************VPSS*******************/

/**********************VO*******************/
/*视频层显示通道数*/
// #define VO_CHN_SUM              (1)
/**********************VENC*******************/

/**********************RGN***********************/
/*RGN绑定VPSS设备号*/
#define RGN_OSD_VPSS            (0)
/*RGN绑定VENC设备号*/
#define RGN_OSD_VENC            (0)
/*RGN绑定VPSS设备号*/
#define RGN_OSD_VPSS_DEVID           (0)
/*RGN绑定VPSS通道号*/
#define RGN_OSD_VPSS_CHNID           (0)

/* osd RGN模块最大个数 */
#define RGN_OSD_MAX_NUM             4

// note /* Hi3516CV610性能限制，隐私遮挡cover只保留一个 */
/* RGN模块遮挡属性最大个数 */
#define RGN_COVER_MAX_NUM           1
/* RGN模块遮挡属性最大个数 */
#define RGN_CORNER_RECT_MAX_NUM     3

/* osd 画布大小 */
#define RGN_OSD_WIDTH               (1248)
#define RGN_OSD_HEIGHT              (96)

/* osd 描绘时间的区域的大小 */
#define RGN_OSD_TIME_HANDLE         (0)
#define RGN_OSD_TIME_START          (100)
#define RGN_OSD_TIME_WIDTH          (432)
#define RGN_OSD_TIME_HEIGHT         (40)

/* osd 描绘IP的区域的大小 */
#define RGN_OSD_IP_HANDLE           (1)
#define RGN_OSD_IP_START            (0)
#define RGN_OSD_IP_TIMES            (60)
#define RGN_OSD_IP_WIDTH            (528)
#define RGN_OSD_IP_HEIGHT           (96)

/*MAC osd 画布大小 */
#define RGN_OSD_MAC_WIDTH           (352)
#define RGN_OSD_MAC_HEIGHT          (74)
#define RGN_OSD_MAC_ENGLISH_WIDTH   (640)
#define RGN_OSD_MAC_ENGLISH_HEIGHT  (74)
#define RGN_OSD_MAC_HANDLE          (2)
#define CN_MAC_TIPS                 "MAC地址未注册"
#define EN_MAC_TIPS                 "MAC address unregistered"

/*preset osd 画布大小 */
#define RGN_OSD_PRESET_WIDTH        (500)
#define RGN_OSD_PRESET_HEIGHT       (40)
#define RGN_OSD_PRESET_HANDLE       (3)
#define RGN_OSD_PRESET_START        (0)
#define RGN_OSD_PRESET_TIMES        (3)

#define CN_PRESET_OK                "成功"
#define CN_PRESET_FAIL              "失败"
#define CN_PRESET_TIPS              "设置%d号预置位%s"
#define EN_PRESET_OK                "OK"
#define EN_PRESET_FAIL              "fail"
#define EN_PRESET_TIPS              "Set preset %d %s"

#define CN_PEOPLE_TIPS              "人数：%d"
#define EN_PEOPLE_TIPS              "People:%d"

/* 人脸抓拍叠加信息-设备编号句柄号 */
#define RGN_DEVICE_ID_HANDLE           (0)
/* 人脸抓拍叠加信息-监控点信息句柄号 */
#define RGN_MONITORY_POINT_INFO_HANDLE (1)
/* 人脸抓拍叠加信息-抓拍时间句柄号 */
#define RGN_CAPTURE_TIME_HANDLE        (7)

/* 人脸抓拍叠加信息-信息垂直边距 */
#define RGN_INFO_VER_MARGIN            (64)
/* 人脸抓拍叠加信息-抓拍时间垂直边距 */
#define RGN_CAPTURE_TIME_VER_MARGIN    (1016)

/**********************RGN***********************/


/* 插件像素宽度默认值 */
#define PLUG_IN_WIDTH_DEFAULT PIXEL_WIDTH_1920
/* 插件像素高度默认值 */
#define PLUG_IN_HEIGHT_DEFAULT PIXEL_HEIGHT_1080

#endif

