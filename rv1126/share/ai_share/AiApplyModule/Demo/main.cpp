/*
 * @FilePath     : main.cpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-21 19:16:25
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-07-08 19:13:43
 * @Description  :
 */
#include <iostream>

#include "AiManage.hpp"

int main(int argc, char const* argv[])
{
    AiManage_NS::CAiManage::get_instance();
    return 0;
}
