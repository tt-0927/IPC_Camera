#!/usr/bin/env python3
"""RTSP 协议探针。

用途：在没有 VLC/NVR 的环境下，对 rtspserver 做可重复的协议级验证，
并把 RTP 头字段导出为 CSV，供 rtsp_rtp_check.py 校验。

覆盖的方法与场景：
  OPTIONS / DESCRIBE / SETUP / PLAY / PAUSE / TEARDOWN / GET_PARAMETER
  错误 CSeq、非法请求行、超长请求、流水线请求、Digest 认证（含错误口令）

用法示例：
  python3 tools/rtsp_probe.py --host 127.0.0.1 --port 8554 --path Streaming/Channels/101 --transport tcp
  python3 tools/rtsp_probe.py --host 127.0.0.1 --port 8554 --user admin --password pwd --all
  python3 tools/rtsp_probe.py --host 127.0.0.1 --port 8554 --suite negative
"""

from __future__ import annotations

import argparse
import base64
import csv
import hashlib
import os
import socket
import sys
import time
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

CRLF = "\r\n"


class RtspError(RuntimeError):
    """协议层断言失败。"""


@dataclass
class Response:
    status: int
    reason: str
    headers: Dict[str, str]
    body: bytes
    raw: bytes

    def header(self, name: str) -> Optional[str]:
        return self.headers.get(name.lower())


@dataclass
class RtpPacket:
    channel: int
    payload_type: int
    marker: int
    sequence: int
    timestamp: int
    ssrc: int
    size: int
    nal_type: int


@dataclass
class Stats:
    responses: int = 0
    rtp_packets: int = 0
    interleaved_frames: int = 0
    rtcp_datagrams: int = 0
    rtp_rows: List[RtpPacket] = field(default_factory=list)


