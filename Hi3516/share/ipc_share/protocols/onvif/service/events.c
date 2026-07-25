/**
 * @file events.c
 * @author tianl (tianl@kfb.cn)
 * @date 2025-09-23
 * 
 * @brief onvif events服务接口
 */
#include "onvif_server_wrapper.h"

//#define ONVIF_SUB_TIMEOUT (30*24*60*60) // 订阅事件过期时间 1个月
#define ONVIF_SUB_TIMEOUT (30*60) // 订阅事件过期时间 30分钟

#define ONVIF_ALARM_TRUE        "true"          /* 报警事件触发的值 */
#define ONVIF_ALARM_FALSE       "false"         /* 报警事件触发的值 */

// 枚举：XML 中的 TopicSet 顶层模块（与 XML 节点一一对应）
typedef enum 
{
    ONVIF_TOPIC_MODULE_VIDEO_SOURCE = 0,  // <tns1:VideoSource>
    ONVIF_TOPIC_MODULE_DEVICE,            // <tns1:Device>
    ONVIF_TOPIC_MODULE_USER_ALARM,        // <tns1:UserAlarm>
    ONVIF_TOPIC_MODULE_RULE_ENGINE,       // <tns1:RuleEngine>
    ONVIF_TOPIC_MODULE_AUDIO_ANALYTICS,   // <tns1:AudioAnalytics>
    ONVIF_TOPIC_MODULE_CONFIGURATION,     // <tns1:Configuration>
    ONVIF_TOPIC_MODULE_RECORDING_CONFIG,  // <tns1:RecordingConfig>
    ONVIF_TOPIC_MODULE_MONITORING,        // <tns1:Monitoring>
    ONVIF_TOPIC_MODULE_MEDIA,             // <tns1:Media>
    ONVIF_TOPIC_MODULE_MAX                // 枚举边界（总模块数）
} ONVIFTopicModule_E;

// ========================== 1. <tns1:VideoSource> 模块完整 XML 宏 ==========================
#define ONVIF_XML_MODULE_VIDEO_SOURCE \
"<tns1:VideoSource wstop:topic=\"true\">\r\n" \
"  <MotionAlarm wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"true\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Source\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:SimpleItemDescription Name=\"State\" Type=\"xs:boolean\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </MotionAlarm>\r\n" \
"  <ImageTooDark wstop:topic=\"false\">\r\n" \
"    <ImagingService wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"Source\" Type=\"tt:ReferenceToken\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"State\" Type=\"xs:boolean\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </ImagingService>\r\n" \
"  </ImageTooDark>\r\n" \
"</tns1:VideoSource>\r\n"

// ========================== 2. <tns1:Device> 模块完整 XML 宏 ==========================
#define ONVIF_XML_MODULE_DEVICE \
"<tns1:Device wstop:topic=\"true\">\r\n" \
"  <Trigger wstop:topic=\"true\">\r\n" \
"    <tnshik:AlarmIn wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"AlarmInToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"State\" Type=\"xs:boolean\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </tnshik:AlarmIn>\r\n" \
"    <DigitalInput wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"InputToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"LogicalState\" Type=\"xs:boolean\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </DigitalInput>\r\n" \
"    <Relay wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"RelayToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"LogicalState\" Type=\"tt:RelayLogicalState\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </Relay>\r\n" \
"  </Trigger>\r\n" \
"  <HardwareFailure wstop:topic=\"true\">\r\n" \
"    <tnshik:HardDiskFull wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"false\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"HardDiskNo\" Type=\"xs:int\" />\r\n" \
"        </tt:Source>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </tnshik:HardDiskFull>\r\n" \
"    <tnshik:HardDiskError wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"false\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"HardDiskNo\" Type=\"xs:int\" />\r\n" \
"        </tt:Source>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </tnshik:HardDiskError>\r\n" \
"  </HardwareFailure>\r\n" \
"  <tnshik:Network wstop:topic=\"true\">\r\n" \
"    <tnshik:EthernetBroken wstop:topic=\"true\" />\r\n" \
"    <tnshik:IPAddrConflict wstop:topic=\"true\" />\r\n" \
"  </tnshik:Network>\r\n" \
"</tns1:Device>\r\n"

// ========================== 3. <tns1:UserAlarm> 模块完整 XML 宏 ==========================
#define ONVIF_XML_MODULE_USER_ALARM \
"<tns1:UserAlarm wstop:topic=\"true\">\r\n" \
"  <tnshik:IllegalAccess wstop:topic=\"true\" />\r\n" \
"</tns1:UserAlarm>\r\n"

// ========================== 4. <tns1:RuleEngine> 模块完整 XML 宏 ==========================
#define ONVIF_XML_MODULE_RULE_ENGINE \
"<tns1:RuleEngine wstop:topic=\"false\">\r\n" \
"  <CellMotionDetector wstop:topic=\"false\">\r\n" \
"    <Motion wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"IsMotion\" Type=\"xs:boolean\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </Motion>\r\n" \
"  </CellMotionDetector>\r\n" \
"  <LineDetector wstop:topic=\"false\">\r\n" \
"    <Crossed wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"false\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"ObjectId\" Type=\"xs:integer\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </Crossed>\r\n" \
"  </LineDetector>\r\n" \
"  <FieldDetector wstop:topic=\"false\">\r\n" \
"    <ObjectsInside wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Key>\r\n" \
"          <tt:SimpleItemDescription Name=\"ObjectId\" Type=\"xs:integer\" />\r\n" \
"        </tt:Key>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"IsInside\" Type=\"xs:boolean\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </ObjectsInside>\r\n" \
"  </FieldDetector>\r\n" \
"  <TamperDetector wstop:topic=\"false\">\r\n" \
"    <Tamper wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"IsTamper\" Type=\"xs:boolean\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </Tamper>\r\n" \
"  </TamperDetector>\r\n" \
"</tns1:RuleEngine>\r\n"

