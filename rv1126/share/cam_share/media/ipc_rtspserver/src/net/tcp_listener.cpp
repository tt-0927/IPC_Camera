/**
 * @FilePath     : tcp_listener.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : TCP 监听器实现
 */

#include "net/tcp_listener.h"

#include "support/log.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "listen";
/** listen backlog：连接上限由业务层控制，这里只给内核一个合理的队列。 */
constexpr int kBacklog = 16;
} // namespace

TcpListener::~TcpListener()
{
    Stop();
}

Result TcpListener::Start(EventLoop *loop, const std::string &bind_address, std::uint16_t port, AcceptCallback callback)
{
    if (loop == nullptr || !callback)
    {
        return Result::Fail(Status::InvalidArgument);
    }
    Stop();

    loop_ = loop;
    callback_ = std::move(callback);

    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0)
    {
        return Result::Io(errno);
    }

    int reuse = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (bind_address.empty() || bind_address == "0.0.0.0")
    {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (::inet_pton(AF_INET, bind_address.c_str(), &addr.sin_addr) != 1)
    {
        ::close(fd);
        return Result::Fail(Status::InvalidArgument);
    }

    if (::bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0)
    {
        const int saved = errno;
        ::close(fd);
        return Result::Io(saved);
    }

    if (::listen(fd, kBacklog) < 0)
    {
        const int saved = errno;
        ::close(fd);
        return Result::Io(saved);
    }

    /* 回读真实端口（port=0 时由内核分配，便于测试）。 */
    sockaddr_in local{};
    socklen_t local_len = sizeof local;
    if (::getsockname(fd, reinterpret_cast<sockaddr *>(&local), &local_len) == 0)
    {
        port_ = ntohs(local.sin_port);
    }
    else
    {
        port_ = port;
    }

    const Result added = loop_->Add(fd,
                                    EPOLLIN,
                                    [this](std::uint32_t events)
                                    {
                                        OnReadable(events);
                                    });
    if (!added.ok())
    {
        ::close(fd);
        return added;
    }

    fd_ = fd;
    return Result::Ok();
}

void TcpListener::Stop()
{
    /* 时序：先从事件循环摘除注册（防止回调再入），再 close(fd)，最后置空回调。 */
    if (fd_ >= 0)
    {
        if (loop_ != nullptr)
        {
            loop_->Remove(fd_);
        }
        ::close(fd_);
        fd_ = -1;
    }
    callback_ = nullptr;
}

void TcpListener::OnReadable(std::uint32_t events)
{
    /* fd 已失效（已 Stop）才把 ERR/HUP 事件当作监听 socket 的收尾；fd 有效时
     * 不因 ERR/HUP 提前返回，仍继续 accept（监听 socket 的 ERR/HUP 极少见，
     * accept 会返回真实错误码）。 */
    if ((events & (EPOLLERR | EPOLLHUP)) != 0u && fd_ < 0)
    {
        return;
    }

    /* 单次事件里循环 accept，直到没有新连接或达到单轮上限，避免惊群与饥饿。 */
    for (int i = 0; i < 8; ++i)
    {
        sockaddr_in peer{};
        socklen_t peer_len = sizeof peer;
        const int client = ::accept4(fd_, reinterpret_cast<sockaddr *>(&peer), &peer_len, SOCK_CLOEXEC | SOCK_NONBLOCK);
        if (client < 0)
        {
            /* EINTR/EAGAIN：无新连接或被信号打断，不算错误直接收工；其余 errno
             * 记日志后同样收工，监听 socket 短暂故障留给下一轮事件重试。 */
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                return;
            }
            IPC_RTSP_LOGW(kTag, "accept 失败 errno:%d", errno);
            return;
        }

        char ip_text[INET_ADDRSTRLEN] = { 0 };
        ::inet_ntop(AF_INET, &peer.sin_addr, ip_text, sizeof ip_text);

        callback_(client, std::string(ip_text), ntohs(peer.sin_port));
    }
}

} // namespace detail
} // namespace ipc_rtsp
