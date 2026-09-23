/**
 * @file    dotfont.h
 * @brief   轻量级点阵字库加载与文本渲染（LVGL bin 格式，纯 C99，零第三方依赖）
 *
 * 用途：替代 OSD 叠加链路中的 SDL2 + SDL2_ttf + FreeType + TTF 字库方案。
 *       字库文件由 tools/osd_font/gen_font.sh 离线生成（GB2312 全字库 16px 1bpp，
 *       约 251KB），板端只读不写。
 *
 * 设计约束：
 *  - 渲染无内部状态、无锁，dotfont_open 完成后多线程并发调用安全；
 *  - 目标像素格式固定 ARGB4444（uint16：bit15-12=A，11-8=R，7-4=G，3-0=B），
 *    与 Hi3516 RGN overlay 的 OT_PIXEL_FORMAT_ARGB_4444 内存布局一致，
 *    渲染结果可直接经 mppRgn_loadPicture 上屏；
 *  - 通过 scale 参数支持 OSD 的 16/32/48/64 四档字号（= 基准 16px 的 1/2/3/4 倍
 *    最近邻展开），不缩放时传 1；
 *  - 资源上界明确：常驻内存 = 字库文件大小（约 251KB，建议用 mmap 映射按需换入），
 *    渲染缓冲由调用方提供，模块内部不做任何动态分配。
 *
 * 字库二进制格式为 lv_font_conv（v1.5.3）--format bin 的私有布局，字段语义经
 * 板端逐字节实证验证，详见 docs/osd_dot_font/01_点阵字库格式规范.md。
 */

