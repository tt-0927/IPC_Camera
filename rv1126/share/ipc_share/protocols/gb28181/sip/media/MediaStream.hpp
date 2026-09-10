/*
 * @Author       : EasonLu
 * @Date         : 2025-02-27 15:31:15
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-02-27 19:26:53
 * @FilePath     : MediaStream.hpp
 * @Description  : 媒体基类
 */
#pragma once
#include "ExternSip.h"
#include <memory>
#include <string>

namespace SIP
{

    enum STREAM_TYPE
    {
        STREAM_TYPE_NONE = 0,
        STREAM_TYPE_GB = 1,    /* GB28181方式推流 */
        STREAM_TYPE_PROXY = 2, /* 拉流代理 */
        STREAM_TYPE_PUSH = 3,  /* 设备端主动推流 */
        STREAM_TYPE_MAX
    };

    class MediaStream
    {
    public:
        typedef std::shared_ptr<MediaStream> Ptr;
        MediaStream() = default;
        MediaStream(const std::string &app, const std::string &stream_id, STREAM_TYPE type) : _app(app), _stream_id(stream_id), _type(type) {}

        // MARK: 基类必须要有一个虚函数
        virtual ~MediaStream() {}

        std::string GetApp() const { return _app; }
        std::string GetStreamID() const { return _stream_id; }
        STREAM_TYPE GetType() const { return _type; }

        eXosip_t *exosip_context = nullptr;

    private:
        std::string _app;
        std::string _stream_id;
        STREAM_TYPE _type = STREAM_TYPE::STREAM_TYPE_NONE;
    };

    class MediaStreamProxy : public MediaStream,
                             public std::enable_shared_from_this<MediaStreamProxy>
    {
    public:
        MediaStreamProxy(const std::string &app, const std::string &stream_id)
            : MediaStream(app, stream_id, STREAM_TYPE::STREAM_TYPE_PROXY) {};
    };

    class MediaStreamPushed : public MediaStream,
                              public std::enable_shared_from_this<MediaStreamPushed>
    {
    public:
        MediaStreamPushed(const std::string &app, const std::string &stream_id)
            : MediaStream(app, stream_id, STREAM_TYPE::STREAM_TYPE_PUSH) {};
    };

}
