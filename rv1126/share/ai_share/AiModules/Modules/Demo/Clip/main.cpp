/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-11
 *
 * @brief 图文检索
 */
#include <chrono>
#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include "opencv2/opencv.hpp"

#include "ClipImageV1_0.hpp"
#include "ClipTextV1_0.hpp"
#include "FaissDatabase.hpp"
#include "ClipSqlite.hpp"

#define DataSize 1024

/* 进度条 */
void showProgress(int current, int total)
{
    int bar_width = 50; /* 进度条的宽度 */
    float progress = static_cast<float>(current) / total;

    std::cout << "[";
    int pos = bar_width * progress;
    for (int i = 0; i < bar_width; ++i)
    {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% " << "[" << current << "/" << total << "]\r";
    std::cout.flush();
}

void pause()
{
    std::cout << "按任意键继续..." << std::endl;
    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略之前留在输入缓冲区的内容
    std::cin.get(); // 读取一个字符，但不显示在屏幕上
    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略之前留在输入缓冲区的内容
}

/* 创建图片保存的文件夹 */
bool createOrClearDirectory(const std::string &dir_path)
{
    std::filesystem::path path(dir_path);

    // 如果文件夹已存在，清空文件夹内容
    if (std::filesystem::exists(path))
    {
        try
        {
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                std::filesystem::remove_all(entry); // 删除文件或子目录
            }
            std::cout << "目标文件夹已清空: " << dir_path << std::endl;
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "清空文件夹失败: " << e.what() << std::endl;
            return false;
        }
    }
    else
    {
        // 如果文件夹不存在，创建新文件夹
        try
        {
            std::filesystem::create_directories(path);
            std::cout << "目标文件夹已创建: " << dir_path << std::endl;
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "创建文件夹失败: " << e.what() << std::endl;
            return false;
        }
    }
    return true;
}

// 文字检索函数（示例，需要实际实现）
bool textSearch(
    ClipText_NS::CClipTextV1_0 *textDemo,
    sqlite3 *pSqliteDb,
    Modules_NS::CFaceDatabase *pFeatureDb,
    std::string &query,
    int nSearchNum)
{
    std::string sDirPath = "Results"; /* 图片保存路径 */

    system("clear");
    std::cout << "调用文字检索接口，查询：" << query << std::endl;
    // 这里添加实际的文字检索代码
    /* 推理 */
    ClipText_NS::InData_S stTextInData;
    stTextInData.sText = query;
    std::vector<float> vTextResult;
    textDemo->process(stTextInData, vTextResult);
    pFeatureDb->normalize(vTextResult);

    /* 查询 */
    Modules_NS::SearchResult_S stSearchRes;
    bool res = pFeatureDb->search(vTextResult, stSearchRes, nSearchNum);
    if (res)
    {
        if (stSearchRes.vDistances.size() != 0 || stSearchRes.vIndices.size() != 0)
        {
            bool bF1 = createOrClearDirectory(sDirPath);
            if (!bF1)
            {
                printf("创建结果文件夹失败\n");
                pause();
                return false;
            }
            for (int nIndex = 0; nIndex < stSearchRes.vIndices.size(); nIndex++)
            {
                std::cout <<"[" << nIndex << "]  数据库ID: " << stSearchRes.vIndices[nIndex] << "; 相似度: " << stSearchRes.vDistances[nIndex] << std::endl;
                /* 查询Sqilte数据库 */
                std::string sImgPath;
                bool bF = queryImages(pSqliteDb, stSearchRes.vIndices[nIndex], sImgPath);
                /* 查询成功 */
                if (bF)
                {
                    try
                    {
                        std::string sSavePath = sDirPath + "/" + std::to_string(stSearchRes.vDistances[nIndex]) + ".jpg";
                        std::filesystem::copy_file(
                            sImgPath,
                            sSavePath,
                            std::filesystem::copy_options::overwrite_existing);
                        std::cout << sImgPath << " 文件已拷贝到: " << sSavePath << std::endl;
                    }
                    catch (const std::filesystem::filesystem_error &e)
                    {
                        std::cerr << "文件拷贝失败: " << e.what() << std::endl;
                        pause();
                        return false;
                    }
                }
            }
        }
        else
        {
            std::cout << "没找到相似的" << std::endl;
        }
    }

    pause();
    return true;
}

