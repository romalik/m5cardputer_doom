/**
 * @file app_record.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-19
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_wifi_scan.h"
#include "spdlog/spdlog.h"
// #include "../utils/wifi_common_test/wifi_common_test.h"
#include "../utils/theme/theme_define.h"
#include "esp_wifi.h"
#include <WiFi.h>

using namespace MOONCAKE::APPS;
// using namespace WIFI_COMMON_TEST;


void AppWifiScan::onCreate()
{
    spdlog::info("{} onCreate", getAppName());

    // Get hal
    _data.hal = mcAppGetDatabase()->Get("HAL")->value<HAL::Hal *>();
}


void AppWifiScan::_scan() {
    WiFi.scanNetworks(true, true, false, _data._short_interval ? 200 : 500);
}


void AppWifiScan::_update_display() {
    _data.hal->canvas()->fillScreen(THEME_COLOR_BG);

    _data.hal->canvas()->setCursor(0, 0);
    _data.hal->canvas()->setTextSize(1);
    int idx = 0;
    for (auto& i : _data.scan_result)
    {
        if (idx - _data._page*6 >= 6)
            break;

        // spdlog::info("{} {}", i.ssid, i.rssi);

        if (idx >= _data._page*6) {
            _data.hal->canvas()->setTextColor(
                (i.rssi > -65) ? TFT_GREEN :
                (i.rssi > -90) ? TFT_YELLOW : TFT_RED, THEME_COLOR_BG);

            _data.hal->canvas()->printf("%d", i.rssi);
            _data.hal->canvas()->printf(" %02d", i.channel);
            _data.hal->canvas()->printf(" %s\n", i.ssid.substr(0, 18).c_str());
        }
        idx++;
    }

    _data.hal->canvas()->setTextColor(TFT_LIGHTGREY);
    _data.hal->canvas()->drawCenterString(std::to_string(_data.scan_result.size()).c_str(), _data.hal->canvas()->width() - 15, _data.hal->canvas()->height() - 17);
    _data.hal->canvas()->setFont(FONT_SMALL);
    if (_data._short_interval)
        _data.hal->canvas()->drawCenterString("RSSI CH  SSID    200ms   APs:", 91, _data.hal->canvas()->height() - 10);
    else
        _data.hal->canvas()->drawCenterString("RSSI CH  SSID    500ms   APs:", 91, _data.hal->canvas()->height() - 10);
    _data.hal->canvas()->drawCenterString(std::to_string(_data._page+1).c_str(), _data.hal->canvas()->width() - 10, 0);
    _data.hal->canvas()->setFont(FONT_BASIC);
    _data.hal->canvas_update();

}

void AppWifiScan::onResume()
{
    ANIM_APP_OPEN();


    _data.hal->canvas()->fillScreen(THEME_COLOR_BG);
    _data.hal->canvas()->setFont(FONT_REPL);
    _data.hal->canvas()->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    _data.hal->canvas()->setTextSize(1);
    _data.hal->canvas()->setCursor(10, 5);
    _data.hal->canvas()->print("Scanning...");
    _data.hal->canvas_update();

    // _data.esp_netif = SCAN::multi_scan_init();
    WiFi.mode(WIFI_STA);

    // SCAN::multi_scan_trigger(_data._short_interval);
    _scan();
    // _data.last_scan_start_time = millis();

}

#define MAX_PAGES 200
void AppWifiScan::onRunning()
{
    if (_data.hal->homeButton()->pressed())
    {
        _data.hal->playNextSound();

        _data.hal->canvas()->fillScreen(THEME_COLOR_BG);
        _data.hal->canvas()->setFont(FONT_REPL);
        _data.hal->canvas()->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
        _data.hal->canvas()->setTextSize(1);
        _data.hal->canvas()->setCursor(10, 5);
        _data.hal->canvas()->print("Stopping...");
        _data.hal->canvas_update();


        // SCAN::multi_scan_destroy(_data.esp_netif);

        // waiting for last scan complete
        while (WiFi.scanComplete() < 0);

        WiFi.scanDelete();

        if (WiFi.status() != WL_CONNECTED) {
            WiFi.disconnect(true);
        }

        spdlog::info("quit wifi scan");
        destroyApp();
    }

    // keys: up/down: change page, other: change interval
    auto pressed_key_count = _data.hal->keyboard()->keyList().size();
    auto pressing_key = _data.hal->keyboard()->getKey();
    if (pressed_key_count != _data._last_pressed_key_count && pressed_key_count) {
        if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_SEMICOLON
            || _data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_COMMA) {
            if (_data._page > 0) _data._page --;
        } else if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_DOT
            || _data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_KPSLASH) {
            if (_data._page < (MAX_PAGES-1)) _data._page ++;
        } else {
            _data._short_interval = !_data._short_interval;
        }

        _update_display();
    }
    _data._last_pressed_key_count = pressed_key_count;

    // if ((millis() - _data.last_scan_start_time) > (_data._short_interval ? 1680 : 5040) || !_data.last_scan_start_time) {
    //     // _data.ap_count = SCAN::multi_scan_get_result(_data._scan_result, MAX_PAGES * 6);
    //     // SCAN::multi_scan_trigger(_data._short_interval);
    //     _scan();
    //     _data.last_scan_start_time = millis();

    //     _update_display();
    // }

    auto ap_count = WiFi.scanComplete();
    if (ap_count >= 0 && !_data._time_delayed_scan_at) {
        _data.scan_result.clear();
        for (int i = 0; i < ap_count; ++i) {
            std::string ssid;
            if (WiFi.SSID(i) != "")
                ssid = WiFi.SSID(i).substring(0, 18).c_str();
            else
                ssid = ('-' + WiFi.BSSIDstr(i)).c_str();
            _data.scan_result.emplace_back(ssid, WiFi.RSSI(i), WiFi.channel(i));
        }
        if (WiFi.status() == WL_CONNECTED) {
            // delay between scans to keep other function normal
            _data._time_delayed_scan_at = millis() + 500;
        } else {
            _scan();
        }
        _update_display();
    }

    if (_data._time_delayed_scan_at && _data._time_delayed_scan_at < millis()) {
        _data._time_delayed_scan_at = 0;
        _scan();
    }

}
