/**
 * @file app_record.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-19
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

#include "assets/record_big.h"
#include "assets/record_small.h"

static constexpr const size_t record_number = 256;
static constexpr const size_t record_length = 200;
static constexpr const size_t record_size = record_number * record_length;
static constexpr const size_t record_samplerate = 16000;

namespace MOONCAKE
{
    namespace APPS
    {
        class AppRecord : public APP_BASE
        {
            private:
                struct Data_t
                {
                    HAL::Hal* hal = nullptr;
                    uint8_t old_volume;
                    int16_t prev_y[record_length];
                    int16_t prev_h[record_length];
                    size_t rec_record_idx = 2;
                    size_t draw_record_idx = 0;
                    int16_t *rec_data = nullptr;
                };
                Data_t _data;

            public:
                void onCreate() override;
                void onResume() override;
                void onRunning() override;
        };

        class AppRecord_Packer : public APP_PACKER_BASE
        {
            std::string getAppName() override { return "RECORD"; }
            void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_record_big, image_data_record_small)); }
            void* newApp() override { return new AppRecord; }
            void deleteApp(void *app) override { delete (AppRecord*)app; }
        };
    }
}
