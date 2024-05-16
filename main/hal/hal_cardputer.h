/**
 * @file hal_cardputer.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "hal.h"

namespace HAL
{
    class HalCardputer : public Hal
    {
        private:
            void _display_init();
            void _keyboard_init();
            void _button_init();
            void _bat_init();

        public:
            std::string type() override { return "cardputer"; }
            void init() override;
            uint8_t getBatLevel() override;

        public:
            static void LcdBgLightTest(HalCardputer* hal);
    };
}

extern float __cardputer_hal_bat_v;