// ========================== 5. <tns1:AudioAnalytics> 模块完整 XML 宏 ==========================
#define ONVIF_XML_MODULE_AUDIO_ANALYTICS \
"<tns1:AudioAnalytics wstop:topic=\"true\">\r\n" \
"  <Audio wstop:topic=\"true\">\r\n" \
"    <DetectedSound wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"false\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"AudioSourceConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"AudioAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\" />\r\n" \
"          <tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Key>\r\n" \
"          <tt:SimpleItemDescription Name=\"isSoundDetected\" Type=\"xs:boolean\" />\r\n" \
"        </tt:Key>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"UTCTime\" Type=\"xs:dateTime\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </DetectedSound>\r\n" \
"  </Audio>\r\n" \
"</tns1:AudioAnalytics>\r\n"


// ========================== 6. <tns1:Configuration> 模块完整 XML 宏 ==========================
#define ONVIF_XML_MODULE_CONFIGURATION \
"<tns1:Configuration wstop:topic=\"true\">\r\n" \
"  <Profile wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:Profile\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </Profile>\r\n" \
"  <VideoEncoderConfiguration wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:VideoEncoderConfiguration\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </VideoEncoderConfiguration>\r\n" \
"  <VideoSourceConfiguration wstop:topic=\"true\">\r\n" \
"    <MediaService wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"false\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:VideoSourceConfiguration\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </MediaService>\r\n" \
"  </VideoSourceConfiguration>\r\n" \
"  <AudioEncoderConfiguration wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:AudioEncoderConfiguration\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </AudioEncoderConfiguration>\r\n" \
"  <AudioSourceConfiguration wstop:topic=\"true\">\r\n" \
"    <MediaService wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"false\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:AudioSourceConfiguration\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </MediaService>\r\n" \
"  </AudioSourceConfiguration>\r\n" \
"  <AudioOutputConfiguration wstop:topic=\"true\">\r\n" \
"    <MediaService wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"false\">\r\n" \
"        <tt:Source>\r\n" \
"          <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"        </tt:Source>\r\n" \
"        <tt:Data>\r\n" \
"          <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:AudioOutputConfiguration\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </MediaService>\r\n" \
"  </AudioOutputConfiguration>\r\n" \
"  <MetadataConfiguration wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:MetadataConfiguration\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </MetadataConfiguration>\r\n" \
"  <PTZConfiguration wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:PTZConfiguration\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </PTZConfiguration>\r\n" \
"  <VideoAnalyticsConfiguration wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:VideoAnalyticsConfiguration\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </VideoAnalyticsConfiguration>\r\n" \
"</tns1:Configuration>\r\n"

// ========================== 7. <tns1:RecordingConfig> 模块完整 XML 宏 ==========================
#define ONVIF_XML_MODULE_RECORDING_CONFIG \
"<tns1:RecordingConfig wstop:topic=\"true\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\r\n" \
"  <JobState wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"true\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"RecordingJobToken\" Type=\"tt:RecordingJobReference\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:SimpleItemDescription Name=\"State\" Type=\"xs:string\" />\r\n" \
"        <tt:ElementItemDescription Name=\"Information\" Type=\"tt:RecordingJobStateInformation\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </JobState>\r\n" \
"  <RecordingJobConfiguration wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"RecordingJobToken\" Type=\"tt:RecordingJobReference\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:RecordingJobConfiguration\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </RecordingJobConfiguration>\r\n" \
"  <RecordingConfiguration wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"RecordingToken\" Type=\"tt:RecordingReference\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:RecordingConfiguration\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </RecordingConfiguration>\r\n" \
"  <TrackConfiguration wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"RecordingToken\" Type=\"tt:RecordingReference\" />\r\n" \
"        <tt:SimpleItemDescription Name=\"TrackToken\" Type=\"tt:TrackReference\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:ElementItemDescription Name=\"Configuration\" Type=\"tt:TrackConfiguration\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </TrackConfiguration>\r\n" \
"</tns1:RecordingConfig>\r\n"

// ========================== 8. <tns1:Monitoring> 模块完整 XML 宏      ==========================
#define ONVIF_XML_MODULE_MONITORING \
"<tns1:Monitoring wstop:topic=\"false\">\r\n" \
"  <ProcessorUsage wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"true\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:SimpleItemDescription Name=\"Value\" Type=\"xs:float\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </ProcessorUsage>\r\n" \
"  <OperatingTime wstop:topic=\"false\">\r\n" \
"    <LastReset wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"Status\" Type=\"xs:dateTime\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </LastReset>\r\n" \
"  </OperatingTime>\r\n" \
"  <OperatingTime wstop:topic=\"false\">\r\n" \
"    <LastReboot wstop:topic=\"true\">\r\n" \
"      <tt:MessageDescription IsProperty=\"true\">\r\n" \
"        <tt:Data>\r\n" \
"          <tt:SimpleItemDescription Name=\"Status\" Type=\"xs:dateTime\" />\r\n" \
"        </tt:Data>\r\n" \
"      </tt:MessageDescription>\r\n" \
"    </LastReboot>\r\n" \
"  </OperatingTime>\r\n" \
"</tns1:Monitoring>\r\n"

// ========================== 9. <tns1:Media> 模块完整 XML 宏 ==========================
#define ONVIF_XML_MODULE_MEDIA \
"<tns1:Media wstop:topic=\"false\">\r\n" \
"  <ProfileChanged wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Data>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </ProfileChanged>\r\n" \
"  <ConfigurationChanged wstop:topic=\"true\">\r\n" \
"    <tt:MessageDescription IsProperty=\"false\">\r\n" \
"      <tt:Source>\r\n" \
"        <tt:SimpleItemDescription Name=\"Token\" Type=\"tt:ReferenceToken\" />\r\n" \
"      </tt:Source>\r\n" \
"      <tt:Data>\r\n" \
"        <tt:SimpleItemDescription Name=\"Type\" Type=\"xs:string\" />\r\n" \
"      </tt:Data>\r\n" \
"    </tt:MessageDescription>\r\n" \
"  </ConfigurationChanged>\r\n" \
"</tns1:Media>\r\n"

