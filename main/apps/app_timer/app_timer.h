/**
 * @file app_timer.h
 * @author Forairaaaaa, Wu23333
 * @brief
 * @version 0.2
 * @date 2024-01-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#include <mooncake.h>
#include "../../hal/hal.h"
#include "../utils/theme/theme_define.h"
#include "../utils/anim/anim_define.h"
#include "../utils/icon/icon_define.h"

#include "assets/timer_big.h"
#include "assets/timer_small.h"


namespace MOONCAKE {
    namespace APPS {
        class AppTimer : public APP_BASE {
            private:
                struct Data_t {
                    HAL::Hal* hal = nullptr;
                    char string_buffer[100];
                    uint8_t _old_volume;
                    int64_t _time_last_update = 0;
                    int64_t _time_last_alarmed = 0;
                    KEYBOARD::Point2D_t _last_pressed_key = {-1, -1};
                };
                Data_t _data;

            public:
                void onCreate() override;
                void onResume() override;
                void onRunning() override;
        };

        class AppTimer_Packer : public APP_PACKER_BASE {
            // std::string getAppName() override { return "TIMER"; }
            std::string getAppName() override { return "CLOCK"; }
            void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_timer_big, image_data_timer_small)); }
            void* newApp() override { return new AppTimer; }
            void deleteApp(void *app) override { delete (AppTimer*)app; }
        };
    }
}
