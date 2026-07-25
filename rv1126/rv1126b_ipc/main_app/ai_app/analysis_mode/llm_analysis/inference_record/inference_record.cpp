/**
 * @FilePath     : inference_record.cpp
 * @Author       : leiyy
 * @Date         : 2025-09-15
 * @LastEditors  : leiyy
 * @LastEditTime : 2025-09-15
 * @Description  : 推理记录数据
 */

#include "inference_record.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <iostream>

namespace fs = std::filesystem;

// InferenceRecord 实现

InferenceRecord::InferenceRecord()
    : id(0)
    , confidence(1.0f)
{
    timestamp = getCurrentTimestamp();
}

InferenceRecord::InferenceRecord(const std::string& input, const std::string& output, 
                               const cv::Mat& img, const std::string& model, float conf)
    : id(0)
    , input_text(input)
    , output_text(output)
    , image(img)
    , model_type(model)
    , confidence(conf)
{
    timestamp = getCurrentTimestamp();
}

std::string InferenceRecord::serialize() const {
    cJSON* json = toCJSON();
    if (!json) {
        return "";
    }
    
    char* json_str = cJSON_PrintUnformatted(json);
    std::string result(json_str);
    
    cJSON_free(json_str);
    cJSON_Delete(json);
    
    return result;
}

bool InferenceRecord::deserialize(const std::string& data) {
    cJSON* json = cJSON_Parse(data.c_str());
    if (!json) {
        std::cerr << "JSON解析错误" << std::endl;
        return false;
    }
    
    bool result = fromCJSON(json);
    cJSON_Delete(json);
    
    return result;
}

cJSON* InferenceRecord::toCJSON() const {
    cJSON* json = cJSON_CreateObject();
    if (!json) {
        return nullptr;
    }
    
    cJSON_AddNumberToObject(json, "id", id);
    cJSON_AddStringToObject(json, "timestamp", timestamp.c_str());
    cJSON_AddStringToObject(json, "input_text", input_text.c_str());
    cJSON_AddStringToObject(json, "output_text", output_text.c_str());
    cJSON_AddStringToObject(json, "image_path", image_path.c_str());
    cJSON_AddStringToObject(json, "model_type", model_type.c_str());
    cJSON_AddNumberToObject(json, "confidence", confidence);
    
    // 添加标签数组
    cJSON* tags_array = cJSON_CreateArray();
    if (tags_array) {
        for (const auto& tag : tags) {
            cJSON_AddItemToArray(tags_array, cJSON_CreateString(tag.c_str()));
        }
        cJSON_AddItemToObject(json, "tags", tags_array);
    }
    
    return json;
}

