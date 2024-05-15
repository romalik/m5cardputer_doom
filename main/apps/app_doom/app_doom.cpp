/**
 * @file app_set_wifi.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.6
 * @date 2023-09-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_doom.h"
#include "spdlog/spdlog.h"

// #include "../utils/wifi_connect_wrap/wifi_connect_wrap.h"
// #include "../utils/sntp_wrap/sntp_wrap.h"
#include <WiFi.h>
#include "esp_sntp.h"

using namespace MOONCAKE::APPS;


#define _keyboard _data.hal->keyboard()
#define _canvas _data.hal->canvas()
#define _canvas_update _data.hal->canvas_update
#define _canvas_clear() _canvas->fillScreen(THEME_COLOR_BG)


void AppDOOM::_update_input() {
    // spdlog::info("{} {}", _keyboard->keyList().size(), _data.last_key_num);

    // If changed
    if (_keyboard->keyList().size() != _data.last_key_num) {
        // If key pressed
        if (_keyboard->keyList().size() != 0) {
            // Update states and values
            _keyboard->updateKeysState();

            // If enter
            if (_keyboard->keysState().enter) {
                // New line
                _canvas->print(" \n");

                _update_state();
            }

            // If space
            else if (_keyboard->keysState().space) {
                _canvas->print(' ');
                _data.repl_input_buffer += ' ';
            }

            // If delete
            else if (_keyboard->keysState().del) {
                if (_data.repl_input_buffer.size()) {
                    // Pop input buffer
                    _data.repl_input_buffer.pop_back();

                    // Pop canvas display
                    int cursor_x = _canvas->getCursorX();
                    int cursor_y = _canvas->getCursorY();

                    if (cursor_x - FONT_REPL_WIDTH < 0) {
                        // Last line
                        cursor_y -= FONT_REPL_HEIGHT;
                        cursor_x = _canvas->width() - FONT_REPL_WIDTH;
                    } else {
                        cursor_x -= FONT_REPL_WIDTH;
                    }

                    spdlog::info("new cursor {} {}", cursor_x, cursor_y);

                    _canvas->setCursor(cursor_x, cursor_y);
                    _canvas->print("  ");
                    _canvas->setCursor(cursor_x, cursor_y);
                }
            }

            // Normal chars
            else {
                for (auto& i : _keyboard->keysState().values) {
                    // spdlog::info("{}", i);

                    _canvas->print(i);
                    _data.repl_input_buffer += i;
                }
            }

            _canvas_update();

            // Update last key num
            _data.last_key_num = _keyboard->keyList().size();
        }
        else {
            // Reset last key num
            _data.last_key_num = 0;
        }
    }
}


void AppDOOM::_update_cursor() {
}



void AppDOOM::_update_state() {
}


unsigned short * __sprite_data;

LGFX_Sprite * doom_canvas;

KEYBOARD::Keyboard* __keyboard;

bool scale = false;

void __update_sprite() {

    if(scale) {
        doom_canvas->pushRotateZoom(240.0f/2.0f,135.0f/2.0f,0.0f,1.0f,(135.0f/160.0f));
    } else {
        doom_canvas->pushSprite(0,0);
    }
        
}

void __set_sprite_pallete(unsigned char * pallete) {
    printf("__set_sprite_pallete()\n");
    for(int i = 0; i< 256; i++)
    {
        unsigned int r = *pallete++;
        unsigned int g = *pallete++;
        unsigned int b = *pallete++;

        doom_canvas->setPaletteColor(i,r,g,b);
    }
}

#include <queue>
std::queue<uint8_t> evt_queue;

#define KB_RIGHT    '/'
#define KB_LEFT     ','
#define KB_UP       ';'
#define KB_DOWN     '.'
#define KB_ESC      '`'
#define KB_ENTER    0x01
#define KB_L        'l'
#define KB_QUOTE    '\''
#define KB_CTRL     0x02
#define KB_A        'a'
#define KB_TAB      0x03
#define KB_F        'f'
#define KB_Z        'z'
#define KB_EQUAL    '='
#define KB_MINUS    '-'

unsigned char kb_state[127] = {0};


void check_evts() {
    unsigned char new_state[127] = {0};
    __keyboard->updateKeyList();
    __keyboard->updateKeysState();
    if (__keyboard->keysState().enter) {
        new_state[KB_ENTER] = 1;
    } else if(__keyboard->keysState().ctrl) {
        new_state[KB_CTRL]  = 1;
    } else if(__keyboard->keysState().tab) {
        new_state[KB_TAB]  = 1;
    }
    for (auto& i : __keyboard->keysState().values) {
        new_state[i] = 1;
        if(i == 's') {
            scale = !scale;
        }
    }

    for(int i = 0; i<127; i++) {
        if(kb_state[i] != new_state[i]) {
            kb_state[i] = new_state[i];
            evt_queue.push((new_state[i] << 7) | (i&0x7f));
        }
    }
}

unsigned char __get_event() {
    check_evts();
    if(evt_queue.empty()) {
        return 0;
    } else {
        uint8_t evt = evt_queue.front();
        printf("Send event 0x%02x (%s '%c')\n", evt, evt&0x80?"DOWN":"UP  ", evt&0x7f);
        evt_queue.pop(); 
        return evt;
    }

}

void AppDOOM::onCreate() {
    spdlog::info("{} onCreate", getAppName());

    // Get hal
    _data.hal = mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();
    _canvas_clear();
    _canvas->setTextScroll(true);
    _canvas->setBaseColor(THEME_COLOR_BG);
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _canvas->setFont(FONT_REPL);
    _canvas->setTextSize(FONT_SIZE_REPL);
    _canvas->setCursor(0, 0);

    doom_canvas = new LGFX_Sprite(_data.hal->display());
    doom_canvas->setColorDepth(lgfx::color_depth_t::palette_8bit);
    doom_canvas->createPalette();
    
    doom_canvas->createSprite(240,160);

    __keyboard = _keyboard;

    __sprite_data = (unsigned short *)doom_canvas->getBuffer();

    _canvas->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    _canvas->printf("Doom\n");
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _canvas->printf(">>> ");
    _canvas_update();

}


void AppDOOM::onResume() {
    ANIM_APP_OPEN();

    _canvas_clear();
    _canvas->setTextScroll(true);
    _canvas->setBaseColor(THEME_COLOR_BG);
    _canvas->setTextColor(THEME_COLOR_REPL_TEXT, THEME_COLOR_BG);
    _canvas->setFont(FONT_REPL);
    _canvas->setTextSize(FONT_SIZE_REPL);
    _canvas->setCursor(0, 0);

}

extern "C" int doom_main(int argc, const char * argv);




void AppDOOM::onRunning() {
    doom_main(0,0);

    if (_data.hal->homeButton()->pressed()) {
        _data.hal->playNextSound();
        spdlog::info("quit set wifi");
        destroyApp();
    }
}


void AppDOOM::onDestroy() {
    _canvas->setTextScroll(false);
}
