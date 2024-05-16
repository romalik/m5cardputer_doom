/**
 * @file hal.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-18
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "../../components/LovyanGFX/src/LovyanGFX.hpp"
#include "keyboard/keyboard.h"
#include "button/Button.h"
#include <iostream>
#include <string>

namespace HAL
{
    /**
    * @brief Hal base for DI
    *
    */
    class Hal
    {
        protected:
            LGFX_Device* _display;
            LGFX_Sprite* _canvas;
            LGFX_Sprite* _canvas_system_bar;
            LGFX_Sprite* _canvas_keyboard_bar;

            KEYBOARD::Keyboard* _keyboard;
            Button* _homeButton;

            bool _sntp_adjusted;

        public:
            Hal() :
            _display(nullptr),
            _canvas(nullptr),
            _canvas_system_bar(nullptr),
            _canvas_keyboard_bar(nullptr),
            _keyboard(nullptr),
            _homeButton(nullptr),
            _sntp_adjusted(false)
            {}

            // Getter
            inline LGFX_Device* display() { return _display; }
            inline LGFX_Sprite* canvas() { return _canvas; }
            inline LGFX_Sprite* canvas_system_bar() { return _canvas_system_bar; }
            inline LGFX_Sprite* canvas_keyboard_bar() { return _canvas_keyboard_bar; }
            inline KEYBOARD::Keyboard* keyboard() { return _keyboard; }
            inline Button* homeButton() { return _homeButton; }

            inline void setSntpAdjusted(bool isAdjusted) { _sntp_adjusted = isAdjusted; }
            inline bool isSntpAdjusted(void) { return _sntp_adjusted; }

            // Override
            virtual std::string type() { return "null"; }
            virtual void init() {}

            virtual void playLastSound() {}
            virtual void playNextSound() {}
            virtual void playKeyboardSound() {}

            virtual uint8_t getBatLevel() { return 100; }
    };
}
