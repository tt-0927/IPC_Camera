/**
 * @FilePath     : common_vlm.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-11-28 9:02:10
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-11-28 14:40:56
 * @Description  : visionText 公共处理函数(OpenCV处理相关)
 */


#include "common_vlm.hpp"
#include "event_vlm_manage.hpp"
#include "event_linkage.h"
#include <fstream>
#include <chrono>
#include <filesystem>


/**
 * @brief 将时间字符串从 "YYYYMMDD_HHMMSS" 格式转换为 "YYYY/MM/DD HH:MM:SS" 格式
 */
std::string CmProcess::convertTimeFormat(const std::string& strRecordCreateTime) 
{
    /*验证输入字符串长度*/
    if (strRecordCreateTime.length() != 15) {
        // 格式不正确，返回原字符串
        return strRecordCreateTime;
    }
    
    /* 提取年、月、日、时、分、秒*/
    std::string year = strRecordCreateTime.substr(0, 4);
    std::string month = strRecordCreateTime.substr(4, 2);
    std::string day = strRecordCreateTime.substr(6, 2);
    std::string hour = strRecordCreateTime.substr(9, 2);
    std::string minute = strRecordCreateTime.substr(11, 2);
    std::string second = strRecordCreateTime.substr(13, 2);
    
    /*  构建新格式的字符串*/
    std::string newFormat = year + "/" + month + "/" + day + " " + hour + ":" + minute + ":" + second;
    
    return newFormat;
}

int CmProcess::loadEncodeImage(const std::string& path, cv::Mat& image)
{
    std::lock_guard<std::mutex> lock(m_processMutex);

    try {
        m_cacheLoaded = cv::imread(path);
        
        if (m_cacheLoaded.empty()) {
            dlog_error("无法加载图像: %s", path.c_str());
            return -1;
        }

        dlog_debug("从本地加载图像: %s", path.c_str());

        // 统一转为 RGB (复用 m_cacheRgb)
        if (m_cacheLoaded.channels() == 3) {
            // 注意：imread 默认是 BGR
            cv::cvtColor(m_cacheLoaded, m_cacheRgb, cv::COLOR_BGR2RGB);
        } else if (m_cacheLoaded.channels() == 1) {
            cv::cvtColor(m_cacheLoaded, m_cacheRgb, cv::COLOR_GRAY2RGB);
        } else {
            m_cacheLoaded.copyTo(m_cacheRgb);
        }

        // 扩展为正方形并调整大小
        if (make_square_and_resize(m_cacheRgb, image) != OK) {
            dlog_error("Failed to process loaded image");
            return -1;
        }
    }
    catch (const cv::Exception& e) {
        dlog_error("OpenCV Exception inside loadEncodeImage: %s", e.what());
        return -1;
    }

    return 0;
}

/**
 * @brief 填充正方形 + 缩放，且复用内存
 * @param src 输入图像
 * @param dst 输入图像
 * @return 返回一个填充后的正方形图像
 */
int CmProcess::make_square_and_resize(const cv::Mat& src, cv::Mat& dst)
{
    if (src.empty()) return ERR;

    // 计算填充参数
    int width = src.cols;
    int height = src.rows;
    int size = std::max(width, height);
    
    int top = 0, bottom = 0, left = 0, right = 0;
    if (height > width) {
        left = (height - width) / 2;
        right = height - width - left;
    } else {
        top = (width - height) / 2;
        bottom = width - height - top;
    }

    cv::Scalar background_color(127.5, 127.5, 127.5);

    try {
        //  填充边框 (结果存入成员变量 m_cacheSquare)
        // 复用内存，避免了每次申请
        cv::copyMakeBorder(src, m_cacheSquare, top, bottom, left, right, cv::BORDER_CONSTANT, background_color);

        // 缩放到模型尺寸
        cv::Size new_size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT);
        cv::resize(m_cacheSquare, dst, new_size, 0, 0, cv::INTER_LINEAR);
    }
    catch (const cv::Exception& e) {
        dlog_error("OpenCV Error in make_square: %s", e.what());
        return ERR;
    }

    return OK;
}


