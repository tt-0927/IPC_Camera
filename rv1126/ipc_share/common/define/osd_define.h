/**
 * @FilePath     : osd_define.h
 * @Author       : huangjunda
 * @Date         : 2025-05-26 17:14:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-22 15:35:02
 * @Description  : OSD定义
 */

#pragma once

#include <string>
#include <vector>
#include "common_define.h"

namespace Osd
{
    /* OSD语言（中英文） */
    typedef enum
    {
        LANGUEAGE_CHINESE = 0, /* 中文 */
        LANGUEAGE_ENGLISH,     /* 英文 */
        LANGUEAGE_MAX
    } LANGUEAGE_E;

    /* OSD对齐方式 */
    typedef enum
    {
        ALIGN_TOP_LEFT = 0, /* 左上对齐 */
        ALIGN_BOTTOM_LEFT,  /* 左下对齐 */
        ALIGN_TOP_RIGHT,    /* 右上对齐 */
        ALIGN_BOTTOM_RIGHT, /* 右下对齐 */
        ALIGN_CENTER,       /* 中心对齐 */
        ALIGN_MAX
    } Align_E;

    /* OSD参考尺寸（实际OSD模板的尺寸根据公式进行计算：实际模板尺寸 = 设置模板尺寸 * 实际分辨率 / 参考分辨率） */
    typedef enum
    {
        REFERENCE_SIZE_640_384 = 0, /* 640 * 384 */
        REFERENCE_SIZE_480P,        /* 640 * 480 */
        REFERENCE_SIZE_640_640,     /* 640 * 640 */
        REFERENCE_SIZE_576P,        /* 1024 * 576 */
        REFERENCE_SIZE_720P,        /* 1280 * 720 */
        REFERENCE_SIZE_1080P,       /* 1920 * 1080 */
        REFERENCE_SIZE_2K,          /* 2560 * 1440 */
        REFERENCE_SIZE_2_5K,        /* 2880 * 1620 */
        REFERENCE_SIZE_4K,         /* 3840 * 2160 */
        REFERENCE_SIZE_MAX
    } ReferenceSize_E;

    /* OSD元素种类（值越大，层次越高） */
    typedef enum
    {
        ELEMENT_TYPE_PEOPLE = 1, /* 人头 */
        ELEMENT_TYPE_CUSTOMIZE,  /* 自定义 */
        ELEMENT_TYPE_PRESET,     /* 预置位 */
        ELEMENT_TYPE_IP,         /* IP地址 */
        ELEMENT_TYPE_MAC,        /* 物理地址注册信息（网页不可选择） */
        ELEMENT_TYPE_TIME,       /* 时间 */
        ELEMENT_TYPE_NAME,       /* 名称 */
        ELEMENT_TYPE_MAX
    } ElementType_E;

    /* OSD元素种类（值越大，层次越高） */
    typedef enum
    {
        POS_START = 0,
        POS_END,
        POS_MAX
    } Pos_E;

    /* 坐标信息 */
    typedef struct
    {
        int nX;
        int nY;
    } CoordinateInfo_S;

    /* Overplay配置信息 */
    typedef struct
    {
        bool bEnableFlicker;         /* 闪烁 */
        bool bEnableRevColor;        /* 反色使能（hi3516不支持，功能开启后，“字体颜色”、“背景颜色”、"文字透明度"和“背景透明度”的设置将失效） */
        Align_E enAlign;             /* 对齐方式 */
        int nHorMargin;              /* 水平边距 */
        int nVerMargin;              /* 垂直边距 */
        int nWidth;                  /* 插件小窗口中osd显示框的宽（<=0表示运行时自适应） */
        int nHeight;                 /* 插件小窗口中osd显示框的高 */
        int nLineSpace;              /* 行间距（未使用） */
        int nFontSize;               /* 字体大小（实际字体大小根据公式进行计算：实际字体大小 = 设置字体大小 * 实际分辨率宽 / 参考分辨率宽） */
        std::string strFontColor;    /* 字体颜色（rgb888格式0x000000~0xFFFFFF） */
        std::string strBackColor;    /* 背景颜色（rgb888格式0x000000~0xFFFFFF） */
        int nFontAlpha;              /* 字体透明度（百分比） */
        int nBackAlpha;              /* 背景透明度（百分比） */
        ElementType_E enElementType; /* 元素种类 */
        bool bEnableTimeZone;        /* 时区使能 */
        bool bEnableWeek;            /* 星期使能 */
        bool bEnablePeriod;          /* 时间段使能 */
        std::string strCustomize;    /* 自定义信息 */
    } Overplay_S;

