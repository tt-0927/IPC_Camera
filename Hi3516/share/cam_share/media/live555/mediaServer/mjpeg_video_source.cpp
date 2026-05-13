/*
 * @FilePath: mjpeg_video_source.cpp
 * @Author: yangwenyao
 * @Date: 2023-06-13 11:08:38
 * @LastEditors: ywy
 * @LastEditTime: 2023-09-19 15:51:13
 * @Descripttion: mjpeg rtp source
 */
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "mjpeg_video_source.h"
#include "assert.h"
enum
{
    START_MARKER = 0xFF,
    SOI_MARKER   = 0xD8,
    JFIF_MARKER  = 0xE0,
    CMT_MARKER   = 0xFE,
    DQT_MARKER   = 0xDB,
    SOF_MARKER   = 0xC0,
    DHT_MARKER   = 0xC4,
    SOS_MARKER   = 0xDA,
    EOI_MARKER   = 0xD9,
    DRI_MARKER   = 0xDD
};

//  extern void __attribute__ ((noinline)) memcpy_neon_pld(void *dest, const void *src, size_t n);

/*!*****************************************************************************
 * \brief Constructor
*************************************************************************************/
MJPEG_Video_Source::MJPEG_Video_Source(UsageEnvironment& env, FramedSource* source) : 
		JPEGVideoSource(env), 
		fSource(source),
		fType(1),
		fWidth(0), 
		fHeight(0), 
		fLastQFactor(255),
		fWidthPixels(0), fHeightPixels(0),
		fPrecision(0)
{

}
 
/*!*****************************************************************************
 * \brief Destructor
*************************************************************************************/
MJPEG_Video_Source::~MJPEG_Video_Source()
{
	Medium::close(fSource);
}

/*!*****************************************************************************
 * \brief return quantification tables
*************************************************************************************/
u_int8_t const* MJPEG_Video_Source::quantizationTables( u_int8_t& precision, u_int16_t& length )
{
	length = fQtableLength;
	precision = fPrecision;
	return fQuantizationTable;
	
}

void MJPEG_Video_Source::doGetNextFrame()
{
	// m_pToken = envir().taskScheduler().scheduleDelayedTask(m_toDelay, getNextFrame, this);
	if(fSource)
    {
        fSource->getNextFrame(fTo, fMaxSize,
                              afterGettingFrame, this,
                              FramedSource::handleClosure, this);
    }
}
void MJPEG_Video_Source::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
        struct timeval presentationTime, unsigned durationInMicroseconds)
{
    MJPEG_Video_Source* source = (MJPEG_Video_Source*) clientData;
    source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void MJPEG_Video_Source::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
        struct timeval presentationTime, unsigned durationInMicroseconds)
{
	int soi = 0;
	int sos = 0;
	int dataLen = 0;

