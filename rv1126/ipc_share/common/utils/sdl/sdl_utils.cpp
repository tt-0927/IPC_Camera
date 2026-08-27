/**
 * @FilePath     : sdl_utils.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-09 14:57:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-10 09:25:52
 * @Description  : SDL工具类
 */

#include "sdl_utils.h"
#include "dlog.h"
#include <algorithm>
#include <cmath>

/* 对齐宏定义 */
/* 向上对齐 */
#ifndef ALIGN_UP
#define ALIGN_UP(x, align)   (((x) + ((align) -1)) & ~((align) -1))
#endif
/* 向下对齐 */
#ifndef ALIGN_BACK
#define ALIGN_BACK(x, align) ((x) & ~((align) -1))
#endif

CSDLUtils::CSDLUtils() :
    m_bIsSDLInited(false),
    m_bIsTTFInited(false)
{
}

CSDLUtils::~CSDLUtils()
{
    deinit();
}

int CSDLUtils::init()
{
    /* 初始化SDL */
    if (!m_bIsSDLInited)
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            dlog_error("SDL初始化失败: %s", SDL_GetError());
            return -1;
        }
        m_bIsSDLInited = true;
        dlog_info("SDL初始化成功");
    }

    /* 初始化TTF */
    if (!m_bIsTTFInited)
    {
        if (TTF_Init() < 0)
        {
            dlog_error("TTF初始化失败: %s", TTF_GetError());
            SDL_Quit();
            m_bIsSDLInited = false;
            return -1;
        }
        m_bIsTTFInited = true;
        dlog_info("TTF初始化成功");
    }

    return 0;
}

int CSDLUtils::deinit()
{
    /* 清理字体缓存 */
    std::lock_guard<std::mutex> lock(m_mutexFont);
    for (auto &pair : m_mapFontCache)
    {
        if (pair.second)
        {
            TTF_CloseFont(pair.second);
        }
    }
    m_mapFontCache.clear();

    /* 去初始化TTF */
    if (m_bIsTTFInited)
    {
        TTF_Quit();
        m_bIsTTFInited = false;
        dlog_info("TTF去初始化完成");
    }

    /* 去初始化SDL */
    if (m_bIsSDLInited)
    {
        SDL_Quit();
        m_bIsSDLInited = false;
        dlog_info("SDL去初始化完成");
    }

    return 0;
}

SDL_Surface *CSDLUtils::createSurface(int nWidth, int nHeight, Uint32 u32PixelFormat, const Color_S *pBgColor)
{
    /* 2字节对齐 */
    nWidth = ALIGN_UP(nWidth, 2);
    nHeight = ALIGN_UP(nHeight, 2);

    SDL_PixelFormat *pFormat = SDL_AllocFormat(u32PixelFormat);
    if (!pFormat)
    {
        dlog_error("分配像素格式失败: %s", SDL_GetError());
        return nullptr;
    }

    SDL_Surface *pSurface = SDL_CreateRGBSurface(0,
                                                 nWidth,
                                                 nHeight,
                                                 pFormat->BitsPerPixel,
                                                 pFormat->Rmask,
                                                 pFormat->Gmask,
                                                 pFormat->Bmask,
                                                 pFormat->Amask);

    SDL_FreeFormat(pFormat);

    if (!pSurface)
    {
        dlog_error("创建表面失败 (%dx%d): %s", nWidth, nHeight, SDL_GetError());
        return nullptr;
    }

    /* 填充背景色 */
    if (pBgColor)
    {
        Uint32 u32Color = SDL_MapRGBA(pSurface->format, pBgColor->r, pBgColor->g, pBgColor->b, pBgColor->a);
        SDL_FillRect(pSurface, nullptr, u32Color);
    }
    else
    {
        /* 默认透明背景 */
        Uint32 u32Transparent = SDL_MapRGBA(pSurface->format, 0, 0, 0, 0);
        SDL_FillRect(pSurface, nullptr, u32Transparent);
    }

    return pSurface;
}

