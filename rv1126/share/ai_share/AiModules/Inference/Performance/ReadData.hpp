#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cerrno> 
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp" 

/**
 * @brief 读取txt文件，使用,和空格分割数据
 * @param sFileName
 * @return std::vector<std::vector<std::string>>
 */
std::vector<std::vector<std::string>> readFile(const std::string &sFileName);

/**
 * @brief 判断字符串是否为证书
 * @param str
 * @return true
 * @return false
 */
bool isInteger(const std::string &str);

/**
 * @brief 判断字符串是否为float
 * @param str
 * @return true
 * @return false
 */
bool isFloat(const std::string &str);

/**
 * @brief 判断字符串是否为double
 * @param str
 * @return true
 * @return false
 */
bool isDouble(const std::string &str);

/**
 * @brief 等比例填充缩放
 * @param inputImage 
 * @param ImgSize 
 * @param outputImage 
 */
void resizeAndPadImage(cv::Mat inputImage,cv::Size ImgSize, cv::Mat &outputImage);

/**
 * @brief 将一个二维数组的每行第一个元素，单独保存一个容器，剩余的放到另外一个容器
 * @param vInput 需要拆分的二维数组
 * @param vOutput 第一个元素组成的新容器
 */
void splitHead(std::vector<std::vector<std::string>> &vInput, std::vector<std::string> &vOutput);

/**
 * @brief 将容器里面的字符串，自适应转为整形
 * @param vInput 输入数据
 * @param vOutput 转换后的数据
 * @return true
 * @return false
 */
bool convertInt(std::vector<std::vector<std::string>> &vInput, std::vector<std::vector<int>> &vOutput);

/**
 * @brief 将容器里面的字符串，自适应转为浮点型
 * @param vInput 输入数据
 * @param vOutput 转换后的数据
 * @return true
 * @return false
 */
bool convertFloat(std::vector<std::vector<std::string>> &vInput, std::vector<std::vector<float>> &vOutput);