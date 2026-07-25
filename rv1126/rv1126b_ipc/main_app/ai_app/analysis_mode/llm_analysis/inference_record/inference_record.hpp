/**
 * @FilePath     : inference_record.hpp
 * @Author       : leiyy
 * @Date         : 2025-09-15
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-09-15
 * @Description  : 推理记录数据
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "cJSON.h"
#include "dlog.h"

/**
 * @brief 推理记录数据结构体
 */
struct InferenceRecord {
    int id;                         // 记录ID
    std::string timestamp;          // 时间戳 (格式: YYYY-MM-DD HH:MM:SS)
    std::string input_text;         // 输入文本
    std::string output_text;        // 输出文本
    cv::Mat image;                  // 关联图像
    std::string image_path;         // 图像保存路径
    std::string model_type;         // 使用的模型类型
    float confidence;               // 置信度 (0.0-1.0)
    std::vector<std::string> tags;  // 标签，用于分类和搜索
    
    /**
     * @brief 默认构造函数
     */
    InferenceRecord();
    
    /**
     * @brief 带参构造函数
     * @param input 输入文本
     * @param output 输出文本
     * @param img 关联图像
     * @param model 模型类型
     * @param conf 置信度
     */
    InferenceRecord(const std::string& input, const std::string& output, 
                   const cv::Mat& img = cv::Mat(), const std::string& model = "LLM", 
                   float conf = 1.0f);
    
    /**
     * @brief 序列化为JSON字符串
     * @return JSON字符串
     */
    std::string serialize() const;
    
    /**
     * @brief 从JSON字符串反序列化
     * @param data JSON字符串
     * @return 成功返回true，失败返回false
     */
    bool deserialize(const std::string& data);
    
    /**
     * @brief 转换为cJSON对象
     * @return cJSON对象（需要手动释放）
     */
    cJSON* toCJSON() const;
    
    /**
     * @brief 从cJSON对象加载
     * @param j cJSON对象
     * @return 成功返回true，失败返回false
     */
    bool fromCJSON(const cJSON* j);
    
    /**
     * @brief 检查记录是否有效
     * @return 有效返回true，无效返回false
     */
    bool isValid() const;
    
    /**
     * @brief 获取简短的摘要信息
     * @return 摘要字符串
     */
    std::string getSummary() const;
    
    /**
     * @brief 添加标签
     * @param tag 标签
     */
    void addTag(const std::string& tag);
    
    /**
     * @brief 移除标签
     * @param tag 标签
     * @return 成功移除返回true，标签不存在返回false
     */
    bool removeTag(const std::string& tag);
    
    /**
     * @brief 检查是否包含指定标签
     * @param tag 标签
     * @return 包含返回true，否则返回false
     */
    bool hasTag(const std::string& tag) const;
    
    /**
     * @brief 清空所有标签
     */
    void clearTags();
    
    /**
     * @brief 获取当前时间戳
     * @return 时间戳字符串
     */
    static std::string getCurrentTimestamp();
};

/**
 * @brief 推理记录管理器类
 */
class InferenceHistory {
public:
    /**
     * @brief 构造函数
     * @param max_records 最大记录数，默认为10
     * @param storage_path 存储路径，默认为"/mnt/inference_records/"
     */
    explicit InferenceHistory(size_t max_records = 10, 
                             const std::string& storage_path = "/mnt/inference_records/");
    
    /**
     * @brief 析构函数
     */
    ~InferenceHistory();
    
    // 禁用拷贝构造函数和赋值运算符
    InferenceHistory(const InferenceHistory&) = delete;
    InferenceHistory& operator=(const InferenceHistory&) = delete;
    
    /**
     * @brief 添加推理记录
     * @param record 推理记录
     * @return 成功返回true，失败返回false
     */
    bool addRecord(const InferenceRecord& record);
    
    /**
     * @brief 获取所有记录
     * @return 记录列表
     */
    std::vector<InferenceRecord> getAllRecords() const;
    
    /**
     * @brief 根据ID获取记录
     * @param id 记录ID
     * @return 记录对象，如果不存在返回空对象
     */
    InferenceRecord getRecord(int id) const;
    
    /**
     * @brief 根据标签筛选记录
     * @param tag 标签
     * @return 匹配的记录列表
     */
    std::vector<InferenceRecord> getRecordsByTag(const std::string& tag) const;
    
    /**
     * @brief 根据时间范围筛选记录
     * @param start_time 开始时间 (格式: YYYY-MM-DD HH:MM:SS)
     * @param end_time 结束时间 (格式: YYYY-MM-DD HH:MM:SS)
     * @return 匹配的记录列表
     */
    std::vector<InferenceRecord> getRecordsByTime(
        const std::string& start_time, 
        const std::string& end_time) const;
    
    /**
     * @brief 根据关键词搜索记录
     * @param keyword 关键词
     * @param search_in_input 是否在输入文本中搜索
     * @param search_in_output 是否在输出文本中搜索
     * @return 匹配的记录列表
     */
    std::vector<InferenceRecord> searchRecords(
        const std::string& keyword,
        bool search_in_input = true,
        bool search_in_output = true) const;
    
    /**
     * @brief 清空所有记录
     */
    void clearAll();

    /**
     * @brief 保存记录到文件,不获取锁
     * @return 成功返回true，失败返回false
     */
    bool saveToFileNoLock() const;
    
    /**
     * @brief 保存记录到文件
     * @return 成功返回true，失败返回false
     */
    bool saveToFile() const;

    /**
     * @brief 从文件加载记录,不获取锁
     * @return 成功返回true，失败返回false
     */
    bool loadFromFileNoLock();
    
    /**
     * @brief 从文件加载记录
     * @return 成功返回true，失败返回false
     */
    bool loadFromFile();
    
    /**
     * @brief 获取记录数量
     * @return 记录数量
     */
    size_t getRecordCount() const;
    
    /**
     * @brief 设置最大记录数
     * @param max_records 最大记录数
     */
    void setMaxRecords(size_t max_records);
    
    /**
     * @brief 获取最大记录数
     * @return 最大记录数
     */
    size_t getMaxRecords() const;
    
    /**
     * @brief 获取存储路径
     * @return 存储路径
     */
    std::string getStoragePath() const;
    
    /**
     * @brief 设置存储路径
     * @param path 存储路径
     * @return 成功返回true，失败返回false
     */
    bool setStoragePath(const std::string& path);
    
    /**
     * @brief 导出记录到CSV文件
     * @param file_path CSV文件路径
     * @return 成功返回true，失败返回false
     */
    bool exportToCSV(const std::string& file_path) const;
    
private:
    // 生成唯一ID
    int generateId();
    
    // 保存图像到文件系统
    std::string saveImage(const cv::Mat& image, int record_id);
    
    // 加载图像从文件系统
    cv::Mat loadImage(const std::string& image_path) const;
    
    // 清理过期的图像文件
    void cleanupOldImages();
    
    // 确保存储目录存在
    bool ensureStorageDirectories() const;
    
    mutable std::mutex m_mutex;
    std::vector<InferenceRecord> m_records;
    size_t m_max_records;
    int m_next_id;
    std::string m_storage_path;
};