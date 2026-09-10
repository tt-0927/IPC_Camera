/**
 * @File     gat1400_types.h
 * @brief
 * @DateTime 2018/2/4 10:21:23
 * @Author   Nanuns
 */
#ifndef __GAT1400_TYPES_H_
#define __GAT1400_TYPES_H_

#include <new>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include "dlog.h"
#include "gat1400_category.h"
#include "json.hpp"
#include "gat1400_utils.h"

#define SecurityArray  std::vector
#define SecurityString std::string
#define SpeedType      double
#define LongitudeType  double
#define LatitudeType   double

constexpr char* KEY_DEVICE_ID = "DeviceID";
constexpr char* KEY_REGISTER_OBJECT = "RegisterObject";
constexpr char* KEY_UNREGISTER_OBJECT = "UnRegisterObject";
constexpr char* KEY_KEEPALIVE_OBJECT = "KeepaliveObject";
constexpr char* KEY_SYSTEM_TIME = "SystemTime";
constexpr char* KEY_SYSTEM_TIME_OBJECT = "SystemTimeObject";
constexpr char* KEY_SUB_IMAGE_LIST = "SubImageList";
constexpr char* KEY_SUB_IMAGE_INFO_OBJECT = "SubImageInfoObject";
constexpr char* KEY_PERSON_LIST_OBJECT = "PersonListObject";
constexpr char* KEY_PERSON_OBJECT = "PersonObject";
constexpr char* KEY_NONMOTOR_VEHICLE_LIST_OBJECT = "NonMotorVehicleListObject";
constexpr char* KEY_NONMOTOR_VEHICLE_OBJECT = "NonMotorVehicleObject";
constexpr char* KEY_MOTOR_VEHICLE_LIST_OBJECT = "MotorVehicleListObject";
constexpr char* KEY_MOTOR_VEHICLE_OBJECT = "MotorVehicleObject";
constexpr char* KEY_FACE_LIST_OBJECT = "FaceListObject";
constexpr char* KEY_FACE_OBJECT = "FaceObject";


using SecurityParams = std::multimap<std::string, std::string>;

typedef SecurityArray<SecurityString>  security_idlist_t;

// 结构体获取json中的值
template<typename T>
void json_get_if_exists(const nlohmann::json& j, const std::string& key, T& target) {
    if (j.contains(key)) {
        target = j.at(key).get<T>();
    }
}

template<typename T>
void handlePointType(nlohmann::json &j, const T &c)
{
        // 坐标类型
        if (c.PointInfo.pointType == 0) {
            int LeftTopX, LeftTopY, RightBtmX, RightBtmY;
            PointUtils pointUtils(c.PointInfo.srcWidth, c.PointInfo.srcHeight);
            pointUtils.toPermyriad(c.LeftTopX, c.LeftTopY, LeftTopX, LeftTopY);
            pointUtils.toPermyriad(c.RightBtmX, c.RightBtmY, RightBtmX, RightBtmY);
            j["LeftTopX"] = LeftTopX;
            j["LeftTopY"] = LeftTopY;
            j["RightBtmX"] = RightBtmX;
            j["RightBtmY"] = RightBtmY;
        }
        else if (c.PointInfo.pointType == 1) {
            j["LeftTopX"] = c.LeftTopX;
            j["LeftTopY"] = c.LeftTopY;
            j["RightBtmX"] = c.RightBtmX;
            j["RightBtmY"] = c.RightBtmY;
        }
        else if (c.PointInfo.pointType == 2) {
            PointUtils pointUtils(c.PointInfo.srcWidth, c.PointInfo.srcHeight);
            float LeftTopX, LeftTopY, RightBtmX, RightBtmY;
            pointUtils.toNormalize(c.LeftTopX, c.LeftTopY, LeftTopX, LeftTopY);
            pointUtils.toNormalize(c.RightBtmX, c.RightBtmY, RightBtmX, RightBtmY);
            j["LeftTopX"] = static_cast<nlohmann::json::number_float_t>(LeftTopX);
            j["LeftTopY"] = static_cast<nlohmann::json::number_float_t>(LeftTopY);
            j["RightBtmX"] = static_cast<nlohmann::json::number_float_t>(RightBtmX);
            j["RightBtmY"] = static_cast<nlohmann::json::number_float_t>(RightBtmY);

            dlog_debug("[%d,%d]->[%f,%f];[%d,%d]->[%f,%f]", c.LeftTopX, c.LeftTopY, LeftTopX, LeftTopY, c.RightBtmX, c.RightBtmY, RightBtmX, RightBtmY);
        }
}

typedef struct _security_point_info_t {
    int pointType = 1;
    int srcWidth = 0;
    int srcHeight = 0;
} security_point_info_t;

/**
 * @Struct   security_videoslice_info_t
 * @Brief    VideoSliceInfo
 * @DateTime 2018/8/10 11:20:41
 * @Modify   2018/8/10 11:20:52
 * @Author   Nanuns
 */
typedef struct _security_videoslice_info_t {
    SecurityString VideoID;
    SecurityInfoType InfoKind;
    SecurityString VideoSource;
    SecurityString IsAbstractVideo;
    SecurityString OriginVideoID;
    SecurityString OriginVideoURL;
    SecurityString EventSort;
    SecurityString DeviceID;
    SecurityString StoragePath;
    SecurityString ThumbnailStoragePath;
    SecurityString FileHash;
    SecurityString FileFormat;
    SecurityString CodedFormat;
    int            AudioFlag;
    SecurityString AudioCodedFormat;
    SecurityString Title;
    SecurityString TitleNote;
    SecurityString SpecialName;
    SecurityString Keyword;
    SecurityString ContentDescription;
    SecurityString MainCharacter;
    SecurityString ShotPlaceCode;
    SecurityString ShotPlaceFullAdress;
    LongitudeType  ShotPlaceLongitude;
    LatitudeType   ShoPlacetLatitude;
    LatitudeType   ShotPlaceLatitude; // TODO
    SecurityString HorizontalShotDirection;
    SecurityString VerticalShotDirection;
    SecurityString SecurityLevel;
    double         VideoLen;
    SecurityString BeginTime;
    SecurityString EndTime;
    int            TimeErr;
    int            Width;
    int            Height;
    SecurityString QualityGrade;
    SecurityString CollectorName;
    SecurityString CollectorOrg;
    SecurityString CollectorIDType;
    SecurityString CollectorID;
    SecurityString EntryClrk;
    SecurityString EntryClrkOrg;
    SecurityString EntryClrkIDType;
    SecurityString EntryClrkID;
    SecurityString EntryTime;
    int            VideoProcFlag;
    int64_t        FileSize;
} security_videoslice_info_t;
typedef SecurityArray<security_videoslice_info_t> security_videoslice_infos_t;

/**
 * @Struct   security_image_info_t
 * @Brief    ImageInfo ͼ�����
 * @DateTime 2018-08-01T10:52:46+0800
 * @Modify   2018-08-01T10:52:46+0800
 * @Author   Nanuns
 */
typedef struct _security_image_info {
    SecurityString ImageID;             // R/O
    SecurityInfoType InfoKind;          // R ��Ϣ���࣬�˹��ɼ������Զ��ɼ�
    SecurityString ImageSource;         // R ͼ����Դ
    SecurityString SourceVideoID;       // O ��Դ��Ƶ��ʶ�������ͼ������Ƶ��ͼ�����ֶ�����Դ��Ƶ����ƵID
    SecurityString OriginImageID;       // R/O ԭʼͼ���ʶ
    SecurityString EventSort;
    SecurityString DeviceID;
    SecurityString StoragePath;
    SecurityString FileHash;
    SecurityString FileFormat;
    SecurityString ShotTime;
    SecurityString Title;
    SecurityString TitleNote;
    SecurityString SpecialIName;
    SecurityString KeyWord;
    SecurityString ContentDescription;
    SecurityString SubjectCharacter;
    SecurityString ShotPlaceCode;
    SecurityString shotPlaceFullAddress;
    LongitudeType  ShotPlaceLongitude;
    LatitudeType   ShotPlaceLatitude;
    SecurityString HorizontalShotDirection;
    SecurityString VerticalShotDirection;
    SecurityString SecurityLevel;
    int            Width;
    int            Height;
    SecurityString CameraManufacturer;
    SecurityString CameraVersion;
    int            ApertureValue;
    int            ISOSensitivity;              // ISO�й�ֵ
    int            FocalLength;
    SecurityString QualityGrade;
    SecurityString CollectorName;
    SecurityString CollectorOrg;
    SecurityString CollectorIDType;
    SecurityString CollectorID;
    SecurityString EntryClrk;
    SecurityString EntryClrkOrg;
    SecurityString EntryClrkIDType;
    SecurityString EntryTime;
    SecurityString ImageProcFlag;
    int            FileSize;
} security_image_info_t;
typedef SecurityArray<security_image_info_t>                security_image_infos_t;

/**
 * @Struct   security_subimage_info_t
 * @Brief    SubImageInfo ͼ���Ӷ����ˡ�������ȶ�����԰���һ�����߶��ͼ���Ӷ���
 * @DateTime 2018-08-10T11:18:46+0800
 * @Modify   2018-08-10T11:18:46+0800
 * @Author   Nanuns
 */
