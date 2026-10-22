/**
 * @FilePath     : smolrtsp_shim.c
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 11:35:00
 * @Description  : smolrtsp C99 隔离层实现
 */

/*
 * 本文件是全项目唯一包含 smolrtsp 头文件的编译单元：C++ 侧只能经
 * smolrtsp_shim.h 暴露的普通 C 接口使用协议核心，从而保证 99 宏体系不扩散
 * 到业务代码、vendor 源码零修改、未来替换/升级协议核心时改动面被限制在本
 * 文件。
 *
 * 资源约定：不做任何 per-packet 堆分配（上游 TCP interleaved 发送内部的
 * alloca+memcpy 除外），所有对象在创建时一次性分配。
 */

#include "smolrtsp_shim.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <smolrtsp.h>
/* smolrtsp.h 未聚合 RTCP 类型，这里显式包含。 */
#include <smolrtsp/types/rtcp.h>
#ifdef SMOLRTSP_WITH_JPEG
/* MJPEG（RFC 2435）打包传输；IPC_RTSP_WITH_MJPEG 关闭时无实现。 */
#include <smolrtsp/jpeg_transport.h>
#endif

/* ------------------------------------------------------------------------- */
/* Writer：把 smolrtsp 的输出转交给调用方提供的回调                          */
/* ------------------------------------------------------------------------- */

/** Writer 实现体的最大格式化长度；RTSP 响应行/头都很短，超过即视为异常。 */
#define IPC_RTSP_SHIM_WRITEF_MAX 512

typedef struct ipc_rtsp_shim_writer ShimWriter;

/** Writer 句柄实体：把 smolrtsp 的写操作转成 C 回调。 */
struct ipc_rtsp_shim_writer
{
    ipc_rtsp_shim_write_fn write_fn;
    ipc_rtsp_shim_filled_fn filled_fn;
    void *user;
};

/** SmolRTSP_Writer::write 实现：转发到调用方 write_fn（未设置返回 -1）。 */
static ssize_t ShimWriter_write(VSelf, CharSlice99 data)
{
    VSELF(ShimWriter);
    assert(self);

    if (self->write_fn == NULL)
    {
        return -1;
    }
    return self->write_fn(self->user, data.ptr, data.len);
}

/** SmolRTSP_Writer::lock 实现：单 I/O 线程模型下为 no-op（保留接口语义）。 */
static void ShimWriter_lock(VSelf)
{
    VSELF(ShimWriter);
    (void) self;
    /* 单 I/O 线程模型下 writer 不需要额外加锁；保留接口语义为 no-op。 */
}

/** SmolRTSP_Writer::unlock 实现：与 lock 对应的 no-op。 */
static void ShimWriter_unlock(VSelf)
{
    VSELF(ShimWriter);
    (void) self;
}

/** SmolRTSP_Writer::filled 实现：转发到调用方 filled_fn（未设置返回 0）。 */
static size_t ShimWriter_filled(VSelf)
{
    VSELF(ShimWriter);
    assert(self);

    if (self->filled_fn == NULL)
    {
        return 0;
    }
    return self->filled_fn(self->user);
}

/**
 * SmolRTSP_Writer::vwritef 实现：格式化进栈缓冲后经 ShimWriter_write 写出；
 * 超过 IPC_RTSP_SHIM_WRITEF_MAX 视为异常返回 -1，不截断。
 */
static int ShimWriter_vwritef(VSelf, const char *restrict fmt, va_list ap)
{
    VSELF(ShimWriter);
    assert(self);
    assert(fmt);

    char buf[IPC_RTSP_SHIM_WRITEF_MAX];
    const int n = vsnprintf(buf, sizeof buf, fmt, ap);
    if (n < 0)
    {
        return -1;
    }
    if ((size_t) n >= sizeof buf)
    {
        /* 超过固定缓冲长度说明调用方参数非法或响应异常，直接报错而不是截断。 */
        return -1;
    }
    return (int) ShimWriter_write(self, CharSlice99_new(buf, (size_t) n));
}

