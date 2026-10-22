/**
 * @FilePath     : smolrtsp_shim.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 11:35:00
 * @Description  : smolrtsp 隔离层（C99 ABI）
 */

/*
 * 本头文件是 C++ 业务代码访问 smolrtsp 的唯一入口。设计原因：smolrtsp 头
 * 使用 `restrict` 等 C99 关键字与 interface99/datatype99 宏，不能在 C++ 中
 * 直接包含；业务层不得扩散 99 宏体系，vendor 源码保持零修改；所有类型都是
 * 普通 C 类型，可在 C/C++ 之间安全传递（不传 STL、不抛异常）。
 *
 * 生命周期约定：parser 归调用方所有；parser_run 返回的 request 指针只在
 * 下一次 parser_run 之前有效，必须当场取值；response 依赖调用方提供的
 * writer 回调，回调对象必须比它活得久；rtp 在 UDP 模式下接管 fd（drop 时
 * close），TCP 模式不持 fd。
 */
#ifndef IPC_RTSP_SMOLRTSP_SHIM_H
#define IPC_RTSP_SMOLRTSP_SHIM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* 请求解析结果。 */
#define IPC_RTSP_SHIM_PARSE_COMPLETE 0 /**< 解析出一个完整请求 */
#define IPC_RTSP_SHIM_PARSE_PARTIAL  1 /**< 数据不足，需要更多字节 */
#define IPC_RTSP_SHIM_PARSE_FAILURE  2 /**< 请求非法 */

/* interleaved 帧解析结果。 */
#define IPC_RTSP_SHIM_IL_COMPLETE        0 /**< 解析出完整 interleaved 帧 */
#define IPC_RTSP_SHIM_IL_PARTIAL         1 /**< 数据不足，需要更多字节 */
#define IPC_RTSP_SHIM_IL_NOT_INTERLEAVED 2 /**< 非 interleaved 数据（首字节不是 '$'） */

