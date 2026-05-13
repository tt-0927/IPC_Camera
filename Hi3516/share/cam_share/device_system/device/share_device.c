/*
 * @FilePath     : share_device.c
 * @Author       : zhouzirui
 * @Date         : 2025-02-08 15:49:46
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-03-04 14:46:13
 * @Description  : 设备相关共享定义
 */
#include "share_device.h"
#include "edukit_conf.h"
#include "share_define.h"
#include "dlog.h"
#include "xml_base.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if 1
#define COMPANY_INFO "ITC"
#ifdef TV_6124HU
#define DEVICE_INFO "TV-6124HU"
#define REDORD_INFO "tv_6124hu"
#elif defined(TV_C204U)
#define DEVICE_INFO "TV-C204U"
#define REDORD_INFO "tv_c204u"
#elif defined(TV_C304U)
#define DEVICE_INFO "TV-C304U"
#define REDORD_INFO "tv_c304u"
#elif defined(TV_6204K)
#define DEVICE_INFO "TV-6204K"
#define REDORD_INFO "tv_6204k"
#else
#define DEVICE_INFO "TV-XXXXX"
#define REDORD_INFO "tv_xxxxx"
#endif

/* 本地记录的当前设备ID，减少读取文件次数 */
DeviceID_E g_enDevID = DEVICEID_TS_BUTT;

/*
 * 返回1说明没有特做版本信息
 * 返回0说明有特做版本信息
 * 返回<0是其他错误
 *
 * */
int especial_get_deviceInfo(Device_Info_t *device_info)
{
	if (NULL == device_info)
	{
		dlog(LOG_ERROR, "share_get_deviceInfo param is err!\n");
		return -1;
	}

/*
	if (access(ESPECIALL_DEVICE_CONFIG, F_OK) == -1)
	{
		//不存在特做文件，所以是正常设备
		return 1; //
	}
*/
	/*
	if (TRUE != xml_get_charNode2("/root/Device_info/Device_name/", device_info->device_name, ESPECIALL_DEVICE_CONFIG, sizeof(device_info->device_name)) ||
		TRUE != xml_get_charNode2("/root/Device_info/Company_name/", device_info->company_name, ESPECIALL_DEVICE_CONFIG, sizeof(device_info->company_name)) ||
		TRUE != xml_get_charNode2("/root/Device_info/Record_name/", device_info->record_name, ESPECIALL_DEVICE_CONFIG, sizeof(device_info->record_name)))
	{
		return 1; //获取失败，正常设备
	}
	*/

	return 1;
}

int share_init_currDeviceID()
{
    g_enDevID = share_get_currDeviceID();
    return 0;
}

DeviceID_E share_get_currDeviceID()
{
    if (DEVICEID_TS_BUTT != g_enDevID && DEVICEID_TS_COUNT != g_enDevID)
    {
        return g_enDevID;
    }
    DeviceID_E enRet = DEVICEID_TS_BUTT;
    for (; enRet < DEVICEID_TS_COUNT; enRet++)
    {
        if (share_check_devIDByFile(enRet))
        {
            /* 更新全局记录，有可能不会调用share_init_currDeviceID() */
            g_enDevID = enRet;
            return enRet;
        }
    }
    return 0;
}