// ==========================  TopicExpressionDialect 宏（事件主题表达式方言）==========================
// ONVIF 自定义方言（支持多主题批量订阅，优先使用）
#define ONVIF_TOPIC_EXPR_DIALECT_CONCRETE_SET  "http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet"
// OASIS 标准方言（仅支持单个主题订阅，兼容早期设备）
#define ONVIF_TOPIC_EXPR_DIALECT_OASIS_CONCRETE "http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\r\n"

// ==========================  MessageContentFilterDialect 宏（事件内容过滤方言）==========================
// ONVIF 事件内容过滤方言（用于过滤事件中的具体字段，如 State/LogicalState）
#define ONVIF_MSG_CONTENT_FILTER_DIALECT       "http://www.onvif.org/ver10/tev/messageContentFilter/ItemFilter\r\n"

// ==========================  MessageContentSchemaLocation 宏（事件内容Schema地址）==========================
// ONVIF 事件内容的 XML Schema 定义地址（设备解析事件内容的格式依据）
#define ONVIF_MSG_CONTENT_SCHEMA_LOCATION      "http://www.onvif.org/onvif/ver10/schema/onvif.xsd\r\n"

static const char *g_arrTopics[] = 
{
    ONVIF_XML_MODULE_VIDEO_SOURCE,
    ONVIF_XML_MODULE_DEVICE,
    ONVIF_XML_MODULE_USER_ALARM,
    ONVIF_XML_MODULE_RULE_ENGINE,
    ONVIF_XML_MODULE_AUDIO_ANALYTICS,
    ONVIF_XML_MODULE_CONFIGURATION,
    ONVIF_XML_MODULE_RECORDING_CONFIG,
    ONVIF_XML_MODULE_MONITORING,
    ONVIF_XML_MODULE_MEDIA 
};

