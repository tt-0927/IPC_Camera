/**
 * @FilePath     : face_capture_temp_file.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 15:07:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 16:23:49
 * @Description  : 人脸抓拍临时图片文件 RAII 管理
 */

#pragma once

#include <cstdio>
#include <string>

namespace FaceDetectInternal
{
class CFaceCaptureTempFile
{
public:
    /**
     * @brief   : 构造临时文件管理对象
     * @param    {std::string} &strFilePath：临时文件路径
     * @return   {void}
     */
    explicit CFaceCaptureTempFile(const std::string &strFilePath) : m_strFilePath(strFilePath)
    {
    }

    /**
     * @brief   : 析构时自动删除临时文件
     * @return   {void}
     */
    ~CFaceCaptureTempFile()
    {
        remove();
    }

    CFaceCaptureTempFile(const CFaceCaptureTempFile &) = delete;
    CFaceCaptureTempFile &operator=(const CFaceCaptureTempFile &) = delete;

    /**
     * @brief   : 获取临时文件路径
     * @return   {const std::string&} 临时文件路径
     */
    const std::string &path() const
    {
        return m_strFilePath;
    }

    /**
     * @brief   : 立即删除临时文件
     * @return   {void}
     */
    void remove()
    {
        if (!m_strFilePath.empty())
        {
            std::remove(m_strFilePath.c_str());
            m_strFilePath.clear();
        }
    }

private:
    /* 临时文件路径，析构或主动删除后清空，避免重复删除 */
    std::string m_strFilePath;
};
} // namespace FaceDetectInternal
