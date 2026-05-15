/**
 * @file main.c
 * @brief HTTP 人脸服务端模拟 Demo
 *
 * 编译示例:
 *   cmake -S . -B build
 *   cmake --build build
 *
 * 运行示例:
 *   ./HttpFaceServerDemo 9000 http://127.0.0.1:18080/face/event 5
 *
 * 说明:
 *   本 Demo 用于模拟设备侧 HTTP 能力：
 *   1. 提供 HTTP-SDK 转发命令接口，便于客户端 Demo 发送 NET_TV_* 命令；
 *   2. 定时向客户端 HTTP 回调地址推送人脸抓拍和人脸比对事件。
 */

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET socket_handle_t;
#define CLOSE_SOCKET closesocket
#define THREAD_RET DWORD WINAPI
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_handle_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSE_SOCKET close
#define THREAD_RET void *
#endif

#define HTTP_RECV_BUFFER_SIZE (16 * 1024)
#define HTTP_SMALL_BUFFER_SIZE 512
#define HTTP_URL_HOST_SIZE 128
#define HTTP_URL_PATH_SIZE 512
#define HTTP_BOUNDARY "----NetTvFaceHttpDemoBoundary7MA4YWxkTrZu0gW"

static volatile sig_atomic_t g_running = 1;
static socket_handle_t g_listen_socket = INVALID_SOCKET;

/**
 * @brief 打印 socket 调用失败原因
 * @param operation 失败的操作名称
 */
static void print_socket_error(const char *operation)
{
#ifdef _WIN32
    printf("%s失败, WSAGetLastError=%d\n", operation, WSAGetLastError());
#else
    printf("%s失败, errno=%d (%s)\n", operation, errno, strerror(errno));
#endif
}

/**
 * @brief HTTP 响应结果
 */
typedef struct HttpResult
{
    int status;
    char body[HTTP_SMALL_BUFFER_SIZE];
} HttpResult;

/**
 * @brief HTTP URL 解析结果
 */
typedef struct UrlParts
{
    char host[HTTP_URL_HOST_SIZE];
    int port;
    char path[HTTP_URL_PATH_SIZE];
} UrlParts;

/**
 * @brief 动态缓冲区，用于拼接 multipart 请求体和 HTTP 报文
 */
typedef struct DynamicBuffer
{
    char *data;
    size_t size;
    size_t capacity;
} DynamicBuffer;

/**
 * @brief multipart 表单字段
 */
typedef struct FormPart
{
    const char *name;
    const unsigned char *data;
    size_t data_len;
    const char *filename;
    const char *content_type;
} FormPart;

/**
 * @brief 服务监听线程参数
 */
typedef struct ServerThreadParam
{
    int listen_port;
} ServerThreadParam;

/**
 * @brief 推送线程参数
 */
typedef struct PushThreadParam
{
    char callback_url[HTTP_URL_HOST_SIZE + HTTP_URL_PATH_SIZE];
    int interval_sec;
} PushThreadParam;

/**
 * @brief 返回一段最小 JPEG 数据，用于模拟图片字段
 */
static const unsigned char g_demo_jpeg[] = {
    0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46,
    0x49, 0x46, 0x00, 0x01, 0x01, 0x01, 0x00, 0x48,
    0x00, 0x48, 0x00, 0x00, 0xff, 0xdb, 0x00, 0x43,
    0x00, 0xff, 0xc0, 0x00, 0x0b, 0x08, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x01, 0x11, 0x00, 0xff, 0xc4,
    0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0xff, 0xc4, 0x00, 0x14,
    0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xda, 0x00, 0x08, 0x01, 0x01,
    0x00, 0x00, 0x3f, 0x00, 0x37, 0xff, 0xd9
};

/**
 * @brief 处理退出信号
 * @param signal_value 信号值
 */
static void signal_handler(int signal_value)
{
    if (signal_value == SIGINT || signal_value == SIGTERM)
    {
        g_running = 0;
    }
}

/**
 * @brief 跨平台毫秒睡眠
 * @param milliseconds 睡眠时间，单位毫秒
 */