    /* Cover配置信息 */
    typedef struct
    {
        bool bEnableRectangle;                       /* 矩形使能（若使能，返回矩形的起始坐标和结束坐标两个点：(nCoordinate[0])和(nCoordinate[1])，不使能则需要四边形四个点的坐标） */
        bool bEnableSolid;                           /* 实心使能（实心只支持矩形不支持四边形） */
        std::string strBackColor;                    /* 背景颜色（rgb888格式0x000000~0xFFFFFF） */
        std::vector<CoordinateInfo_S> stuCoordinate; /* 坐标 */
    } Cover_S;

    /* OSD信息 */
    typedef struct
    {
        int nID;                   /* 序号 */
        bool bEnable;              /* 使能 */
        std::string strName;       /* 名称 */
        ReferenceSize_E enRefSize; /* 参考尺寸 */
    } Info_S;

    /* Overplay信息 */
    typedef struct _OverplayInfo_S_
    {
        Info_S stuInfo;         /* osd信息 */
        Overplay_S stuOverplay; /* overplay信息 */

        void clear()
        {
            stuInfo.nID = 1;
            stuInfo.bEnable = false;
            stuInfo.strName = "Customize";
            stuInfo.enRefSize = REFERENCE_SIZE_1080P;

            stuOverplay.bEnableFlicker = false;
            stuOverplay.bEnableRevColor = false;
            stuOverplay.enAlign = ALIGN_TOP_LEFT;
            stuOverplay.nHorMargin = 0;
            stuOverplay.nVerMargin = 0;
            stuOverplay.nWidth = 0;
            stuOverplay.nHeight = 0;
            stuOverplay.nLineSpace = 0;
            stuOverplay.nFontSize = 64;
            stuOverplay.strFontColor = "0x000000"; /* 黑色 */
            stuOverplay.strBackColor = "0xFFFFFF"; /* 白色 */
            stuOverplay.nFontAlpha = 0;            /* 不透明 */
            stuOverplay.nBackAlpha = 100;          /* 全透明 */
            stuOverplay.enElementType = ELEMENT_TYPE_CUSTOMIZE;
            stuOverplay.bEnableTimeZone = false;
            stuOverplay.bEnableWeek = false;
            stuOverplay.bEnablePeriod = false;
            stuOverplay.strCustomize = "Customize";
        }
    } OverplayInfo_S;

    /* Cover信息 */
    typedef struct _CoverInfo_S_
    {
        Info_S stuInfo;   /* osd信息 */
        Cover_S stuCover; /* cover信息 */

        void clear()
        {
            stuInfo.nID = 1;
            stuInfo.bEnable = false;
            stuInfo.strName = "Cover";
            stuInfo.enRefSize = REFERENCE_SIZE_1080P;

            stuCover.bEnableRectangle = true;
            stuCover.bEnableSolid = true;
            stuCover.strBackColor = "0x000000"; /* 黑色 */
            stuCover.stuCoordinate.resize(2);
            stuCover.stuCoordinate.at(POS_START).nX = -2; /* 默认在分辨率显示外 */
            stuCover.stuCoordinate.at(POS_START).nY = -2; /* 默认在分辨率显示外 */
            stuCover.stuCoordinate.at(POS_END).nX = 2;    /* 最小宽度 */;
            stuCover.stuCoordinate.at(POS_END).nY = 2;    /* 最小高度 */;
        }
    } CoverInfo_S;

    /* OSD时间信息 */
    typedef struct
    {
        std::string strZone;   /* 时区 */
        std::string strWeek;   /* 星期 */
        std::string strTime;   /* 时间 */
        std::string strTime12; /* 12小时制式时间 */
    } TimeInfo_S;

    /* OSD共用信息 */
    typedef struct
    {
        TimeInfo_S stuTimeInfo; /* 时间信息 */
        std::string strMac;     /* 物理地址注册信息 */
        std::string strIp;      /* IP地址 */
        std::string strPreset;  /* 预置位 */
        std::string strPeople;  /* 人头 */

    } ShareInfo_S;

    /* ------------------------------对接网页------------------------------ */
    /* OSD属性 */
    typedef enum
    {
        OSD_ATTR_ALPHA_N_FLASH_N = 0, /* 不透明，不闪烁 */
        OSD_ATTR_ALPHA_N_FLASH_Y,     /* 不透明，  闪烁 */
        OSD_ATTR_ALPHA_Y_FLASH_N,     /*   透明，不闪烁 */
        OSD_ATTR_ALPHA_Y_FLASH_Y      /*   透明，  闪烁 */
    } OSD_ATTRIBUTE_E;

