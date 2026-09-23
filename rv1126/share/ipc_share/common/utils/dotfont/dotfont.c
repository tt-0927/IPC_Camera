/**
 * @file    dotfont.c
 * @brief   轻量级点阵字库加载与文本渲染实现（LVGL bin 格式）
 *
 * 二进制布局要点（lv_font_conv 1.5.3 --format bin，已逐字段实证）：
 *  - 文件由若干表顺序连接，每表以 [4B 记录长度][4B 表标记] 开头，靠记录长度跳表；
 *  - 多字节数值一律小端；glyph 内部位流为大端位序（MSB first）；
 *  - cmap 的子表 data_off 为相对 cmap 表起点（含 12B 表头与子表头数组）的偏移；
 *  - loca 条目存的是含 glyf 表头 8B 的偏移，glyph 数据绝对位置 = glyf_off + loca[gid]；
 *  - glyph 位流：advanceWidth(adv_bits) / bbox_x(xy_bits,有符号) / bbox_y(xy_bits,有符号)
 *    / bbox_w(wh_bits) / bbox_h(wh_bits)，随后是连续 bpp 位图（行尾不对齐到字节）；
 *  - 字形坐标系：原点在基线左端，bbox (x,y) 为左下角，位图自 bbox 顶行起逐行向下；
 *  - 1bpp 字库生成时强制不压缩（compression=0），本实现仅支持未压缩位图。
 */

#include "dotfont.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* mmap 支持仅限类 POSIX 平台；极端裁剪环境可定义 DOTFONT_NO_MMAP 关闭 */
#ifndef DOTFONT_NO_MMAP
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#define DOTFONT_HAVE_MMAP 1
#endif

/* ---------------- 表标记 ---------------- */

#define DOTFONT_TAG(a, b, c, d) ((uint32_t) (a) | ((uint32_t) (b) << 8) | ((uint32_t) (c) << 16) | ((uint32_t) (d) << 24))

#define TAG_HEAD DOTFONT_TAG('h', 'e', 'a', 'd')
#define TAG_CMAP DOTFONT_TAG('c', 'm', 'a', 'p')
#define TAG_LOCA DOTFONT_TAG('l', 'o', 'c', 'a')
#define TAG_GLYF DOTFONT_TAG('g', 'l', 'y', 'f')

/* head 表内字段偏移（相对表起点，完整定义见 docs/osd_dot_font/01_点阵字库格式规范.md） */
#define HEAD_OFF_SIZE_PX     14u
#define HEAD_OFF_ASCENT      16u
#define HEAD_OFF_DESCENT     18u
#define HEAD_OFF_LOCFMT      34u
#define HEAD_OFF_ADV_FMT     36u
#define HEAD_OFF_BPP         37u
#define HEAD_OFF_XY_BITS     38u
#define HEAD_OFF_WH_BITS     39u
#define HEAD_OFF_ADV_BITS    40u
#define HEAD_OFF_COMPRESSION 41u

/* cmap 子表格式 */
#define CMAP_FMT_FORMAT0      0u /* uint8 delta 数组，gid = delta + offset */
#define CMAP_FMT_SPARSE       1u /* uint16 cp 列表 + uint16 delta 列表 */
#define CMAP_FMT_FORMAT0_TINY 2u /* 连续区段，gid = offset + (cp - start) */
#define CMAP_FMT_SPARSE_TINY  3u /* uint16 cp 列表，gid = offset + idx */

/* cmap 表结构偏移 */
#define CMAP_OFF_COUNT      8u /* uint32 子表数 */
#define CMAP_SUBHEAD_SIZE   16u
#define CMAP_SUB_OFF_DATA   0u  /* uint32 数据偏移（相对 cmap 表起点） */
#define CMAP_SUB_OFF_START  4u  /* uint32 起始码点 */
#define CMAP_SUB_OFF_LEN    8u  /* uint16 区段长度 */
#define CMAP_SUB_OFF_GIDOFF 10u /* uint16 gid 基数 */
#define CMAP_SUB_OFF_COUNT  12u /* uint16 sparse 条目数 */
#define CMAP_SUB_OFF_FORMAT 14u /* uint8 格式 */

/* glyph id 0 为保留空字形 */
#define GLYPH_ID_RESERVED 0u

