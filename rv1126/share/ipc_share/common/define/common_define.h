/**
 * @FilePath     : common_define.h
 * @Author       : huangjunda
 * @Date         : 2025-03-26 14:22:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 09:33:35
 * @Description  : 公共定义
 */
#pragma once

#include <functional>
#include <ctime>
#include <chrono>
#include <iomanip>
#include "Singleton.h"
namespace Common
{
    typedef enum
    {
        REQUESTER_NONE = 0x0,
        REQUESTER_WEB = 0x1,
        REQUESTER_QT = 0x2,
        REQUESTER_STREAM = 0x3,
        REQUESTER_REPLAY = 0x4,
        REQUESTER_UPGRADE = 0x5,
        REQUESTER_SYS_SET = 0x6,
        REQUESTER_AI_APP = 0x7,
        REQUESTER_RECORD = 0x8,
        REQUESTER_REGISTER = 0x9,
        REQUESTER_SSL_WEB = 0x10,
        REQUESTER_PLATFROM = 0x11,
        REQUESTER_OPERATION = 0x12
    } Requester_E;

    using StatusCallback = std::function<void(Requester_E, bool)>;

    typedef struct PageInfo
    {
        /* 当前页号 -1 获取全部*/
        int nCurPage = -1;
        /* 页数据量 */
        int nPageSize = 0;
        /* 总数量 */
        int nDataTotal = 0;
        /* 总页数 */
        int nPageTotal = 0;
    } PageInfo_S;

    /// @brief 时间数据结构
    typedef struct _Time_S_
    {
        unsigned int nHour;     /* 时 [0,23] */
        unsigned int nMinute;   /* 分 [0,59] */
        unsigned int nSecond;   /* 秒 [0,59] */
        unsigned int nMilliSec; /* 毫秒 [0,999] */
        /* 重载默认构造函数 */
        _Time_S_()
            : nHour(0),
              nMinute(0),
              nSecond(0),
              nMilliSec(0)
        {
        }
        /* 重载赋值运算符 */
        _Time_S_ &operator=(const _Time_S_ &x)
        {
            if (this != &x)
            {
                nHour = x.nHour;
                nMinute = x.nMinute;
                nSecond = x.nSecond;
                nMilliSec = x.nMilliSec;
            }
            return *this;
        }
    } Time_S;

    /// @brief 起止时间段数据结构
    typedef struct _SchedTime_S_
    {
        Time_S stStart; /* 开始时间 */
        Time_S stStop;  /* 结束时间 */
        /* 重载默认构造函数 */
        _SchedTime_S_()
        {
            stStart.nHour = 0;   // 默认开始 00:00
            stStart.nMinute = 0;
            stStart.nSecond = 0;
            
            stStop.nHour = 24;    // 默认结束 24:00
            stStop.nMinute = 0;
            stStop.nSecond = 0;
        }
        /* 重载运算符 */
        _SchedTime_S_ &operator=(const _SchedTime_S_ &x)
        {
            if (this != &x)
            {
                stStart = x.stStart;
                stStop = x.stStop;
            }
            return *this;
        }
    } SchedTime_S;

    typedef enum 
    {
        EVENT_CROSS = 1,   /*越界*/ 
        EVENT_INTRU,       /*入侵*/ 
        EVENT_ENTRY,       /*进入*/ 
        EVENT_EXIT,        /*离开*/ 
        EVENT_FACE_DET,    /*人脸检测*/ 
        EVENT_FACE_FEA     /*人脸比对*/ 
    } EventType_E;


