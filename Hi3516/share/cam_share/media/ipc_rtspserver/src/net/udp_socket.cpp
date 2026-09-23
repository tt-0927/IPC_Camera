/**
 * @FilePath     : udp_socket.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : UDP socket 实现
 */

#include "net/udp_socket.h"

#include "support/log.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ipc_rtsp
{
namespace detail
{
namespace
{
constexpr const char *kTag = "udp";
} // namespace

Result UdpSocket::CreateConnected(const std::string &ip, std::uint16_t port, int dscp, int *fd, std::uint16_t *local_port)
{
    if (fd == nullptr || ip.empty() || port == 0)
    {
        return Result::Fail(Status::InvalidArgument);
    }

    const int sock = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (sock < 0)
    {
        return Result::Io(errno);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1)
    {
        ::close(sock);
        return Result::Fail(Status::InvalidArgument);
    }

    if (::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof addr) < 0)
    {
        const int saved = errno;
        ::close(sock);
        return Result::Io(saved);
    }

    if (dscp >= 0)
    {
        /* DSCP 占 ToS 字节高 6 位：左移 2 位写入 IP_TOS，低 2 位 ECN 保留 0。 */
        const int tos = (dscp & 0x3f) << 2;
        if (::setsockopt(sock, IPPROTO_IP, IP_TOS, &tos, sizeof tos) < 0)
        {
            IPC_RTSP_LOGW(kTag, "设置UDP DSCP失败 dscp:%d errno:%d", dscp, errno);
        }
    }

    sockaddr_in local{};
    socklen_t local_len = sizeof local;
    if (::getsockname(sock, reinterpret_cast<sockaddr *>(&local), &local_len) == 0)
    {
        if (local_port != nullptr)
        {
            *local_port = ntohs(local.sin_port);
        }
    }
    else
    {
        IPC_RTSP_LOGW(kTag, "读取UDP本端端口失败 errno:%d", errno);
        /* 约定：读不到本端端口时写 0，调用方据此判定端口不可用。 */
        if (local_port != nullptr)
        {
            *local_port = 0;
        }
    }

    *fd = sock;
    return Result::Ok();
}

} // namespace detail
} // namespace ipc_rtsp