// 图片检索函数（示例，需要实际实现）
bool imageSearch(
    ClipImage_NS::CClipImageV1_0 *imageDemo,
    sqlite3 *pSqliteDb,
    Modules_NS::CFaceDatabase *pFeatureDb,
    std::string &imagePath,
    int nSearchNum)
{
    std::string sDirPath = "Results"; /* 图片保存路径 */

    system("clear");
    std::cout << "调用图片检索接口，图片路径：" << imagePath << std::endl;
    // 这里添加实际的图片检索代码

    /* 推理 */
    ClipImage_NS::InData_S stImageInData;
    /* 移除字符串中的所有空格 */
    imagePath.erase(std::remove(imagePath.begin(), imagePath.end(), ' '), imagePath.end());
    stImageInData.inMat = cv::imread(imagePath);
    if (stImageInData.inMat.empty())
    {
        printf("查找的图片不存在!\n");
        pause();
        return false;
    }
    std::vector<float> vImageResult;
    imageDemo->process(stImageInData, vImageResult);
    pFeatureDb->normalize(vImageResult);

    /* 查询 */
    Modules_NS::SearchResult_S stSearchRes;
    bool res = pFeatureDb->search(vImageResult, stSearchRes, nSearchNum);
    if (res)
    {
        if (stSearchRes.vDistances.size() != 0 || stSearchRes.vIndices.size() != 0)
        {
            bool bF1 = createOrClearDirectory(sDirPath);
            if (!bF1)
            {
                printf("创建结果文件夹失败\n");
                pause();
                return false;
            }
            for (int nIndex = 0; nIndex < stSearchRes.vIndices.size(); nIndex++)
            {
                std::cout << nIndex << ": 最相似的是: " << stSearchRes.vIndices[nIndex] << "; 距离: " << stSearchRes.vDistances[nIndex] << std::endl;
                /* 查询Sqilte数据库 */
                std::string sImgPath;
                bool bF = queryImages(pSqliteDb, stSearchRes.vIndices[nIndex], sImgPath);
                /* 查询成功 */
                if (bF)
                {
                    try
                    {
                        std::string sSavePath = sDirPath + "/" + std::to_string(stSearchRes.vDistances[nIndex]) + ".jpg";
                        std::filesystem::copy_file(
                            sImgPath,
                            sSavePath,
                            std::filesystem::copy_options::overwrite_existing);
                        std::cout << sImgPath << " 文件已拷贝到: " << sSavePath << std::endl;
                    }
                    catch (const std::filesystem::filesystem_error &e)
                    {
                        std::cerr << "文件拷贝失败: " << e.what() << std::endl;
                        pause();
                        return false;
                    }
                }
            }
        }
        else
        {
            std::cout << "没找到相似的" << std::endl;
        }
    }

    pause();
    return true;
}

/* 检索功能页面 */
void enterSearch(ClipImage_NS::CClipImageV1_0 *imageDemo,
                 ClipText_NS::CClipTextV1_0 *textDemo,
                 sqlite3 *pSqliteDb,
                 Modules_NS::CFaceDatabase *pFeatureDb)
{
    std::string userChoice;
    std::string textQuery;
    std::string imagePath;
    int nSearchNum = 1;

    while (true)
    {
        std::cout << "1、文字检索\n";
        std::cout << "2、图片检索\n";
        std::cout << "3、返回" << std::endl;

        std::cout << "请输入您的选择（1、2 或 3）：\n";
        // std::cin >> userChoice;
        std::getline(std::cin, userChoice, '\n');
        userChoice.erase(std::remove(userChoice.begin(), userChoice.end(), ' '), userChoice.end());

        if (userChoice == "1")
        {
            std::cout << "请查询的数量：";
            std::string sSearchNum;
            std::getline(std::cin, sSearchNum, '\n');
            sSearchNum.erase(std::remove(sSearchNum.begin(), sSearchNum.end(), ' '), sSearchNum.end());
            try
            {
                nSearchNum = std::stoi(sSearchNum);
            }
            catch (...)
            {
                printf("查询数量设置失败，设置为默认值：1\n");
            }

            std::cout << "请输入文字：";
            std::getline(std::cin, textQuery, '\n');
            bool bF = textSearch(textDemo, pSqliteDb, pFeatureDb, textQuery, nSearchNum); // 调用文字检索接口
            if (!bF)
            {
                continue;
            }
        }
        else if (userChoice == "2")
        {
            std::cout << "请查询的数量：";
            std::string sSearchNum;
            std::getline(std::cin, sSearchNum, '\n');
            sSearchNum.erase(std::remove(sSearchNum.begin(), sSearchNum.end(), ' '), sSearchNum.end());
            try
            {
                nSearchNum = std::stoi(sSearchNum);
            }
            catch (...)
            {
                printf("查询数量设置失败，设置为默认值：1\n");
            }

            std::cout << "请输入图片路径：";
            std::getline(std::cin, imagePath, '\n');
            bool bF = imageSearch(imageDemo, pSqliteDb, pFeatureDb, imagePath, nSearchNum); // 调用图片检索接口
            if (!bF)
            {
                continue;
            }
        }
        else if (userChoice == "3")
        {
            break;
        }
        else
        {
            std::cout << "无效的输入，请输入1或2。" << std::endl;
        }
        system("clear");
    }
}

