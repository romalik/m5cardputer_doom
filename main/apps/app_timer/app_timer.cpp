/**
 * @file app_timer.cpp
 * @author Forairaaaaa, Wu23333
 * @brief
 * @version 0.8
 * @date 2024-01-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app_timer.h"
#include "spdlog/spdlog.h"
#include <WiFi.h>


using namespace MOONCAKE::APPS;


void AppTimer::onCreate() {
    spdlog::info("{} onCreate", getAppName());

    // Get hal
    _data.hal = mcAppGetDatabase()->Get("HAL")->value<HAL::Hal *>();
}


void AppTimer::onResume() {
    ANIM_APP_OPEN();

    _data._old_volume = _data.hal->Speaker()->getVolume();
    _data.hal->canvas()->fillScreen(THEME_COLOR_BG);
    _data.hal->canvas()->setFont(FONT_REPL);
    _data.hal->canvas()->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    _data.hal->canvas()->setTextSize(2);
}

static int64_t _time_stopwatch_start = 0;
static int64_t _time_stopwatch_paused_at = 0;

static int64_t _time_timer_end = 0;
static int64_t _time_timer_paused_at = 0;

void AppTimer::onRunning() {
    if ((millis() - _data._time_last_update) > 10) {

        _data.hal->canvas()->fillScreen(THEME_COLOR_BG);

        _data.hal->canvas()->setCursor(4, 2);
        // _data.hal->canvas()->setCursor(10, 5);
        _data.hal->canvas()->setTextSize(1);

        int64_t time_dis = millis();
        if (!_time_stopwatch_start && !_time_timer_end) {
            _data.hal->canvas()->print(_data.hal->isSntpAdjusted() ? "SNTP Time" : "Startup Time");
            _data.hal->canvas()->setTextColor(TFT_LIGHTGREY);
            _data.hal->canvas()->setFont(FONT_SMALL);
            _data.hal->canvas()->drawCenterString("[t] - timer   [s] - stopwatch", _data.hal->canvas()->width() / 2, _data.hal->canvas()->height() - 15);
            _data.hal->canvas()->setTextColor(TFT_ORANGE);
            _data.hal->canvas()->setFont(FONT_BASIC);

            if (_data.hal->isSntpAdjusted()) {
                static time_t now;
                static struct tm timeinfo;
                time(&now);
                localtime_r(&now, &timeinfo);
                snprintf(_data.string_buffer, sizeof(_data.string_buffer), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            } else {
                snprintf(_data.string_buffer, sizeof(_data.string_buffer), "%02lld:%02lld:%02lld", time_dis / 3600000, (time_dis / 60000) % 60, (time_dis / 1000) % 60);
            }
            _data.hal->canvas()->setTextSize(3);
            // _data.hal->canvas()->drawCenterString(_data.string_buffer, _data.hal->canvas()->width() / 2 - 4, 14);
            _data.hal->canvas()->drawCenterString(_data.string_buffer, _data.hal->canvas()->width() / 2 - 4, 18);
            // _data.hal->canvas()->drawCenterString(_data.string_buffer, _data.hal->canvas()->width() / 2, _data.hal->canvas()->height() / 2 - 26);
            // _data.hal->canvas()->setTextSize(2);
            // _data.hal->canvas()->drawCenterString(_data.string_buffer, _data.hal->canvas()->width() / 2, _data.hal->canvas()->height() / 2 - 17);

            // _data.hal->canvas()->setCursor(0, 5);
            // _data.hal->canvas()->setTextSize(3);
            // _data.hal->canvas()->println(_data.string_buffer);
            // _data.hal->canvas()->setTextSize(1);
            // _data.hal->canvas()->print(_data.hal->isSntpAdjusted() ? "SNTP Time" : "Startup Time");

        } else if (_time_timer_end) {
            if (!_time_timer_paused_at) {
                time_dis = _time_timer_end - time_dis;
                if (time_dis < 0) {
                    time_dis = -time_dis;
                    _data.hal->canvas()->setTextColor(TFT_RED);
                    _data.hal->canvas()->print("Timer - overflow");
                    _data.hal->canvas()->setTextColor(TFT_LIGHTGREY);
                    _data.hal->canvas()->setFont(FONT_SMALL);
                    _data.hal->canvas()->drawCenterString("[del] +30min   [other] stop timer", _data.hal->canvas()->width() / 2, _data.hal->canvas()->height() - 15);
                    _data.hal->canvas()->setTextColor(TFT_RED);
                    _data.hal->canvas()->setFont(FONT_BASIC);
                } else {
                    _data.hal->canvas()->setTextColor(TFT_GREENYELLOW);
                    _data.hal->canvas()->print("Timer - running");
                    _data.hal->canvas()->setTextColor(TFT_LIGHTGREY);
                    _data.hal->canvas()->setFont(FONT_SMALL);
                    _data.hal->canvas()->drawCenterString("[del] +30min   [other] pause", _data.hal->canvas()->width() / 2, _data.hal->canvas()->height() - 15);
                    _data.hal->canvas()->setTextColor(TFT_ORANGE);
                    _data.hal->canvas()->setFont(FONT_BASIC);
                }
            } else {
                time_dis = _time_timer_end - _time_timer_paused_at;
                _data.hal->canvas()->print("Timer - paused");
                _data.hal->canvas()->setTextColor(TFT_LIGHTGREY);
                _data.hal->canvas()->setFont(FONT_SMALL);
                _data.hal->canvas()->drawCenterString("[del] exit   [other] continue", _data.hal->canvas()->width() / 2, _data.hal->canvas()->height() - 15);
                _data.hal->canvas()->setTextColor(TFT_ORANGE);
                _data.hal->canvas()->setFont(FONT_BASIC);
            }
            if (_time_timer_paused_at || (time_dis / 20) % 5 >= 2)
                snprintf(_data.string_buffer, sizeof(_data.string_buffer), "%02lld:%02lld:%02lld", time_dis / 3600000, (time_dis / 60000) % 60, (time_dis / 1000) % 60);
            else
                snprintf(_data.string_buffer, sizeof(_data.string_buffer), "%02lld %02lld %02lld", time_dis / 3600000, (time_dis / 60000) % 60, (time_dis / 1000) % 60);
            _data.hal->canvas()->setTextSize(2);
            _data.hal->canvas()->drawCenterString(_data.string_buffer, _data.hal->canvas()->width() / 2 - 15, _data.hal->canvas()->height() / 2 - 17);

            snprintf(_data.string_buffer, sizeof(_data.string_buffer), ".%02lld", (time_dis / 10) % 100);
            _data.hal->canvas()->setTextSize(1.4);
            _data.hal->canvas()->drawCenterString(_data.string_buffer, _data.hal->canvas()->width() / 2 + 65, _data.hal->canvas()->height() / 2 - 8);
            _data.hal->canvas()->setTextColor(TFT_ORANGE);

        } else if (_time_stopwatch_start) {
            if (!_time_stopwatch_paused_at) {
                time_dis -= _time_stopwatch_start;
                _data.hal->canvas()->setTextColor(TFT_GREENYELLOW);
                _data.hal->canvas()->print("Stopwatch - running");
                _data.hal->canvas()->setTextColor(TFT_LIGHTGREY);
                _data.hal->canvas()->setFont(FONT_SMALL);
                _data.hal->canvas()->drawCenterString("[del] reset   [other] pause", _data.hal->canvas()->width() / 2, _data.hal->canvas()->height() - 15);
                _data.hal->canvas()->setTextColor(TFT_ORANGE);
                _data.hal->canvas()->setFont(FONT_BASIC);
            } else {
                time_dis = _time_stopwatch_paused_at - _time_stopwatch_start;
                _data.hal->canvas()->print("Stopwatch - paused");
                _data.hal->canvas()->setTextColor(TFT_LIGHTGREY);
                _data.hal->canvas()->setFont(FONT_SMALL);
                _data.hal->canvas()->drawCenterString("[del] exit   [other] continue", _data.hal->canvas()->width() / 2, _data.hal->canvas()->height() - 15);
                _data.hal->canvas()->setTextColor(TFT_ORANGE);
                _data.hal->canvas()->setFont(FONT_BASIC);
            }
            if (_time_stopwatch_paused_at || (time_dis / 20) % 5 >= 2)
                snprintf(_data.string_buffer, sizeof(_data.string_buffer), "%02lld:%02lld:%02lld", time_dis / 3600000, (time_dis / 60000) % 60, (time_dis / 1000) % 60);
            else
                snprintf(_data.string_buffer, sizeof(_data.string_buffer), "%02lld %02lld %02lld", time_dis / 3600000, (time_dis / 60000) % 60, (time_dis / 1000) % 60);
            _data.hal->canvas()->setTextSize(2);
            _data.hal->canvas()->drawCenterString(_data.string_buffer, _data.hal->canvas()->width() / 2 - 15, _data.hal->canvas()->height() / 2 - 17);

            snprintf(_data.string_buffer, sizeof(_data.string_buffer), ".%02lld", (time_dis / 10) % 100);
            _data.hal->canvas()->setTextSize(1.4);
            _data.hal->canvas()->drawCenterString(_data.string_buffer, _data.hal->canvas()->width() / 2 + 65, _data.hal->canvas()->height() / 2 - 8);

        }

        _data.hal->canvas_update();
        // }

        _data._time_last_update = millis();
    }

    #define STOPWATCH_ALARM_DELTA_S 15 * 60
    if (_time_stopwatch_start && !_time_stopwatch_paused_at) {
        // stopwatch running
        int64_t cur_time = millis();
        if ((cur_time - _time_stopwatch_start) % (STOPWATCH_ALARM_DELTA_S * 1000) < 300 && cur_time - _data._time_last_alarmed > 100) {
            // _data.hal->playKeyboardSound();
            _data.hal->Speaker()->setChannelVolume(TONE_CHANNEL, 255);
            _data.hal->Speaker()->tone(3000, 15, TONE_CHANNEL);
            _data._time_last_alarmed = cur_time;
        }
    }

    if (!_time_timer_paused_at && _time_timer_end && millis() > _time_timer_end) {
        int64_t cur_time = millis();
        if (cur_time - _data._time_last_alarmed > 2000) {
            _data.hal->Speaker()->setVolume(255);
            _data.hal->Speaker()->setChannelVolume(TONE_CHANNEL, 255);
            _data.hal->Speaker()->tone(400, 1000, TONE_CHANNEL);
            _data._time_last_alarmed = cur_time;
        }
    }

    auto pressing_key = _data.hal->keyboard()->getKey();
    if (pressing_key.x >= 0 && !(pressing_key.x == _data._last_pressed_key.x && pressing_key.y == _data._last_pressed_key.y)) {
        if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_BACKSPACE && _time_stopwatch_start) {
            if (_time_stopwatch_paused_at) {
                // exit
                _time_stopwatch_start = 0;
                _time_stopwatch_paused_at = 0;
            } else {
                // reset
                _time_stopwatch_start = millis();
            }
        } else if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_BACKSPACE && _time_timer_end) {
            if (_time_timer_paused_at) {
                // exit
                _time_timer_end = 0;
                _time_timer_paused_at = 0;
            } else {
                // reset
                _time_timer_end += 1800 * 1000;
                _time_timer_paused_at = 0;
                _data.hal->Speaker()->setVolume(_data._old_volume);     // restore volume
            }
        } else if ((_time_timer_end && _time_timer_paused_at) || (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_T && !_time_stopwatch_start && !_time_timer_end)) {
            // timer contine/run
            if (_time_timer_paused_at) {
                _time_timer_end = millis() + (_time_timer_end - _time_timer_paused_at);
            } else {
                _time_timer_end = millis() + 10 * 1000;
            }
            _time_timer_paused_at = 0;
        } else if ((_time_stopwatch_start && _time_stopwatch_paused_at) || (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_S && !_time_stopwatch_start && !_time_timer_end)) {
            // stopwatch contine/run
            _time_stopwatch_start = millis() - (_time_stopwatch_paused_at - _time_stopwatch_start);
            _time_stopwatch_paused_at = 0;
        } else {
            // pause
            if (_time_stopwatch_start) {
                _time_stopwatch_paused_at = millis();
            } else if (_time_timer_end) {
                if (millis() > _time_timer_end) {
                    // if overflow: pause is exit
                    _time_timer_end = 0;
                    _time_timer_paused_at = 0;
                    _data.hal->Speaker()->setVolume(_data._old_volume);     // restore volume
                } else {
                    _time_timer_paused_at = millis();
                }
            }
        }
    }
    _data._last_pressed_key = pressing_key;

    if (!_time_stopwatch_start && !_time_timer_end) {
        // save power?
        delay(100);
        // if (WiFi.status() != WL_NO_SHIELD) {
        //     // wifi on
        //     delay(100);
        // } else {
        //     // wifi not on, use light sleep
        //     esp_sleep_enable_timer_wakeup(200 * 1000);
        //     esp_light_sleep_start();
        // }
    }

    if (_data.hal->homeButton()->pressed()) {
        _data.hal->Speaker()->setVolume(_data._old_volume);
        _data.hal->playNextSound();
        spdlog::info("quit timer");
        destroyApp();
    }
}
