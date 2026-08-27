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
#include <cmath>

/*定义视频分片时长，秒*/
#define SLICING_TIME 60
/* EXTINF解析容忍时长，秒：实际分片需等待关键帧，可能略超过标准分片时长 */
#define EXTINF_DURATION_TOLERANCE_SEC 10

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
    m_outFile << "#EXT-X-TARGETDURATION:" << SLICING_TIME << "\n";
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
        if (line != "#EXT-X-ENDLIST")  // 忽略 ENDLIST 标记
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
    m_outFile << "#EXT-X-TARGETDURATION:" << SLICING_TIME << "\n";
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
static std::string trim(const std::string &str)
{
    if (str.empty())
    {
        return "";
    }

    size_t nStart = 0;
    size_t nEnd   = str.size() - 1;

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
std::vector<std::string> M3U8::get_M3u8TsFileName(const std::string &strFilePath)
{
    std::vector<std::string> strTsFiles;
    std::ifstream            file(strFilePath.c_str());
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
    int hour   = atoi(timeStr.substr(11, 2).c_str());
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

    std::ifstream file(m_path, std::ios::binary);
    if (!file.is_open())
    {
        dlog_error("打开文件失败: %s", m_path.c_str());
        return -1;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size == 0)
    {
        return 0;
    }

    std::string file_content(file_size, '\0');
    file.read(&file_content[0], file_size);
    file.close();

    // ================= split lines =================
    std::vector<std::string_view> lines;

    size_t start = 0;
    for (size_t i = 0; i < file_content.size(); ++i)
    {
        if (file_content[i] == '\n' || file_content[i] == '\r')
        {
            if (i > start)
            {
                lines.emplace_back(&file_content[start], i - start);
            }

            start = i + 1;
        }
    }

    if (start < file_content.size())
    {
        lines.emplace_back(&file_content[start], file_content.size() - start);
    }

    // ================= tags =================
    const std::string_view PDT_TAG = "#EXT-X-PROGRAM-DATE-TIME:";
    const std::string_view INF_TAG = "#EXTINF:";
    const std::string_view ST_TAG  = "#START-TIME:";
    const std::string_view ET_TAG  = "#END-TIME:";

    const size_t PDT_LEN = PDT_TAG.size();
    const size_t INF_LEN = INF_TAG.size();
    // const size_t ST_LEN  = ST_TAG.size();

    // ================= clear =================
    m_segmentTimes.clear();
    m_segmentDurations.clear();
    m_videoTimes.clear();

    bool timePending = false;
    bool lastSegmentMayBeGapPlaceholder = false;
    bool invalidExtinfWaitingStartMarker = false;
    float invalidExtinfDuration = 0.0f;

    // ================= parse =================
    for (const auto &line_sv : lines)
    {
        if (line_sv.empty())
        {
            continue;
        }

        // PDT
        if (line_sv.rfind(PDT_TAG, 0) == 0)
        {
            if (invalidExtinfWaitingStartMarker)
            {
                dlog_warn("非法EXTINF:%f", invalidExtinfDuration);
                invalidExtinfWaitingStartMarker = false;
                invalidExtinfDuration = 0.0f;
            }

            std::string time_str(line_sv.substr(PDT_LEN));

            if (time_str.length() < 19)
            {
                dlog_warn("非法PDT:%s", time_str.c_str());
                continue;
            }

            // 连续PDT：回滚未消费PDT
            if (timePending)
            {
                dlog_warn("连续PDT丢弃:%s", m_segmentTimes.back().c_str());

                m_segmentTimes.pop_back();
            }

            m_segmentTimes.push_back(std::move(time_str));
            timePending = true;
            lastSegmentMayBeGapPlaceholder = false;
            invalidExtinfWaitingStartMarker = false;
        }

        // EXTINF
        else if (line_sv.rfind(INF_TAG, 0) == 0)
        {
            if (!timePending || m_segmentTimes.empty())
            {
                dlog_warn("EXTINF无对应PDT");
                continue;
            }

            std::string_view v = line_sv.substr(INF_LEN);

            size_t comma = v.find(',');
            if (comma != std::string_view::npos)
            {
                v = v.substr(0, comma);
            }

            float duration = 0.0f;

            try
            {
                duration = std::stof(std::string(v)); 
            } catch (...)
            {
                dlog_error("EXTINF解析失败");

                if (!m_segmentTimes.empty())
                {
                    m_segmentTimes.pop_back();
                }

                timePending = false;
                continue;
            }

            /* note: 防NaN/inf和异常范围，旧格式间隙占位片段会在后续START-TIME处静默跳过 */
            if (!std::isfinite(duration) || duration <= 0.0f || duration > SLICING_TIME + EXTINF_DURATION_TOLERANCE_SEC)
            {
                if (!m_segmentTimes.empty())
                {
                    m_segmentTimes.pop_back();
                }

                timePending = false;
                lastSegmentMayBeGapPlaceholder = false;
                invalidExtinfWaitingStartMarker = true;
                invalidExtinfDuration = duration;
                continue;
            }

            m_segmentDurations.push_back((int)duration);
            timePending = false;
            lastSegmentMayBeGapPlaceholder = true;
            invalidExtinfWaitingStartMarker = false;
        }

        // START-TIME：按状态精准删除，防止错位
        else if (line_sv.rfind(ST_TAG, 0) == 0)
        {
            dlog_debug("START-TIME");

            if (timePending)
            {
                // 只有PDT，未配对INF：只删时间
                if (!m_segmentTimes.empty())
                {
                    m_segmentTimes.pop_back();
                }
            }
            else if (invalidExtinfWaitingStartMarker)
            {
                /* note: 旧文件中超长间隙片段已在EXTINF处丢弃，此处只消费START-TIME，避免误删前一个真实片段 */
                invalidExtinfWaitingStartMarker = false;
                invalidExtinfDuration = 0.0f;
            }
            else if (!lastSegmentMayBeGapPlaceholder)
            {
                /* note: 新文件只写END-TIME/START-TIME边界，不再生成占位片段，此处不删除真实片段 */
            }
            else
            {
                // 已完整配对：同时删除旧格式中用于表达无录像间隙的占位片段
                if (!m_segmentTimes.empty() && !m_segmentDurations.empty())
                {
                    m_segmentTimes.pop_back();
                    m_segmentDurations.pop_back();
                }
                else
                {
                    dlog_warn("START-TIME无完整片段可删");
                }
            }

            timePending = false;
            lastSegmentMayBeGapPlaceholder = false;
        }
        else if (line_sv.rfind(ET_TAG, 0) == 0)
        {
            if (invalidExtinfWaitingStartMarker)
            {
                dlog_warn("非法EXTINF:%f", invalidExtinfDuration);
            }

            /* note: 真实录像片段后会写END-TIME，后续START-TIME只表示新间隙边界，不能回删该片段 */
            lastSegmentMayBeGapPlaceholder = false;
            invalidExtinfWaitingStartMarker = false;
            invalidExtinfDuration = 0.0f;
        }
        else if (!line_sv.empty() && line_sv.front() != '#')
        {
            /* note: ts文件名之后若紧跟START-TIME，表示兼容旧格式的间隙占位片段 */
        }
    }

    // ================= EOF cleanup =================
    if (timePending)
    {
        dlog_warn("EOF未配对PDT丢弃");

        if (!m_segmentTimes.empty())
        {
            m_segmentTimes.pop_back();
        }

        timePending = false;
    }

    if (invalidExtinfWaitingStartMarker)
    {
        dlog_warn("非法EXTINF:%f", invalidExtinfDuration);
    }

    // ================= consistency fix =================
    if (m_segmentTimes.size() != m_segmentDurations.size())
    {
        dlog_error("不一致 times=%zu durations=%zu", m_segmentTimes.size(), m_segmentDurations.size());

        size_t min_count = std::min(m_segmentTimes.size(), m_segmentDurations.size());

        if (min_count == 0)
        {
            dlog_error("无有效数据");
            return 0;
        }

        m_segmentTimes.resize(min_count);
        m_segmentDurations.resize(min_count);
    }

    if (m_segmentTimes.empty())
    {
        return 0;
    }

    // ================= merge segments =================
    for (size_t i = 0; i < m_segmentTimes.size(); ++i)
    {
        int t = parse_time(m_segmentTimes[i]);
        if (t < 0)
        {
            continue;
        }

        int end = t + m_segmentDurations[i];

        if (m_videoTimes.empty())
        {
            m_videoTimes.push_back({t, end});
            continue;
        }

        auto &last = m_videoTimes.back();

        if (t <= last.nEndTime + 2)
        {
            last.nEndTime = end;
        }
        else
        {
            m_videoTimes.push_back({t, end});
        }
    }

    return 0;
}