int CmProcess::saveImage(const unsigned char* pSrcData, std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_processMutex);

    if (pSrcData == nullptr) {
        dlog_error("Input YUV data pointer is null"); 
        return ERR;
    } 

    try {

        if (m_cacheBgr.empty()) {
            m_cacheBgr = cv::Mat(PIXEL_HEIGHT_720, PIXEL_WIDTH_1280, CV_8UC3);
        }

        bool rga_ok = rga_image_transform(
            (void*)pSrcData, PIXEL_WIDTH_1280, PIXEL_HEIGHT_720,RK_FORMAT_YCbCr_420_SP, //NV12
            m_cacheBgr.data, PIXEL_WIDTH_1280, PIXEL_HEIGHT_720,RK_FORMAT_BGR_888  
        );

        if (!rga_ok) {
            dlog_error("RGA hardware process failed");
            return ERR;
        }

        std::vector<uchar> buf;
        std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 75 };

        if (!cv::imencode(".jpg", m_cacheBgr, buf, params)) {
            dlog_error("JPEG memory encode failed");
            return ERR;
        }

        FILE* fp = fopen(filename.c_str(), "wb");
        if (fp) {
            fwrite(buf.data(), 1, buf.size(), fp);
            fclose(fp);

        } else {
            dlog_error("Failed to open file for write: %s", filename.c_str());
            return ERR;
        }

        // 显式碎内存
        malloc_trim(0); 

    }
    catch (const cv::Exception& e) {
        dlog_error("OpenCV Exception in optimized saveImage: %s", e.what());
        return ERR;
    }

    return OK;
}

int CmProcess::yuv_convert_rgb(const unsigned char* pSrcData, cv::Mat& Desframe)
{
    std::lock_guard<std::mutex> lock(m_processMutex);

    if (pSrcData == nullptr)
    {
        dlog_error("Input YUV data pointer is null"); 
        return ERR;
    }

    try 
    {

        cv::Mat nv12_mat(PIXEL_HEIGHT_720 * 3 / 2, PIXEL_WIDTH_1280, CV_8UC1, (void*)pSrcData);

        cv::cvtColor(nv12_mat, m_cacheRgbMat, cv::COLOR_YUV2RGB_NV12); 

        if (m_cacheRgbMat.empty())
        {
            dlog_error("Failed to convert YUV to RGB");
            return ERR;
        }
        
        int h = m_cacheRgbMat.rows;
        int w = m_cacheRgbMat.cols;
        int top = 0, bottom = 0, left = 0, right = 0;

        // 计算需要填充的边距
        if (h > w) {
            left = (h - w) / 2;
            right = h - w - left;
        } else {
            top = (w - h) / 2;
            bottom = w - h - top;
        }

        cv::Scalar background_color(127.5, 127.5, 127.5);

        cv::copyMakeBorder(m_cacheRgbMat, m_cacheSquareMat, top, bottom, left, right, cv::BORDER_CONSTANT, background_color);

        if (m_cacheSquareMat.empty())
        {
            dlog_error("Failed to make border");
            return ERR;
        }

        cv::Size new_size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT);
        
        cv::resize(m_cacheSquareMat, Desframe, new_size, 0, 0, cv::INTER_LINEAR);

        if (Desframe.empty())
        {
            dlog_error("Failed to resize image to model input size");
            return ERR;
        }
    }
    catch (const cv::Exception& e)
    {
        dlog_error("OpenCV Exception in yuv_convert: %s (code: %d)", e.what(), e.code);
        
        if (e.code == cv::Error::StsNoMem) {
            dlog_error("OOM detected! Releasing caches.");
            m_cacheRgbMat.release();
            m_cacheSquareMat.release();
        }
        return ERR;
    }
    catch (const std::exception& e)
    {
        dlog_error("Standard Exception: %s", e.what());
        return ERR;
    }
    catch (...)
    {
        dlog_error("Unknown Exception occurred");
        return ERR;
    }

    return OK;
}

