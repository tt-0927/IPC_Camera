/**
 * @FilePath     : config_strategy.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-09-30 10:16:19
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-30 10:40:51
 * @Description  : 配置策略接口 (模板)
 */

#pragma once

/**
* @brief   : 配置策略接口 (模板)
* @note    : 定义了所有具体配置策略必须实现的行为，
*          包括填充默认值和根据业务进行定制。
* @tparam  {T} : 配置结构体的类型 (例如：VideoConfig_S, AudioConfig_S)
*/
template <typename T>
class CConfigStrategy
{
public:
    /**
    * @brief   : 虚析构函数
    */
    virtual ~CConfigStrategy() = default;

    /**
    * @brief   : 填充默认配置
    * @param   {T} &config : 需要被填充的配置结构体引用
    * @return  {void}
    */
    virtual void fillDefault(T &config) = 0;

    /**
    * @brief   : 定制化配置
    * @note    : 在填充默认值之后调用，用于根据特定业务需求调整配置
    * @param   {T} &config : 需要被定制的配置结构体引用
    * @return  {void}
    */
    virtual void customize(T &config) = 0;
};