/** SmolRTSP_Writer::writef 实现：va_start 包装 vwritef。 */
static int ShimWriter_writef(VSelf, const char *restrict fmt, ...)
{
    VSELF(ShimWriter);
    assert(self);
    assert(fmt);

    va_list ap;
    va_start(ap, fmt);
    const int ret = ShimWriter_vwritef(self, fmt, ap);
    va_end(ap);

    return ret;
}

impl(SmolRTSP_Writer, ShimWriter);

/* ------------------------------------------------------------------------- */
/* 请求解析                                                                  */
/* ------------------------------------------------------------------------- */

/** 对外请求句柄：包装 SmolRTSP_Request，切片数据指向解析缓冲。 */
struct ipc_rtsp_shim_request
{
    SmolRTSP_Request req;
};

/** 解析器实体：持有"当前请求"存储，parser_run 返回的指针即指向它。 */
struct ipc_rtsp_shim_parser
{
    struct ipc_rtsp_shim_request current;
};

ipc_rtsp_shim_parser *ipc_rtsp_shim_parser_new(void)
{
    ipc_rtsp_shim_parser *self = malloc(sizeof *self);
    if (self == NULL)
    {
        return NULL;
    }
    self->current.req = SmolRTSP_Request_uninit();
    return self;
}

void ipc_rtsp_shim_parser_free(ipc_rtsp_shim_parser *self)
{
    free(self);
}

int ipc_rtsp_shim_parser_run(ipc_rtsp_shim_parser *self, const void *buf, size_t len, size_t *consumed, ipc_rtsp_shim_request **req)
{
    assert(self);
    assert(consumed);
    assert(req);

    self->current.req = SmolRTSP_Request_uninit();

    /* smolrtsp 不修改输入，这里的去 const 只为满足 CharSlice99 的签名。 */
    const SmolRTSP_ParseResult res = SmolRTSP_Request_parse(&self->current.req, CharSlice99_new((char *) buf, len));

    if (SmolRTSP_ParseResult_is_failure(res))
    {
        return IPC_RTSP_SHIM_PARSE_FAILURE;
    }
    if (SmolRTSP_ParseResult_is_partial(res))
    {
        return IPC_RTSP_SHIM_PARSE_PARTIAL;
    }

    size_t offset = 0;

    /* datatype99 的 match/of 是语句宏，clang-format 会破坏其结构，必须关闭格式化。
     * cppcheck 同样无法展开这些宏（见 tools/cppcheck 行内抑制）。 */
    /* clang-format off */
    match(res) {
        of(SmolRTSP_ParseResult_Success, status) match(*status) {
            of(SmolRTSP_ParseStatus_Complete, off) {
                offset = *off;
            }
            otherwise {
                return IPC_RTSP_SHIM_PARSE_PARTIAL;
            }
        }
        otherwise {
            return IPC_RTSP_SHIM_PARSE_FAILURE;
        }
    }
/* clang-format on */

*consumed = offset;
*req = &self->current;
return IPC_RTSP_SHIM_PARSE_COMPLETE;
}

const char *ipc_rtsp_shim_request_method(const ipc_rtsp_shim_request *self, size_t *len)
{
    assert(self);
    if (len != NULL)
    {
        *len = self->req.start_line.method.len;
    }
    return self->req.start_line.method.ptr;
}

const char *ipc_rtsp_shim_request_uri(const ipc_rtsp_shim_request *self, size_t *len)
{
    assert(self);
    if (len != NULL)
    {
        *len = self->req.start_line.uri.len;
    }
    return self->req.start_line.uri.ptr;
}

uint32_t ipc_rtsp_shim_request_cseq(const ipc_rtsp_shim_request *self)
{
    assert(self);
    return self->req.cseq;
}

const char *ipc_rtsp_shim_request_header(const ipc_rtsp_shim_request *self, const char *name, size_t *len)
{
    assert(self);
    assert(name);

    CharSlice99 value = CharSlice99_empty();
    if (!SmolRTSP_HeaderMap_find(&self->req.header_map, CharSlice99_from_str((char *) name), &value))
    {
        return NULL;
    }
    if (len != NULL)
    {
        *len = value.len;
    }
    return value.ptr;
}

const char *ipc_rtsp_shim_request_body(const ipc_rtsp_shim_request *self, size_t *len)
{
    assert(self);
    if (len != NULL)
    {
        *len = self->req.body.len;
    }
    return self->req.body.ptr;
}