/** Web service operation '__tev__Renew' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__Renew(struct soap* soap, struct _wsnt__Renew *tev__Renew, struct _wsnt__RenewResponse *tev__RenewResponse)
{
    int nDuration = 1800; // Default 30 minutes
    int nActualDuration = 0;
    const char* strAddress = NULL;
    time_t now = time(NULL);

#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__Renew Start----------");
#endif

    if (!tev__Renew)
    {
         dlog_error("__tev__Renew: Request is NULL");
         return soap_sender_fault(soap, "wsnt__Renew is NULL", NULL);
    }
    if (!tev__RenewResponse)
    {
         dlog_error("__tev__Renew: Response is NULL");
         return soap_sender_fault(soap, "wsnt__RenewResponse is NULL", NULL);
    }

    if(soap->header && soap->header->wsa5__To)
    {
        strAddress = soap->header->wsa5__To;
    }
    else 
    {
        dlog_error("__tev__Renew: wsa5__To header missing");
    }


    dlog_debug("__tev__Renew: TerminationTime is NULL, using default %d seconds", nDuration);
    

    // Call Manager
    if (onvif_renew_subscription(strAddress, nDuration, &nActualDuration) != 0)
    {
         dlog_error("__tev__Renew: Subscription Not Found for Address: %s", strAddress ? strAddress : "(null)");
         return soap_sender_fault(soap, "Subscription Not Found", NULL);
    }
    
    // Construct Response
    tev__RenewResponse->TerminationTime = now + nActualDuration;
    
    // Allocate pointer for CurrentTime
    tev__RenewResponse->CurrentTime = (time_t*)soap_malloc(soap, sizeof(time_t));
    if (tev__RenewResponse->CurrentTime)
    {
        *tev__RenewResponse->CurrentTime = now;
    }

    dlog_debug("__tev__Renew: Success. New TerminationTime: %lld (Duration: %d)", (long long)tev__RenewResponse->TerminationTime, nActualDuration);

    return SOAP_OK;
}

/** Web service operation '__tev__Unsubscribe' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__Unsubscribe(struct soap* soap, struct _wsnt__Unsubscribe *wsnt__Unsubscribe, struct _wsnt__UnsubscribeResponse *wsnt__UnsubscribeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__Unsubscribe----------");
#endif
  
    if(soap->header != NULL && soap->header->wsa5__To != NULL)
    {
        dlog_debug("==========wsa5__To Unsubscribe addr is %s",soap->header->wsa5__To);
        onvif_destroy_subscription(soap->header->wsa5__To);
    }

    return SOAP_OK;
}

/** Web service operation '__tev__Subscribe' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__Subscribe(struct soap* soap, struct _wsnt__Subscribe *wsnt__Subscribe, struct _wsnt__SubscribeResponse *wsnt__SubscribeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__Subscribe----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__GetCurrentMessage' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__GetCurrentMessage(struct soap* soap, struct _wsnt__GetCurrentMessage *wsnt__GetCurrentMessage, struct _wsnt__GetCurrentMessageResponse *wsnt__GetCurrentMessageResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__GetCurrentMessage----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__Notify' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__Notify(struct soap* soap, struct _wsnt__Notify *wsnt__Notify)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__Notify----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__GetMessages' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__GetMessages(struct soap* soap, struct _wsnt__GetMessages *wsnt__GetMessages, struct _wsnt__GetMessagesResponse *wsnt__GetMessagesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__GetMessages----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__DestroyPullPoint' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__DestroyPullPoint(struct soap* soap, struct _wsnt__DestroyPullPoint *wsnt__DestroyPullPoint, struct _wsnt__DestroyPullPointResponse *wsnt__DestroyPullPointResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__DestroyPullPoint----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__Notify_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__Notify_(struct soap* soap, struct _wsnt__Notify *wsnt__Notify)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__Notify_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__CreatePullPoint' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__CreatePullPoint(struct soap* soap, struct _wsnt__CreatePullPoint *wsnt__CreatePullPoint, struct _wsnt__CreatePullPointResponse *wsnt__CreatePullPointResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__CreatePullPoint----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__Renew_' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__Renew_(struct soap* soap, struct _wsnt__Renew *wsnt__Renew, struct _wsnt__RenewResponse *wsnt__RenewResponse)
{
//#if ONVIF_LOG_SWITCH
#if 1
    dlog_debug("----------__tev__Renew_----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__Unsubscribe__' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__Unsubscribe_(struct soap* soap, struct _wsnt__Unsubscribe *wsnt__Unsubscribe, struct _wsnt__UnsubscribeResponse *wsnt__UnsubscribeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__Unsubscribe__----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__PauseSubscription' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__PauseSubscription(struct soap* soap, struct _wsnt__PauseSubscription *wsnt__PauseSubscription, struct _wsnt__PauseSubscriptionResponse *wsnt__PauseSubscriptionResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__PauseSubscription----------");
#endif
    return SOAP_OK;
}

/** Web service operation '__tev__ResumeSubscription' (returns SOAP_OK or error code) */
SOAP_FMAC5 int SOAP_FMAC6 __tev__ResumeSubscription(struct soap* soap, struct _wsnt__ResumeSubscription *wsnt__ResumeSubscription, struct _wsnt__ResumeSubscriptionResponse *wsnt__ResumeSubscriptionResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__ResumeSubscription----------");
#endif
    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Unsubscribe__(struct soap* soap, struct _wsnt__Unsubscribe *wsnt__Unsubscribe, struct _wsnt__UnsubscribeResponse *wsnt__UnsubscribeResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__Unsubscribe__----------");
#endif
    return SOAP_OK;
}



SOAP_FMAC5 int SOAP_FMAC6 __tev__GetEventProperties(struct soap* soap, struct _tev__GetEventProperties *tev__GetEventProperties, struct _tev__GetEventPropertiesResponse *tev__GetEventPropertiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__GetEventProperties----------");
#endif
    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    if(!tev__GetEventPropertiesResponse)
    {
        dlog_error("tev__GetEventPropertiesResponse is NULL");
        return soap_receiver_fault(soap, "tev__GetEventPropertiesResponse is NULL", NULL);    
    }

    tev__GetEventPropertiesResponse->wsnt__FixedTopicSet = xsd__boolean__true_;
    if(tev__GetEventPropertiesResponse->wstop__TopicSet == NULL)
    {
       tev__GetEventPropertiesResponse->wstop__TopicSet = soap_new_wstop__TopicSetType(soap,-1);
    }

    tev__GetEventPropertiesResponse->wstop__TopicSet->__size = ONVIF_TOPIC_MODULE_MAX;
    tev__GetEventPropertiesResponse->wstop__TopicSet->__any = (char **)soap_malloc(soap, sizeof(char *) * tev__GetEventPropertiesResponse->wstop__TopicSet->__size);

     for (int i = 0; i < tev__GetEventPropertiesResponse->wstop__TopicSet->__size; i++) 
     {
        tev__GetEventPropertiesResponse->wstop__TopicSet->__any[i] = (char *)soap_strdup(soap, g_arrTopics[i]);
    }

    tev__GetEventPropertiesResponse->__sizeTopicExpressionDialect = 2;

    tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect = (char **)soap_malloc(
        soap, 
        sizeof(char *) * tev__GetEventPropertiesResponse->__sizeTopicExpressionDialect
    );
    if (tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect == NULL) 
    {
        printf("Error: soap_malloc for wsnt__TopicExpressionDialect failed\n");
    }

    tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect[0] = (char *)soap_strdup(soap, ONVIF_TOPIC_EXPR_DIALECT_CONCRETE_SET);
    tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect[1] = (char *)soap_strdup(soap, ONVIF_TOPIC_EXPR_DIALECT_OASIS_CONCRETE);

    if (tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect[0] == NULL || tev__GetEventPropertiesResponse->wsnt__TopicExpressionDialect[1] == NULL) 
    {
        printf("Error: soap_strdup for TopicExpressionDialect failed\n");
    }

    tev__GetEventPropertiesResponse->__sizeMessageContentFilterDialect = 1;
    tev__GetEventPropertiesResponse->MessageContentFilterDialect = (char **)soap_malloc(
        soap, 
        sizeof(char *) * tev__GetEventPropertiesResponse->__sizeMessageContentFilterDialect
    );
    if (tev__GetEventPropertiesResponse->MessageContentFilterDialect == NULL) 
    {
        printf("Error: soap_malloc for MessageContentFilterDialect failed\n");
    }
    tev__GetEventPropertiesResponse->MessageContentFilterDialect[0] = (char *)soap_strdup(soap, ONVIF_MSG_CONTENT_FILTER_DIALECT);
    if (tev__GetEventPropertiesResponse->MessageContentFilterDialect[0] == NULL) 
    {
        printf("Error: soap_strdup for MessageContentFilterDialect failed\n");
    }

    tev__GetEventPropertiesResponse->__sizeMessageContentSchemaLocation = 1;
    tev__GetEventPropertiesResponse->MessageContentSchemaLocation = (char **)soap_malloc(
        soap, 
        sizeof(char *) * tev__GetEventPropertiesResponse->__sizeMessageContentSchemaLocation
    );
    if (tev__GetEventPropertiesResponse->MessageContentSchemaLocation == NULL) 
    {
        printf("Error: soap_malloc for MessageContentSchemaLocation failed\n");
    }
    tev__GetEventPropertiesResponse->MessageContentSchemaLocation[0] = (char *)soap_strdup(soap, ONVIF_MSG_CONTENT_SCHEMA_LOCATION);
    if (tev__GetEventPropertiesResponse->MessageContentSchemaLocation[0] == NULL) 
    {
        printf("Error: soap_strdup for MessageContentSchemaLocation failed\n");
    }

    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__CreatePullPointSubscription(struct soap* soap, struct _tev__CreatePullPointSubscription *tev__CreatePullPointSubscription, struct _tev__CreatePullPointSubscriptionResponse *tev__CreatePullPointSubscriptionResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__CreatePullPointSubscription----------");
#endif
    if(!tev__CreatePullPointSubscriptionResponse)
    {
        dlog_error("tev__CreatePullPointSubscriptionResponse is NULL");
        return soap_receiver_fault(soap, "tev__CreatePullPointSubscriptionResponse is NULL", NULL);    
    }

    int nRet = onvif_authentication(soap);
    if(SOAP_OK != nRet)
    {
        return nRet;
    }

    if(soap->header != NULL)
    {
        if(soap->header->wsa__MessageID != NULL)
        {
            soap->header->wsa__MessageID = NULL;
        }

        if(soap->header->wsa__ReplyTo != NULL)
        {
            soap->header->wsa__ReplyTo = NULL;
        } 

        if(soap->header->wsa__To != NULL)
        {
            soap->header->wsa__To = NULL;
        }

        if(soap->header->wsa__Action != NULL)
        {
            soap->header->wsa__Action = NULL;
        }

        soap->header->wsa5__Action = soap_strdup(soap, "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionResponse");
    }

    char strSubscriptionAddress[256] = {0};

    time_t timestamp = time(NULL); // 获取当前时间戳（秒级）
    struct tm *tm_info = gmtime(&timestamp); // 转换为UTC时间结构体

    char timeBuffer[32] = {0};
    size_t rc = strftime(timeBuffer, sizeof(timeBuffer), "%Y%m%dT%H%M%SZ", tm_info);

    if (rc == 0) 
    {
        dlog_error("时间格式化失败，buffer太小");
    } 
    else 
    {
        dlog_debug("UTC时间: %s\n", timeBuffer);
    }
    int nHttport = ONVIF_HTTP_PORT;
    nHttport = onvif_get_httpPort();
    if(nHttport < 0)
    {
        nHttport = ONVIF_HTTP_PORT;
    }
    if(nHttport != ONVIF_HTTP_PORT)
    {
        snprintf(strSubscriptionAddress, sizeof(strSubscriptionAddress), "http://%s:%d/onvif/Events/PullSubManager_%s", get_primary_ip(),nHttport, timeBuffer);
    }
    else
    {
         snprintf(strSubscriptionAddress, sizeof(strSubscriptionAddress), "http://%s/onvif/Events/PullSubManager_%s", get_primary_ip(), timeBuffer);
    }
    //snprintf(strSubscriptionAddress, sizeof(strSubscriptionAddress), "http://%s/onvif/Events/PullSubManager_%s", get_primary_ip(), timeBuffer);
    //snprintf(strSubscriptionAddress, sizeof(strSubscriptionAddress), "http://%s:%d/onvif/Events/PullSubManager_%s", get_primary_ip(),ONVIF_TCP_PORT, timeBuffer);
    // dlog_debug("最终订阅地址：%s", strSubscriptionAddress);

    if(onvif_create_subscription(strSubscriptionAddress) != 0)
    {
        dlog_debug("订阅地址: %s 创建失败 返回空订阅地址",strSubscriptionAddress );
    }
    else
    {
        tev__CreatePullPointSubscriptionResponse->SubscriptionReference.Address = soap_strdup(soap, strSubscriptionAddress);
        tev__CreatePullPointSubscriptionResponse->wsnt__CurrentTime = time(NULL);
        tev__CreatePullPointSubscriptionResponse->wsnt__TerminationTime = time(NULL)+ONVIF_SUB_TIMEOUT;
        dlog_debug("订阅地址: %s 创建成功 ",strSubscriptionAddress );
    }

    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__SetSynchronizationPoint(struct soap* soap, struct _tev__SetSynchronizationPoint *tev__SetSynchronizationPoint, struct _tev__SetSynchronizationPointResponse *tev__SetSynchronizationPointResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__SetSynchronizationPoint----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__PullMessages(struct soap* soap, struct _tev__PullMessages *tev__PullMessages, struct _tev__PullMessagesResponse *tev__PullMessagesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__PullMessages----------");
#endif

    // dlog_debug("----------__tev__PullMessages-----start-----");

    // int nRet = onvif_authentication(soap);
    // if(SOAP_OK != nRet)
    // {
    //     return nRet;
    // }
    OnvifAlarmEventBatch_S stEventBatch;
    memset(&stEventBatch, 0, sizeof(OnvifAlarmEventBatch_S));
    if(tev__PullMessagesResponse == NULL)
    {
        dlog_error("tev__PullMessagesResponse is NULL");
        return soap_sender_fault(soap, "tev__PullMessagesResponse is NULL", NULL);
    }

    if(tev__PullMessages == NULL)
    {
        dlog_error("tev__PullMessages is NULL");
        return soap_sender_fault(soap, "tev__PullMessages is NULL", NULL);
    }

    if(soap->header != NULL && soap->header->wsa5__To != NULL)
    {
        dlog_debug("==========wsa5__To PullMessages addr is %s",soap->header->wsa5__To);
         if(onvif_pull_events(soap->socket, soap->header->wsa5__To, &stEventBatch, PULLMSG_TIMEOUT_UNIT) < 0)
        {
            return soap_sender_fault(soap, "PullMessages addr is NULL", NULL);
        }
       
    }
    else
    {
        // dlog_debug("wsa5__To PullMessages addr  null  ");
    }
    tev__PullMessagesResponse->CurrentTime = time(NULL);
    tev__PullMessagesResponse->TerminationTime  = time(NULL) + stEventBatch.nExpireTime/*ONVIF_SUB_TIMEOUT*/;
    // dlog_debug("%ld %d %s", tev__PullMessages->Timeout, tev__PullMessages->MessageLimit, soap->path);
    
#if 0
    sleep(1);
    stEventBatch.nEventNum = 1;
    stEventBatch.events[0].enAlarmType = MOTION_DETECTION_ALARM;
#endif
#if ONVIF_LOG_SWITCH
    dlog_debug("PullMessages 事件个数 [%d] ",stEventBatch.nEventNum);
#endif
    // dlog_debug("PullMessages 事件个数 [%d] ",stEventBatch.nEventNum);
    tev__PullMessagesResponse->__sizeNotificationMessage = stEventBatch.nEventNum;
    if(tev__PullMessagesResponse->wsnt__NotificationMessage != NULL)
    {
     #if ONVIF_LOG_SWITCH
        dlog_debug("PullMessages wsnt__NotificationMessage (no null) ");
    #endif
        soap_dealloc(soap, tev__PullMessagesResponse->wsnt__NotificationMessage);
    }
    if(stEventBatch.nEventNum == 1)
    {
        tev__PullMessagesResponse->wsnt__NotificationMessage = soap_new_wsnt__NotificationMessageHolderType(soap,-1);
    }
    else
    {
        tev__PullMessagesResponse->wsnt__NotificationMessage = soap_new_wsnt__NotificationMessageHolderType(soap,tev__PullMessagesResponse->__sizeNotificationMessage);
    }
    //tev__PullMessagesResponse->wsnt__NotificationMessage = (struct wsnt__NotificationMessageHolderType *)soap_malloc(soap, sizeof(struct wsnt__NotificationMessageHolderType) * tev__PullMessagesResponse->__sizeNotificationMessage);
    if(stEventBatch.nEventNum == 0)
    {
    #if ONVIF_LOG_SWITCH
        dlog_debug("PullMessages 没有获取到事件(null) ");
    #endif
        return SOAP_OK;
    } 
    if(tev__PullMessagesResponse->wsnt__NotificationMessage->Topic == NULL)
    {
        //dlog_debug("tev__PullMessagesResponse->wsnt__NotificationMessage->Topic (null) ");
        //tev__PullMessagesResponse->wsnt__NotificationMessage->Topic = (struct wsnt__TopicExpressionType *)soap_malloc(soap, sizeof(struct wsnt__TopicExpressionType));
        tev__PullMessagesResponse->wsnt__NotificationMessage->Topic =  soap_new_wsnt__TopicExpressionType(soap,-1);
    }
    
#if 1
    for(int i = 0;i < stEventBatch.nEventNum;i++ )
    {
    #if ONVIF_LOG_SWITCH
        dlog_debug("PullMessages[%d] 报警事件类型 [%d] ",i,(int )stEventBatch.events[i].enAlarmType);
    #endif
        OnvifAlarmEventType_E enAlarmType;
        enAlarmType = stEventBatch.events[i].enAlarmType;

        struct wsnt__NotificationMessageHolderType *p = tev__PullMessagesResponse->wsnt__NotificationMessage + i;
        if(p == NULL)
        {
            dlog_debug("PullMessages[%d] wsnt__NotificationMessageHolderType [null] ",i);
            continue;
        }

        p->SubscriptionReference = soap_new_wsa5__EndpointReferenceType(soap,-1);
        p->SubscriptionReference->Address = soap_strdup(soap, "");

        p->ProducerReference = soap_new_wsa5__EndpointReferenceType(soap,-1);
        p->ProducerReference->Address = soap_strdup(soap, "");
        //p->Topic = (struct wsnt__TopicExpressionType *)soap_malloc(soap, sizeof(struct wsnt__TopicExpressionType));
        p->Topic = soap_new_wsnt__TopicExpressionType(soap,-1);
        if(!p->Topic)
        {
            dlog_debug("PullMessages[%d] wsnt__TopicExpressionType [null] ",i);
            continue;
        }
        p->Topic->Dialect = soap_strdup(soap, ONVIF_TOPIC_EXPR_DIALECT_CONCRETE_SET);
        
        //p->Message.tt__Message = (struct _tt__Message *)soap_malloc(soap, sizeof(struct _tt__Message));
        p->Message.tt__Message = soap_new__tt__Message(soap,-1);
        if(!p->Message.tt__Message)
        {
            continue;
        }

        //p->Message.tt__Message->UtcTime = tev__PullMessagesResponse->CurrentTime;
        p->Message.tt__Message->UtcTime = stEventBatch.events[i].alarmTime;

        //p->Message.tt__Message->PropertyOperation = (enum tt__PropertyOperation *)soap_malloc(soap, sizeof(enum tt__PropertyOperation));
        p->Message.tt__Message->PropertyOperation = soap_new_tt__PropertyOperation(soap,-1);
        if(!p->Message.tt__Message->PropertyOperation)
        {
            continue;
        }
        *(p->Message.tt__Message->PropertyOperation) = tt__PropertyOperation__Initialized;

        //p->Message.tt__Message->Source = (struct tt__ItemList *)soap_malloc(soap, sizeof(struct tt__ItemList));
        p->Message.tt__Message->Source = soap_new_tt__ItemList(soap,-1);
        if(!p->Message.tt__Message->Source)
        {
            continue;
        }

        p->Message.tt__Message->Data = soap_new_tt__ItemList(soap,-1);
        //p->Message.tt__Message->Data = (struct tt__ItemList *)soap_malloc(soap, sizeof(struct tt__ItemList));
        if(!p->Message.tt__Message->Data)
        {
            continue;
        }

        p->Message.tt__Message->Data->__sizeSimpleItem = 1;
        p->Message.tt__Message->Data->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,-1);
        //p->Message.tt__Message->Data->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem));
        if(!p->Message.tt__Message->Data->SimpleItem)
        {
            continue;
        }    

        switch (enAlarmType)
        {
            /* 移动侦测 */
            case MOTION_DETECTION_ALARM:
            {
      
                dlog_debug("PullMessages[%d] 移动侦测构造数据",i);
  
                p->Topic->__mixed = soap_strdup(soap, MOTION_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = soap_new__tt__ItemList_SimpleItem(soap,3);
                //p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                }

                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Value = soap_strdup(soap, MOTION_EVENT_RULE);
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                }

                //p->Message.tt__Message->Data->SimpleItem->Name = MOTION_NAME;
                //p->Message.tt__Message->Data->SimpleItem->Value = ONVIF_ALARM_TRUE;
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
              
                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, MOTION_NAME);
                break;
            }
               
            /* 遮挡报警 */   
            case IMAGE_OBSTRUTION_ALARM:
            {
                dlog_debug("PullMessages[%d] 遮挡报警构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, TAMPER_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                   
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, TAMPEREVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, TAMPER_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }

                break;
            }
            
            /* 区域入侵 */    
            case INTRUSION_ALARM:
            {
                dlog_debug("PullMessages[%d]区域入侵构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, FIELD_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                     pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, FIELD_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, FIELD_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }

                break;
            }
            /* 越界侦测 */
            case LINGERING_ALARM:
            {
                dlog_debug("PullMessages[%d]越界侦测构造数据  ",i);
                p->Topic->__mixed = soap_strdup(soap, LINE_EVENT_THEME);

                 p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, LINE_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, LINE_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }

                break;
            }
             /* 进入区域 */
            case ONVIF_ENTER_REGION:
            {
                dlog_debug("PullMessages[%d]进入区域构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, ENTER_REGION_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, ENTER_REGION_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, ENTER_REGION_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 离开区域 */
            case ONVIF_LEAVE_REGION:
            {
                dlog_debug("PullMessages[%d]离开区域构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, LEAVE_REGION_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, LEAVE_REGION_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, LEAVE_REGION_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 音频异常侦测 */
            case ONVIF_AUDIO_ANOMALY:
            {
                 dlog_debug("PullMessages[%d]音频异常侦测构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, AUDIO_ANOMALY_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 1;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem));

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
            
                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, AUDIOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE_AUDIOSOURCE_SOURCETOKEN);
                }
                
                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, AUDIO_ANOMALY_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 音频异常-声强陡升 */
            case ONVIF_AUDIO_SUDDEN_RISE:
            {
                dlog_debug("PullMessages[%d]音频异常-声强陡升构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, AUDIO_SUDDEN_RISE_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 1;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem));

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
            
                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, AUDIOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE_AUDIOSOURCE_SOURCETOKEN);
                }
                
                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, AUDIO_SUDDEN_RISE_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 音频异常-声强陡降 */
            case ONVIF_AUDIO_SUDDEN_DROP:
            {
                dlog_debug("PullMessages[%d]音频异常-声强陡降构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, AUDIO_SUDDEN_DROP_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 1;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem));

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
            
                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, AUDIOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE_AUDIOSOURCE_SOURCETOKEN);
                }
                
                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, AUDIO_SUDDEN_DROP_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 场景变更 */
            case ONVIF_SCENE_CHANGE:
            {
                 dlog_debug("PullMessages[%d]场景变更构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, SCENE_CHANGE_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, SCENE_CHANGE_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, SCENE_CHANGE_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 人脸侦测 */
            case ONVIF_FACE_DETECT:
            {
                 dlog_debug("PullMessages[%d]人脸侦测域构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, FACE_DETECT_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, FACE_DETECT_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, FACE_DETECT_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 徘徊侦测 */
            case ONVIF_LOITERING_DETECT:
            {
                 dlog_debug("PullMessages[%d]徘徊侦测构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, LOITERING_DETECT_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, LOITERING_DETECT_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, LOITERING_DETECT_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 人员聚集 */
            case ONVIF_CROWD_GATHERING:
            {
                dlog_debug("PullMessages[%d]人员聚集构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, CROWD_GATHERING_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, CROWD_GATHERING_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, CROWD_GATHERING_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
             /* 停车侦测 */
            case ONVIF_PARKING_DETECT:
            {
                 dlog_debug("PullMessages[%d]停车侦测构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, PARKING_DETECT_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, PARKING_DETECT_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, PARKING_DETECT_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
             /* 物品遗留 */
            case ONVIF_UNATTENDED_OBJECT:
            {
                 dlog_debug("PullMessages[%d]物品遗留构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, UNATTENDED_OBJECT_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, UNATTENDED_OBJECT_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 物品拿取 */
             case ONVIF_OBJECT_REMOVAL:
            {
                 dlog_debug("PullMessages[%d]物品拿取构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, OBJECT_REMOVAL_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, OBJECT_REMOVAL_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, OBJECT_REMOVAL_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 宠物识别 */
             case ONVIF_PET_RECOGNITION:
            {
                 dlog_debug("PullMessages[%d]宠物识别构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, PET_RECOGNITION_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, PET_RECOGNITION_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, PET_RECOGNITION_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            /* 人脸抓拍 */
             case ONVIF_FACE_CAPTURE:
            {
                 dlog_debug("PullMessages[%d]人脸抓拍构造数据 ",i);
                p->Topic->__mixed = soap_strdup(soap, FACE_CAPTURE_EVENT_THEME);

                p->Message.tt__Message->Source->__sizeSimpleItem = 3;
                p->Message.tt__Message->Source->SimpleItem = (struct _tt__ItemList_SimpleItem *)soap_malloc(soap, sizeof(struct _tt__ItemList_SimpleItem) * 3);

                struct _tt__ItemList_SimpleItem *pSimpleItem_0 = p->Message.tt__Message->Source->SimpleItem + 0;
                struct _tt__ItemList_SimpleItem *pSimpleItem_1 = p->Message.tt__Message->Source->SimpleItem + 1;
                struct _tt__ItemList_SimpleItem *pSimpleItem_2 = p->Message.tt__Message->Source->SimpleItem + 2;

                 if(pSimpleItem_0 != NULL)
                {
                    pSimpleItem_0->Name = soap_strdup(soap, VIDEOSOURCE_TOKEN_NAME);
                    pSimpleItem_0->Value = soap_strdup(soap, PROFILE1_VIDEOSOURCE_SOURCETOKEN);
                }
                
                if(pSimpleItem_1 != NULL)
                {
                    pSimpleItem_1->Name = soap_strdup(soap, VIDEOANALTICS_TOKEN_NAME);
                    pSimpleItem_1->Value = soap_strdup(soap, VIDEOANALTICS_TOKEN);
                }

                if(pSimpleItem_2 != NULL)
                {
                    pSimpleItem_2->Name = soap_strdup(soap, RULE_TOKEN_NAME);
                    pSimpleItem_2->Value = soap_strdup(soap, FACE_CAPTURE_EVENT_RULE);
                }

                p->Message.tt__Message->Data->SimpleItem->Name = soap_strdup(soap, FACE_CAPTURE_NAME);
                if(stEventBatch.events[i].nValue == 0)
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_FALSE);
                }
                else
                {
                    p->Message.tt__Message->Data->SimpleItem->Value = soap_strdup(soap, ONVIF_ALARM_TRUE);
                }
                break;
            }
            default:
                dlog_debug("PullMessages[%d]找不到构造类型！ ",i);
                break;
        }
       
    }
#endif
    //dlog_debug("PullMessages 结束 ");

    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__GetServiceCapabilities(struct soap* soap, struct _tev__GetServiceCapabilities *tev__GetServiceCapabilities, struct _tev__GetServiceCapabilitiesResponse *tev__GetServiceCapabilitiesResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__GetServiceCapabilities----------");
#endif

    if(tev__GetServiceCapabilitiesResponse == NULL )
    {
        dlog_error("_tev__GetServiceCapabilitiesResponse is NULL");
        tev__GetServiceCapabilitiesResponse = soap_new__tev__GetServiceCapabilitiesResponse(soap,-1);
    }

    if(tev__GetServiceCapabilitiesResponse->Capabilities == NULL)
    {
        tev__GetServiceCapabilitiesResponse->Capabilities = soap_new_tev__Capabilities(soap,-1);
    }
    
    if(tev__GetServiceCapabilitiesResponse->Capabilities->WSSubscriptionPolicySupport == NULL)
    {
        tev__GetServiceCapabilitiesResponse->Capabilities->WSSubscriptionPolicySupport = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    }
    *(tev__GetServiceCapabilitiesResponse->Capabilities->WSSubscriptionPolicySupport) = (enum xsd__boolean )ONVIF_SUPPORT_WS_SUB_POLICY;

    if(tev__GetServiceCapabilitiesResponse->Capabilities->WSPausableSubscriptionManagerInterfaceSupport == NULL)
    {
        tev__GetServiceCapabilitiesResponse->Capabilities->WSPausableSubscriptionManagerInterfaceSupport = (enum xsd__boolean *)soap_malloc(soap, sizeof(enum xsd__boolean));
    }
    *(tev__GetServiceCapabilitiesResponse->Capabilities->WSPausableSubscriptionManagerInterfaceSupport) = (enum xsd__boolean )ONVIF_SUPPORT_PAUSABLE_SUB;
   
    if(tev__GetServiceCapabilitiesResponse->Capabilities->MaxNotificationProducers == NULL)
    {
        tev__GetServiceCapabilitiesResponse->Capabilities->MaxNotificationProducers = (int *)soap_malloc(soap, sizeof(int));
    }
    *(tev__GetServiceCapabilitiesResponse->Capabilities->MaxNotificationProducers) = ONVIF_MAX_NOTIFICATION_PRODUCERS;

     if(tev__GetServiceCapabilitiesResponse->Capabilities->MaxPullPoints == NULL)
    {
        tev__GetServiceCapabilitiesResponse->Capabilities->MaxPullPoints = (int *)soap_malloc(soap, sizeof(int));
    }
    *(tev__GetServiceCapabilitiesResponse->Capabilities->MaxPullPoints) = ONVIF_MAX_PULLPOINTS;
    

    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Seek(struct soap* soap, struct _tev__Seek *tev__Seek, struct _tev__SeekResponse *tev__SeekResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__Seek----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__AddEventBroker(struct soap* soap, struct _tev__AddEventBroker *tev__AddEventBroker, struct _tev__AddEventBrokerResponse *tev__AddEventBrokerResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__AddEventBroker----------");
#endif
    return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__GetEventBrokers(struct soap* soap, struct _tev__GetEventBrokers *tev__GetEventBrokers, struct _tev__GetEventBrokersResponse *tev__GetEventBrokersResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__GetEventBrokers----------");
#endif
    return SOAP_FAULT;
}

/** Web service operation '__tev__DeleteEventBroker' implementation, should return SOAP_OK or error code */
SOAP_FMAC5 int SOAP_FMAC6 __tev__DeleteEventBroker(struct soap* soap, struct _tev__DeleteEventBroker *tev__DeleteEventBroker, struct _tev__DeleteEventBrokerResponse *tev__DeleteEventBrokerResponse)
{
#if ONVIF_LOG_SWITCH
    dlog_debug("----------__tev__DeleteEventBroker----------");
#endif
    return SOAP_FAULT;
}