    /// @brief 坐标数据结构
    typedef struct _Pos_S_
    {
        int nX; /* X坐标 */
        int nY; /* Y坐标 */
        /* 重载默认构造函数 */
        _Pos_S_()
            : nX(0),
              nY(0)
        {
        }
        /* 重载赋值运算符 */
        _Pos_S_ &operator=(const _Pos_S_ &x)
        {
            if (this != &x)
            {
                nX = x.nX;
                nY = x.nY;
            }
            return *this;
        }

        /**
         * @brief   : 分辨率坐标转换函数
         * @param    {int} srcWidth：原始分辨率宽度
         * @param    {int} srcHeight：原始分辨率高度
         * @param    {int} dstWidth：目标分辨率宽度
         * @param    {int} dstHeight：目标分辨率高度
         * @return   {bool} 转换是否成功
         */
        bool ConvertResolution(int srcWidth, int srcHeight, int dstWidth, int dstHeight)
        {
            /* 参数有效性检查 */
            if (srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0)
            {
                return false;
            }

            /* 计算缩放比例 */
            float scaleX = static_cast<float>(dstWidth) / srcWidth;
            float scaleY = static_cast<float>(dstHeight) / srcHeight;

            /* 转换所有坐标点 */
            nX *= scaleX;
            nY *= scaleY;
            return true;
        }
    } Pos_S;

    /// @brief 坐标数据结构
    typedef struct _PositionFloat_S_
    {
        float fX; /* X坐标 */
        float fY; /* Y坐标 */
        /* 重载默认构造函数 */
        _PositionFloat_S_()
            : fX(0.0f),
              fY(0.0f)
        {
        }
        /* 接受初始化列表的构造函数 */
        _PositionFloat_S_(float x, float y)
            : fX(x),
              fY(y)
        {
        }
        /* 重载赋值运算符 */
        _PositionFloat_S_ &operator=(const _PositionFloat_S_ &x)
        {
            if (this != &x)
            {
                fX = x.fX;
                fY = x.fY;
            }
            return *this;
        }

        /**
         * @brief   : 分辨率坐标转换函数
         * @param    {int} srcWidth：原始分辨率宽度
         * @param    {int} srcHeight：原始分辨率高度
         * @param    {int} dstWidth：目标分辨率宽度
         * @param    {int} dstHeight：目标分辨率高度
         * @return   {bool} 转换是否成功
         */
        bool ConvertResolution(int srcWidth, int srcHeight, int dstWidth, int dstHeight)
        {
            /* 参数有效性检查 */
            if (srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0)
            {
                return false;
            }

            /* 计算缩放比例 */
            float scaleX = static_cast<float>(dstWidth) / srcWidth;
            float scaleY = static_cast<float>(dstHeight) / srcHeight;

            /* 转换所有坐标点 */
            fX *= scaleX;
            fY *= scaleY;
            return true;
        }

        /**
         * @brief   : 判断坐标是否有效
         * @param    {float} fMaxX x轴最大值
         * @param    {float} fMaxY Y轴最大值
         * @return   {bool} true：有效 false：无效
         */
        bool IsValid(float fMaxX = 1920.0f, float fMaxY = 1080.0f) const
        {
            if (fX < 0.0f || fY < 0.0f || fX > fMaxX || fY > fMaxY)
            {
                return false;
            }
            return true;
        }

    } PosF_S;

