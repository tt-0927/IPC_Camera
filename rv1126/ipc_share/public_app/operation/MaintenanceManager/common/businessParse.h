/*
 * @Author       : suzhl
 * @Date         : 2024-08-12 17:16:58
 * @LastEditors  :
 * @LastEditTime :
 * @FilePath     : businessParse.h
 * @Description  : 业务解析
 */

/**
 * @brief  解析需获取升级包数据
 * @param  [char*] pMessage - control发送来的message
 * @param  [int*] nType - 升级包类型
 * @param  [int*] nPageId - 获取升级包页码
 * @param  [char*] pKeyWord - 项目型号
 * @return [int] 请求结果： 成功返回0，失败-1
 * @author lixl
 * @note
 */
int parse_getPackInfo(const char *pMessage, int *nType, int *nPageId, char *pKeyWord, const int nLen);

/**
 * @brief  解析升级包下载路径
 * @param  [char*] pMessage - control发送来的message
 * @param  [int*] nId - 升级包ID
 * @param  [char*] pFilePath - 升级包下载路径
 * @param  [int] nLen - 下载进度
 * @return [int] 请求结果： 成功返回0，失败返回-1
 * @author lixl
 * @note
 */
int parse_uploadPack(const char *pMessage, int *nId, char *pFilePath, const int nLen);

/**
 * @brief  解析是否自动升级
 * @param  [char*] pMessage - control发送来的message
 * @param  [char*] pVersion - 当前版本
 * @param  [char*] pKeyWord - 项目型号
 * @return [int] 请求结果： 成功返回0，失败返回-1
 * @author lixl
 * @note
 */
int parse_autoUpdate(const char *pMessage, int *nType, char *pVersion, char *pKeyWord, const int nVersionSize, const int nKeyWordSize);