int CSDLUtils::renderText(SDL_Surface *pSurface, const TextRenderConfig_S &stConfig, const char *pFontPath)
{
    if (!pSurface || !pFontPath || stConfig.strText.empty())
    {
        dlog_error("参数错误");
        return -1;
    }

    /* 加载字体 */
    TTF_Font *pFont = loadFont(pFontPath, stConfig.nFontSize);
    if (!pFont)
    {
        return -1;
    }

    /* 渲染文本 */
    SDL_Color sdlFontColor = toSDLColor(stConfig.stFontColor);
    SDL_Color sdlBackColor = toSDLColor(stConfig.stBackColor);

    SDL_Surface *pTextSurface = nullptr;
    if (stConfig.stBackColor.a == 0)
    {
        /* 透明背景 */
        pTextSurface = TTF_RenderUTF8_Blended(pFont, stConfig.strText.c_str(), sdlFontColor);
    }
    else
    {
        /* 实心背景 */
        pTextSurface = TTF_RenderUTF8_Shaded(pFont, stConfig.strText.c_str(), sdlFontColor, sdlBackColor);
    }

    if (!pTextSurface)
    {
        dlog_error("文本渲染失败: %s", TTF_GetError());
        return -1;
    }

    /* 计算文本位置 */
    int nX = 0, nY = 0;
    calculateTextPosition(pSurface->w,
                          pSurface->h,
                          pTextSurface->w,
                          pTextSurface->h,
                          stConfig.enPosition,
                          stConfig.nOffsetX,
                          stConfig.nOffsetY,
                          nX,
                          nY);

    /* 将文本表面复制到目标表面 */
    SDL_Rect dstRect = { nX, nY, pTextSurface->w, pTextSurface->h };
    SDL_BlitSurface(pTextSurface, nullptr, pSurface, &dstRect);

    SDL_FreeSurface(pTextSurface);
    return 0;
}

int CSDLUtils::measureText(const std::string &strText,
                           int nFontSize,
                           const char *pFontPath,
                           int &nTextWidth,
                           int &nTextHeight)
{
    nTextWidth = 0;
    nTextHeight = 0;
    if (!pFontPath || strText.empty() || nFontSize <= 0)
    {
        return -1;
    }

    TTF_Font *pFont = loadFont(pFontPath, nFontSize);
    if (!pFont)
    {
        return -1;
    }

    if (TTF_SizeUTF8(pFont, strText.c_str(), &nTextWidth, &nTextHeight) != 0)
    {
        dlog_error("测量文本尺寸失败: %s", TTF_GetError());
        nTextWidth = 0;
        nTextHeight = 0;
        return -1;
    }

    return 0;
}

int CSDLUtils::drawRect(SDL_Surface *pSurface, const Rect_S &stRect, const Color_S &stColor, int nThickness)
{
    if (!pSurface || nThickness <= 0)
    {
        return -1;
    }

    /* 边界检查 */
    int nX1 = std::max(0, stRect.nX1);
    int nY1 = std::max(0, stRect.nY1);
    int nX2 = std::min(pSurface->w - 1, stRect.nX2);
    int nY2 = std::min(pSurface->h - 1, stRect.nY2);

    if (nX1 >= nX2 || nY1 >= nY2)
    {
        return -1;
    }

    int nW = nX2 - nX1 + 1;
    int nH = nY2 - nY1 + 1;

    /* 确保厚度不超过矩形大小 */
    int nSafeThickness = std::min(nThickness, std::min(nW / 2, nH / 2));
    if (nSafeThickness <= 0)
        nSafeThickness = 1;

    Uint32 u32Color = SDL_MapRGBA(pSurface->format, stColor.r, stColor.g, stColor.b, stColor.a);

    SDL_Rect rect;
    /* 上边 */
    rect = { nX1, nY1, nW, nSafeThickness };
    SDL_FillRect(pSurface, &rect, u32Color);

    /* 下边 */
    rect = { nX1, nY2 - nSafeThickness + 1, nW, nSafeThickness };
    SDL_FillRect(pSurface, &rect, u32Color);

    /* 左边 */
    rect = { nX1, nY1, nSafeThickness, nH };
    SDL_FillRect(pSurface, &rect, u32Color);

    /* 右边 */
    rect = { nX2 - nSafeThickness + 1, nY1, nSafeThickness, nH };
    SDL_FillRect(pSurface, &rect, u32Color);

    return 0;
}

int CSDLUtils::drawRects(SDL_Surface *pSurface,
                         const std::vector<Rect_S> &vRects,
                         const Color_S &stColor,
                         int nThickness)
{
    if (!pSurface)
    {
        return -1;
    }

    for (const auto &rect : vRects)
    {
        drawRect(pSurface, rect, stColor, nThickness);
    }

    return 0;
}

