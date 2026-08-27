#include "StreamManager.h"
#include "CallSession.h"

using namespace SIP;
StreamManager *StreamManager::m_pInstance = nullptr;
std::mutex StreamManager::m_mtx;
void StreamManager::AddStream(MediaStream::Ptr stream)
{
    std::scoped_lock<std::mutex> lock(_mutex);
    _streams[stream->GetStreamID()] = stream;
}

void StreamManager::RemoveStream(const std::string &id)
{
    std::scoped_lock<std::mutex> lock(_mutex);
    _streams.erase(id);
}

MediaStream::Ptr StreamManager::GetStream(const std::string &id)
{
    std::scoped_lock<std::mutex> lock(_mutex);
    auto iter = _streams.find(id);
    if (iter != _streams.end())
    {
        return iter->second;
    }
    return nullptr;
}

MediaStream::Ptr StreamManager::GetStreamByCallID(int id)
{
    std::scoped_lock<std::mutex> lock(_mutex);
    for (auto &&s : _streams)
    {
        if (s.second->GetType() == STREAM_TYPE_GB)
        {
            auto session = std::dynamic_pointer_cast<CallSession>(s.second);
            if (session && session->GetCallID() == id)
            {
                return s.second;
            }
        }
    }
    return nullptr;
}

std::vector<MediaStream::Ptr> StreamManager::GetAllStream()
{
    std::vector<MediaStream::Ptr> streams;
    for (auto &&s : _streams)
    {
        streams.push_back(s.second);
    }
    return streams;
}

std::vector<MediaStream::Ptr> StreamManager::GetStreamByType(STREAM_TYPE type)
{
    std::vector<MediaStream::Ptr> streams;
    for (auto &&s : _streams)
    {
        if (s.second->GetType() == type)
            streams.push_back(s.second);
    }
    return streams;
}

MediaStream::Ptr StreamManager::MakeStream(const std::string &stream_id, const std::string &app, STREAM_TYPE type)
{
    MediaStream::Ptr stream = nullptr;
    if (type == STREAM_TYPE_PROXY)
    {
        stream = std::make_shared<MediaStreamProxy>(app, stream_id);
    }
    else if (type == STREAM_TYPE_PUSH)
    {
        stream = std::make_shared<MediaStreamPushed>(app, stream_id);
    }

    if (stream)
    {
        std::scoped_lock<std::mutex> lock(_mutex);
        _streams[stream->GetStreamID()] = stream;
    }
    return stream;
}

void StreamManager::ClearStreams()
{
    std::scoped_lock<std::mutex> lock(_mutex);
    _streams.clear();
}