bool InferenceRecord::fromCJSON(const cJSON* json) {
    if (!json || !cJSON_IsObject(json)) {
        return false;
    }
    
    try {
        // 获取各个字段
        const cJSON* id_item = cJSON_GetObjectItemCaseSensitive(json, "id");
        if (cJSON_IsNumber(id_item)) {
            id = id_item->valueint;
        }
        
        const cJSON* timestamp_item = cJSON_GetObjectItemCaseSensitive(json, "timestamp");
        if (cJSON_IsString(timestamp_item)) {
            timestamp = timestamp_item->valuestring;
        }
        
        const cJSON* input_text_item = cJSON_GetObjectItemCaseSensitive(json, "input_text");
        if (cJSON_IsString(input_text_item)) {
            input_text = input_text_item->valuestring;
        }
        
        const cJSON* output_text_item = cJSON_GetObjectItemCaseSensitive(json, "output_text");
        if (cJSON_IsString(output_text_item)) {
            output_text = output_text_item->valuestring;
        }
        
        const cJSON* image_path_item = cJSON_GetObjectItemCaseSensitive(json, "image_path");
        if (cJSON_IsString(image_path_item)) {
            image_path = image_path_item->valuestring;
        }
        
        const cJSON* model_type_item = cJSON_GetObjectItemCaseSensitive(json, "model_type");
        if (cJSON_IsString(model_type_item)) {
            model_type = model_type_item->valuestring;
        }
        
        const cJSON* confidence_item = cJSON_GetObjectItemCaseSensitive(json, "confidence");
        if (cJSON_IsNumber(confidence_item)) {
            confidence = static_cast<float>(confidence_item->valuedouble);
        }
        
        // 处理标签数组
        tags.clear();
        const cJSON* tags_item = cJSON_GetObjectItemCaseSensitive(json, "tags");
        if (tags_item && cJSON_IsArray(tags_item)) {
            const cJSON* tag_item = nullptr;
            cJSON_ArrayForEach(tag_item, tags_item) {
                if (cJSON_IsString(tag_item)) {
                    tags.push_back(tag_item->valuestring);
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "JSON转换错误: " << e.what() << std::endl;
        return false;
    }
}

bool InferenceRecord::isValid() const {
    return !timestamp.empty() && (!input_text.empty() || !output_text.empty());
}

std::string InferenceRecord::getSummary() const {
    std::stringstream ss;
    ss << "ID: " << id << " | " << timestamp;
    
    if (!input_text.empty()) {
        std::string short_input = input_text.substr(0, 30);
        if (input_text.length() > 30) short_input += "...";
        ss << " | 输入: " << short_input;
    }
    
    if (!output_text.empty()) {
        std::string short_output = output_text.substr(0, 30);
        if (output_text.length() > 30) short_output += "...";
        ss << " | 输出: " << short_output;
    }
    
    return ss.str();
}

void InferenceRecord::addTag(const std::string& tag) {
    if (!hasTag(tag)) {
        tags.push_back(tag);
    }
}

bool InferenceRecord::removeTag(const std::string& tag) {
    auto it = std::find(tags.begin(), tags.end(), tag);
    if (it != tags.end()) {
        tags.erase(it);
        return true;
    }
    return false;
}

bool InferenceRecord::hasTag(const std::string& tag) const {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

void InferenceRecord::clearTags() {
    tags.clear();
}

std::string InferenceRecord::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

// InferenceHistory 实现

InferenceHistory::InferenceHistory(size_t max_records, const std::string& storage_path)
    : m_max_records(max_records)
    , m_next_id(1)
    , m_storage_path(storage_path)
{
    // 确保存储目录存在
    ensureStorageDirectories();
    
    // 尝试从文件加载记录
    loadFromFile();
}

InferenceHistory::~InferenceHistory() {
    // 自动保存记录到文件
    saveToFile();
}

bool InferenceHistory::addRecord(const InferenceRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 检查记录是否有效
    if (!record.isValid()) {
        std::cerr << "无效的记录，无法添加" << std::endl;
        return false;
    }

    // 如果达到最大记录数，移除最旧的记录
    if (m_records.size() >= m_max_records) {
        // 先移除关联的图像文件
        if (!m_records.front().image_path.empty()) {
            try {
                fs::remove(m_records.front().image_path);
            } catch (const std::exception& e) {
                std::cerr << "删除图像文件失败: " << e.what() << std::endl;
            }
        }
        m_records.erase(m_records.begin());
    }

    // 创建记录副本并分配ID
    InferenceRecord new_record = record;
    new_record.id = generateId();

    // 保存图像
    if (!new_record.image.empty()) {
        new_record.image_path = saveImage(new_record.image, new_record.id);
    }

    // 添加到记录列表
    m_records.push_back(new_record);

    // 自动保存到文件
    return saveToFileNoLock();
}

std::vector<InferenceRecord> InferenceHistory::getAllRecords() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 返回记录的副本
    std::vector<InferenceRecord> result;
    result.reserve(m_records.size());
    
    for (const auto& record : m_records) {
        result.push_back(record);
    }
    
    return result;
}

InferenceRecord InferenceHistory::getRecord(int id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& record : m_records) {
        if (record.id == id) {
            // 如果需要，加载图像
            InferenceRecord result = record;
            if (!result.image_path.empty() && result.image.empty()) {
                result.image = loadImage(result.image_path);
            }
            return result;
        }
    }
    
    return InferenceRecord{}; // 返回空记录
}

std::vector<InferenceRecord> InferenceHistory::getRecordsByTag(const std::string& tag) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<InferenceRecord> result;
    
    for (const auto& record : m_records) {
        if (record.hasTag(tag)) {
            result.push_back(record);
        }
    }
    
    return result;
}

std::vector<InferenceRecord> InferenceHistory::getRecordsByTime(
    const std::string& start_time, 
    const std::string& end_time) const {
    
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<InferenceRecord> result;
    
    for (const auto& record : m_records) {
        if (record.timestamp >= start_time && record.timestamp <= end_time) {
            result.push_back(record);
        }
    }
    
    return result;
}

std::vector<InferenceRecord> InferenceHistory::searchRecords(
    const std::string& keyword,
    bool search_in_input,
    bool search_in_output) const {
    
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<InferenceRecord> result;
    
    for (const auto& record : m_records) {
        bool match = false;
        
        if (search_in_input) {
            match = match || (record.input_text.find(keyword) != std::string::npos);
        }
        
        if (search_in_output && !match) {
            match = match || (record.output_text.find(keyword) != std::string::npos);
        }
        
        if (match) {
            result.push_back(record);
        }
    }
    
    return result;
}

void InferenceHistory::clearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 删除所有图像文件
    for (const auto& record : m_records) {
        if (!record.image_path.empty()) {
            try {
                fs::remove(record.image_path);
            } catch (const std::exception& e) {
                std::cerr << "删除图像文件失败: " << e.what() << std::endl;
            }
        }
    }
    
    m_records.clear();
    m_next_id = 1;
    
    // 删除存储文件
    try {
        fs::remove(m_storage_path + "history.json");
    } catch (const std::exception& e) {
        std::cerr << "删除历史文件失败: " << e.what() << std::endl;
    }
}

