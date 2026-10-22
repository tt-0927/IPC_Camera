#ifndef H264_PARSE_HH
#define H264_PARSE_HH
typedef  unsigned int UINT;
typedef  unsigned char BYTE;
typedef  unsigned long DWORD;
bool h264_decode_sps(BYTE * buf_, unsigned int nLen, unsigned int &width, unsigned int &height, unsigned int &fps);

// bool h264_decode_sps(BYTE * buf_, unsigned int nLen, unsigned int &width, unsigned int &height, unsigned int &fps);

#endif
