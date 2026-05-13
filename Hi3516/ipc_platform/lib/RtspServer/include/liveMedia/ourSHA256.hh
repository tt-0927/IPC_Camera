/**
* @FilePath     : ourSHA256.hh
* @Author       : liuhm
* @Date         : 2025-04-17 16:30:02
* @LastEditors  : liuhm
* @LastEditTime : 2025-04-17 16:30:02
* @Descripttion : 
*/

#ifndef _OUR_SHA256_HH
#define _OUR_SHA256_HH

/**
 * @brief       : 初始化音频流采集处理模块
 * @param        {要加密的数据}
 * @param        {要加密的数据长度}
 * @param        {加密输出后的数据}
 * @return       {void}
 */
void our_SHA256Data(unsigned char* data, unsigned len, char* output);

#endif