static void sleep_ms(int milliseconds)
{
#ifdef _WIN32
    Sleep((DWORD)milliseconds);
#else
    usleep((useconds_t)milliseconds * 1000);
#endif
}

/**
 * @brief 初始化网络环境
 * @return 0 表示成功，非 0 表示失败
 */
static int network_init(void)
{
#ifdef _WIN32
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data);
#else
    return 0;
#endif
}

/**
 * @brief 清理网络环境
 */
static void network_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * @brief 获取当前毫秒时间戳
 * @return 当前 Unix 毫秒时间戳
 */
static long long current_timestamp_ms(void)
{
#ifdef _WIN32
    return (long long)time(NULL) * 1000LL;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)ts.tv_nsec / 1000000LL;
#endif
}

/**
 * @brief 初始化动态缓冲区
 * @param buffer 动态缓冲区指针
 */
static void buffer_init(DynamicBuffer *buffer)
{
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

/**
 * @brief 释放动态缓冲区
 * @param buffer 动态缓冲区指针
 */
static void buffer_free(DynamicBuffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

/**
 * @brief 预留动态缓冲区容量
 * @param buffer 动态缓冲区指针
 * @param extra_size 额外所需字节数
 * @return 0 表示成功，非 0 表示失败
 */
static int buffer_reserve(DynamicBuffer *buffer, size_t extra_size)
{
    size_t required = buffer->size + extra_size + 1;
    char *new_data = NULL;
    size_t new_capacity = buffer->capacity;

    if (required <= buffer->capacity)
    {
        return 0;
    }

    if (new_capacity == 0)
    {
        new_capacity = 1024;
    }

    while (new_capacity < required)
    {
        new_capacity *= 2;
    }

    new_data = (char *)realloc(buffer->data, new_capacity);
    if (new_data == NULL)
    {
        return -1;
    }

    buffer->data = new_data;
    buffer->capacity = new_capacity;
    return 0;
}

/**
 * @brief 向动态缓冲区追加二进制数据
 * @param buffer 动态缓冲区指针
 * @param data 数据指针
 * @param data_len 数据长度
 * @return 0 表示成功，非 0 表示失败
 */
static int buffer_append(DynamicBuffer *buffer, const void *data, size_t data_len)
{
    if (buffer_reserve(buffer, data_len) != 0)
    {
        return -1;
    }

    memcpy(buffer->data + buffer->size, data, data_len);
    buffer->size += data_len;
    buffer->data[buffer->size] = '\0';
    return 0;
}

/**
 * @brief 向动态缓冲区追加字符串
 * @param buffer 动态缓冲区指针
 * @param text 字符串
 * @return 0 表示成功，非 0 表示失败
 */
static int buffer_append_string(DynamicBuffer *buffer, const char *text)
{
    return buffer_append(buffer, text, strlen(text));
}

/**
 * @brief 向动态缓冲区追加格式化字符串
 * @param buffer 动态缓冲区指针
 * @param fmt 格式字符串
 * @return 0 表示成功，非 0 表示失败
 */
static int buffer_append_format(DynamicBuffer *buffer, const char *fmt, ...)
{
    va_list args;
    va_list args_copy;
    int len = 0;
    char *temp = NULL;
    int ret = 0;

    va_start(args, fmt);
    va_copy(args_copy, args);
    len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (len < 0)
    {
        va_end(args);
        return -1;
    }

    temp = (char *)malloc((size_t)len + 1);
    if (temp == NULL)
    {
        va_end(args);
        return -1;
    }

    vsnprintf(temp, (size_t)len + 1, fmt, args);
    va_end(args);

    ret = buffer_append(buffer, temp, (size_t)len);
    free(temp);
    return ret;
}

/**
 * @brief 发送完整 socket 数据
 * @param sock socket 句柄
 * @param data 数据指针
 * @param data_len 数据长度
 * @return 0 表示成功，非 0 表示失败
 */
static int send_all(socket_handle_t sock, const char *data, size_t data_len)
{
    size_t sent_total = 0;
    while (sent_total < data_len)
    {
        size_t remain = data_len - sent_total;
        int chunk = remain > 0x7fffffff ? 0x7fffffff : (int)remain;
        int sent = send(sock, data + sent_total, chunk, 0);
        if (sent <= 0)
        {
            return -1;
        }
        sent_total += (size_t)sent;
    }
    return 0;
}

/**
 * @brief 判断 HTTP 路径是否匹配，允许携带 query 参数
 * @param path 请求路径
 * @param expected 期望路径
 * @return 1 表示匹配，0 表示不匹配
 */
static int path_matches(const char *path, const char *expected)
{
    size_t expected_len = strlen(expected);
    return strncmp(path, expected, expected_len) == 0 &&
           (path[expected_len] == '\0' || path[expected_len] == '?');
}

/**
 * @brief 发送 JSON HTTP 响应
 * @param client 客户端 socket
 * @param status_code HTTP 状态码
 * @param body JSON 响应体
 */
static void send_json_response(socket_handle_t client, int status_code, const char *body)
{
    char header[HTTP_SMALL_BUFFER_SIZE];
    const char *status_text = status_code == 200 ? "OK" : "Not Found";
    int body_len = (int)strlen(body);
    int header_len = snprintf(header,
                              sizeof(header),
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: %d\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                              "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                              "Connection: close\r\n"
                              "\r\n",
                              status_code,
                              status_text,
                              body_len);

    if (header_len > 0)
    {
        send_all(client, header, (size_t)header_len);
        send_all(client, body, (size_t)body_len);
    }
}

/**
 * @brief 打印收到的 HTTP 命令
 * @param name 命令名称
 * @param method HTTP 方法
 * @param path HTTP 路径
 * @param body HTTP 请求体
 */
static void print_command_request(const char *name, const char *method, const char *path, const char *body)
{
    printf("\n[命令] %s 方法=%s 路径=%s\n", name, method, path);
    if (body != NULL && body[0] != '\0')
    {
        printf("请求体:\n%s\n", body);
    }
}

/**
 * @brief 处理单个 HTTP 客户端连接
 * @param client 客户端 socket
 */
static void handle_http_client(socket_handle_t client)
{
    char buffer[HTTP_RECV_BUFFER_SIZE];
    char method[16] = {0};
    char path[HTTP_URL_PATH_SIZE] = {0};
    char *body = NULL;
    int recv_len = recv(client, buffer, sizeof(buffer) - 1, 0);

    if (recv_len <= 0)
    {
        return;
    }

    buffer[recv_len] = '\0';
    if (sscanf(buffer, "%15s %511s", method, path) != 2)
    {
        send_json_response(client, 404, "{\"Ret\":-1,\"Message\":\"Bad request\",\"Data\":{}}");
        return;
    }

    body = strstr(buffer, "\r\n\r\n");
    if (body != NULL)
    {
        body += 4;
    }
    else
    {
        body = "";
    }

    if (strcmp(method, "OPTIONS") == 0)
    {
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"OK\",\"Data\":{}}");
    }
    else if ((strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) &&
             path_matches(path, "/api/v1/sdk/command"))
    {
        print_command_request("HTTP-SDK转发命令", method, path, body);
        if (strstr(body, "NET_TV_GET_FACE_COMPARE_INFO") != NULL)
        {
            send_json_response(client, 200, "{\"ActionCode\":2529,\"Return\":0,\"Data\":{\"Enable\":true,\"LinkageSuccessMode\":{\"Tradition\":[6,7],\"AlarmLinkage\":[],\"RecordChn\":[]},\"LinkageFailMode\":{\"Tradition\":[6],\"AlarmLinkage\":[],\"RecordChn\":[]}}}");
        }
        else if (strstr(body, "NET_TV_SET_FACE_COMPARE_INFO") != NULL)
        {
            send_json_response(client, 200, "{\"ActionCode\":2528,\"Return\":0,\"Data\":{}}");
        }
        else
        {
            send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"HTTP-SDK command forwarded\",\"Data\":{}}");
        }
    }
    else if (strcmp(method, "GET") == 0 && path_matches(path, "/api/v1/face/capture/config"))
    {
        print_command_request("获取人脸抓拍配置", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"OK\",\"Data\":{\"Enable\":true,\"Rule\":{\"Interval\":5}}}");
    }
    else if ((strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) &&
             path_matches(path, "/api/v1/face/capture/config"))
    {
        print_command_request("设置人脸抓拍配置", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"Face capture config updated\",\"Data\":{}}");
    }
    else if (strcmp(method, "GET") == 0 && path_matches(path, "/api/v1/face/libs"))
    {
        print_command_request("获取目标库", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"OK\",\"Data\":{\"Libs\":[{\"FaceLibID\":1,\"LibName\":\"员工库\"}]}}");
    }
    else if (strcmp(method, "POST") == 0 && path_matches(path, "/api/v1/face/libs"))
    {
        print_command_request("添加目标库", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"Face lib added\",\"Data\":{\"FaceLibID\":1}}");
    }
    else if (strcmp(method, "PUT") == 0 && path_matches(path, "/api/v1/face/libs"))
    {
        print_command_request("修改目标库", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"Face lib updated\",\"Data\":{}}");
    }
    else if (strcmp(method, "DELETE") == 0 && path_matches(path, "/api/v1/face/libs"))
    {
        print_command_request("删除目标库", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"Face lib deleted\",\"Data\":{}}");
    }
    else if (strcmp(method, "GET") == 0 && path_matches(path, "/api/v1/face/persons"))
    {
        print_command_request("获取人脸", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"OK\",\"Data\":{\"Persons\":[{\"FaceID\":10001,\"FaceLibID\":1,\"Name\":\"张三\"}]}}");
    }
    else if (strcmp(method, "POST") == 0 && path_matches(path, "/api/v1/face/persons"))
    {
        print_command_request("添加人脸", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"Face person added\",\"Data\":{\"FaceID\":10001}}");
    }
    else if (strcmp(method, "PUT") == 0 && path_matches(path, "/api/v1/face/persons"))
    {
        print_command_request("修改人脸", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"Face person updated\",\"Data\":{}}");
    }
    else if (strcmp(method, "DELETE") == 0 && path_matches(path, "/api/v1/face/persons"))
    {
        print_command_request("删除人脸", method, path, body);
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"Face person deleted\",\"Data\":{}}");
    }
    else if (strcmp(method, "GET") == 0 && path_matches(path, "/health"))
    {
        send_json_response(client, 200, "{\"Ret\":0,\"Message\":\"OK\",\"Data\":{}}");
    }
    else
    {
        send_json_response(client, 404, "{\"Ret\":-1,\"Message\":\"Not found\",\"Data\":{}}");
    }
}

