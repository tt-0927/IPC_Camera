/***
 * @FilePath     : image_processor.cpp
 * @Author       : huangjunda
 * @Date         : 2025-06-11 16:11:54
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-06-11 16:14:28
 * @Description  :
 */

#include "image_processor.h"

// class CImageProcessor
// {
// public:
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
                               const char *fontPath)
{
    // 1. 初始化TTF
    if (TTF_Init() < 0)
    {
        fprintf(stderr, "TTF初始化失败: %s\n", SDL_GetError());
        return NULL;
    }

    // 2. 加载字体
    TTF_Font *font = TTF_OpenFont(fontPath, fontSize);
    if (!font)
    {
        fprintf(stderr, "字体加载失败: %s\n", TTF_GetError());
        TTF_Quit();
        return NULL;
    }

    // 3. 创建文字表面
    SDL_Surface *textSurface = NULL;

    // 检查是否需要透明背景
    if (bgColor.a == 0)
    {
        // 透明背景
        textSurface = TTF_RenderUTF8_Blended(font, text, textColor);
    }
    else
    {
        // 实心背景
        textSurface = TTF_RenderUTF8_Shaded(font, text, textColor, bgColor);
    }

    if (!textSurface)
    {
        fprintf(stderr, "文字渲染失败: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        TTF_Quit();
        return NULL;
    }

    // 4. 创建目标尺寸的表面
    SDL_Surface *targetSurface = NULL;

    // 如果没有指定尺寸，使用文字实际尺寸
    if (width == 0 || height == 0)
    {
        width = textSurface->w;
        height = textSurface->h;
    }

    // 5. 转换到指定格式
    targetSurface = SDL_ConvertSurface(textSurface, pixelFormat, 0);

    // 6. 释放临时资源
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
    TTF_Quit();

    return targetSurface;
}

/**
 * @brief 保存表面为BMP文件
 *
 * @param surface 需要保存的表面
 * @param filename 输出文件名
 * @return int 0成功, 非0失败
 */
int SaveSurfaceToBMP(SDL_Surface *surface, const char *filename)
{
    if (!surface || !filename)
        return -1;

    if (SDL_SaveBMP(surface, filename) != 0)
    {
        fprintf(stderr, "BMP保存失败: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}