/* ------------------------------------------------------------------------- */
/* Writer 句柄与响应                                                         */
/* ------------------------------------------------------------------------- */

ipc_rtsp_shim_writer *ipc_rtsp_shim_writer_new(ipc_rtsp_shim_write_fn write_fn, ipc_rtsp_shim_filled_fn filled_fn, void *user)
{
    if (write_fn == NULL)
    {
        return NULL;
    }

    ShimWriter *self = malloc(sizeof *self);
    if (self == NULL)
    {
        return NULL;
    }
    self->write_fn = write_fn;
    self->filled_fn = filled_fn;
    self->user = user;
    return self;
}

void ipc_rtsp_shim_writer_free(ipc_rtsp_shim_writer *self)
{
    free(self);
}

struct ipc_rtsp_shim_response
{
    ShimWriter *writer;    /* 非拥有：由连接持有 */
    SmolRTSP_Context *ctx; /* 拥有：free 时先 drop 上下文再释放本包装 */
};

ipc_rtsp_shim_response *ipc_rtsp_shim_response_new_with_cseq(uint32_t cseq, ipc_rtsp_shim_writer *writer)
{
    assert(writer);

    ipc_rtsp_shim_response *self = malloc(sizeof *self);
    if (self == NULL)
    {
        return NULL;
    }

    self->writer = writer;

    const SmolRTSP_Writer w = DYN(ShimWriter, SmolRTSP_Writer, writer);
    self->ctx = SmolRTSP_Context_new(w, cseq);
    if (self->ctx == NULL)
    {
        free(self);
        return NULL;
    }
    return self;
}

ipc_rtsp_shim_response *ipc_rtsp_shim_response_new(const ipc_rtsp_shim_request *req, ipc_rtsp_shim_writer *writer)
{
    assert(req);

    return ipc_rtsp_shim_response_new_with_cseq(req->req.cseq, writer);
}

void ipc_rtsp_shim_response_free(ipc_rtsp_shim_response *self)
{
    if (self == NULL)
    {
        return;
    }
    if (self->ctx != NULL)
    {
        VTABLE(SmolRTSP_Context, SmolRTSP_Droppable).drop(self->ctx);
        self->ctx = NULL;
    }
    free(self);
}

void ipc_rtsp_shim_response_header_n(ipc_rtsp_shim_response *self, const char *name, size_t name_len, const char *value, size_t value_len)
{
    assert(self);
    assert(name);

    smolrtsp_header(self->ctx, CharSlice99_new((char *) name, name_len), "%.*s", (int) value_len, value);
}

void ipc_rtsp_shim_response_header(ipc_rtsp_shim_response *self, const char *name, const char *value)
{
    assert(name);
    assert(value);

    ipc_rtsp_shim_response_header_n(self, name, strlen(name), value, strlen(value));
}

void ipc_rtsp_shim_response_body(ipc_rtsp_shim_response *self, const void *data, size_t len)
{
    assert(self);

    if (data == NULL || len == 0)
    {
        smolrtsp_body(self->ctx, CharSlice99_empty());
        return;
    }
    smolrtsp_body(self->ctx, CharSlice99_new((char *) data, len));
}

ssize_t ipc_rtsp_shim_response_send(ipc_rtsp_shim_response *self, uint16_t code, const char *reason)
{
    assert(self);
    assert(reason);

    return smolrtsp_respond(self->ctx, code, reason);
}

int ipc_rtsp_shim_write_interleaved(ipc_rtsp_shim_writer *self, uint8_t channel_id, const void *payload, size_t len)
{
    assert(self);

    if (payload == NULL || len == 0 || len > IPC_RTSP_SHIM_INTERLEAVED_PAYLOAD_MAX)
    {
        return -1;
    }

    /*
     * 与上游 TCP transport 保持一致：长度字段以网络字节序写入，并把 4 字节头
     * 与 RTCP payload 合并为一次 Writer::write。这样媒体预留区不足时整包失败，
     * 不会在连接输出缓冲中残留半个 interleaved frame。
     */
    const uint32_t header = smolrtsp_interleaved_header(channel_id, htons((uint16_t) len));
    uint8_t frame[sizeof header + IPC_RTSP_SHIM_INTERLEAVED_PAYLOAD_MAX];
    const size_t frame_len = sizeof header + len;
    memcpy(frame, &header, sizeof header);
    memcpy(frame + sizeof header, payload, len);

    SmolRTSP_Writer w = DYN(ShimWriter, SmolRTSP_Writer, self);
    VCALL(w, lock);
    const ssize_t written = VCALL(w, write, CharSlice99_new((char *) frame, frame_len));
    VCALL(w, unlock);

    return written == (ssize_t) frame_len ? 0 : -1;
}

