/**
 * @FilePath     : m3u8_file.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-16 10:29:12
 * @Description  : M3U8文件处理
 */

#include "m3u8_file.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <ctime>
#include <iomanip>
#include <cstring>

#include "dlog.h"
// #include "time.h"

CM3U8File::CM3U8File(std::string path)
{
    if (!path.empty())
    {
        if (access(path.c_str(), F_OK) != 0 || path != m_path)
        {
            create(path);
        }
    }
    m_path = path;
}

CM3U8File::~CM3U8File()
{
    /* 关闭文件 */
    if (m_outFile.is_open())
    {
        m_outFile.close();
    }
}

int CM3U8File::set_path(std::string path)
{
    m_path = path;
    if (path.empty())
    {
        dlog_error("path is empty");
        return -1;
    }
    if (access(path.c_str(), F_OK) == 0)
    {
        return 1;
    }
    return create(path);
}

int CM3U8File::create(std::string &path)
{
    if (path.empty())
    {
        dlog_error("path is empty");
        return -1;
    }
    /* 创建输出文件流 */
    m_outFile.open(path, std::ios::in | std::ios::out | std::ios::app);
    if (!m_outFile.is_open())
    {
        dlog_error("open m_outFile failed: %s", path.c_str());
        return -1;
    }
    /* 文件头 */
    m_outFile << "#EXTM3U\n";
    m_outFile << "#EXT-X-VERSION:3\n";
    m_outFile << "#EXT-X-TARGETDURATION:" << SLICING_TIME << "\n";
    m_outFile << "#EXT-X-MEDIA-SEQUENCE:0\n\n";

    m_outFile << "#EXT-X-ENDLIST\n";
    m_outFile.flush();
    m_outFile.close();
    return 0;
}

int CM3U8File::add_ts(SliceInfo_S stSliceInfo)
{
    if(stSliceInfo.filename.empty())
    {
        dlog_error("ts filename is empty");
        return -1;
    }
    std::fstream file(m_path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        dlog_error("open m_outFile failed: %s", m_path.c_str());
        return -1;
    }

    int64_t nDurationMs = stSliceInfo.nEndTimeMs - stSliceInfo.nStartTimeMs;
    // if (nDurationMs < SLICING_TIME * 1000)
    // {
    //     nDurationMs = SLICING_TIME * 1000;
    // }

    int64_t nLastEndTime = 0;

    // Seek to the end of the file
    file.seekg(0, std::ios::end);
    size_t nFileLen = file.tellg();

    const size_t fileTailSize = 560;
    size_t nLen = std::min(fileTailSize, nFileLen);
    std::string achBuf;
    achBuf.resize(nLen);

    // Seek back nLen bytes
    file.seekg(-static_cast<int>(nLen), std::ios::end);
    file.read(&achBuf[0], nLen);

    // Find "#EXT-X-ENDLIST" position
    std::string key = "#EXT-X-ENDLIST";
    size_t endListPos = achBuf.rfind(key);
    if (endListPos == std::string::npos)
    {
        endListPos = 0;
    }

    std::cout << "endListPos " << endListPos << std::endl;
    // Find "#END-TIME" position and extract the timestamp
    key = "#END-TIME:";
    auto endTimePos = achBuf.rfind(key);
    if (endTimePos != std::string::npos)
    {
        size_t startPos = endTimePos + key.size();
        size_t endPos = achBuf.find('\n', startPos); // 找到下一个换行符
        std::string timestamp;
        if (endPos != std::string::npos)
        {
            timestamp = achBuf.substr(startPos, endPos - startPos); // 返回#END-TIME:与\n之间的字符串
            nLastEndTime = std::stoll(timestamp);
        }
    }
    else
    {
        endTimePos = 0;
    }

    // Seek back to overwrite from "#EXT-X-ENDLIST"
    int nEndPos = endTimePos == 0 ? endListPos : endTimePos;
    file.seekp(-static_cast<int>(nLen - nEndPos), std::ios::end);
    /* 插入空白分片数据 */
    if (nLastEndTime > 0 && stSliceInfo.nStartTimestampMs > 0 && stSliceInfo.nStartTimestampMs - nLastEndTime > 2000)
    {
        add_nullFile(file, stSliceInfo.nStartTimestampMs, nLastEndTime);
    }

    // Write program date-time
    file << "#EXT-X-PROGRAM-DATE-TIME:" << stSliceInfo.startTime << "\n";

    // Write EXTINF and file name
    file << "#EXTINF:" << std::fixed << std::setprecision(3) << static_cast<double>(nDurationMs) / 1000.0 << ",\n";
    /* 截取出文件名 */
    size_t pos = stSliceInfo.filename.find_last_of('/');
    if (pos != std::string::npos)
    {
        stSliceInfo.filename = stSliceInfo.filename.substr(pos + 1);
    }
    file << stSliceInfo.filename << "\n";
    /* 记录结束录制时的实际时间戳 */
    file << "#END-TIME:" << stSliceInfo.nEndTimestampMs << "\n";
    file << "#EXT-X-ENDLIST\n";
    file.flush();
    file.close();
    return 0;
}

void CM3U8File::add_nullFile(std::fstream &file, int64_t nStartTime, int64_t nEndTime)
{
    if (!file.is_open())
    {
        return;
    }

    std::ostringstream oss;

    oss << "#END-TIME:" << nEndTime << "\n";
    /* note: 录像中断间隙不是实际媒体片段，只写边界标记，避免生成超长EXTINF和不存在的ts文件 */
    oss << "#START-TIME:" << nStartTime << "\n";

    file << oss.str();
}

int CM3U8File::write_head()
{
    if (!m_outFile.is_open())
    {
        dlog_error("open m_outFile failed: %s", m_path.c_str());
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

int CM3U8File::write_data(CM3U8File::Data_S stData)
{
    if (!m_outFile.is_open())
    {
        dlog_error("open m_outFile failed: %s", m_path.c_str());
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

int CM3U8File::write_tail()
{
    if (!m_outFile.is_open())
    {
        dlog_error("open m_outFile failed: %s", m_path.c_str());
        return -1;
    }
    /* 文件尾巴 */
    m_outFile << "#EXT-X-ENDLIST\n";
    return 0;
}