// 物理复制函数：将录像克隆到备份目录
std::string CmProcess::copyClosestTSFile(const std::string& recordDir, const std::string& createTime, const std::string& targetDir) 
{
    std::string sourcePath = findClosestTSByM3U8(recordDir, createTime);
    if (sourcePath.empty() || !std::filesystem::exists(sourcePath)) {
        dlog_error("AI备份失败：找不到源视频 %s", createTime.c_str());
        return "";
    }

    try {
        // 确保目标目录已创建
        if (!std::filesystem::exists(targetDir)) {
            std::filesystem::create_directories(targetDir);
        }

        std::error_code ec;
        // 获取当前可用空间
        auto spaceInfo = std::filesystem::space(targetDir, ec);
        
        // 仅在空间低于 300MB（低水位线）时，才触发批量清理
        if (!ec && spaceInfo.available < MIN_FREE_SPACE_BYTES) {
            dlog_warn("目标目录可用空间低于临界值 (%llu MB < %d MB)，开始批量清理旧视频...", 
                      spaceInfo.available / (1024 * 1024), MIN_FREE_SPACE_MB);

            // 扫描目录，获取所有 .ts 视频文件
            std::vector<std::filesystem::path> tsFiles;
            for (const auto& entry : std::filesystem::directory_iterator(targetDir, ec)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.length() >= 15 && entry.path().extension() == ".ts") {
                        tsFiles.push_back(entry.path());
                    }
                }
            }

            if (!ec && !tsFiles.empty()) {
                // 按文件名（时间戳）从小到大排序，最旧的文件在最前面
                std::sort(tsFiles.begin(), tsFiles.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
                    return a.filename().string() < b.filename().string();
                });

                // 批量删除，直到预估可用空间达到高水位线（500MB）
                uintmax_t estimatedAvailable = spaceInfo.available;
                for (const auto& file : tsFiles) {
                    if (estimatedAvailable >= SAFE_FREE_SPACE_BYTES) {
                        break; // 已恢复到安全空间，停止删除
                    }

                    // 获取当前文件大小用于预估空间恢复量
                    uintmax_t fileSize = 0;
                    auto fsize = std::filesystem::file_size(file, ec);
                    if (!ec) {
                        fileSize = fsize;
                    }

                    // 物理删除该文件
                    std::filesystem::remove(file, ec);
                    if (!ec) {
                        estimatedAvailable += fileSize; // 累加预估空间
                        dlog_info("批量清理旧视频成功: %s (%llu KB)", file.filename().string().c_str(), fileSize / 1024);
                    } else {
                        dlog_error("批量清理文件失败: %s, 错误: %s", file.c_str(), ec.message().c_str());
                        break; // 出现删不掉的情况（如只读等），立即中断，防止死循环
                    }
                }
            } else if (tsFiles.empty()) {
                dlog_warn("目标目录下无可清理的备份文件");
            }
        }

        // 执行视频文件复制
        std::filesystem::path src(sourcePath);
        std::string targetPath = targetDir + "/" + createTime + "_" + src.filename().string();

        std::filesystem::copy_file(sourcePath, targetPath, std::filesystem::copy_options::overwrite_existing);
        // 使用硬链接
        // std::error_code ec;
        // std::filesystem::create_hard_link(sourcePath, targetPath, ec);
        // if(ec) 
        // {
        //     dlog_error("硬链接失败 (可能跨分区): %s, 尝试降级为复制", ec.message().c_str());
        //     std::filesystem::copy_file(sourcePath, targetPath, std::filesystem::copy_options::overwrite_existing);
        // }

        dlog_info("AI视频已备份: %s", targetPath.c_str());
        return targetPath;
    } catch (const std::exception& e) {
        dlog_error("物理复制异常: %s", e.what());
        return "";
    }
}