int CSDLUtils::drawRectWithLabels(SDL_Surface *pSurface,
                                  const Rect_S &stRect,
                                  const std::map<TextPosition_E, std::string> &mapLabels,
                                  const Color_S &stColor,
                                  int nThickness,
                                  const TextRenderConfig_S &stTextConfig,
                                  const char *pFontPath)
{
    if (!pSurface || !pFontPath)
    {
        return -1;
    }

    /* 绘制矩形框 */
    if (drawRect(pSurface, stRect, stColor, nThickness) != 0)
    {
        return -1;
    }

    /* 加载字体 */
    TTF_Font *pFont = loadFont(pFontPath, stTextConfig.nFontSize);
    if (!pFont)
    {
        return -1;
    }

    /* 渲染各个位置的标签 */
    for (const auto &pair : mapLabels)
    {
        if (pair.second.empty())
        {
            continue;
        }

        /* 渲染文本 */
        SDL_Color sdlFontColor = toSDLColor(stTextConfig.stFontColor);
        SDL_Color sdlBackColor = toSDLColor(stTextConfig.stBackColor);

        SDL_Surface *pTextSurface = nullptr;
        if (stTextConfig.stBackColor.a == 0)
        {
            pTextSurface = TTF_RenderUTF8_Blended(pFont, pair.second.c_str(), sdlFontColor);
        }
        else
        {
            pTextSurface = TTF_RenderUTF8_Shaded(pFont, pair.second.c_str(), sdlFontColor, sdlBackColor);
        }

        if (!pTextSurface)
        {
            continue;
        }

        /* 计算标签位置 */
        int nX = 0, nY = 0;
        calculateLabelPosition(stRect, pTextSurface->w, pTextSurface->h, pair.first, 4, nX, nY);

        /* 贴图 */
        SDL_Rect dstRect = { nX, nY, pTextSurface->w, pTextSurface->h };
        SDL_BlitSurface(pTextSurface, nullptr, pSurface, &dstRect);

        SDL_FreeSurface(pTextSurface);
    }

    return 0;
}

int CSDLUtils::saveSurfaceToBMP(SDL_Surface *pSurface, const char *pFilePath)
{
    if (!pSurface || !pFilePath)
    {
        return -1;
    }

    if (SDL_SaveBMP(pSurface, pFilePath) != 0)
    {
        dlog_error("保存BMP失败: %s", SDL_GetError());
        return -1;
    }

    return 0;
}

void CSDLUtils::releaseSurface(SDL_Surface *pSurface)
{
    if (pSurface)
    {
        SDL_FreeSurface(pSurface);
        pSurface = nullptr;
    }
}

SDL_Color CSDLUtils::toSDLColor(const Color_S &stColor)
{
    SDL_Color sdlColor;
    sdlColor.r = stColor.r;
    sdlColor.g = stColor.g;
    sdlColor.b = stColor.b;
    sdlColor.a = stColor.a;
    return sdlColor;
}

CSDLUtils::Color_S CSDLUtils::fromSDLColor(const SDL_Color &sdlColor)
{
    Color_S stColor;
    stColor.r = sdlColor.r;
    stColor.g = sdlColor.g;
    stColor.b = sdlColor.b;
    stColor.a = sdlColor.a;
    return stColor;
}

TTF_Font *CSDLUtils::loadFont(const char *pFontPath, int nFontSize)
{
    /* 生成缓存键 */
    std::string strKey = std::string(pFontPath) + "_" + std::to_string(nFontSize);

    std::lock_guard<std::mutex> lock(m_mutexFont);

    /* 检查缓存 */
    auto it = m_mapFontCache.find(strKey);
    if (it != m_mapFontCache.end())
    {
        return it->second;
    }

    /* 获取最近的可用字体大小 */
    int nActualSize = getNearestFontSize(nFontSize, pFontPath);

    /* 加载字体 */
    TTF_Font *pFont = TTF_OpenFont(pFontPath, nActualSize);
    if (!pFont)
    {
        dlog_error("加载字体失败 (%s, size=%d): %s", pFontPath, nActualSize, TTF_GetError());
        return nullptr;
    }

    /* 缓存字体 */
    m_mapFontCache[strKey] = pFont;
    return pFont;
}

