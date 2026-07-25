#include <fstream>
#include <iostream>
#include <vector>

#include "SavePcm.hpp"

bool Modules_NS::savePcm(const char* pData, const int nSize, const char* strOutputPath)
{

    // 创建一个 ofstream 对象，并以追加模式打开文件
    std::ofstream file;
    file.open(strOutputPath, std::ios::binary | std::ios::app);  // 使用二进制模式和追加模式
    // 检查文件是否成功打开
    if (!file.is_open()) 
    {
        std::cerr << "无法打开文件: " << strOutputPath << std::endl;
        return false;
    }
    // 追加写入内容
    file.write(reinterpret_cast<const char*>(pData), nSize); // 写入二进制数据
    // 关闭文件
    file.close();

    return true;
}