typedef struct _security_subimage_info_t {
    SecurityString  ImageID;
    int             EventSort       = 0;
    SecurityString  DeviceID;
    SecurityString  StoragePath;
    SecurityString  Type;
    SecurityString  FileFormat;
    SecurityString  ShotTime;
    int             Width           = 0;
    int             Height          = 0;
    SecurityString  Data; // base64Binary

    friend void from_json(const nlohmann::json &j, _security_subimage_info_t &c) {
        c.ImageID = j.value("ImageID", "");
        c.EventSort = j.value("EventSort", -1);
        c.DeviceID = j.value("DeviceID", "");
        c.StoragePath = j.value("StoragePath", "");
        c.Type = j.value("Type", "");
        c.FileFormat = j.value("FileFormat", "");
        c.ShotTime = j.value("ShotTime", "");
        c.Width = j.value("Width", -1);
        c.Height = j.value("Height", -1);
        c.Data = j.value("Data", "");
    }

    friend void to_json(nlohmann::json &j, const _security_subimage_info_t &c) {
        j = nlohmann::json {
            {"ImageID", c.ImageID},
            {"EventSort", c.EventSort},
            {"DeviceID", c.DeviceID},
            {"StoragePath", c.StoragePath},
            {"Type", c.Type},
            {"FileFormat", c.FileFormat},
            {"ShotTime", c.ShotTime},
            {"Width", c.Width},
            {"Height", c.Height},
            {"Data", c.Data},
        };
    }
} security_subimage_info_t;
typedef SecurityArray<security_subimage_info_t> security_subimage_infos_t;


/**
 * @Struct   security_face_t
 * @Brief    ��������
 * @DateTime 2018-08-01T10:52:46+0800
 * @Modify   2018-08-01T10:52:46+0800
 * @Author   Nanuns
 */
typedef struct _security_face_t {
    SecurityString FaceID;
    SecurityInfoType InfoKind;
    SecurityString SourceID;
    SecurityString DeviceID;
    int            LeftTopX                 = 0;
    int            LeftTopY                 = 0;
    int            RightBtmX                = 0;
    int            RightBtmY                = 0;
    SecurityString LocationMarkTime;
    SecurityString FaceAppearTime;
    SecurityString FaceDisAppearTime;
    SecurityString IDType;
    SecurityString IDNumber;
    SecurityString Name;
    SecurityString UsedName;
    SecurityString Alias;
    SecurityString GenderCode;
    int            AgeUpLimit               = 0;
    int            AgeLowerLimit            = 0;
    SecurityString EthicCode;
    SecurityString NationalityCode;
    SecurityString NativeCityCode;
    SecurityString ResidenceAdminDivision;
    SecurityString ChineseAccentCode;
    SecurityString JobCategory;
    int            AccompanyNumber          = 0;
    SecurityString SkinColor;
    SecurityString HairStyle;
    SecurityString HairColor;
    SecurityString FaceStyle;
    SecurityString FacialFeature;
    SecurityString PhysicalFeature;
    SecurityString RespiratorColor;
    SecurityString CapStyle;
    SecurityString CapColor;
    SecurityString GlassStyle;
    SecurityString GlassColor;
    int            IsDriver                 = 0;
    int            IsForeigner              = 0;
    SecurityString PassportType;
    SecurityString ImmigrantTypeCode;
    int            IsSuspectedTerrorist     = 0;
    SecurityString SuspectedTerroristNumber;
    int            IsCriminalInvolved       = 0;
    SecurityString CriminalInvolvedSpecilisationCode;
    SecurityString BodySpeciallMark;
    SecurityString CrimeMethod;
    SecurityString CrimeCharacterCode;
    SecurityString EscapedCriminalNumber;
    int            IsDetainees              = 0;
    SecurityString DetentionHouseCode;
    SecurityString DetaineesIdentity;
    SecurityString DetaineesSpecialIdentity;
    SecurityString MemberTypeCode;
    int            IsVictim                 = 0;
    SecurityString VictimType;
    SecurityString InjuredDegree;
    SecurityString CorpseConditionCode;
    int            IsSuspiciousPerson       = 0;
    int            Attitude                 = 0;
    double         Similaritydegree         = 0.0;
    SecurityString EyebrowStyle;
    SecurityString NoseStyle;
    SecurityString MustacheStyle;
    SecurityString LipStyle;
    SecurityString WrinklePouch;
    SecurityString AcneStain;
    SecurityString FreckleBirthmark;
    SecurityString ScarDimple;
    SecurityString OtherFeature;
    security_subimage_infos_t SubImageList;
    security_point_info_t PointInfo;
    SecurityString ShotTime;    // 宇视 VIID_2018增加字段

    friend void from_json(const nlohmann::json &j, _security_face_t &c) {
        json_get_if_exists(j, "FaceID", c.FaceID);
        json_get_if_exists(j, "InfoKind", c.InfoKind);
        json_get_if_exists(j, "SourceID", c.SourceID);
        json_get_if_exists(j, "DeviceID", c.DeviceID);
        json_get_if_exists(j, "LocationMarkTime", c.LocationMarkTime);
        json_get_if_exists(j, "FaceAppearTime", c.FaceAppearTime);
        json_get_if_exists(j, "FaceDisAppearTime", c.FaceDisAppearTime);
        json_get_if_exists(j, "IDType", c.IDType);
        json_get_if_exists(j, "IDNumber", c.IDNumber);
        json_get_if_exists(j, "Name", c.Name);
        json_get_if_exists(j, "UsedName", c.UsedName);
        json_get_if_exists(j, "Alias", c.Alias);
        json_get_if_exists(j, "GenderCode", c.GenderCode);
        json_get_if_exists(j, "AgeUpLimit", c.AgeUpLimit);
        json_get_if_exists(j, "AgeLowerLimit", c.AgeLowerLimit);
        json_get_if_exists(j, "EthicCode", c.EthicCode);
        json_get_if_exists(j, "NationalityCode", c.NationalityCode);
        json_get_if_exists(j, "NativeCityCode", c.NativeCityCode);
        json_get_if_exists(j, "ResidenceAdminDivision", c.ResidenceAdminDivision);
        json_get_if_exists(j, "ChineseAccentCode", c.ChineseAccentCode);
        json_get_if_exists(j, "JobCategory", c.JobCategory);
        json_get_if_exists(j, "AccompanyNumber", c.AccompanyNumber);
        json_get_if_exists(j, "SkinColor", c.SkinColor);
        json_get_if_exists(j, "HairStyle", c.HairStyle);
        json_get_if_exists(j, "HairColor", c.HairColor);
        json_get_if_exists(j, "FaceStyle", c.FaceStyle);
        json_get_if_exists(j, "FacialFeature", c.FacialFeature);
        json_get_if_exists(j, "PhysicalFeature", c.PhysicalFeature);
        json_get_if_exists(j, "RespiratorColor", c.RespiratorColor);
        json_get_if_exists(j, "CapStyle", c.CapStyle);
        json_get_if_exists(j, "CapColor", c.CapColor);
        json_get_if_exists(j, "GlassStyle", c.GlassStyle);
        json_get_if_exists(j, "GlassColor", c.GlassColor);
        json_get_if_exists(j, "IsDriver", c.IsDriver);
        json_get_if_exists(j, "IsForeigner", c.IsForeigner);
        json_get_if_exists(j, "PassportType", c.PassportType);
        json_get_if_exists(j, "ImmigrantTypeCode", c.ImmigrantTypeCode);
        json_get_if_exists(j, "IsSuspectedTerrorist", c.IsSuspectedTerrorist);
        json_get_if_exists(j, "SuspectedTerroristNumber", c.SuspectedTerroristNumber);
        json_get_if_exists(j, "IsCriminalInvolved", c.IsCriminalInvolved);
        json_get_if_exists(j, "CriminalInvolvedSpecilisationCode", c.CriminalInvolvedSpecilisationCode);
        json_get_if_exists(j, "BodySpeciallMark", c.BodySpeciallMark);
        json_get_if_exists(j, "CrimeMethod", c.CrimeMethod);
        json_get_if_exists(j, "CrimeCharacterCode", c.CrimeCharacterCode);
        json_get_if_exists(j, "EscapedCriminalNumber", c.EscapedCriminalNumber);
        json_get_if_exists(j, "IsDetainees", c.IsDetainees);
        json_get_if_exists(j, "DetentionHouseCode", c.DetentionHouseCode);
        json_get_if_exists(j, "DetaineesIdentity", c.DetaineesIdentity);
        json_get_if_exists(j, "DetaineesSpecialIdentity", c.DetaineesSpecialIdentity);
        json_get_if_exists(j, "MemberTypeCode", c.MemberTypeCode);
        json_get_if_exists(j, "IsVictim", c.IsVictim);
        json_get_if_exists(j, "VictimType", c.VictimType);
        json_get_if_exists(j, "InjuredDegree", c.InjuredDegree);
        json_get_if_exists(j, "CorpseConditionCode", c.CorpseConditionCode);
        json_get_if_exists(j, "IsSuspiciousPerson", c.IsSuspiciousPerson);
        json_get_if_exists(j, "Attitude", c.Attitude);
        json_get_if_exists(j, "Similaritydegree", c.Similaritydegree);
        json_get_if_exists(j, "EyebrowStyle", c.EyebrowStyle);
        json_get_if_exists(j, "NoseStyle", c.NoseStyle);
        json_get_if_exists(j, "MustacheStyle", c.MustacheStyle);
        json_get_if_exists(j, "LipStyle", c.LipStyle);
        json_get_if_exists(j, "WrinklePouch", c.WrinklePouch);
        json_get_if_exists(j, "AcneStain", c.AcneStain);
        json_get_if_exists(j, "FreckleBirthmark", c.FreckleBirthmark);
        json_get_if_exists(j, "ScarDimple", c.ScarDimple);
        json_get_if_exists(j, "OtherFeature", c.OtherFeature);
        
        // SubImageList
        nlohmann::json jSubImageInfoObject = j[KEY_SUB_IMAGE_LIST][KEY_SUB_IMAGE_INFO_OBJECT];
        for(int i = 0; i < jSubImageInfoObject.size(); i++) {
            c.SubImageList.emplace_back(jSubImageInfoObject[i].get<_security_subimage_info_t>());
        }
    }

    friend void to_json(nlohmann::json &j, const _security_face_t &c) {
        // 部分字段
        j = nlohmann::json {
            {"FaceID", c.FaceID},
            {"InfoKind", c.InfoKind},
            {"SourceID", c.SourceID},
            {"DeviceID", c.DeviceID},
            {"LocationMarkTime", c.LocationMarkTime},
            {"FaceAppearTime", c.FaceAppearTime},
            {"FaceDisAppearTime", c.FaceDisAppearTime},
            {"IDType", c.IDType},
            {"IDNumber", c.IDNumber},
            {"Name", c.Name},
            {"UsedName", c.UsedName},
            {"Alias", c.Alias},
            {"GenderCode", c.GenderCode},
            {"AgeUpLimit", c.AgeUpLimit},
            {"AgeLowerLimit", c.AgeLowerLimit},
            {"EthicCode", c.EthicCode},
            {"NationalityCode", c.NationalityCode},
            {"NativeCityCode", c.NativeCityCode},
            {"ResidenceAdminDivision", c.ResidenceAdminDivision},
            {"ChineseAccentCode", c.ChineseAccentCode},
            {"JobCategory", c.JobCategory},
            {"AccompanyNumber", c.AccompanyNumber},
            {"SkinColor", c.SkinColor},
            {"HairStyle", c.HairStyle},
            {"HairColor", c.HairColor},
            {"FaceStyle", c.FaceStyle},
            {"FacialFeature", c.FacialFeature},
            {"PhysicalFeature", c.PhysicalFeature},
            {"RespiratorColor", c.RespiratorColor},
            {"CapStyle", c.CapStyle},
            {"CapColor", c.CapColor},
            {"GlassStyle", c.GlassStyle},
            {"GlassColor", c.GlassColor},
            {"IsDriver", c.IsDriver},
            {"IsForeigner", c.IsForeigner},
            {"PassportType", c.PassportType},
            {"ImmigrantTypeCode", c.ImmigrantTypeCode},
            {"IsSuspectedTerrorist", c.IsSuspectedTerrorist},
            {"SuspectedTerroristNumber", c.SuspectedTerroristNumber},
            {"IsCriminalInvolved", c.IsCriminalInvolved},
            {"CriminalInvolvedSpecilisationCode", c.CriminalInvolvedSpecilisationCode},
            {"BodySpeciallMark", c.BodySpeciallMark},
            {"CrimeMethod", c.CrimeMethod},
            {"CrimeCharacterCode", c.CrimeCharacterCode},
            {"EscapedCriminalNumber", c.EscapedCriminalNumber},
            {"IsDetainees", c.IsDetainees},
            {"DetentionHouseCode", c.DetentionHouseCode},
            {"DetaineesIdentity", c.DetaineesIdentity},
            {"DetaineesSpecialIdentity", c.DetaineesSpecialIdentity},
            {"MemberTypeCode", c.MemberTypeCode},
            {"IsVictim", c.IsVictim},
            {"VictimType", c.VictimType},
            {"InjuredDegree", c.InjuredDegree},
            {"CorpseConditionCode", c.CorpseConditionCode},
            {"IsSuspiciousPerson", c.IsSuspiciousPerson},
            {"Attitude", c.Attitude},
            {"Similaritydegree", c.Similaritydegree},
            {"EyebrowStyle", c.EyebrowStyle},
            {"NoseStyle", c.NoseStyle},
            {"MustacheStyle", c.MustacheStyle},
            {"LipStyle", c.LipStyle},
            {"WrinklePouch", c.WrinklePouch},
            {"AcneStain", c.AcneStain},
            {"FreckleBirthmark", c.FreckleBirthmark},
            {"ScarDimple", c.ScarDimple},
            {"OtherFeature", c.OtherFeature}
        };

        /* 坐标类型处理 */
        handlePointType(j, c);
        /* 宇视 VIID_2018增加字段 */
        if (!c.ShotTime.empty()) {
            j["ShotTime"] = c.ShotTime;
        }

        // 填充SubImageList字段
        for (int i = 0; i < c.SubImageList.size(); i++) {
            nlohmann::json jSubImageObject = c.SubImageList[i];
            j[KEY_SUB_IMAGE_LIST][KEY_SUB_IMAGE_INFO_OBJECT].push_back(jSubImageObject);
        }
    }
} security_face_t;
typedef SecurityArray<security_face_t> security_faces_t;