int CSDLUtils::getNearestFontSize(int nRequestSize, const char *pFontPath)
{
    /* 打开字体文件检查是否为位图字体 使用0大小来获取默认face（主要是为了获取face数量） */
    TTF_Font *pFont = TTF_OpenFont(pFontPath, 0);
    if (!pFont)
    {
        return nRequestSize;
    }

    /* 获取字体文件中包含的face数量 */
    long lnFaces = TTF_FontFaces(pFont);
    /* 关闭字体文件 */
    TTF_CloseFont(pFont);

    /* 矢量字体，支持任意大小 */
    if (lnFaces <= 0)
    {
        return nRequestSize;
    }

    /* 位图字体，查找最接近的大小 常见字体大小列表（可根据需要扩展） */
    const int achCommonSizes[] = {
        16, 20, 24, 28, 32, /* 小标题/正文 (16-32) */
        36, 40, 44, 48,     /* 中等标题 (36-48) */
        56, 60, 64, 72, 76, /* 大标题 (56-72) */
        80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128
    }; /* 超大字/海报尺寸 (80-128) */

    int nBestSize = nRequestSize;
    int nMinDiff = nRequestSize;

    /* 位图字体 - 查找最接近的可用大小 */
    for (int nSize : achCommonSizes)
    {
        /* 计算当前大小与请求大小的差异 */
        int nDiff = std::abs(nSize - nRequestSize);
        /* 如果找到更接近的大小 */
        if (nDiff < nMinDiff)
        {
            nMinDiff = nDiff;
            nBestSize = nSize;
        }
    }

    return nBestSize;
}

void CSDLUtils::calculateTextPosition(int nSurfaceWidth,
                                      int nSurfaceHeight,
                                      int nTextWidth,
                                      int nTextHeight,
                                      TextPosition_E enPosition,
                                      int nOffsetX,
                                      int nOffsetY,
                                      int &nOutX,
                                      int &nOutY)
{
    switch (enPosition)
    {
    case TEXT_POS_TOP_LEFT:
        nOutX = nOffsetX;
        nOutY = nOffsetY;
        break;
    case TEXT_POS_TOP_RIGHT:
        nOutX = nSurfaceWidth - nTextWidth - nOffsetX;
        nOutY = nOffsetY;
        break;
    case TEXT_POS_BOTTOM_LEFT:
        nOutX = nOffsetX;
        nOutY = nSurfaceHeight - nTextHeight - nOffsetY;
        break;
    case TEXT_POS_BOTTOM_RIGHT:
        nOutX = nSurfaceWidth - nTextWidth - nOffsetX;
        nOutY = nSurfaceHeight - nTextHeight - nOffsetY;
        break;
    case TEXT_POS_CENTER:
        nOutX = (nSurfaceWidth - nTextWidth) / 2 + nOffsetX;
        nOutY = (nSurfaceHeight - nTextHeight) / 2 + nOffsetY;
        break;
    default:
        nOutX = nOffsetX;
        nOutY = nOffsetY;
        break;
    }
}

void CSDLUtils::calculateLabelPosition(const Rect_S &stRect,
                                       int nTextWidth,
                                       int nTextHeight,
                                       TextPosition_E enPosition,
                                       int nMargin,
                                       int &nOutX,
                                       int &nOutY)
{
    switch (enPosition)
    {
    case TEXT_POS_TOP_LEFT:
        nOutX = stRect.nX1 + nMargin;
        nOutY = stRect.nY1 - nTextHeight - nMargin;
        break;
    case TEXT_POS_TOP_RIGHT:
        nOutX = stRect.nX2 - nTextWidth - nMargin;
        nOutY = stRect.nY1 - nTextHeight - nMargin;
        break;
    case TEXT_POS_BOTTOM_LEFT:
        nOutX = stRect.nX1 + nMargin;
        nOutY = stRect.nY2 + nMargin;
        break;
    case TEXT_POS_BOTTOM_RIGHT:
        nOutX = stRect.nX2 - nTextWidth - nMargin;
        nOutY = stRect.nY2 + nMargin;
        break;
    case TEXT_POS_CENTER:
        nOutX = stRect.nX1 + (stRect.nX2 - stRect.nX1 - nTextWidth) / 2;
        nOutY = stRect.nY1 + (stRect.nY2 - stRect.nY1 - nTextHeight) / 2;
        break;
    default:
        nOutX = stRect.nX1 + nMargin;
        nOutY = stRect.nY1 - nTextHeight - nMargin;
        break;
    }
}