/**
 * @brief 创建监听 socket
 * @param port 监听端口
 * @return socket 句柄，失败返回 INVALID_SOCKET
 */
static socket_handle_t create_listen_socket(int port)
{
    socket_handle_t sock = INVALID_SOCKET;
    struct sockaddr_in addr;
    int reuse = 1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        print_socket_error("socket");
        return INVALID_SOCKET;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        print_socket_error("bind");
        CLOSE_SOCKET(sock);
        return INVALID_SOCKET;
    }

    if (listen(sock, 16) == SOCKET_ERROR)
    {
        print_socket_error("listen");
        CLOSE_SOCKET(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

/**
 * @brief HTTP 命令服务线程
 * @param arg ServerThreadParam 指针
 */
static THREAD_RET server_thread_proc(void *arg)
{
    ServerThreadParam *param = (ServerThreadParam *)arg;
    if (g_listen_socket == INVALID_SOCKET)
    {
        g_running = 0;
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }

    printf("HTTP人脸命令服务监听地址: http://0.0.0.0:%d\n", param->listen_port);

    while (g_running)
    {
        fd_set read_set;
        struct timeval timeout;
        int select_ret = 0;
        socket_handle_t client = INVALID_SOCKET;

        FD_ZERO(&read_set);
        FD_SET(g_listen_socket, &read_set);
        timeout.tv_sec = 0;
        timeout.tv_usec = 200 * 1000;

        select_ret = select((int)g_listen_socket + 1, &read_set, NULL, NULL, &timeout);
        if (select_ret <= 0)
        {
            continue;
        }

        client = accept(g_listen_socket, NULL, NULL);
        if (client == INVALID_SOCKET)
        {
            continue;
        }

        handle_http_client(client);
        CLOSE_SOCKET(client);
    }

    if (g_listen_socket != INVALID_SOCKET)
    {
        CLOSE_SOCKET(g_listen_socket);
        g_listen_socket = INVALID_SOCKET;
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/**
 * @brief 解析 HTTP URL，仅支持 http://host:port/path
 * @param url URL 字符串
 * @param parts 输出解析结果
 * @return 0 表示成功，非 0 表示失败
 */
static int parse_http_url(const char *url, UrlParts *parts)
{
    const char *cursor = url;
    const char *slash = NULL;
    const char *colon = NULL;
    size_t host_len = 0;
    char host_port[HTTP_URL_HOST_SIZE + 16] = {0};

    memset(parts, 0, sizeof(*parts));
    parts->port = 80;
    strcpy(parts->path, "/");

    if (strncmp(cursor, "http://", 7) == 0)
    {
        cursor += 7;
    }
    else if (strstr(cursor, "://") != NULL)
    {
        printf("C语言Demo仅支持http://回调地址: %s\n", url);
        return -1;
    }

    slash = strchr(cursor, '/');
    if (slash == NULL)
    {
        host_len = strlen(cursor);
    }
    else
    {
        host_len = (size_t)(slash - cursor);
        snprintf(parts->path, sizeof(parts->path), "%s", slash);
    }

    if (host_len == 0 || host_len >= sizeof(host_port))
    {
        return -1;
    }

    memcpy(host_port, cursor, host_len);
    host_port[host_len] = '\0';

    colon = strrchr(host_port, ':');
    if (colon != NULL)
    {
        size_t pure_host_len = (size_t)(colon - host_port);
        if (pure_host_len == 0 || pure_host_len >= sizeof(parts->host))
        {
            return -1;
        }
        memcpy(parts->host, host_port, pure_host_len);
        parts->host[pure_host_len] = '\0';
        parts->port = atoi(colon + 1);
    }
    else
    {
        snprintf(parts->host, sizeof(parts->host), "%s", host_port);
    }

    if (parts->port <= 0)
    {
        parts->port = 80;
    }
    return 0;
}

/**
 * @brief 连接指定主机
 * @param host 主机名或 IP
 * @param port 端口
 * @return socket 句柄，失败返回 INVALID_SOCKET
 */
static socket_handle_t connect_to_host(const char *host, int port)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *item = NULL;
    char port_text[16];
    socket_handle_t sock = INVALID_SOCKET;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(port_text, sizeof(port_text), "%d", port);

    if (getaddrinfo(host, port_text, &hints, &result) != 0)
    {
        return INVALID_SOCKET;
    }

    for (item = result; item != NULL; item = item->ai_next)
    {
        sock = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (sock == INVALID_SOCKET)
        {
            continue;
        }

        if (connect(sock, item->ai_addr, (int)item->ai_addrlen) == 0)
        {
            break;
        }

        CLOSE_SOCKET(sock);
        sock = INVALID_SOCKET;
    }

    freeaddrinfo(result);
    return sock;
}

/**
 * @brief 构造 multipart 请求体
 * @param parts 表单字段数组
 * @param part_count 表单字段数量
 * @param out_body 输出请求体
 * @return 0 表示成功，非 0 表示失败
 */
static int build_multipart_body(const FormPart *parts, size_t part_count, DynamicBuffer *out_body)
{
    size_t i = 0;
    buffer_init(out_body);

    for (i = 0; i < part_count; ++i)
    {
        const FormPart *part = &parts[i];
        if (buffer_append_format(out_body, "--%s\r\n", HTTP_BOUNDARY) != 0)
        {
            return -1;
        }

        if (part->filename != NULL && part->filename[0] != '\0')
        {
            if (buffer_append_format(out_body,
                                     "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
                                     "Content-Type: %s\r\n\r\n",
                                     part->name,
                                     part->filename,
                                     part->content_type ? part->content_type : "application/octet-stream") != 0)
            {
                return -1;
            }
        }
        else
        {
            if (buffer_append_format(out_body,
                                     "Content-Disposition: form-data; name=\"%s\"\r\n\r\n",
                                     part->name) != 0)
            {
                return -1;
            }
        }

        if (buffer_append(out_body, part->data, part->data_len) != 0 ||
            buffer_append_string(out_body, "\r\n") != 0)
        {
            return -1;
        }
    }

    return buffer_append_format(out_body, "--%s--\r\n", HTTP_BOUNDARY);
}

/**
 * @brief 发送 multipart HTTP 推送
 * @param callback_url 客户端回调 URL
 * @param parts 表单字段数组
 * @param part_count 表单字段数量
 * @return HTTP 响应结果
 */
static HttpResult post_multipart_event(const char *callback_url, const FormPart *parts, size_t part_count)
{
    UrlParts url_parts;
    DynamicBuffer body;
    DynamicBuffer request;
    HttpResult result;
    socket_handle_t sock = INVALID_SOCKET;
    char response[HTTP_SMALL_BUFFER_SIZE];
    int recv_len = 0;

    memset(&result, 0, sizeof(result));
    buffer_init(&body);
    buffer_init(&request);

    if (parse_http_url(callback_url, &url_parts) != 0)
    {
        printf("[推送] 回调地址无效: %s\n", callback_url);
        return result;
    }

    if (build_multipart_body(parts, part_count, &body) != 0)
    {
        printf("[推送] 构造multipart请求体失败\n");
        buffer_free(&body);
        return result;
    }

    if (buffer_append_format(&request,
                             "POST %s HTTP/1.1\r\n"
                             "Host: %s:%d\r\n"
                             "Content-Type: multipart/form-data; boundary=%s\r\n"
                             "Content-Length: %u\r\n"
                             "Connection: close\r\n"
                             "\r\n",
                             url_parts.path,
                             url_parts.host,
                             url_parts.port,
                             HTTP_BOUNDARY,
                             (unsigned int)body.size) != 0 ||
        buffer_append(&request, body.data, body.size) != 0)
    {
        printf("[推送] 构造HTTP请求失败\n");
        buffer_free(&body);
        buffer_free(&request);
        return result;
    }

    sock = connect_to_host(url_parts.host, url_parts.port);
    if (sock == INVALID_SOCKET)
    {
        printf("[推送] 连接客户端失败: %s:%d\n", url_parts.host, url_parts.port);
        buffer_free(&body);
        buffer_free(&request);
        return result;
    }

    if (send_all(sock, request.data, request.size) != 0)
    {
        printf("[推送] 发送HTTP请求失败\n");
        CLOSE_SOCKET(sock);
        buffer_free(&body);
        buffer_free(&request);
        return result;
    }

    recv_len = recv(sock, response, sizeof(response) - 1, 0);
    if (recv_len > 0)
    {
        response[recv_len] = '\0';
        sscanf(response, "HTTP/%*s %d", &result.status);
        snprintf(result.body, sizeof(result.body), "%s", response);
    }

    CLOSE_SOCKET(sock);
    buffer_free(&body);
    buffer_free(&request);
    return result;
}

/**
 * @brief 构造文本表单字段
 * @param name 字段名
 * @param text 字段值
 * @return 表单字段
 */
static FormPart make_text_part(const char *name, const char *text)
{
    FormPart part;
    part.name = name;
    part.data = (const unsigned char *)text;
    part.data_len = strlen(text);
    part.filename = NULL;
    part.content_type = NULL;
    return part;
}

/**
 * @brief 构造文件表单字段
 * @param name 字段名
 * @param data 文件数据
 * @param data_len 文件长度
 * @param filename 文件名
 * @param content_type 文件类型
 * @return 表单字段
 */
static FormPart make_file_part(const char *name,
                               const unsigned char *data,
                               size_t data_len,
                               const char *filename,
                               const char *content_type)
{
    FormPart part;
    part.name = name;
    part.data = data;
    part.data_len = data_len;
    part.filename = filename;
    part.content_type = content_type;
    return part;
}

/**
 * @brief 构造并推送人脸抓拍事件
 * @param callback_url 客户端回调 URL
 */
static void push_face_capture_event(const char *callback_url)
{
    char timestamp[32];
    char jpeg_len[32];
    HttpResult result;
    FormPart parts[16];
    int index = 0;

    snprintf(timestamp, sizeof(timestamp), "%lld", current_timestamp_ms());
    snprintf(jpeg_len, sizeof(jpeg_len), "%u", (unsigned int)sizeof(g_demo_jpeg));
    (void)jpeg_len;

    parts[index++] = make_text_part("EventType", "FACE_CAPTURE");
    parts[index++] = make_text_part("Command", "NET_TV_ALARM_FACE_CAPTURE");
    parts[index++] = make_text_part("AlarmType", "NET_TV_ALARM_FACE_CAPTURE");
    parts[index++] = make_text_part("AlarmCode", "12290");
    parts[index++] = make_text_part("DeviceCode", "SDK_HTTP_FACE_SERVER");
    parts[index++] = make_text_part("Channel", "0");
    parts[index++] = make_text_part("TimestampMs", timestamp);
    parts[index++] = make_text_part("TargetCount", "1");
    parts[index++] = make_text_part("Targets[0].Left", "120");
    parts[index++] = make_text_part("Targets[0].Top", "80");
    parts[index++] = make_text_part("Targets[0].Right", "360");
    parts[index++] = make_text_part("Targets[0].Bottom", "420");
    parts[index++] = make_text_part("Targets[0].Confidence", "0.965000");
    parts[index++] = make_text_part("Targets[0].Ipd", "64");
    parts[index++] = make_file_part("PanoramaImage", g_demo_jpeg, sizeof(g_demo_jpeg), "face_capture_panorama.jpg", "image/jpeg");
    parts[index++] = make_file_part("TargetImages[0]", g_demo_jpeg, sizeof(g_demo_jpeg), "face_capture_target_1.jpg", "image/jpeg");

    result = post_multipart_event(callback_url, parts, (size_t)index);
    printf("[推送] 人脸抓拍 FACE_CAPTURE HTTP状态码=%d %s\n",
           result.status,
           result.status == 200 ? "成功" : "失败");
}

/**
 * @brief 构造并推送人脸比对事件
 * @param callback_url 客户端回调 URL
 * @param success 是否比对成功
 */
static void push_face_compare_event(const char *callback_url, int success)
{
    char timestamp[32];
    char capture_len[32];
    char lib_len[32];
    HttpResult result;
    FormPart parts[21];
    int index = 0;

    snprintf(timestamp, sizeof(timestamp), "%lld", current_timestamp_ms());
    snprintf(capture_len, sizeof(capture_len), "%u", (unsigned int)sizeof(g_demo_jpeg));
    snprintf(lib_len, sizeof(lib_len), "%u", success ? (unsigned int)sizeof(g_demo_jpeg) : 0U);

    parts[index++] = make_text_part("EventType", "FACE_COMPARE");
    parts[index++] = make_text_part("Command", "NET_TV_ALARM_FACE_COMPARE");
    parts[index++] = make_text_part("AlarmType", "NET_TV_ALARM_FACE_COMPARE");
    parts[index++] = make_text_part("AlarmCode", "12295");
    parts[index++] = make_text_part("DeviceCode", "SDK_HTTP_FACE_SERVER");
    parts[index++] = make_text_part("Channel", "0");
    parts[index++] = make_text_part("TimestampMs", timestamp);
    parts[index++] = make_text_part("CompareResult", success ? "1" : "0");
    parts[index++] = make_text_part("FaceID", success ? "10001" : "-1");
    parts[index++] = make_text_part("Similarity", success ? "0.876000" : "0.320000");
    parts[index++] = make_text_part("SimilarityPercent", success ? "87" : "32");
    parts[index++] = make_text_part("FaceName", success ? "张三" : "");
    parts[index++] = make_text_part("FaceLibName", success ? "员工库" : "");
    parts[index++] = make_text_part("LibFacePath", success ? "/opt/cam/face/lib/10001.jpg" : "");
    parts[index++] = make_text_part("CaptureFacePath", "/tmp/face_compare_capture.jpg");
    parts[index++] = make_text_part("CaptureImagePath", "/tmp/face_compare_panorama.jpg");
    parts[index++] = make_text_part("CaptureFaceImgLen", capture_len);
    parts[index++] = make_text_part("LibFaceImgLen", lib_len);
    parts[index++] = make_file_part("CaptureFaceImage", g_demo_jpeg, sizeof(g_demo_jpeg), "face_compare_capture.jpg", "image/jpeg");

    if (success)
    {
        parts[index++] = make_file_part("LibFaceImage", g_demo_jpeg, sizeof(g_demo_jpeg), "face_compare_library.jpg", "image/jpeg");
    }

    result = post_multipart_event(callback_url, parts, (size_t)index);
    printf("[推送] 人脸比对 FACE_COMPARE 比对结果=%s HTTP状态码=%d %s\n",
           success ? "成功" : "失败",
           result.status,
           result.status == 200 ? "推送成功" : "推送失败");
}

/**
 * @brief 定时推送线程
 * @param arg PushThreadParam 指针
 */
static THREAD_RET push_thread_proc(void *arg)
{
    PushThreadParam *param = (PushThreadParam *)arg;
    int index = 0;

    while (g_running)
    {
        if (index % 3 == 0)
        {
            push_face_capture_event(param->callback_url);
        }
        else
        {
            push_face_compare_event(param->callback_url, index % 2 == 1);
        }

        ++index;
        for (int i = 0; i < param->interval_sec && g_running; ++i)
        {
            sleep_ms(1000);
        }
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int main(int argc, char *argv[])
{
    ServerThreadParam server_param;
    PushThreadParam push_param;

#ifdef _WIN32
    HANDLE server_thread = NULL;
    HANDLE push_thread = NULL;
#else
    pthread_t server_thread;
    pthread_t push_thread;
#endif

    if (argc < 4)
    {
        printf("用法: %s <命令监听端口> <客户端回调地址> <推送间隔秒>\n", argv[0]);
        printf("示例: %s 9000 http://127.0.0.1:18080/face/event 5\n", argv[0]);
        return 0;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (network_init() != 0)
    {
        printf("网络初始化失败\n");
        return 1;
    }

    server_param.listen_port = atoi(argv[1]);
    memset(&push_param, 0, sizeof(push_param));
    snprintf(push_param.callback_url, sizeof(push_param.callback_url), "%s", argv[2]);
    push_param.interval_sec = atoi(argv[3]);
    if (push_param.interval_sec <= 0)
    {
        push_param.interval_sec = 1;
    }

    printf("推送目标地址: %s\n", push_param.callback_url);
    printf("按 Ctrl+C 停止Demo。\n");

    g_listen_socket = create_listen_socket(server_param.listen_port);
    if (g_listen_socket == INVALID_SOCKET)
    {
        printf("监听端口%d失败，请检查端口是否已被占用。\n",
               server_param.listen_port);
        network_cleanup();
        return 1;
    }

#ifdef _WIN32
    server_thread = CreateThread(NULL, 0, server_thread_proc, &server_param, 0, NULL);
    push_thread = CreateThread(NULL, 0, push_thread_proc, &push_param, 0, NULL);
    if (server_thread == NULL || push_thread == NULL)
    {
        printf("创建Windows线程失败\n");
        g_running = 0;
    }
#else
    if (pthread_create(&server_thread, NULL, server_thread_proc, &server_param) != 0 ||
        pthread_create(&push_thread, NULL, push_thread_proc, &push_param) != 0)
    {
        printf("创建pthread线程失败\n");
        g_running = 0;
    }
#endif

    while (g_running)
    {
        sleep_ms(200);
    }

    if (g_listen_socket != INVALID_SOCKET)
    {
        CLOSE_SOCKET(g_listen_socket);
        g_listen_socket = INVALID_SOCKET;
    }

#ifdef _WIN32
    if (push_thread != NULL)
    {
        WaitForSingleObject(push_thread, INFINITE);
        CloseHandle(push_thread);
    }
    if (server_thread != NULL)
    {
        WaitForSingleObject(server_thread, INFINITE);
        CloseHandle(server_thread);
    }
#else
    pthread_join(push_thread, NULL);
    pthread_join(server_thread, NULL);
#endif

    network_cleanup();
    printf("HTTP人脸服务端Demo已停止。\n");
    return 0;
}