/**
 * @Struct   security_file_info_t
 * @Brief    FileInfo
 * @DateTime 2018-08-01T10:52:46+0800
 * @Modify   2018-08-01T10:52:46+0800
 * @Author   Nanuns
 */
typedef struct _security_file_info {
    SecurityString FileID;
    SecurityInfoType InfoKind;
    SecurityString Source;
    SecurityString FileName;
    SecurityString StoragePath;
    SecurityString FileHash;
    SecurityString FileFormat;
    SecurityString Title;
    SecurityString SecurityLevel;
    SecurityString SubmiterName;
    SecurityString SubmiterOrg;
    SecurityString EntryTime;
    int            FileSize;
} security_file_info_t;
typedef SecurityArray<security_file_info_t>                 security_file_infos_t;

/**
 * @Struct   security_person_t
 * @Brief
 * @DateTime 2018-08-01T10:52:46+0800
 * @Modify   2018-08-01T10:52:46+0800
 * @Author   Nanuns
 */
typedef struct _security_person_t {
    SecurityString PersonID;
    SecurityInfoType InfoKind;
    SecurityString SourceID;
    SecurityString DeviceID;
    int            LeftTopX                 = 0;
    int            LeftTopY                 = 0;
    int            RightBtmX                = 0;
    int            RightBtmY                = 0;
    SecurityString LocationMarkTime;
    SecurityString PersonAppearTime;
    SecurityString PersonDisAppearTime;
    SecurityString IDType;
    SecurityString IDNumber;
    SecurityString Name;
    SecurityString UsedName;
    SecurityString Alias;
    SecurityString GenderCode;
    int            AgeUpLimit               = 0;
    int            AgeLowerLimit            = 0;
    SecurityString EthicCode;
    SecurityString NationalityCode;
    SecurityString NativeCityCode;
    SecurityString ResidenceAdminDivision;
    SecurityString ChineseAccentCode;
    SecurityString PersonOrg;
    SecurityString JobCategory;
    int            AccompanyNumber          = 0;
    int            HeightUpLimit            = 0;
    int            HeightLowerLimit         = 0;
    SecurityString BodyType;
    SecurityString SkinColor;
    SecurityString HairStyle;
    SecurityString HairColor;
    SecurityString Gesture;
    SecurityString Status;
    SecurityString FaceStyle;
    SecurityString FacialFeature;
    SecurityString PhysicalFeature;
    SecurityString BodyFeature;
    SecurityString HabitualMovement;
    SecurityString Behavior;
    SecurityString BehaviorDescription;
    SecurityString Appendant;
    SecurityString AppendantDescription;
    SecurityString UmbrellaColor;
    SecurityString RespiratorColor;
    SecurityString CapStyle;
    SecurityString CapColor;
    SecurityString GlassStyle;
    SecurityString GlassColor;
    SecurityString ScarfColor;
    SecurityString BagStyle;
    SecurityString BagColor;
    SecurityString CoatStyle;
    SecurityString CoatLength;
    SecurityString CoatColor;
    SecurityString TrousersStyle;
    SecurityString TrousersColor;
    SecurityString TrousersLen;
    SecurityString ShoesStyle;
    SecurityString ShoesColor;
    int            IsDriver                 = 0;
    int            IsForeigner              = 0;
    SecurityString PassportType;
    SecurityString ImmigrantTypeCode;
    int            IsSuspectedTerrorist     = 0;
    SecurityString SuspectedTerroristNumber;
    int            IsCriminalInvolved       = 0;
    SecurityString CriminalInvolvedSpecilisationCode;
    SecurityString BodySpeciallMark;
    SecurityString CrimeMethod;
    SecurityString CrimeCharacterCode;
    SecurityString EscapedCriminalNumber;
    int            IsDetainees              = 0;
    SecurityString DetentionHouseCode;
    SecurityString DetaineesIdentity;
    SecurityString DetaineesSpecialIdentity;
    SecurityString MemberTypeCode;
    int            IsVictim                 = 0;
    SecurityString VictimType;
    SecurityString InjuredDegree;
    SecurityString CorpseConditionCode;
    int            IsSuspiciousPerson       = 0;
    security_subimage_infos_t SubImageList;
    security_point_info_t PointInfo;
    SecurityString ShotTime;    // 宇视 VIID_2018增加字段

    friend void from_json(const nlohmann::json &j, _security_person_t &c) {
        c.PersonID = j.value("PersonID", "");
        c.InfoKind = j.value("InfoKind", SecurityInfoType::Other);
        c.SourceID = j.value("SourceID", "");
        c.DeviceID = j.value("DeviceID", "");
        c.LocationMarkTime = j.value("LocationMarkTime", "");
        c.PersonAppearTime = j.value("PersonAppearTime", "");
        c.PersonDisAppearTime = j.value("PersonDisAppearTime", "");
        c.IDType = j.value("IDType", "");
        c.IDNumber = j.value("IDNumber", "");
        c.Name = j.value("Name", "");
        c.UsedName = j.value("UsedName", "");
        c.Alias = j.value("Alias", "");
        c.GenderCode = j.value("GenderCode", "");
        c.AgeUpLimit = j.value("AgeUpLimit", -1);
        c.AgeLowerLimit = j.value("AgeLowerLimit", -1);
        c.EthicCode = j.value("EthicCode", "");
        c.NationalityCode = j.value("NationalityCode", "");
        c.NativeCityCode = j.value("NativeCityCode", "");
        c.ResidenceAdminDivision = j.value("ResidenceAdminDivision", "");
        c.ChineseAccentCode = j.value("ChineseAccentCode", "");
        c.PersonOrg = j.value("PersonOrg", "");
        c.JobCategory = j.value("JobCategory", "");
        c.AccompanyNumber = j.value("AccompanyNumber", -1);
        c.HeightUpLimit = j.value("HeightUpLimit", -1);
        c.HeightLowerLimit = j.value("HeightLowerLimit", -1);
        c.BodyType = j.value("BodyType", "");
        c.SkinColor = j.value("SkinColor", "");
        c.HairStyle = j.value("HairStyle", "");
        c.HairColor = j.value("HairColor", "");
        c.Gesture = j.value("Gesture", "");
        c.Status = j.value("Status", "");
        c.FaceStyle = j.value("FaceStyle", "");
        c.FacialFeature = j.value("FacialFeature", "");
        c.PhysicalFeature = j.value("PhysicalFeature", "");
        c.BodyFeature = j.value("BodyFeature", "");
        c.HabitualMovement = j.value("HabitualMovement", "");
        c.Behavior = j.value("Behavior", "");
        c.BehaviorDescription = j.value("BehaviorDescription", "");
        c.Appendant = j.value("Appendant", "");
        c.AppendantDescription = j.value("AppendantDescription", "");
        c.UmbrellaColor = j.value("UmbrellaColor", "");
        c.RespiratorColor = j.value("RespiratorColor", "");
        c.CapStyle = j.value("CapStyle", "");
        c.CapColor = j.value("CapColor", "");
        c.GlassStyle = j.value("GlassStyle", "");
        c.GlassColor = j.value("GlassColor", "");
        c.ScarfColor = j.value("ScarfColor", "");
        c.BagStyle = j.value("BagStyle", "");
        c.BagColor = j.value("BagColor", "");
        c.CoatStyle = j.value("CoatStyle", "");
        c.CoatLength = j.value("CoatLength", "");
        c.CoatColor = j.value("CoatColor", "");
        c.TrousersStyle = j.value("TrousersStyle", "");
        c.TrousersColor = j.value("TrousersColor", "");
        c.TrousersLen = j.value("TrousersLen", "");
        c.ShoesStyle = j.value("ShoesStyle", "");
        c.ShoesColor = j.value("ShoesColor", "");
        c.IsDriver = j.value("IsDriver", -1);
        c.IsForeigner = j.value("IsForeigner", -1);
        c.PassportType = j.value("PassportType", "");
        c.ImmigrantTypeCode = j.value("ImmigrantTypeCode", "");
        c.IsSuspectedTerrorist = j.value("IsSuspectedTerrorist", -1);
        c.SuspectedTerroristNumber = j.value("SuspectedTerroristNumber", "");
        c.IsCriminalInvolved = j.value("IsCriminalInvolved", -1);
        c.CriminalInvolvedSpecilisationCode = j.value("CriminalInvolvedSpecilisationCode", "");
        c.BodySpeciallMark = j.value("BodySpeciallMark", "");
        c.CrimeMethod = j.value("CrimeMethod", "");
        c.CrimeCharacterCode = j.value("CrimeCharacterCode", "");
        c.EscapedCriminalNumber = j.value("EscapedCriminalNumber", "");
        c.IsDetainees = j.value("IsDetainees", -1);
        c.DetentionHouseCode = j.value("DetentionHouseCode", "");
        c.DetaineesIdentity = j.value("DetaineesIdentity", "");
        c.DetaineesSpecialIdentity = j.value("DetaineesSpecialIdentity", "");
        c.MemberTypeCode = j.value("MemberTypeCode", "");
        c.IsVictim = j.value("IsVictim", -1);
        c.VictimType = j.value("VictimType", "");
        c.InjuredDegree = j.value("InjuredDegree", "");
        c.CorpseConditionCode = j.value("CorpseConditionCode", "");
        c.IsSuspiciousPerson = j.value("IsSuspiciousPerson", -1);

        // SubImageList
        nlohmann::json jSubImageInfoObject = j[KEY_SUB_IMAGE_LIST][KEY_SUB_IMAGE_INFO_OBJECT];
        for(int i = 0; i < jSubImageInfoObject.size(); i++) {
            c.SubImageList.emplace_back(jSubImageInfoObject[i].get<_security_subimage_info_t>());
        }
    }

    friend void to_json(nlohmann::json &j, const _security_person_t &c) {
        j = nlohmann::json {
            {"PersonID", c.PersonID},
            {"InfoKind", c.InfoKind},
            {"SourceID", c.SourceID},
            {"DeviceID", c.DeviceID},
            {"LocationMarkTime", c.LocationMarkTime},
            {"PersonAppearTime", c.PersonAppearTime},
            {"PersonDisAppearTime", c.PersonDisAppearTime},
            {"IDType", c.IDType},
            {"IDNumber", c.IDNumber},
            {"Name", c.Name},
            {"UsedName", c.UsedName},
            {"Alias", c.Alias},
            {"GenderCode", c.GenderCode},
            {"AgeUpLimit", c.AgeUpLimit},
            {"AgeLowerLimit", c.AgeLowerLimit},
            {"EthicCode", c.EthicCode},
            {"NationalityCode", c.NationalityCode},
            {"NativeCityCode", c.NativeCityCode},
            {"ResidenceAdminDivision", c.ResidenceAdminDivision},
            {"ChineseAccentCode", c.ChineseAccentCode},
            {"PersonOrg", c.PersonOrg},
            {"JobCategory", c.JobCategory},
            {"AccompanyNumber", c.AccompanyNumber},
            {"HeightUpLimit", c.HeightUpLimit},
            {"HeightLowerLimit", c.HeightLowerLimit},
            {"BodyType", c.BodyType},
            {"UsedSkinColorName", c.SkinColor},
            {"HairStyle", c.HairStyle},
            {"HairColor", c.HairColor},
            {"Gesture", c.Gesture},
            {"Status", c.Status},
            {"FaceStyle", c.FaceStyle},
            {"FacialFeature", c.FacialFeature},
            {"PhysicalFeature", c.PhysicalFeature},
            {"BodyFeature", c.BodyFeature},
            {"HabitualMovement", c.HabitualMovement},
            {"Behavior", c.Behavior},
            {"BehaviorDescription", c.BehaviorDescription},
            {"Appendant", c.Appendant},
            {"AppendantDescription", c.AppendantDescription},
            {"UmbrellaColor", c.UmbrellaColor},
            {"RespiratorColor", c.RespiratorColor},
            {"CapStyle", c.CapStyle},
            {"CapColor", c.CapColor},
            {"GlassStyle", c.GlassStyle},
            {"GlassColor", c.GlassColor},
            {"ScarfColor", c.ScarfColor},
            {"BagStyle", c.BagStyle},
            {"BagColor", c.BagColor},
            {"CoatStyle", c.CoatStyle},
            {"CoatLength", c.CoatLength},
            {"CoatColor", c.CoatColor},
            {"TrousersStyle", c.TrousersStyle},
            {"TrousersColor", c.TrousersColor},
            {"TrousersLen", c.TrousersLen},
            {"ShoesStyle", c.ShoesStyle},
            {"ShoesColor", c.ShoesColor},
            {"IsDriver", c.IsDriver},
            {"IsForeigner", c.IsForeigner},
            {"PassportType", c.PassportType},
            {"ImmigrantTypeCode", c.ImmigrantTypeCode},
            {"IsSuspectedTerrorist", c.IsSuspectedTerrorist},
            {"SuspectedTerroristNumber", c.SuspectedTerroristNumber},
            {"IsCriminalInvolved", c.IsCriminalInvolved},
            {"CriminalInvolvedSpecilisationCode", c.CriminalInvolvedSpecilisationCode},
            {"BodySpeciallMark", c.BodySpeciallMark},
            {"CrimeMethod", c.CrimeMethod},
            {"CrimeCharacterCode", c.CrimeCharacterCode},
            {"EscapedCriminalNumber", c.EscapedCriminalNumber},
            {"IsDetainees", c.IsDetainees},
            {"DetentionHouseCode", c.DetentionHouseCode},
            {"DetaineesIdentity", c.DetaineesIdentity},
            {"DetaineesSpecialIdentity", c.DetaineesSpecialIdentity},
            {"MemberTypeCode", c.MemberTypeCode},
            {"IsVictim", c.IsVictim},
            {"VictimType", c.VictimType},
            {"InjuredDegree", c.InjuredDegree},
            {"CorpseConditionCode", c.CorpseConditionCode},
            {"IsSuspiciousPerson", c.IsSuspiciousPerson}
        };
        
        /* 坐标类型处理 */
        handlePointType(j, c);
        /* 宇视 VIID_2018增加字段 */
        if (!c.ShotTime.empty()) {
            j["ShotTime"] = c.ShotTime;
        }

        // 填充SubImageList字段
        for (int i = 0; i < c.SubImageList.size(); i++) {
            nlohmann::json jSubImageObject = c.SubImageList[i];
            j[KEY_SUB_IMAGE_LIST][KEY_SUB_IMAGE_INFO_OBJECT].push_back(jSubImageObject);
        }
    }
} security_person_t;
typedef SecurityArray<security_person_t> security_persons_t;