/* UTF-8 解码失败时的替换码点 */
#define UTF8_REPLACEMENT 0xFFFFFFFFu

/* ---------------- 小端读取 ---------------- */

static uint16_t rd_u16(const uint8_t *p)
{
    return (uint16_t) (p[0] | ((uint16_t) p[1] << 8));
}

static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

static int16_t rd_s16(const uint8_t *p)
{
    return (int16_t) rd_u16(p);
}

/* ---------------- 位读取（大端位序，MSB first） ---------------- */

/** 从 data 的第 bit 位起顺序读 nbits 位 */
static uint32_t br_read_at(const uint8_t *data, size_t bit, int nbits)
{
    uint32_t value = 0;
    int i;

    for (i = 0; i < nbits; i++)
    {
        size_t pos = bit + (size_t) i;
        value = (value << 1) | ((data[pos >> 3] >> (7u - (pos & 7u))) & 1u);
    }

    return value;
}

/* ---------------- 颜色工具（ARGB4444：A<<12 | R<<8 | G<<4 | B） ---------------- */

static uint16_t to_argb4444(const dotfont_color_t *color)
{
    return (uint16_t) (((uint16_t) (color->a >> 4) << 12) | ((uint16_t) (color->r >> 4) << 8) | ((uint16_t) (color->g >> 4) << 4) |
                       (uint16_t) (color->b >> 4));
}

/**
 * @brief   抗锯齿像素混合：dst = dst*(1-cov/max) + fg*(cov/max)
 * @note    仅 2/4bpp 字库走到此路径；1bpp 覆盖率为满值，不会进入
 */
static uint16_t blend_pixel(uint16_t dst_px, uint16_t fg_px, int coverage, int max_coverage)
{
    uint16_t result = 0;
    int shift;
    int inv = max_coverage - coverage;

    for (shift = 0; shift < 16; shift += 4)
    {
        int d = (int) ((dst_px >> shift) & 0xF);
        int f = (int) ((fg_px >> shift) & 0xF);
        int v = (d * inv + f * coverage + max_coverage / 2) / max_coverage;

        result |= (uint16_t) (v & 0xF) << shift;
    }

    return result;
}

/* ---------------- 字库对象 ---------------- */

struct dotfont
{
    uint8_t *data; /* 字库数据（堆内存或 mmap 映射） */
    size_t size;   /* 数据总长 */
    int owns_data; /* close 时是否 free data */
    int is_mmap;   /* mmap 映射时 close 走 munmap */

    /* head 关键字段 */
    int size_px;     /* 基准像素字号 */
    int ascent;      /* 基线以上高度（正） */
    int descent;     /* 基线以下深度（负） */
    int bpp;         /* 每像素位数 1/2/4 */
    int compression; /* 压缩算法，仅支持 0（raw） */
    int adv_bits;
    int xy_bits;
    int wh_bits;
    int adv_fmt; /* 0=整数 1=FP4.4 */
    int loc_fmt; /* 0=Offset16 1=Offset32 */

    /* 各表位置 */
    size_t cmap_off;
    uint32_t cmap_sub_count;
    size_t loca_off;
    uint32_t loca_count;
    size_t glyf_off;
    size_t glyf_size;
};

/* ---------------- 表解析 ---------------- */

