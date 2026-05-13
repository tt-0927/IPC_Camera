/***
 * @FilePath     : image_processor.h
 * @Author       : huangjunda
 * @Date         : 2025-06-11 16:12:10
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-06-11 16:15:45
 * @Description  :
 */

#pragma once

#include "SDL.h"
#include "SDL_ttf.h"

/**
 * @brief 创建文字渲染表面
 *
 * @param text 需要渲染的文本内容
 * @param width 输出表面的宽度 (0表示自动适应)
 * @param height 输出表面的高度 (0表示自动适应)
 * @param fontSize 字体大小
 * @param textColor 文字颜色 (SDL_Color格式)
 * @param bgColor 背景颜色 (SDL_Color格式)
 * @param pixelFormat 目标颜色格式
 * @param fontPath 字体文件路径
 * @return SDL_Surface* 渲染后的表面，使用完后需要调用SDL_FreeSurface
 */
SDL_Surface *CreateTextSurface(const char *text, int width, int height,
                               int fontSize, SDL_Color textColor,
                               SDL_Color bgColor, SDL_PixelFormat *pixelFormat,
                               const char *fontPath);

/**
 * @brief 保存表面为BMP文件
 *
 * @param surface 需要保存的表面
 * @param filename 输出文件名
 * @return int 0成功, 非0失败
 */
int SaveSurfaceToBMP(SDL_Surface *surface, const char *filename);