/**
 * @Struct   security_motorvehicle_t
 * @Brief    ����������
 * @DateTime 2018/8/10 11:43:12
 * @Modify   2018/8/10 11:43:18
 * @Author   Nanuns
 */
typedef struct _security_motorvehicle_t {
    SecurityString MotorVehicleID;
    SecurityInfoType InfoKind;
    SecurityString SourceID;
    SecurityString TollgateID;
    SecurityString DeviceID;
    SecurityString StorageUrl1;
    SecurityString StorageUrl2;
    SecurityString StorageUrl3;
    SecurityString StorageUrl4;
    SecurityString StorageUrl5;
    int            LeftTopX                 = 0;
    int            LeftTopY                 = 0;
    int            RightBtmX                = 0;
    int            RightBtmY                = 0;
    SecurityString MarkTime;
    SecurityString AppearTime;
    SecurityString DisappearTime;
    int            LaneNo                   = 0;
    SecurityString HasPlate;
    SecurityString PlateClass;
    SecurityString PlateColor;
    SecurityString PlateNo;
    SecurityString PlateNoAttach;
    SecurityString PlateDescribe;
    SecurityString IsDecked;
    SecurityString IsAltered;
    SecurityString IsCovered;
    SpeedType      Speed                    = -1;
    SecurityString Direction;
    SecurityString DrivingStatusCode;
    int            UsingPropertiesCode      = 0;
    SecurityString VehicleClass;
    SecurityString VehicleBrand;
    SecurityString VehicleModel;
    SecurityString VehicleStyles;
    int            VehicleLength            = 0;
    int            VehicleWidth             = 0;
    int            VehicleHeight            = 0;
    SecurityString VehicleColor;
    SecurityString VehicleColorDepth;
    SecurityString VehicleHood;
    SecurityString VehicleTrunk;
    SecurityString VehicleWheel;
    SecurityString WheelPrintedPattern;
    SecurityString VehicleWindow;
    SecurityString VehicleRoof;
    SecurityString VehicleDoor;
    SecurityString SideOfVehicle;
    SecurityString CarOfVehicle;
    SecurityString RearviewMirror;
    SecurityString VehicleChassis;
    SecurityString VehicleShielding;
    SecurityString FilmColor;
    SecurityString IsModified;
    SecurityString HitMarkInfo;
    SecurityString VehicleBodyDesc;
    SecurityString VehicleFrontItem;
    SecurityString DescOfFrontItem;
    SecurityString VehicleRearItem;
    SecurityString DescOfRearItem;
    int            NumOfPassenger           = 0;
    SecurityString PassTime;
    SecurityString NameOfPassedRoad;
    SecurityString IsSuspicious;
    int            Sunvisor                 = 0;
    int            SafetyBelt               = 0;
    int            Calling                  = 0;
    SecurityString PlateReliability;
    SecurityString PlateCharReliability;
    SecurityString BrandReliability;
    security_subimage_infos_t SubImageList;
    security_point_info_t PointInfo;
    SecurityString ShotTime;    // 宇视 VIID_2018增加字段

    friend void from_json(const nlohmann::json &j, _security_motorvehicle_t &c) {
        json_get_if_exists(j, "MotorVehicleID", c.MotorVehicleID);
        json_get_if_exists(j, "InfoKind", c.InfoKind);
        json_get_if_exists(j, "SourceID", c.SourceID);
        json_get_if_exists(j, "TollgateID", c.TollgateID);
        json_get_if_exists(j, "DeviceID", c.DeviceID);
        json_get_if_exists(j, "StorageUrl1", c.StorageUrl1);
        json_get_if_exists(j, "StorageUrl2", c.StorageUrl2);
        json_get_if_exists(j, "StorageUrl3", c.StorageUrl3);
        json_get_if_exists(j, "StorageUrl4", c.StorageUrl4);
        json_get_if_exists(j, "StorageUrl5", c.StorageUrl5);
        json_get_if_exists(j, "MarkTime", c.MarkTime);
        json_get_if_exists(j, "AppearTime", c.AppearTime);
        json_get_if_exists(j, "DisappearTime", c.DisappearTime);
        json_get_if_exists(j, "LaneNo", c.LaneNo);
        json_get_if_exists(j, "HasPlate", c.HasPlate);
        json_get_if_exists(j, "PlateClass", c.PlateClass);
        json_get_if_exists(j, "PlateColor", c.PlateColor);
        json_get_if_exists(j, "PlateNo", c.PlateNo);
        json_get_if_exists(j, "PlateNoAttach", c.PlateNoAttach);
        json_get_if_exists(j, "PlateDescribe", c.PlateDescribe);
        json_get_if_exists(j, "IsDecked", c.IsDecked);
        json_get_if_exists(j, "IsAltered", c.IsAltered);
        json_get_if_exists(j, "IsCovered", c.IsCovered);
        json_get_if_exists(j, "Speed", c.Speed);
        json_get_if_exists(j, "Direction", c.Direction);
        json_get_if_exists(j, "DrivingStatusCode", c.DrivingStatusCode);
        json_get_if_exists(j, "UsingPropertiesCode", c.UsingPropertiesCode);
        json_get_if_exists(j, "VehicleClass", c.VehicleClass);
        json_get_if_exists(j, "VehicleBrand", c.VehicleBrand);
        json_get_if_exists(j, "VehicleModel", c.VehicleModel);
        json_get_if_exists(j, "VehicleStyles", c.VehicleStyles);
        json_get_if_exists(j, "VehicleLength", c.VehicleLength);
        json_get_if_exists(j, "VehicleWidth", c.VehicleWidth);
        json_get_if_exists(j, "VehicleHeight", c.VehicleHeight);
        json_get_if_exists(j, "VehicleColor", c.VehicleColor);
        json_get_if_exists(j, "VehicleColorDepth", c.VehicleColorDepth);
        json_get_if_exists(j, "VehicleHood", c.VehicleHood);
        json_get_if_exists(j, "VehicleTrunk", c.VehicleTrunk);
        json_get_if_exists(j, "VehicleWheel", c.VehicleWheel);
        json_get_if_exists(j, "WheelPrintedPattern", c.WheelPrintedPattern);
        json_get_if_exists(j, "VehicleWindow", c.VehicleWindow);
        json_get_if_exists(j, "VehicleRoof", c.VehicleRoof);
        json_get_if_exists(j, "VehicleDoor", c.VehicleDoor);
        json_get_if_exists(j, "SideOfVehicle", c.SideOfVehicle);
        json_get_if_exists(j, "CarOfVehicle", c.CarOfVehicle);
        json_get_if_exists(j, "RearviewMirror", c.RearviewMirror);
        json_get_if_exists(j, "VehicleChassis", c.VehicleChassis);
        json_get_if_exists(j, "VehicleShielding", c.VehicleShielding);
        json_get_if_exists(j, "FilmColor", c.FilmColor);
        json_get_if_exists(j, "IsModified", c.IsModified);
        json_get_if_exists(j, "HitMarkInfo", c.HitMarkInfo);
        json_get_if_exists(j, "VehicleBodyDesc", c.VehicleBodyDesc);
        json_get_if_exists(j, "VehicleFrontItem", c.VehicleFrontItem);
        json_get_if_exists(j, "DescOfFrontItem", c.DescOfFrontItem);
        json_get_if_exists(j, "VehicleRearItem", c.VehicleRearItem);
        json_get_if_exists(j, "DescOfRearItem", c.DescOfRearItem);
        json_get_if_exists(j, "NumOfPassenger", c.NumOfPassenger);
        json_get_if_exists(j, "PassTime", c.PassTime);
        json_get_if_exists(j, "NameOfPassedRoad", c.NameOfPassedRoad);
        json_get_if_exists(j, "IsSuspicious", c.IsSuspicious);
        json_get_if_exists(j, "Sunvisor", c.Sunvisor);
        json_get_if_exists(j, "SafetyBelt", c.SafetyBelt);
        json_get_if_exists(j, "Calling", c.Calling);
        json_get_if_exists(j, "PlateReliability", c.PlateReliability);
        json_get_if_exists(j, "PlateCharReliability", c.PlateCharReliability);
        json_get_if_exists(j, "BrandReliability", c.BrandReliability);
        // SubImageList
        nlohmann::json jSubImageInfoObject = j[KEY_SUB_IMAGE_LIST][KEY_SUB_IMAGE_INFO_OBJECT];
        for(int i = 0; i < jSubImageInfoObject.size(); i++) {
            c.SubImageList.emplace_back(jSubImageInfoObject[i].get<_security_subimage_info_t>());
        }
    }

    friend void to_json(nlohmann::json &j, const _security_motorvehicle_t &c) {
        j = nlohmann::json {
            {"MotorVehicleID", c.MotorVehicleID},
            {"InfoKind", c.InfoKind},
            {"SourceID", c.SourceID},
            {"TollgateID", c.TollgateID},
            {"DeviceID", c.DeviceID},
            {"StorageUrl1", c.StorageUrl1},
            {"StorageUrl2", c.StorageUrl2},
            {"StorageUrl3", c.StorageUrl3},
            {"StorageUrl4", c.StorageUrl4},
            {"StorageUrl5", c.StorageUrl5},
            {"MarkTime", c.MarkTime},
            {"AppearTime", c.AppearTime},
            {"DisappearTime", c.DisappearTime},
            {"LaneNo", c.LaneNo},
            {"HasPlate", c.HasPlate},
            {"PlateClass", c.PlateClass},
            {"PlateColor", c.PlateColor},
            {"PlateNo", c.PlateNo},
            {"PlateNoAttach", c.PlateNoAttach},
            {"PlateDescribe", c.PlateDescribe},
            {"IsDecked", c.IsDecked},
            {"IsAltered", c.IsAltered},
            {"IsCovered", c.IsCovered},
            {"Speed", c.Speed},
            {"Direction", c.Direction},
            {"DrivingStatusCode", c.DrivingStatusCode},
            {"UsingPropertiesCode", c.UsingPropertiesCode},
            {"VehicleClass", c.VehicleClass},
            {"VehicleBrand", c.VehicleBrand},
            {"VehicleModel", c.VehicleModel},
            {"VehicleStyles", c.VehicleStyles},
            {"VehicleLength", c.VehicleLength},
            {"VehicleWidth", c.VehicleWidth},
            {"VehicleHeight", c.VehicleHeight},
            {"VehicleColor", c.VehicleColor},
            {"VehicleColorDepth", c.VehicleColorDepth},
            {"VehicleHood", c.VehicleHood},
            {"VehicleTrunk", c.VehicleTrunk},
            {"VehicleWheel", c.VehicleWheel},
            {"WheelPrintedPattern", c.WheelPrintedPattern},
            {"VehicleWindow", c.VehicleWindow},
            {"VehicleRoof", c.VehicleRoof},
            {"VehicleDoor", c.VehicleDoor},
            {"SideOfVehicle", c.SideOfVehicle},
            {"CarOfVehicle", c.CarOfVehicle},
            {"RearviewMirror", c.RearviewMirror},
            {"VehicleChassis", c.VehicleChassis},
            {"VehicleShielding", c.VehicleShielding},
            // 这两个字段在EasyCVR上报不正确格式
            // {"FilmColor", c.FilmColor},
            // {"IsModified", c.IsModified},
            {"HitMarkInfo", c.HitMarkInfo},
            {"VehicleBodyDesc", c.VehicleBodyDesc},
            {"VehicleFrontItem", c.VehicleFrontItem},
            {"DescOfFrontItem", c.DescOfFrontItem},
            {"VehicleRearItem", c.VehicleRearItem},
            {"DescOfRearItem", c.DescOfRearItem},
            {"NumOfPassenger", c.NumOfPassenger},
            {"PassTime", c.PassTime},
            {"NameOfPassedRoad", c.NameOfPassedRoad},
            {"IsSuspicious", c.IsSuspicious},
            {"Sunvisor", c.Sunvisor},
            {"SafetyBelt", c.SafetyBelt},
            {"Calling", c.Calling},
            {"PlateReliability", c.PlateReliability},
            {"PlateCharReliability", c.PlateCharReliability},
            {"BrandReliability", c.BrandReliability}
        };

        /* 坐标类型处理 */
        handlePointType(j, c);
        /* 宇视 VIID_2018增加字段 */
        if (!c.ShotTime.empty()) {
            j["ShotTime"] = c.ShotTime;
        }

        // 填充SubImageList字段
        for (int i = 0; i < c.SubImageList.size(); i++) {
            nlohmann::json jSubImageObject = c.SubImageList[i];
            j[KEY_SUB_IMAGE_LIST][KEY_SUB_IMAGE_INFO_OBJECT].push_back(jSubImageObject);
        }
    }
} security_motorvehicle_t;
typedef SecurityArray<security_motorvehicle_t> security_motorvehicles_t;

