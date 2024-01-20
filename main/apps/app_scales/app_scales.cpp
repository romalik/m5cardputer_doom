/**
 * @file app_scales.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.6
 * @date 2023-09-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_scales.h"
#include "lgfx/v1/misc/enum.hpp"
#include "spdlog/spdlog.h"
#include <cstdint>
#include "../utils/theme/theme_define.h"
#include "SCALES/UNIT_SCALES.h"



using namespace MOONCAKE::APPS;


#define _keyboard _data.hal->keyboard()
#define _canvas _data.hal->canvas()
#define _canvas_update _data.hal->canvas_update
#define _canvas_clear() _canvas->fillScreen(THEME_COLOR_BG)


void AppScales::onCreate()
{
    spdlog::info("{} onCreate", getAppName());

    // Get hal
    _data.hal = mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();
}


void AppScales::onResume()
{
    ANIM_APP_OPEN();

    _data.current_state = state_init;
    memset(_data.filter_window, 0, sizeof(_data.filter_window));
}


void AppScales::onRunning()
{
    if (_data.current_state == state_init) {
        _canvas_clear();
        _canvas->setBaseColor(THEME_COLOR_BG);
        _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
        _canvas->setFont(FONT_REPL);
        _canvas->setTextSize(FONT_SIZE_REPL);
        _canvas->setCursor(0, 0);
        _canvas->printf("not connected");
        _canvas_update();

        if (_data._scales.begin(&Wire, 2, 1, DEVICE_DEFAULT_ADDR)) {

            // use our own filter
            _data._scales.setLPFilter(1);
            _data._scales.setAvgFilter(0);
            _data._scales.setEmaFilter(0);
            _data.current_state = state_connected;
            _data.hal->playNextSound();
        }

    } else if (_data.current_state == state_connected) {
        int adc      = _data._scales.getRawADC();
        if (!adc) {
            // disconnected
            _data.current_state = state_init;
            _data.hal->playNextSound();
        } else {
            float raw_weight = _data._scales.getWeight();
            float gap    = _data._scales.getGapValue();
            uint8_t version  = _data._scales.getFirmwareVersion();

            // update filter
            _data.filter_window[(_data._filter_window_i++) & (FILTER_WINDOW_SIZE-1)] = raw_weight;

            float weight = 0;
            for (auto &v : _data.filter_window) weight += v;
            weight /= FILTER_WINDOW_SIZE;
            float val = weight - _data.offset;

            if (millis() - _data._last_update > 200) {
                _canvas_clear();
                _canvas->setBaseColor(THEME_COLOR_BG);
                _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
                _canvas->setFont(FONT_REPL);
                _canvas->setTextSize(FONT_SIZE_REPL);
                _canvas->setCursor(0, 0);
                _canvas->printf("connected          fw: %d\n\n", version);
                _canvas->setTextSize(2);
                _canvas->printf("%.3f g\n", val);
                // if (val < 0)
                //     _canvas->printf("+%.3f g\n", val);
                // else
                //     _canvas->printf(" %.3f g\n", val);
                _canvas->setTextSize(1);
                _canvas->printf("GAP:     %.3f\n", gap);
                _canvas->printf("RawADC:  %d\n", adc);
                _canvas_update();

                _data._last_update = millis();
            }

            // offset
            if (!_data._scales.getBtnStatus() || _data.hal->keyboard()->getKey().x >= 0) {
                // _data._scales.setOffset();
                _data.offset = weight;
            }

            // rgb led
            if (val >= 4000) _data._scales.setLEDColor(0xff0000);
            else if (val >= 5) _data._scales.setLEDColor(0x222222);
            else _data._scales.setLEDColor(0x000000);
        }
    }

    if (_data.hal->homeButton()->pressed())
    {
        _data.hal->playNextSound();
        spdlog::info("quit scales");
        destroyApp();
    }
}


void AppScales::onDestroy()
{

}
