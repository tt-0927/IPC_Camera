#!/usr/bin/env python3
"""
SDK Client Demo Web 测试服务器 (零外部依赖版)
通过浏览器按钮驱动 ClientTestDemo，终端输出实时显示在网页上。

用法:
    python3 web_server.py [demo_binary_path]
    # 浏览器打开 http://localhost:8080

依赖: 仅 Python3 标准库 + pty (Linux)
"""
import os, sys, pty, select, json, time, threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
from socketserver import ThreadingMixIn

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEMO_BIN = sys.argv[1] if len(sys.argv) > 1 else "./build/ClientTestDemo"

# ===================== Demo 进程管理 =====================

class DemoSession:
    """管理一个 Demo 子进程 + PTY"""

    def __init__(self):
        self.pid = None
        self.master_fd = None
        self.output_buf = ""
        self.lock = threading.Lock()
        self.running = False
        self._thread = None

    def start(self, binary, server_ip, port, user, passwd):
        self.stop()
        if not os.path.isfile(binary):
            return False, f"找不到二进制文件: {binary}"
        if not os.access(binary, os.X_OK):
            return False, f"无执行权限: {binary}"

        pid, fd = pty.fork()
        if pid == 0:
            os.execvp(binary, [binary, server_ip, str(port), user, passwd])
        self.pid, self.master_fd = pid, fd
        self.running = True
        self.output_buf = ""
        self._thread = threading.Thread(target=self._reader, daemon=True)
        self._thread.start()
        time.sleep(0.8)
        return True, f"PID={pid}"

    def _reader(self):
        while self.running and self.master_fd is not None:
            try:
                r, _, _ = select.select([self.master_fd], [], [], 0.3)
                if self.master_fd in r:
                    data = os.read(self.master_fd, 8192)
                    if not data:
                        self.running = False
                        break
                    with self.lock:
                        text = data.decode("utf-8", errors="replace")
                        self.output_buf += text
                        if len(self.output_buf) > 500000:
                            self.output_buf = self.output_buf[-250000:]
            except OSError:
                self.running = False
                break

    def send(self, text):
        if self.master_fd is None:
            return False
        try:
            os.write(self.master_fd, (text + "\n").encode())
            return True
        except OSError:
            return False

    def get_output(self, since=0):
        with self.lock:
            data = self.output_buf[since:]
            return data, len(self.output_buf)

    def stop(self):
        self.running = False
        if self.pid:
            try:
                os.kill(self.pid, 9)
            except OSError:
                pass
            try:
                os.waitpid(self.pid, os.WNOHANG)
            except (OSError, ChildProcessError):
                pass
        if self.master_fd is not None:
            try:
                os.close(self.master_fd)
            except OSError:
                pass
        self.pid = self.master_fd = None

session = DemoSession()

# ===================== HTTP 请求处理 =====================

def json_bytes(obj):
    return json.dumps(obj, ensure_ascii=False).encode("utf-8")

class Handler(BaseHTTPRequestHandler):
    """路由: GET / | POST /api/connect | POST /api/disconnect | POST /api/send | GET /api/output"""

    def log_message(self, fmt, *args):
        pass  # 静默日志，避免刷屏

    def _send_json(self, obj, code=200):
        body = json_bytes(obj)
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _read_body(self):
        length = int(self.headers.get("Content-Length", 0))
        return self.rfile.read(length) if length else b""

    # ---------- GET ----------

    def do_GET(self):
        parsed = urlparse(self.path)

        if parsed.path == "/":
            html_path = os.path.join(SCRIPT_DIR, "templates", "index.html")
            try:
                with open(html_path, "rb") as f:
                    data = f.read()
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)
            except FileNotFoundError:
                self._send_json({"error": "找不到 index.html"}, 404)
            return

        if parsed.path == "/api/output":
            qs = parse_qs(parsed.query)
            pos = int(qs.get("pos", [0])[0])
            data, new_pos = session.get_output(pos)
            self._send_json({"data": data, "pos": new_pos, "alive": session.running})
            return

        self._send_json({"error": "not found"}, 404)

    # ---------- POST ----------

    def do_POST(self):
        parsed = urlparse(self.path)
        raw = self._read_body()
        try:
            body = json.loads(raw) if raw else {}
        except json.JSONDecodeError:
            body = {}

        if parsed.path == "/api/connect":
            ok, msg = session.start(
                body.get("binary", DEMO_BIN),
                body.get("ip", "172.16.25.213"),
                int(body.get("port", 9019)),
                body.get("user", "admin"),
                body.get("passwd", ""),
            )
            self._send_json({"ok": ok, "msg": msg})
            return

        if parsed.path == "/api/disconnect":
            session.send("99")
            time.sleep(0.3)
            session.stop()
            self._send_json({"ok": True})
            return

        if parsed.path == "/api/send":
            cmd = body.get("cmd", "")
            self._send_json({"ok": session.send(cmd)})
            return

        self._send_json({"error": "not found"}, 404)

# ===================== 多线程 HTTP 服务器 =====================

class ThreadedServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True
    allow_reuse_address = True

# ===================== Main =====================

if __name__ == "__main__":
    port = int(os.environ.get("WEB_PORT", 8080))
    server = ThreadedServer(("0.0.0.0", port), Handler)
    print(f"Web 测试界面: http://0.0.0.0:{port}")
    print(f"Demo 二进制:  {DEMO_BIN}")
    print("Ctrl+C 退出")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n正在关闭...")
    finally:
        session.stop()
        server.server_close()