    /// @brief 矩形数据结构
    typedef struct _Rect_S_
    {
        int nX;      /* X坐标 */
        int nY;      /* Y坐标 */
        int nWidth;  /* 宽度 */
        int nHeight; /* 高度 */
        int nColor;  /* 颜色 */
        /* 重载默认构造函数 */
        _Rect_S_()
            : nX(0),
              nY(0),
              nWidth(0),
              nHeight(0),
              nColor(0)
        {
        }
        /* 重载赋值运算符 */
        _Rect_S_ &operator=(const _Rect_S_ &x)
        {
            if (this != &x)
            {
                nX = x.nX;
                nY = x.nY;
                nWidth = x.nWidth;
                nHeight = x.nHeight;
                nColor = x.nColor;  
            }
            return *this;
        }

        /**
         * @brief   : 重载!=运算符，判断两个矩形是否不相等
         * @param    {const Rect_S&} other：要比较的另一个矩形
         * @return   {bool} 两个矩形不相等返回true，否则返回false
         */
        bool operator!=(const _Rect_S_ &other) const
        {
            // 只要有一个成员不相等，则两个矩形不相等
            return (nX != other.nX) || (nY != other.nY) || (nWidth != other.nWidth) ||
                   (nHeight != other.nHeight) || (nColor != other.nColor);
        }

        /**
         * @brief   : 分辨率坐标转换函数
         * @param    {int} srcWidth：原始分辨率宽度
         * @param    {int} srcHeight：原始分辨率高度
         * @param    {int} dstWidth：目标分辨率宽度
         * @param    {int} dstHeight：目标分辨率高度
         * @return   {bool} 转换是否成功
         */
        bool ConvertResolution(int srcWidth, int srcHeight, int dstWidth, int dstHeight)
        {
            /* 参数有效性检查 */
            if (srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0)
            {
                return false;
            }

            /* 计算缩放比例 */
            float scaleX = static_cast<float>(dstWidth) / srcWidth;
            float scaleY = static_cast<float>(dstHeight) / srcHeight;

            /* 转换所有坐标点 */
            nX *= scaleX;
            nY *= scaleY;
            nWidth *= scaleX;
            nHeight *= scaleY;
            return true;
        }
 
        /**
         * @brief   : 判断矩形是否有效
         * @param    {int} nMaxX x轴最大值
         * @param    {int} nMaxY y轴最大值
         * @return   {bool} true：有效 false：无效
         */
        bool IsValid(int nMaxX = 1920, int nMaxY = 1080) const
        {
            /* 检查宽高是否至少有一个为0 */
            if (nX < 0 || nY < 0 || (nX + nWidth) > nMaxX || (nY + nHeight) > nMaxY)
            {
                return false;
            }

            return true;
        }

        /**
         * @brief   : 判断矩形是否为空（宽度或高度为零）
         * @return   {bool} 如果宽度或高度为零，则返回 true；否则返回 false
         */
        bool isEmpty() const
        {
            return (nWidth <= 0 || nHeight <= 0);
        }
    } Rect_S;

    /// @brief 矩形数据结构
    typedef struct _RectFloat_S_
    {
        float fX;      /* X坐标 */
        float fY;      /* Y坐标 */
        float fWidth;  /* 宽度 */
        float fHeight; /* 高度 */
        /* 重载默认构造函数 */
        _RectFloat_S_()
            : fX(0.0f),
              fY(0.0f),
              fWidth(0.0f),
              fHeight(0.0f)
        {
        }
        /* 重载赋值运算符 */
        _RectFloat_S_ &operator=(const _RectFloat_S_ &x)
        {
            if (this != &x)
            {
                fX = x.fX;
                fY = x.fY;
                fWidth = x.fWidth;
                fHeight = x.fHeight;
            }
            return *this;
        }

        /**
         * @brief   : 判断矩形是否有效
         * @param    {float} fMaxX x轴最大值
         * @param    {float} fMaxY y轴最大值
         * @return   {bool} true：有效 false：无效
         */
        bool IsValid(float fMaxX = 1920.0f, float fMaxY = 1080.0f) const
        {
            /* 检查宽高是否至少有一个为0 */
            if (fX < 0 || fY < 0 || (fX + fWidth) > fMaxX || (fY + fHeight) > fMaxY)
            {
                return false;
            }

            return true;
        }
    } RectF_S;

    /* 矩形左上角右下角坐标数据结构 */
    typedef struct _RectInfo_S_
    {
        /* 左上角坐标 */
        int nX1 = -1;
        int nY1 = -1;
        /* 右下角坐标 */
        int nX2 = -1;
        int nY2 = -1;
        /* 重载默认构造函数 */
        _RectInfo_S_() : nX1(0), 
                         nY1(0), 
                         nX2(0), 
                         nY2(0)
        {
        }
        /* 重载赋值运算符 */
        _RectInfo_S_ &operator=(const _RectInfo_S_ &x)
        {
            if (this != &x)
            {
                nX1 = x.nX1;
                nY1 = x.nY1;
                nX2 = x.nX2;
                nY2 = x.nY2;
            }
            return *this;
        }

        /**
         * @brief   : 分辨率坐标转换函数
         * @param    {int} srcWidth：原始分辨率宽度
         * @param    {int} srcHeight：原始分辨率高度
         * @param    {int} dstWidth：目标分辨率宽度
         * @param    {int} dstHeight：目标分辨率高度
         * @return   {bool} 转换是否成功
         */
        bool ConvertResolution(int srcWidth, int srcHeight, int dstWidth, int dstHeight)
        {
            /* 参数有效性检查 */
            if (srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0)
            {
                return false;
            }

            /* 计算缩放比例 */
            float scaleX = static_cast<float>(dstWidth) / srcWidth;
            float scaleY = static_cast<float>(dstHeight) / srcHeight;

            /* 转换所有坐标点 */
            nX1 *= scaleX;
            nY1 *= scaleY;
            nX2 *= scaleX;
            nY2 *= scaleY;
            return true;
        }
    } RectInfo_S;

