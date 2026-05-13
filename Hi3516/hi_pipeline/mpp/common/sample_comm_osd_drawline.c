/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include <stdio.h>
#include "sample_comm.h"

#define OSD_DRAWLINE_NUM_2  2
#define OSD_DRAWLINE_NUM_4  4
#define OSD_DRAWLINE_NUM_6  6
#define OSD_DRAWLINE_NUM_8  8
#define OSD_DRAWLINE_NUM_16 16

static td_s32 sample_comm_osd_drawline_check_param(const ot_size *size, const sample_osd_line *line)
{
    if (line->thick == 0) {
        sample_print("thick should > 0!\n");
        return TD_FAILURE;
    }
    if ((line->is_display != TD_TRUE) && (line->is_display != TD_FALSE)) {
        sample_print("is_display(%d) is illegal!\n", line->is_display);
        return TD_FAILURE;
    }

    if (line->point1.x < 0 || line->point2.x < 0 || line->point1.y < 0 || line->point2.y < 0) {
        sample_print("The coordinates of the points should >= 0!\n");
        return TD_FAILURE;
    }
    if ((td_u32)line->point1.x >= size->width || (td_u32)line->point2.x >= size->width) {
        sample_print("Coordinate x should < width!\n");
        return TD_FAILURE;
    }
    if ((td_u32)line->point1.y >= size->height || (td_u32)line->point2.y >= size->height) {
        sample_print("Coordinate y should < height!\n");
        return TD_FAILURE;
    }
    if ((line->point1.x == line->point2.x) && (line->point1.y == line->point2.y)) {
        sample_print("point1 is valued to point2!\n");
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}

static td_s32 sample_comm_osd_drawline_check_align_by_pixel_format(ot_pixel_format pixel_format,
    const ot_size *size, const sample_osd_line *line)
{
    td_u32 align_num = 1;
    if (pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT2_2X2 || pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT4_2X2) {
        align_num = OSD_DRAWLINE_NUM_2;
    } else if (pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT2_4X4 || pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT4_4X4) {
        align_num = OSD_DRAWLINE_NUM_4;
    }

    if (align_num == 1) {
        return TD_SUCCESS;
    }

    if (size->width % align_num != 0 || size->height % align_num != 0) {
        sample_print("width(%u) height(%u) is not align %u\n", size->width, size->height, align_num);
        return TD_FAILURE;
    }

    if (line->point1.x % align_num != 0 || line->point1.y % align_num != 0 ||
        line->point2.x % align_num != 0 || line->point2.y % align_num != 0) {
        sample_print("x1(%d) y1(%d) x2(%d) y2(%d) is not align %u\n", line->point1.x, line->point1.y,
            line->point2.x, line->point2.y, align_num);
        return TD_FAILURE;
    }

    if (line->thick % align_num != 0) {
        sample_print("line thick(%u) is not align %u\n", line->thick, align_num);
        return TD_FAILURE;
    }

    return TD_SUCCESS;
}

static td_void sample_comm_osd_drawline_create_line_point(sample_line *line_point, const sample_osd_line *line,
    td_bool check_point_x)
{
    td_bool swap_point;

    if (check_point_x) {
        swap_point = (line->point1.x >= line->point2.x);
    } else {
        swap_point = (line->point1.y >= line->point2.y);
    }

    if (swap_point) {
        line_point->start_x = line->point2.x;
        line_point->start_y = line->point2.y;
        line_point->end_x = line->point1.x;
        line_point->end_y = line->point1.y;
    } else {
        line_point->start_x = line->point1.x;
        line_point->start_y = line->point1.y;
        line_point->end_x = line->point2.x;
        line_point->end_y = line->point2.y;
    }
}

static td_u32 sample_get_offset_pixel_by_set_bmp(td_s32 start_x, td_u32 width, td_s32 height,
    ot_pixel_format pixel_format)
{
    switch (pixel_format) {
        case OT_PIXEL_FORMAT_ARGB_1555:
        case OT_PIXEL_FORMAT_ARGB_4444:
        case OT_PIXEL_FORMAT_ARGB_CLUT2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4:
            return (start_x + width * height);
        case OT_PIXEL_FORMAT_ARGB_CLUT2_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_2X2:
            return (start_x + width * height / OSD_DRAWLINE_NUM_2);
        case OT_PIXEL_FORMAT_ARGB_CLUT2_4X4:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_4X4:
            return (start_x + width * height / OSD_DRAWLINE_NUM_4);
        default:
            sample_print("error pixel format!\n");
    }
    return 0;
}

static td_u32 sample_get_offset_pixel_by_get_canvas(td_s32 start_x, td_u32 stride, td_s32 height,
    ot_pixel_format pixel_format)
{
    switch (pixel_format) {
        case OT_PIXEL_FORMAT_ARGB_1555:
        case OT_PIXEL_FORMAT_ARGB_4444:
            return (start_x + stride * height / OSD_DRAWLINE_NUM_2);
        case OT_PIXEL_FORMAT_ARGB_CLUT2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_4X4:
            return (start_x + stride * height * OSD_DRAWLINE_NUM_4);
        case OT_PIXEL_FORMAT_ARGB_CLUT4:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_4X4:
            return (start_x + stride * height * OSD_DRAWLINE_NUM_2);
        default:
            sample_print("error pixel format!\n");
    }
    return 0;
}

static td_u32 sample_get_offset_byte(sample_drawline_param *drawline_param, td_s32 start_x, td_s32 h)
{
    td_u32 pixel_num;
    ot_pixel_format pixel_format = drawline_param->pixel_format;
    ot_size *size = &drawline_param->size;

    if (drawline_param->is_set_bmp) {
        pixel_num = sample_get_offset_pixel_by_set_bmp(start_x, size->width, h, pixel_format);
    } else {
        pixel_num = sample_get_offset_pixel_by_get_canvas(start_x, drawline_param->stride, h, pixel_format);
    }

    switch (pixel_format) {
        case OT_PIXEL_FORMAT_ARGB_1555:
        case OT_PIXEL_FORMAT_ARGB_4444:
            return pixel_num * OSD_DRAWLINE_NUM_2;
        case OT_PIXEL_FORMAT_ARGB_CLUT2:
            return pixel_num / OSD_DRAWLINE_NUM_4;
        case OT_PIXEL_FORMAT_ARGB_CLUT2_2X2:
            return pixel_num / OSD_DRAWLINE_NUM_8;
        case OT_PIXEL_FORMAT_ARGB_CLUT2_4X4:
            return pixel_num / OSD_DRAWLINE_NUM_16;
        case OT_PIXEL_FORMAT_ARGB_CLUT4:
            return pixel_num / OSD_DRAWLINE_NUM_2;
        case OT_PIXEL_FORMAT_ARGB_CLUT4_2X2:
            return pixel_num / OSD_DRAWLINE_NUM_4;
        case OT_PIXEL_FORMAT_ARGB_CLUT4_4X4:
            return pixel_num / OSD_DRAWLINE_NUM_8;
        default:
            sample_print("error pixel format!\n");
    }
    return 0;
}

static td_u32 sample_get_pixel_step(ot_pixel_format pixel_format)
{
    if (pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT2_2X2 || pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT4_2X2) {
        return OSD_DRAWLINE_NUM_2;
    } else if (pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT2_4X4 || pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT4_4X4) {
        return OSD_DRAWLINE_NUM_4;
    }
    return 1;
}

static td_void sample_osd_drawline_set_color_value(td_void *vir_addr, td_u32 color, td_bool is_display,
    td_bool is_clut)
{
    td_u8 color_u8;
    td_u16 color_u16;

    if (is_display) {
        if (is_clut) {
            color_u8 = (td_u8)color;
            *((td_u8 *)vir_addr) |= color_u8;
        } else {
            color_u16 = (td_u16)color;
            *((td_u16 *)vir_addr) |= color_u16;
        }
    } else {
        if (is_clut) {
            color_u8 = (td_u8)color;
            *((td_u8 *)vir_addr) &= ~color_u8;
        } else {
            color_u16 = (td_u16)color;
            *((td_u16 *)vir_addr) &= ~color_u16;
        }
    }
}

static td_void sample_osd_drawline_vertical_argb1555_4444(sample_drawline_param *drawline_param, sample_line *line)
{
    td_s32 i;
    td_u16 *temp = TD_NULL;
    td_u32 cnt;
    td_u32 offset_byte;
    td_u8 *virt_addr = drawline_param->data;
    sample_osd_line *osd_line = drawline_param->line;

    for (i = line->start_y; i < line->end_y; i++) {
        offset_byte = sample_get_offset_byte(drawline_param, line->start_x, i);
        temp = (td_u16 *)(virt_addr + offset_byte);
        for (cnt = 0; cnt < osd_line->thick; cnt++) {
            sample_osd_drawline_set_color_value(temp, osd_line->color, osd_line->is_display, TD_FALSE);
            temp++;
        }
    }
}

static td_void sample_get_multiple_and_remainder(ot_pixel_format pixel_format, td_u32 *multiple, td_u32 *remainder)
{
    if (pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT2 || pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT2_2X2 ||
        pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT2_4X4) {
        *multiple = OSD_DRAWLINE_NUM_2;
        *remainder = OSD_DRAWLINE_NUM_4;
    } else if (pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT4 || pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT4_2X2 ||
        pixel_format == OT_PIXEL_FORMAT_ARGB_CLUT4_4X4) {
        *multiple = OSD_DRAWLINE_NUM_4;
        *remainder = OSD_DRAWLINE_NUM_2;
    } else {
        *multiple = 1;
        *remainder = 1;
    }
}

static td_void sample_comm_osd_drawline_clut_vertical(sample_drawline_param *drawline_param, sample_line *line)
{
    td_s32 i;
    td_s32 step;
    td_u8 *temp = TD_NULL;
    td_u32 cnt;
    td_u32 offset_byte;
    td_u8 point_in_bit;
    td_u8 *virt_addr = drawline_param->data;
    sample_osd_line *osd_line = drawline_param->line;
    td_u32 multiple;
    td_u32 remainder;
    td_u32 color;

    step = (td_s32)sample_get_pixel_step(drawline_param->pixel_format);
    sample_get_multiple_and_remainder(drawline_param->pixel_format, &multiple, &remainder);

    for (i = line->start_y; i < line->end_y; i += step) {
        offset_byte = sample_get_offset_byte(drawline_param, line->start_x, i);
        temp = virt_addr + offset_byte;
        point_in_bit = (td_u8)((line->start_x / step) % (td_s32)remainder);

        for (cnt = 0; cnt < osd_line->thick; cnt += (td_u32)step) {
            color = (osd_line->color << (multiple * point_in_bit));
            sample_osd_drawline_set_color_value(temp, color, osd_line->is_display, TD_TRUE);
            point_in_bit++;
            if (point_in_bit % remainder == 0) {
                temp++;
                point_in_bit = 0;
            }
        }
    }
}

static td_void sample_osd_drawline_horizontal_argb1555_4444(sample_drawline_param *drawline_param, sample_line *line)
{
    td_u32 i;
    td_u16 *temp = TD_NULL;
    td_u32 x1_in_bytes, x2_in_bytes;
    td_u8 *virt_addr = drawline_param->data;
    sample_osd_line *osd_line = drawline_param->line;

    x1_in_bytes = sample_get_offset_byte(drawline_param, line->start_x, line->start_y);
    x2_in_bytes = sample_get_offset_byte(drawline_param, line->end_x, line->end_y);

    for (i = x1_in_bytes; i <= x2_in_bytes; i += OSD_DRAWLINE_NUM_2) {
        temp = (td_u16 *)(virt_addr + i);
        sample_osd_drawline_set_color_value(temp, osd_line->color, osd_line->is_display, TD_FALSE);
    }
}

static td_u32 sample_comm_get_start_x_bytes(sample_drawline_param *drawline_param, sample_line *line)
{
    td_u32 pix_num;
    td_u32 div_up_num;
    ot_pixel_format pixel_format = drawline_param->pixel_format;

    if (drawline_param->is_set_bmp) {
        pix_num = sample_get_offset_pixel_by_set_bmp(line->start_x, drawline_param->size.width, line->start_y,
            pixel_format);
    } else {
        pix_num = sample_get_offset_pixel_by_get_canvas(line->start_x, drawline_param->stride, line->start_y,
            pixel_format);
    }

    switch (pixel_format) {
        case OT_PIXEL_FORMAT_ARGB_CLUT4:
            div_up_num = OSD_DRAWLINE_NUM_2;
            break;
        case OT_PIXEL_FORMAT_ARGB_CLUT2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_2X2:
            div_up_num = OSD_DRAWLINE_NUM_4;
            break;
        case OT_PIXEL_FORMAT_ARGB_CLUT2_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_4X4:
            div_up_num = OSD_DRAWLINE_NUM_8;
            break;
        case OT_PIXEL_FORMAT_ARGB_CLUT2_4X4:
            div_up_num = OSD_DRAWLINE_NUM_16;
            break;
        default:
            div_up_num = 1;
            break;
    }

    return OT_DIV_UP(pix_num, div_up_num);
}

static td_u8 sample_osd_drawline_get_point_in_bit(ot_pixel_format pixel_format, td_s32 x)
{
    td_u32 step;
    td_u32 remainder;

    step = sample_get_pixel_step(pixel_format);
    switch (pixel_format) {
        case OT_PIXEL_FORMAT_ARGB_CLUT2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_4X4:
            remainder = OSD_DRAWLINE_NUM_4;
            break;
        case OT_PIXEL_FORMAT_ARGB_CLUT4:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_4X4:
            remainder = OSD_DRAWLINE_NUM_2;
            break;
        default:
            remainder = 1;
            break;
    }
    return (td_u8)((x / step) % remainder);
}

static td_u32 sample_osd_drawline_get_startx_color_by_bit(td_u8 point_in_bit, ot_pixel_format pixel_format,
    td_u32 color)
{
    switch (pixel_format) {
        case OT_PIXEL_FORMAT_ARGB_CLUT2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_4X4:
            if (point_in_bit == 1) {
                return ((color << OSD_DRAWLINE_NUM_6) | (color << OSD_DRAWLINE_NUM_4) | (color << OSD_DRAWLINE_NUM_2));
            } else if (point_in_bit == 2) { /* 2: 2bit */
                return ((color << OSD_DRAWLINE_NUM_6) | (color << OSD_DRAWLINE_NUM_4));
            } else if (point_in_bit == 3) { /* 3: 3bit */
                return (color << OSD_DRAWLINE_NUM_6);
            }
            return 0;
        case OT_PIXEL_FORMAT_ARGB_CLUT4:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_4X4:
            return ((point_in_bit == 1) ? (color << OSD_DRAWLINE_NUM_4) : 0);
        default:
            return 0;
    }
}

static td_u32 sample_osd_drawline_get_endx_color_by_bit(td_u8 point_in_bit, ot_pixel_format pixel_format, td_u32 color)
{
    switch (pixel_format) {
        case OT_PIXEL_FORMAT_ARGB_CLUT2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_4X4:
            if (point_in_bit == 1) {
                return color;
            } else if (point_in_bit == 2) { /* 2: 2bit */
                return ((color << OSD_DRAWLINE_NUM_2) | color);
            } else if (point_in_bit == 3) { /* 3: 3bit */
                return ((color << OSD_DRAWLINE_NUM_4) | (color << OSD_DRAWLINE_NUM_2) | color);
            }
            return 0;
        case OT_PIXEL_FORMAT_ARGB_CLUT4:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_4X4:
            return ((point_in_bit == 1) ? color : 0);
        default:
            return 0;
    }
}

static td_u32 sample_osd_drawline_get_clut_color(ot_pixel_format pixel_format, td_u32 color)
{
    switch (pixel_format) {
        case OT_PIXEL_FORMAT_ARGB_CLUT2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT2_4X4:
            return ((color << OSD_DRAWLINE_NUM_6) | (color << OSD_DRAWLINE_NUM_4) |
                (color << OSD_DRAWLINE_NUM_2) | color);
        case OT_PIXEL_FORMAT_ARGB_CLUT4:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_2X2:
        case OT_PIXEL_FORMAT_ARGB_CLUT4_4X4:
            return ((color << OSD_DRAWLINE_NUM_4) | color);
        default:
            return 0;
    }
}

static td_void sample_comm_osd_drawline_clut_horizontal(sample_drawline_param *drawline_param, sample_line *line)
{
    td_u32 i;
    td_u8 *temp = TD_NULL;
    td_u8 point_in_bit;
    td_u32 x1_in_bytes, x2_in_bytes;
    td_u8 *virt_addr = drawline_param->data;
    sample_osd_line *osd_line = drawline_param->line;
    td_u32 width = drawline_param->size.width;
    td_u32 color;
    ot_pixel_format pixel_format = drawline_param->pixel_format;

    point_in_bit = sample_osd_drawline_get_point_in_bit(pixel_format, line->start_x);
    x1_in_bytes = sample_comm_get_start_x_bytes(drawline_param, line);
    color = sample_osd_drawline_get_startx_color_by_bit(point_in_bit, pixel_format, osd_line->color);
    temp = virt_addr + sample_get_offset_byte(drawline_param, line->start_x, line->start_y);
    sample_osd_drawline_set_color_value(temp, color, osd_line->is_display, TD_TRUE);

    point_in_bit = sample_osd_drawline_get_point_in_bit(pixel_format, line->end_x);
    if (line->end_x + osd_line->thick >= width) {
        x2_in_bytes = sample_get_offset_byte(drawline_param, (td_s32)width, line->end_y);
    } else {
        x2_in_bytes = sample_get_offset_byte(drawline_param, line->end_x + (td_s32)osd_line->thick, line->end_y);
    }
    color = sample_osd_drawline_get_endx_color_by_bit(point_in_bit, pixel_format, osd_line->color);
    temp = virt_addr + x2_in_bytes;
    sample_osd_drawline_set_color_value(temp, color, osd_line->is_display, TD_TRUE);

    color = sample_osd_drawline_get_clut_color(pixel_format, osd_line->color);
    for (i = x1_in_bytes; i < x2_in_bytes; i++) {
        temp = virt_addr + i;
        sample_osd_drawline_set_color_value(temp, color, osd_line->is_display, TD_TRUE);
    }
}

static td_void sample_comm_osd_drawline_slope_argb1555_4444(sample_drawline_param *drawline_param,
    td_u32 point_x, td_s32 y)
{
    td_u32 i;
    td_u32 offset_byte;
    td_u16 *temp;
    sample_osd_line *osd_line = drawline_param->line;
    td_u32 width = drawline_param->size.width;
    
    offset_byte = sample_get_offset_byte(drawline_param, (td_s32)point_x, y);
    temp = (td_u16 *)(drawline_param->data + offset_byte);
    for (i = 0; i < osd_line->thick; i++) {
        if (point_x + i >= width) {
            break;
        }
        sample_osd_drawline_set_color_value(temp, osd_line->color, osd_line->is_display, TD_FALSE);
        temp++;
    }
}

static td_void sample_comm_osd_drawline_slope_clut(sample_drawline_param *drawline_param, td_u32 point_x, td_s32 y)
{
    td_s32 ii;
    td_u32 step;
    td_u8 point_in_bit;
    td_u8 *temp = TD_NULL;
    td_u32 cnt;
    td_u32 offset_byte;
    td_u32 width = drawline_param->size.width;
    sample_osd_line *osd_line = drawline_param->line;
    td_u32 multiple;
    td_u32 remainder;
    td_u32 color;

    step = sample_get_pixel_step(drawline_param->pixel_format);
    sample_get_multiple_and_remainder(drawline_param->pixel_format, &multiple, &remainder);

    ii = OT_ALIGN_DOWN(y, (td_s32)step);
    offset_byte = sample_get_offset_byte(drawline_param, (td_s32)point_x, ii);
    temp = drawline_param->data + offset_byte;
    point_in_bit = (td_u8)((point_x / step) % remainder);
    for (cnt = 0; cnt < osd_line->thick; cnt += step) {
        if (point_x + cnt >= width) {
            break;
        }
        color = (osd_line->color << (multiple * point_in_bit));
        sample_osd_drawline_set_color_value(temp, color, osd_line->is_display, TD_TRUE);
        point_in_bit++;
        if (point_in_bit % remainder == 0) {
            temp++;
            point_in_bit = 0;
        }
    }
}

static td_s32 sample_comm_osd_drawlinne_get_u(td_s32 d)
{
    td_s32 t_d;

    t_d = (d > 0 ? 1 : 0);
    return (t_d * 2 - 1); /* 2: double */
}

static td_void sample_comm_do_osd_drawline_low_slope(sample_drawline_param *drawline_param, sample_line *line)
{
    td_s32 ux;
    td_s32 uy;
    td_s32 y;
    td_s32 i;
    td_s32 eps = 0;
    td_s32 dx = line->end_x - line->start_x;
    td_s32 dy = line->end_y - line->start_y;
    ot_pixel_format pixel_format = drawline_param->pixel_format;

    ux = sample_comm_osd_drawlinne_get_u(dx);
    uy = sample_comm_osd_drawlinne_get_u(dy);

    dx = (dx >= 0 ? dx : (-dx));
    dy = (dy >= 0 ? dy : (-dy));
    y = line->start_y;

    for (i = line->start_x; i <= line->end_x; i += ux) {
        if (pixel_format == OT_PIXEL_FORMAT_ARGB_1555 || pixel_format == OT_PIXEL_FORMAT_ARGB_4444) {
            sample_comm_osd_drawline_slope_argb1555_4444(drawline_param, (td_u32)i, y);
        } else {
            sample_comm_osd_drawline_slope_clut(drawline_param, (td_u32)i, y);
        }
        eps += dy;
        if ((eps * OSD_DRAWLINE_NUM_2) >= dx) {
            y += uy;
            eps -= dx;
        }
    }
}

static td_void sample_comm_osd_drawline_high_slope(sample_drawline_param *drawline_param, sample_line *line)
{
    td_s32 ux;
    td_s32 uy;
    td_s32 x;
    td_s32 i;
    td_s32 dx = line->end_x - line->start_x;
    td_s32 dy = line->end_y - line->start_y;
    td_s32 eps = 0;
    ot_pixel_format pixel_format = drawline_param->pixel_format;

    ux = sample_comm_osd_drawlinne_get_u(dx);
    uy = sample_comm_osd_drawlinne_get_u(dy);

    x = line->start_x;
    dx = (dx >= 0 ? dx : (-dx));
    dy = (dy >= 0 ? dy : (-dy));

    for (i = line->start_y; i <= line->end_y; i += uy) {
        if (pixel_format == OT_PIXEL_FORMAT_ARGB_1555 || pixel_format == OT_PIXEL_FORMAT_ARGB_4444) {
            sample_comm_osd_drawline_slope_argb1555_4444(drawline_param, (td_u32)x, i);
        } else {
            sample_comm_osd_drawline_slope_clut(drawline_param, (td_u32)x, i);
        }
        eps += dx;
        if ((eps * OSD_DRAWLINE_NUM_2) >= dy) {
            x += ux;
            eps -= dy;
        }
    }
}

static td_void sample_comm_osd_drawline_vertical(sample_drawline_param *drawline_param, sample_line *line)
{
    ot_pixel_format pixel_format = drawline_param->pixel_format;
    if (pixel_format == OT_PIXEL_FORMAT_ARGB_1555 || pixel_format == OT_PIXEL_FORMAT_ARGB_4444) {
        sample_osd_drawline_vertical_argb1555_4444(drawline_param, line);
    } else {
        sample_comm_osd_drawline_clut_vertical(drawline_param, line);
    }
}

static td_void sample_comm_do_osd_drawline_horizontal(sample_drawline_param *drawline_param, sample_line *line)
{
    ot_pixel_format pixel_format = drawline_param->pixel_format;
    if (pixel_format == OT_PIXEL_FORMAT_ARGB_1555 || pixel_format == OT_PIXEL_FORMAT_ARGB_4444) {
        sample_osd_drawline_horizontal_argb1555_4444(drawline_param, line);
    } else {
        sample_comm_osd_drawline_clut_horizontal(drawline_param, line);
    }
}

static td_void sample_comm_osd_drawline_horizontal(sample_drawline_param *drawline_param, sample_line *line)
{
    td_u32 i;
    td_u32 step;
    sample_line osd_line = {0};

    (td_void)memcpy_s(&osd_line, sizeof(sample_line), line, sizeof(sample_line));

    step = sample_get_pixel_step(drawline_param->pixel_format);

    for (i = 0; i < drawline_param->line->thick; i += step) {
        sample_comm_do_osd_drawline_horizontal(drawline_param, &osd_line);
        osd_line.start_y += (td_s32)step;
        osd_line.end_y += (td_s32)step;
    }
}

static td_void sample_comm_osd_drawline_low_slope(sample_drawline_param *drawline_param, sample_line *line)
{
    td_u32 i;
    td_u32 step;
    sample_line osd_line = {0};

    (td_void)memcpy_s(&osd_line, sizeof(sample_line), line, sizeof(sample_line));

    step = sample_get_pixel_step(drawline_param->pixel_format);

    for (i = 0; i < drawline_param->line->thick; i += step) {
        sample_comm_do_osd_drawline_low_slope(drawline_param, &osd_line);
        osd_line.start_y += (td_s32)step;
        osd_line.end_y += (td_s32)step;
    }
}

static td_s32 sample_comm_do_osd_drawline(sample_drawline_param *drawline_param)
{
    sample_line line;
    sample_osd_line *dline = drawline_param->line;
    td_bool is_hor;
    td_bool is_ver;
    td_bool is_low_slope;

    is_hor = (dline->point1.x != dline->point2.x) && (dline->point1.y == dline->point2.y);
    is_ver = (dline->point1.x == dline->point2.x) && (dline->point1.y != dline->point2.y);
    is_low_slope = (ABS(dline->point2.y - dline->point1.y) < ABS(dline->point2.x - dline->point1.x));
    sample_comm_osd_drawline_create_line_point(&line, drawline_param->line, (is_hor || is_low_slope));

    if (is_ver && (line.start_x + dline->thick > drawline_param->size.width)) {
        sample_print("(x + thick) should <= width!\n");
        return TD_FAILURE;
    }

    if (is_hor) {
        sample_comm_osd_drawline_horizontal(drawline_param, &line);
    } else if (is_ver) {
        sample_comm_osd_drawline_vertical(drawline_param, &line);
    } else if (is_low_slope) {
        sample_comm_osd_drawline_low_slope(drawline_param, &line);
    } else {
        sample_comm_osd_drawline_high_slope(drawline_param, &line);
    }
    return TD_SUCCESS;
}

td_s32 sample_comm_osd_drawline(sample_drawline_param *drawline_param)
{
    td_s32 ret;

    if (drawline_param == TD_NULL) {
        sample_print("drawline param is not allow null!\n");
        return TD_FAILURE;
    }

    if (drawline_param->is_set_bmp == TD_FALSE && drawline_param->stride == 0) {
        sample_print("stride is zero\n");
        return TD_FAILURE;
    }

    ret = sample_comm_osd_drawline_check_param(&drawline_param->size, drawline_param->line);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    ret = sample_comm_osd_drawline_check_align_by_pixel_format(drawline_param->pixel_format,
        &drawline_param->size, drawline_param->line);
    if (ret != TD_SUCCESS) {
        return ret;
    }

    return sample_comm_do_osd_drawline(drawline_param);
}