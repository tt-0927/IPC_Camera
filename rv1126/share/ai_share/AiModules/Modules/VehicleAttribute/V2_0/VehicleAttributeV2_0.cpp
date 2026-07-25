/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-06 19:49:00
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-07 14:40:16
 * @FilePath: /1126/share/ai_share/AiModules/Modules/VehicleAttribute/V2_0/VehicleAttributeV2_0.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#pragma once

#include "VehicleAttributeV2_0.hpp"
#include "SaveImage.hpp"

#ifdef Debug_VehicleAttributeV2_0

const char *VehicleTypeToString(
    VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType type)
{
    using VT = VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType;

    switch (type)
    {
    case VT::HEAVY_TRUCK:
        return "重型货车";
    case VT::SUV:
        return "SUV";
    case VT::MPV_BUSINESS:
        return "商务车/多用途车";
    case VT::LARGE_BUS:
        return "大型巴士";
    case VT::LIGHT_PASSENGER:
        return "轻型客车";
    case VT::SMALL_MPV:
        return "小型多用途车";
    case VT::PICKUP:
        return "皮卡";
    case VT::SEDAN:
        return "轿车";
    case VT::SMALL_TRUCK:
        return "小型卡车";
    default:
        return "未知车辆类型";
    }
}

const char *VehicleColorToString(
    VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor color)
{
    using VC = VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor;

    switch (color)
    {
    case VC::BLACK:
        return "黑色";
    case VC::BLUE:
        return "蓝色";
    case VC::BROWN:
        return "棕色";
    case VC::CYAN:
        return "青色";
    case VC::DARK_GRAY:
        return "深灰色";
    case VC::GRAY:
        return "灰色";
    case VC::GREEN:
        return "绿色";
    case VC::RED:
        return "红色";
    case VC::WHITE:
        return "白色";
    case VC::YELLOW:
        return "黄色";
    default:
        return "未知颜色";
    }
}

