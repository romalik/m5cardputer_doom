/**
 * @file app_set_wifi.h
 * @author Forairaaaaa
 * @brief
 * @version 0.6
 * @date 2023-09-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <mooncake.h>
#include "../../hal/hal.h"
#include "../utils/theme/theme_define.h"
#include "../utils/anim/anim_define.h"
#include "../utils/icon/icon_define.h"

#include "assets/doom_big.h"
#include "assets/doom_small.h"


namespace MOONCAKE
{
    namespace APPS
    {
        class AppDOOM : public APP_BASE
        {
            private:


                struct Data_t
                {
                    HAL::Hal* hal = nullptr;

                    int last_key_num = 0;
                    std::string repl_input_buffer;
                    bool is_caps = false;
                    char* value_buffer = nullptr;

                    uint32_t cursor_update_time_count = 0;
                    uint32_t cursor_update_period = 500;
                    bool cursor_state = false;

                    
                    std::string wifi_ssid;
                    std::string wifi_password;
                    bool _alreay_connected;
                };
                Data_t _data;

                void _update_input();
                void _update_cursor();
                void _update_state();

            public:
                void onCreate() override;
                void onResume() override;
                void onRunning() override;
                void onDestroy() override;
        };

        class AppDOOM_Packer : public APP_PACKER_BASE
        {
            std::string getAppName() override { return "DOOM"; }
            void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_doom_big, image_data_doom_small)); }
            void* newApp() override { return new AppDOOM; }
            void deleteApp(void *app) override { delete (AppDOOM*)app; }
        };
    }
}
