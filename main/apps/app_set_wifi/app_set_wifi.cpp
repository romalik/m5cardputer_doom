/**
 * @file app_set_wifi.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.6
 * @date 2023-09-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_set_wifi.h"
#include "spdlog/spdlog.h"

// #include "../utils/wifi_connect_wrap/wifi_connect_wrap.h"
// #include "../utils/sntp_wrap/sntp_wrap.h"
#include <WiFi.h>
#include "esp_sntp.h"

using namespace MOONCAKE::APPS;


#define _keyboard _data.hal->keyboard()
#define _canvas _data.hal->canvas()
#define _canvas_update _data.hal->canvas_update
#define _canvas_clear() _canvas->fillScreen(THEME_COLOR_BG)


void AppSetWiFi::_update_input() {
    // spdlog::info("{} {}", _keyboard->keyList().size(), _data.last_key_num);

    // If changed
    if (_keyboard->keyList().size() != _data.last_key_num) {
        // If key pressed
        if (_keyboard->keyList().size() != 0) {
            // Update states and values
            _keyboard->updateKeysState();

            // If enter
            if (_keyboard->keysState().enter) {
                // New line
                _canvas->print(" \n");

                _update_state();
            }

            // If space
            else if (_keyboard->keysState().space) {
                _canvas->print(' ');
                _data.repl_input_buffer += ' ';
            }

            // If delete
            else if (_keyboard->keysState().del) {
                if (_data.repl_input_buffer.size()) {
                    // Pop input buffer
                    _data.repl_input_buffer.pop_back();

                    // Pop canvas display
                    int cursor_x = _canvas->getCursorX();
                    int cursor_y = _canvas->getCursorY();

                    if (cursor_x - FONT_REPL_WIDTH < 0) {
                        // Last line
                        cursor_y -= FONT_REPL_HEIGHT;
                        cursor_x = _canvas->width() - FONT_REPL_WIDTH;
                    } else {
                        cursor_x -= FONT_REPL_WIDTH;
                    }

                    spdlog::info("new cursor {} {}", cursor_x, cursor_y);

                    _canvas->setCursor(cursor_x, cursor_y);
                    _canvas->print("  ");
                    _canvas->setCursor(cursor_x, cursor_y);
                }
            }

            // Normal chars
            else {
                for (auto& i : _keyboard->keysState().values) {
                    // spdlog::info("{}", i);

                    _canvas->print(i);
                    _data.repl_input_buffer += i;
                }
            }

            _canvas_update();

            // Update last key num
            _data.last_key_num = _keyboard->keyList().size();
        }
        else {
            // Reset last key num
            _data.last_key_num = 0;
        }
    }
}


void AppSetWiFi::_update_cursor() {
    if ((millis() - _data.cursor_update_time_count) > _data.cursor_update_period) {
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

void AppSetWiFi::_update_state() {
    if (_data.current_state == state_init) {
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

    } else if (_data.current_state == state_wait_ssid) {
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
    } else if (_data.current_state == state_wait_password) {
        _data.wifi_password = _data.repl_input_buffer;
        // Reset buffer
        _data.repl_input_buffer = "";
        _data.current_state = state_connect;
        spdlog::info("wifi password set: {}", _data.wifi_password);
        _canvas->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
        _canvas->printf("WiFi config:\n- %s\n- %s\nConnecting...\n", _data.wifi_ssid.c_str(), _data.wifi_password.c_str());
        _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
        _canvas_update();

    }



    if (_data.current_state == state_connect) {


        // wifi_connect_wrap_config(_data.wifi_ssid.c_str(), _data.wifi_password.c_str());
        // wifi_connect_wrap_connect();
        if (!_data._alreay_connected) {
            WiFi.begin(_data.wifi_ssid.c_str(), _data.wifi_password.c_str());
            WiFi.waitForConnectResult(20 * 1000);
        }

        // if (wifi_connect_wrap_is_wifi_connect_success() != 0)
        if (WiFi.status() == WL_CONNECTED) {
            _canvas->setTextColor(TFT_GREENYELLOW, THEME_COLOR_BG);
            // _canvas->printf("Connected\nSNTP adjusting...\n");
            _canvas->printf("Connected\n");
            _canvas->println("IP: " + WiFi.localIP().toString());

            // sntp_warp_init();

            if (!esp_sntp_enabled()) {
                setenv("TZ", "CST-8", 1);
                tzset();
                esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
                esp_sntp_setservername(0, "pool.ntp.org");
                esp_sntp_init();
                _canvas->printf("SNTP started\n");
            }
            _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
            _canvas_update();

            _data.hal->setSntpAdjusted(true);

            _canvas->setTextColor(TFT_GREENYELLOW, THEME_COLOR_BG);
            _canvas->printf("Done\n");
            _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
            _canvas->printf(">>> ");
            _canvas_update();
        } else {
            _canvas->setTextColor(TFT_RED, THEME_COLOR_BG);
            _canvas->printf("Failed\n");
            _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
            _canvas->printf(">>> ");
            _canvas_update();
        }

        // wifi_connect_wrap_disconnect();
        // if (!_data._alreay_connected) {
        //     WiFi.disconnect(true);
        // }

        _data.current_state = state_wait_quit;

    }

    if (_data.current_state == state_already_connected) {
        _canvas->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
        _canvas->printf("WiFi already set\nTurn off WiFi? [y/n]\n");
        _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
        _canvas->printf(">>> ");
        _data.current_state = state_whether_disable_wifi;

    } else if (_data.current_state == state_whether_disable_wifi) {
        if (_data.repl_input_buffer == "y") {
            WiFi.disconnect(true);
            _canvas->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
            _canvas->printf("WiFi off\n");
        } else {
            _canvas->printf("cancel\n");
        }
        _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
        _canvas->printf(">>> ");
        _data.current_state = state_wait_quit;
    }
}


void AppSetWiFi::onCreate() {
    spdlog::info("{} onCreate", getAppName());

    // Get hal
    _data.hal = mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();
}


void AppSetWiFi::onResume() {
    ANIM_APP_OPEN();

    _canvas_clear();
    _canvas->setTextScroll(true);
    _canvas->setBaseColor(THEME_COLOR_BG);
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _canvas->setFont(FONT_REPL);
    _canvas->setTextSize(FONT_SIZE_REPL);
    _canvas->setCursor(0, 0);

    _data._alreay_connected = (WiFi.status() == WL_CONNECTED);
    if (_data._alreay_connected) {
        if (!esp_sntp_enabled())
            _data.current_state = state_connect;
        else
            _data.current_state = state_already_connected;
    } else {
        _data.current_state = state_init;
    }
    _update_state();
}


void AppSetWiFi::onRunning() {
    if (_data.current_state != state_wait_quit)
        _update_input();
    _update_cursor();

    if (_data.hal->homeButton()->pressed()) {
        _data.hal->playNextSound();
        spdlog::info("quit set wifi");
        destroyApp();
    }
}


void AppSetWiFi::onDestroy() {
    _canvas->setTextScroll(false);
}
