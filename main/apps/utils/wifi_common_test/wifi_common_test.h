/**
 * @file wifi_common_test.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-08-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "esp_wifi.h"


namespace WIFI_COMMON_TEST
{
    static bool is_nvs_inited = false;
    void nvs_init();

    namespace SCAN
    {
        #define DEFAULT_SCAN_LIST_SIZE 10
        struct ScanResult_t
        {
            std::string ssid;
            int8_t rssi = 0;
            int8_t channel = 0;
        };
        uint8_t scan(std::string& result);
        uint8_t scan(std::string& result);
        uint8_t scan(std::string& result);
        void scan(std::vector<ScanResult_t>& scanResult);
        esp_netif_t* multi_scan_init();
        void multi_scan_trigger(bool short_interval);
        uint16_t multi_scan_get_result(std::vector<ScanResult_t>& scanResult, int max_count = DEFAULT_SCAN_LIST_SIZE);
        void multi_scan_destroy(esp_netif_t* sta_netif);
    }

}