/**
 * @Struct   security_nonmotorvehicle_t
 * @Brief    �ǻ���������
 * @DateTime 2018/8/10 11:20:41
 * @Modify   2018/8/10 11:20:52
 * @Author   Nanuns
 */
typedef struct _security_nonmotorvehicle_t {
    SecurityString NonMotorVehicleID;
    SecurityInfoType InfoKind;
    SecurityString SourceID;
    SecurityString DeviceID;
    int            LeftTopX                 = 0;
    int            LeftTopY                 = 0;
    int            RightBtmX                = 0;
    int            RightBtmY                = 0;
    SecurityString MarkTime;
    SecurityString AppearTime;
    SecurityString DisappearTime;
    SecurityString HasPlate;
    SecurityString PlateClass;
    SecurityString PlateColor;
    SecurityString PlateNo;
    SecurityString PlateNoAttach;
    SecurityString PlateDescribe;
    SecurityString IsDecked;
    SecurityString IsAltered;
    SecurityString IsCovered;
    SpeedType      Speed                    = -1;
    SecurityString DrivingStatusCode;
    int            UsingPropertiesCode      = 0;
    SecurityString VehicleBrand;
    SecurityString VehicleType;
    int            VehicleLength            = 0;
    int            VehicleWidth             = 0;
    int            VehicleHeight            = 0;
    SecurityString VehicleColor;
    SecurityString VehicleHood;
    SecurityString VehicleTrunk;
    SecurityString VehicleWheel;
    SecurityString WheelPrintedPattern;
    SecurityString VehicleWindow;
    SecurityString VehicleRoof;
    SecurityString VehicleDoor;
    SecurityString SideOfVehicle;
    SecurityString CarOfVehicle;
    SecurityString RearviewMirror;
    SecurityString VehicleChassis;
    SecurityString VehicleShielding;
    int            FilmColor                = 0;
    int            IsModified               = 0;
    security_subimage_infos_t SubImageList;
    security_point_info_t PointInfo;
    SecurityString ShotTime;    // 宇视 VIID_2018增加字段

    friend void from_json(const nlohmann::json &j, _security_nonmotorvehicle_t &c) {
        json_get_if_exists(j, "NonMotorVehicleID", c.NonMotorVehicleID);
        json_get_if_exists(j, "InfoKind", c.InfoKind);
        json_get_if_exists(j, "SourceID", c.SourceID);
        json_get_if_exists(j, "DeviceID", c.DeviceID);
        json_get_if_exists(j, "MarkTime", c.MarkTime);
        json_get_if_exists(j, "AppearTime", c.AppearTime);
        json_get_if_exists(j, "DisappearTime", c.DisappearTime);
        json_get_if_exists(j, "HasPlate", c.HasPlate);
        json_get_if_exists(j, "PlateClass", c.PlateClass);
        json_get_if_exists(j, "PlateColor", c.PlateColor);
        json_get_if_exists(j, "PlateNo", c.PlateNo);
        json_get_if_exists(j, "PlateNoAttach", c.PlateNoAttach);
        json_get_if_exists(j, "PlateDescribe", c.PlateDescribe);
        json_get_if_exists(j, "IsDecked", c.IsDecked);
        json_get_if_exists(j, "IsAltered", c.IsAltered);
        json_get_if_exists(j, "IsCovered", c.IsCovered);
        json_get_if_exists(j, "Speed", c.Speed);
        json_get_if_exists(j, "DrivingStatusCode", c.DrivingStatusCode);
        json_get_if_exists(j, "UsingPropertiesCode", c.UsingPropertiesCode);
        json_get_if_exists(j, "VehicleBrand", c.VehicleBrand);
        json_get_if_exists(j, "VehicleType", c.VehicleType);
        json_get_if_exists(j, "VehicleLength", c.VehicleLength);
        json_get_if_exists(j, "VehicleWidth", c.VehicleWidth);
        json_get_if_exists(j, "VehicleHeight", c.VehicleHeight);
        json_get_if_exists(j, "VehicleColor", c.VehicleColor);
        json_get_if_exists(j, "VehicleHood", c.VehicleHood);
        json_get_if_exists(j, "VehicleTrunk", c.VehicleTrunk);
        json_get_if_exists(j, "VehicleWheel", c.VehicleWheel);
        json_get_if_exists(j, "WheelPrintedPattern", c.WheelPrintedPattern);
        json_get_if_exists(j, "VehicleWindow", c.VehicleWindow);
        json_get_if_exists(j, "VehicleRoof", c.VehicleRoof);
        json_get_if_exists(j, "VehicleDoor", c.VehicleDoor);
        json_get_if_exists(j, "SideOfVehicle", c.SideOfVehicle);
        json_get_if_exists(j, "CarOfVehicle", c.CarOfVehicle);
        json_get_if_exists(j, "RearviewMirror", c.RearviewMirror);
        json_get_if_exists(j, "VehicleChassis", c.VehicleChassis);
        json_get_if_exists(j, "VehicleShielding", c.VehicleShielding);
        json_get_if_exists(j, "FilmColor", c.FilmColor);
        json_get_if_exists(j, "IsModified", c.IsModified);

        // SubImageList
        nlohmann::json jSubImageInfoObject = j[KEY_SUB_IMAGE_LIST][KEY_SUB_IMAGE_INFO_OBJECT];
        for(int i = 0; i < jSubImageInfoObject.size(); i++) {
            c.SubImageList.emplace_back(jSubImageInfoObject[i].get<_security_subimage_info_t>());
        }
    }

    friend void to_json(nlohmann::json &j, const _security_nonmotorvehicle_t &c) {
        j = nlohmann::json {
            {"NonMotorVehicleID", c.NonMotorVehicleID},
            {"InfoKind", c.InfoKind},
            {"SourceID", c.SourceID},
            {"DeviceID", c.DeviceID},
            {"MarkTime", c.MarkTime},
            {"AppearTime", c.AppearTime},
            {"DisappearTime", c.DisappearTime},
            {"HasPlate", c.HasPlate},
            {"PlateClass", c.PlateClass},
            {"PlateColor", c.PlateColor},
            {"PlateNo", c.PlateNo},
            {"PlateNoAttach", c.PlateNoAttach},
            {"PlateDescribe", c.PlateDescribe},
            {"IsDecked", c.IsDecked},
            {"IsAltered", c.IsAltered},
            {"IsCovered", c.IsCovered},
            {"Speed", c.Speed},
            {"DrivingStatusCode", c.DrivingStatusCode},
            {"UsingPropertiesCode", c.UsingPropertiesCode},
            {"VehicleBrand", c.VehicleBrand},
            {"VehicleType", c.VehicleType},
            {"VehicleLength", c.VehicleLength},
            {"VehicleWidth", c.VehicleWidth},
            {"VehicleHeight", c.VehicleHeight},
            {"VehicleColor", c.VehicleColor},
            {"VehicleHood", c.VehicleHood},
            {"VehicleTrunk", c.VehicleTrunk},
            {"VehicleWheel", c.VehicleWheel},
            {"WheelPrintedPattern", c.WheelPrintedPattern},
            {"VehicleWindow", c.VehicleWindow},
            {"VehicleRoof", c.VehicleRoof},
            {"VehicleDoor", c.VehicleDoor},
            {"SideOfVehicle", c.SideOfVehicle},
            {"CarOfVehicle", c.CarOfVehicle},
            {"RearviewMirror", c.RearviewMirror},
            {"VehicleChassis", c.VehicleChassis},
            {"VehicleShielding", c.VehicleShielding},
            {"FilmColor", c.FilmColor},
            {"IsModified", c.IsModified}
        };

        /* 坐标类型处理 */
        handlePointType(j, c);
        /* 宇视 VIID_2018增加字段 */
        if (!c.ShotTime.empty()) {
            j["ShotTime"] = c.ShotTime;
        }

        // 填充SubImageList字段
        for (int i = 0; i < c.SubImageList.size(); i++) {
            nlohmann::json jSubImageObject = c.SubImageList[i];
            j[KEY_SUB_IMAGE_LIST][KEY_SUB_IMAGE_INFO_OBJECT].push_back(jSubImageObject);
        }
    }
} security_nonmotorvehicle_t;
typedef SecurityArray<security_nonmotorvehicle_t> security_nonmotorvehicles_t;