/* ------------------------------------------------------------------------- */
/* RTP / NAL                                                                 */
/* ------------------------------------------------------------------------- */

struct ipc_rtsp_shim_rtp
{
    SmolRTSP_RtpTransport *rtp; /**< 拥有：free 时 drop（UDP 模式级联 close fd） */
    int udp_fd;                 /**< TCP 模式为 -1；仅记录接管的 fd 便于诊断，当前无读取点 */
};

/** NAL 头的编码格式，决定打包时读取几字节头。 */
typedef enum
{
    SHIM_CODEC_H264 = 0, /**< H.264：NAL 头 1 字节 */
    SHIM_CODEC_H265 = 1, /**< H.265：NAL 头 2 字节 */
} shim_codec;

struct ipc_rtsp_shim_nal
{
    SmolRTSP_NalTransport *nal; /**< 拥有：free 时 drop（级联释放 RTP/下层 transport） */
    ipc_rtsp_shim_rtp *rtp;     /**< 仅用于生命周期：nal 释放时一并释放本包装 */
    shim_codec codec;           /**< NAL 头编码格式（决定发送时读取的字节数） */
};

/**
 * 包装 smolrtsp RTP transport 为对外句柄，并接管其所有权。
 *
 * @param rtp 已创建的 transport（本函数失败时在此 drop，调用方无须清理）
 * @param udp_fd UDP 模式下记录接管的 fd；TCP 模式传 -1
 * @return 对外句柄；rtp 为空或分配失败返回 NULL（rtp 已被释放）
 */
static ipc_rtsp_shim_rtp *shim_rtp_wrap(SmolRTSP_RtpTransport *rtp, int udp_fd)
{
    if (rtp == NULL)
    {
        return NULL;
    }
    ipc_rtsp_shim_rtp *self = malloc(sizeof *self);
    if (self == NULL)
    {
        VTABLE(SmolRTSP_RtpTransport, SmolRTSP_Droppable).drop(rtp);
        return NULL;
    }
    self->rtp = rtp;
    self->udp_fd = udp_fd;
    return self;
}

ipc_rtsp_shim_rtp *ipc_rtsp_shim_rtp_new_tcp(ipc_rtsp_shim_writer *writer,
                                             uint8_t channel_id,
                                             size_t max_buffer,
                                             uint32_t ssrc,
                                             uint8_t payload_ty,
                                             uint32_t clock_rate,
                                             uint64_t ts_base_us)
{
    assert(writer);

    const SmolRTSP_Writer w = DYN(ShimWriter, SmolRTSP_Writer, writer);
    const SmolRTSP_Transport t = smolrtsp_transport_tcp(w, channel_id, max_buffer);
    return shim_rtp_wrap(SmolRTSP_RtpTransport_new_with_ssrc_ts_base(t, payload_ty, clock_rate, ssrc, ts_base_us), -1);
}

ipc_rtsp_shim_rtp *ipc_rtsp_shim_rtp_new_udp(int fd, uint32_t ssrc, uint8_t payload_ty, uint32_t clock_rate, uint64_t ts_base_us)
{
    assert(fd >= 0);

    const SmolRTSP_Transport t = smolrtsp_transport_udp(fd);
    SmolRTSP_RtpTransport *rtp = SmolRTSP_RtpTransport_new_with_ssrc_ts_base(t, payload_ty, clock_rate, ssrc, ts_base_us);
    if (rtp == NULL)
    {
        /* transport 未创建成功时没有任何对象接管 fd（transport.h 契约是
         * "drop 时才 close"）；本接口无论成败均接管 fd，失败必须自行关闭，
         * 否则调用方按"返回 NULL 即无需释放"的约定使用时会泄漏 fd。 */
        close(fd);
        return NULL;
    }
    return shim_rtp_wrap(rtp, fd);
}