// 删除文件函数：安全清理磁盘空间
void CmProcess::removePhysicalFiles(const std::string& imagePath, const std::string& videoPath) 
{
    try {
        //删除图片文件
        if (!imagePath.empty() && std::filesystem::exists(imagePath)) {
            std::filesystem::remove(imagePath);
            dlog_debug("删除过期图片: %s", imagePath.c_str());
        }
        
        // 删除视频备份 (安全校验：只删除包含 VLM_VIDEO 的路径)
        if (!videoPath.empty() && videoPath.find(VLM_VIDEO) != std::string::npos) {
            if (std::filesystem::exists(videoPath)) {
                std::filesystem::remove(videoPath);
                dlog_debug("删除过期备份视频: %s", videoPath.c_str());
            }
        }
    } catch (...) {}
}


std::string CmProcess::save_Detection_Image(const unsigned char* pImageData, size_t nImageSize, bool bTextSave)
{
    if (!pImageData || nImageSize == 0)
    {
        return "";
    }

    std::string strFinalDir = bTextSave ? EVENT_TEXT_ANALYSIS_PIC_PATH : EVENT_IMAGE_ANALYSIS_PIC_PATH;

    try {
        if (!std::filesystem::exists(strFinalDir)) {
            std::filesystem::create_directories(strFinalDir);

            std::string chmodCmd = "chmod 777 " + strFinalDir;
            system(chmodCmd.c_str());
        }
    } catch (const std::exception& e) {
        dlog_error("创建目录失败: %s", e.what());
        return "";
    }

    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    std::ostringstream timeStream;
    timeStream << std::put_time(localTime, "%Y%m%d_%H%M%S");
    std::string strFileName = timeStream.str() + ".jpeg";

    std::string strTmpPath = "/tmp/" + strFileName;     
    std::string strFinalPath = strFinalDir + strFileName; 

    if (saveImage(pImageData, strTmpPath) != OK) {
        dlog_error("写入 /tmp 失败");
        return "";
    }

  try {
        std::filesystem::copy_file(strTmpPath, strFinalPath, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(strTmpPath);
    } catch (const std::exception& e) {
        dlog_error("移动文件失败: %s", e.what());
        // 移动失败，清理临时文件
        if (std::filesystem::exists(strTmpPath)) {
            std::filesystem::remove(strTmpPath);
        }
        return "";
    }
    
    dlog_debug("图片已成功从内存转至: %s", strFinalPath.c_str());
    return strFinalPath;
}

std::string CmProcess::save_Jpeg_Buffer_Safe(const std::vector<unsigned char>& jpegData, bool bTextSave)
{
    if (jpegData.empty()) return "";

    std::string strFinalDir = bTextSave ? EVENT_TEXT_ANALYSIS_PIC_PATH : EVENT_IMAGE_ANALYSIS_PIC_PATH;

    try {
        if (!std::filesystem::exists(strFinalDir)) {
            std::filesystem::create_directories(strFinalDir);
            chmod(strFinalDir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO); 
        }
    } catch (const std::exception& e) {
        dlog_error("创建图片目录失败: %s", e.what());
        return "";
    }

    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    std::ostringstream timeStream;
    timeStream << std::put_time(localTime, "%Y%m%d_%H%M%S");
    std::string strFileName = timeStream.str() + ".jpeg";
    std::string strFinalPath = strFinalDir + strFileName;

    FILE* fp = fopen(strFinalPath.c_str(), "wb");
    if (fp) {
        size_t written = fwrite(jpegData.data(), 1, jpegData.size(), fp);
        fclose(fp);
        
        if (written != jpegData.size()) {
            dlog_error("图片写入不完整: %s", strFinalPath.c_str());
            std::filesystem::remove(strFinalPath);
            return "";
        }
    } else {
        dlog_error("无法打开文件进行写入: %s", strFinalPath.c_str());
        return "";
    }

    dlog_debug("图片已成功保存至: %s", strFinalPath.c_str());
    return strFinalPath;
}