/**
 * @Struct   security_thing_t
 * @Brief    ��Ʒ����
 * @DateTime 2018/8/10 13:17:10
 * @Modify   2018/8/10 13:17:13
 * @Author   Nanuns
 */
typedef struct _security_thing_t {
    SecurityString ThingID;
    SecurityInfoType InfoKind;
    SecurityString SourceID;
    SecurityString DeviceID;
    int            LeftTopX;
    int            LeftTopY;
    int            RightBtmX;
    int            RightBtmY;
    SecurityString LocationMarkTime;
    SecurityString AppearTime;
    SecurityString DisappearTime;
    SecurityString Name;
    SecurityString Shape;
    SecurityString Color;
    SecurityString Size;
    SecurityString Material;
    SecurityString Characteristic;
    SecurityString Propertiy;
    SecurityString InvolvedObjectType;
    SecurityString FirearmsAmmunitionType;
    SecurityString ToolTraceType;
    SecurityString EvidenceType;
    SecurityString CaseEvidenceType;
    security_subimage_infos_t SubImageList;
} security_thing_t;
typedef SecurityArray<security_thing_t> security_things_t;

/**
 * @Struct   security_scene_t
 * @Brief    ��������
 * @DateTime 2018/8/10 13:17:10
 * @Modify   2018/8/10 13:17:13
 * @Author   Nanuns
 */