const char *VehicleBrandToString(
    VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleBrand brand)
{
    using VB = VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleBrand;

    switch (brand)
    {
    case VB::ACURA:
        return "阿库拉";
    case VB::ANKAI:
        return "安凯";
    case VB::AUDI:
        return "奥迪";
    case VB::BAIC:
        return "北汽";
    case VB::BMW:
        return "宝马";
    case VB::BYD:
        return "比亚迪";
    case VB::BAOJUN:
        return "宝骏";
    case VB::BESTUNE:
        return "奔腾";
    case VB::BUICK:
        return "别克";
    case VB::CADILLAC:
        return "凯迪拉克";
    case VB::CHANGAN:
        return "长安";
    case VB::CHANGHE:
        return "长河";
    case VB::CHERY:
        return "奇瑞";
    case VB::CHEVROLET:
        return "雪佛兰";
    case VB::CHRYSLER:
        return "克莱斯勒";
    case VB::CITROEN:
        return "雪铁龙";
    case VB::COWIN:
        return "凯翼";
    case VB::DODGE:
        return "道奇";
    case VB::DS:
        return "DS";
    case VB::DONGFENG:
        return "东风";
    case VB::FAW:
        return "一汽";
    case VB::FAW_JIEFANG:
        return "一汽解放";
    case VB::FIAT:
        return "菲亚特";
    case VB::FORD:
        return "福特";
    case VB::FORLAND_TIMES:
        return "时代汽车";
    case VB::FOTON:
        return "福田";
    case VB::GAC_TRUMPCHI:
        return "广汽传祺";
    case VB::GEELY:
        return "吉利";
    case VB::GREAT_WALL:
        return "长城";
    case VB::HIGER:
        return "海格";
    case VB::HAFEI:
        return "哈飞";
    case VB::HAIMA:
        return "海马";
    case VB::HAVAL:
        return "哈弗";
    case VB::HUATAI:
        return "华泰";
    case VB::HEIBAO:
        return "黑豹汽车";
    case VB::HENGTONG:
        return "恒通";
    case VB::HONDA:
        return "本田";
    case VB::HONGQI:
        return "红旗";
    case VB::HUANGHAI:
        return "黄海";
    case VB::INFINITI:
        return "英菲尼迪";
    case VB::ISUZU:
        return "五十铃";
    case VB::IVECO:
        return "依维柯";
    case VB::JAC:
        return "江淮";
    case VB::JAGUAR:
        return "捷豹";
    case VB::JEEP:
        return "Jeep";
    case VB::JMC:
        return "江铃";
    case VB::JINBEI:
        return "金杯";
    case VB::KINGLONG:
        return "金龙";
    case VB::KIA:
        return "起亚";
    case VB::KAMA:
        return "凯马";
    case VB::KARRY:
        return "开瑞";
    case VB::LUXGEN:
        return "纳智捷";
    case VB::LAND_ROVER:
        return "路虎";
    case VB::LUFENG:
        return "陆风";
    case VB::LIEBAO:
        return "猎豹";
    case VB::LEXUS:
        return "雷克萨斯";
    case VB::LIFAN:
        return "力帆";
    case VB::LINCOLN:
        return "林肯";
    case VB::MG:
        return "名爵";
    case VB::MINI:
        return "MINI";
    case VB::MASERATI:
        return "玛莎拉蒂";
    case VB::MAZDA:
        return "马自达";
    case VB::BENZ:
        return "梅赛德斯-奔驰";
    case VB::SMALL_TRUCK:
        return "小型卡车";
    case VB::MITSUBISHI:
        return "三菱";
    case VB::HYUNDAI:
        return "现代";
    case VB::MUSTANG:
        return "野马";
    case VB::NISSAN:
        return "日产";
    case VB::OLD_SCOOTER:
        return "老年代步车";
    case VB::PEUGEOT:
        return "标致";
    case VB::PORSCHE:
        return "保时捷";
    case VB::RENAULT:
        return "雷诺";
    case VB::ROEWE:
        return "荣威";
    case VB::MAXUS:
        return "上汽大通";
    case VB::SINOTRUK:
        return "中国重汽";
    case VB::SUNWIN:
        return "申沃";
    case VB::SHACMAN_TONGJIA:
        return "陕汽通佳";
    case VB::SKODA:
        return "斯柯达";
    case VB::SMART:
        return "Smart";
    case VB::SOUTHEAST:
        return "东南";
    case VB::SSANGYONG:
        return "双龙";
    case VB::SUBARU:
        return "斯巴鲁";
    case VB::SUZUKI:
        return "铃木";
    case VB::TKNG:
        return "唐骏";
    case VB::TESLA:
        return "特斯拉";
    case VB::TOYOTA:
        return "丰田";
    case VB::TRUCK:
        return "卡车";
    case VB::VENUCIA:
        return "启辰";
    case VB::VOLKSWAGEN:
        return "大众";
    case VB::VOLVO:
        return "沃尔沃";
    case VB::WULING:
        return "五菱";
    case VB::WUZHENG:
        return "五征";
    case VB::YUEJIN:
        return "跃进";
    case VB::YUTONG:
        return "宇通";
    case VB::ZHONGHUA:
        return "中华";
    case VB::ZHONGTONG:
        return "中通";
    case VB::ZXAUTO:
        return "中兴";
    case VB::ZOTYE:
        return "众泰";
    case VB::BULK_TRUCK:
        return "散货车";
    case VB::PICKUP_TRUCK:
        return "皮卡";

    default:
        return "未知品牌";
    }
}

void VehicleAttribute_NS::CVehicleAttributeV2_0::PrintVehicleAttribute(const Result_S &stResult)
{
    auto type = static_cast<VehicleType>(stResult.nVehicleType);
    printf("车辆类型: %d - %s\n", stResult.nVehicleType, VehicleTypeToString(type));

    auto color = static_cast<VehicleColor>(stResult.nVehicleColor);
    printf("车辆颜色: %d - %s\n", stResult.nVehicleColor, VehicleColorToString(color));

    auto brand = static_cast<VehicleBrand>(stResult.nVehicleBrand);
    printf("车辆品牌: %d - %s\n", stResult.nVehicleBrand, VehicleBrandToString(brand)); 
}
#endif

VehicleAttribute_NS::CVehicleAttributeV2_0::CVehicleAttributeV2_0(InParam_S stInParam)
    : m_stInParam(stInParam)
{
}

VehicleAttribute_NS::CVehicleAttributeV2_0::~CVehicleAttributeV2_0()
{
    unInit();
}

/* 初始化 */
bool VehicleAttribute_NS::CVehicleAttributeV2_0::init()
{
    bool bRet = false;

    m_pVehicleAttribute = new Inference_NS::CAttribute(m_stInParam.strModelPath);
    if (m_pVehicleAttribute)
    {
        if (m_pVehicleAttribute->init())
        {
            bRet = m_pVehicleAttribute->getSizeLimit(
                0,
                m_nLimitWidth,
                m_nLimitHeight,
                m_nLimitChannel);
        }
    }

    if (!bRet)
    {
        printf("模型初始化失败 [%s]\n",
               m_stInParam.strModelPath.c_str());
        goto FAIL;
    }
    return bRet;

FAIL:

    unInit();

    return false;
}

