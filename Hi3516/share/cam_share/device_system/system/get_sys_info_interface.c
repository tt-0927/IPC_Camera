/*
 * @FilePath     : get_sys_info_interface.c
 * @Author       : zjc
 * @Date         : 2022-2-25 20:21:30
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-15 16:50:24
 * @Description  : 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "get_sys_info_interface.h"

static void convertMaxSizeUnit(char *out)
{
    if (out == NULL)
    {
        printf("parameter error\n");
        return;
    }
    char *unit = "KMGT";
    int index = 0;
    double tmp = atof(out);
    while (tmp / 1024 > 1)
    {
        tmp /= 1024;
        index++;
    }
    snprintf(out, sizeof(double) * 8, "%.1f%c", tmp, unit[index]);
    return;
}

int get_mem_info(MemInfo_S *pInfo)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp)
        return -1;

    char line[256];
    while (fgets(line, sizeof(line), fp))
    {
        if (strstr(line, "MemTotal:"))
        {
            sscanf(line, "MemTotal: %lu kB", &pInfo->ulTotal);
        }
        else if (strstr(line, "MemFree:"))
        {
            sscanf(line, "MemFree: %lu kB", &pInfo->ulFree);
        }
        else if (strstr(line, "Buffers:"))
        {
            sscanf(line, "Buffers: %lu kB", &pInfo->ulBuffers);
        }
        else if (strstr(line, "Cached:"))
        {
            sscanf(line, "Cached: %lu kB", &pInfo->ulCached);
        }
        else if (strstr(line, "MemAvailable:"))
        {
            sscanf(line, "MemAvailable: %lu kB", &pInfo->ulAvailable);
        }
    }
    fclose(fp);

    return 0;
}

double calculate_usage(MemInfo_S *pInfo)
{

    if (pInfo->ulAvailable > 0)
    {
        return 100.0 * (1.0 - (double)pInfo->ulAvailable / pInfo->ulTotal);
    }

    unsigned long used = pInfo->ulTotal - pInfo->ulFree - pInfo->ulBuffers - pInfo->ulCached;
    return 100.0 * ((double)used / pInfo->ulTotal);
}
