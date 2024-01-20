/**
 * @file app_env.h
 * @author Wu23333
 * @brief
 * @version 0.1
 * @date 2024-01-09
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

#include "assets/env_big.h"
#include "assets/env_small.h"

#include "lib/SensirionI2CSht4x.h"
#include "lib/Adafruit_BMP280.h"
#include "lib/Adafruit_Sensor.h"

namespace MOONCAKE
{
    namespace APPS
    {
        class AppENV : public APP_BASE
        {
            private:
                enum State_t
                {
                    state_init = 0,
                    state_connected,
                };

                struct Data_t
                {
                    HAL::Hal* hal = nullptr;
                    State_t current_state = state_init;
                    Adafruit_BMP280 bmp;
                    SensirionI2CSht4x sht4x;
                    int64_t _last_update = 0;
                };
                Data_t _data;

            public:
                void onCreate() override;
                void onResume() override;
                void onRunning() override;
                void onDestroy() override;
        };

        class AppENV_Packer : public APP_PACKER_BASE
        {
            std::string getAppName() override { return "ENV IV"; }
            void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_env_big, image_data_env_small)); }
            void* newApp() override { return new AppENV; }
            void deleteApp(void *app) override { delete (AppENV*)app; }
        };
    }
}
