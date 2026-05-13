/*
 * @Description: 
 * @Author: yanzehui
 * @Date: 2022-06-02 09:07:40
 * @LastEditTime: 2022-06-02 09:18:35
 * @LastEditors: yanzehui
 * @Reference: 检查日志文件有效期
 */

#ifndef OS_SHARE_LOG_CHECK_INCLUDE_
#define OS_SHARE_LOG_CHECK_INCLUDE_

#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 
#include <dirent.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <sys/types.h>

#include <math.h>
#include <time.h>


#ifdef __cplusplus
extern "C"{
#endif
/*
* @description: 删除过期日志
* @param[in]: pLogPath: log的路径
* @param[in]: nSaveDays: 文件保存的天数
* @return: 0-成功  其他失败
* @others 其他说明
*/
int removeLogFile(char* pLogPath, int nSaveDays);


#ifdef __cplusplus
}
#endif

#endif //OS_SHARE_LOG_CHECK_INCLUDE_
