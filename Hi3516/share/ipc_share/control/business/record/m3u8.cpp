#include "m3u8.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <ctime>
#include <iomanip>
#include "dlog.h"
#include <cstring>
#include <algorithm>

M3U8::M3U8()
{
    
}

M3U8::M3U8(std::string path)
    : m_path(path)
{
    if (!m_path.empty())
    {
        if (access(m_path.c_str(), F_OK) == 0)
        {
            m_isExit = true;
            parse();
        }
        else
        {
            m_isExit = false;
            create();
        }
    }
}

M3U8::~M3U8()
{
    /* 关闭文件 */
    if (m_outFile.is_open())
    {
        m_outFile.close();
    }
}

bool M3U8::is_exit()
{
    return m_isExit;
}

int M3U8::create()
{
    /* 创建输出文件流 */
    m_outFile.open(m_path, std::ios::in | std::ios::out | std::ios::app);
    if (!m_outFile.is_open())
    {
        dlog_error("open file failed: %s", m_path.c_str());
        return -1;
    }
    /* 文件头 */
    m_outFile << "#EXTM3U\n";
    m_outFile << "#EXT-X-VERSION:3\n";
    m_outFile << "#EXT-X-TARGETDURATION:10\n";
    m_outFile << "#EXT-X-MEDIA-SEQUENCE:0\n\n";

    m_outFile << "#EXT-X-ENDLIST\n";
    m_outFile.close();
    return 0;
}

int M3U8::add_ts(Data_S stData)
{
    std::ifstream inFile(m_path);
    if (!inFile.is_open())
    {
        std::cerr << "Failed to open file for reading: " << m_path << std::endl;
        return -1;
    }

    std::string content;
    std::string line;
    while (std::getline(inFile, line))
    {
        if (line != "#EXT-X-ENDLIST") // 忽略 ENDLIST 标记
        {
            content += line + "\n";
        }
    }
    inFile.close();

    // 重新写入文件
    m_outFile.open(m_path, std::ios::out | std::ios::trunc);
    if (!m_outFile.is_open())
    {
        dlog_error("open file failed: %s", m_path.c_str());
        return -1;
    }

    m_outFile << content;
    std::string data;
    data = "#EXT-X-PROGRAM-DATE-TIME:" + stData.startTime + "\n";
    m_outFile << data;
    data = "#EXTINF:" + std::to_string(stData.nDuration) + "\n";
    m_outFile << data;
    data = stData.filename + "\n";
    m_outFile << data;
    
    m_outFile << "#EXT-X-ENDLIST\n";
    m_outFile.flush();
    m_outFile.close();
    return 0;
}

int M3U8::write_head()
{
    if (!m_outFile.is_open())
    {
        dlog_error("open file failed: %s", m_path.c_str());
        return -1;
    }
    /* 文件头 */
    m_outFile << "#EXTM3U\n";
    m_outFile << "#EXT-X-VERSION:3\n";
    m_outFile << "#EXT-X-TARGETDURATION:10\n";
    m_outFile << "#EXT-X-MEDIA-SEQUENCE:0\n\n";

    m_outFile << "#EXT-X-ENDLIST\n";
    return 0;
}

int M3U8::write_data(M3U8::Data_S stData)
{
    if (!m_outFile.is_open())
    {
        dlog_error("open file failed: %s", m_path.c_str());
        return -1;
    }
    std::string data;
    data = "#EXT-X-PROGRAM-DATE-TIME:" + stData.startTime + "\n";
    m_outFile << data;
    data = "#EXTINF:" + std::to_string(stData.nDuration) + "\n";
    m_outFile << data;
    data = stData.filename + "\n";
    m_outFile << data;
    return 0;
}

int M3U8::write_tail()
{
    if (!m_outFile.is_open())
    {
        dlog_error("open file failed: %s", m_path.c_str());
        return -1;
    }
    /* 文件尾巴 */
    m_outFile << "#EXT-X-ENDLIST\n";
    return 0;
}

// 去除字符串首尾的空白字符
static std::string trim(const std::string& str) 
{
    if (str.empty())
    {
        return "";
    } 

    size_t nStart = 0;
    size_t nEnd = str.size() - 1;

    // 移除开头空白
    while (nStart <= nEnd && std::isspace(static_cast<unsigned char>(str[nStart]))) 
    {
        nStart++;
    }

    // 移除结尾空白
    while (nEnd >= nStart && std::isspace(static_cast<unsigned char>(str[nEnd]))) 
    {
        nEnd--;
    }

    return str.substr(nStart, nEnd - nStart + 1);
}

