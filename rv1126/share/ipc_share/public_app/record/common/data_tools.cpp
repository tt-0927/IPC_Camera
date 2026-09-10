/**
 * @FilePath     : data_tools.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-28 10:36:11
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-30 09:20:25
 * @Description  : 数据转换接口函数接口
 */

#include "data_tools.h"

#include <cmath>
#include <ctime>
#include <errno.h>
//#include <filesystem>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <cstring>
#include "libavcodec/get_bits.h"


#define NAL_BLA_W_LP   16
#define NAL_BLA_W_RADL 17
#define NAL_BLA_N_LP   18
#define NAL_IDR_W_RADL 19
#define NAL_IDR_N_LP   20
#define NAL_CRA_NUT    21
#define NAL_VPS        32
#define NAL_SPS        33
#define NAL_PPS        34

#define MAX_SPS_COUNT 32
#define MAX_PPS_COUNT 256


/* 给解析帧信息使用 */
#if 1

typedef unsigned char UINT8;

typedef unsigned int UNWORD;

typedef unsigned long int DWORD;

typedef unsigned int UINT;

typedef unsigned char BYTE;

static UINT Ue(BYTE* pBuff, UINT nLen, UINT* nStartBit)
{
    // 计算0bit的个数
    UINT nZeroNum = 0;
    while ((*nStartBit) < nLen * 8)
    {
        if (pBuff[(*nStartBit) / 8] & (0x80 >> ((*nStartBit) % 8)))    //&:按位与，%取余
        {
            break;
        }
        nZeroNum++;
        (*nStartBit)++;
    }
    (*nStartBit)++;


    // 计算结果
    DWORD dwRet = 0;
    UINT  i     = 0;
    for (i = 0; i < nZeroNum; i++)
    {
        dwRet <<= 1;
        if (pBuff[(*nStartBit) / 8] & (0x80 >> ((*nStartBit) % 8)))
        {
            dwRet += 1;
        }
        (*nStartBit)++;
    }
    return (1 << nZeroNum) - 1 + dwRet;
}

static int Se(BYTE* pBuff, UINT nLen, UINT* nStartBit)
{
    int    UeVal  = Ue(pBuff, nLen, nStartBit);
    double k      = UeVal;
    int    nValue = ceil(k / 2);    // ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
    if (UeVal % 2 == 0)
    {
        nValue = -nValue;
    }
    return nValue;
}

static DWORD u(UINT BitCount, BYTE* buf, UINT* nStartBit)
{
    DWORD dwRet = 0;
    UINT  i     = 0;
    for (i = 0; i < BitCount; i++)
    {
        dwRet <<= 1;
        if (buf[(*nStartBit) / 8] & (0x80 >> ((*nStartBit) % 8)))
        {
            dwRet += 1;
        }
        (*nStartBit)++;
    }
    return dwRet;
}

/**
 * H264的NAL起始码防竞争机制
 *
 * @param buf SPS数据内容
 *
 * @无返回值
 */
static void de_emulation_prevention1(BYTE* buf, unsigned int* buf_size)
{
    unsigned int i = 0, j = 0;
    BYTE*        tmp_ptr      = NULL;
    unsigned int tmp_buf_size = 0;
    int          val          = 0;


    tmp_ptr      = buf;
    tmp_buf_size = *buf_size;
    for (i = 0; i < (tmp_buf_size - 2); i++)
    {
        // check for 0x000003
        val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
        if (val == 0)
        {
            // kick out 0x03
            for (j = i + 2; j < tmp_buf_size - 1; j++)
            {
                tmp_ptr[j] = tmp_ptr[j + 1];
            }

            // and so we should devrease bufsize
            (*buf_size)--;
        }
    }
}

static int scaling_list(unsigned int* scalingList, int sizeOfScalingList, unsigned int useDefaultScalingMatrixFlag, BYTE* buf, UINT nLen, UINT* nStartBit)
{
    int lastScale = 8;
    int nextScale = 8;
    for (int j = 0; j < sizeOfScalingList; j++)
    {
        if (nextScale != 0)
        {
            int delta_scale             = Se(buf, nLen, nStartBit);
            nextScale                   = (lastScale + delta_scale + 256) % 256;
            useDefaultScalingMatrixFlag = (j == 0 && nextScale == 0);
        }
        scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
        lastScale      = scalingList[j];
    }

    return 0;
}

