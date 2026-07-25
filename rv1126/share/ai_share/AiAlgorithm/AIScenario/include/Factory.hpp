/*
*  File Name:Factory.hpp
*  Created on: 2024年5月17日
*  Author: liaoet
*  description :AI场景调用 	
*  Modify date: 
*/

#pragma once
#include "AIScenario.hpp"
#include "HumanCutout.hpp"

template<typename Product>
class Factory {
public:
    static Product* create() {
        return new Product();
    }
};