#pragma once

#include "dcl.h"
#include "dcl_memory.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "macro.hpp"

#include <string>
#include <vector>

namespace dcl
{
    typedef dclPixelFormat pixelFormat_t;
    typedef dclFormat dataLayout_t;
    typedef dclDataType dclDataType_t;

    struct Mat
    {
        unsigned char *data{nullptr};
        uint64_t phyAddr{0};
        int channels{0};
        int height{0}; // 16 pixel align
        int width{0};  // 16 pixel align
        bool own{false};
        pixelFormat_t pixelFormat{DCL_PIXEL_FORMAT_BGR_888_PACKED};

        int original_height{0};
        int original_width{0};

        Mat() = default;
        // ~Mat() { free(); }

        Mat(int _height, int _width, pixelFormat_t _pixelFormat)
        {
            create(_height, _width, _pixelFormat);
        }

        int create(int _height, int _width, pixelFormat_t _pixelFormat)
        {
            if (data)
            {
                printf("data != nullptr\n");
                return -1;
            }
            height = _height;
            width = _width;
            original_height = _height;
            original_width = _width;
            pixelFormat = _pixelFormat;
            switch (pixelFormat)
            {
            case DCL_PIXEL_FORMAT_BGR_888_PACKED:
            case DCL_PIXEL_FORMAT_RGB_888_PACKED:
            case DCL_PIXEL_FORMAT_BGR_888_PLANAR:
            case DCL_PIXEL_FORMAT_RGB_888_PLANAR:
            case DCL_PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            case DCL_PIXEL_FORMAT_YVU_SEMIPLANAR_420:
                channels = 3;
                break;
            case DCL_PIXEL_FORMAT_YUV_400:
                channels = 1;
                break;
            default:
                printf("Not support pixel_format: %d\n", pixelFormat);
                return -1;
            }

            own = true;
            dclError ret = dclrtMallocEx((void **)&data, &phyAddr, size(), 16, DCL_MEM_MALLOC_NORMAL_ONLY);
            if (DCL_SUCCESS != ret)
            {
                printf("dclrtMallocEx failed, errorCode: %d\n", ret);
                return -1;
            }
            return 0;
        }

        void free()
        {
            if (own)
                DCLRT_FREE(data);
        }

        bool empty() const { return !data; }

        unsigned char *ptr() const { return data; }

        int c() const { return channels; }

        int h() const { return height; }

        int w() const { return width; }

        size_t size() const
        {
            switch (pixelFormat)
            {
            case DCL_PIXEL_FORMAT_BGR_888_PACKED:
            case DCL_PIXEL_FORMAT_RGB_888_PACKED:
            case DCL_PIXEL_FORMAT_BGR_888_PLANAR:
            case DCL_PIXEL_FORMAT_RGB_888_PLANAR:
            case DCL_PIXEL_FORMAT_YUV_400: // gray
                return channels * height * width;
            case DCL_PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            case DCL_PIXEL_FORMAT_YVU_SEMIPLANAR_420:
                return height * width * channels / 2;
            default:
                printf("Not support pixel_format: %d\n", pixelFormat);
                std::exit(-1);
            }
        }
    };

    struct Tensor
    {
        int idx{-1};
        std::string name;
        void *data{nullptr}; // device
        void *host_data{nullptr};
        uint64_t phyAddr{0};
        int64_t dimCount{0};
        int64_t dims[DCL_MAX_DIM_CNT]{};
        dataLayout_t layout{DCL_FORMAT_UNDEFINED};
        dclDataType_t dtype{DCL_FLOAT};

        void copyDims(dclmdlIODims &d)
        {
            dimCount = d.dimCount;
            memcpy(dims, d.dims, dimCount * sizeof(int64_t));
        }

        std::string dimsToString()
        {
            std::string shape_s;
            for (int d = 0; d < dimCount - 1; ++d)
            {
                shape_s += std::to_string(dims[d]);
                shape_s += " ";
            }
            shape_s += std::to_string(dims[dimCount - 1]);
            return shape_s;
        }

        size_t size() const
        {
            if (0 == dimCount)
                return 0;
            size_t length = 1;
            for (int i = 0; i < dimCount; ++i)
                length *= dims[i];
            return length;
        }

        size_t size_bytes()
        {
            switch (dtype)
            {
            case DCL_INT64:
            case DCL_UINT64:
                return size() * 8;
            case DCL_FLOAT:
            case DCL_INT32:
            case DCL_UINT32:
                return size() * 4;
            case DCL_FLOAT16:
            case DCL_INT16:
            case DCL_UINT16:
                return size() * 2;
            case DCL_INT8:
            case DCL_UINT8:
            case DCL_BOOL:
                return size();
            default:
                break;
            }
        }
    };

    struct Box
    {
        int x1{0};
        int y1{0};
        int x2{0};
        int y2{0};

        Box() = default;

        Box(int _x1, int _y1, int _x2, int _y2)
        {
            x1 = _x1;
            y1 = _y1;
            x2 = _x2;
            y2 = _y2;
        }

        int w() const { return x2 - x1 + 1; }

        int h() const { return y2 - y1 + 1; }

        int x() const { return x1; }

        int y() const { return y1; }

        int cx() const { return (x1 + x2) / 2; }

        int cy() const { return (y1 + y2) / 2; }
    };

    struct Size
    {
        int h{0}, w{0};
        Size() = default;
        Size(int _h, int _w)
        {
            h = _h;
            w = _w;
        }
    };

    struct Point
    {
        int x{0};
        int y{0};
        float score{0.0f};
        Point() = default;
        Point(int _x, int _y)
        {
            x = _x;
            y = _y;
        }
        int label{0}; // for sam
    };

    struct Point2f
    {
        float x{0};
        float y{0};
        float score{0.0f};
        Point2f() = default;
        Point2f(float _x, float _y)
        {
            x = _x;
            y = _y;
        }
        int label{0}; // for sam
    };

    struct Color
    {
        uint8_t b{0};
        uint8_t g{0};
        uint8_t r{0};

        Color() = default;
        Color(uint8_t _b, uint8_t _g, uint8_t _r) { set(_b, _g, _r); }

        void set(uint8_t _b, uint8_t _g, uint8_t _r)
        {
            b = _b;
            g = _g;
            r = _r;
        }
    };

    typedef std::vector<cv::Point> contour_t;

    typedef struct
    {
        float conf{0.0f};
        int cls{-1};      // cls index
        std::string name; // cls_name
        Box box;
        Point pts[5]{};   // 5 landmark
        Point kpts[17]{}; // 17-keypoints
        float mask[32]{};
        dcl::Mat prob;
        std::vector<contour_t> contours;
    } detection_t;

    typedef struct
    {
        int cls{-1};      // cls index
        std::string name; // cls_name
        float conf{0.0f};
    } classification_t;

    typedef std::vector<detection_t> detections_t;

    typedef struct Frame : public Mat
    {
        uint64_t idx{0};

    } frame_t;
} // namespace dcl