bool InferenceHistory::saveToFileNoLock() const {

    try {
        cJSON* json = cJSON_CreateObject();
        if (!json) {
            return false;
        }

        cJSON_AddNumberToObject(json, "max_records", static_cast<double>(m_max_records));
        cJSON_AddNumberToObject(json, "next_id", m_next_id);
 
        // 创建记录数组
        cJSON* records_array = cJSON_CreateArray();
        if (records_array) {
            for (const auto& record : m_records) {
                cJSON* record_json = record.toCJSON();
                if (record_json) {
                    cJSON_AddItemToArray(records_array, record_json);
                }
            }
            cJSON_AddItemToObject(json, "records", records_array);
        }

        // 将JSON转换为字符串
        char* json_str = cJSON_Print(json);
        if (!json_str) {
            cJSON_Delete(json);
            return false;
        }

        // 写入文件
        std::ofstream file(m_storage_path + "history.json");
        if (!file.is_open()) {
            cJSON_free(json_str);
            cJSON_Delete(json);
            std::cerr << "无法打开文件进行写入: " << m_storage_path + "history.json" << std::endl;
            return false;
        }

        file << json_str;
        file.close();

        // 释放内存
        cJSON_free(json_str);
        cJSON_Delete(json);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存记录到文件失败: " << e.what() << std::endl;
        return false;
    }
}

bool InferenceHistory::saveToFile() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    return saveToFileNoLock();
}


bool InferenceHistory::loadFromFileNoLock() {
    
    std::string file_path = m_storage_path + "history.json";
    if (!fs::exists(file_path)) {
        return true; // 文件不存在不是错误
    }
    
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "无法打开文件进行读取: " << file_path << std::endl;
            return false;
        }
        
        // 读取文件内容
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        // 解析JSON
        cJSON* json = cJSON_Parse(buffer.str().c_str());
        if (!json) {
            std::cerr << "JSON解析失败" << std::endl;
            return false;
        }
        
        // 获取基本字段
        const cJSON* max_records_item = cJSON_GetObjectItemCaseSensitive(json, "max_records");
        if (cJSON_IsNumber(max_records_item)) {
            m_max_records = static_cast<size_t>(max_records_item->valuedouble);
        }
        
        const cJSON* next_id_item = cJSON_GetObjectItemCaseSensitive(json, "next_id");
        if (cJSON_IsNumber(next_id_item)) {
            m_next_id = next_id_item->valueint;
        }
        
        // 获取记录数组
        m_records.clear();
        const cJSON* records_item = cJSON_GetObjectItemCaseSensitive(json, "records");
        if (records_item && cJSON_IsArray(records_item)) {
            const cJSON* record_item = nullptr;
            cJSON_ArrayForEach(record_item, records_item) {
                InferenceRecord record;
                if (record.fromCJSON(record_item)) {
                    m_records.push_back(record);
                }
            }
        }
        
        // 清理可能存在的无效图像文件
        cleanupOldImages();
        
        cJSON_Delete(json);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "从文件加载记录失败: " << e.what() << std::endl;
        return false;
    }
}

bool InferenceHistory::loadFromFile() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    return loadFromFileNoLock();
}

size_t InferenceHistory::getRecordCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records.size();
}

void InferenceHistory::setMaxRecords(size_t max_records) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_max_records = max_records;
    
    // 如果当前记录数超过新的最大值，移除最旧的记录
    while (m_records.size() > m_max_records) {
        if (!m_records.front().image_path.empty()) {
            try {
                fs::remove(m_records.front().image_path);
            } catch (const std::exception& e) {
                std::cerr << "删除图像文件失败: " << e.what() << std::endl;
            }
        }
        m_records.erase(m_records.begin());
    }
    
    // 保存更改
    saveToFileNoLock();
}

size_t InferenceHistory::getMaxRecords() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_max_records;
}

std::string InferenceHistory::getStoragePath() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_storage_path;
}

