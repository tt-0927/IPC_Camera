/**
 * @file PlatformCompat.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief PlatformCompat 模块接口与类型定义
 * 功能说明：
 * 1. 声明 PlatformCompat 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#pragma once


#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <io.h>

    /* Windows 下 socket 类型为 SOCKET (unsigned int)，错误值为 INVALID_SOCKET */
    using socket_fd_t = SOCKET;
    constexpr socket_fd_t INVALID_SOCKET_FD = INVALID_SOCKET;

    /* Windows 没有 ssize_t */
    #ifndef NETSDK_SSIZE_T_DEFINED
        typedef intptr_t ssize_t;
        #define NETSDK_SSIZE_T_DEFINED
    #endif

    /* 关闭 socket：Windows 用 closesocket，Linux 用 close */
    #define NETSDK_SOCKET_CLOSE(fd) closesocket(fd)

    /* 设置 socket 为非阻塞 */
    #define NETSDK_SOCKET_SET_NONBLOCK(fd) \
        do { \
            u_long mode = 1; \
            ioctlsocket(fd, FIONBIO, &mode); \
        } while (0)

    /* NETSDK_MICRO_SLEEP 在 Windows 上不存在，用 Sleep 替代（参数单位：微秒 → 毫秒） */
    #define NETSDK_MICRO_SLEEP(us) Sleep((us) / 1000)

    /* Windows 下 NETSDK_POLL 用 WSAPoll 替代 */
    #define NETSDK_POLL WSAPoll

    /* Windows 下 errno 由 WSAGetLastError() 获取 */
    #define NETSDK_SOCKET_GET_ERROR() WSAGetLastError()

    #ifndef EWOULDBLOCK
        #define EWOULDBLOCK WSAEWOULDBLOCK
    #endif
    #ifndef EAGAIN
        #define EAGAIN WSAEWOULDBLOCK
    #endif

    /* Windows 下 SO_BINDTODEVICE 不可用 */
    #ifndef SO_BINDTODEVICE
        #define SO_BINDTODEVICE 0
    #endif

    /* shutdown 参数：Windows 用 SD_BOTH，Linux 用 SHUT_RDWR */
    #define SHUT_RDWR SD_BOTH

#else /* Linux / POSIX */
    #include <arpa/inet.h>
    #include <cerrno>
    #include <fcntl.h>
    #include <net/if.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <poll.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <unistd.h>

    using socket_fd_t = int;
    constexpr socket_fd_t INVALID_SOCKET_FD = -1;

    #define NETSDK_POLL poll
    #define NETSDK_SOCKET_CLOSE(fd) close(fd)

    #define NETSDK_SOCKET_SET_NONBLOCK(fd) \
        do { \
            int flags = fcntl(fd, F_GETFL, 0); \
            fcntl(fd, F_SETFL, flags | O_NONBLOCK); \
        } while (0)

    #define NETSDK_SOCKET_GET_ERROR() errno
    #define NETSDK_MICRO_SLEEP(us) usleep((us))
#endif /* _WIN32 */