static int parse_tables(dotfont_t *font)
{
    size_t off = 0;
    int has_head = 0;

    while (off + 8u <= font->size)
    {
        uint32_t rec_size = rd_u32(font->data + off);
        uint32_t tag = rd_u32(font->data + off + 4u);

        if (rec_size < 8u || off + rec_size > font->size)
        {
            break;
        }

        if (TAG_HEAD == tag && rec_size >= 48u)
        {
            font->size_px = rd_u16(font->data + off + HEAD_OFF_SIZE_PX);
            font->ascent = rd_u16(font->data + off + HEAD_OFF_ASCENT);
            font->descent = rd_s16(font->data + off + HEAD_OFF_DESCENT);
            font->loc_fmt = font->data[off + HEAD_OFF_LOCFMT];
            font->adv_fmt = font->data[off + HEAD_OFF_ADV_FMT];
            font->bpp = font->data[off + HEAD_OFF_BPP];
            font->xy_bits = font->data[off + HEAD_OFF_XY_BITS];
            font->wh_bits = font->data[off + HEAD_OFF_WH_BITS];
            font->adv_bits = font->data[off + HEAD_OFF_ADV_BITS];
            font->compression = font->data[off + HEAD_OFF_COMPRESSION];
            has_head = 1;
        }
        else if (TAG_CMAP == tag && rec_size >= 12u)
        {
            font->cmap_off = off;
            font->cmap_sub_count = rd_u32(font->data + off + CMAP_OFF_COUNT);
        }
        else if (TAG_LOCA == tag && rec_size >= 12u)
        {
            font->loca_off = off;
            font->loca_count = rd_u32(font->data + off + 8u);
        }
        else if (TAG_GLYF == tag)
        {
            font->glyf_off = off;
            font->glyf_size = rec_size;
        }

        off += rec_size;
    }

    if (!has_head || 0u == font->cmap_sub_count || 0u == font->loca_count || 0u == font->glyf_size)
    {
        return -1;
    }

    /* 本实现仅支持未压缩位图（gen_font.sh 生成时 --no-compress，1bpp 天然未压缩） */
    if (0 != font->compression)
    {
        return -1;
    }

    if (font->bpp < 1 || font->bpp > 4 || 3 == font->bpp)
    {
        return -1;
    }

    if (font->size_px <= 0 || font->ascent <= 0)
    {
        return -1;
    }

    return 0;
}

/* ---------------- 文件加载 ---------------- */