class RtspClient:
    """极简 RTSP 客户端：只依赖标准库。"""

    def __init__(self, host: str, port: int, timeout: float = 5.0):
        self.host = host
        self.port = port
        self.cseq = 0
        self.session: Optional[str] = None
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.sock.settimeout(timeout)
        self.buffer = b""
        self.stats = Stats()
        self.user: Optional[str] = None
        self.password: Optional[str] = None
        self.udp_rtp: Optional[socket.socket] = None
        self.udp_rtcp: Optional[socket.socket] = None
        self.udp_rtp_port = 0
        self.udp_rtcp_port = 0

    def close(self) -> None:
        for sock in (self.sock, self.udp_rtp, self.udp_rtcp):
            try:
                if sock is not None:
                    sock.close()
            except OSError:
                pass

    def open_udp_ports(self) -> Tuple[int, int]:
        """为 RTP/RTCP over UDP 绑定两个本端端口，返回 (rtp_port, rtcp_port)。"""
        self.udp_rtp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp_rtp.bind(("0.0.0.0", 0))
        self.udp_rtp.settimeout(0.3)
        self.udp_rtcp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp_rtcp.bind(("0.0.0.0", 0))
        self.udp_rtcp.settimeout(0.3)
        self.udp_rtp_port = self.udp_rtp.getsockname()[1]
        self.udp_rtcp_port = self.udp_rtcp.getsockname()[1]
        return self.udp_rtp_port, self.udp_rtcp_port

    def pump_udp(self, budget_s: float = 0.2) -> int:
        """
        把 UDP socket 上排队的数据报读走；返回本次读到的 RTP 包数。

        注意：RTCP 是低频（默认 5 s 一个 SR），必须用非阻塞方式轮询，否则每轮都会
        在 RTCP socket 上白等一个超时周期，导致整个收流窗口被拖长。
        """
        drained = 0
        endpoint = time.time() + budget_s
        if self.udp_rtp is not None:
            self.udp_rtp.settimeout(0.02)
        if self.udp_rtcp is not None:
            self.udp_rtcp.settimeout(0.0)  # 非阻塞

        while time.time() < endpoint:
            got_any = False
            for sock, is_rtcp in ((self.udp_rtp, False), (self.udp_rtcp, True)):
                if sock is None:
                    continue
                try:
                    payload = sock.recv(65536)
                except (socket.timeout, BlockingIOError, OSError):
                    continue
                got_any = True
                if is_rtcp:
                    self.stats.rtcp_datagrams += 1
                    continue
                self._record_interleaved(0, payload)
                drained += 1
            if not got_any:
                break
        return drained

    # ---------------- 底层收发 ----------------

    def send_raw(self, data: bytes) -> None:
        self.sock.sendall(data)

    def _recv_some(self) -> bytes:
        chunk = self.sock.recv(65536)
        if not chunk:
            raise RtspError("连接被对端关闭")
        self.buffer += chunk
        return chunk

    def _ensure_buffered(self) -> None:
        """保证缓冲区至少有一个字节。"""
        while not self.buffer:
            self._recv_some()

    def _read_line(self) -> bytes:
        while b"\r\n" not in self.buffer:
            self._recv_some()
        line, _, rest = self.buffer.partition(b"\r\n")
        self.buffer = rest
        return line

    def _read_exact(self, count: int) -> bytes:
        while len(self.buffer) < count:
            self._recv_some()
        data, self.buffer = self.buffer[:count], self.buffer[count:]
        return data

    def _drain_interleaved(self) -> int:
        """解析缓冲区中所有完整的 interleaved 帧，返回解析个数。"""
        drained = 0
        while self.buffer[:1] == b"$" and len(self.buffer) >= 4:
            channel = self.buffer[1]
            length = int.from_bytes(self.buffer[2:4], "big")
            if len(self.buffer) < 4 + length:
                break
            payload = self.buffer[4 : 4 + length]
            self.buffer = self.buffer[4 + length :]
            self._record_interleaved(channel, payload)
            drained += 1
        return drained

    def pump(self, timeout: float = 0.5) -> int:
        """收一次数据并解析可解析的 interleaved 帧；超时返回 0。"""
        old_timeout = self.sock.gettimeout()
        self.sock.settimeout(timeout)
        try:
            self._recv_some()
        except socket.timeout:
            self.sock.settimeout(old_timeout)
            return 0
        finally:
            self.sock.settimeout(old_timeout)
        return self._drain_interleaved()

    def read_response(self) -> Response:
        """读取一个完整 RTSP 响应（跳过并记录 interleaved 数据帧）。"""
        while True:
            # 必须先有数据再判断类型：否则会把 interleaved 帧当成响应行。
            self._ensure_buffered()
            if self.buffer[:1] == b"$":
                if self._drain_interleaved() == 0:
                    self._recv_some()
                continue

            line = self._read_line()
            if not line.startswith(b"RTSP/"):
                raise RtspError(f"响应行非法: {line!r}")

            parts = line.decode("ascii", "replace").split(" ", 2)
            status = int(parts[1])
            reason = parts[2] if len(parts) > 2 else ""

            headers: Dict[str, str] = {}
            while True:
                header_line = self._read_line()
                if not header_line:
                    break
                key, _, value = header_line.decode("ascii", "replace").partition(":")
                headers[key.strip().lower()] = value.strip()

            body = b""
            if "content-length" in headers:
                body = self._read_exact(int(headers["content-length"]))

            self.stats.responses += 1
            response = Response(status=status, reason=reason, headers=headers, body=body, raw=line)
            if "session" in headers and self.session is None:
                self.session = headers["session"].split(";")[0]
            return response

    def _record_interleaved(self, channel: int, payload: bytes) -> None:
        self.stats.interleaved_frames += 1
        if len(payload) < 12:
            return
        payload_type = payload[1] & 0x7F
        if payload_type in (72, 73, 74, 75, 76):
            # RTCP（SR/RR/SDES/BYE/APP，200-204 去掉 marker 位后的值）：不计入 RTP 明细。
            self.stats.rtcp_datagrams += 1
            return
        packet = RtpPacket(
            channel=channel,
            payload_type=payload[1] & 0x7F,
            marker=(payload[1] >> 7) & 0x01,
            sequence=int.from_bytes(payload[2:4], "big"),
            timestamp=int.from_bytes(payload[4:8], "big"),
            ssrc=int.from_bytes(payload[8:12], "big"),
            size=len(payload),
            nal_type=payload[12] & 0x1F if len(payload) > 12 else -1,
        )
        self.stats.rtp_packets += 1
        self.stats.rtp_rows.append(packet)

    # ---------------- 请求构造 ----------------

    def request(
        self,
        method: str,
        url: str,
        extra_headers: Optional[Dict[str, str]] = None,
        expect_status: Optional[int] = 200,
        authorization: Optional[str] = None,
        cseq: Optional[int] = None,
    ) -> Response:
        self.cseq = self.cseq + 1 if cseq is None else cseq
        lines = [f"{method} {url} RTSP/1.0", f"CSeq: {self.cseq}"]
        if self.session:
            lines.append(f"Session: {self.session}")
        if authorization:
            lines.append(f"Authorization: {authorization}")
        for key, value in (extra_headers or {}).items():
            lines.append(f"{key}: {value}")
        payload = (CRLF.join(lines) + CRLF + CRLF).encode("ascii")
        self.send_raw(payload)

        response = self.read_response()
        if expect_status is not None and response.status != expect_status:
            raise RtspError(f"{method} 期望 {expect_status}，实际 {response.status} {response.reason}")
        return response

    def digest_authorization(self, method: str, url: str, www_authenticate: str, user: str, password: str) -> str:
        """根据 401 挑战构造 Digest 头（支持带/不带 qop）。"""
        params = _parse_auth_params(www_authenticate)
        realm = params.get("realm", "")
        nonce = params.get("nonce", "")
        qop = params.get("qop", "")
        algorithm = params.get("algorithm", "MD5")
        if algorithm.upper() not in ("MD5", ""):
            raise RtspError(f"不支持的摘要算法: {algorithm}")

        ha1 = hashlib.md5(f"{user}:{realm}:{password}".encode()).hexdigest()
        ha2 = hashlib.md5(f"{method}:{url}".encode()).hexdigest()
        if qop:
            cnonce = base64.b16encode(os.urandom(8)).decode().lower()
            nc = "00000001"
            response = hashlib.md5(f"{ha1}:{nonce}:{nc}:{cnonce}:auth:{ha2}".encode()).hexdigest()
            return (
                f'Digest username="{user}", realm="{realm}", nonce="{nonce}", uri="{url}", '
                f'response="{response}", qop=auth, nc={nc}, cnonce="{cnonce}"'
            )
        response = hashlib.md5(f"{ha1}:{nonce}:{ha2}".encode()).hexdigest()
        return f'Digest username="{user}", realm="{realm}", nonce="{nonce}", uri="{url}", response="{response}"'

    def request_with_auth(self, method: str, url: str, extra_headers: Optional[Dict[str, str]] = None) -> Response:
        """带自动 401 重试的请求。"""
        response = self.request(method, url, extra_headers=extra_headers, expect_status=None)
        if response.status == 401 and self.user is not None:
            challenge = response.header("www-authenticate") or ""
            authorization = self.digest_authorization(method, url, challenge, self.user, self.password or "")
            response = self.request(method, url, extra_headers=extra_headers, expect_status=None, authorization=authorization)
        return response


