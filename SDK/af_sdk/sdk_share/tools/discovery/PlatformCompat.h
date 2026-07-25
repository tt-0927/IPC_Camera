#pragma once
/**
 * @file PlatformCompat.h
 * @brief Windows/Linux 平台兼容头文件
 *        统一 socket 相关头文件、类型定义和函数映射
 */

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
    #ifndef _SSIZE_T_DEFINED
        typedef intptr_t ssize_t;
        #define _SSIZE_T_DEFINED
    #endif

    /* 关闭 socket：Windows 用 closesocket，Linux 用 close */
    #define socket_close(fd) closesocket(fd)

    /* 设置 socket 为非阻塞 */
    #define socket_set_nonblock(fd) \
        do { \
            u_long mode = 1; \
            ioctlsocket(fd, FIONBIO, &mode); \
        } while (0)

    /* usleep 在 Windows 上不存在，用 Sleep 替代（参数单位：微秒 → 毫秒） */
    #define usleep(us) Sleep((us) / 1000)

    /* Windows 下 poll 用 WSAPoll 替代 */
    #define poll WSAPoll

    /* Windows 下 errno 由 WSAGetLastError() 获取 */
    #define socket_errno() WSAGetLastError()

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

    #define socket_close(fd) close(fd)

    #define socket_set_nonblock(fd) \
        do { \
            int flags = fcntl(fd, F_GETFL, 0); \
            fcntl(fd, F_SETFL, flags | O_NONBLOCK); \
        } while (0)

    #define socket_errno() errno
#endif /* _WIN32 */
