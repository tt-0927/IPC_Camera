#include "FaissDatabase.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <functional>
#include <random>

std::default_random_engine engine;
std::uniform_real_distribution<float> distribution(0.0, 1.0);


using namespace Modules_NS;


void add(CFaceDatabase &faceDb, int nNum)
{
    /* 开始时间点 */
    auto start = std::chrono::high_resolution_clock::now();

    /* 添加人脸向量和自定义 ID */
    for (int id = nNum; id > 0; id--)
    {
        std::vector<float> vector1(256);
        for (auto &v : vector1)
        {
            v = static_cast<float>(rand()) / RAND_MAX; // 生成随机向量
        }
        faceDb.add(id, vector1);
    }

    /* 结束时间点 */
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "当前数据库的向量数量: " << faceDb.getSize() << std::endl;
    // 输出执行时间
    std::cout << "添加数据耗时: " << duration.count() << " ms" << std::endl;
}

void addNormalize(CFaceDatabase &faceDb, int nNum)
{
    /* 开始时间点 */
    auto start = std::chrono::high_resolution_clock::now();

    /* 添加人脸向量和自定义 ID */
    for (int id = nNum; id > 0; id--)
    {
        std::vector<float> vector1(256);
        for (auto &v : vector1)
        {
            v = static_cast<float>(rand()) / RAND_MAX; // 生成随机向量
        }

        faceDb.normalize(vector1);
        faceDb.add(id, vector1);
    }

    /* 结束时间点 */
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "当前数据库的向量数量: " << faceDb.getSize() << std::endl;
    // 输出执行时间
    std::cout << "添加数据耗时: " << duration.count() << " ms" << std::endl;
}

void search(CFaceDatabase &faceDb)
{
    /* 开始时间点 */
    auto start = std::chrono::high_resolution_clock::now();

    /* 查询相似人脸 */
    std::vector<float> query_vector(256);
    for (auto &v : query_vector)
        v = static_cast<float>(distribution(engine));

    SearchResult_S stSearchRes;
    bool res = faceDb.search(query_vector, stSearchRes);
    if (res)
    {
        if (stSearchRes.vDistances.size() != 0 || stSearchRes.vIndices.size() != 0)
        {
            for (int nIndex = 0; nIndex < stSearchRes.vIndices.size(); nIndex++)
            {
                std::cout << nIndex << ": 最相似的是: " << stSearchRes.vIndices[nIndex] << "; 距离: " << stSearchRes.vDistances[nIndex] << std::endl;
            }
        }
        else
        {
            std::cout << "没找到相似的" << std::endl;
        }
    }

    /* 结束时间点 */
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // 输出执行时间
    std::cout << "查询数据耗时: " << duration.count() << " ms" << std::endl;
}

void searchNormalize(CFaceDatabase &faceDb)
{
    /* 开始时间点 */
    auto start = std::chrono::high_resolution_clock::now();

    /* 查询相似人脸 */
    std::vector<float> query_vector(256);
    for (auto &v : query_vector)
    {
        v = static_cast<float>(distribution(engine));
    }

    faceDb.normalize(query_vector);

    SearchResult_S stSearchRes;
    bool res = faceDb.search(query_vector, stSearchRes);
    if (res)
    {
        if (stSearchRes.vDistances.size() != 0 || stSearchRes.vIndices.size() != 0)
        {
            for (int nIndex = 0; nIndex < stSearchRes.vIndices.size(); nIndex++)
            {
                std::cout << nIndex << ": 最相似的是: " << stSearchRes.vIndices[nIndex] << "; 距离: " << stSearchRes.vDistances[nIndex] << std::endl;
            }
        }
        else
        {
            std::cout << "没找到相似的" << std::endl;
        }
    }

    /* 结束时间点 */
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // 输出执行时间
    std::cout << "查询数据耗时: " << duration.count() << " ms" << std::endl;
}

void save(CFaceDatabase &faceDb)
{
    /* 开始时间点 */
    auto start = std::chrono::high_resolution_clock::now();

    faceDb.save("face.index");

    /* 结束时间点 */
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // 输出执行时间
    std::cout << "保存索引耗时: " << duration.count() << " ms" << std::endl;
}

void load(CFaceDatabase &faceDb)
{
    /* 开始时间点 */
    auto start = std::chrono::high_resolution_clock::now();

    faceDb.load("face.index");
    std::cout << "重新加载数据库后的数量: " << faceDb.getSize() << std::endl;

    /* 结束时间点 */
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // 输出执行时间
    std::cout << "重新索引耗时: " << duration.count() << " ms" << std::endl;
}

void remove(CFaceDatabase &faceDb)
{
    /* 开始时间点 */
    auto start = std::chrono::high_resolution_clock::now();

    // 删除所有人脸
    std::vector<int64_t> vIds;
    faceDb.getIndexIDs(vIds);
    int nNums = vIds.size();
    if (nNums == 0)
    {
        printf("数据库为空，不能进行删除向量操作\n");
        return;
    }

    for (auto item : vIds)
    {
        printf("------删除ID[%d]-----\n", item);
        faceDb.remove(item);
    }
    std::cout << "删除所有向量后的数据库的数量: " << faceDb.getSize() << std::endl;

    /* 结束时间点 */
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // 输出执行时间
    std::cout << "删除" << nNums << "个向量共耗时: " << duration.count() << " ms; " << "平均耗时:" << duration.count() / nNums << " ms" << std::endl;
}

void clear(CFaceDatabase &faceDb)
{
    /* 开始时间点 */
    auto start = std::chrono::high_resolution_clock::now();

    // 清空数据库
    faceDb.clear();
    std::cout << "清空数据库后的数量: " << faceDb.getSize() << std::endl;

    /* 结束时间点 */
    auto end = std::chrono::high_resolution_clock::now();
    // 计算持续时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // 输出执行时间
    std::cout << "清空索引耗时: " << duration.count() << " ms" << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <function_index> [<additional_parameters>]" << std::endl;
        printf("输入的数据格式如下：\n 0 - 添加随机长度为256的向量\n 1 - 使用一个随机256的向量，进行搜索\n 2 - 加载索引\n 3 - 按照ID移除向量\n 4 - 清空数据\n 5 - 添加随机长度为256的（归一化）向量\n 6 - 使用一个随机256的（归一化）向量，进行搜索\n");

        return 1;
    }

    int functionIndex = std::stoi(argv[1]);

    /* 创建人脸数据库  */
    CFaceDatabase faceDb;
    faceDb.createDb(256, IndexFlatIP);

    // 检查 functionIndex 是否有效
    if (functionIndex < 0 || functionIndex > 6)
    {
        std::cerr << "Invalid function index" << std::endl;
        return 1;
    }

    // 调用选定的函数
    switch (functionIndex)
    {
    case 0:
        if (argc < 3)
        {
            std::cerr << "Add function requires a count parameter" << std::endl;
            return 1;
        }
        add(faceDb, std::stoi(argv[2]));
        save(faceDb);
        break;
    case 1:
        load(faceDb);
        search(faceDb);
        break;
    case 2:
        load(faceDb);
        break;
    case 3:
        load(faceDb);
        remove(faceDb);
        save(faceDb);
        break;
    case 4:
        load(faceDb);
        clear(faceDb);
        save(faceDb);
        break;
    case 5:
        addNormalize(faceDb, std::stoi(argv[2]));
        save(faceDb);
        break;
    case 6:
        load(faceDb);
        searchNormalize(faceDb);
        save(faceDb);
        break;
    default:
        std::cerr << "Invalid function index" << std::endl;
        return 1;
    }

    return 0;
}