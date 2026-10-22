#ifndef PUSHURLBASE_H
#define PUSHURLBASE_H

#include <iostream>
#include <string>
#include <atomic>
#include <mutex>
#include "sdk_packet_write.h"
#include "media_ffmpeg.h"
#include "media_define_type.h"
#include "mediaAVbuffer.h"
#include "osa_thread.h"
#include "os_ringBuf.h"

using namespace std;

class PushUrlBase
{
public:
    PushUrlBase(bool isHaveAudio = false);
    ~PushUrlBase();

    enum LiveStatus
    {
        LIVE_NULL = 0,  //断开，未推流
        LIVE_NORMAL,    //正常直播中
        LIVE_RECONNECT  //重连中
    };

    /* 发送数据 */
    int sendFrame(AVMediaPacket_S* packet);

    /* 获取当前直播状态 */
    LiveStatus getCurrentStatus();

    /* 推流 */
    int liveUrl(std::string url);

    /* 使能推流 */
    int enableLive(bool enable);

 private:

    /* 码流信息 */
    struct AVMess
    {
        int width;
        int height;
        double frameRate;
        int64_t videoBitRate;
        int audioSampleRate;
        int audioBitRate;
    };
    AVMess liveAvMess_;     //码流信息
    bool videoMessHave_;    //已经拿到视频信息
    bool audioMessHave_;    //已经拿到音频信息

	/* 处理线程 */
	class liveDealThr : public OSAThread
	{
	public:
    	liveDealThr(PushUrlBase* obj)
				:m_obj(obj){}
		~liveDealThr(){}

		virtual int run();

	private:
		PushUrlBase* m_obj;
	};

	int putFrameQue(AVMediaPacket_S* frame);
	int popFrameQue(AVMediaPacket_S* frame,int timeout);

    /* 连接服务器 */
    int pushurl_base_init(AVMess mess);

    /* 重连服务器 */
    int ffmpegReconnect(AVMess mess);

    /* 维护连接以及送数据 */
    int connectServerSendPacket(AVMediaPacket_S* CFrameData);

    /* 送ffmpeg */
    int sendFrameFfmpeg(AVMediaPacket_S* CFrameData);

    /* 停止推流 */
    int stop();

    /*添加adts头；
     * buffer->缓冲区；
     * bufferSize->包含7字节头信息 + aac裸流信息；
    */
    int audioHeaderAdts(unsigned char* buffer,int bufferSize);

private:

    CMedia_File_Inputparam_t stIntputparm;
    std::string m_chPushUrl;                //推流地址
    sdk_packet_write* CSdkPacketPush;
    LiveStatus currentStatus_;
    int64_t reconnectTime_;                 //重连开始的时间
    std::atomic<bool> isEnable_;            //是否使能推流
    std::string updateUrl_;                 //新的url
    std::atomic<bool> isUpdataUrl_;         //是否更新了新的url
    std::mutex updateMutex_;
    unsigned char* audioBuffer_;
    int audioBufferSize_;

	OS_ringBufHndl m_ringQueBuff;
	AVMediaPacket_S m_packet[30];
	std::atomic<bool> m_isExit;
	bool m_isHaveAudio;
	liveDealThr m_liveDealThr;

};
#endif // PUSHURLBASE_H









