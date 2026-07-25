#include <sqlite3.h>
#include <iostream>
#include <string>

// 初始化数据库和表
int initDatabase(sqlite3 *&db, const std::string &dbPath);

// 插入图片信息
int insertImage(sqlite3 *db, const std::string &address);

// 查询图片信息
bool queryImages(sqlite3 *db, int imageId, std::string &address);

// 加载数据库的函数
int loadDatabase(sqlite3 *&db, const std::string &dbPath);