#include "ReadData.hpp"

std::vector<std::vector<std::string>> readFile(const std::string &sFileName)
{
    std::vector<std::vector<std::string>> vResults;
    std::ifstream file(sFileName);

    if (!file.is_open())
    {
        std::cerr << "Error opening file!" << std::endl;
        return vResults;
    }
    std::string sLine;
    while (std::getline(file, sLine))
    {
        /* 将逗号替换为空格，以便可以使用 stringstream 直接分割 */
        std::replace(sLine.begin(), sLine.end(), ',', ' ');

        std::vector<std::string> tokens;
        std::stringstream ss(sLine);
        std::string token;

        /* 使用空格分割字符串 */
        while (ss >> token)
        {
            tokens.push_back(token);
        }

        vResults.push_back(tokens);
    }

    file.close();
    return vResults;
}

/* 将一个双重容器的第一个元素，提取到一个新的容器中 */
void splitHead(std::vector<std::vector<std::string>> &vInput, std::vector<std::string> &vOutput)
{
    for (auto &row : vInput)
    {
        /* 确保行不为空 */
        if (!row.empty())
        {
            /* 将第一个元素插入里另外一个新容器 */
            vOutput.push_back(row.front());
            /* 移除每一行的第一个元素 */
            row.erase(row.begin());
        }
    }
}

/* 判断字符串是否为证书 */
bool isInteger(const std::string &str)
{
    char *end;
    errno = 0; // 重置 errno
    std::strtol(str.c_str(), &end, 10);
    return errno == 0 && *end == '\0' && end != str.c_str();
}

/* 判断字符串是否为float */
bool isFloat(const std::string &str)
{
    char *end;
    errno = 0; // 重置 errno
    std::strtof(str.c_str(), &end);
    return errno == 0 && *end == '\0' && end != str.c_str();
}

/* 判断字符串是否为double */
bool isDouble(const std::string &str)
{
    char *end;
    errno = 0; // 重置 errno
    std::strtod(str.c_str(), &end);
    return errno == 0 && *end == '\0' && end != str.c_str();
}

/* 等比例填充缩放 */
void resizeAndPadImage(cv::Mat inputImage, cv::Size ImgSize, cv::Mat &outputImage)
{
    int imageWidth = inputImage.cols;
    int imageHeight = inputImage.rows;
    float fResizeScale = static_cast<float>(ImgSize.width) / std::max(imageWidth, imageHeight);

    int newWidth = static_cast<int>(imageWidth * fResizeScale);
    int newHeight = static_cast<int>(imageHeight * fResizeScale);

    // std::cout << "计算后的缩放：" << newWidth << "x" << newHeight << std::endl;

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));
    cv::Mat output = cv::Mat(ImgSize.height, ImgSize.width, CV_8UC3, cv::Scalar(128, 128, 128));

    int nXOffset = static_cast<int>((ImgSize.width - newWidth) / 2);
    int nYOffset = static_cast<int>((ImgSize.height - newHeight) / 2);
    resizedImage.copyTo(output(cv::Rect(nXOffset, nYOffset, newWidth, newHeight)));
    outputImage = output;
}

/* 将容器里面的字符串，自适应转为整形 */
bool convertInt(std::vector<std::vector<std::string>> &vInput, std::vector<std::vector<int>> &vOutput)
{
    for (const auto &row : vInput)
    {
        std::vector<int> intRow; // 当前行的整型数组
        for (const auto &str : row)
        {
            try
            {
                // 将字符串转换为整型
                int value = std::stoi(str);
                intRow.push_back(value); // 添加到当前行
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "Invalid argument: " << str << " cannot be converted to an integer." << std::endl;
                return false;
            }
            catch (const std::out_of_range &e)
            {
                std::cerr << "Out of range: " << str << " is out of integer range." << std::endl;
                return false;
            }
        }
        vOutput.push_back(intRow);
    }
    return true;
}

/* 将容器里面的字符串，自适应转为浮点型 */
bool convertFloat(std::vector<std::vector<std::string>> &vInput, std::vector<std::vector<float>> &vOutput)
{
    for (const auto &row : vInput)
    {
        std::vector<float> intRow; // 当前行的整型数组
        for (const auto &str : row)
        {
            try
            {
                /* 将字符串转换为整型 */
                float value = std::stof(str);
                intRow.push_back(value);
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "Invalid argument: " << str << " cannot be converted to a float." << std::endl;
                return false;
            }
            catch (const std::out_of_range &e)
            {
                std::cerr << "Out of range: " << str << " is out of float range." << std::endl;
                return false;
            }
        }
        vOutput.push_back(intRow);
    }
    return true;
}

/*
int main()
{
    std::string filename = "data.txt";
    std::vector<std::vector<std::string>> data = readFile(filename);

    std::vector<std::string> vOutput;
    splitHead(data, vOutput);

    for (const auto &line : data)
    {
        for (int i = 0; i < line.size(); i++)
        {
            std::cout << line[i] << " ";
        }
        std::cout << std::endl;
    }
    printf("------------------------------------\n");
    std::vector<std::vector<int>> vOutputInt;
    bool bFlag = convertInt(data, vOutputInt);
    for (const auto &line : vOutputInt)
    {
        for (int i = 0; i < line.size(); i++)
        {
            std::cout << line[i] << " ";
        }
        std::cout << std::endl;
    }
    printf("------------------------------------\n");
    std::vector<std::vector<float>> vOutputFloat;
    bFlag = convertFloat(data, vOutputFloat);
    for (const auto &line : vOutputFloat)
    {
        for (int i = 0; i < line.size(); i++)
        {
            std::cout << line[i] << " ";
        }
        std::cout << std::endl;
    }

    printf("------------------------------------\n");
    for (int i = 0; i < vOutput.size(); i++)
    {
        std::cout << vOutput[i] << " ";
    }
    std::cout << std::endl;


    return 0;
}
*/