def _parse_auth_params(header: str) -> Dict[str, str]:
    params: Dict[str, str] = {}
    text = header.split(" ", 1)[1] if " " in header else header
    for item in text.split(","):
        key, _, value = item.partition("=")
        params[key.strip().lower()] = value.strip().strip('"')
    return params


# --------------------------------------------------------------------------- #
# 测试用例
# --------------------------------------------------------------------------- #


def case_options(client: RtspClient, url: str) -> None:
    response = client.request("OPTIONS", url, expect_status=200)
    public = response.header("public") or ""
    for method in ("DESCRIBE", "SETUP", "PLAY", "TEARDOWN", "GET_PARAMETER"):
        if method not in public:
            raise RtspError(f"OPTIONS 未声明方法 {method}: {public}")


def case_describe(client: RtspClient, url: str) -> str:
    response = client.request_with_auth("DESCRIBE", url, {"Accept": "application/sdp"})
    if response.status != 200:
        raise RtspError(f"DESCRIBE 失败: {response.status}")
    if response.header("content-type") != "application/sdp":
        raise RtspError("DESCRIBE 响应缺少 application/sdp")
    sdp = response.body.decode("ascii", "replace")
    for required in ("v=0", "m=video", "a=rtpmap:", "a=control:"):
        if required not in sdp:
            raise RtspError(f"SDP 缺少字段 {required}")
    if "sprop-parameter-sets=" not in sdp and "sprop-sps=" not in sdp:
        raise RtspError("SDP 缺少参数集（sprop）")
    return sdp


