/**
 * @file hal_cardputer.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "hal_cardputer.h"
#include "display/hal_display.hpp"
#include "bat/adc_read.h"

#include "common_define.h"


using namespace HAL;


void HalCardputer::_display_init()
{
    printf("init display\n");

    // Display
    _display = new LGFX_Cardputer;
    _display->init();
    _display->setRotation(1);


}


void HalCardputer::_keyboard_init()
{
    _keyboard = new KEYBOARD::Keyboard;
    _keyboard->init();
}



void HalCardputer::_button_init()
{
    _homeButton = new Button(0);
    _homeButton->begin();
}


void HalCardputer::_bat_init()
{
    adc_read_init();
}


void HalCardputer::init()
{
    printf("hal init\n");

    _display_init();

    _button_init();
    _bat_init();
}


// double getBatVolBfb(double batVcc) //获取电压的百分比，经过换算并非线性关系
// {
//     double bfb = 0.0;

//     //y = 497.50976 x4 - 7,442.07254 x3 + 41,515.70648 x2 - 102,249.34377 x + 93,770.99821
//     bfb = 497.50976 * batVcc * batVcc * batVcc * batVcc
//             - 7442.07254 * batVcc * batVcc * batVcc
//             + 41515.70648 * batVcc * batVcc
//             - 102249.34377 * batVcc
//             + 93770.99821;
//     if (bfb > 100)
//         bfb = 100.0;
//     else if (bfb < 0)
//         bfb = 3.0;

//     return bfb;
// }


float __cardputer_hal_bat_v = 0;
uint8_t HalCardputer::getBatLevel()
{
    // spdlog::info("get bat: {}", adc_read_get_value());
    // spdlog::info("bat level: {}", getBatVolBfb((double)(adc_read_get_value() * 2 / 1000)));

    // return 100;
    // return (uint8_t)getBatVolBfb(4.2);
    // return (uint8_t)getBatVolBfb((double)((adc_read_get_value() * 2 + 100) / 1000));


    // https://docs.m5stack.com/zh_CN/core/basic_v2.7
    __cardputer_hal_bat_v = static_cast<float>(adc_read_get_value()) * 2 / 1000;
    printf("batV: %f\n", __cardputer_hal_bat_v);
    uint8_t result = 0;
    if (__cardputer_hal_bat_v >= 3.90)
        result = 100;
    else if (__cardputer_hal_bat_v >= 3.80)
        result = 75;
    else if (__cardputer_hal_bat_v >= 3.65)
        result = 50;
    else if (__cardputer_hal_bat_v >= 3.45)
        result = 25;
    else
        result = 0;
    return result;
}




void HalCardputer::LcdBgLightTest(HalCardputer* hal)
{
    hal->display()->setTextSize(3);

    std::vector<int> color_list = {TFT_RED, TFT_GREEN, TFT_BLUE};
    for (auto i : color_list)
    {
        hal->display()->fillScreen(i);

        for (int i = 0; i < 256; i++)
        {
            hal->display()->setCursor(0, 0);
            hal->display()->printf("%d", i);
            hal->display()->setBrightness(i);
            delay(20);
        }
        delay(1000);

        for (int i = 255; i >= 0; i--)
        {
            hal->display()->setCursor(0, 0);
            hal->display()->printf("%d", i);
            hal->display()->setBrightness(i);
            delay(20);
        }
    }
}
