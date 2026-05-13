/**
 * @file SavePcm.hpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-10-10
 *
 * @brief
 */

namespace Modules_NS
{
    /**
     * @brief 将音频数据追加保存成PCM文件
     * @param pData PCM数据
     * @param nSize PCM数据大小
     * @param strOutputPath 保存PCM的文件名
     * @return true 
     * @return false 
     */
    bool savePcm(const char *pData, const int nSize, const char *strOutputPath);
}