void ipc_rtsp_shim_rtp_free(ipc_rtsp_shim_rtp *self)
{
    if (self == NULL)
    {
        return;
    }
    if (self->rtp != NULL)
    {
        VTABLE(SmolRTSP_RtpTransport, SmolRTSP_Droppable).drop(self->rtp);
        self->rtp = NULL;
    }
    free(self);
}

bool ipc_rtsp_shim_rtp_is_full(ipc_rtsp_shim_rtp *self)
{
    assert(self);
    return SmolRTSP_RtpTransport_is_full(self->rtp);
}

uint32_t ipc_rtsp_shim_rtp_ssrc(ipc_rtsp_shim_rtp *self)
{
    assert(self);
    return SmolRTSP_RtpTransport_ssrc(self->rtp);
}

uint16_t ipc_rtsp_shim_rtp_next_seq(ipc_rtsp_shim_rtp *self)
{
    assert(self);
    /*
     * smolrtsp 基线（8ae3b511）下 seq_num 构造时为 0，且与 pkt_count 在每次
     * 成功发送时同步自增，因此"下一个包的序号"恒等于 pkt_count——用官方
     * getter 推导，不依赖结构布局。升级第三方库时需复核该不变式。
     * live555/VLC 的 NPT 计算要求 PLAY 响应 RTP-Info 同时含 seq 与 rtptime，
     * 缺 seq 会导致播放时间恒为 0（见 26 号文档）。
     */
    return (uint16_t) SmolRTSP_RtpTransport_pkt_count(self->rtp);
}

uint32_t ipc_rtsp_shim_rtp_pkt_count(ipc_rtsp_shim_rtp *self)
{
    assert(self);
    return SmolRTSP_RtpTransport_pkt_count(self->rtp);
}

uint32_t ipc_rtsp_shim_rtp_octet_count(ipc_rtsp_shim_rtp *self)
{
    assert(self);
    return SmolRTSP_RtpTransport_octet_count(self->rtp);
}

uint32_t ipc_rtsp_shim_rtp_last_ts(ipc_rtsp_shim_rtp *self)
{
    assert(self);
    return SmolRTSP_RtpTransport_last_rtp_ts(self->rtp);
}

int ipc_rtsp_shim_rtp_send(ipc_rtsp_shim_rtp *self, uint32_t raw_ts, bool marker, const void *payload, size_t len)
{
    assert(self);

    return SmolRTSP_RtpTransport_send_packet(self->rtp,
                                             SmolRTSP_RtpTimestamp_Raw(raw_ts),
                                             marker,
                                             U8Slice99_empty(),
                                             U8Slice99_new((uint8_t *) payload, len));
}

/**
 * 包装 smolrtsp NAL transport 为对外句柄，并接管 nal 与 rtp 的生命周期。
 *
 * @param nal 已创建的 NAL transport（本函数失败时在此 drop）
 * @param rtp 关联的 RTP 句柄（失败时在此释放；成功后由 nal_free 级联释放）
 * @param codec NAL 头编码格式（决定发送时读取的字节数）
 * @return 对外句柄；nal 为空或分配失败返回 NULL（nal/rtp 均已释放）
 */
static ipc_rtsp_shim_nal *shim_nal_wrap(SmolRTSP_NalTransport *nal, ipc_rtsp_shim_rtp *rtp, shim_codec codec)
{
    if (nal == NULL)
    {
        ipc_rtsp_shim_rtp_free(rtp);
        return NULL;
    }
    ipc_rtsp_shim_nal *self = malloc(sizeof *self);
    if (self == NULL)
    {
        VTABLE(SmolRTSP_NalTransport, SmolRTSP_Droppable).drop(nal);
        free(rtp);
        return NULL;
    }
    self->nal = nal;
    self->rtp = rtp;
    self->codec = codec;
    return self;
}

/* 创建 H.264 NAL 发送器：SMOLRTSP_WITH_H264 未定义时 max_nalu_size 被静默
 * 忽略，沿用默认 1200 字节分片预算。 */
