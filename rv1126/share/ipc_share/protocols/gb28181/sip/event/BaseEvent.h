/*
 * @Author       : EasonLu
 * @Date         : 2025-02-13 10:18:34
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2025-05-10 16:05:18
 * @FilePath     : BaseEvent.h
 * @Description  : 事件处理基类
 */
#pragma once
#include "MANSCDP.h"
#include "SipEvent.hpp"
#include "pugixml.hpp"
namespace SIP
{
    class Device;
    class BaseEvent
    {
    public:
        typedef std::shared_ptr<BaseEvent> ptr;
        BaseEvent() = default;
        virtual ~BaseEvent() = default;

        virtual bool Handle(const SipEvent::Ptr &e, pugi::xml_document &doc);

        virtual bool Handle(const SipEvent::Ptr &e) { return false; }

        void SetDeivce(std::shared_ptr<Device> pDevice) { m_pDevice = pDevice; }

        int SendResponse(const SipEvent::Ptr &e, int nStatus);

        int SendMessage(const SipEvent::Ptr &e, const std::string &strBody);
        int SendMessageWithCallID(const SipEvent::Ptr &e,const std::string &strBody);//add by longll
        std::string format_xml(const std::string &xml);  //add 
        /**
         * @brief  从请求头中读取Expires字段的数据
         * @param  [Ptr] &e - SIP事件
         * @return [int] <0 则获取失败
         * @author EasonLu
         * @note   0为关闭,>0为有效期(单位秒)
         */
        int GetExpires(const SipEvent::Ptr &e);

        int GetEvent(const SipEvent::Ptr &e, std::string &strEvent);

        int ParseHeader(const SipEvent::Ptr &e);

        /**
         * @brief  解析请求中的URI
         * @param  [Ptr] &e - SIP事件
         * @param  [string] &strFromUri - From URI
         * @param  [string] &strToUri - To URI
         * @return [*]
         * @author EasonLu
         * @note
         */
        int GetRequestURI(const SipEvent::Ptr &e,
                          std::string &strFromUri,
                          std::string &strToUri);

    protected:
        int SendResponseAndGetAddress(eXosip_t *excontext, int tid, int status, std::string &address, uint16_t &port);
        int SendCallAck(eXosip_t *excontext, int did);
        int GetStatusCodeFromResponse(osip_message_t *response);
        std::string GetMsgIDFromRequest(osip_message_t *request);
        int SendResponse(eXosip_t *excontext, const std::string &strToUri, const std::string &strFromUri, const std::string &strBody);

    protected:
        /* 设备数据 */
        std::shared_ptr<Device> m_pDevice = nullptr;
        /* xml解析对象 */
        pugi::xml_document m_doc;
        /* 当前事件的数据头解析结构 */
        manscdp_msgbody_header_t m_header;
    };
}