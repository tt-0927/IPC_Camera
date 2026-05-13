/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-06 19:49:00
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-02-26 16:51:16
 * @FilePath: /1126/share/ai_share/AiModules/Modules/VehicleAttribute/V2_0/VehicleAttributeV2_0.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#pragma once

#include <unordered_map>
// #include "VehicleAttribute.hpp"
#include "Attribute.hpp"
#include "VehicleAttributeExt.hpp"

#define Debug_VehicleAttributeV2_0

namespace VehicleAttribute_NS {
class CVehicleAttributeV2_0 {
  public:
    CVehicleAttributeV2_0(InParam_S stInParam);
    ~CVehicleAttributeV2_0();

    /**
     * @brief 初始化
     * @return true
     * @return false
     */
    bool init();

    /**
     * @brief 反初始化
     * @return true
     * @return false
     */
    bool unInit();

    /**
     * @brief 处理数据
     * @param stInData 传入的视频数据
     * @param nResult 分析的参数
     * @return true
     * @return false
     */
    bool process(InData_S stInData, std::vector<Result_S> &nResult);
    bool resizeAndPadImage(cv::Mat inputImage, cv::Mat &outputImage);

    enum class VehicleType : int
    {
        HEAVY_TRUCK = 0,  // 重型货车
        SUV,              // SUV
        MPV_BUSINESS,     // 商务车/多用途车
        LARGE_BUS,        // 大型巴士
        LIGHT_PASSENGER,  // 轻型客车
        SMALL_MPV,        // 小型多用途车
        PICKUP,           // 皮卡
        SEDAN,            // 轿车
        SMALL_TRUCK       // 小型卡车
    };

    enum class VehicleColor : int
    {
        BLACK = 9,  // 黑色
        BLUE,       // 蓝色
        BROWN,      // 棕色
        CYAN,       // 青色
        DARK_GRAY,  // 深灰色
        GRAY,       // 灰色
        GREEN,      // 绿色
        RED,        // 红色
        WHITE,      // 白色
        YELLOW      // 黄色
    };

    enum class VehicleBrand : int
    {
        ACURA = 19,       // 阿库拉
        ANKAI,            // 安凯
        AUDI,             // 奥迪
        BAIC,             // 北汽
        BMW,              // 宝马
        BYD,              // 比亚迪
        BAOJUN,           // 宝骏
        BESTUNE,          // 奔腾
        BUICK,            // 别克
        CADILLAC,         // 凯迪拉克
        CHANGAN,          // 长安
        CHANGHE,          // 长河
        CHERY,            // Cheryl
        CHEVROLET,        // 雪佛兰
        CHRYSLER,         // 克莱斯勒
        CITROEN,          // 雪铁龙
        COWIN,            // Cowin
        DODGE,            // 道奇
        DS,               // DS
        DONGFENG,         // 东风
        FAW,              // 一汽
        FAW_JIEFANG,      // 一汽解放
        FIAT,             // 菲亚特
        FORD,             // 福特
        FORLAND_TIMES,    // Forland times
        FOTON,            // 福田
        GAC_TRUMPCHI,     // 广汽传祺
        GEELY,            // 吉利
        GREAT_WALL,       // 长城
        HIGER,            // 海格
        HAFEI,            // 哈飞
        HAIMA,            // 海马
        HAVAL,            // 哈弗
        HUATAI,           // 华泰
        HEIBAO,           // 黑豹汽车
        HENGTONG,         // 恒通
        HONDA,            // 本田
        HONGQI,           // 红旗
        HUANGHAI,         // 黄海
        INFINITI,         // 英菲尼迪
        ISUZU,            // 五十铃
        IVECO,            // 伊维柯
        JAC,              // 江淮
        JAGUAR,           // 捷豹
        JEEP,             // 吉普
        JMC,              // 江铃
        JINBEI,           // 金杯
        KINGLONG,         // 景龙
        KIA,              // 起亚
        KAMA,             // 凯马
        KARRY,            // 卡力
        LUXGEN,           // 纳智捷
        LAND_ROVER,       // 路虎
        LUFENG,           // 陆风
        LIEBAO,           // 猎豹
        LEXUS,            // 雷克萨斯
        LIFAN,            // 力帆
        LINCOLN,          // 林肯
        MG,               // 名爵
        MINI,             // 迷你
        MASERATI,         // 玛莎拉蒂
        MAZDA,            // 马自达
        BENZ,             // 梅赛德斯-奔驰
        SMALL_TRUCK,      // 小型卡车
        MITSUBISHI,       // 三菱
        HYUNDAI,          // 现代
        MUSTANG,          // 野马
        NISSAN,           // 日产
        OLD_SCOOTER,      // Old Scooter
        PEUGEOT,          // 标致
        PORSCHE,          // 保时捷
        RENAULT,          // 雷诺
        ROEWE,            // 荣威
        MAXUS,            // 上汽大通
        SINOTRUK,         // 中国重汽
        SUNWIN,           // SUNWIN
        SHACMAN_TONGJIA,  // 陕汽通佳
        SKODA,            // 斯柯达
        SMART,            // Smart
        SOUTHEAST,        // 东南汽车
        SSANGYONG,        // 双龙
        SUBARU,           // 斯巴鲁
        SUZUKI,           // 铃木
        TKNG,             // TKNG
        TESLA,            // 特斯拉
        TOYOTA,           // 丰田
        TRUCK,            // 卡车
        VENUCIA,          // Venucia
        VOLKSWAGEN,       // 大众
        VOLVO,            // 沃尔沃
        WULING,           // 五菱
        WUZHENG,          // 五征
        YUEJIN,           // 月津
        YUTONG,           // 宇通
        ZHONGHUA,         // 中华
        ZHONGTONG,        // 中通
        ZXAUTO,           // 中兴
        ZOTYE,            // 众泰
        BULK_TRUCK,       // 散货车
        PICKUP_TRUCK      // 皮卡
    };

  private:

// #ifdef Debug_VehicleAttributeV2_0
    void PrintVehicleAttribute(const Result_S &stResult);
// #endif

    /* 初始化参数 */
    InParam_S m_stInParam;

    Inference_NS::CAttribute *m_pVehicleAttribute = nullptr;

    /* 算法输入参数限制 */
    int m_nLimitWidth   = 192;
    int m_nLimitHeight  = 256;
    
    int m_nLimitChannel = 3;
};

}  // namespace VehicleAttribute_NS