/* 判断文件是否为图片文件（根据扩展名） */
bool is_image(const std::filesystem::path &file)
{
    std::string extension = file.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // 转换为小写字母
    return (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".bmp" || extension == ".gif");
}

/* 遍历目录并返回所有图片文件路径 */
std::vector<std::string> get_image_paths_from_directory(std::string &directory_path)
{
    std::vector<std::string> image_paths;

    // 检查目录是否有效
    if (!std::filesystem::exists(directory_path) || !std::filesystem::is_directory(directory_path))
    {
        std::cerr << "Invalid directory path: " << directory_path << std::endl;
        return image_paths; // 返回空容器
    }

    // 遍历目录
    for (auto &entry : std::filesystem::directory_iterator(directory_path))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            const std::filesystem::path &file_path = entry.path();
            if (is_image(file_path))
            {
                image_paths.push_back(file_path.string()); // 将图片路径加入容器
            }
        }
    }

    return image_paths; // 返回所有图片路径的容器
}

/* 加载图片文件夹，提取特征，保存数据库 */
bool detect_data(ClipImage_NS::CClipImageV1_0 *imageDemo)
{
    system("clear");
    std::string imagePath;
    std::cout << "请输入图片文件夹路径：";
    std::cin.ignore(); // 忽略之前读取的换行符
    std::getline(std::cin, imagePath, '\n');
    /* ========删除已存在的数据库 ============ */
    std::string sFeaturePath = imagePath + ".faiss";
    std::string sSqlPath = imagePath + ".db";
    // printf("sFeaturePath[%s],sSqlPath[%s]\n", sFeaturePath.c_str(), sSqlPath.c_str());
    if (std::remove(sSqlPath.c_str()) != 0)
    {
        std::perror("Sqlite数据库删除失败");
    }
    if (std::remove(sFeaturePath.c_str()) != 0)
    {
        std::perror("Faiss向量数据库删除失败");
    }

    /* 创建新的数据库 */
    sqlite3 *pSqliteDb = nullptr;
    int rc = initDatabase(pSqliteDb, sSqlPath.c_str());
    if (rc != SQLITE_OK)
    {
        printf("Sqlite数据库加载失败\n");
        return false;
    }
    Modules_NS::CFaceDatabase *pFeatureDb = new Modules_NS::CFaceDatabase();
    pFeatureDb->createDb(DataSize, Modules_NS::IndexFlatIP);
    // 这里可以添加提取特征的代码
    ClipImage_NS::InData_S stImageInData;
    std::vector<std::string> vImgPaths = get_image_paths_from_directory(imagePath);
    /* 遍历文件夹 */
    for (int nIndex = 0; nIndex < vImgPaths.size(); nIndex++)
    {
        /* 1、提取特征 */
        ClipImage_NS::InData_S stImageInData;
        stImageInData.inMat = cv::imread(vImgPaths[nIndex]);
        /* 推理 */
        std::vector<float> vImageResult;
        imageDemo->process(stImageInData, vImageResult);

        /* 2、保存Sqlite */
        insertImage(pSqliteDb, vImgPaths[nIndex]);

        /* 3、保存向量数据库 */
        pFeatureDb->normalize(vImageResult);
        pFeatureDb->add(nIndex + 1, vImageResult);

        showProgress(nIndex, vImgPaths.size());
    }
    /* 保存数据库 */
    char *charPath = strdup(sFeaturePath.c_str());
    pFeatureDb->save(charPath);

    /* 释放空间 */
    sqlite3_close(pSqliteDb);
    delete pFeatureDb;
    pause();
    return true;
}