typedef struct _security_scene_t {
    SecurityString SceneID;
    SecurityInfoType InfoKind;
    SecurityString SourceID;
    SecurityString DeviceID;
    SecurityString BeginTime;
    SecurityString PlaceType;
    SecurityString WeatherType;
    SecurityString SceneDescribe;
    SecurityString SceneType;
    SecurityString RoadAlignmentType;
    int            RoadTerraintype;
    SecurityString RoadSurfaceType;
    SecurityString RoadCoditionType;
    SecurityString RoadJunctionSectionType;
    SecurityString RoadLightingType;
    SecurityString Illustration;
    SecurityString WindDirection;
    SecurityString Illumination;
    SecurityString FieldCondition;
    double         Temperature;
    SecurityString Humidity;
    SecurityString PopulationDensity;
    SecurityString DenseDegree;
    int            Importance;
    security_subimage_infos_t SubImageList;
} security_scene_t;
typedef SecurityArray<security_scene_t> security_scenes_t;

/**
 * @Struct   security_caseinfo_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_caseinfo_t {
    SecurityString CaseID;
    SecurityString CaseLinkMark;
    SecurityString CaseName;
    SecurityString CaseAbstract;
    SecurityString ClueID;
    SecurityString TimeUpLimit;
    SecurityString TimeLowerLimit;
    SecurityString CreateTime;
    SecurityString PlaceCode;
    SecurityString PlaceFullAddress;
    int            SuspectNumber;
    SecurityString WitnessIDs;
    SecurityString CreatorName;
    SecurityString CreatorIDType;
    SecurityString CreatorID;
    SecurityString CreatorOrg;
    LongitudeType  Longitude;
    LatitudeType   Latitude;
    SecurityString EventIDs;
    SecurityString MotorVehicleIDs;
    SecurityString NonMotorVehicleIDs;
    SecurityString PersonIDs;
    SecurityString FaceIDs;
    SecurityString ThingIDs;
    SecurityString FileIDs;
    SecurityString SceneIDs;
    SecurityString RelateCaseIdList;
    SecurityString ParentCaseId;
    int            State;
} security_caseinfo_t;
typedef SecurityArray<security_caseinfo_t>                  security_caseinfos_t;

/**
 * @Struct   security_videoslice_t
 * @Brief
 * @DateTime 2018/8/10 11:20:41
 * @Modify   2018/8/10 11:20:52
 * @Author   Nanuns
 */
typedef struct _security_videoslice_t {
    security_videoslice_info_t  VideoSliceInfo;
    security_persons_t          PersonList;
    security_faces_t            FaceList;
    security_motorvehicles_t    MotorVehicleList;
    security_nonmotorvehicles_t NonmotorVehicleList;
    security_things_t           ThingList;
    security_scenes_t           SceneList;
    SecurityString              Data;
} security_videoslice_t;
typedef SecurityArray<security_videoslice_t> security_videoslices_t;

/**
 * @Struct   security_image_t
 * @Brief
 * @DateTime 2018/8/10 11:19:14
 * @Modify   2018/8/10 11:19:20
 * @Author   Nanuns
 */
typedef struct _security_image_t {
    security_image_info_t       ImageInfo;
    security_persons_t          PersonList;
    security_faces_t            FaceList;
    security_motorvehicles_t    MotorVehicleList;
    security_nonmotorvehicles_t NonMotorVehicleList;
    security_things_t           ThingList;
    security_scenes_t           SceneList;
    SecurityString              Data;
} security_image_t;
typedef SecurityArray<security_image_t>                     security_images_t;

/**
 * @Struct   security_file_t
 * @Brief    File
 * @DateTime 2018-08-01T10:52:46+0800
 * @Modify   2018-08-01T10:52:46+0800
 * @Author   Nanuns
 */
typedef struct _security_file {
    security_file_info_t FileInfo;
    security_persons_t PersonList;
    security_faces_t FaceList;
    security_motorvehicles_t MotorVehicleList;
    security_nonmotorvehicles_t NonMotorVehicleList;
    security_things_t ThingList;
    security_scenes_t SceneList;
    SecurityString Data;
} security_file_t;
typedef SecurityArray<security_file_t>                      security_files_t;

/**
 * @Struct   security_case_t
 * @Brief    ��Ƶ���¼�����
 * @DateTime 2018/8/10 13:28:48
 * @Modify   2018/8/10 13:28:52
 * @Author   Nanuns
 */
typedef struct _security_case_t {
    security_caseinfo_t CaseInfo;
    security_videoslices_t VideoSliceList;
    security_images_t ImageList;
    security_files_t FileList;
    security_persons_t PersonList;
    security_faces_t FaceList;
    security_motorvehicles_t MotorVehicleList;
    security_nonmotorvehicles_t NonMotorVehicleList;
    security_things_t ThingList;
    security_scenes_t SceneList;
} security_case_t;
typedef SecurityArray<security_case_t> security_cases_t;

/**
 * @Struct   security_videolabel_t
 * @Brief    ��Ƶͼ���ǩ����
 * @DateTime 2018/8/10 13:39:59
 * @Modify   2018/8/10 13:40:04
 * @Author   Nanuns
 */
typedef struct _security_videolabel_t {
    typedef struct _color_area_t {
        int AreaPosX;
        int AreaPosY;
        int AreaWidth;
        int AreaHeight;
    } color_area_t;
    typedef SecurityArray<color_area_t> color_areas_t;

    typedef struct _target_t {
        int PosX;
        int PosY;
        int Width;
        int Height;
        int Status;
        int SpeedVal;
        int SpeedRad;
    } target_t;
    typedef SecurityArray<target_t> targets_t;

    typedef struct _behavior_analysis_t {
        int            EventLevel;
        SecurityString BehaviorBeginTime;
        SecurityString BehaviorBeginTimeRlt;
        int            BehaviorBeginFrameNoRlt;
        SecurityString BehaviorEndTime;
        SecurityString BehaviorEndTimeRlt;
        int            BehaviorEndFrameNoRlt;
    } behavior_analysis_t;
    //typedef SecurityArray<behavior_analysis_t> behavior_analysiss_t;

    SecurityString VideoLabelID;
    SecurityString EventSort;
    SecurityString EventRuleID;
    SecurityString VideoImageID;
    SecurityString VideoImageUrl;
    SecurityString CameraID;
    SecurityString IVADeviceID;
    SecurityString CreateTimeAbs;
    int            CreateTimeRlt;
    int64_t        CreateFrameNoRlt;
    SecurityString PersonID;
    SecurityString FaceID;
    SecurityString MotorVehicleID;
    SecurityString NonMotorVehicleID;
    SecurityString ThingID;
    SecurityString SceneID;
    SecurityString TargetColor;
    int            ColorCount;
    color_areas_t  ColorAreaSet;
    int            MoveObjectNum;
    targets_t      MoveObjectSet;
    behavior_analysis_t BehaviorAnalysisObject;
    SecurityString ImagePath;
    SecurityString Desc;
    int            TargetNum;
    int            PersonNum;
    int            FaceNum;
    int            VehicleNum;
    int            ThingNum;
    int            TargetDensityAbs;
    int            PersonDensityAbs;
    int            FacenDensityAbs;
    int            VehicleDensityAbs;
    int            ThingDensityAbs;
    int            TargetDensityRlt;
    int            PersonDensityRlt;
    int            FacenDensityRlt;
    int            VehicleDensityRlt;
    int            ThingDensityRlt;
    int            TotalTargetFlowRate;
    int            PersonFlowRate;
    int            VehicleFlowRate;
    int            FlowDirection;
} security_videolabel_t;
typedef SecurityArray<security_videolabel_t>                security_videolabels_t;
/**
 * @Struct   security_videolabel_all_content_t
 * @Brief
 * @DateTime 2018/8/10 11:20:41
 * @Modify   2018/8/10 11:20:52
 * @Author   Nanuns
 */
