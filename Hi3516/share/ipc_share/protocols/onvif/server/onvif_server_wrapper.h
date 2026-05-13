/***
 * @FilePath     : onvif_server_wrapper.h
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2025-04-02 09:59:36
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-26 10:00:31
 * @Description  : onvif服务端调用接口封装
 */

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "soapH.h"
#include "edukit_network.h"
#include "wsaapi.h"
#include "wsseapi.h"
#include "httpda.h"
#include "dlog.h"
#include "share_define.h"
#include "share_os.h"

#include "onvif_type.h"
#include "onvif_token.h"
#include "onvif_Capabilities.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief onvif 鉴权
     * @param soap 
     * @return int 
     */
    int onvif_authentication(struct soap *soap);
    /***
    * @description : 判断ipv4地址合规性
    * @author      : huangjunda
    * @return       {int}
    */
    int onvif_isValidIpv4(const char *str);
    /**
     * @brief 获取http端口
     * @return int 
     */
    int onvif_get_httpPort();

    /***
    * @description : 获取mac地址
    * @author      : huangjunda
    * @return       {char *}
    */
    char *onvif_get_mac();

    /***
    * @description : 获取onvif认证模式
    * @author      : huangjunda
    * @return       {int}
    */
    int onvif_get_auth_mode();

    /***
    * @description : 验证onvif用户
    * @author      : huangjunda
    * @return       {int}
    */
    int onvif_vert_user(const char *pUser);

    /***
    * @description : 获取onvif用户
    * @author      : huangjunda
    * @return       {int}
    */
    char *onvif_get_user();

    /***
    * @description : 获取onvif密码
    * @author      : huangjunda
    * @return       {int}
    */
    char *onvif_get_passwd(char * pUser);

    /***
    * @description : 写日志
    * @author      : huangjunda
    * @param       {const char*} pclientIp - 客户端的IP地址字符串
    * @return       {int}
    */
    void onvif_write_log(int nType, int nAction, const char *pclientIp);

    /***
    * @description : 设备重启
    * @author      : huangjunda
    * @param       {struct soap*} soap - 当前请求的gSOAP上下文
    * @return       {int}
    */
    int onvif_reboot(struct soap *soap);

    /*** 
     * @description : 获取设备信息
     * @author      : huangjunda
     * @param        {OnvifDeviceInfo_t} *stInfo
     * @return       {int}
     */     
    int onvif_get_device_info(OnvifDeviceInfo_t *stInfo);

    /***
     * @description : 获取RTSP通道取流地址
     * @author      : lianghaoyao
     * @param        {int} - nChn 码流通道
     * @return       {char *} - 流地址
     */
    char *onvif_get_rtsp_url(int nChn);

    /***
     * @description : onvif设置IP地址
     * @author      : lianghaoyao
     * @param        {ONvifNetworkInfo_S} - stOnvifInfo 网络配置信息
     * @return       {int} - 0成功，其他失败
     */
    int onvif_set_ipAdress(ONvifNetworkInfo_S stOnvifInfo);

    int onvif_get_ipInfo(ONvifNetworkInfo_S *pstOnvifInfo);

    /***
     * @description : onvif设置网关
     * @author      : lianghaoyao
     * @param        {char *} - strGateway 网关
     * @return       {int} - 0成功，其他失败
     */
    int onvif_set_Gateway(char *strGateway);

    /***
     * @description : onvif获取ntp服务器地址
     * @author      : lianghaoyao
     * @param        {char *} - ntpServerAddress 网关
     * @param        {int} - nLen ntpServerAddress长度
     * @return       {int} - 0成功，其他失败
     */
    int onvif_get_ntpServerAddress(char *ntpServerAddress, int nLen);

    /***
     * @description : onvif获取profile参数
     * @author      : lianghaoyao
     * @param        {OnvifProfile_t *} - pstProfile 存放所需视频参数结构体指针
     * @param        {int} - nStreamNum 获取流下标
     * @return       {int} - 0成功，其他失败
     */
    int onvif_get_profileParam(OnvifProfile_t *pstProfile, int nStreamNum);

    /***
     * @description : onvif获取osd数量
     * @author      : lianghaoyao
     * @return       {int} - osd数量
     */
    int onvif_get_osdSize();

    /***
     * @description : onvif获取osd参数
     * @author      : lianghaoyao
     * @param        {OnvifOsdCfg_t *} - pstOsdCfg 存放osd参数结构体指针
     * @param        {int} - nSzie osd数量
     * @return       {int} - 0成功，其他失败
     */
    int onvif_get_osdParam(OnvifOsdCfg_t *pstOsdCfg, int *pnSzie);

    /***
     * @description : onvif设置osd参数
     * @author      : lianghaoyao
     * @param        {OnvifOsdCfg_t *} - pstOsdCfg 存放osd参数结构体指针
     * @return       {int} - 0成功，其他失败
     */
    int onvif_set_osdParam(OnvifOsdCfg_t *pstOsdCfg,char *pToken);

    /***
     * @description : onvif关闭指定osd
     * @author      : lianghaoyao
     * @param        {char *} - strOsdToken 请求关闭的osd的token
     * @return       {int} - 0成功，其他失败
     */
    int onvif_delete_osd(char *strOsdToken);
    /**
     * @brief 开启指定token
     * @param strOsdToken 
     * @return int 
     */
    int onvif_create_osd(OnvifOsdCfg_t *pstOsdCfg,char *pToken);

    /**
     * @brief 续订事件订阅
     * @param strAddress 订阅地址
     * @param nDurationSec 续订时长(秒)
     * @param pCurrentSec [输出]实际续订时长
     * @return int 0:成功, other:失败
     */
    int onvif_renew_subscription(const char* strAddress, int nDurationSec, int* pCurrentSec);

    /***
     * @description : onvif获取图像显示配置
     * @author      : lianghaoyao
     * @param        {OnvifImageParam_t *} - pOnvifImageParam 图像配置参数结构体
     * @return       {int} - 0成功，其他失败
     */
    int onvif_get_imageParam(OnvifImageParam_t *pOnvifImageParam);

    /***
     * @description : onvif设置图像显示配置
     * @author      : lianghaoyao
     * @param        {OnvifImageParam_t *} - pOnvifImageParam 图像配置参数结构体
     * @return       {int} - 0成功，其他失败
     */
    int onvif_set_imageParam(OnvifImageParam_t *pOnvifImageParam);

    /***
     * @description : onvif获取视频编码能力(选项) - 旧接口，默认返回第一个
     * @author      : lianghaoyao
     * @param        {OnvifVideoParam_t *} - pstVideoParams 视频参数结构体
     * @return       {int} - 0成功，其他失败
     */
    int onvif_get_video_capabilities(OnvifVideoParam_t *pstVideoParams, int nStreamNum);

    /***
     * @description : 获取指定码流支持的编码格式数量
     * @param        {int} nStreamNum 码流号
     * @return       {int} - 数量
     */
    int onvif_get_supported_codec_count(int nStreamNum);

    /***
     * @description : onvif获取指定索引的视频编码能力
     * @param        {OnvifVideoParam_t *} - pstVideoParams 视频参数结构体
     * @param        {int} nStreamNum 码流号
     * @param        {int} nCodecIndex 编码能力索引
     * @return       {int} - 0成功，其他失败
     */
    int onvif_get_video_capability_by_index(OnvifVideoParam_t *pstVideoParams, int nStreamNum, int nCodecIndex);

    /***
     * @description : onvif获取视频参数(当前配置)
     * @author      : lianghaoyao
     * @param        {OnvifVideoParam_t *} - pstVideoParams 视频参数结构体
     * @return       {int} - 0成功，其他失败
     */
    int onvif_get_videoParams(OnvifVideoParam_t *pstVideoParams, int nStreamNum);

    /***
     * @description : onvif设置视频参数
     * @author      : lianghaoyao
     * @param        {OnvifVideoParam_t *} - pstVideoParams 视频参数结构体
     * @return       {int} - 0成功，其他失败
     */
    int onvif_set_videoParams(OnvifVideoParam_t *pstVideoParams, int nStreamNum);

    /***
     * @description : onvif获取音频参数
     * @author      : lianghaoyao
     * @param        {OnvifAudioParam_t *} - pstAudioParams 视频参数结构体
     * @return       {int} - 0成功，其他失败
     */
    int onvif_get_audioParams(OnvifAudioParam_t *pstAudioParams);

    /***
     * @description : onvif设置音频参数
     * @author      : lianghaoyao
     * @param        {OnvifAudioParam_t *} - pstAudioParams 视频参数结构体
     * @return       {int} - 0成功，其他失败
     */
    int onvif_set_audioParams(OnvifAudioParam_t *pstAudioParams);

    /**
     * @brief 创建订阅地址
     * @param pAddress 订阅地址
     * @return int 
     */
    int onvif_create_subscription(const char* pAddress);

    /**
     * @brief 销毁订阅
     * @param pAddress 订阅地址
     */
    void onvif_destroy_subscription(const char* pAddress);

    /**
     * @brief 通过订阅地址获取事件报警
     * @param pAddress 订阅地址
     * @param pBatch 时间报警信息
     * @param nTimeoutMs 超时时长
     * @return int 
     */
    int onvif_pull_events(int socket_fd, const char* pAddress, OnvifAlarmEventBatch_S* pBatch, int nTimeoutMs);

    /**
     * @brief 获取移动侦测信息
     * @param pInfo 移动侦测信息
     * @return int 
     */
    int onvif_get_motion_info(OnvifMotionDetection_S *pInfo);
    /**
     * @brief 设置移动侦测分析信息
     * @param pInfo 移动侦测信息
     * @return int 
     */
    int onvif_set_motion_analytics(OnvifMotionDetection_S *pInfo);
     /**
     * @brief 设置移动侦测规则
     * @param pInfo 移动侦测信息
     * @return int 
     */
    int onvif_set_motion_rule(OnvifMotionDetection_S *pInfo);
    /**
     * @brief 获取遮挡报警数据信息
     * @param pInfo 遮挡报警信息
     * @return int 
     */
    int onvif_get_tamp_info(ONvifTamperDetection_S *pInfo);
    /**
     * @brief 设置遮挡报警分析信息
     * @param pInfo 遮挡报警信息
     * @return int 
     */
    int onvif_set_tamp_analytics(ONvifTamperDetection_S *pInfo);
    /**
     * @brief 设置遮挡报警分析信息
     * @param pInfo 遮挡报警信息
     * @return int 
     */
    int onvif_set_tamp_rule(ONvifTamperDetection_S *pInfo);

    /* New Analytics Getters/Setters */
    /* New Analytics Getters/Setters with Multi-Region Support */
    int onvif_get_enter_region_count();
    int onvif_get_enter_region_info(int nIndex, OnvifRegionDetection_S *pInfo);
    int onvif_set_enter_region_info(int nIndex, OnvifRegionDetection_S *pInfo);
    
    int onvif_get_leave_region_count();
    int onvif_get_leave_region_info(int nIndex, OnvifRegionDetection_S *pInfo);
    int onvif_set_leave_region_info(int nIndex, OnvifRegionDetection_S *pInfo);
    
    int onvif_get_audio_anomaly_count();
    int onvif_get_audio_anomaly_info(int nIndex, OnvifAudioAnomaly_S *pInfo);
    int onvif_set_audio_anomaly_info(int nIndex, OnvifAudioAnomaly_S *pInfo);

    int onvif_get_scene_change_count();
    int onvif_get_scene_change_info(int nIndex, OnvifSceneChange_S *pInfo);
    int onvif_set_scene_change_info(int nIndex, OnvifSceneChange_S *pInfo);

    int onvif_get_face_detect_count();
    int onvif_get_face_detect_info(int nIndex, OnvifFaceDetection_S *pInfo);
    int onvif_set_face_detect_info(int nIndex, OnvifFaceDetection_S *pInfo);

    int onvif_get_loitering_detect_count();
    int onvif_get_loitering_detect_info(int nIndex, OnvifLoiteringDetection_S *pInfo);
    int onvif_set_loitering_detect_info(int nIndex, OnvifLoiteringDetection_S *pInfo);

    int onvif_get_crowd_gathering_count();
    int onvif_get_crowd_gathering_info(int nIndex, OnvifCrowdGathering_S *pInfo);
    int onvif_set_crowd_gathering_info(int nIndex, OnvifCrowdGathering_S *pInfo);

    int onvif_get_parking_detect_count();
    int onvif_get_parking_detect_info(int nIndex, OnvifParkingDetection_S *pInfo);
    int onvif_set_parking_detect_info(int nIndex, OnvifParkingDetection_S *pInfo);

    int onvif_get_unattended_object_count();
    int onvif_get_unattended_object_info(int nIndex, OnvifUnattendedObject_S *pInfo);
    int onvif_set_unattended_object_info(int nIndex, OnvifUnattendedObject_S *pInfo);

    int onvif_get_object_removal_count();
    int onvif_get_object_removal_info(int nIndex, OnvifObjectRemoval_S *pInfo);
    int onvif_set_object_removal_info(int nIndex, OnvifObjectRemoval_S *pInfo);

    int onvif_get_pet_recognition_count();
    int onvif_get_pet_recognition_info(int nIndex, OnvifPetRecognition_S *pInfo);
    int onvif_set_pet_recognition_info(int nIndex, OnvifPetRecognition_S *pInfo);

    int onvif_get_face_capture_count();
    int onvif_get_face_capture_info(int nIndex, OnvifFaceCapture_S *pInfo);
    int onvif_set_face_capture_info(int nIndex, OnvifFaceCapture_S *pInfo);

    int onvif_get_tripwire_count();
    int onvif_get_tripwire_info(int nIndex, OnvifTripwireDetection_S *pInfo);
    int onvif_set_tripwire_info(int nIndex, OnvifTripwireDetection_S *pInfo);

    int onvif_get_intrusion_count();
    int onvif_get_intrusion_info(int nIndex, OnvifFieldDetection_S *pInfo);
    int onvif_set_intrusion_info(int nIndex, OnvifFieldDetection_S *pInfo);


#ifdef __cplusplus
}
#endif