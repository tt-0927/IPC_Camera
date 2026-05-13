/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-07-17 15:48:13
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2023-07-17 15:49:22
 * @FilePath: \tv-620hu\code\osshare\live555\mediaServer\aac_server_subsession.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _AAC_SERVER_SUBSESSION_H
#define _AAC_SERVER_SUBSESSION_H
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "aac_audio_source.h"
class aacAudioServerMediaSubsession: public OnDemandServerMediaSubsession
{
public:
  static aacAudioServerMediaSubsession*
  createNew(UsageEnvironment& env, Boolean reuseFirstSource ,  Audio_Source_Info_t& aac_sourec_info);

protected:
  aacAudioServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource, Audio_Source_Info_t& aac_sourec_info);
      // called only by createNew();
  virtual ~aacAudioServerMediaSubsession();

protected: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);
private:
  Audio_Source_Info_t m_aacSouceInfo;
};
#endif