typedef struct _security_videolabel_all_content_t {

} security_videolabel_all_content_t;
typedef SecurityArray<security_videolabel_all_content_t>    security_videolabel_all_contents_t;

/**
 * @Struct   security_analysis_rule_t
 * @Brief    ��Ƶͼ������������
 * @DateTime 2018/8/10 13:54:47
 * @Modify   2018/8/10 13:54:50
 * @Author   Nanuns
 */
typedef struct _security_analysis_rule_t {
    typedef struct _point_t {
        int PointX;
        int PointY;
    } point_t;
    typedef SecurityArray<point_t> points_t;

    typedef struct _line_t {
        int Direction;
        points_t PointObjectList;
    } line_t;
    typedef SecurityArray<line_t> lines_t;

    typedef struct _direction_type_t {
        int Direction;
        int TargetType;
    } direction_type_t;
    typedef SecurityArray<direction_type_t> directions_t;

    SecurityString AnalysisRuleID;
    SecurityString VideoImageID;
    SecurityString VideoImageUrl;
    SecurityString CameraID;
    int            Width;
    int            Height;
    int            MinDuration;
    int            LineMinDuration;
    int            LineMaxDuration;
    int            MaxDuration;
    int            LineNum;
    lines_t        LineSet;
    int            PointNum;
    int            PointID;
    points_t       PointSet;
    int            Direction;
    int            ActionType;
    int            BeginPointX;
    int            BeginPointY;
    int            EndPointX;
    int            EndPointY;
    int            DirectionNum;
    directions_t   DirectionSet;
    int            DensityUnit;
} security_analysis_rule_t;
typedef SecurityArray<security_analysis_rule_t>             security_analysis_rules_t;

/**
 * @Struct   security_device_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_device_t {

} security_device_t;
typedef SecurityArray<security_device_t> security_devices_t;

/**
 * @Struct   security_device_status_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_device_status_t {

} security_device_status_t;
typedef SecurityArray<security_device_status_t> security_device_statuss_t;

/**
 * @Struct   security_lane_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_lane_t {
    int            id;
    SecurityString tollgateId;
    int            laneNo;
    SecurityString name;
    SecurityString direction;
    SecurityString desc;
    int            maxSpeed;
    int            cityPass;
    SecurityString apeId;
} security_lane_t;
typedef SecurityArray<security_lane_t> security_lanes_t;

/**
 * @Struct   security_tollgate_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_tollgate_t {

} security_tollgate_t;
typedef SecurityArray<security_tollgate_t> security_tollgates_t;

/**
 * @Struct   security_ape_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_ape_t {

} security_ape_t;
typedef SecurityArray<security_ape_t> security_apes_t;

/**
 * @Struct   security_aps_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_aps_t {

} security_aps_t;
typedef SecurityArray<security_aps_t> security_apss_t;

/**
 * @Struct   security_aps_status_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_aps_status_t {

} security_aps_status_t;
typedef SecurityArray<security_aps_status_t> security_aps_statuss_t;

/**
 * @Struct   security_disposition_t
 * @Brief
 * @DateTime 2018/8/10 14:07:00
 * @Modify   2018/8/10 14:07:05
 * @Author   Nanuns
 */
typedef struct _security_disposition_t {
    SecurityString id;
    SecurityString title;
    SecurityString category;
    SecurityString targetFeature;
    SecurityString targetImageUri;
    int priorityLevel;
    SecurityString applicantName;
    SecurityString applicantInfo;
    SecurityString applicantOrg;
    SecurityString beginTime;
    SecurityString endTime;
    SecurityString createTime;
    int            operateType;
    int            status;
    int            range;
    SecurityString tollgateList;
    SecurityString area;
    SecurityString receiveAddr;
    SecurityString receiveMobile;
    SecurityString reson;
    SecurityString removeOrg;
    SecurityString removePerson;
    SecurityString removeTime;
    SecurityString removeReason;
    security_subimage_infos_t subimageList;
} security_disposition_t;
typedef SecurityArray<security_disposition_t>               security_dispositions_t;

/**
 * @Struct   security_disposition_notification_t
 * @Brief
 * @DateTime 2018/8/10 13:54:47
 * @Modify   2018/8/10 13:54:50
 * @Author   Nanuns
 */
typedef struct _security_disposition_notification_t {
    SecurityString NotificationID;
    SecurityString dispositionId;
    SecurityString title;
    SecurityString triggerTime;
    SecurityString cntObjectId;
    security_person_t person;
    security_motorvehicle_t motorVehicle; // MotorVehicleObject
} security_disposition_notification_t;
typedef SecurityArray<security_disposition_notification_t>  security_disposition_notifications_t;

/**
 * @Struct   security_subscribe_t
 * @Brief
 * @DateTime 2018/8/10 11:20:41
 * @Modify   2018/8/10 11:20:52
 * @Author   Nanuns
 */
typedef struct _security_subscribe_t {
    SecurityString SubscribeID;
    SecurityString title;
    SecurityString detail;
    SecurityString resourceUri;
    SecurityString applicantName;
    SecurityString applicantOrg;
    SecurityString beginTime;
    SecurityString endTime;
    SecurityString receiveAddr;
    int            reportInterval;
    SecurityString reason;
    int            operateType;
    int            status;
    SecurityString cancelOrg;
    SecurityString cancelPerson;
    SecurityString cancelTime;
    SecurityString cancelReson;
} security_subscribe_t;
typedef SecurityArray<security_subscribe_t>                 security_subscribes_t;

/**
 * @Struct   security_subscribe_notification_t
 * @Brief
 * @DateTime 2018/8/10 14:31:34
 * @Modify   2018/8/10 14:31:38
 * @Author   Nanuns
 */
typedef struct _security_subscribe_notification_t {
    SecurityString              id;
    SecurityString              subscribeId;
    SecurityString              title;
    SecurityString              triggerTime;
    SecurityString              infoIds;
    security_cases_t            caseList;
    security_tollgates_t        tollgateList;
    security_lanes_t            laneList;
    security_devices_t          apeList;
    security_device_statuss_t   deviceStautusList;
    security_apss_t             apsList;
    security_aps_statuss_t      apsStatusList;
    security_persons_t          personList;
    security_faces_t            faceList;
    security_motorvehicles_t    motorVehicleList;
    security_nonmotorvehicles_t nonMotorVehicleList;
    security_things_t           thingList;
    security_scenes_t           sceneList;
} security_subscribe_notification_t;
typedef SecurityArray<security_subscribe_notification_t>    security_subscribe_notifications_t;

typedef struct _security_viidserver_t {
    SecurityString id;
    SecurityString serverName;
    SecurityString ipAddr;
    SecurityString ipv6Addr;
    int            port;
    SecurityString upServerId;
    SecurityString subServerId;
    bool           isOnline;
    SecurityString lastOnlineTime;
} security_viidserver_t;
typedef SecurityArray<security_viidserver_t> security_viidservers_t;

typedef struct _security_task_t {

} security_task_t;
typedef SecurityArray<security_task_t>                      security_tasks_t;

typedef struct _security_task_control_t {

} security_task_control_t;
typedef SecurityArray<security_task_control_t>              security_task_controls_t;

typedef struct _attr_condition {
    SecurityString key;
    SecurityString value;
} attr_condition_t;
typedef SecurityArray<attr_condition_t>                     attr_conditions_t;

/**
 * @Struct   security_system_time_t
 * @Brief    SystemTimeObject
 * @DateTime 2018-08-01T10:52:46+0800
 * @Modify   2018-08-01T10:52:46+0800
 * @Author   Nanuns
 */
typedef struct _security_system_time_t {
    SecurityString VIIDServerID;
    SecurityString TimeMode; // 1:���� 2:�ֶ�
    SecurityString LocalTime;
    SecurityString TimeZone;

    friend void from_json(const nlohmann::json &j, _security_system_time_t &c) {
        c.VIIDServerID = j.value("VIIDServerID", "");
        c.TimeMode = j.value("TimeMode", "");
        c.LocalTime = j.value("LocalTime", "");
        c.TimeZone = j.value("TimeZone", "");
    }

    friend void to_json(nlohmann::json &j, const _security_system_time_t &c) {
        j = nlohmann::json{
            {"VIIDServerID", c.VIIDServerID},
            {"TimeMode", c.TimeMode},
            {"LocalTime", c.LocalTime},
            {"TimeZone", c.TimeZone}
        };
    }
} security_system_time_t;

typedef struct _security_system_status_t {
    SecurityString viidServerId;
    bool           isOnline;
    SecurityString currentTime;
} security_system_status_t;

typedef struct _security_time_server_t {
    int            ntpServerId;
    SecurityString serverName;
    SecurityString ipAddr;
    SecurityString ipv6Addr;
    int            port;
} security_time_server_t;

// ResponseStatusObject
typedef struct _security_response_status_t {
    SecurityString RequestURL;
    int            StatusCode;
    SecurityString StatusString;
    SecurityString Id;
    SecurityString LocalTime;
} security_response_status_t;
// ResponseStatusList
typedef SecurityArray<security_response_status_t> security_response_statuss_t;
#endif