def case_setup(client: RtspClient, url: str, transport: str, track: str = "trackID=0") -> Response:
    setup_url = f"{url}/{track}"
    if transport == "tcp":
        header = "RTP/AVP/TCP;unicast;interleaved=0-1"
    else:
        rtp_port, rtcp_port = client.open_udp_ports()
        header = f"RTP/AVP;unicast;client_port={rtp_port}-{rtcp_port}"
    response = client.request_with_auth("SETUP", setup_url, {"Transport": header})
    if response.status != 200:
        raise RtspError(f"SETUP 失败: {response.status}")
    if not response.header("session"):
        raise RtspError("SETUP 响应缺少 Session 头")
    if not response.header("transport"):
        raise RtspError("SETUP 响应缺少 Transport 头")
    return response


def case_play_and_collect(client: RtspClient, url: str, seconds: float, transport: str = "tcp") -> None:
    response = client.request_with_auth("PLAY", url, {"Range": "npt=now-"})
    if response.status != 200:
        raise RtspError(f"PLAY 失败: {response.status}")

    deadline = time.time() + seconds
    while time.time() < deadline:
        if transport == "udp":
            client.pump_udp()
        else:
            client.pump(timeout=0.3)

    if client.stats.rtp_packets == 0:
        raise RtspError(f"PLAY 后没有收到任何 RTP 包（{transport}）")


def case_keepalive(client: RtspClient, url: str) -> None:
    response = client.request_with_auth("GET_PARAMETER", url, {"Content-Length": "0"})
    if response.status != 200:
        raise RtspError(f"GET_PARAMETER 失败: {response.status}")


def case_pause(client: RtspClient, url: str) -> None:
    response = client.request_with_auth("PAUSE", url)
    if response.status != 200:
        raise RtspError(f"PAUSE 失败: {response.status}")


def case_teardown(client: RtspClient, url: str) -> None:
    response = client.request_with_auth("TEARDOWN", url)
    if response.status != 200:
        raise RtspError(f"TEARDOWN 失败: {response.status}")


def case_illegal_request_line(host: str, port: int) -> None:
    client = RtspClient(host, port, timeout=5.0)
    try:
        # 完整但非法的请求（把响应当请求发；解析器必须判定为 Failure 而不是 Partial）。
        client.send_raw(b"RTSP/1.0 200 OK\r\nCSeq: 1\r\n\r\n")
        response = client.read_response()
        if response.status != 400:
            raise RtspError(f"非法请求行应返回 400，实际 {response.status}")
    finally:
        client.close()


def case_unsupported_method(host: str, port: int, url: str) -> None:
    client = RtspClient(host, port, timeout=5.0)
    try:
        response = client.request("RECORD", url, expect_status=None)
        if response.status not in (401, 405):
            raise RtspError(f"未支持方法应返回 405（或鉴权 401），实际 {response.status}")
    finally:
        client.close()


def case_oversized_request(host: str, port: int, url: str) -> None:
    client = RtspClient(host, port, timeout=5.0)
    try:
        client.send_raw(b"OPTIONS " + url.encode() + b" RTSP/1.0\r\nCSeq: 1\r\nX-Pad: " + b"A" * 12000 + b"\r\n\r\n")
        try:
            response = client.read_response()
        except RtspError:
            return  # 直接关闭也符合预期
        if response.status != 400:
            raise RtspError(f"超长请求应返回 400，实际 {response.status}")
    finally:
        client.close()


def case_pipelined_requests(host: str, port: int, url: str) -> None:
    client = RtspClient(host, port, timeout=5.0)
    try:
        client.send_raw(
            f"OPTIONS {url} RTSP/1.0{CRLF}CSeq: 1{CRLF}{CRLF}OPTIONS {url} RTSP/1.0{CRLF}CSeq: 2{CRLF}{CRLF}".encode()
        )
        first = client.read_response()
        second = client.read_response()
        if first.status != 200 or second.status != 200:
            raise RtspError(f"流水线请求失败: {first.status}/{second.status}")
    finally:
        client.close()


