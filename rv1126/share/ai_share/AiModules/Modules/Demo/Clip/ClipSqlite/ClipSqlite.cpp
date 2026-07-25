#include "ClipSqlite.hpp"
#include <iostream>
#include <string>

// 初始化数据库和表
int initDatabase(sqlite3 *&db, const std::string &dbPath)
{
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return rc;
    }

    const char *sqlCreateTable = "CREATE TABLE IF NOT EXISTS images("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "address TEXT NOT NULL);";

    rc = sqlite3_exec(db, sqlCreateTable, nullptr, 0, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL 错误: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return rc;
    }

    return SQLITE_OK;
}

// 加载数据库的函数
int loadDatabase(sqlite3 *&db, const std::string &dbPath)
{
    // 打开数据库文件，如果文件不存在，则会创建新的数据库文件
    int result = sqlite3_open(dbPath.c_str(), &db);

    // 检查数据库是否成功打开
    if (result != SQLITE_OK)
    {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    std::cout << "数据库打开成功！" << std::endl;
    return SQLITE_OK;
}

// 插入图片信息
int insertImage(sqlite3 *db, const std::string &address)
{
    char *errMsg = nullptr;
    const char *sqlInsert = "INSERT INTO images(address) VALUES(?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sqlInsert, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db) << std::endl;
        return rc;
    }

    sqlite3_bind_text(stmt, 1, address.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::cerr << "插入数据失败: " << sqlite3_errmsg(db) << std::endl;
    }
    // else
    // {
    //     std::cout << "图片信息插入成功。" << std::endl;
    // }

    sqlite3_finalize(stmt);
    return rc;
}

// 查询图片信息
bool queryImages(sqlite3 *db, int imageId, std::string& address)
{
    if (!db)
    {
        std::cerr << "[数据库未初始化或已关闭！]" << std::endl;
        return false;
    }
    sqlite3_stmt *stmt;
    const char *sql = "SELECT address FROM images WHERE id = ?;";

    // 准备SQL语句
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 绑定图片ID参数
    sqlite3_bind_int(stmt, 1, imageId);

    // 执行查询
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        // 获取查询结果
        address = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        // std::cout << "图片地址: " << address << std::endl;
    }
    else if (rc == SQLITE_DONE)
    {
        std::cout << "没有找到ID为 " << imageId << " 的图片。" << std::endl;
    }
    else
    {
        std::cerr << "查询失败: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return true;
}

// int main()
// {
//     sqlite3 *db;
//     int rc = initDatabase(db, "image_database.db");
//     if (rc != SQLITE_OK)
//     {
//         return 1;
//     }

//     std::string imageAddress = "/home/sdb/csj/AI/PAR/temp.jpg";
//     insertImage(db, imageAddress);

//     queryImages(db, 0);

//     sqlite3_close(db);
//     return 0;
// }