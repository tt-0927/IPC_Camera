#ifndef RK_COMPARE_IMAGE
#define RK_COMPARE_IMAGE

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _COMPAREIMAGEPARAM_{
	    /* 两个视频流或图片，char 的BGR，长度为nLength1、nLength2 */
	    char *pImage1Buffer;
	    int nLength1;
	    char *pImage2Buffer;
	    int nLength2;
	    /*视频帧或者图片的大小，用于检查上输入的数据是否符合条件；不符合，直接exit(0) */
	    int nWidth,nHeight;
	    /* 像素差阈值，当差值小于指定的nThreshold，可以忽略该像素点。 */ 
	    int nThreshold;
	}CompareImageParam_S;


	/* 图片对比函数，输入两张图片地址，返回float的百分比 */
	float compareImage_Path(char* strImagePath1, char* strImagePath2);

	/*功能： 图片对比函数，输入两张同样大小的cv::Mat图片，(可以指定像素差值)返回float的百分比
	  * return：返回float的百分比，即变化像素点占总像素点的百分比。
	 */
	float compareImage_Image_Threshold(CompareImageParam_S stCompareParam);

#ifdef __cplusplus
}
#endif

#endif // RK_COMPARE_IMAGE