bool InferenceHistory::setStoragePath(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 检查路径是否有效
    try {
        if (!fs::exists(path)) {
            if (!fs::create_directories(path)) {
                std::cerr << "无法创建目录: " << path << std::endl;
                return false;
            }
        }
        
        // 保存当前记录
        if (!saveToFileNoLock()) {
            return false;
        }
        
        // 更新路径
        m_storage_path = path;
        if (m_storage_path.back() != '/') {
            m_storage_path += '/';
        }
        
        // 确保存储目录存在
        if (!ensureStorageDirectories()) {
            return false;
        }
        
        // 加载记录
        return loadFromFileNoLock();
    } catch (const std::exception& e) {
        std::cerr << "设置存储路径失败: " << e.what() << std::endl;
        return false;
    }
}

bool InferenceHistory::exportToCSV(const std::string& file_path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "无法打开CSV文件: " << file_path << std::endl;
            return false;
        }
        
        // 写入CSV头部
        file << "ID,Timestamp,Input,Output,Model,Confidence,Tags,ImagePath\n";
        
        // 写入记录
        for (const auto& record : m_records) {
            file << record.id << ",";
            file << "\"" << record.timestamp << "\",";
            file << "\"" << record.input_text << "\",";
            file << "\"" << record.output_text << "\",";
            file << "\"" << record.model_type << "\",";
            file << record.confidence << ",";
            
            // 标签
            file << "\"";
            for (size_t i = 0; i < record.tags.size(); ++i) {
                if (i > 0) file << ";";
                file << record.tags[i];
            }
            file << "\",";
            
            file << "\"" << record.image_path << "\"\n";
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "导出到CSV失败: " << e.what() << std::endl;
        return false;
    }
}

int InferenceHistory::generateId() {
    return m_next_id++;
}

std::string InferenceHistory::saveImage(const cv::Mat& image, int record_id) {
    std::string filename = m_storage_path + "images/" + std::to_string(record_id) + ".jpg";
    
    try {

         // 检查输入图像有效性
        if (image.empty()) {
            std::cerr << "输入图像无效" << std::endl;
            return "";
        }

        // 颜色空间转换：RGB→BGR（OpenCV默认保存格式为BGR）
        cv::Mat bgr_image;
        cv::cvtColor(image, bgr_image, cv::COLOR_RGB2BGR);

        // 使用适当的压缩参数保存图像
        std::vector<int> compression_params;
        compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        compression_params.push_back(95); // 高质量
        
        if (!cv::imwrite(filename, bgr_image, compression_params)) {
            std::cerr << "保存图像失败: " << filename << std::endl;
            return "";
        }
        
        return filename;
    } catch (const std::exception& e) {
        std::cerr << "保存图像异常: " << e.what() << std::endl;
        return "";
    }
}

cv::Mat InferenceHistory::loadImage(const std::string& image_path) const {
    try {
        cv::Mat image = cv::imread(image_path, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cerr << "加载图像失败或图像为空: " << image_path << std::endl;
        }
        return image;
    } catch (const std::exception& e) {
        std::cerr << "加载图像异常: " << e.what() << std::endl;
        return cv::Mat();
    }
}

void InferenceHistory::cleanupOldImages() {
    try {
        // 获取所有图像文件
        std::vector<std::string> image_files;
        std::string image_dir = m_storage_path + "images/";
        
        if (fs::exists(image_dir)) {
            for (const auto& entry : fs::directory_iterator(image_dir)) {
                if (entry.is_regular_file() && 
                    entry.path().extension() == ".jpg") {
                    image_files.push_back(entry.path().string());
                }
            }
        }
        
        // 获取当前记录中使用的图像文件
        std::vector<std::string> used_images;
        for (const auto& record : m_records) {
            if (!record.image_path.empty()) {
                used_images.push_back(record.image_path);
            }
        }
        
        // 删除未使用的图像文件
        for (const auto& image_file : image_files) {
            if (std::find(used_images.begin(), used_images.end(), image_file) == used_images.end()) {
                fs::remove(image_file);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "清理图像文件失败: " << e.what() << std::endl;
    }
}

bool InferenceHistory::ensureStorageDirectories() const {
    try {
        // 创建主存储目录
        if (!fs::exists(m_storage_path)) {
            if (!fs::create_directories(m_storage_path)) {
                std::cerr << "无法创建存储目录: " << m_storage_path << std::endl;
                return false;
            }
        }
        
        // 创建图像存储目录
        std::string image_dir = m_storage_path + "images/";
        if (!fs::exists(image_dir)) {
            if (!fs::create_directories(image_dir)) {
                std::cerr << "无法创建图像目录: " << image_dir << std::endl;
                return false;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "确保存储目录存在失败: " << e.what() << std::endl;
        return false;
    }
}