ipc_rtsp_shim_nal *ipc_rtsp_shim_nal_new_h264(ipc_rtsp_shim_rtp *rtp, size_t max_nalu_size)
{
    assert(rtp);

    SmolRTSP_NalTransportConfig config = SmolRTSP_NalTransportConfig_default();
#ifdef SMOLRTSP_WITH_H264
    config.max_h264_nalu_size = max_nalu_size;
#endif
    return shim_nal_wrap(SmolRTSP_NalTransport_new_with_config(rtp->rtp, config), rtp, SHIM_CODEC_H264);
}

/* 创建 H.265 NAL 发送器：SMOLRTSP_WITH_H265 未定义时 max_nalu_size 被静默
 * 忽略，沿用默认 1200 字节分片预算。 */
ipc_rtsp_shim_nal *ipc_rtsp_shim_nal_new_h265(ipc_rtsp_shim_rtp *rtp, size_t max_nalu_size)
{
    assert(rtp);

    SmolRTSP_NalTransportConfig config = SmolRTSP_NalTransportConfig_default();
#ifdef SMOLRTSP_WITH_H265
    config.max_h265_nalu_size = max_nalu_size;
#endif
    return shim_nal_wrap(SmolRTSP_NalTransport_new_with_config(rtp->rtp, config), rtp, SHIM_CODEC_H265);
}

void ipc_rtsp_shim_nal_free(ipc_rtsp_shim_nal *self)
{
    if (self == NULL)
    {
        return;
    }
    if (self->nal != NULL)
    {
        /* NAL transport 的 drop 会级联释放 RTP transport 与下层 transport。 */
        VTABLE(SmolRTSP_NalTransport, SmolRTSP_Droppable).drop(self->nal);
        self->nal = NULL;
    }
    free(self->rtp);
    self->rtp = NULL;
    free(self);
}

int ipc_rtsp_shim_nal_send(ipc_rtsp_shim_nal *self, uint32_t raw_ts, bool au_end, const void *nal, size_t len)
{
    assert(self);

    if (nal == NULL || len == 0)
    {
        return -1;
    }

    const uint8_t *bytes = nal;

    if (self->nal == NULL)
    {
        return -1;
    }

    /* 调用方传入完整 NAL（含 NAL 头）；头长度由构造时的 codec 决定：
     * H.264 为 1 字节，H.265 为 2 字节。 */
    SmolRTSP_NalUnit unit;
    memset(&unit, 0, sizeof unit);

    if (self->codec == SHIM_CODEC_H264)
    {
        if (len <= SMOLRTSP_H264_NAL_HEADER_SIZE)
        {
            return -1;
        }
        const SmolRTSP_H264NalHeader h = SmolRTSP_H264NalHeader_parse(bytes[0]);
        unit.header = SmolRTSP_NalHeader_H264(h);
        unit.payload = U8Slice99_new((uint8_t *) (bytes + SMOLRTSP_H264_NAL_HEADER_SIZE), len - SMOLRTSP_H264_NAL_HEADER_SIZE);
    }
    else
    {
        if (len <= SMOLRTSP_H265_NAL_HEADER_SIZE)
        {
            return -1;
        }
        const SmolRTSP_H265NalHeader h = SmolRTSP_H265NalHeader_parse((uint8_t *) bytes);
        unit.header = SmolRTSP_NalHeader_H265(h);
        unit.payload = U8Slice99_new((uint8_t *) (bytes + SMOLRTSP_H265_NAL_HEADER_SIZE), len - SMOLRTSP_H265_NAL_HEADER_SIZE);
    }

    return SmolRTSP_NalTransport_send_packet(self->nal, SmolRTSP_RtpTimestamp_Raw(raw_ts), au_end, unit);
}

/* ------------------------------------------------------------------------- */
/* JPEG（RFC 2435）                                                          */
/* ------------------------------------------------------------------------- */

#ifdef SMOLRTSP_WITH_JPEG

struct ipc_rtsp_shim_jpeg
{
    SmolRTSP_JpegTransport *jpeg; /**< 拥有：free 时 drop（级联释放 RTP/下层 transport） */
    ipc_rtsp_shim_rtp *rtp;       /**< 仅用于生命周期：jpeg 释放时一并释放 */
};