	unsigned char* pFrame = NULL;
	unsigned char* pFrameEnd = NULL;
	do
	{
		fFrameSize = 0;
		fNumTruncatedBytes = 0;

		pFrame = fTo;
		pFrameEnd = fTo + frameSize - 1;

		if(START_MARKER != *(pFrameEnd - 1) || EOI_MARKER != *pFrameEnd)
		{
			// EOI
			envir() << "WW_JPEGVideoStreamSource::afterGettingFrame1: JPEG has no EOI_MARKER\n";
			break;
		}
		pFrameEnd--;
		sos = 0;

		while(pFrame < pFrameEnd && 0 == sos)
		{

			if(START_MARKER != *pFrame)
			{
				printf("WW_JPEGVideoStreamSource::afterGettingFrame1: No START_MARKER, *pFrame = 0x%x\n", (unsigned int) (*pFrame));
				break;
			}

			pFrame++;
			switch(*pFrame)
			{
				case SOI_MARKER: // SOI
				{
					pFrame++;
					soi = 1;
					fQtableLength = 0;
					break;
				}
				case JFIF_MARKER: //APP0 (JFIF segment marker)
				case 0xE1:
				case 0xE2:
				case 0xE3:
				case 0xE4:
				case 0xE5:
				case 0xE6:
				case 0xE7:
				case 0xE8:
				case 0xE9:
				case 0xEA:
				case 0xEB:
				case 0xEC:
				case 0xED:
				case 0xEE:
				case 0xEF:
				{
					pFrame++;
					int nAppLength = 0;
					nAppLength = (pFrame[0] << 8) | pFrame[1];
					pFrame += nAppLength;
					break;
				}
				case DQT_MARKER: //DQT (Define Quantization Table)
				{
					pFrame++;
					int nIndex =0;
					int nIndex2 = 0;
					int nDqtLength = 0;
					unsigned short u16DQtNum = 0; //0:DQT Y, 1:DQT Cb/Cr
					unsigned short u16DqtPrecision = 0;

					nDqtLength = (pFrame[0] << 8) | pFrame[1];
					// assert(67 == nDqtLength);

					u16DQtNum = pFrame[2] & 0x0F;
					u16DqtPrecision = pFrame[2] >> 4;
					assert(0 == u16DqtPrecision);

					nIndex2 = fQtableLength;

					if(0 == u16DqtPrecision)
					{
						fPrecision &= ~(0x1 << u16DQtNum);
						fQtableLength += 64;
					}
					else if(1 == u16DqtPrecision)
					{
						fPrecision |= (0x1 << u16DQtNum);
						fQtableLength += (64 * 2);
					}

					if((nIndex + 3 + fQtableLength) < frameSize)
					{
						for(nIndex = 3; nIndex2 < fQtableLength; nIndex2++)
						{
							fQuantizationTable[nIndex2] = pFrame[nIndex++];
						}
						// printf("\033[33m fQtableLength = %d\033[0m\n", fQtableLength);
					}
					pFrame += nDqtLength;
					#if 0
                        envir() << "Define Quantization Table: " << u16DQtNum << "\n";
                        envir() << "fQtableLength = " << fQtableLength << "\n";
                        envir() << "fPrecision = " << fPrecision << "\n";
					#endif
					break;
				}
				
				case SOF_MARKER: //SOF0 (Start Of Frame 0)
				{
					int nSofLen = 0;
					unsigned int u16Index = 2;                        
					u_int8_t dataPrecision = 0;
					u_int8_t numOfComponents = 0;
					u_int8_t componentId[3] = {0};
					u_int8_t samplingFactor[3] = {0};
					u_int8_t QtableNum[3] = {0};
					
					pFrame++;
					/* 正确解析 SOF 段的大端格式长度 */
					nSofLen = (pFrame[0] << 8) | pFrame[1];

					if((pFrame + nSofLen) > pFrameEnd) // 边界检查
					{
						envir() << "所报告的长度会超出缓冲区的范围，这意味着帧已损坏" << "\n";
						break; 
					}

					if((u16Index + 8) <  nSofLen)
					{
						dataPrecision = pFrame[u16Index++];
						assert(8 == dataPrecision);
						fHeightPixels = (pFrame[u16Index]<<8)|(pFrame[u16Index+1]);
						u16Index += 2;
						fWidthPixels = (pFrame[u16Index] << 8) | (pFrame[u16Index+1]);
						u16Index += 2;

						fWidth = (fWidthPixels + 7) / 8;
						fHeight = (fHeightPixels + 7) / 8;

						/* 最多传输2048宽高 */
                        if(fWidthPixels > 2048 || fHeightPixels > 2048)
						{
							fWidth = 0;
							fHeight = 0;
						}

						numOfComponents = pFrame[u16Index++];
						assert(3 == numOfComponents);  //YUV

						for(int j = 0; j < numOfComponents; j++)
						{
							componentId[j] = pFrame[u16Index++];
							samplingFactor[j] = pFrame[u16Index++];
							QtableNum[j] = pFrame[u16Index++];
						}

						
						if((componentId[0] == 1 && samplingFactor[0] == 0x21 && QtableNum[0] == 0) &&
								(componentId[1] == 2 && samplingFactor[1] == 0x11 && QtableNum[1] == 1) &&
								(componentId[2] == 3 && samplingFactor[2] == 0x11 && QtableNum[2] == 1))
						{
							fType = 0;  // YUV 4:2:2
						}
						else if((componentId[0] == 1 && samplingFactor[0] == 0x22 && QtableNum[0] == 0) &&
								(componentId[1] == 2 && samplingFactor[1] == 0x11 && QtableNum[1] == 1) &&
								(componentId[2] == 3 && samplingFactor[2] == 0x11 && QtableNum[2] == 1))
						{
							fType = 1;  // YUV 4:2:0
						}

					}

					pFrame += nSofLen;
					break;
				}
				
				case DHT_MARKER: //DHT( Define Huffman Table)
				{
					// DHT Y-DC diff, Y-AC-Coef, Cb/Cr-DC diff, Cb/Cr-AC-Coef
					int nDCTLength = 0;

					pFrame++;
					nDCTLength = (pFrame[0] << 8) | pFrame[1];
					pFrame += nDCTLength;

					break;
				}

				case DRI_MARKER: // DRI (Define Restart Interval)
				{
					int nDriLength = 0;
					
					pFrame++; // Move past the DRI marker itself
					nDriLength = (pFrame[0] << 8) | pFrame[1];
					pFrame += nDriLength; // Skip the entire DRI segment (marker + length field + data)
					
					break;
				}

				case CMT_MARKER: // COM (Comment marker 0xFFFE)
				{
					int cmtLength = 0;
					pFrame++; // Move past the COM marker itself
					cmtLength = (pFrame[0] << 8) | pFrame[1];
					pFrame += cmtLength; // Skip the entire comment segment
					break;
				}

				case SOS_MARKER: //SOS (Start Of Scan)
				{
					int SOSLength = 0;
					//envir() << "Start Of Scan\n";

					pFrame++;
					SOSLength = (pFrame[0] << 8) | pFrame[1];
					pFrame += SOSLength;

					//envir() << "SOSLength = " << SOSLength << "\n";

					sos = 1;

					break;
				}
				case EOI_MARKER: //EOI

					envir() << "End of Image\n";
					break;
				default:
					break;
			}
			
				
			if(0 == soi)
			{
				envir() << "WW_JPEGVideoStreamSource::afterGettingFrame1: JPEG has no SOI_MARKER\n";
				break;
			}
		}

		if(0 == sos)
		{
			envir() << "WW_JPEGVideoStreamSource::afterGettingFrame1: JPEG has no SOS_MARKER\n";
			break;
		}
		
		unsigned int scanDataSize = pFrameEnd - pFrame;

		if (scanDataSize > fMaxSize) {
			// This shouldn't happen if fMaxSize is large enough, but it's a good safety check.
			fNumTruncatedBytes = scanDataSize - fMaxSize;
			scanDataSize = fMaxSize;
		}

        // Use memmove because the source (pFrame) and destination (fTo) buffers overlap.
        // It safely handles moving the data to the start of the buffer.
		memmove(fTo, pFrame, scanDataSize);

		fFrameSize = frameSize;
		fNumTruncatedBytes = numTruncatedBytes;
		fPresentationTime = presentationTime;
		fDurationInMicroseconds = durationInMicroseconds;
	}
	while(0);

	afterGetting(this);
}