static int load_whole_file(const char *path, dotfont_t *font)
{
    FILE *fp = fopen(path, "rb");
    long file_size;
    uint8_t *buf;

    if (NULL == fp)
    {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0 || (file_size = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0)
    {
        fclose(fp);
        return -1;
    }

    buf = (uint8_t *) malloc((size_t) file_size);
    if (NULL == buf)
    {
        fclose(fp);
        return -1;
    }

    if (fread(buf, 1, (size_t) file_size, fp) != (size_t) file_size)
    {
        free(buf);
        fclose(fp);
        return -1;
    }

    fclose(fp);

    font->data = buf;
    font->size = (size_t) file_size;
    font->owns_data = 1;
    font->is_mmap = 0;

    return 0;
}

#ifdef DOTFONT_HAVE_MMAP
static int load_mmap_file(const char *path, dotfont_t *font)
{
    int fd = open(path, O_RDONLY);
    struct stat st;
    void *mapped;

    if (fd < 0)
    {
        return -1;
    }

    if (fstat(fd, &st) != 0 || st.st_size <= 0)
    {
        close(fd);
        return -1;
    }

    mapped = mmap(NULL, (size_t) st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (MAP_FAILED == mapped)
    {
        return -1;
    }

    font->data = (uint8_t *) mapped;
    font->size = (size_t) st.st_size;
    font->owns_data = 0;
    font->is_mmap = 1;

    return 0;
}
#endif

static dotfont_t *dotfont_create(void)
{
    return (dotfont_t *) calloc(1, sizeof(dotfont_t));
}

static int dotfont_finalize(dotfont_t *font)
{
    if (0 != parse_tables(font))
    {
        dotfont_close(font);
        return -1;
    }

    return 0;
}

int dotfont_open(dotfont_t **out_font, const char *path)
{
    dotfont_t *font;

    if (NULL == out_font || NULL == path)
    {
        return -1;
    }

    *out_font = NULL;
    font = dotfont_create();
    if (NULL == font)
    {
        return -1;
    }

    if (0 != load_whole_file(path, font) || 0 != dotfont_finalize(font))
    {
        return -1;
    }

    *out_font = font;
    return 0;
}

#ifdef DOTFONT_HAVE_MMAP
int dotfont_open_mmap(dotfont_t **out_font, const char *path)
{
    dotfont_t *font;

    if (NULL == out_font || NULL == path)
    {
        return -1;
    }

    *out_font = NULL;
    font = dotfont_create();
    if (NULL == font)
    {
        return -1;
    }

    if (0 != load_mmap_file(path, font) || 0 != dotfont_finalize(font))
    {
        return -1;
    }

    *out_font = font;
    return 0;
}
#else
int dotfont_open_mmap(dotfont_t **out_font, const char *path)
{
    /* 平台无 mmap，退化为整文件读入 */
    return dotfont_open(out_font, path);
}
#endif

int dotfont_open_mem(dotfont_t **out_font, const void *data, size_t len, int take_own)
{
    dotfont_t *font;

    if (NULL == out_font || NULL == data || 0u == len)
    {
        return -1;
    }

    *out_font = NULL;
    font = dotfont_create();
    if (NULL == font)
    {
        return -1;
    }

    font->data = (uint8_t *) data;
    font->size = len;
    font->owns_data = (0 != take_own);
    font->is_mmap = 0;

    if (0 != dotfont_finalize(font))
    {
        return -1;
    }

    *out_font = font;
    return 0;
}

void dotfont_close(dotfont_t *font)
{
    if (NULL == font)
    {
        return;
    }

#ifdef DOTFONT_HAVE_MMAP
    if (font->is_mmap)
    {
        (void) munmap(font->data, font->size);
    }
    else if (font->owns_data)
    {
        free(font->data);
    }
#else
    if (font->owns_data)
    {
        free(font->data);
    }
#endif

    free(font);
}

int dotfont_get_metrics(const dotfont_t *font, int *out_px, int *out_line_h)
{
    if (NULL == font)
    {
        return -1;
    }

    if (NULL != out_px)
    {
        *out_px = font->size_px;
    }

    if (NULL != out_line_h)
    {
        *out_line_h = font->ascent - font->descent;
    }

    return 0;
}

int dotfont_get_ascent(const dotfont_t *font)
{
    if (NULL == font)
    {
        return -1;
    }

    return font->ascent;
}

/* ---------------- cmap 查找：Unicode 码点 -> glyph id ---------------- */

static int cmap_lookup(const dotfont_t *font, uint32_t codepoint, uint32_t *out_gid)
{
    uint32_t i;

    for (i = 0; i < font->cmap_sub_count; i++)
    {
        const uint8_t *sub = font->data + font->cmap_off + 12u + (size_t) i * CMAP_SUBHEAD_SIZE;
        uint32_t data_off = rd_u32(sub + CMAP_SUB_OFF_DATA);
        uint32_t range_start = rd_u32(sub + CMAP_SUB_OFF_START);
        uint32_t range_end = range_start + rd_u16(sub + CMAP_SUB_OFF_LEN);
        uint16_t gid_offset = rd_u16(sub + CMAP_SUB_OFF_GIDOFF);
        uint16_t entry_count = rd_u16(sub + CMAP_SUB_OFF_COUNT);
        uint8_t format = sub[CMAP_SUB_OFF_FORMAT];
        const uint8_t *payload = font->data + font->cmap_off + data_off;

        if (codepoint < range_start || codepoint >= range_end)
        {
            continue;
        }

        switch (format)
        {
        case CMAP_FMT_FORMAT0:
        {
            /* uint8 delta 数组；缺失项 delta 为 0（指向保留空字形），视为缺字 */
            uint32_t gid = (uint32_t) gid_offset + payload[codepoint - range_start];
            if (GLYPH_ID_RESERVED == gid)
            {
                return -1;
            }
            *out_gid = gid;
            return 0;
        }

        case CMAP_FMT_FORMAT0_TINY:
        {
            uint32_t gid = (uint32_t) gid_offset + (codepoint - range_start);
            if (GLYPH_ID_RESERVED == gid)
            {
                return -1;
            }
            *out_gid = gid;
            return 0;
        }

        case CMAP_FMT_SPARSE_TINY:
        {
            uint16_t k;
            for (k = 0; k < entry_count; k++)
            {
                if (rd_u16(payload + (size_t) k * 2u) == (uint16_t) (codepoint - range_start))
                {
                    uint32_t gid = (uint32_t) gid_offset + k;
                    if (GLYPH_ID_RESERVED == gid)
                    {
                        return -1;
                    }
                    *out_gid = gid;
                    return 0;
                }
            }
            return -1;
        }

        case CMAP_FMT_SPARSE:
        {
            uint16_t k;
            const uint8_t *ids = payload + (size_t) entry_count * 2u;
            for (k = 0; k < entry_count; k++)
            {
                if (rd_u16(payload + (size_t) k * 2u) == (uint16_t) (codepoint - range_start))
                {
                    uint32_t gid = (uint32_t) gid_offset + rd_u16(ids + (size_t) k * 2u);
                    if (GLYPH_ID_RESERVED == gid)
                    {
                        return -1;
                    }
                    *out_gid = gid;
                    return 0;
                }
            }
            return -1;
        }

        default:
            return -1;
        }
    }

    return -1;
}

/* ---------------- UTF-8 解码 ---------------- */

/**
 * @brief   解析下一个 UTF-8 码点
 * @param   [in,out] pp    当前位置指针，返回时推进到下一字符
 * @param   [in]     end   缓冲结束位置
 * @return  Unicode 码点；非法序列返回 UTF8_REPLACEMENT 并推进 1 字节
 */
static uint32_t utf8_next(const char **pp, const char *end)
{
    const uint8_t *p = (const uint8_t *) *pp;
    uint32_t cp;
    int len;
    int i;

    if (*pp >= end)
    {
        return UTF8_REPLACEMENT;
    }

    if (p[0] < 0x80u)
    {
        *pp = (const char *) p + 1;
        return p[0];
    }

    if ((p[0] & 0xE0u) == 0xC0u)
    {
        len = 2;
        cp = p[0] & 0x1Fu;
    }
    else if ((p[0] & 0xF0u) == 0xE0u)
    {
        len = 3;
        cp = p[0] & 0x0Fu;
    }
    else if ((p[0] & 0xF8u) == 0xF0u)
    {
        len = 4;
        cp = p[0] & 0x07u;
    }
    else
    {
        *pp = (const char *) p + 1;
        return UTF8_REPLACEMENT;
    }

    if (*pp + len > end)
    {
        *pp = (const char *) p + 1;
        return UTF8_REPLACEMENT;
    }

    for (i = 1; i < len; i++)
    {
        if ((p[i] & 0xC0u) != 0x80u)
        {
            *pp = (const char *) p + 1;
            return UTF8_REPLACEMENT;
        }
        cp = (cp << 6) | (p[i] & 0x3Fu);
    }

    *pp = (const char *) p + len;
    return cp;
}

/* ---------------- 渲染 ---------------- */

/**
 * @brief   解码 glyph 位流并将位图按 scale 展开绘制到目标缓冲
 * @param   [in] pen_x        笔尖 x（基准像素，未乘 scale）
 * @param   [in] baseline_y   基线 y（目标坐标，已含 scale）
 * @param   [in] dst_off_x    目标缓冲 x 偏移（文本行左上角）
 * @param   [out] out_advance 字形 advance（基准像素）
 * @return  0 成功（含空字形）；-1 解析失败
 */
static int render_glyph(const dotfont_t *font,
                        uint32_t gid,
                        int pen_x,
                        int baseline_y,
                        int dst_off_x,
                        int scale,
                        uint16_t fg4444,
                        int has_bg,
                        uint16_t *dst,
                        int dst_w,
                        int dst_h,
                        int dst_stride,
                        int *out_advance)
{
    uint32_t glyph_offset;
    int adv_raw;
    int box_x;
    int box_y;
    int box_w;
    int box_h;
    int max_coverage;
    int gx;
    int gy;
    size_t bitmap_bit;

    if (GLYPH_ID_RESERVED == gid || (uint32_t) font->loca_count <= gid)
    {
        return -1;
    }

    if (0 != font->loc_fmt)
    {
        glyph_offset = rd_u32(font->data + font->loca_off + 12u + (size_t) gid * 4u);
    }
    else
    {
        glyph_offset = rd_u16(font->data + font->loca_off + 12u + (size_t) gid * 2u);
    }

    if ((size_t) glyph_offset >= font->glyf_size)
    {
        return -1;
    }

    /* 字形头位流：advance / bbox_x / bbox_y / bbox_w / bbox_h */
    adv_raw = (int) br_read_at(font->data + font->glyf_off, (size_t) glyph_offset * 8u, font->adv_bits);
    {
        size_t bit = (size_t) glyph_offset * 8u + (size_t) font->adv_bits;
        box_x = (int) br_read_at(font->data + font->glyf_off, bit, font->xy_bits);
        box_x = (box_x & (1 << (font->xy_bits - 1))) ? (box_x - (1 << font->xy_bits)) : box_x;
        bit += (size_t) font->xy_bits;

        box_y = (int) br_read_at(font->data + font->glyf_off, bit, font->xy_bits);
        box_y = (box_y & (1 << (font->xy_bits - 1))) ? (box_y - (1 << font->xy_bits)) : box_y;
        bit += (size_t) font->xy_bits;

        box_w = (int) br_read_at(font->data + font->glyf_off, bit, font->wh_bits);
        bit += (size_t) font->wh_bits;

        box_h = (int) br_read_at(font->data + font->glyf_off, bit, font->wh_bits);
        bit += (size_t) font->wh_bits;

        bitmap_bit = bit;
    }

    /* advance 取整（FP4.4 四舍五入）；空字形只推进笔尖 */
    *out_advance = (1 == font->adv_fmt) ? ((adv_raw + 8) >> 4) : adv_raw;

    if (box_w <= 0 || box_h <= 0)
    {
        return 0;
    }

    max_coverage = (1 << font->bpp) - 1;

    for (gy = 0; gy < box_h; gy++)
    {
        /* 字形第 gy 行相对基线偏移：bbox 顶行 = box_y + box_h - 1，向下递减 */
        int y_rel = box_y + box_h - 1 - gy;
        int dst_y = baseline_y - (y_rel + 1) * scale;

        for (gx = 0; gx < box_w; gx++)
        {
            int coverage = (int) br_read_at(font->data + font->glyf_off,
                                            bitmap_bit + ((size_t) gy * (size_t) box_w + (size_t) gx) * (size_t) font->bpp,
                                            font->bpp);
            int dst_x = dst_off_x + (pen_x + box_x + gx) * scale;
            int sx;
            int sy;

            if (coverage <= 0)
            {
                continue;
            }

            for (sy = 0; sy < scale; sy++)
            {
                int py = dst_y + sy;
                uint16_t *row;

                if (py < 0 || py >= dst_h)
                {
                    continue;
                }

                row = dst + (size_t) py * (size_t) dst_stride;

                for (sx = 0; sx < scale; sx++)
                {
                    int px = dst_x + sx;

                    if (px < 0 || px >= dst_w)
                    {
                        continue;
                    }

                    if (coverage >= max_coverage)
                    {
                        row[px] = fg4444;
                    }
                    else if (has_bg)
                    {
                        row[px] = blend_pixel(row[px], fg4444, coverage, max_coverage);
                    }
                    else if (coverage * 2 > max_coverage)
                    {
                        /* 透明背景上的抗锯齿像素：覆盖率过半才落笔，避免暗边 */
                        row[px] = fg4444;
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * @brief   绘制缺字占位方框（空心线框，提示字符不在字库覆盖范围）
 * @param   [in] pen_x        笔尖 x（基准像素）
 * @param   [in] dst_off_x    目标缓冲 x 偏移（文本行左上角）
 * @return  占位宽度（基准像素）
 */
static int render_missing_box(const dotfont_t *font,
                              int pen_x,
                              int baseline_y,
                              int dst_off_x,
                              int scale,
                              uint16_t fg4444,
                              uint16_t *dst,
                              int dst_w,
                              int dst_h,
                              int dst_stride)
{
    int box_w = (font->size_px + 1) / 2 - 1; /* 约半字宽，视觉轻量 */
    int box_h = font->ascent - 1;            /* 顶到 ascent，底到基线 */
    int line = scale > 0 ? scale : 1;
    int x0 = dst_off_x + pen_x * scale;
    int y0 = baseline_y - box_h * scale;
    int i;

    for (i = 0; i < box_h * scale; i++)
    {
        for (int j = 0; j < box_w * scale; j++)
        {
            int px = x0 + j;
            int py = y0 + i;
            int on_border = (i < line) || (i >= box_h * scale - line) || (j < line) || (j >= box_w * scale - line);

            if (px < 0 || px >= dst_w || py < 0 || py >= dst_h || !on_border)
            {
                continue;
            }

            dst[(size_t) py * (size_t) dst_stride + px] = fg4444;
        }
    }

    return box_w + 1;
}

/**
 * @brief   单行文本排版：累加 advance 得到文本宽度，可选逐字渲染
 * @param   [in] do_render  0 仅测量；1 渲染
 */
static int layout_line(const dotfont_t *font,
                       const char *utf8,
                       int scale,
                       uint16_t fg4444,
                       int has_bg,
                       uint16_t *dst,
                       int dst_w,
                       int dst_h,
                       int dst_stride,
                       int dst_x,
                       int dst_y,
                       int do_render,
                       int *out_w,
                       int *out_h)
{
    const char *p;
    const char *end;
    int baseline_y = dst_y + font->ascent * scale;
    int pen_x = 0;
    int rendered = 0;

    if (NULL == utf8)
    {
        return -1;
    }

    p = utf8;
    end = utf8 + strlen(utf8);

    while (p < end)
    {
        uint32_t cp = utf8_next(&p, end);

        if ('\n' == cp || '\r' == cp)
        {
            continue; /* 单行接口忽略换行符 */
        }

        if (UTF8_REPLACEMENT == cp)
        {
            /* 非法字节：占位方框并推进半个字宽 */
            if (do_render)
            {
                pen_x += render_missing_box(font, pen_x, baseline_y, dst_x, scale, fg4444, dst, dst_w, dst_h, dst_stride);
            }
            else
            {
                pen_x += (font->size_px + 1) / 2;
            }
            rendered = 1;
            continue;
        }

        {
            uint32_t gid = 0;

            if (0 != cmap_lookup(font, cp, &gid))
            {
                /* 字库未覆盖：占位方框 */
                if (do_render)
                {
                    pen_x += render_missing_box(font, pen_x, baseline_y, dst_x, scale, fg4444, dst, dst_w, dst_h, dst_stride);
                }
                else
                {
                    pen_x += (font->size_px + 1) / 2;
                }
                rendered = 1;
                continue;
            }

            if (do_render)
            {
                int advance = 0;

                if (0 != render_glyph(font, gid, pen_x, baseline_y, dst_x, scale, fg4444, has_bg, dst, dst_w, dst_h, dst_stride, &advance))
                {
                    continue; /* 解析失败跳过，避免污染后续排版 */
                }

                pen_x += advance;
            }
            else
            {
                /* 仅测量：读取 glyph 头部 advance。dst 高度传 0 使绘制循环全部
                 * 越界裁剪，不解引用目标缓冲 */
                int advance = 0;

                if (0 != render_glyph(font, gid, pen_x, baseline_y, dst_x, scale, fg4444, has_bg, NULL, 0, 0, 0, &advance))
                {
                    continue;
                }

                pen_x += advance;
            }

            rendered = 1;
        }
    }

    if (NULL != out_w)
    {
        *out_w = pen_x * scale;
    }

    if (NULL != out_h)
    {
        *out_h = (rendered ? (font->ascent - font->descent) : 0) * scale;
    }

    return 0;
}

int dotfont_measure(const dotfont_t *font, const char *utf8, int scale, int *out_w, int *out_h)
{
    if (NULL == font || NULL == utf8 || scale < 1)
    {
        return -1;
    }

    return layout_line(font, utf8, scale, 0, 0, NULL, 0, 0, 0, 0, 0, 0, out_w, out_h);
}

int dotfont_render(const dotfont_t *font,
                   const char *utf8,
                   int scale,
                   const dotfont_color_t *fg,
                   const dotfont_color_t *bg,
                   uint16_t *dst,
                   int dst_w,
                   int dst_h,
                   int dst_stride,
                   int dst_x,
                   int dst_y,
                   int *out_w,
                   int *out_h)
{
    uint16_t fg4444;
    int has_bg;
    int text_w = 0;
    int line_h;
    int gx;
    int gy;
    int ret;

    if (NULL == font || NULL == utf8 || NULL == dst || scale < 1 || dst_w < 1 || dst_h < 1 || dst_stride < dst_w || NULL == fg)
    {
        return -1;
    }

    fg4444 = to_argb4444(fg);
    has_bg = (NULL != bg);

    /* 先测量，确定背景填充范围 */
    if (0 != layout_line(font, utf8, scale, fg4444, has_bg, dst, dst_w, dst_h, dst_stride, dst_x, dst_y, 0, &text_w, NULL))
    {
        return -1;
    }

    line_h = (font->ascent - font->descent) * scale;

    /* 实心背景：填充文本行矩形（文本宽 x 行高），越界部分自动裁剪 */
    if (has_bg)
    {
        uint16_t bg4444 = to_argb4444(bg);

        for (gy = 0; gy < line_h; gy++)
        {
            int py = dst_y + gy;

            if (py < 0 || py >= dst_h)
            {
                continue;
            }

            for (gx = 0; gx < text_w; gx++)
            {
                int px = dst_x + gx;

                if (px < 0 || px >= dst_w)
                {
                    continue;
                }

                dst[(size_t) py * (size_t) dst_stride + px] = bg4444;
            }
        }
    }

    ret = layout_line(font, utf8, scale, fg4444, has_bg, dst, dst_w, dst_h, dst_stride, dst_x, dst_y, 1, out_w, out_h);

    if (0 == ret && NULL != out_w && *out_w != text_w)
    {
        *out_w = text_w; /* 渲染与测量结果对齐 */
    }

    return ret;
}

/* ---------------- 逐字符布局与渲染（逐字符反色等场景） ---------------- */

int dotfont_iter_begin(dotfont_iter_t *it, const dotfont_t *font, const char *utf8, int scale)
{
    if (NULL == it || NULL == font || NULL == utf8 || scale < 1)
    {
        return -1;
    }

    it->font = font;
    it->p = utf8;
    it->end = utf8 + strlen(utf8);
    it->scale = scale;
    it->pen_x = 0;

    return 0;
}

int dotfont_iter_next(dotfont_iter_t *it, uint32_t *out_codepoint, int *out_pen_x, int *out_advance)
{
    if (NULL == it || NULL == it->font)
    {
        return -1;
    }

    while (it->p < it->end)
    {
        uint32_t cp = utf8_next(&it->p, it->end);

        if ('\n' == cp || '\r' == cp)
        {
            continue; /* 单行接口忽略换行符，与整行排版一致 */
        }

        if (NULL != out_codepoint)
        {
            *out_codepoint = cp;
        }

        if (NULL != out_pen_x)
        {
            *out_pen_x = it->pen_x;
        }

        /* advance：可查字形读头部；缺字/非法字节取半字宽占位（与整行排版一致） */
        if (NULL != out_advance)
        {
            uint32_t gid = 0;

            if (UTF8_REPLACEMENT == cp || 0 != cmap_lookup(it->font, cp, &gid))
            {
                *out_advance = (it->font->size_px + 1) / 2;
            }
            else
            {
                /* 复用 render_glyph 的字形头解析；dst 高度 0 使绘制全部裁剪，
                 * 不会解引用目标缓冲 */
                (void) render_glyph(it->font, gid, 0, 0, 0, it->scale, 0, 0, NULL, 0, 0, 0, out_advance);
            }
        }

        /* 推进笔尖：使用上面已计算的 advance（未请求输出时自行计算） */
        {
            int advance = 0;
            uint32_t gid = 0;

            if (NULL != out_advance)
            {
                advance = *out_advance;
            }
            else if (UTF8_REPLACEMENT == cp || 0 != cmap_lookup(it->font, cp, &gid))
            {
                advance = (it->font->size_px + 1) / 2;
            }
            else
            {
                (void) render_glyph(it->font, gid, 0, 0, 0, it->scale, 0, 0, NULL, 0, 0, 0, &advance);
            }

            it->pen_x += advance;
        }

        return 1;
    }

    return 0;
}

int dotfont_render_char(const dotfont_t *font,
                        uint32_t codepoint,
                        int scale,
                        const dotfont_color_t *fg,
                        uint16_t *dst,
                        int dst_w,
                        int dst_h,
                        int dst_stride,
                        int pen_x,
                        int baseline_y,
                        int *out_advance)
{
    uint32_t gid = 0;
    uint16_t fg4444;
    int advance = 0;

    if (NULL == font || NULL == fg || NULL == dst || scale < 1 || dst_w < 1 || dst_h < 1 || dst_stride < dst_w)
    {
        return -1;
    }

    fg4444 = to_argb4444(fg);

    if (UTF8_REPLACEMENT == codepoint || 0 != cmap_lookup(font, codepoint, &gid))
    {
        advance = render_missing_box(font, pen_x, baseline_y, 0, scale, fg4444, dst, dst_w, dst_h, dst_stride);
    }
    else
    {
        (void) render_glyph(font, gid, pen_x, baseline_y, 0, scale, fg4444, 0, dst, dst_w, dst_h, dst_stride, &advance);
    }

    if (NULL != out_advance)
    {
        *out_advance = advance;
    }

    return 0;
}