/* 反初始化 */
bool VehicleAttribute_NS::CVehicleAttributeV2_0::unInit()
{
    if (m_pVehicleAttribute)
    {
        delete m_pVehicleAttribute;
        m_pVehicleAttribute = nullptr;
    }
    return true;
}

/* 处理数据 */
bool VehicleAttribute_NS::CVehicleAttributeV2_0::process(
    InData_S               stInData,
    std::vector<Result_S> &vstResult)
{

    if (stInData.inMat.empty())
    {
        printf("传入图片为空\n");
        return false;
    }

    if (!m_pVehicleAttribute)
    {
        printf("未初始化算法类\n");
        return false;
    }

    bool bRet = true;

    if (m_stInParam.bDebug)
    {
        /* 保存图片 */
        if (!stInData.inMat.empty() && !m_stInParam.strOriginalDataPath.empty())
        {
            if (!Modules_NS::saveImage(stInData.inMat, m_stInParam.strOriginalDataPath))
            {
                printf("Debug-保存图片失败[%s]\n", m_stInParam.strOriginalDataPath.c_str());
            }
        }
    }

    /* 前处理 */
    if (stInData.inMat.channels() != m_nLimitChannel)
    {
        printf("模型需要的通道数和输入图片的通道数不一致 inMat[%d] != m_nLimitChannel[%d]\n",
               stInData.inMat.channels(),
               m_nLimitChannel);
        return false;
    }

    if (stInData.inMat.cols != m_nLimitWidth || stInData.inMat.rows != m_nLimitHeight)
    {
        // cv::resize(stInData.inMat, stInData.inMat, cv::Size(m_nLimitWidth, m_nLimitHeight));
        resizeAndPadImage(stInData.inMat, stInData.inMat);
    }

    /* 推理+后处理 */
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    Inference_NS::InputData_S            stInputData;
    stInputData.pData     = (float *)stInData.inMat.data;

    stInputData.pData = (float *)stInData.inMat.data;
    stInputData.nDataSize = static_cast<size_t>(stInData.inMat.total() * stInData.inMat.elemSize() * sizeof(float));

    bRet = m_pVehicleAttribute->inference(stInputData, vClsDatas);

    if (!bRet)
    {
        printf("算法分析失败\n");
        return false;
    }

    for (int i = 0; i < vClsDatas.size(); i++)
    {
        const std::vector<Inference_NS::Cls_S> &vCls = vClsDatas[i].vCls;
        Result_S                                stResult;

        for (int j = 0; j < vCls.size(); j++)
        {
            const Inference_NS::Cls_S &cls = vCls[j];
            if (cls.nLabel >= (int)VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::HEAVY_TRUCK && cls.nLabel <= (int)VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleType::SMALL_TRUCK)
            {
                stResult.nVehicleType = cls.nLabel;
            }
            else if (cls.nLabel >= (int)VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::BLACK && cls.nLabel <= (int)VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleColor::YELLOW)
            {
                stResult.nVehicleColor = cls.nLabel;
            }
            else if (cls.nLabel >= (int)VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleBrand::ACURA && cls.nLabel <= (int)VehicleAttribute_NS::CVehicleAttributeV2_0::VehicleBrand::PICKUP_TRUCK)
            {
                stResult.nVehicleBrand = cls.nLabel;
            }
        }
        
#ifdef Debug_VehicleAttributeV2_0
        PrintVehicleAttribute(stResult);
#endif
        // stRes.strName = vOutDatas[i].strName;
        // stRes.fConfidence = vOutDatas[i].fConfidence;
        vstResult.push_back(stResult);
    }

    return true;
}

bool VehicleAttribute_NS::CVehicleAttributeV2_0::resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage)
{
    int   imageWidth     = inputImage.cols;
    int   imageHeight    = inputImage.rows;
    float m_fResizeScale = std::min(static_cast<float>(m_nLimitWidth) / imageWidth, static_cast<float>(m_nLimitHeight) / imageHeight);

    int newWidth  = static_cast<int>(imageWidth * m_fResizeScale);
    int newHeight = static_cast<int>(imageHeight * m_fResizeScale);

    cv::Mat resizedImage;
    cv::resize(inputImage, resizedImage, cv::Size(newWidth, newHeight));

    int Xoffset = m_nLimitWidth - newWidth;
    int Yoffset = m_nLimitHeight - newHeight;
    int left    = static_cast<int>(Xoffset / 2);
    int top     = static_cast<int>(Yoffset / 2);
    int right   = Xoffset - left;
    int bottom  = Yoffset - top;

    cv::copyMakeBorder(resizedImage, resizedImage, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(127, 127, 127));
    outputImage = resizedImage;
    return true;
}