/**
* 解码SPS,获取视频图像宽、高和帧率信息
*
* @param buf SPS数据内容
* @param nLen SPS数据的长度
* @param width 图像宽度
* @param height 图像高度

* @成功则返回true , 失败则返回false
*/
static int h264_decode_sps(BYTE* buf, int nLen, int* width, int* height, int* fps)
{
    if (nLen <= 5)
    {
        return -1;
    }

    UINT StartBit = 0;
    *fps          = 0;
    *width        = 0;
    *height       = 0;

    de_emulation_prevention1(buf, (unsigned int*)&nLen);

    DWORD forbidden_zero_bit = u(1, buf, &StartBit);

    DWORD nal_ref_idc = u(2, buf, &StartBit);

    DWORD nal_unit_type = u(5, buf, &StartBit);
    if (nal_unit_type == 7)
    {
        int profile_idc          = u(8, buf, &StartBit);
        int constraint_set0_flag = u(1, buf, &StartBit);    //(buf[1] & 0x80)>>7;
        int constraint_set1_flag = u(1, buf, &StartBit);    //(buf[1] & 0x40)>>6;
        int constraint_set2_flag = u(1, buf, &StartBit);    //(buf[1] & 0x20)>>5;
        int constraint_set3_flag = u(1, buf, &StartBit);    //(buf[1] & 0x10)>>4;
        int reserved_zero_4bits  = u(4, buf, &StartBit);
        int level_idc            = u(8, buf, &StartBit);

        UINT seq_parameter_set_id = Ue(buf, nLen, &StartBit);
        if (profile_idc == 100 || profile_idc == 110 ||
            profile_idc == 122 || profile_idc == 144)
        {
            int chroma_format_idc = Ue(buf, nLen, &StartBit);
            if (chroma_format_idc == 3)
            {
                DWORD residual_colour_transform_flag;
                residual_colour_transform_flag = u(1, buf, &StartBit);
            }
            /*UINT bit_depth_luma_minus8 =*/Ue(buf, nLen, &StartBit);
            /*UINT bit_depth_chroma_minus8 =*/Ue(buf, nLen, &StartBit);
            /*DWORD qpprime_y_zero_transform_bypass_flag = */ u(1, buf, &StartBit);
            DWORD seq_scaling_matrix_present_flag = u(1, buf, &StartBit);
            int   i                               = 0;
            int   seq_scaling_list_present_flag[12];
            if (seq_scaling_matrix_present_flag)
            {
                for (i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++)
                {
                    unsigned int ScalingList4x4[16];
                    unsigned int UseDefaultScalingMatrix4x4Flag[12];
                    unsigned int ScalingList8x8[64];
                    unsigned int UseDefaultScalingMatrix8x8Flag[12];
                    seq_scaling_list_present_flag[i] = u(1, buf, &StartBit);
                    if (seq_scaling_list_present_flag[i])
                    {
                        if (i < 6)
                        {
                            scaling_list(ScalingList4x4, 16, UseDefaultScalingMatrix4x4Flag[i], buf, nLen, &StartBit);
                        }
                        else
                        {
                            scaling_list(ScalingList8x8, 64, UseDefaultScalingMatrix8x8Flag[i - 6], buf, nLen, &StartBit);
                        }
                    }
                }
            }
        }

        /*UINT log2_max_frame_num_minus4 =*/Ue(buf, nLen, &StartBit);
        UINT pic_order_cnt_type = Ue(buf, nLen, &StartBit);
        if (pic_order_cnt_type == 0)
        {
            UINT log2_max_pic_order_cnt_lsb_minus4;
            log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, &StartBit);
        }
        else if (pic_order_cnt_type == 1)
        {
            /*DWORD delta_pic_order_always_zero_flag =*/u(1, buf, &StartBit);
            /*int offset_for_non_ref_pic = */ Se(buf, nLen, &StartBit);
            /*int offset_for_top_to_bottom_field = */ Se(buf, nLen, &StartBit);
            UINT num_ref_frames_in_pic_order_cnt_cycle;
            num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, &StartBit);

            int offset_for_ref_frame[num_ref_frames_in_pic_order_cnt_cycle];    // = new int[num_ref_frames_in_pic_order_cnt_cycle];
            int i = 0;
            for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
            {
                offset_for_ref_frame[i] = Se(buf, nLen, &StartBit);
            }
            // delete[] offset_for_ref_frame;
        }
        /*UINT num_ref_frames = */ Ue(buf, nLen, &StartBit);
        DWORD gaps_in_frame_num_value_allowed_flag;
        gaps_in_frame_num_value_allowed_flag = u(1, buf, &StartBit);
        UINT pic_width_in_mbs_minus1;
        pic_width_in_mbs_minus1 = Ue(buf, nLen, &StartBit);
        UINT pic_height_in_map_units_minus1;
        pic_height_in_map_units_minus1 = Ue(buf, nLen, &StartBit);

        *width  = (pic_width_in_mbs_minus1 + 1) * 16;
        *height = (pic_height_in_map_units_minus1 + 1) * 16;

        DWORD frame_mbs_only_flag;
        frame_mbs_only_flag = u(1, buf, &StartBit);
        if (!frame_mbs_only_flag)
        {
            DWORD mb_adaptive_frame_field_flag;
            mb_adaptive_frame_field_flag = u(1, buf, &StartBit);
        }
        DWORD direct_8x8_inference_flag;
        direct_8x8_inference_flag = u(1, buf, &StartBit);
        DWORD frame_cropping_flag;
        frame_cropping_flag = u(1, buf, &StartBit);
        if (frame_cropping_flag)
        {
            UINT frame_crop_left_offset;
            frame_crop_left_offset = Ue(buf, nLen, &StartBit);
            UINT frame_crop_right_offset;
            frame_crop_right_offset = Ue(buf, nLen, &StartBit);
            UINT frame_crop_top_offset;
            frame_crop_top_offset = Ue(buf, nLen, &StartBit);
            UINT frame_crop_bottom_offset;
            frame_crop_bottom_offset = Ue(buf, nLen, &StartBit);
        }
        DWORD vui_parameter_present_flag;
        vui_parameter_present_flag = u(1, buf, &StartBit);
        if (vui_parameter_present_flag)
        {
            DWORD aspect_ratio_info_present_flag;
            aspect_ratio_info_present_flag = u(1, buf, &StartBit);
            if (aspect_ratio_info_present_flag)
            {
                DWORD aspect_ratio_idc;
                aspect_ratio_idc = u(8, buf, &StartBit);
                if (aspect_ratio_idc == 255)
                {
                    DWORD sar_width;
                    sar_width = u(16, buf, &StartBit);
                    DWORD sar_height;
                    sar_height = u(16, buf, &StartBit);
                }
            }
            DWORD overscan_info_present_flag;
            overscan_info_present_flag = u(1, buf, &StartBit);
            if (overscan_info_present_flag)
            {
                DWORD overscan_appropriate_flagu;
                overscan_appropriate_flagu = u(1, buf, &StartBit);
            }
            DWORD video_signal_type_present_flag;
            video_signal_type_present_flag = u(1, buf, &StartBit);
            if (video_signal_type_present_flag)
            {
                DWORD video_format;
                video_format = u(3, buf, &StartBit);
                DWORD video_full_range_flag;
                video_full_range_flag = u(1, buf, &StartBit);
                DWORD colour_description_present_flag;
                colour_description_present_flag = u(1, buf, &StartBit);
                if (colour_description_present_flag)
                {
                    DWORD colour_primaries;
                    colour_primaries = u(8, buf, &StartBit);
                    DWORD transfer_characteristics;
                    transfer_characteristics = u(8, buf, &StartBit);
                    DWORD matrix_coefficients;
                    matrix_coefficients = u(8, buf, &StartBit);
                }
            }
            DWORD chroma_loc_info_present_flag;
            chroma_loc_info_present_flag = u(1, buf, &StartBit);
            if (chroma_loc_info_present_flag)
            {
                UINT chroma_sample_loc_type_top_field;
                chroma_sample_loc_type_top_field = Ue(buf, nLen, &StartBit);
                UINT chroma_sample_loc_type_bottom_field;
                chroma_sample_loc_type_bottom_field = Ue(buf, nLen, &StartBit);
            }
            DWORD timing_info_present_flag;
            timing_info_present_flag = u(1, buf, &StartBit);

            if (timing_info_present_flag)
            {
                DWORD num_units_in_tick;
                num_units_in_tick = u(32, buf, &StartBit);
                DWORD time_scale;
                time_scale = u(32, buf, &StartBit);
                *fps       = time_scale / num_units_in_tick;
                DWORD fixed_frame_rate_flag;
                fixed_frame_rate_flag = u(1, buf, &StartBit);
                if (fixed_frame_rate_flag)
                {
                    *fps = *fps / 2;
                }
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }
}
#endif