ipc_rtsp_shim_jpeg *ipc_rtsp_shim_jpeg_new(ipc_rtsp_shim_rtp *rtp, size_t max_packet_size)
{
    assert(rtp);

    const SmolRTSP_JpegTransportConfig config = {
        .max_packet_size = max_packet_size,
    };
    SmolRTSP_JpegTransport *jpeg = SmolRTSP_JpegTransport_new_with_config(rtp->rtp, config);
    if (jpeg == NULL)
    {
        ipc_rtsp_shim_rtp_free(rtp);
        return NULL;
    }
    ipc_rtsp_shim_jpeg *self = malloc(sizeof *self);
    if (self == NULL)
    {
        /* Jpeg transport 的 drop 会级联释放 RTP transport 与下层 transport。 */
        VTABLE(SmolRTSP_JpegTransport, SmolRTSP_Droppable).drop(jpeg);
        free(rtp);
        return NULL;
    }
    self->jpeg = jpeg;
    self->rtp = rtp;
    return self;
}

void ipc_rtsp_shim_jpeg_free(ipc_rtsp_shim_jpeg *self)
{
    if (self == NULL)
    {
        return;
    }
    if (self->jpeg != NULL)
    {
        /* 级联释放 RTP transport 与下层 transport，这里只回收包装结构。 */
        VTABLE(SmolRTSP_JpegTransport, SmolRTSP_Droppable).drop(self->jpeg);
        self->jpeg = NULL;
    }
    free(self->rtp);
    self->rtp = NULL;
    free(self);
}

bool ipc_rtsp_shim_jpeg_is_full(ipc_rtsp_shim_jpeg *self)
{
    assert(self);
    return SmolRTSP_JpegTransport_is_full(self->jpeg);
}

int ipc_rtsp_shim_jpeg_send_frame(ipc_rtsp_shim_jpeg *self, uint32_t raw_ts, const ipc_rtsp_shim_jpeg_frame *frame)
{
    assert(self);
    assert(frame);

    if (frame->scan == NULL && frame->scan_len > 0)
    {
        return -1;
    }

    const SmolRTSP_JpegFrame jf = {
        .hdr = {
            .type_specific = 0,
            .fragment_offset = 0, /* 由 transport 逐包覆写。 */
            .type = frame->type,
            .q = frame->q,
            .width_blocks = frame->width_blocks,
            .height_blocks = frame->height_blocks,
        },
        .qt0 = (frame->qt0 != NULL) ? U8Slice99_new((uint8_t *) frame->qt0, frame->qt0_len) : U8Slice99_empty(),
        .qt1 = (frame->qt1 != NULL) ? U8Slice99_new((uint8_t *) frame->qt1, frame->qt1_len) : U8Slice99_empty(),
        .scan_data = U8Slice99_new((uint8_t *) frame->scan, frame->scan_len),
        .restart_interval = frame->restart_interval,
    };

    return SmolRTSP_JpegTransport_send_frame(self->jpeg, SmolRTSP_RtpTimestamp_Raw(raw_ts), jf);
}

#endif /* SMOLRTSP_WITH_JPEG */

/* ------------------------------------------------------------------------- */
/* 时钟与 RTCP                                                               */
/* ------------------------------------------------------------------------- */

uint32_t ipc_rtsp_shim_rtp_ts_from_us(uint64_t time_us, uint32_t clock_rate, uint64_t ts_base_us)
{
    return smolrtsp_rtp_ts_from_sys_clock_us(time_us, clock_rate, ts_base_us);
}

size_t ipc_rtsp_shim_rtcp_sr(void *buf,
                             size_t cap,
                             uint32_t ssrc,
                             uint32_t ntp_sec,
                             uint32_t ntp_frac,
                             uint32_t rtp_ts,
                             uint32_t pkt_count,
                             uint32_t octet_count)
{
    assert(buf);

    SmolRTSP_RtcpSr sr = {
        .padding = false,
        .rc = 0,
        .ssrc = htonl(ssrc), /* 序列化层裸字节追加：与 RTP 头的字节序对齐 */
        .ntp_sec = htonl(ntp_sec),
        .ntp_frac = htonl(ntp_frac),
        .rtp_ts = htonl(rtp_ts),
        .pkt_count = htonl(pkt_count),
        .octet_count = htonl(octet_count),
    };

    const size_t size = SmolRTSP_RtcpSr_size(sr);
    if (cap < size)
    {
        return 0;
    }
    uint8_t *const serialized_sr = SmolRTSP_RtcpSr_serialize(sr, buf);
    (void) serialized_sr;
    return size;
}