    /// @brief 日期数据结构
    typedef struct _Date_S_
    {
        unsigned int nYear;  /* 年 [1900-3000] */
        unsigned int nMonth; /* 月 [1-12] */
        unsigned int nDay;   /* 日 [1-31] */
        
        /* 重载默认构造函数 */
        _Date_S_()
            : nYear(2025),
              nMonth(1),
              nDay(1)
        {
        }
        
        /* 带参数的构造函数 */
        _Date_S_(unsigned int year, unsigned int month, unsigned int day)
            : nYear(year),
              nMonth(month),
              nDay(day)
        {
        }
        
        /* 重载赋值运算符 */
        _Date_S_ &operator=(const _Date_S_ &x)
        {
            if (this != &x)
            {
                nYear = x.nYear;
                nMonth = x.nMonth;
                nDay = x.nDay;
            }
            return *this;
        }
        
        /* 重载比较运算符 */
        bool operator<(const _Date_S_ &other) const
        {
            if (nYear != other.nYear) return nYear < other.nYear;
            if (nMonth != other.nMonth) return nMonth < other.nMonth;
            return nDay < other.nDay;
        }
        
        bool operator<=(const _Date_S_ &other) const
        {
            return (*this < other) || (*this == other);
        }
        
        bool operator>(const _Date_S_ &other) const
        {
            return !(*this <= other);
        }
        
        bool operator>=(const _Date_S_ &other) const
        {
            return !(*this < other);
        }
        
        bool operator==(const _Date_S_ &other) const
        {
            return (nYear == other.nYear) && (nMonth == other.nMonth) && (nDay == other.nDay);
        }
        
        bool operator!=(const _Date_S_ &other) const
        {
            return !(*this == other);
        }
        
        /**
         * @brief 获取当前日期
         * @return Date_S 当前日期
         */
        static _Date_S_ GetCurrentDate()
        {
            time_t currentTime = time(nullptr);
            struct tm *currentTm = localtime(&currentTime);
            return _Date_S_(currentTm->tm_year + 1900, currentTm->tm_mon + 1, currentTm->tm_mday);
        }
        
        /**
         * @brief 检查日期是否有效
         * @return bool true-有效 false-无效
         */
        bool IsValid() const
        {
            if (nYear < 1900 || nYear > 3000) return false;
            if (nMonth < 1 || nMonth > 12) return false;
            if (nDay < 1 || nDay > 31) return false;
            
            /* 检查每月的天数 */
            static const unsigned int DAYS_IN_MONTH[] = { 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U };
            unsigned int nMaxDays = DAYS_IN_MONTH[nMonth - 1];

            /* 闰年2月有29天 */
            if (nMonth == 2 && IsLeapYear(nYear))
            {
                nMaxDays = 29U;
            }

            return nDay <= nMaxDays;
        }

        /**
         * @brief 判断是否为闰年
         * @param year 年份
         * @return bool true-闰年 false-平年
         */
        static bool IsLeapYear(unsigned int year)
        {
            return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        }
        
        /**
         * @brief 转换为天数（用于日期比较）
         * @return int 从公元元年开始的天数（近似值）
         */
        int ToDays() const
        {
            return nYear * 365 + nMonth * 31 + nDay;
        }
        
    } Date_S;
    
} // namespace Common
