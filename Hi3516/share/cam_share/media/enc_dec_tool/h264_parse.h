#ifndef H264_PARSE_h
#define H264_PARSE_h
#include <stdio.h>    

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

typedef struct
{
    unsigned int profile_idc;
    unsigned int level_idc;
    
    unsigned int width;
    unsigned int height;
    unsigned int fps;       //SPS中可能不包含FPS信息
} SpsInfo_S;
/**
 解析SPS数据信息
 @param data SPS数据内容，需要Nal类型为0x7数据的开始(比如：67 42 00 28 ab 40 22 01 e3 cb cd c0 80 80 a9 02)
 @param dataSize SPS数据的长度
 @param info SPS解析之后的信息数据结构体
 @return success:1，fail:0
 */
int h264_parse_sps(const unsigned char *data, unsigned int dataSize, SpsInfo_S *info);


#ifdef __cplusplus
}
#endif	// __cplusplus
#endif//H264_PARSE_h