// 解析M3U8文件并提取.ts文件名
std::vector<std::string> M3U8::get_M3u8TsFileName(const std::string& strFilePath) 
{
    std::vector<std::string> strTsFiles;
    std::ifstream file(strFilePath.c_str());
    if (!file.is_open())
    {
        dlog_error("无法打开文件:%s", strFilePath.c_str());
        return strTsFiles;
    }

    std::string strLine;
    while (std::getline(file, strLine)) 
    {
        std::string strTrimmedLine = trim(strLine);

        // 跳过空行和注释行（以#开头）
        if (strTrimmedLine.empty() || strTrimmedLine[0] == '#') 
        {
            continue;
        }

        // 直接添加非注释行（即.ts文件名）
        strTsFiles.push_back(strTrimmedLine);
    }

    return strTsFiles;
}

int M3U8::parse_time(const std::string &timeStr)
{
    // 时间格式严格校验（确保为"YYYY-MM-DD HH:MM:SS"）
    if (timeStr.length() < 19)
    {
        dlog_error("时间字符串过短: %s", timeStr.c_str());
        return -1;
    }

    if (timeStr[4] != '-' || timeStr[7] != '-' || 
        timeStr[10] != ' ' || timeStr[13] != ':' || timeStr[16] != ':')
    {
        dlog_error("时间格式无效 (应为YYYY-MM-DD HH:MM:SS): %s", timeStr.c_str());
        return -1;
    }

    // 直接解析时分秒（忽略年月日，仅计算当天秒数）
    int hour = atoi(timeStr.substr(11, 2).c_str());
    int minute = atoi(timeStr.substr(14, 2).c_str());
    int second = atoi(timeStr.substr(17, 2).c_str());

    // 边界检查（确保时间值合法）
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
    {
        dlog_error("时间值越界: %d:%d:%d", hour, minute, second);
        return -1;
    }

    // 计算当天总秒数（核心：直接返回时分秒对应的秒数）
    return hour * 3600 + minute * 60 + second;
}

std::vector<Record_NS::VideoTime_S> M3U8::get_videoTime()
{
    return m_videoTimes;
}

