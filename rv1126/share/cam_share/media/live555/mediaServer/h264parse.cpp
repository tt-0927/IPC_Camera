#include<stdio.h>
#include <math.h>
#include "h264_parse.h"
#if 0
unsigned int Ue(char *pBuff, unsigned int nLen, unsigned int &nStartBit)
{
    //计算0bit的个数
    unsigned int nZeroNum = 0;
    while (nStartBit < nLen * 8)
    {
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
        {
            break;
        }
        nZeroNum++;
        nStartBit++;
    }
    nStartBit ++;


    //计算结果
    unsigned long dwRet = 0;
    for (unsigned int i=0; i<nZeroNum; i++)
    {
        dwRet <<= 1;
        if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
            dwRet += 1;
        }
        nStartBit++;
    }
    return (1 << nZeroNum) - 1 + dwRet;
}


int Se(char *pBuff, unsigned int nLen, unsigned int &nStartBit)
{


	int UeVal=Ue(pBuff,nLen,nStartBit);
	double k=UeVal;
	int nValue=ceil(k/2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2==0)
	{
		nValue=-nValue;
	}

	return nValue;


}


unsigned long u(unsigned int BitCount,char * buf,unsigned int &nStartBit)
{
    unsigned long dwRet = 0;
    for (unsigned int i=0; i<BitCount; i++)
    {
        dwRet <<= 1;
        if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
        {
            dwRet += 1;
        }
        nStartBit++;
    }
return dwRet;
}


bool h264_decode_seq_parameter_set(char * buf,unsigned int nLen,int &Width,int &Height)
{
	unsigned int StartBit=0;
	u(1,buf,StartBit);
	u(2,buf,StartBit);
	int nal_unit_type=u(5,buf,StartBit);
	if(nal_unit_type==7)
	{
		int profile_idc=u(8,buf,StartBit);
		u(1,buf,StartBit);//(buf[1] & 0x80)>>7;
		u(1,buf,StartBit);//(buf[1] & 0x40)>>6;
		u(1,buf,StartBit);//(buf[1] & 0x20)>>5;
		u(1,buf,StartBit);//(buf[1] & 0x10)>>4;
		u(4,buf,StartBit);
		u(8,buf,StartBit);

		Ue(buf,nLen,StartBit);

		if( profile_idc == 100 || profile_idc == 110 ||
		profile_idc == 122 || profile_idc == 144 )
		{
			int chroma_format_idc=Ue(buf,nLen,StartBit);
			if( chroma_format_idc == 3 )
			u(1,buf,StartBit);
			Ue(buf,nLen,StartBit);
			Ue(buf,nLen,StartBit);
			u(1,buf,StartBit);
			int seq_scaling_matrix_present_flag=u(1,buf,StartBit);


			if( seq_scaling_matrix_present_flag )
			{
				for( int i = 0; i < 8; i++ )
				{
					u(1,buf,StartBit);
				}
			}
		}

		Ue(buf,nLen,StartBit);
		int pic_order_cnt_type=Ue(buf,nLen,StartBit);
		if( pic_order_cnt_type == 0 )
		{
			Ue(buf,nLen,StartBit);
		}


		else if( pic_order_cnt_type == 1 )
		{
			u(1,buf,StartBit);
			Se(buf,nLen,StartBit);
			Se(buf,nLen,StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,StartBit);

			int *offset_for_ref_frame=new int[num_ref_frames_in_pic_order_cnt_cycle];
			for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
			offset_for_ref_frame[i]=Se(buf,nLen,StartBit);
			delete [] offset_for_ref_frame;
		}
		Ue(buf,nLen,StartBit);
		u(1,buf,StartBit);
		int pic_width_in_mbs_minus1=Ue(buf,nLen,StartBit);
		int pic_height_in_map_units_minus1=Ue(buf,nLen,StartBit);

		Width=(pic_width_in_mbs_minus1+1)*16;
		Height=(pic_height_in_map_units_minus1+1)*16;

		return true;
	}
	return false;
}
#endif

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	//计算0bit的个数  
	UINT nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余  
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;


	//计算结果  
	DWORD dwRet = 0;
	for (UINT i = 0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}


int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00  
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}