/* 给判断关键帧是否h264使用 */
#if 1

const uint8_t ff_log2_tab[256] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static av_always_inline av_const int logFunc(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000)
    {
        v >>= 16;
        n  += 16;
    }
    if (v & 0xff00)
    {
        v >>= 8;
        n  += 8;
    }
    n += ff_log2_tab[v];

    return n;
}

/**
 * Read an unsigned Exp-Golomb code in the range 0 to UINT32_MAX-1.
 */
static inline unsigned get_ue_golomb_long(GetBitContext* gb)
{
    unsigned buf, log;

    buf = show_bits_long(gb, 32);
    log = 31 - logFunc(buf);
    skip_bits_long(gb, log);

    return get_bits_long(gb, log + 1) - 1;
}
#endif


/* 获取h264数据是否为关键帧 */
bool CDataTools::isIFrame_h264(const char* pchData, int nSize)
{
    if (pchData[0] == 0x00 && pchData[1] == 0x00)
    {
        /* 兼容 起始码为 00 00 01 的某些摄像头*/
        if (pchData[2] == 0x01)
        {
            if ((pchData[3] & 0x1F) == 7 ||
                (pchData[3] & 0x1F) == 8)
            {
                return true;
            }
            else if ((pchData[3] & 0x1F) == 6)
            {
                for (int i = 4; i + 3 < nSize; i++)
                {
                    if (pchData[i] == 0x00 &&
                        pchData[i + 1] == 0x00 &&
                        pchData[i + 2] == 0x01)
                    {
                        if ((pchData[i + 3] & 0x1F) == 7 ||
                            (pchData[i + 3] & 0x1F) == 8)
                        {
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
            }
        }
        /* 起始码为 00 00 00 01*/
        else if (pchData[2] == 0x00 && pchData[3] == 0x01)
        {
            if ((pchData[4] & 0x1F) == 7 ||
                (pchData[4] & 0x1F) == 8)
            {
                return true;
            }
            else if ((pchData[4] & 0x1F) == 6)
            {
                for (int i = 5; i + 4 < nSize; i++)
                {
                    if (pchData[i] == 0x00 &&
                        pchData[i + 1] == 0x00 &&
                        pchData[i + 2] == 0x00 &&
                        pchData[i + 3] == 0x01)
                    {
                        if ((pchData[i + 4] & 0x1F) == 7 ||
                            (pchData[i + 4] & 0x1F) == 8)
                        {
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return false;
}

/* 获取h265数据是否为关键帧 */
bool CDataTools::isIFrame_h265(const char* pchData, int nSize)
{

    if (pchData[0] == 0x00 && pchData[1] == 0x00)
    {
        /* 兼容 起始码为 00 00 01 的某些摄像头*/
        if (pchData[2] == 0x01)
        {
            if ((pchData[3] & 0x7E) >> 1 == 32)
            {
                return true;
            }
        }
        /* 起始码为 00 00 00 01*/
        else if (pchData[2] == 0x00 && pchData[3] == 0x01)
        {
            if ((pchData[4] & 0x7E) >> 1 == 32)
            {
                return true;
            }
        }
    }

    return false;
}

/* 判断是否为h264数据 */
bool CDataTools::is_h264Format(const char* pchData, int nSize)
{
    // if ((pchData[4] & 0x1F) == 6 ||
    //     (pchData[4] & 0x1F) == 7 ||
    //     (pchData[4] & 0x1F) == 8)
    // {
    //     return true;
    // }
    // return false;

    uint8_t* pBuffer = (uint8_t*)pchData;

    uint32_t      code = -1;
    int           sps = 0, pps = 0, idr = 0, res = 0, sli = 0;
    int           i, ret;
    int           pps_ids[MAX_PPS_COUNT + 1] = { 0 };
    int           sps_ids[MAX_SPS_COUNT + 1] = { 0 };
    unsigned      pps_id, sps_id;
    GetBitContext gb;

    for (i = 0; i + 2 < nSize; i++)
    {
        code = (code << 8) + pBuffer[i];
        if ((code & 0xffffff00) == 0x100)
        {
            int ref_idc = (code >> 5) & 3;
            int type    = code & 0x1F;

            static const int8_t ref_zero[] = {
                2, 0, 0, 0, 0, -1, 1, -1,
                -1, 1, 1, 1, 1, -1, 2, 2,
                2, 2, 2, 0, 2, 2, 2, 2,
                2, 2, 2, 2, 2, 2, 2, 2
            };

            if (code & 0x80)    // forbidden_bit
            {
                return false;
            }

            if (ref_zero[type] == 1 && ref_idc)
            {
                return false;
            }
            if (ref_zero[type] == -1 && !ref_idc)
            {
                return false;
            }
            if (ref_zero[type] == 2)
            {
                if (!(code == 0x100 && !pBuffer[i + 1] && !pBuffer[i + 2]))
                {
                    res++;
                }
            }

            ret = init_get_bits8(&gb, pBuffer + i + 1, nSize - i - 1);
            if (ret < 0)
            {
                return false;
            }

            switch (type)
            {
                case 1:
                case 5:
                    get_ue_golomb_long(&gb);
                    if (get_ue_golomb_long(&gb) > 9U)
                    {
                        return false;
                    }
                    pps_id = get_ue_golomb_long(&gb);
                    if (pps_id > MAX_PPS_COUNT)
                    {
                        return false;
                    }
                    if (!pps_ids[pps_id])
                    {
                        break;
                    }

                    if (type == 1)
                    {
                        sli++;
                    }
                    else
                    {
                        idr++;
                    }
                    break;
                case 7:
                    skip_bits(&gb, 14);
                    if (get_bits(&gb, 2))
                    {
                        return false;
                    }
                    skip_bits(&gb, 8);
                    sps_id = get_ue_golomb_long(&gb);
                    if (sps_id > MAX_SPS_COUNT)
                    {
                        return false;
                    }
                    sps_ids[sps_id] = 1;
                    sps++;
                    break;
                case 8:
                    pps_id = get_ue_golomb_long(&gb);
                    if (pps_id > MAX_PPS_COUNT)
                    {
                        return false;
                    }
                    sps_id = get_ue_golomb_long(&gb);
                    if (sps_id > MAX_SPS_COUNT)
                    {
                        return false;
                    }
                    if (!sps_ids[sps_id])
                    {
                        break;
                    }
                    pps_ids[pps_id] = 1;
                    pps++;
                    break;
            }
        }
    }

    if (sps && pps && (idr || sli > 3) && res < (sps + pps + idr))
    {
        return true;    // 1 more than .mpg
    }

    return false;
}

/* 判断是否为h265数据 */
bool CDataTools::is_h265Format(const char* pchData, int nSize)
{
    // if ((pchData[4] & 0x7E) >> 1 == 32)
    // {
    //     return true;
    // }
    // return false;

    uint32_t nCode = -1;

    int nVps  = 0;
    int nSps  = 0;
    int nPps  = 0;
    int nIrap = 0;
    int i     = 0;

    for (i = 0; i < nSize - 1; i++)
    {
        nCode = (nCode << 8) + pchData[i];

        if ((nCode & 0xffffff00) == 0x100)
        {
            uint8_t nNal2 = pchData[i + 1];
            int     nType = (nCode & 0x7E) >> 1;

            if (nCode & 0x81)    // forbidden and reserved zero bits
            {
                return false;
            }

            if (nNal2 & 0xf8)    // reserved zero
            {
                return false;
            }

            switch (nType)
            {
                case NAL_VPS:
                {
                    nVps++;
                    break;
                }

                case NAL_SPS:
                {
                    nSps++;
                    break;
                }

                case NAL_PPS:
                {
                    nPps++;
                    break;
                }

                case NAL_BLA_N_LP:
                case NAL_BLA_W_LP:
                case NAL_BLA_W_RADL:
                case NAL_CRA_NUT:
                case NAL_IDR_N_LP:
                case NAL_IDR_W_RADL:
                {
                    nIrap++;
                    break;
                }
            }
        }
    }

    if (nVps && nSps && nPps && nIrap)
    {
        return true;
    }
    return false;
}

/* 解析-SPS-h264数据 */
bool CDataTools::parse_h264SPS(char* pchData, int nLen, int* pnWidth, int* pnHeight, int* pnFps)
{
    unsigned char* pData = (unsigned char*)malloc(nLen);
    memcpy(pData, (pchData) + 4, nLen - 4);

    h264_decode_sps(pData, nLen, pnWidth, pnHeight, pnFps);

    free(pData);
    pData = NULL;

    return true;
}

/* 解析-SPS-h265数据 */
bool CDataTools::parse_h265SPS(char* pchData, int nLen, int* pnWidth, int* pnHeight, int* pnFps)
{
    *pnWidth  = 1920;
    *pnHeight = 1080;
    *pnFps    = 30;

    return true;
}

/* 写入文件 */
int CDataTools::write_toFile(const char* pchFilePath, const char* pchJsonData)
{
    if (pchFilePath == NULL || pchJsonData == NULL)
    {
        printf("传入参数异常");
        return -1;
    }

    struct stat stFileStat = { 0 };
    if (stat(pchFilePath, &stFileStat) == 0)
    {
        if (S_ISDIR(stFileStat.st_mode))
        {
            printf("传入的文件路径为文件夹[%s]", pchFilePath);
            return -1;
        }
        else if (access(pchFilePath, W_OK) != 0)
        {
            if (chmod(pchFilePath, S_IWUSR) != 0)
            {
                printf("没有写权限，添加写权限失败[%s]", pchFilePath);
                return -1;
            }
        }
    }


    FILE* pFp = fopen(pchFilePath, "w+");
    if (pFp == NULL)
    {
        printf("打开文件失败[%s]", pchFilePath);
        return -1;
    }

    size_t nLen     = strlen(pchJsonData) + 1;
    size_t nWritten = fwrite(pchJsonData, sizeof(char), nLen, pFp);

    fclose(pFp);

    if (nWritten != nLen)
    {
        printf("写文件失败[%s]", pchFilePath);
        return -1;
    }

    return 0;
}

/* 读取文件 */
char* CDataTools::read_fromFile(const char* pchFilePath)
{
    if (pchFilePath == NULL)
    {
        printf("传入参数异常");
        return NULL;
    }

    struct stat stFileStat = { 0 };

    if (stat(pchFilePath, &stFileStat) != 0)
    {
        printf("文件[%s]信息异常[%s]", pchFilePath, strerror(errno));
        return NULL;
    }

    if (S_ISDIR(stFileStat.st_mode))
    {
        printf("传入的文件路径为文件夹[%s]", pchFilePath);
        return NULL;
    }

    FILE* pFp = fopen(pchFilePath, "r");
    if (pFp == NULL)
    {
        printf("打开文件失败[%s]", pchFilePath);
        return NULL;
    }

    size_t nSize       = stFileStat.st_size;
    char*  pchJsonData = (char*)malloc(nSize + 1);
    if (pchJsonData == NULL)
    {
        printf("创建空间失败");
        fclose(pFp);
        return NULL;
    }

    size_t nReadSize = fread(pchJsonData, sizeof(char), nSize, pFp);
    if (nReadSize != nSize)
    {
        printf("读取数据长度异常");
        free(pchJsonData);
        fclose(pFp);
        return NULL;
    }

    pchJsonData[nSize] = '\0';

    fclose(pFp);

    return pchJsonData;
}

/* 打印运行时间 */
void CDataTools::PrintMs(const char* pText)
{
    static clock_t lastTime = 0;
    clock_t        curTime  = clock();
    if (lastTime == 0)
    {
        lastTime = curTime;
        return;
    }
    long long lMs = 0;
    lMs           = ((double)(curTime - lastTime) / CLOCKS_PER_SEC) * 1000;
    if (pText != NULL)
    {
        printf("===========%s = %lldms\n", pText, lMs);
    }
    lastTime = clock();
}

/* 追加文件数据 */
int CDataTools::appendDataToFile(const char* pchFileName, const void* pData, size_t nDataSize)
{
    /* 打开文件以追加数据 */
    FILE* file = fopen(pchFileName, "ab");
    if (!file)
    {
        printf("无法打开文件 %s", pchFileName);
        return -1;
    }

    /* 写入数据到文件 */
    size_t bytesWritten = fwrite(pData, 1, nDataSize, file);
    if (bytesWritten != nDataSize)
    {
        printf("写入数据到文件 %s 失败", pchFileName);
        fclose(file);
        return -1;
    }

    /* 关闭文件 */
    fclose(file);

    return 0;
}

/* 打开文件并清空原有的数据，写入文件 */
int CDataTools::writeDataToFile(const char* pchFileName, const void* pData, size_t nDataSize)
{
    /* 打开文件以追加数据 */
    FILE* file = fopen(pchFileName, "w+");
    if (!file)
    {
        printf("无法打开文件 %s", pchFileName);
        return -1;
    }

    /* 写入数据到文件 */
    size_t bytesWritten = fwrite(pData, 1, nDataSize, file);
    if (bytesWritten != nDataSize)
    {
       printf("写入数据到文件 %s 失败", pchFileName);
        fclose(file);
        return -1;
    }

    /* 关闭文件 */
    fclose(file);

    return 0;
}

/* 删除空文件夹 */
bool CDataTools::deleteNullFolder(const std::string& strFolderPath)
{
	DIR* dir = opendir(strFolderPath.c_str());
	if (!dir) {
		return false; // 目录不存在
	}

	struct dirent* entry;
	bool isEmpty = true;
	while ((entry = readdir(dir)) != nullptr) {
		if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0) {
			isEmpty = false;
			break;
		}
	}
	closedir(dir);

	if (isEmpty) {
		if (rmdir(strFolderPath.c_str()) == 0) {
			return true;
		} else {
			return false; // 删除目录失败
		}
	} else {
		return false; // 目录不为空
	}
}

/* 创建目录 */
bool CDataTools::createFile(const std::string& strFilePath)
{
	// 检查文件是否已经存在
	if (std::ifstream(strFilePath.c_str()).good()) {
		return true;
	}

	// 获取文件的路径部分
	std::string dirPath = strFilePath.substr(0, strFilePath.find_last_of("/\\"));
	
	// 创建文件所在的目录
	if (!dirPath.empty() && access(dirPath.c_str(), F_OK) != 0) 
    {
		// 使用递归创建目录
		std::string command;
        command = "mkdir -p " + dirPath;
		if (system(command.c_str()) != 0) 
        {
			return false; // 创建目录失败
		}
	}

    #if 0

    // 获取文件的扩展名
	std::string extension = strFilePath.substr(strFilePath.find_last_of('.'));
	if (!extension.empty())
	{
		char command[1024] = {0};
		memset(command, 0, sizeof(command));
		snprintf(command, sizeof(command), "touch  \"%s\"", strFilePath.c_str());
		system(command);
		if (system(command) != 0)
		{
            dlog_error("执行命令[%s]失败",command);
			/*创建目录失败*/
			return false;
		}
		/*修改文件权限为 777*/
		memset(command, 0, sizeof(command));
		snprintf(command, sizeof(command), "chmod 777 \"%s\"", strFilePath.c_str());
		if (system(command) != 0)
		{
            dlog_error("执行命令[%s]失败",command);
		}
	}
    #endif


	// 检查文件是否成功创建
	return std::ifstream(strFilePath.c_str()).good();
}
