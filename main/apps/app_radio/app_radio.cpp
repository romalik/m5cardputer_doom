/**
 * @file app_radio.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.6
 * @date 2023-09-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_radio.h"
#include "spdlog/spdlog.h"
#include "../../hal/bat/adc_read.h"


using namespace MOONCAKE::APPS;


#define _keyboard _data.hal->keyboard()
#define _canvas _data.hal->canvas()
#define _canvas_update _data.hal->canvas_update
#define _canvas_clear() _canvas->fillScreen(THEME_COLOR_BG)

// AppRadio::Data_t *AppRadio::_current_data = nullptr;
bool AppRadio::__running = false;

void AppRadio::_update_input()
{
    // spdlog::info("{} {}", _keyboard->keyList().size(), _data.last_key_num);

    // If changed
    if (_keyboard->keyList().size() != _data.last_key_num)
    {
        // If key pressed
        if (_keyboard->keyList().size() != 0)
        {
            // Update states and values
            _keyboard->updateKeysState();

            // If enter
            if (_keyboard->keysState().enter)
            {
                // New line
                _canvas->print(" \n");

                _update_state();
            }

            // If space
            else if (_keyboard->keysState().space)
            {
                _canvas->print(' ');
                _data.repl_input_buffer += ' ';
            }

            // If delete
            else if (_keyboard->keysState().del)
            {
                if (_data.repl_input_buffer.size())
                {
                    // Pop input buffer
                    _data.repl_input_buffer.pop_back();

                    // Pop canvas display
                    int cursor_x = _canvas->getCursorX();
                    int cursor_y = _canvas->getCursorY();

                    if (cursor_x - FONT_REPL_WIDTH < 0)
                    {
                        // Last line
                        cursor_y -= FONT_REPL_HEIGHT;
                        cursor_x = _canvas->width() - FONT_REPL_WIDTH;
                    }
                    else
                    {
                        cursor_x -= FONT_REPL_WIDTH;
                    }

                    spdlog::info("new cursor {} {}", cursor_x, cursor_y);

                    _canvas->setCursor(cursor_x, cursor_y);
                    _canvas->print("  ");
                    _canvas->setCursor(cursor_x, cursor_y);
                }
            }

            // Normal chars
            else
            {
                for (auto& i : _keyboard->keysState().values)
                {
                    // spdlog::info("{}", i);

                    _canvas->print(i);
                    _data.repl_input_buffer += i;
                }
            }

            _canvas_update();

            // Update last key num
            _data.last_key_num = _keyboard->keyList().size();
        }
        else
        {
            // Reset last key num
            _data.last_key_num = 0;
        }
    }
}


void AppRadio::_update_cursor()
{
    if ((millis() - _data.cursor_update_time_count) > _data.cursor_update_period)
    {
        // Get cursor
        int cursor_x = _canvas->getCursorX();
        int cursor_y = _canvas->getCursorY();

        // spdlog::info("cursor {} {}", cursor_x, cursor_y);

        _canvas->print(_data.cursor_state ? '_' : ' ');
        _canvas->setCursor(cursor_x, cursor_y);
        _canvas_update();

        _data.cursor_state = !_data.cursor_state;
        _data.cursor_update_time_count = millis();
    }
}


static char _wifi_ssid[50] = "IOTNetwork";
static char _wifi_password[50] = "fwintheshell";

void AppRadio::_update_state()
{
    if (_data.current_state == state_init)
    {
        _canvas->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
        _canvas->printf("WiFi SSID:\n");
        _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
        _canvas->printf(">>> ");
        _canvas_update();

        _data.current_state = state_wait_ssid;

        // wifi_connect_get_config(_wifi_ssid, _wifi_password);
        if (*_wifi_ssid) {
            // spdlog::info("std::string(_wifi_ssid) [{}]", std::string(_wifi_ssid));
            _data.repl_input_buffer = std::string(_wifi_ssid);
            _canvas->print(_wifi_ssid);
            _canvas_update();
        }

    }

    else if (_data.current_state == state_wait_ssid)
    {
        _canvas->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
        _canvas->printf("WiFi Password:\n");
        _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
        _canvas->printf(">>> ");
        _canvas_update();

        _data.wifi_ssid = _data.repl_input_buffer;

        // Reset buffer
        _data.repl_input_buffer = "";
        _data.current_state = state_wait_password;
        spdlog::info("wifi ssid set: {}", _data.wifi_ssid);

        if (*_wifi_password) {
            _data.repl_input_buffer = std::string(_wifi_password);
            _canvas->print(_wifi_password);
            _canvas_update();
        }
    }

    else if (_data.current_state == state_wait_password)
    {
        _data.wifi_password = _data.repl_input_buffer;
        // Reset buffer
        _data.repl_input_buffer = "";
        _data.current_state = state_setup;
        spdlog::info("wifi password set: {}", _data.wifi_password);

    }

    if (_data.current_state == state_setup) {
        _setup();
        _data.current_state = state_started;
    }

    if (_data.current_state == state_started) {
        while (1) {
            if (_data.hal->homeButton()->pressed()      // go home (keep radio playing)
                 || !__running) {                       // radio stopped (press ESC)

                _data.hal->playNextSound();
                spdlog::info("quit radio");
                destroyApp();
                break;
            }
            _loop();
            if (millis() - _data._last_update_battery_time > 5000) {
                float bat_v = static_cast<float>(adc_read_get_value()) * 2 / 1000;
                _data.hal->display()->setTextColor(TFT_GREEN, TFT_BLACK);
                _data.hal->display()->drawRightString((std::to_string(bat_v).substr(0, 5) + "v").c_str(), _data.hal->display()->width(), 0, FONT_SMALL);
                _data.hal->display()->setTextColor(TFT_WHITE);
                _data._last_update_battery_time = millis();
            }
        }
    }
}


void AppRadio::onCreate()
{
    spdlog::info("{} onCreate", getAppName());
    // AppRadio::_current_data = &_data;

    // Get hal
    _data.hal = mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();
    // _data.out = new AudioOutputM5Speaker(_data.hal->Speaker(), m5spk_virtual_channel);
}


void AppRadio::onResume()
{
    ANIM_APP_OPEN();

    _canvas_clear();
    _canvas->setTextScroll(true);
    _canvas->setBaseColor(THEME_COLOR_BG);
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _canvas->setFont(FONT_REPL);
    _canvas->setTextSize(FONT_SIZE_REPL);
    _canvas->setCursor(0, 0);

    _data._wifi_already_connected = (WiFi.status() == WL_CONNECTED);
    if (!__running)  {
        if (!_data._wifi_already_connected) {
            _data.current_state = state_init;
        } else {
            _data.current_state = state_setup;
        }
    } else {
        _data.hal->display()->clear();
        gfxSetup(_data.hal->display());
        meta_mod_bits = 3;
        _data.current_state = state_started;
    }

    _update_state();
}


void AppRadio::onRunning()
{
    if (_data.current_state != state_started)
    {
        _update_input();
        _update_cursor();
    }

    if (_data.hal->homeButton()->pressed())
    {
        _data.hal->playNextSound();
        spdlog::info("quit set wifi");
        destroyApp();
    }
}


void AppRadio::onDestroy()
{
    _canvas->setTextScroll(false);
}
