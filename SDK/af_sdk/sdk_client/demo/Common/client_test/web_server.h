/**
 * @file web_server.h
 * @brief 嵌入式 HTTP 服务器 (header-only, 零外部依赖)
 *        用于 SDK Client Demo 的 Web 测试界面
 *
 * 用法:
 *   WebServer ws;
 *   ws.start(8080);     // 启动 HTTP 服务器
 *   ws.enqueueCommand(1); // 排队执行测试命令 1
 *   ws.stop();           // 停止
 *
 * 注意: 此实现仅适用于 Linux (使用 POSIX socket/pipe/pthread)
 */
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* 外部 HTML 内容 (由 gen_index_h.py 生成) */
#include "index_html.h"

class WebServer {
public:
    WebServer() = default;
    ~WebServer() { stop(); }

    /* 启动服务器: 重定向 stdout -> pipe, 启动监听线程 + 输出读取线程 */
    bool start(int port) {
        if (running_) return false;

        /* 创建管道用于捕获 stdout */
        int pipefd[2];
        if (pipe(pipefd) != 0) {
            fprintf(stderr, "[WebServer] pipe() 失败\n");
            return false;
        }

        /* 创建管道用于网页输入 -> stdin (交互参数经 /api/input 到达) */
        int inpipefd[2];
        if (pipe(inpipefd) != 0) {
            fprintf(stderr, "[WebServer] pipe(stdin) 失败\n");
            close(pipefd[0]);
            close(pipefd[1]);
            return false;
        }

        fflush(stdout);
        saved_stdout_ = dup(STDOUT_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);  /* stdout -> pipe 写端 */
        close(pipefd[1]);                /* 只保留 fd 1 作为写端 */
        pipe_read_fd_ = pipefd[0];

        saved_stdin_ = dup(STDIN_FILENO);
        dup2(inpipefd[0], STDIN_FILENO); /* stdin <- pipe 读端 */
        close(inpipefd[0]);              /* 只保留 fd 0 作为读端 */
        stdin_write_fd_ = inpipefd[1];

        /* stdout 指向管道后为全缓冲, 置为无缓冲让提示符实时到达网页 */
        setvbuf(stdout, nullptr, _IONBF, 0);

        running_ = true;
        out_thread_ = std::thread(&WebServer::outputReader, this);

        listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd_ < 0) { running_ = false; return false; }
        int opt = 1;
        setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons((uint16_t)port);

