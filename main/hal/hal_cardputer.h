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
            void _mic_init();
            void _speaker_init();
            void _button_init();
            void _bat_init();

        public:
            std::string type() override { return "cardputer"; }
            void init() override;
            void playKeyboardSound() override { _speaker->setChannelVolume(TONE_CHANNEL, 100); _speaker->tone(1250, 20, TONE_CHANNEL); }
            void playLastSound() override { _speaker->setChannelVolume(TONE_CHANNEL, 75); _speaker->tone(1500, 10, TONE_CHANNEL); }
            void playNextSound() override { _speaker->setChannelVolume(TONE_CHANNEL, 100); _speaker->tone(1000, 20, TONE_CHANNEL); }
            // void playKeyboardSound() override { _speaker->setChannelVolume(CHANNEL_TONE, 24*3); _speaker->tone(2500, 20, CHANNEL_TONE); }
            // void playLastSound() override { _speaker->setChannelVolume(CHANNEL_TONE, 18*3); _speaker->tone(3000, 20, CHANNEL_TONE); }
            // void playNextSound() override { _speaker->setChannelVolume(CHANNEL_TONE, 24*3); _speaker->tone(2000, 20, CHANNEL_TONE); }
            uint8_t getBatLevel() override;

        public:
            static void MicTest(HalCardputer* hal);
            static void SpeakerTest(HalCardputer* hal);
            static void LcdBgLightTest(HalCardputer* hal);
    };
}

extern float __cardputer_hal_bat_v;