#ifndef DOTFONT_H
#define DOTFONT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** 颜色（各分量 0-255） */
    typedef struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    } dotfont_color_t;

    /** 字库句柄（不透明） */
    typedef struct dotfont dotfont_t;

    /**
     * @brief   打开字库文件：整文件读入堆内存
     * @param   [out] out_font      成功时返回句柄，失败置 NULL
     * @param   [in]  path          字库文件路径
     * @return  0 成功；-1 失败（文件不可读 / 格式非法 / 内存不足）
     * @note    常驻内存 = 文件大小。板端 Flash->RAM 全量常驻时使用本接口。
     */
    int dotfont_open(dotfont_t **out_font, const char *path);

    /**
     * @brief   以 mmap 只读映射方式打开字库文件
     * @param   [out] out_font      成功时返回句柄，失败置 NULL
     * @param   [in]  path          字库文件路径
     * @return  0 成功；-1 失败
     * @note    推荐板端使用：物理内存按访问页换入，未用到的汉字不占 RAM。
     */
    int dotfont_open_mmap(dotfont_t **out_font, const char *path);

    /**
     * @brief   从已有内存块打开字库（如资源已由其他模块加载）
     * @param   [out] out_font      成功时返回句柄，失败置 NULL
     * @param   [in]  data          字库数据首地址
     * @param   [in]  len           数据长度（字节）
     * @param   [in]  take_own      非 0：close 时 free(data)；0：数据由调用方管理
     * @return  0 成功；-1 失败
     */
    int dotfont_open_mem(dotfont_t **out_font, const void *data, size_t len, int take_own);

    /**
     * @brief   关闭字库并释放资源
     * @param   [in] font           dotfont_open* 返回的句柄，允许传 NULL
     */
    void dotfont_close(dotfont_t *font);

    /**
     * @brief   获取字库基准像素字号与行高
     * @param   [in]  font          字库句柄
     * @param   [out] out_px        基准字号（如 16）
     * @param   [out] out_line_h    基准行高 = ascent - descent（如 20）
     * @return  0 成功；-1 参数错误
     */
    int dotfont_get_metrics(const dotfont_t *font, int *out_px, int *out_line_h);

    /**
     * @brief   获取字库基线以上高度（ascent，基准像素）
     * @param   [in]  font          字库句柄
     * @return  ascent（正数）；font 为空返回 -1
     * @note    逐字符渲染时基线 y = 文本行顶 y + ascent * scale，
     *          与 dotfont_render 的整行排版基线一致
     */
    int dotfont_get_ascent(const dotfont_t *font);

    /**
     * @brief   单行 UTF-8 文本的逐字符布局迭代器
     * @note    用于逐字符取笔尖位置与 advance（如逐字符反色：按字符覆盖区域采样
     *          背景亮度决定该字符颜色）。坐标均为基准像素（未乘 scale），
     *          pen_x 相对文本行首。
     */
    typedef struct dotfont_iter
    {
        const dotfont_t *font; /* 内部使用：字库句柄 */
        const char *p;         /* 内部使用：当前解析位置 */
        const char *end;       /* 内部使用：缓冲结束位置 */
        int scale;             /* 内部使用：缩放倍数（仅用于校验） */
        int pen_x;             /* 内部使用：下一个字符笔尖 x（基准像素） */
    } dotfont_iter_t;

    /**
     * @brief   初始化逐字符布局迭代器
     * @param   [out] it            迭代器
     * @param   [in]  font          字库句柄
     * @param   [in]  utf8          UTF-8 文本（单行，'\n'/'\r' 会被跳过）
     * @param   [in]  scale         缩放倍数（>=1，须与后续渲染一致）
     * @return  0 成功；-1 参数错误
     */
    int dotfont_iter_begin(dotfont_iter_t *it, const dotfont_t *font, const char *utf8, int scale);

    /**
     * @brief   取下一个字符的布局信息
     * @param   [in,out] it             迭代器
     * @param   [out] out_codepoint     Unicode 码点
     * @param   [out] out_pen_x         该字符笔尖 x（基准像素，相对行首）
     * @param   [out] out_advance       该字符 advance（基准像素，含缺字占位宽度）
     * @return  1 取到一个字符；0 文本结束；-1 参数错误
     */
    int dotfont_iter_next(dotfont_iter_t *it, uint32_t *out_codepoint, int *out_pen_x, int *out_advance);

    /**
     * @brief   渲染单个字符到指定笔尖位置（ARGB4444）
     * @param   [in]  font          字库句柄
     * @param   [in]  codepoint     Unicode 码点（字库未覆盖时渲染半字宽空心方框占位）
     * @param   [in]  scale         缩放倍数（>=1）
     * @param   [in]  fg            前景颜色
     * @param   [out] dst           目标缓冲
     * @param   [in]  dst_w/h       目标缓冲宽高（像素）
     * @param   [in]  dst_stride    行距（像素，>= dst_w）
     * @param   [in]  pen_x         笔尖 x（基准像素，相对文本行首，内部乘 scale）
     * @param   [in]  baseline_y    基线 y（像素，= 行顶 + ascent*scale，与整行渲染一致）
     * @param   [out] out_advance   该字符 advance（基准像素），允许 NULL
     * @return  0 成功；-1 参数错误
     */
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
                            int *out_advance);

    /**
     * @brief   测量单行 UTF-8 文本在指定缩放下的渲染尺寸
     * @param   [in]  font          字库句柄
     * @param   [in]  utf8          UTF-8 编码文本（单行，不含 '\n'）
     * @param   [in]  scale         整数缩放倍数（>=1，32px 传 2、48px 传 3、64px 传 4）
     * @param   [out] out_w         文本宽度（像素，含字符 advance 累加）
     * @param   [out] out_h         文本高度（= 行高 * scale）
     * @return  0 成功；-1 参数错误
     */
    int dotfont_measure(const dotfont_t *font, const char *utf8, int scale, int *out_w, int *out_h);

    /**
     * @brief   渲染单行 UTF-8 文本到 ARGB4444 缓冲
     *
     * 渲染原点：文本行左上角对齐 (dst_x, dst_y)，行高区域为 line_h*scale。
     * 1bpp 字库无抗锯齿，覆盖像素直接取前景色，非覆盖像素在 bg 非 NULL 时取背景色、
     * bg 为 NULL 时保留目标缓冲原内容（即透明背景）。
     * 越出目标缓冲的部分被裁剪，不产生越界写。
     *
     * @param   [in]  font          字库句柄
     * @param   [in]  utf8          UTF-8 编码文本（单行，不含 '\n'）
     * @param   [in]  scale         整数缩放倍数（>=1）
     * @param   [in]  fg            前景（文字）颜色
     * @param   [in]  bg            背景色，NULL 表示透明背景
     * @param   [out] dst           目标缓冲（ARGB4444，uint16 数组）
     * @param   [in]  dst_w         目标缓冲宽（像素）
     * @param   [in]  dst_h         目标缓冲高（像素）
     * @param   [in]  dst_stride    目标缓冲行距（像素，>= dst_w）
     * @param   [in]  dst_x         文本行左上角 x
     * @param   [in]  dst_y         文本行左上角 y
     * @param   [out] out_w         渲染文本实际宽度（像素），允许 NULL
     * @param   [out] out_h         渲染文本实际高度（像素），允许 NULL
     * @return  0 成功；-1 参数错误
     */
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
                       int *out_h);

#ifdef __cplusplus
}
#endif

#endif /* DOTFONT_H */