def case_auth_negative(host: str, port: int, url: str, user: str, password: str) -> None:
    """错误口令必须 401，正确口令必须 200。"""
    client = RtspClient(host, port, timeout=5.0)
    response = client.request("DESCRIBE", url, expect_status=None)
    if response.status != 401:
        raise RtspError(f"无凭据应返回 401，实际 {response.status}")
    challenge = response.header("www-authenticate") or ""
    if "realm=" not in challenge or "nonce=" not in challenge:
        raise RtspError(f"401 缺少 Digest 挑战字段: {challenge}")

    bad = client.digest_authorization("DESCRIBE", url, challenge, user, password + "-wrong")
    response = client.request("DESCRIBE", url, expect_status=None, authorization=bad)
    if response.status != 401:
        raise RtspError(f"错误口令应返回 401，实际 {response.status}")

    good = client.digest_authorization("DESCRIBE", url, challenge, user, password)
    response = client.request("DESCRIBE", url, expect_status=None, authorization=good)
    if response.status != 200:
        raise RtspError(f"正确口令应返回 200，实际 {response.status}")
    client.close()


def write_rtp_csv(path: str, rows: List[RtpPacket]) -> None:
    with open(path, "w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["index", "channel", "payload_type", "marker", "sequence", "timestamp", "ssrc", "size", "nal_type"])
        for index, row in enumerate(rows):
            writer.writerow([index, row.channel, row.payload_type, row.marker, row.sequence, row.timestamp, row.ssrc, row.size, row.nal_type])


def main() -> int:
    parser = argparse.ArgumentParser(description="RTSP 协议探针")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8554)
    parser.add_argument("--path", default="Streaming/Channels/101")
    parser.add_argument("--transport", choices=["tcp", "udp"], default="tcp")
    parser.add_argument("--user")
    parser.add_argument("--password", default="")
    parser.add_argument("--seconds", type=float, default=2.0)
    parser.add_argument("--suite", choices=["basic", "negative", "auth", "all"], default="basic")
    parser.add_argument("--rtp-csv", default="")
    args = parser.parse_args()

    url = f"rtsp://{args.host}:{args.port}/{args.path}"
    failures = 0

    def run(name: str, func) -> None:
        nonlocal failures
        try:
            func()
            print(f"[通过] {name}")
        except Exception as exc:  # noqa: BLE001 - 探针需要打印任意失败原因
            failures += 1
            print(f"[失败] {name}: {exc}")

    if args.suite in ("basic", "all"):
        client = RtspClient(args.host, args.port)
        client.user = args.user
        client.password = args.password
        try:
            run("OPTIONS", lambda: case_options(client, url))
            run("DESCRIBE", lambda: case_describe(client, url))
            run("SETUP", lambda: case_setup(client, url, args.transport))
            run("PLAY+收流", lambda: case_play_and_collect(client, url, args.seconds, args.transport))
            run("GET_PARAMETER(keepalive)", lambda: case_keepalive(client, url))
            run("PAUSE", lambda: case_pause(client, url))
            run("TEARDOWN", lambda: case_teardown(client, url))
            print(
                f"       统计: 响应={client.stats.responses} interleaved={client.stats.interleaved_frames} "
                f"RTP={client.stats.rtp_packets}"
            )
            if args.rtp_csv and client.stats.rtp_rows:
                write_rtp_csv(args.rtp_csv, client.stats.rtp_rows)
                print(f"       RTP 明细已写入 {args.rtp_csv}")
        finally:
            client.close()

    if args.suite in ("negative", "all"):
        run("负向: 非法请求行", lambda: case_illegal_request_line(args.host, args.port))
        run("负向: 未支持方法", lambda: case_unsupported_method(args.host, args.port, url))
        run("负向: 超长请求", lambda: case_oversized_request(args.host, args.port, url))
        run("负向: 流水线请求", lambda: case_pipelined_requests(args.host, args.port, url))

    if args.suite in ("auth", "all"):
        if not args.user:
            print("[跳过] 鉴权用例（未提供 --user）")
        else:
            run("鉴权用例集合", lambda: case_auth_negative(args.host, args.port, url, args.user, args.password))

    print(f"结果: {'全部通过' if failures == 0 else f'{failures} 项失败'}")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
