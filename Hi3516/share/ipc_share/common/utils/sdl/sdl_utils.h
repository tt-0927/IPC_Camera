/**
 * @FilePath     : sdl_utils.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-09 14:57:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-10 08:53:44
 * @Description  : SDL工具类，用于处理基于SDL的文本和图形渲染
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "SDL.h"
#include "SDL_ttf.h"
#include "Singleton.h"

/**
 * @brief   : SDL工具类，提供文本渲染、图形绘制等功能
 * @note    : 使用单例模式，确保SDL资源的全局唯一性
 */
class CSDLUtils : public CSingleton<CSDLUtils>
{
public:
    /* 颜色结构 */
    typedef struct
    {
        uint8_t r; /* 红色分量 (0-255) */
        uint8_t g; /* 绿色分量 (0-255) */
        uint8_t b; /* 蓝色分量 (0-255) */
        uint8_t a; /* 透明度 (0=完全透明, 255=不透明) */
    } Color_S;

    /* 矩形框结构 */
    typedef struct
    {
        int nX1; /* 左上角X坐标 */
        int nY1; /* 左上角Y坐标 */
        int nX2; /* 右下角X坐标 */
        int nY2; /* 右下角Y坐标 */
    } Rect_S;

    /* 文本位置枚举 */
    typedef enum
    {
        TEXT_POS_TOP_LEFT = 0, /* 左上角 */
        TEXT_POS_TOP_RIGHT,    /* 右上角 */
        TEXT_POS_BOTTOM_LEFT,  /* 左下角 */
        TEXT_POS_BOTTOM_RIGHT, /* 右下角 */
        TEXT_POS_CENTER,       /* 中心 */
    } TextPosition_E;

    /* 文本渲染配置 */
    typedef struct
    {
        std::string strText;       /* 文本内容 */
        int nFontSize;             /* 字体大小 */
        Color_S stFontColor;       /* 文字颜色 */
        Color_S stBackColor;       /* 背景颜色 */
        TextPosition_E enPosition; /* 文本位置 */
        int nOffsetX;              /* X轴偏移 */
        int nOffsetY;              /* Y轴偏移 */
    } TextRenderConfig_S;

    virtual ~CSDLUtils();

    /**
     * @brief   : 初始化SDL和TTF库
     * @return   {int} 0：成功，非0：失败
     */
    int init();

    /**
     * @brief   : 去初始化SDL和TTF库
     * @return   {int} 0：成功，非0：失败
     */
    int deinit();

    /**
     * @brief   : 创建指定格式的空白表面
     * @param    {int} nWidth：宽度（字节对齐）
     * @param    {int} nHeight：高度（字节对齐）
     * @param    {Uint32} u32PixelFormat：像素格式（如SDL_PIXELFORMAT_ARGB4444）
     * @param    {Color_S} *pBgColor：背景颜色（NULL表示透明）
     * @return   {SDL_Surface*} 表面指针，使用完需调用releaseSurface释放
     */
    SDL_Surface *createSurface(int nWidth, int nHeight, Uint32 u32PixelFormat, const Color_S *pBgColor = nullptr);

    /**
     * @brief   : 在表面上渲染文本
     * @param    {SDL_Surface} *pSurface：目标表面
     * @param    {TextRenderConfig_S} &stConfig：文本渲染配置
     * @param    {const char} *pFontPath：字体文件路径
     * @return   {int} 0：成功，非0：失败
     */
    int renderText(SDL_Surface *pSurface, const TextRenderConfig_S &stConfig, const char *pFontPath);

    /**
     * @brief   : 测量文本渲染后的像素尺寸（与renderText使用相同字体加载逻辑）
     * @param    {const string} &strText：文本内容
     * @param    {int} nFontSize：字体大小
     * @param    {const char} *pFontPath：字体文件路径
     * @param    {int} &nTextWidth：输出文本宽度
     * @param    {int} &nTextHeight：输出文本高度
     * @return   {int} 0：成功，非0：失败
     */
    int measureText(const std::string &strText,
                    int nFontSize,
                    const char *pFontPath,
                    int &nTextWidth,
                    int &nTextHeight);

    /**
     * @brief   : 在表面上绘制矩形框
     * @param    {SDL_Surface} *pSurface：目标表面
     * @param    {Rect_S} &stRect：矩形坐标
     * @param    {Color_S} &stColor：框颜色
     * @param    {int} nThickness：线条粗细
     * @return   {int} 0：成功，非0：失败
     */
    int drawRect(SDL_Surface *pSurface, const Rect_S &stRect, const Color_S &stColor, int nThickness = 2);

    /**
     * @brief   : 在表面上绘制多个矩形框
     * @param    {SDL_Surface} *pSurface：目标表面
     * @param    {vector<Rect_S>} &vRects：矩形坐标向量
     * @param    {Color_S} &stColor：框颜色
     * @param    {int} nThickness：线条粗细
     * @return   {int} 0：成功，非0：失败
     */
    int drawRects(SDL_Surface *pSurface, const std::vector<Rect_S> &vRects, const Color_S &stColor, int nThickness = 2);