/* 下层传输类型。 */
#define IPC_RTSP_SHIM_LOWER_UDP 0 /**< 下层传输为 UDP（RTP/RTCP 独立 socket） */
#define IPC_RTSP_SHIM_LOWER_TCP 1 /**< 下层传输为 TCP（interleaved 复用 RTSP 连接） */

    typedef struct ipc_rtsp_shim_parser ipc_rtsp_shim_parser;
    typedef struct ipc_rtsp_shim_request ipc_rtsp_shim_request;
    typedef struct ipc_rtsp_shim_writer ipc_rtsp_shim_writer;
    typedef struct ipc_rtsp_shim_response ipc_rtsp_shim_response;
    typedef struct ipc_rtsp_shim_rtp ipc_rtsp_shim_rtp;
    typedef struct ipc_rtsp_shim_nal ipc_rtsp_shim_nal;
    typedef struct ipc_rtsp_shim_jpeg ipc_rtsp_shim_jpeg;

    /** writer 写回调：返回实际写入字节数，失败返回负值。 */
    typedef ssize_t (*ipc_rtsp_shim_write_fn)(void *user, const void *data, size_t len);

    /** writer 水位回调：返回当前输出缓冲字节数（用于背压判断）。 */
    typedef size_t (*ipc_rtsp_shim_filled_fn)(void *user);

    /** Transport 头解析结果（带 has_* 标志：标志位为 0 时对应字段无效）。 */
    typedef struct
    {
        int lower_transport;       /**< IPC_RTSP_SHIM_LOWER_UDP / TCP（解析成功即有效） */
        int has_interleaved;       /**< 1 表示下列 interleaved 字段有效，0 时无效 */
        uint8_t rtp_channel;       /**< RTP interleaved 通道号（has_interleaved=1 时有效） */
        uint8_t rtcp_channel;      /**< RTCP interleaved 通道号（has_interleaved=1 时有效） */
        int has_client_port;       /**< 1 表示下列端口字段有效，0 时无效 */
        uint16_t client_rtp_port;  /**< 客户端 RTP 端口（has_client_port=1 时有效） */
        uint16_t client_rtcp_port; /**< 客户端 RTCP 端口（has_client_port=1 时有效） */
    } ipc_rtsp_shim_transport;

    /* ------------------------------------------------------------------ */
    /* 请求解析                                                           */
    /* ------------------------------------------------------------------ */

    /**
     * 创建 RTSP 请求解析器（内部持有"当前请求"存储）。
     *
     * @return 解析器句柄，归调用方所有；分配失败返回 NULL
     */
    ipc_rtsp_shim_parser *ipc_rtsp_shim_parser_new(void);
    /**
     * 释放解析器（仅回收 parser 本体，不涉及 fd/回调资源）；NULL 安全。
     *
     * @param self 待释放解析器，可为 NULL
     * @return {void}
     */
    void ipc_rtsp_shim_parser_free(ipc_rtsp_shim_parser *self);

    /**
     * 尝试从 @p buf 解析一个完整 RTSP 请求。
     *
     * @param self 解析器（每次成功解析前内部请求会被重置）
     * @param buf 输入字节流（内容不被修改，切片指向其内部）
     * @param len 输入字节数
     * @param[out] consumed 仅 COMPLETE 时写入本次消费的字节数；
     *                     PARTIAL/FAILURE 时不写入
     * @param[out] req 仅 COMPLETE 时指向本次请求（内部存储，下次 parser_run
     *                 前有效，须当场取值）；PARTIAL/FAILURE 时不写入
     * @return IPC_RTSP_SHIM_PARSE_* 之一
     */
    int ipc_rtsp_shim_parser_run(ipc_rtsp_shim_parser *self, const void *buf, size_t len, size_t *consumed, ipc_rtsp_shim_request **req);

    /**
     * 请求方法（返回内部切片指针，不保证以 '\0' 结尾）。
     *
     * @param self 已完成的请求
     * @param[out] len 可为 NULL；非空时写入方法长度
     * @return 方法切片指针（指向解析缓冲）
     */
    const char *ipc_rtsp_shim_request_method(const ipc_rtsp_shim_request *self, size_t *len);
    /**
     * 请求 URI。
     *
     * @param self 已完成的请求
     * @param[out] len 可为 NULL；非空时写入 URI 长度
     * @return URI 切片指针（不保证以 '\0' 结尾）
     */
    const char *ipc_rtsp_shim_request_uri(const ipc_rtsp_shim_request *self, size_t *len);
    /**
     * 请求 CSeq。
     *
     * @param self 已完成的请求
     * @return CSeq 数值
     */
    uint32_t ipc_rtsp_shim_request_cseq(const ipc_rtsp_shim_request *self);
    /**
     * 按名字取请求头；不存在返回 NULL。
     *
     * @param self 已完成的请求
     * @param name 头名（'\0' 结尾）
     * @param[out] len 可为 NULL；非空且找到时写入头值长度
     * @return 头值切片指针（不保证以 '\0' 结尾）；不存在返回 NULL
     */
    const char *ipc_rtsp_shim_request_header(const ipc_rtsp_shim_request *self, const char *name, size_t *len);
    /**
     * 请求 body（可能为空）。
     *
     * @param self 已完成的请求
     * @param[out] len 可为 NULL；非空时写入 body 长度（可为 0）
     * @return body 切片指针；无 body 时长度为 0
     */
    const char *ipc_rtsp_shim_request_body(const ipc_rtsp_shim_request *self, size_t *len);

    /* ------------------------------------------------------------------ */
    /* Writer 与响应                                                      */
    /* ------------------------------------------------------------------ */

    /**
     * 创建一个 writer 句柄，把 smolrtsp 的输出转发到调用方回调。
     *
     * 连接级对象：必须比任何基于它的响应/RTP transport 活得久。
     *
     * @param write_fn 写回调（必填，传 NULL 返回 NULL）
     * @param filled_fn 水位回调，返回当前输出缓冲字节数（可为 NULL，等价恒返 0）
     * @param user 透传给两个回调的上下文
     * @return writer 句柄；参数非法或分配失败返回 NULL
     */
    ipc_rtsp_shim_writer *ipc_rtsp_shim_writer_new(ipc_rtsp_shim_write_fn write_fn, ipc_rtsp_shim_filled_fn filled_fn, void *user);
    /**
     * 释放 writer 句柄；不触及回调与 user 资源。
     *
     * @param self 待释放 writer，可为 NULL
     * @return {void}
     */
    void ipc_rtsp_shim_writer_free(ipc_rtsp_shim_writer *self);

    /**
     * 基于某个请求创建响应对象（持有该请求的 CSeq）。
     *
     * @param req 已解析请求（只读取其 CSeq，不保留其余引用）
     * @param writer 输出目标（非拥有，须比 response 活得久）
     * @return 响应对象；分配失败返回 NULL
     */
    ipc_rtsp_shim_response *ipc_rtsp_shim_response_new(const ipc_rtsp_shim_request *req, ipc_rtsp_shim_writer *writer);

    /**
     * 用显式 CSeq 创建响应对象。
     *
     * 用于延迟响应（例如参数集未就绪时挂起的 DESCRIBE）：此时原始请求的解析缓冲
     * 可能已被消费，只能保留 CSeq。
     *
     * @param cseq 显式指定的 CSeq
     * @param writer 输出目标（非拥有，须比 response 活得久）
     * @return 响应对象；分配失败返回 NULL
     */
    ipc_rtsp_shim_response *ipc_rtsp_shim_response_new_with_cseq(uint32_t cseq, ipc_rtsp_shim_writer *writer);
    /**
     * 释放响应对象：先 drop 内部 smolrtsp 上下文再回收本体；NULL 安全。
     *
     * @param self 待释放响应对象，可为 NULL
     * @return {void}
     */
    void ipc_rtsp_shim_response_free(ipc_rtsp_shim_response *self);

    /**
     * 追加一个响应头（名字和值都以 '\0' 结尾）。
     *
     * @param self 响应对象
     * @param name 头名
     * @param value 头值
     * @return {void}
     */
    void ipc_rtsp_shim_response_header(ipc_rtsp_shim_response *self, const char *name, const char *value);
    /**
     * 追加一个响应头（显式长度，值可以不以 '\0' 结尾）。
     *
     * @param self 响应对象
     * @param name 头名（按 name_len 取用）
     * @param name_len 头名字节数
     * @param value 头值
     * @param value_len 头值字节数
     * @return {void}
     */
    void ipc_rtsp_shim_response_header_n(ipc_rtsp_shim_response *self,
                                         const char *name,
                                         size_t name_len,
                                         const char *value,
                                         size_t value_len);
    /**
     * 设置响应 body。
     *
     * @param self 响应对象
     * @param data body 数据（NULL 或 len=0 表示空 body）
     * @param len body 字节数
     * @return {void}
     */
    void ipc_rtsp_shim_response_body(ipc_rtsp_shim_response *self, const void *data, size_t len);
    /**
     * 序列化并写出响应（状态行/头/body 一次写出）；返回值与 smolrtsp 一致
     * （负值为失败）。
     *
     * @param self 响应对象
     * @param code HTTP 风格状态码（如 200、401）
     * @param reason 短语（'\0' 结尾，如 "OK"）
     * @return 非负为写出字节数，负值为失败
     */
    ssize_t ipc_rtsp_shim_response_send(ipc_rtsp_shim_response *self, uint16_t code, const char *reason);

    /** write_interleaved 单包 payload 上限；产品层 RTCP 复合缓冲必须与此一致。 */
