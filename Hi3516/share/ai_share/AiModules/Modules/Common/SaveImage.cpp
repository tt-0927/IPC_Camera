
#include "SaveImage.hpp"

bool Modules_NS::saveImage(const cv::Mat &image, const std::string &strOutputPath)
{
    struct stat info;
    /* 目录不存在 */
    if (stat(strOutputPath.c_str(), &info) != 0)
    {
        /* 使用命令 mkdir -p 来递归创建目录 */
        std::string strCmd = "mkdir -p \"" + strOutputPath + "\"";

        int nRet = system(strCmd.c_str());
        if (nRet != 0)
        {
            return false;
        }
    }

    /* 获取当前时间（精确到微秒） */
    auto now          = std::chrono::system_clock::now();
    auto time_t_now   = std::chrono::system_clock::to_time_t(now);
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

    /* 将 time_t 转换为本地时间 */
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time_t_now);
#else
    localtime_r(&time_t_now, &timeinfo);
#endif

    /* 格式化时间戳，精确到微秒 */
    std::ostringstream timestamp;
    timestamp << std::put_time(&timeinfo, "%Y%m%d_%H%M%S")
              << "_" << std::setw(6) << std::setfill('0') << microseconds.count();

    /* 构造完整的文件名（包含路径） */
    std::ostringstream filename;
    filename << strOutputPath << "/image_" << timestamp.str() << ".jpg";
    #ifdef RK_3588
    cv::Mat bgrImage;
    cv::cvtColor(image, bgrImage, cv::COLOR_RGB2BGR);
    bool bSaved = cv::imwrite(filename.str(), bgrImage);
    #else
    /* 使用 OpenCV 的 imwrite 函数保存图像 */
    bool bSaved = cv::imwrite(filename.str(), image);
    #endif
    return bSaved; /* 返回保存结果 */
}

bool Modules_NS::saveImage(const cv::Mat &image, const std::string &strOutputPath, int nChnId, int eventType, std::string& outFileName)
{
    struct stat info;
    
    /* 目录不存在 */
    std::string fullOutputPath = strOutputPath + "/D" + std::to_string(nChnId + 1); 
    if (stat(fullOutputPath.c_str(), &info) != 0)
    {
        /* 使用命令 mkdir -p 来递归创建目录 */
        std::string strCmd = "mkdir -p \"" + fullOutputPath + "\"";

        int nRet = system(strCmd.c_str());
        if (nRet != 0)
        {
            return false;
        }
    }

    /* 获取当前时间（精确到微秒） */
    auto now          = std::chrono::system_clock::now();
    auto time_t_now   = std::chrono::system_clock::to_time_t(now);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    /* 将 time_t 转换为本地时间 */
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time_t_now);
#else
    localtime_r(&time_t_now, &timeinfo);
#endif

    /* 格式化时间戳，精确到微秒 */
    std::ostringstream timestamp;
    timestamp << std::put_time(&timeinfo, "%Y%m%d_%H%M%S")
              << "_" << std::setw(3) << std::setfill('0') << milliseconds.count();

    /* 构造完整的文件名（包含路径） */
    std::ostringstream filename;
    filename << fullOutputPath << "/image_Chn" << nChnId << "_" << eventType << "_" << timestamp.str() << ".jpg";

    outFileName = filename.str();

    /* 格式兼容处理 */
    cv::Mat outputImage;
    if (image.channels() == 1)
    {
        /* YUV420格式 */
        cv::cvtColor(image, outputImage, cv::COLOR_YUV2BGR_I420);
    }
    else if (image.channels() == 3)
    {
        cv::cvtColor(image, outputImage, cv::COLOR_RGB2BGR);
    }
    else
    {
        std::cerr << "Error: Unsupported image format (channels=" 
                  << image.channels() << ")" << std::endl;
        
        return false;
    }
    
    // 根据图像大小自动计算压缩质量
    int quality = 85; // 默认质量
    int totalPixels = outputImage.cols * outputImage.rows;
    if (totalPixels >= 1920 * 1080) {
        quality = 75;
    } else if (totalPixels < 640 * 480) {
        quality = 95;
    }
    
    // 设置JPEG压缩参数
    std::vector<int> compression_params = {
        cv::IMWRITE_JPEG_QUALITY, quality,
        cv::IMWRITE_JPEG_OPTIMIZE, 1
    };
    
    // 尝试压缩并保存
    std::vector<uchar> buffer;
    if (cv::imencode(".jpg", outputImage, buffer, compression_params))
    {
        // 将压缩后的数据写入文件
        std::ofstream file(filename.str(), std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            file.close();
            printf("\033[34m %s:%d %s (compressed: %zu bytes, quality: %d) \033[m\n",
                   __func__, __LINE__, filename.str().c_str(), buffer.size(), quality);
            return true;
        }
    }
    
    /* 使用 OpenCV 的 imwrite 函数保存图像 */
    bool bSaved = cv::imwrite(filename.str(), outputImage);
    
    printf("\033[34m %s:%d %s \033[m\n",__func__,__LINE__,filename.str().c_str());
    return bSaved; /* 返回保存结果 */
}
