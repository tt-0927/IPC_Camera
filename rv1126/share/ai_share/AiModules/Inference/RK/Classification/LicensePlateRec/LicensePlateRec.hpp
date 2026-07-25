/*
 * @FilePath     : LicensePlateRec.hpp
 * @Author       : 廖尔涛 liaoet@kfb.cn
 * @Date         : 2024-09-29 16:29:52
 * @LastEditors  : 廖尔涛 liaoet@kfb.cn
 * @LastEditTime : 2024-09-29 16:29:52
 * @Description  : 车牌识别算法
 */
#pragma once

#include "InputDataEXT.hpp"
#include "OutputDataEXT.hpp"
#include "CVInferenceRK.hpp"
#include "iostream"

namespace Inference_NS
{
    class CLicensePlateRec : public CCVInferenceRK
    {
    public:

        CLicensePlateRec(std::string strConfigPath);
        ~CLicensePlateRec();

        /**
         * @brief 重写父类的解析json模型后处理数据，用于适配不同类型的模型
         * @return true 
         * @return false 
         */
        bool checkModelProConfig() override;

        /**
         * @brief 推理数据
         * @param [Inference_NS::InputData_S] stInputData: 传入的结构体
         * @param [std::vector<Inference_NS::ClsData_S>&] vClsDatas: 推理出来的数据
         * @return [*]
         * @note
         */
        bool inference(Inference_NS::InputData_S stInputData, std::vector<Inference_NS::ClsData_S>& vClsDatas);

        /**
         * @brief softMax计算
         * @param [std::vector<float>] vInputData: 输入向量
         * @param [std::vector<float>&] vOutData: 计算结果
         * @return [*]
         * @note
         */
        bool softMax(std::vector<float> vInputData, std::vector<float>& vOutData);

        /**
         * @brief 车牌号输出的字符索引过滤
         * @param [std::vector<int>] vInputData: 输入向量
         * @param [std::vector<Inference_NS::Cls_S>&] vOutPreds: 返回过滤后概率向量的索引
         * @return [*]
         * @note
         */
        bool decodePlate(std::vector<int> vInputPreds, std::vector<Inference_NS::Cls_S>& vOutPreds);
    };


}    // namespace Inference_NS