#define IPC_RTSP_SHIM_INTERLEAVED_PAYLOAD_MAX 256

    /**
     * 发送一个 TCP interleaved RTCP 包；头和 payload 以单次 Writer::write 原子追加。
     * payload 上限为 IPC_RTSP_SHIM_INTERLEAVED_PAYLOAD_MAX 字节。
     *
     * @param self writer 句柄
     * @param channel_id interleaved 通道号
     * @param payload RTCP 复合包数据
     * @param len payload 字节数（0 或超上限直接失败）
     * @return 0 成功，-1 失败（参数非法或写入不完整）
     */
    int ipc_rtsp_shim_write_interleaved(ipc_rtsp_shim_writer *self, uint8_t channel_id, const void *payload, size_t len);

    /* ------------------------------------------------------------------ */
    /* RTP / NAL 发送                                                     */
    /* ------------------------------------------------------------------ */

    /**
     * 基于 TCP interleaved 通道创建 RTP 发送器（writer 必须比其活得久）。
     *
     * @param writer 输出通道（非拥有）
     * @param channel_id interleaved RTP 通道号
     * @param max_buffer 输出背压上限（字节）：writer 缓冲字节数超过它后
     *        rtp_is_full 置 true，由调用方轮询并停止喂数
     * @param ssrc RTP SSRC（主机序）
     * @param payload_ty RTP 负载类型
     * @param clock_rate 时钟频率（Hz）
     * @param ts_base_us 时间戳基准（微秒，RTP 时间戳换算偏移）
     * @return RTP 发送器（接管 transport 所有权）；分配失败返回 NULL
     */
    ipc_rtsp_shim_rtp *ipc_rtsp_shim_rtp_new_tcp(ipc_rtsp_shim_writer *writer,
                                                 uint8_t channel_id,
                                                 size_t max_buffer,
                                                 uint32_t ssrc,
                                                 uint8_t payload_ty,
                                                 uint32_t clock_rate,
                                                 uint64_t ts_base_us);

    /**
     * 基于已 connect 的 UDP socket 创建 RTP 发送器；**接管 fd**（free 时 close）。
     *
     * @param fd 已 connect 的 UDP socket
     * @param ssrc RTP SSRC（主机序）
     * @param payload_ty RTP 负载类型
     * @param clock_rate 时钟频率（Hz）
     * @param ts_base_us 时间戳基准（微秒）
     * @return RTP 发送器（接管 transport 与 fd 所有权）。无论成败均接管 fd：
     *         成功后由 transport drop 关闭；返回 NULL 时本函数已自行 close
     */
    ipc_rtsp_shim_rtp *ipc_rtsp_shim_rtp_new_udp(int fd, uint32_t ssrc, uint8_t payload_ty, uint32_t clock_rate, uint64_t ts_base_us);

    /**
     * 释放 RTP 发送器：drop transport（UDP 模式级联 close fd）并回收包装；NULL 安全。
     *
     * @param self 待释放发送器，可为 NULL
     * @return {void}
     */
    void ipc_rtsp_shim_rtp_free(ipc_rtsp_shim_rtp *self);
    /**
     * 输出背压判断：TCP 模式为 writer 缓冲字节超过 max_buffer；UDP 恒为 false。
     *
     * @param self RTP 发送器
     * @return true 达到背压上限，应暂停喂数；false 可继续发送
     */
    bool ipc_rtsp_shim_rtp_is_full(ipc_rtsp_shim_rtp *self);
    /**
     * 读 RTP SSRC。
     *
     * @param self RTP 发送器
     * @return SSRC（主机序）
     */
    uint32_t ipc_rtsp_shim_rtp_ssrc(ipc_rtsp_shim_rtp *self);
    /**
     * 读已成功发送的 RTP 包累计（与 seq 同步自增，见 rtp_next_seq）。
     *
     * @param self RTP 发送器
     * @return 已发送包数
     */
    uint32_t ipc_rtsp_shim_rtp_pkt_count(ipc_rtsp_shim_rtp *self);

    /**
     * 下一个待发 RTP 包的序号（PLAY 响应 RTP-Info seq 字段）。基于官方 getter
     * 推导，不依赖结构布局；正确性依赖 seq 与 pkt_count 同步自增的不变式，
     * 升级 smolrtsp 时须复核。
     *
     * @param self RTP 发送器
     * @return 下一个包的 16 位序号
     */
    uint16_t ipc_rtsp_shim_rtp_next_seq(ipc_rtsp_shim_rtp *self);
    /**
     * 读已发送负载字节数累计（RFC 3550 SR octet count 口径，不含 RTP 头）。
     *
     * @param self RTP 发送器
     * @return 已发送负载字节数
     */
    uint32_t ipc_rtsp_shim_rtp_octet_count(ipc_rtsp_shim_rtp *self);
    /**
     * 读最近一次成功发送的 RTP 时间戳（线上 32 位值）。
     *
     * @param self RTP 发送器
     * @return 最近一次的 RTP 时间戳
     */
    uint32_t ipc_rtsp_shim_rtp_last_ts(ipc_rtsp_shim_rtp *self);

    /**
     * 发送一个不带 NAL 头语义的 RTP 负载（音频等），时间戳为 32 位线上值。
     *
     * @param self RTP 发送器
     * @param raw_ts 32 位 RTP 时间戳（线上值，由调用方按时钟频率换算）
     * @param marker RTP marker 位
     * @param payload 负载数据
     * @param len 负载字节数
     * @return 0 成功，-1 失败（底层 transport 写出失败）
     */
    int ipc_rtsp_shim_rtp_send(ipc_rtsp_shim_rtp *self, uint32_t raw_ts, bool marker, const void *payload, size_t len);

    /**
     * 创建 H.264 NAL 发送器（自动处理 FU 分片）。SMOLRTSP_WITH_H264 未定义时
     * max_nalu_size 被静默忽略，沿用默认 1200 字节分片预算。
     *
     * @param rtp 底层 RTP 发送器（接管其生命周期，free 本对象时级联释放）
     * @param max_nalu_size 单 NAL 分片预算（字节，超过按 FU 分片）
     * @return NAL 发送器；分配失败返回 NULL（rtp 已被释放）
     */
    ipc_rtsp_shim_nal *ipc_rtsp_shim_nal_new_h264(ipc_rtsp_shim_rtp *rtp, size_t max_nalu_size);
    /**
     * 创建 H.265 NAL 发送器（自动处理 FU 分片）。SMOLRTSP_WITH_H265 未定义时
     * max_nalu_size 被静默忽略，沿用默认 1200 字节分片预算。
     *
     * @param rtp 底层 RTP 发送器（接管其生命周期，free 本对象时级联释放）
     * @param max_nalu_size 单 NAL 分片预算（字节，超过按 FU 分片）
     * @return NAL 发送器；分配失败返回 NULL（rtp 已被释放）
     */
    ipc_rtsp_shim_nal *ipc_rtsp_shim_nal_new_h265(ipc_rtsp_shim_rtp *rtp, size_t max_nalu_size);
    /**
     * 释放 NAL 发送器：级联释放 RTP transport 与下层 transport（UDP 模式含
     * close fd）；NULL 安全。
     *
     * @param self 待释放 NAL 发送器，可为 NULL
     * @return {void}
     */
    void ipc_rtsp_shim_nal_free(ipc_rtsp_shim_nal *self);

    /**
     * 发送一个完整 NAL（含 NAL 头）；@p au_end 仅在 AU 最后一个 NAL 为 true。
     *
     * @param self NAL 发送器
     * @param raw_ts 32 位 RTP 时间戳（线上值）
     * @param au_end 是否为访问单元最后一个 NAL（置 RTP marker）
     * @param nal 完整 NAL 数据（含 NAL 头）
     * @param len NAL 字节数（须大于 NAL 头长度）
     * @return 0 成功，-1 失败（参数非法或写出失败）
     */
    int ipc_rtsp_shim_nal_send(ipc_rtsp_shim_nal *self, uint32_t raw_ts, bool au_end, const void *nal, size_t len);

    /* ------------------------------------------------------------------ */
    /* JPEG（RFC 2435）发送                                                */
    /* ------------------------------------------------------------------ */

    /**
     * RFC 2435 一帧描述；各指针仅在 send 调用期间被引用，函数返回后不再使用。
     * 需构建开启 IPC_RTSP_WITH_MJPEG（默认开启）。
     */
    typedef struct
    {
        uint8_t type;              /**< 0=4:2:2、1=4:2:0；带重启标记为基值+64 */
        uint8_t q;                 /**< 质量因子；>=128 表示首包内嵌量化表 */
        uint8_t width_blocks;      /**< 宽度（8 像素单位） */
        uint8_t height_blocks;     /**< 高度（8 像素单位） */
        uint16_t restart_interval; /**< 重启间隔（MCU），0 表示无 */
        const uint8_t *qt0;        /**< 量化表 0（zigzag 序），可为 NULL */
        size_t qt0_len;
        const uint8_t *qt1; /**< 量化表 1，可为 NULL（灰度/单表帧） */
        size_t qt1_len;
        const uint8_t *scan; /**< 熵编码数据（SOS 之后、EOI 之前），可为 NULL */
        size_t scan_len;
    } ipc_rtsp_shim_jpeg_frame;

    /**
     * 基于 RTP 发送器创建 JPEG 发送器（**接管 rtp 的生命周期**）。
     *
     * @param rtp 底层 RTP 发送器（失败时被释放，成功后由 jpeg_free 级联释放）
     * @param max_packet_size 单个 RTP 包上限（字节），超出按 RFC 2435 分片
     * @return JPEG 发送器；失败返回 NULL（rtp 已被释放）
     */
    ipc_rtsp_shim_jpeg *ipc_rtsp_shim_jpeg_new(ipc_rtsp_shim_rtp *rtp, size_t max_packet_size);
    /**
     * 释放 JPEG 发送器：级联释放 RTP transport 与下层 transport；NULL 安全。
     *
     * @param self 待释放 JPEG 发送器，可为 NULL
     * @return {void}
     */
    void ipc_rtsp_shim_jpeg_free(ipc_rtsp_shim_jpeg *self);
    /**
     * 输出背压判断（透传底层 RTP transport 的 is_full 口径）。
     *
     * @param self JPEG 发送器
     * @return true 达到背压上限，应暂停喂数
     */
    bool ipc_rtsp_shim_jpeg_is_full(ipc_rtsp_shim_jpeg *self);

    /**
     * 按规范打包发送一帧：自动分片、逐包 fragment_offset、首包内嵌量化表、
     * 尾包置 marker。
     *
     * @param self JPEG 发送器
     * @param raw_ts 32 位 RTP 时间戳（线上值）
     * @param frame 帧描述（各指针仅在本次调用期间被引用）
     * @return 0 成功，-1 失败（errno 由底层 transport 设置）
     */
    int ipc_rtsp_shim_jpeg_send_frame(ipc_rtsp_shim_jpeg *self, uint32_t raw_ts, const ipc_rtsp_shim_jpeg_frame *frame);

    /* ------------------------------------------------------------------ */
    /* 时钟与 RTCP 序列化                                                 */
    /* ------------------------------------------------------------------ */

    /**
     * 把微秒级时间映射为指定时钟频率的 RTP 时间戳（含 ts_base 偏移）。
     *
     * @param time_us 微秒时间（与 @p ts_base_us 同基准）
     * @param clock_rate 时钟频率（Hz）
     * @param ts_base_us 时间戳基准（微秒）
     * @return 32 位 RTP 时间戳（模 2^32 回绕）
     */
    uint32_t ipc_rtsp_shim_rtp_ts_from_us(uint64_t time_us, uint32_t clock_rate, uint64_t ts_base_us);

    /**
     * 序列化 RTCP SR。
     *
     * @param[out] buf 输出缓冲
     * @param cap 缓冲容量（不足返回 0，不写缓冲）
     * @param ssrc 发送方 SSRC（主机序，内部转网络序）
     * @param ntp_sec NTP 时间戳秒部（RFC 3550 §6.4）
     * @param ntp_frac NTP 时间戳分数部
     * @param rtp_ts 对应的 RTP 媒体时间戳
     * @param pkt_count 已发送 RTP 包数
     * @param octet_count 已发送负载字节数
     * @return 序列化字节数；容量不足返回 0
     */
    size_t ipc_rtsp_shim_rtcp_sr(void *buf,
                                 size_t cap,
                                 uint32_t ssrc,
                                 uint32_t ntp_sec,
                                 uint32_t ntp_frac,
                                 uint32_t rtp_ts,
                                 uint32_t pkt_count,
                                 uint32_t octet_count);
    /**
     * 序列化 RTCP SDES(CNAME)。
     *
     * @param[out] buf 输出缓冲
     * @param cap 缓冲容量（不足返回 0，不写缓冲）
     * @param ssrc 发送方 SSRC（主机序）
     * @param cname CNAME 字符串（'\0' 结尾）
     * @return 序列化字节数；容量不足返回 0
     */
    size_t ipc_rtsp_shim_rtcp_sdes_cname(void *buf, size_t cap, uint32_t ssrc, const char *cname);
    /**
     * 序列化 RTCP BYE。
     *
     * @param[out] buf 输出缓冲
     * @param cap 缓冲容量（不足返回 0，不写缓冲）
     * @param ssrc 发送方 SSRC（主机序）
     * @param reason 退出原因（可为任意字符串）
     * @return 序列化字节数；容量不足返回 0
     */
    size_t ipc_rtsp_shim_rtcp_bye(void *buf, size_t cap, uint32_t ssrc, const char *reason);

    /* ------------------------------------------------------------------ */
    /* 协议工具                                                           */
    /* ------------------------------------------------------------------ */

    /**
     * 解析 Transport 头。
     *
     * @param value 头值（可不必 '\0' 结尾）
     * @param len 头值字节数
     * @param[out] out 解析结果（成功时整体重置后填写；失败时不修改）
     * @return 0 成功，-1 失败
     */
    int ipc_rtsp_shim_parse_transport(const char *value, size_t len, ipc_rtsp_shim_transport *out);

    /**
     * 解析一个 interleaved 帧（客户端发来的 RTCP RR 等）。
     *
     * @param buf 输入缓冲
     * @param len 输入字节数
     * @param[out] channel_id COMPLETE 时写入通道号，其余情况不写入
     * @param[out] payload 指向 @p buf 内部的负载（COMPLETE 时写入），调用方
     *                     用完即弃
     * @param[out] payload_len COMPLETE 时写入负载字节数，其余情况不写入
     * @param[out] consumed COMPLETE 时写入本次消费的字节数，其余情况不写入
     * @return IPC_RTSP_SHIM_IL_* 之一
     */
    int ipc_rtsp_shim_parse_interleaved(const void *buf,
                                        size_t len,
                                        uint8_t *channel_id,
                                        const void **payload,
                                        size_t *payload_len,
                                        size_t *consumed);

#ifdef __cplusplus
}
#endif

#endif /* IPC_RTSP_SMOLRTSP_SHIM_H */
