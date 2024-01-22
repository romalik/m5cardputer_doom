/**
 * @file system_bar.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-18
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "../../launcher.h"
#include "spdlog/spdlog.h"
#include "../menu/menu_render_callback.hpp"
#include "../../../utils/common_define.h"
#include "hal/hal_cardputer.h"

#include "assets/bat1.h"
#include "assets/bat2.h"
#include "assets/bat3.h"
#include "assets/bat4.h"
#include "assets/bat5.h"
#include "assets/wifi1.h"
#include "assets/wifi2.h"
#include "assets/wifi3.h"
#include "assets/wifi4.h"
#include "assets/wifi5.h"

using namespace MOONCAKE::APPS;

void Launcher::_start_system_bar()
{
    // _data.hal->canvas_system_bar()->fillScreen(TFT_BLUE);
}

void Launcher::_update_system_bar()
{
    if ((millis() - _data.system_bar_update_count) > _data.system_bar_update_preiod)
    {
        // Update state
        _port_update_system_state();


        // Backgound
        int margin_x = 4;
        int margin_y = 4;

        _data.hal->canvas_system_bar()->fillScreen(THEME_COLOR_BG);
        // _data.hal->canvas_system_bar()->fillSmoothRoundRect(
        //     margin_x,
        //     margin_y,
        //     _data.hal->canvas_system_bar()->width() - margin_x * 2,
        //     _data.hal->canvas_system_bar()->height() - margin_y * 2,
        //     (_data.hal->canvas_system_bar()->height() - margin_y * 2) / 2,
        //     THEME_COLOR_SYSTEM_BAR
        // );
        _data.hal->canvas_system_bar()->fillSmoothRoundRect(
            0,  // margin_x,
            margin_y,
            _data.hal->canvas_system_bar()->width() - margin_x,   // * 2,
            _data.hal->canvas_system_bar()->height() - margin_y * 2,
            5,
            THEME_COLOR_SYSTEM_BAR
        );

        _data.hal->canvas_system_bar()->setFont(FONT_BASIC);


        // Time
        _data.hal->canvas_system_bar()->setTextColor(THEME_COLOR_SYSTEM_BAR_TEXT);
        _data.hal->canvas_system_bar()->drawCenterString(
            _data.system_state.time.c_str(),
            // 56,
            50,
            _data.hal->canvas_system_bar()->height() / 2 - FONT_HEIGHT / 2 - 1
        );


        // Wifi stuff
        // int x = 15;
        int x = 9;
        int y = 5;

        if (_data.system_state.wifi_state == 1)
            _data.hal->canvas_system_bar()->pushImage(x, y, 16, 16, image_data_wifi1);
        else if (_data.system_state.wifi_state == 2)
            _data.hal->canvas_system_bar()->pushImage(x, y, 16, 16, image_data_wifi2);
        else if (_data.system_state.wifi_state == 3)
            _data.hal->canvas_system_bar()->pushImage(x, y, 16, 16, image_data_wifi3);
        else if (_data.system_state.wifi_state == 4)
            _data.hal->canvas_system_bar()->pushImage(x, y, 16, 16, image_data_wifi4);
        else if (_data.system_state.wifi_state == 5)
            _data.hal->canvas_system_bar()->pushImage(x, y, 16, 16, image_data_wifi5);



        // _data.hal->canvas_system_bar()->setTextColor(THEME_COLOR_SYSTEM_BAR_TEXT);
        _data.hal->canvas_system_bar()->setTextColor(TFT_DARKGRAY);
        // free mem
        _data.hal->canvas_system_bar()->drawRightString(
            std::to_string(esp_get_free_heap_size() / 1000).append("k").c_str(),
            _data.hal->canvas_system_bar()->width() - 50,
            _data.hal->canvas_system_bar()->height() / 2 - 3 + 4,
            FONT_SMALL
        );

        // Bat stuff
        _data.hal->canvas_system_bar()->drawRightString(
            std::to_string(__cardputer_hal_bat_v).substr(0, 5).append("V").c_str(),
            _data.hal->canvas_system_bar()->width() - 50,
            _data.hal->canvas_system_bar()->height() / 2 - 3 - 4,
            FONT_SMALL
        );

        x = _data.hal->canvas_system_bar()->width() - 45;
        y = 5;

        if (_data.system_state.bat_state == 1)
            _data.hal->canvas_system_bar()->pushImage(x, y, 32, 16, image_data_bat1);
        else if (_data.system_state.bat_state == 2)
            _data.hal->canvas_system_bar()->pushImage(x, y, 32, 16, image_data_bat2);
        else if (_data.system_state.bat_state == 3)
            _data.hal->canvas_system_bar()->pushImage(x, y, 32, 16, image_data_bat3);
        else if (_data.system_state.bat_state == 4)
            _data.hal->canvas_system_bar()->pushImage(x, y, 32, 16, image_data_bat4);
        else if (_data.system_state.bat_state == 5)
            _data.hal->canvas_system_bar()->pushImage(x, y, 32, 16, image_data_bat5);



        // Push
        _data.hal->canvas_system_bar_update();

        // Reset flag
        _data.system_bar_update_count = millis();
    }
}