    /* OSD字体大小 */
    typedef enum
    {
        OSD_FONT_SIZE_ADAPTIVE = 0, /* 自适应 */
        OSD_FONT_SIZE_16,           /* 16 * 16 */
        OSD_FONT_SIZE_32,           /* 32 * 32 */
        OSD_FONT_SIZE_48,           /* 48 * 48 */
        OSD_FONT_SIZE_64            /* 64 * 64 */
    } OSD_FONT_SIZE_E;

    /* OSD颜色 */
    typedef enum
    {
        OSD_COLOR_BLACK = 0, /* 黑色 */
        OSD_COLOR_WHITE,     /* 白色 */
        OSD_COLOR_CUSTOMIZE  /* 自定义 */
    } OSD_COLOR_E;

    /* OSD对齐方式 */
    typedef enum
    {
        OSD_ALIFN_CUSTOMIZE = 0,   /* 自定义 */
        OSD_ALIFN_CHARACTER_LEFT,  /* 字符左对齐 */
        OSD_ALIFN_CHARACTER_RIGHT, /* 字符右对齐 */
        OSD_ALIFN_ALL_LEFT,        /* 全部左对齐 */
        OSD_ALIFN_ALL_RIGHT,       /* 全部右对齐 */
        OSD_ALIFN_GB_MODE          /* 国标模式 */
    } OSD_ALIGN_E;

    /* OSD时间格式 */
    typedef enum
    {
        OSD_TIME_FORMAT_24 = 0, /* 24小时制 */
        OSD_TIME_FORMAT_12      /* 12小时制 */
    } OSD_TIME_FORMAT_E;

    /* OSD日期格式 */
    typedef enum
    {
        ENGLISH_YYYY_MM_DD = 0, /* YYYY-MM-DD(年月日), 例如 2024-10-01 */
        ENGLISH_MM_DD_YYYY,     /* MM-DD-YYYY(月日年), 例如 10-01-2024 */
        ENGLISH_DD_MM_YYYY,     /* DD-MM-YYYY(日月年), 例如 01-10-2024 */
        CHINESE_YYYYMMDD,       /* YYYY年MM月DD日,     例如 2024年10月01日 */
        CHINESE_MMDDYYYY,       /* MM月DD日YYYY年,     例如 10月01日2024年 */
        CHINESE_DDMMYYYY,       /* DD日MM月YYYY年,     例如 01日10月2024年 */
        ENGLISH_YYYYMMDD,       /* YYYY/MM/DD(年月日), 例如 2024/10/01 */
        ENGLISH_MMDDYYYY,       /* MM/DD/YYYY(月日年), 例如 10/01/2024 */
        ENGLISH_DDMMYYYY        /* DD/MM/YYYY(日月年), 例如 01/10/2024 */
    } OSD_DATE_FORMAT_E;

    /* OSD状态信息 */
    typedef struct _OsdAttribute_S_
    {
        /* 横坐标 */
        int nX;
        /* 纵坐标 */
        int nY;
        /* 插件小窗口中osd显示框的宽 */
        int nW; /* <=0表示运行时自适应 */
        /* 插件小窗口中osd显示框的高 */
        int nH;
        /* OSD属性 */
        OSD_ATTRIBUTE_E enAttribute;
        /* OSD字体大小 */
        OSD_FONT_SIZE_E enFontSize;
        /* OSD颜色 */
        OSD_COLOR_E enFontColor;
        /* OSD颜色为自定义时，使用，记录RGB颜色 格式:"#000000"" */
        std::string strFontColor;
        /* OSD token Onvif使用 */
        std::string strToken;

        void clear()
        {
            nX = 0;
            nY = 0;
            nW = 0;
            nH = 0;
            enAttribute = OSD_ATTR_ALPHA_N_FLASH_N;
            enFontSize = OSD_FONT_SIZE_ADAPTIVE;
            enFontColor = OSD_COLOR_BLACK;
            strFontColor = "#000000";
            strToken = "";
        }
    } OsdAttribute_S;

    /* OSD名称信息 */
    typedef struct _OsdNameInfo_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 名称 */
        std::string strName;
        /* OSD状态信息 */
        OsdAttribute_S stOsdAttr;