int M3U8::parse()
{
    if (m_path.empty())
    {
        dlog_error("文件路径为空");
        return -1;
    }

    // 一次性读取文件内容（减少I/O操作）
    std::ifstream file(m_path, std::ios::binary);
    if (!file.is_open())
    {
        dlog_error("打开文件失败: %s", m_path.c_str());
        return -1;
    }

    // 获取文件大小并预分配缓冲区
    file.seekg(0, std::ios::end);
    size_t file_size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    // 如果文件为空，直接返回
    if (file_size == 0) 
    {
        file.close();
        return 0;
    }

    std::string file_content;
    file_content.resize(file_size);
    file.read(&file_content[0], file_size);
    file.close();

    // 分割行（兼容LF和CRLF换行符）
    std::vector<std::string_view> lines;
    size_t pos = 0;
    size_t start = 0;
    const size_t content_len = file_content.size();
    while (pos < content_len)
    {
        if (file_content[pos] == '\n' || file_content[pos] == '\r')
        {
            if (pos > start) // 跳过空行
            {
                lines.emplace_back(&file_content[start], pos - start);
            }
            start = pos + 1;
            // 处理CRLF中的多余回车符
            if (pos + 1 < content_len && file_content[pos] == '\r' && file_content[pos + 1] == '\n')
            {
                pos++;
            }
        }
        pos++;
    }
    // 处理最后一行（无换行符的情况）
    if (start < content_len)
    {
        lines.emplace_back(&file_content[start], content_len - start);
    }

    // 定义M3U8标签常量
    const std::string_view PDT_TAG = "#EXT-X-PROGRAM-DATE-TIME:";
    const size_t PDT_LEN = PDT_TAG.size();
    const std::string_view INF_TAG = "#EXTINF:";
    const size_t INF_LEN = INF_TAG.size();
    const std::string_view ST_TAG = "#START-TIME:";
    const size_t ST_LEN = ST_TAG.size();

    // 清空容器
    m_segmentTimes.clear();
    m_segmentDurations.clear();
    m_videoTimes.clear();

    // 解析每行内容，提取片段时间和时长
    for (const auto& line_sv : lines)
    {
        if (line_sv.empty()) continue;

        // 提取片段开始时间（#EXT-X-PROGRAM-DATE-TIME）
        if (line_sv.size() >= PDT_LEN && line_sv.substr(0, PDT_LEN) == PDT_TAG)
        {
            std::string time_str(line_sv.substr(PDT_LEN));
            
            // YYYY-MM-DD HH:MM:SS 长度为 19
            if (time_str.length() < 19)
            {
                dlog_warn("发现截断的时间标签，已忽略: %s", time_str.c_str());
                continue;
            }
            m_segmentTimes.push_back(std::move(time_str));
        }
        // 提取片段时长（#EXTINF）
        else if (line_sv.size() >= INF_LEN && line_sv.substr(0, INF_LEN) == INF_TAG)
        {
            std::string_view inf_sv = line_sv.substr(INF_LEN);
            // 移除可能的逗号
            size_t comma_pos = inf_sv.find(',');
            if (comma_pos != std::string_view::npos)
            {
                inf_sv = inf_sv.substr(0, comma_pos);
            }
            try
            {
                float duration = std::stof(std::string(inf_sv));
                m_segmentDurations.push_back(static_cast<int>(duration)); 
            }
            catch (const std::exception& e)
            {
                dlog_error("解析时长失败: %s, 行内容: %.*s", e.what(), (int)line_sv.size(), line_sv.data());
            }
        }
        // 处理缺失片段标记
        else if (line_sv.size() >= ST_LEN && line_sv.substr(0, ST_LEN) == ST_TAG)
        {
            if (!m_segmentTimes.empty()) m_segmentTimes.pop_back();
            if (!m_segmentDurations.empty()) m_segmentDurations.pop_back();
        }
    }


    if (m_segmentTimes.size() != m_segmentDurations.size())
    {
        dlog_warn("片段数量不匹配，尝试自动修复: 时间数=%zu, 时长数=%zu", m_segmentTimes.size(), m_segmentDurations.size());
        
        size_t min_count = std::min(m_segmentTimes.size(), m_segmentDurations.size());
        
        // 如果数量过少无法组成有效数据
        if (min_count == 0) {
             dlog_error("有效片段数量为0，无法解析");
             return 0;
        }

        // 裁剪到相同长度
        m_segmentTimes.resize(min_count);
        m_segmentDurations.resize(min_count);
    }

    if (m_segmentTimes.empty()) return 0;

    // 合并连续片段
    int nStartTime1 = -1;
    int nEndTime1 = 0;
    size_t start_idx = 0;

    for (; start_idx < m_segmentTimes.size(); ++start_idx)
    {
        nStartTime1 = parse_time(m_segmentTimes[start_idx]);
        if (nStartTime1 != -1)
        {
            nEndTime1 = nStartTime1 + m_segmentDurations[start_idx];
            m_videoTimes.emplace_back(Record_NS::VideoTime_S{nStartTime1, nEndTime1});
            break; // 找到第一个有效起点
        }
        else
        {
            dlog_warn("第%zu个片段时间无效，跳过", start_idx + 1);
        }
    }

    // 处理后续片段
    for (size_t i = start_idx + 1; i < m_segmentTimes.size(); ++i)
    {
        int nStartTime2 = parse_time(m_segmentTimes[i]);
        
        if (nStartTime2 == -1)
        {
            dlog_warn("第%zu个片段时间解析失败，跳过该片段", i + 1);
            continue;
        }

        if (m_videoTimes.empty())
        {
            nEndTime1 = nStartTime2 + m_segmentDurations[i];
            m_videoTimes.emplace_back(Record_NS::VideoTime_S{nStartTime2, nEndTime1});
            continue;
        }

        int currentEndTime = m_videoTimes.back().nEndTime;

        // 合并条件：下一片段开始时间与当前片段结束时间误差≤±2秒（确保连续）
        if (nStartTime2 >= (currentEndTime - 2) && nStartTime2 <= (currentEndTime + 2))
        {
            // 连续，更新结束时间
            m_videoTimes.back().nEndTime = nStartTime2 + m_segmentDurations[i];
        }
        else
        {
            // 不连续，新增一个片段
            int nNewEndTime = nStartTime2 + m_segmentDurations[i];
            m_videoTimes.emplace_back(Record_NS::VideoTime_S{nStartTime2, nNewEndTime});
        }
    }
    
    return 0;
}