    /**
     * @brief   : 在矩形框周围绘制文本标签
     * @note    : 可在框的四个角显示不同信息（如ID、坐标、时间等）
     * @param    {SDL_Surface} *pSurface：目标表面
     * @param    {Rect_S} &stRect：矩形坐标
     * @param    {map<TextPosition_E, string>} &mapLabels：标签映射（位置->文本）
     * @param    {Color_S} &stColor：框颜色
     * @param    {int} nThickness：框线粗细
     * @param    {TextRenderConfig_S} &stTextConfig：文本渲染配置（字体大小、颜色等）
     * @param    {const char} *pFontPath：字体文件路径
     * @return   {int} 0：成功，非0：失败
     */
    int drawRectWithLabels(SDL_Surface *pSurface,
                           const Rect_S &stRect,
                           const std::map<TextPosition_E, std::string> &mapLabels,
                           const Color_S &stColor,
                           int nThickness,
                           const TextRenderConfig_S &stTextConfig,
                           const char *pFontPath);

    /**
     * @brief   : 保存表面为BMP文件（用于调试）
     * @param    {SDL_Surface} *pSurface：表面指针
     * @param    {const char} *pFilePath：保存路径
     * @return   {int} 0：成功，非0：失败
     */
    int saveSurfaceToBMP(SDL_Surface *pSurface, const char *pFilePath);

    /**
     * @brief   : 释放表面
     * @param    {SDL_Surface} *pSurface：表面指针
     */
    void releaseSurface(SDL_Surface *pSurface);

    /**
     * @brief   : 将颜色结构转换为SDL_Color
     * @param    {Color_S} &stColor：颜色结构
     * @return   {SDL_Color} SDL颜色结构
     */
    static SDL_Color toSDLColor(const Color_S &stColor);

    /**
     * @brief   : 将SDL_Color转换为颜色结构
     * @param    {SDL_Color} &sdlColor：SDL颜色结构
     * @return   {Color_S} 颜色结构
     */
    static Color_S fromSDLColor(const SDL_Color &sdlColor);

private:
    CSDLUtils();
    friend class CSingleton<CSDLUtils>;

    /**
     * @brief   : 加载字体（带缓存）
     * @param    {const char} *pFontPath：字体文件路径
     * @param    {int} nFontSize：字体大小
     * @return   {TTF_Font*} 字体指针
     */
    TTF_Font *loadFont(const char *pFontPath, int nFontSize);

    /**
     * @brief   : 获取最接近的可用字体大小
     * @param    {int} nRequestSize：请求的字体大小
     * @param    {const char} *pFontPath：字体文件路径
     * @return   {int} 实际可用的字体大小
     */
    int getNearestFontSize(int nRequestSize, const char *pFontPath);

    /**
     * @brief   : 计算文本在指定位置的起始坐标
     * @param    {int} nSurfaceWidth：表面宽度
     * @param    {int} nSurfaceHeight：表面高度
     * @param    {int} nTextWidth：文本宽度
     * @param    {int} nTextHeight：文本高度
     * @param    {TextPosition_E} enPosition：文本位置
     * @param    {int} nOffsetX：X轴偏移
     * @param    {int} nOffsetY：Y轴偏移
     * @param    {int} &nOutX：输出X坐标
     * @param    {int} &nOutY：输出Y坐标
     */
    void calculateTextPosition(int nSurfaceWidth,
                               int nSurfaceHeight,
                               int nTextWidth,
                               int nTextHeight,
                               TextPosition_E enPosition,
                               int nOffsetX,
                               int nOffsetY,
                               int &nOutX,
                               int &nOutY);

    /**
     * @brief   : 在矩形框的指定位置计算标签坐标
     * @param    {Rect_S} &stRect：矩形坐标
     * @param    {int} nTextWidth：文本宽度
     * @param    {int} nTextHeight：文本高度
     * @param    {TextPosition_E} enPosition：标签位置
     * @param    {int} nMargin：边距
     * @param    {int} &nOutX：输出X坐标
     * @param    {int} &nOutY：输出Y坐标
     */
    void calculateLabelPosition(const Rect_S &stRect,
                                int nTextWidth,
                                int nTextHeight,
                                TextPosition_E enPosition,
                                int nMargin,
                                int &nOutX,
                                int &nOutY);

private:
    /* SDL初始化标志 */
    bool m_bIsSDLInited;
    /* TTF初始化标志 */
    bool m_bIsTTFInited;
    /* 字体缓存（路径_大小 -> 字体指针） */
    std::map<std::string, TTF_Font *> m_mapFontCache;
    /* 互斥锁（保护字体缓存） */
    std::mutex m_mutexFont;
};