size_t ipc_rtsp_shim_rtcp_sdes_cname(void *buf, size_t cap, uint32_t ssrc, const char *cname)
{
    assert(buf);
    assert(cname);

    SmolRTSP_RtcpSdesCname sdes = {
        .padding = false,
        .ssrc = htonl(ssrc), /* 序列化层裸字节追加：与 RTP 头的字节序对齐 */
        .cname = cname,
    };

    const size_t size = SmolRTSP_RtcpSdesCname_size(sdes);
    if (cap < size)
    {
        return 0;
    }
    uint8_t *const serialized_sdes = SmolRTSP_RtcpSdesCname_serialize(sdes, buf);
    (void) serialized_sdes;
    return size;
}

size_t ipc_rtsp_shim_rtcp_bye(void *buf, size_t cap, uint32_t ssrc, const char *reason)
{
    assert(buf);

    SmolRTSP_RtcpBye bye = {
        .padding = false,
        .ssrc = htonl(ssrc), /* 序列化层裸字节追加：与 RTP 头的字节序对齐 */
        .reason = reason,
    };

    const size_t size = SmolRTSP_RtcpBye_size(bye);
    if (cap < size)
    {
        return 0;
    }
    uint8_t *const serialized_bye = SmolRTSP_RtcpBye_serialize(bye, buf);
    (void) serialized_bye;
    return size;
}

/* ------------------------------------------------------------------------- */
/* 协议工具                                                                  */
/* ------------------------------------------------------------------------- */

int ipc_rtsp_shim_parse_transport(const char *value, size_t len, ipc_rtsp_shim_transport *out)
{
    assert(out);

    SmolRTSP_TransportConfig config;
    memset(&config, 0, sizeof config);
    if (smolrtsp_parse_transport(&config, CharSlice99_new((char *) value, len)) != 0)
    {
        return -1;
    }

    memset(out, 0, sizeof *out);
    out->lower_transport = config.lower == SmolRTSP_LowerTransport_TCP ? IPC_RTSP_SHIM_LOWER_TCP : IPC_RTSP_SHIM_LOWER_UDP;

    ifLet(config.interleaved, SmolRTSP_ChannelPair_Some, pair)
    {
        out->has_interleaved = 1;
        out->rtp_channel = pair->rtp_channel;
        out->rtcp_channel = pair->rtcp_channel;
    }

    ifLet(config.client_port, SmolRTSP_PortPair_Some, pair)
    {
        out->has_client_port = 1;
        out->client_rtp_port = pair->rtp_port;
        out->client_rtcp_port = pair->rtcp_port;
    }

    return 0;
}

int ipc_rtsp_shim_parse_interleaved(const void *buf,
                                    size_t len,
                                    uint8_t *channel_id,
                                    const void **payload,
                                    size_t *payload_len,
                                    size_t *consumed)
{
    assert(channel_id);
    assert(payload);
    assert(payload_len);
    assert(consumed);

    uint8_t channel = 0;
    U8Slice99 body = U8Slice99_empty();
    size_t used = 0;

    const SmolRTSP_InterleavedFrameStatus status = smolrtsp_parse_interleaved_frame(CharSlice99_new((char *) buf, len),
                                                                                    &channel,
                                                                                    &body,
                                                                                    &used);

    switch (status)
    {
    case SmolRTSP_InterleavedFrameStatus_Complete:
        *channel_id = channel;
        *payload = body.ptr;
        *payload_len = body.len;
        *consumed = used;
        return IPC_RTSP_SHIM_IL_COMPLETE;
    case SmolRTSP_InterleavedFrameStatus_Partial:
        return IPC_RTSP_SHIM_IL_PARTIAL;
    default:
        return IPC_RTSP_SHIM_IL_NOT_INTERLEAVED;
    }
}