DWORD u(UINT BitCount, BYTE * buf, UINT &nStartBit)
{
	DWORD dwRet = 0;
	for (UINT i = 0; i<BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
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
void de_emulation_prevention(BYTE* buf, unsigned int* buf_size)
{
	unsigned int i = 0, j = 0;
	BYTE* tmp_ptr = NULL;
	unsigned int tmp_buf_size = 0;
	int val = 0;
	
	
	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i<(tmp_buf_size - 2); i++)
	{
		//check for 0x000003  
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03  
			for (j = i + 2; j<tmp_buf_size - 1; j++)
				tmp_ptr[j] = tmp_ptr[j + 1];

			//and so we should devrease bufsize  
			(*buf_size)--;
		}
	}
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
#include <stdlib.h>
#include <string.h>
bool h264_decode_sps(BYTE * buf_, unsigned int nLen, unsigned int &width, unsigned int &height, unsigned int &fps)
{
	if (nLen <= 5)
	{
		//nslog(NS_INFO, "it is err de_emulation_prevention size=%d\n");
		return false;
	}
	
	bool ret = false;
	UINT StartBit = 0;

	BYTE* buf = (BYTE*)malloc(nLen);
	if(buf == NULL)
	{
		return false;
	}
	memcpy(buf,buf_,nLen);

	/*会把防竞争符去掉，改变流信息*/
	de_emulation_prevention(buf, &nLen);

	int forbidden_zero_bit = u(1, buf, StartBit);
	int nal_ref_idc = u(2, buf, StartBit);
	int nal_unit_type = u(5, buf, StartBit);
	if (nal_unit_type == 7)
	{
		int profile_idc = u(8, buf, StartBit);
		int constraint_set0_flag = u(1, buf, StartBit);//(buf[1] & 0x80)>>7;  
		int constraint_set1_flag = u(1, buf, StartBit);//(buf[1] & 0x40)>>6;  
		int constraint_set2_flag = u(1, buf, StartBit);//(buf[1] & 0x20)>>5;  
		int constraint_set3_flag = u(1, buf, StartBit);//(buf[1] & 0x10)>>4;  
		int reserved_zero_4bits = u(4, buf, StartBit);
		int level_idc = u(8, buf, StartBit);

		UINT seq_parameter_set_id = Ue(buf, nLen, StartBit);

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			int chroma_format_idc = Ue(buf, nLen, StartBit);
			if (chroma_format_idc == 3)
				DWORD residual_colour_transform_flag = u(1, buf, StartBit);
			/*UINT bit_depth_luma_minus8 =*/ Ue(buf, nLen, StartBit);
			/*UINT bit_depth_chroma_minus8 =*/ Ue(buf, nLen, StartBit);
			/*DWORD qpprime_y_zero_transform_bypass_flag = */u(1, buf, StartBit);
			DWORD seq_scaling_matrix_present_flag = u(1, buf, StartBit);

			int seq_scaling_list_present_flag[8];
			if (seq_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 8; i++) {
					seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
				}
			}
		}

		/*UINT log2_max_frame_num_minus4 =*/ Ue(buf, nLen, StartBit);
		UINT pic_order_cnt_type = Ue(buf, nLen, StartBit);
		if (pic_order_cnt_type == 0)
			UINT log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
		else if (pic_order_cnt_type == 1)
		{
			/*DWORD delta_pic_order_always_zero_flag =*/ u(1, buf, StartBit);
			/*int offset_for_non_ref_pic = */Se(buf, nLen, StartBit);
			/*int offset_for_top_to_bottom_field = */Se(buf, nLen, StartBit);
			UINT num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, StartBit);

			int *offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
			delete[] offset_for_ref_frame;
		}
		/*UINT num_ref_frames = */Ue(buf, nLen, StartBit);
		DWORD gaps_in_frame_num_value_allowed_flag = u(1, buf, StartBit);
		UINT pic_width_in_mbs_minus1 = Ue(buf, nLen, StartBit);
		UINT pic_height_in_map_units_minus1 = Ue(buf, nLen, StartBit);

		width = (pic_width_in_mbs_minus1 + 1) * 16;
		height = (pic_height_in_map_units_minus1 + 1) * 16;

		DWORD frame_mbs_only_flag = u(1, buf, StartBit);
		if (!frame_mbs_only_flag)
			DWORD mb_adaptive_frame_field_flag = u(1, buf, StartBit);

		DWORD direct_8x8_inference_flag = u(1, buf, StartBit);
		DWORD frame_cropping_flag = u(1, buf, StartBit);
		if (frame_cropping_flag)
		{
			UINT frame_crop_left_offset = Ue(buf, nLen, StartBit);
			UINT frame_crop_right_offset = Ue(buf, nLen, StartBit);
			UINT frame_crop_top_offset = Ue(buf, nLen, StartBit);
			UINT frame_crop_bottom_offset = Ue(buf, nLen, StartBit);
		}
		DWORD vui_parameter_present_flag = u(1, buf, StartBit);
		if (vui_parameter_present_flag)
		{
			DWORD aspect_ratio_info_present_flag = u(1, buf, StartBit);
			if (aspect_ratio_info_present_flag)
			{
				DWORD aspect_ratio_idc = u(8, buf, StartBit);
				if (aspect_ratio_idc == 255)
				{
					DWORD sar_width = u(16, buf, StartBit);
					DWORD sar_height = u(16, buf, StartBit);
				}
			}
			DWORD overscan_info_present_flag = u(1, buf, StartBit);
			if (overscan_info_present_flag)
				DWORD overscan_appropriate_flagu = u(1, buf, StartBit);
			DWORD video_signal_type_present_flag = u(1, buf, StartBit);
			if (video_signal_type_present_flag)
			{
				DWORD video_format = u(3, buf, StartBit);
				DWORD video_full_range_flag = u(1, buf, StartBit);
				DWORD colour_description_present_flag = u(1, buf, StartBit);
				if (colour_description_present_flag)
				{
					DWORD colour_primaries = u(8, buf, StartBit);
					DWORD transfer_characteristics = u(8, buf, StartBit);
					DWORD matrix_coefficients = u(8, buf, StartBit);
				}
			}
			DWORD chroma_loc_info_present_flag = u(1, buf, StartBit);
			if (chroma_loc_info_present_flag)
			{
				UINT chroma_sample_loc_type_top_field = Ue(buf, nLen, StartBit);
				UINT chroma_sample_loc_type_bottom_field = Ue(buf, nLen, StartBit);
			}
			DWORD timing_info_present_flag = u(1, buf, StartBit);

			if (timing_info_present_flag)
			{
				DWORD num_units_in_tick = u(32, buf, StartBit);
				DWORD time_scale = u(32, buf, StartBit);
				fps = time_scale / num_units_in_tick;
				DWORD fixed_frame_rate_flag = u(1, buf, StartBit);
				if (fixed_frame_rate_flag)
				{
					fps = fps / 2;
				}
			}
		}
		ret = true;
	}
	else
	{
		ret = false;
	}

	if(buf)
	{
		free(buf);
		buf = NULL;
	}
	return ret;
}