/* 数据库加载 */
bool load_data(Modules_NS::CFaceDatabase *pFeatureDb, sqlite3 *&pSqliteDb)
{
    if (pSqliteDb)
    {
        sqlite3_close(pSqliteDb);
    }
    system("clear");
    std::string imagePath;
    std::cout << "请输入数据库路径：";
    std::cin.ignore(); // 忽略之前读取的换行符
    std::getline(std::cin, imagePath, '\n');
    // 这里可以添加加载数据库的代码
    std::string sFeaturePath = imagePath + ".faiss";
    char *charPath = strdup(sFeaturePath.c_str());
    pFeatureDb->load(charPath);
    std::string sSqlPath = imagePath + ".db";
    int rc = loadDatabase(pSqliteDb, sSqlPath.c_str());
    if (rc != SQLITE_OK)
    {
        printf("Sqlite数据库加载失败\n");
        return false;
    }
    std::cout << "数据库路径已设置为：" << imagePath << std::endl;
    printf("\n");
    pause();
    return true;
}

void exit()
{
    system("clear");
    std::cout << "程序已退出。" << std::endl;
}

int main()
{
    /* ------------------------模型初始化------------------------- */
    std::string imageModel = "img.fp32.rknn";
    std::string textModel = "txt.fp32.rknn";
    std::string sVocabPath = "cn_vocab.txt";

    /* 图片特征提取模型 */
    ClipImage_NS::InParam_S stImageInParam;
    stImageInParam.strModelPath = imageModel;
    ClipImage_NS::CClipImageV1_0 *imageDemo = new ClipImage_NS::CClipImageV1_0(stImageInParam);
    imageDemo->init();

    /* 文字特征提取模型 */
    ClipText_NS::InParam_S stTextInParam;
    stTextInParam.strModelPath = textModel;
    stTextInParam.sVocabPath = sVocabPath;
    ClipText_NS::CClipTextV1_0 *textDemo = new ClipText_NS::CClipTextV1_0(stTextInParam);
    textDemo->init();

    // /* 输入 */
    // ClipImage_NS::InData_S stImageInData;
    // ClipText_NS::InData_S stTextInData;
    /* ------------------------------------------=====----------- */
    /* ------------------------创建数据库------------------------- */
    /* 创建普通数据库 */
    sqlite3 *pSqliteDb = nullptr;
    /* 创建向量数据库  */
    Modules_NS::CFaceDatabase *pFeatureDb = new Modules_NS::CFaceDatabase();
    pFeatureDb->createDb(DataSize, Modules_NS::IndexFlatIP);
    /* ------------------------------------------=====----------- */

    /* 选择 */
    std::string userChoice;
    bool bFlag = false;
    while (true)
    {
        system("clear");

        // 打印指令
        std::cout << "1、进入检索页面\n";
        std::cout << "2、提取数据集特征\n";
        std::cout << "3、加载数据库\n";
        std::cout << "4、退出程序" << std::endl;

        // 等待用户输入指令
        std::cout << "请输入您的选择（1、2、3 或 4）\n";
        std::cin >> userChoice;

        // 根据用户输入执行相应操作
        if (userChoice == "1")
        {
            if (!bFlag)
            {
                printf("[未加载数据库]\n");
                continue;
            }
            enterSearch(imageDemo, textDemo, pSqliteDb, pFeatureDb);
        }
        else if (userChoice == "2")
        {
            bool bF2 = detect_data(imageDemo);
            if (!bF2)
            {
                printf("[图片数据写入数据库失败]\n");
                continue;
            }
        }
        else if (userChoice == "3")
        {
            bFlag = load_data(pFeatureDb, pSqliteDb);
            if (!pSqliteDb)
            {
                printf("[Sqlite数据库未加载成功]\n");
            }
        }
        else if (userChoice == "4")
        {
            exit();
            break; // 退出程序
        }
        else
        {
            std::cout << "无效的输入，请输入1、2、3或4。" << std::endl;
        }
    }

    /* 释放变量 */
    delete imageDemo;
    delete textDemo;
    delete pSqliteDb;
    delete pFeatureDb;
    return 0;
}