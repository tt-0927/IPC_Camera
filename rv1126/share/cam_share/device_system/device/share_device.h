/*
 * @FilePath: share_device.h
 * @Author: yangwenyao
 * @Date: 2022-11-22 16:47:08
 * @LastEditors: lianghaoyao 709692194@qq.com
 * @LastEditTime: 2025-02-07 14:37:14
 * @Descripttion: 
 */
#ifndef __SHARE_DEVICE__
#define __SHARE_DEVICE__

typedef struct
{
	char device_name[128];
	char company_name[128];
	char record_name[128];
    char software_version[128];
    char project_type[128];
} Device_Info_t;

/* 设备型号枚举值(NOTE:除TS_BUTT枚举值外，其他枚举不能设置枚举值) */
typedef enum DeviceID
{
    DEVICEID_TS_BUTT = 0, /* 未知型号 */
    DEVICEID_TV_6124HU,
    DEVICEID_TV_C204U,
    DEVICEID_TV_C304U,
    DEVICEID_TV_6204K,
    DEVICEID_TS_COUNT, /* 遍历上限值 */
} DeviceID_E;

/**
 * @brief  初始化当前设备型号，作本地记录缓存
 * @return [*]
 * @author EasonLu
 * @note   防止后续多次读取文件
 */
int share_init_currDeviceID();

/**
 * @brief  获取设备型号
 * @param  [DeviceID_E] *enDevID 设备型号枚举值
 * @return [*]
 * @author EasonLu
 * @note   可调用一次保存返回值，后续不用再读取文件获取
 */
DeviceID_E share_get_currDeviceID();

/**
 * @brief  获取设备信息
 * @param  [Device_Info_t] *device_info
 * @return [*]
 * @author EasonLu
 * @note
 */
int share_get_deviceInfo(Device_Info_t *device_info);

/**
 * @brief  设置设备信息
 * @param  [Device_Info_t] *device_info
 * @return [*]
 * @author EasonLu
 * @note
 */
int share_set_deviceInfo();

/**
 * @brief  获取特做设备信息
 * @param  [Device_Info_t] *device_info
 * @return [*]
 * @author EasonLu
 * @note
 */
int especial_get_deviceInfo(Device_Info_t *device_info);

/**
 * @brief  检查当前设备的宏定义型号
 * @param  [DeviceID_E] enDevID 设备型号枚举值
 * @return [int] 0:则不是当前设备型号 1:则是当前设备型号
 * @author EasonLu
 * @note
 */
int share_check_devIDByDefine(DeviceID_E enDevID);

/**
 * @brief  检查当前设备的文件型号
 * @param  [DeviceID_E] enDevID 设备型号枚举值
 * @return [int] 0:则不是当前设备型号 1:则是当前设备型号 -1:无设备型号文件
 * @author EasonLu
 * @note
 */
int share_check_devIDByFile(DeviceID_E enDevID);

#endif