        if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            fprintf(stderr, "[WebServer] bind(%d) 失败\n", port);
            close(listen_fd_); listen_fd_ = -1; running_ = false; return false;
        }
        listen(listen_fd_, 8);

        srv_thread_ = std::thread(&WebServer::serverLoop, this);
        return true;
    }

    void stop() {
        if (!running_) return;
        running_ = false;
        if (listen_fd_ >= 0) { close(listen_fd_); listen_fd_ = -1; }
        if (srv_thread_.joinable()) srv_thread_.join();

        /* 等待所有连接处理线程退出 (单请求 recv 超时最长 5s), 防止析构后悬垂访问 */
        {
            std::unique_lock<std::mutex> lk(conn_mtx_);
            conn_cv_.wait(lk, [this] { return conn_count_.load() <= 0; });
        }

        /* 关闭网页输入管道并恢复 stdin */
        if (stdin_write_fd_ >= 0) { close(stdin_write_fd_); stdin_write_fd_ = -1; }
        if (saved_stdin_ >= 0) {
            dup2(saved_stdin_, STDIN_FILENO);
            close(saved_stdin_); saved_stdin_ = -1;
        }

        /* 关闭管道写端, 让 reader 线程读到 EOF 退出 */
        if (saved_stdout_ >= 0) {
            dup2(saved_stdout_, STDOUT_FILENO);
            close(saved_stdout_); saved_stdout_ = -1;
        }
        if (out_thread_.joinable()) out_thread_.join();
        if (pipe_read_fd_ >= 0) { close(pipe_read_fd_); pipe_read_fd_ = -1; }
    }

    /* 排队一个命令编号 (对应菜单 0~121, 999=退出程序) */
    void enqueueCommand(int cmd) {
        std::lock_guard<std::mutex> lk(cmd_mtx_);
        cmd_queue_.push(cmd);
    }

    /* 是否有待处理的命令 */
    bool hasCommand() const {
        std::lock_guard<std::mutex> lk(cmd_mtx_);
        return !cmd_queue_.empty();
    }

    /* 取出队头命令 (-1 表示空) */
    int popCommand() {
        std::lock_guard<std::mutex> lk(cmd_mtx_);
        if (cmd_queue_.empty()) return -1;
        int c = cmd_queue_.front(); cmd_queue_.pop(); return c;
    }

    /* 标记测试执行中, 阻止新命令入队 */
    void setBusy(bool b) { busy_.store(b); }
    bool isBusy() const { return busy_.load(); }

    /* 清空命令队列 (中止测试后排空积压的旧命令) */
    void drainQueue() {
        std::lock_guard<std::mutex> lk(cmd_mtx_);
        while (!cmd_queue_.empty()) cmd_queue_.pop();
    }

    /* 空闲期(无测试运行)消费滞留在 stdin 管道中的输入行:
     * - quit: 无测试可中止, 提示后忽略
     * - clear: 立即输出清屏标记, 前端收到后清空显示
     * - 其他: 丢弃并提示, 防止滞留输入被下一条命令的第一个 fgets 误读 */
    void drainIdleInput() {
        if (stdin_write_fd_ < 0) return;
        std::string pending;
        char buf[2048];
        while (true) {
            fd_set rfds; FD_ZERO(&rfds); FD_SET(STDIN_FILENO, &rfds);
            struct timeval tv = {0, 0};
            if (select(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv) <= 0) break;
            ssize_t rd = read(STDIN_FILENO, buf, sizeof(buf));
            if (rd <= 0) break;
            pending.append(buf, (size_t)rd);
            size_t nl;
            while ((nl = pending.find('\n')) != std::string::npos) {
                handleIdleLine(pending.substr(0, nl));
                pending.erase(0, nl + 1);
            }
        }
        if (!pending.empty()) handleIdleLine(pending);  /* 防御: 无换行尾巴按一行处理 */
    }

    /* 获取输出缓冲区新增内容 (增量轮询) */
    std::string getOutput(size_t since) {
        std::lock_guard<std::mutex> lk(out_mtx_);
        if (since >= out_buf_.size()) return "";
        return out_buf_.substr(since);
    }
    size_t outputPos() const {
        std::lock_guard<std::mutex> lk(out_mtx_);
        return out_buf_.size();
    }