        void clear()
        {
            bEnable = false;
            strName = "Camera";
            stOsdAttr.clear();
        }
    } OsdNameInfo_S;

    /* OSD时间信息 */
    typedef struct _OsdTimeInfo_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* 是否启用星期 */
        bool bEnableWeek;
        /* OSD时间格式 */
        OSD_TIME_FORMAT_E enTimeFormat;
        /* OSD日期格式 */
        OSD_DATE_FORMAT_E enDateFormat;
        /* OSD状态信息 */
        OsdAttribute_S stOsdAttr;

        void clear()
        {
            bEnable = false;
            bEnableWeek = false;
            enTimeFormat = OSD_TIME_FORMAT_24;
            enDateFormat = ENGLISH_YYYY_MM_DD;
            stOsdAttr.clear();
        }
    } OsdTimeInfo_S;

    /* OSD字符叠加信息 */
    typedef struct _OsdInfo_S_
    {
        /* 字符ID */
        int nId;
        /* 是否启用 */
        bool bEnable;
        /* 字符串名称 */
        std::string strName;
        /* OSD状态信息 */
        OsdAttribute_S stOsdAttr;

        void clear()
        {
            nId = 0;
            bEnable = false;
            strName = "";
            stOsdAttr.clear();
        }
    } OsdInfo_S;

    /* OSD配置信息 */
    typedef struct _OsdConfig_S_
    {
        /* OSD对齐方式 */
        OSD_ALIGN_E enAlign;
        /* OSD名称信息 */
        OsdNameInfo_S stOsdNameInfo;
        /* OSD时间信息 */
        OsdTimeInfo_S stOsdTimeInfo;
        /* OSD字符叠加信息 */
        std::vector<OsdInfo_S> vecOsdInfo;

        void clear()
        {
            enAlign = OSD_ALIFN_CUSTOMIZE;
            stOsdNameInfo.clear();
            stOsdTimeInfo.clear();
            vecOsdInfo.resize(4);
            /* token初始化 */
            stOsdNameInfo.stOsdAttr.strToken = "OsdToken_name";
            stOsdTimeInfo.stOsdAttr.strToken = "OsdToken_time";
            for (size_t i = 0; i < vecOsdInfo.size(); i++)
            {
                vecOsdInfo[i].clear();
                vecOsdInfo[i].nId = i + 1;
                vecOsdInfo[i].stOsdAttr.strToken = "OsdToken_" + std::to_string(vecOsdInfo[i].nId);
            }
        }

        void init_token()
        {
            /* token初始化 */
            stOsdNameInfo.stOsdAttr.strToken = "OsdToken_name";
            stOsdTimeInfo.stOsdAttr.strToken = "OsdToken_time";
            for (size_t i = 0; i < vecOsdInfo.size(); i++)
            {
                vecOsdInfo[i].stOsdAttr.strToken = "OsdToken_" + std::to_string(vecOsdInfo[i].nId);
            }
        }
    } OsdConfig_S;

    /* Cover状态信息 */
    typedef struct _CoverAttribute_S_
    {
        /* 遮挡ID */
        int nId;
        /* 是否启用 */
        bool bEnable;
        /* 遮挡区域名称 */
        std::string strName;
        /* 遮挡区域颜色 */
        OSD_COLOR_E enColor;
        /* 遮挡区域颜色为自定义时，使用，记录RGB颜色 格式:"#000000"" */
        std::string strColor;
        /* 横坐标 */
        int nX;
        /* 纵坐标 */
        int nY;
        /* 宽度 */
        int nWidth;
        /* 高度 */
        int nHeight;

        void clear()
        {
            nId = 0;
            bEnable = false;
            strName = "遮挡区域";
            enColor = OSD_COLOR_BLACK;
            strColor = "#000000";
            nX = -2; /* 默认在分辨率显示外 */
            nY = -2; /* 默认在分辨率显示外 */
            nWidth = 2;  /* 最小宽度 */
            nHeight = 2; /* 最小高度 */
        }
    } CoverAttribute_S;

    /* Cover配置信息 */
    typedef struct _CoverConfig_S_
    {
        /* 是否启用 */
        bool bEnable;
        /* Cover状态信息 */
        std::vector<CoverAttribute_S> vecCoverAttr;

        void clear()
        {
            bEnable = false;
            vecCoverAttr.resize(4);
            for (size_t i = 0; i < vecCoverAttr.size(); i++)
            {
                vecCoverAttr[i].clear();
                vecCoverAttr[i].nId = i + 1;
                vecCoverAttr[i].strName += std::to_string(i + 1);
            }
        }
    } CoverConfig_S;
}
