#pragma once
#include "media_define_type.h"
#include "ffmpeg_base.h"
#include <iostream>
#include <atomic>
#include <mutex>

typedef int(*write_packetFunCallBack)(void *opaque, uint8_t *buf, int buf_size);
typedef struct Media_File_Inputparam
{
	int video_frame_rate;//如果是0就默认解析，最好要有，有些裸流不带帧率信息无法解析
	int audio_sample_rate;//如果是0就默认解析
	int audio_bit_rate;//如果是0就默认96K

	int video_bit_rate;//如果是0就默认2M
	int width;//如果是0就默认自己解析
	int height;//如果是0就默认自己解析
	char url[2048];
	enum AVCodecID audio_codec_id;
	enum AVCodecID video_codec_id;

	Capture_Data_Type_t write_type;//参考帧，默认音视频都写

	write_packetFunCallBack write_pack_fun;//如过不为空代表取内存，如果为空代表录制
	void *userinputparam;//用于回调写内存的时候帮用户保存;
}CMedia_File_Inputparam_t;



class sdk_packet_write
{
public:
    sdk_packet_write(std::string name = "");
	~sdk_packet_write();
	int sdk_init_mediafile(CMedia_File_Inputparam_t intputparm);

     /* frame不自带时间戳使用 */
	int sdk_write_frame(Capture_CallBack_Data_t* framedata);
     /* frame自带时间戳使用 */
    int sdk_write_frame2(Capture_CallBack_Data_t* pstFramedata);

	int sdk_write_frame_trailer();//写文件尾
	int sdk_add_video_stream();
	int sdk_add_audio_stream();

    int sdk_get_time();

    void setObjectName(std::string name);

    std::string objectName();

    /* 暂停 */
    int sdk_pause();

private:

	int sdk_init_header(Capture_CallBack_Data_t* framedata);
    /* frame不自带时间戳使用 */
	int sdk_fill_packet(AVPacket &writepkt, Capture_CallBack_Data_t* framedata, bool &packdatafree);
    /* frame自带时间戳使用 */
    int sdk_fill_packet2(AVPacket &writepkt, Capture_CallBack_Data_t* framedata, bool &packdatafree);
	int sdk_av_sync(Capture_CallBack_Data_t* framedata);

    int sdk_compate_pts(int64_t reference_pts, int64_t nowpts, \
                        int64_t millsecond, int* continue_count_lost, \
                        int continue_count, int64_t *ouputdifference_value);

    struct AVSyncInfo
    {
        int64_t startTimer;    //开始录制系统的时间
        int writeFrameNum;      //>0需要补的帧数 =0，正常，<0:需要抛弃的帧数
        int jumbFrameNum;       //跳过的帧数
        int continue_lost_frame;    //连续缺的帧数
        int audio_skip_count;       //音频使用，多久统计一次
        int64_t frameCount;       //每次暂停都会清空，重现开始之后开始统计帧数
    };

    /* 判断是否需要补帧/丢帧
     * d_value:实际帧数跟理论帧数的差值；
     * lostframe：实际帧数跟理论帧数的差值大于该值后需要统计，否则不需要统计
     * continue_threshold:实际帧数跟理论帧数的差值连续几次超过统计值；
     * continue_count_lost：记录当前总共的差值；
    */
    int deal_frame_timestamp(int d_value, int lostframe, \
                             int continue_threshold, int* continue_count_lost);
    //检查视频的pts
    int checkVideoPts(AVSyncInfo& sync);
    //检查音频的pts
    int checkAudioPts(AVSyncInfo& sync);

    /* flv格式需要配置aac的等级 */
    void make_dsi(unsigned int sampling_frequency_index, \
                  unsigned int channel_configuration, unsigned char* dsi);

private:

	AVFormatContext	* m_pFormatCtx;
    std::mutex m_ctxMtx;
	int64_t m_video_pts;    //表示此帧的pts,单位是ms
	int64_t m_audio_pts;    //表示此帧的pts,单位是ms
	int64_t m_video_count;
	int64_t m_audio_count;

	int64_t m_per_audioaddpts;
	int64_t m_per_videoaddpts;
	CMedia_File_Inputparam_t m_inputparam;

	enum AVSampleFormat m_audiosample_fmt;
	enum AVPixelFormat m_video_format;
	int	m_audio_channel;
	int m_first_Iframe; // -1:写文件头失败，0：没初始化音视频轨道，1：初始化完成
	int m_video_index;
	int m_audio_index;
    int64_t m_nFirstVideoPts;
    int64_t m_nFirstAudioPts;

	int m_continue_count_lost;
	int m_gop_size;//I
	int m_IframeNem;

	//需要补帧了
	int m_write_frame_count_video;
    std::atomic<int> recordTime_;

    AVBitStreamFilterContext* m_bsfc;

    AVSyncInfo videoSync_;
    AVSyncInfo audioSync_;
    bool m_isFlv;
    std::string m_objectName;
};