private:
    /* ---- 成员 ---- */
    std::atomic<bool>  running_{false};
    std::atomic<bool>  busy_{false};       /* 测试执行中标志 */
    int                listen_fd_{-1};
    int                pipe_read_fd_{-1};
    int                saved_stdout_{-1};
    int                stdin_write_fd_{-1};  /* 网页输入 -> stdin 管道写端 */
    int                saved_stdin_{-1};
    std::thread        srv_thread_;
    std::thread        out_thread_;

    mutable std::mutex out_mtx_;
    std::string        out_buf_;

    mutable std::mutex cmd_mtx_;
    std::queue<int>    cmd_queue_;

    std::mutex         in_mtx_;         /* stdin 管道写锁: 保证整行原子写入 */
    std::atomic<int>   conn_count_{0};  /* 活跃连接处理线程数 */
    std::mutex         conn_mtx_;
    std::condition_variable conn_cv_;   /* conn_count_ 归零通知 */

    /* ---- 输出读取线程: 从 pipe 读 stdout 数据 ---- */
    void outputReader() {
        char buf[4096];
        while (running_) {
            fd_set rfds; FD_ZERO(&rfds); FD_SET(pipe_read_fd_, &rfds);
            struct timeval tv = {0, 200000}; /* 200ms */
            int n = select(pipe_read_fd_ + 1, &rfds, nullptr, nullptr, &tv);
            if (n > 0) {
                ssize_t rd = read(pipe_read_fd_, buf, sizeof(buf));
                if (rd <= 0) break;
                std::lock_guard<std::mutex> lk(out_mtx_);
                out_buf_.append(buf, (size_t)rd);
                if (out_buf_.size() > 500000)
                    out_buf_ = out_buf_.substr(out_buf_.size() - 250000);
            }
        }
    }

    /* ---- HTTP 服务线程 ---- */
    void serverLoop() {
        fprintf(stderr, "[WebServer] serverLoop 启动 (per-connection 线程模式)\n");
        while (running_) {
            struct sockaddr_in cli{};
            socklen_t clen = sizeof(cli);
            int cfd = accept(listen_fd_, (struct sockaddr*)&cli, &clen);
            if (cfd < 0) continue;
            /* 每连接独立线程处理: 浏览器会预建不发送数据的 speculative 连接,
             * 若在 accept 循环内同步处理, 会阻塞在 recv 直至超时, 期间
             * /api/input (quit/clear) 等请求排队, 无法及时写入 stdin 管道,
             * 网页表现为"输入后无响应, 下一条命令时才生效" */
            conn_count_++;
            std::thread([this, cfd] {
                handleClient(cfd);
                close(cfd);
                std::lock_guard<std::mutex> lk(conn_mtx_);
                if (--conn_count_ == 0) conn_cv_.notify_all();
            }).detach();
        }
    }

    void handleClient(int fd) {
        /* 读超时: 浏览器预连接(speculative connection)或僵尸连接不发数据,
         * 会永久阻塞唯一的服务线程, 导致后续所有 HTTP 请求(含 /api/input 和轮询)失效,
         * 网页表现为"输入后卡住"。超时后关闭连接, 服务线程得以继续 accept。 */
        struct timeval tv = {5, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        char buf[4096];
        ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) return;
        buf[n] = '\0';

        char method[8] = {}, path[256] = {};
        sscanf(buf, "%7s %255s", method, path);

        const char *body = strstr(buf, "\r\n\r\n");
        if (body) body += 4;

        if (!strcmp(method, "GET") && !strcmp(path, "/")) {
            sendResp(fd, 200, "text/html; charset=utf-8",
                     INDEX_HTML, strlen(INDEX_HTML));
        } else if (!strcmp(method, "GET") && !strncmp(path, "/api/output", 11)) {
            size_t pos = 0;
            const char *p = strstr(path, "pos=");
            if (p) pos = (size_t)atoll(p + 4);
            /* 分块返回: 大输出(如全通道列表 JSON)经 jsonEscape 后超过固定缓冲区,
             * snprintf 截断会产生非法 JSON, 前端 r.json() 解析失败且 pos 不前进,
             * 终端显示永久冻结( Demo 本身正常)。单次最多发 kMaxChunk 原始字符,
             * pos 只前进实际发送长度, 剩余数据等下次 poll 继续取。 */
            const size_t kMaxChunk = 2048;
            std::string data = getOutput(pos);
            if (data.size() > kMaxChunk) {
                fprintf(stderr, "[WebServer] output chunked: %zu -> %zu (pos=%zu)\n",
                        data.size(), kMaxChunk, pos);
                data.resize(kMaxChunk);
            }
            size_t np = pos + data.size();
            char jb[16384];
            snprintf(jb, sizeof(jb),
                     "{\"data\":%s,\"pos\":%zu,\"alive\":%s}",
                     jsonEscape(data).c_str(), np,
                     running_ ? "true" : "false");
            sendResp(fd, 200, "application/json", jb, strlen(jb));
        } else if (!strcmp(method, "GET") && !strcmp(path, "/api/status")) {
            /* 状态查询: 网页状态栏轮询 busy/alive, 展示测试运行与存活状态 */
            char sb[128];
            snprintf(sb, sizeof(sb),
                     "{\"busy\":%s,\"alive\":%s}",
                     busy_.load() ? "true" : "false",
                     running_ ? "true" : "false");
            sendResp(fd, 200, "application/json", sb, strlen(sb));
        } else if (!strcmp(method, "POST") && !strcmp(path, "/api/cmd")) {
            if (busy_.load()) {
                const char *busy_resp = "{\"ok\":false,\"busy\":true,\"msg\":\"正在测试中, 请等待完成或输入 quit 中止\"}";
                sendResp(fd, 200, "application/json", busy_resp, strlen(busy_resp));
            } else {
                int cmd = 0;
                if (body) {
                    const char *c = strstr(body, "\"cmd\"");
                    if (c) { c = strchr(c, ':'); if (c) cmd = atoi(c + 1); }
                }
                enqueueCommand(cmd);
                const char *ok = "{\"ok\":true}";
                sendResp(fd, 200, "application/json", ok, strlen(ok));
            }
        } else if (!strcmp(method, "POST") && !strcmp(path, "/api/input")) {
            /* 将网页输入的一行文本写入 Demo 的 stdin 管道
             * (整行含换行一次写入并加锁, 避免并发请求时行间交错) */
            if (body && stdin_write_fd_ >= 0) {
                std::string line = jsonGetStr(body, "text");
                line += '\n';
                std::lock_guard<std::mutex> lk(in_mtx_);
                write(stdin_write_fd_, line.c_str(), line.size());
            }
            const char *ok = "{\"ok\":true}";
            sendResp(fd, 200, "application/json", ok, strlen(ok));
        } else {
            const char *nf = "{\"error\":\"not found\"}";
            sendResp(fd, 404, "application/json", nf, strlen(nf));
        }
    }

    void sendResp(int fd, int code, const char *ctype,
                  const char *body, size_t len) {
        char hdr[256];
        int hlen = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n",
            code, code == 200 ? "OK" : "Not Found", ctype, len);
        send(fd, hdr, (size_t)hlen, 0);
        if (len > 0) send(fd, body, len, 0);
    }

    /* 处理空闲期滞留的一行输入 (仅主循环调用) */
    static void handleIdleLine(const std::string& line) {
        if (line == "quit") {
            printf("> [quit] 当前无运行中的测试, 已忽略\n");
        } else if (line == "clear") {
            printf("\n[CLEAR]\n");
        } else if (!line.empty()) {
            printf("> [丢弃] 空闲期输入: %s%s\n",
                   line.substr(0, 64).c_str(),
                   line.size() > 64 ? "..." : "");
        }
        fflush(stdout);
    }

    /* 从 JSON body 中提取字符串字段值 (简易解析, 支持常见转义) */
    static std::string jsonGetStr(const char *body, const char *key) {
        std::string out;
        char pat[32];
        snprintf(pat, sizeof(pat), "\"%s\"", key);
        const char *p = strstr(body, pat);
        if (!p) return out;
        p = strchr(p + strlen(pat), ':');
        if (!p) return out;
        p = strchr(p + 1, '"');
        if (!p) return out;
        for (++p; *p && *p != '"'; p++) {
            if (*p == '\\' && p[1]) {
                p++;
                if (*p == 'n')      out += '\n';
                else if (*p == 't') out += '\t';
                else if (*p == 'r') out += '\r';
                else                out += *p;
            } else {
                out += *p;
            }
        }
        return out;
    }

    /* JSON 字符串转义 */
    static std::string jsonEscape(const std::string &s) {
        std::string o;
        o.reserve(s.size() + 16);
        o += '"';
        for (char c : s) {
            switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char h[8]; snprintf(h, sizeof(h), "\\u%04x", (unsigned char)c);
                    o += h;
                } else {
                    o += c;
                }
            }
        }
        o += '"';
        return o;
    }
};