int share_get_deviceInfo(Device_Info_t *device_info)
{
	if (NULL == device_info)
	{
		dlog(LOG_ERROR, "share_get_deviceInfo param is err!\n");
		return -1;
	}

	if (access(DEVICE_CONFIG, F_OK) == -1)
	{
		FILE *fp = fopen(DEVICE_CONFIG, "w+");
		if (fp == NULL)
		{
			dlog(LOG_DEBUG, "share_get_deviceInfo fopen xml file failed\n");
			return -1;
		}
		else
		{
			dlog(LOG_DEBUG, "fuck share_get_deviceInfo\n");
			fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
			fprintf(fp, "<root>\n");
			fprintf(fp, "<Device_info>\n");
			fprintf(fp, "<Device_name>%s</Device_name>\n", DEVICE_INFO);
			fprintf(fp, "<Company_name>%s</Company_name>\n", COMPANY_INFO);
			fprintf(fp, "<Record_name>%s</Record_name>\n", REDORD_INFO);
			fprintf(fp, "<Software_version>%s</Software_version>\n", PREFIX_VERSION);
			fprintf(fp, "<Project_type>%s</Project_type>\n", PROJECT_TYPE);
			fprintf(fp, "</Device_info>\n");
			fprintf(fp, "</root>");
			fclose(fp);
		}
	}

	if (TRUE != xml_get_charNode2("/root/Device_info/Device_name/", device_info->device_name, DEVICE_CONFIG, sizeof(device_info->device_name)) ||
		TRUE != xml_get_charNode2("/root/Device_info/Company_name/", device_info->company_name, DEVICE_CONFIG, sizeof(device_info->company_name)) ||
		TRUE != xml_get_charNode2("/root/Device_info/Record_name/", device_info->record_name, DEVICE_CONFIG, sizeof(device_info->record_name)) ||
		TRUE != xml_get_charNode2("/root/Device_info/Software_version/", device_info->software_version, DEVICE_CONFIG, sizeof(device_info->software_version)) ||
		TRUE != xml_get_charNode2("/root/Device_info/Project_type/", device_info->project_type, DEVICE_CONFIG, sizeof(device_info->project_type)))
	{
		dlog(LOG_ERROR, "TiXmlMgr_SetConfValue is err!\n");
		memcpy(device_info->device_name, DEVICE_INFO, sizeof(DEVICE_INFO));
		memcpy(device_info->company_name, COMPANY_INFO, sizeof(COMPANY_INFO));
		memcpy(device_info->record_name, REDORD_INFO, sizeof(REDORD_INFO));
		memcpy(device_info->software_version, PREFIX_VERSION, sizeof(PREFIX_VERSION));
		memcpy(device_info->project_type, PROJECT_TYPE, sizeof(PROJECT_TYPE));
		return 0;
	}

	return 0;
}
int share_set_deviceInfo()
{
	if (access(DEVICE_CONFIG, F_OK) == -1)
	{
		FILE *fp = fopen(DEVICE_CONFIG, "w+");
		if (fp == NULL)
		{
			dlog(LOG_DEBUG, "share_set_deviceInfo fopen xml file failed\n");
			return -1;
		}
		else
		{
			dlog(LOG_DEBUG, "fuck share_set_deviceInfo\n");
			fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
			fprintf(fp, "<root>\n");
			fprintf(fp, "<Device_info>\n");
			fprintf(fp, "<Device_name>%s</Device_name>\n", DEVICE_INFO);
			fprintf(fp, "<Company_name>%s</Company_name>\n", COMPANY_INFO);
			fprintf(fp, "<Record_name>%s</Record_name>\n", REDORD_INFO);
			fprintf(fp, "<Software_version>%s</Software_version>\n", PREFIX_VERSION);
			fprintf(fp, "<Project_type>%s</Project_type>\n", PROJECT_TYPE);
			fprintf(fp, "</Device_info>\n");
			fprintf(fp, "</root>");
			fclose(fp);
		}
	}

	xml_set_charNode2("/root/Device_info/Device_name/", DEVICE_INFO, DEVICE_CONFIG);
	xml_set_charNode2("/root/Device_info/Company_name/", COMPANY_INFO, DEVICE_CONFIG);
	xml_set_charNode2("/root/Device_info/Record_name/", REDORD_INFO, DEVICE_CONFIG);
	xml_set_charNode2("/root/Device_info/Software_version/", PREFIX_VERSION, DEVICE_CONFIG);
	xml_set_charNode2("/root/Device_info/Project_type/", PROJECT_TYPE, DEVICE_CONFIG);

	return 0;
}

int share_check_devIDByDefine(DeviceID_E enDevID)
{
#ifdef DEVICE_TYPE
    switch (enDevID)
    {
        case DEVICEID_TV_6124HU:
        {
            if (strcmp(DEVICE_TYPE, DEVICE_INFO) == 0)
            {
                return 1;
            }
            break;
        }
		case DEVICEID_TV_C204U:
        {
            if (strcmp(DEVICE_TYPE, DEVICE_INFO) == 0)
            {
                return 1;
            }
            break;
        }
		case DEVICEID_TV_C304U:
        {
            if (strcmp(DEVICE_TYPE, DEVICE_INFO) == 0)
            {
                return 1;
            }
            break;
        }
		case DEVICEID_TV_6204K:
		{
            if (strcmp(DEVICE_TYPE, DEVICE_INFO) == 0)
            {
                return 1;
            }
            break;
        }
        default:
            break;
    }
#endif
    return 0;
}

int share_check_devIDByFile(DeviceID_E enDevID)
{
    if (access(DEVICE_CONFIG, F_OK) == -1)
    {
        dlog(LOG_ERROR, "无法获取设备信息文件");
        return -1;
    }
    char achDeviceName[128] = { 0 };
    xml_get_charNode2("/root/Device_info/Device_name/",
                      achDeviceName,
                      DEVICE_CONFIG,
                      sizeof(achDeviceName));
    switch (enDevID)
    {
        case DEVICEID_TV_6124HU:
        {
            if (strcmp(achDeviceName, DEVICE_INFO) == 0)
            {
                return 1;
            }
            break;
        }
		case DEVICEID_TV_C204U:
        {
            if (strcmp(achDeviceName, DEVICE_INFO) == 0)
            {
                return 1;
            }
            break;
        }
		case DEVICEID_TV_C304U:
        {
            if (strcmp(achDeviceName, DEVICE_INFO) == 0)
            {
                return 1;
            }
            break;
        }
		case DEVICEID_TV_6204K:
        {
            if (strcmp(achDeviceName, DEVICE_INFO) == 0)
            {
                return 1;
            }
            break;
        }
        default:
            break;
    }

    return 0;
}

#endif

