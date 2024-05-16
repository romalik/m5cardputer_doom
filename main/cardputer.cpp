/**
 * @file cardputer.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.6
 * @date 2023-10-12
 *
 * @copyright Copyright (c) 2023
 *
 */


#include <stdio.h>

#include "hal/hal_cardputer.h"



using namespace HAL;



HalCardputer hal;

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


extern "C" int doom_main(int argc, const char * argv);


extern "C" void app_main(void)
{
    printf("initArduino\n");
    //initArduino();

    printf("initHAL\n");
    // Init hal
    hal.init();


    printf("initCanvas\n");
    doom_canvas = new LGFX_Sprite(hal.display());
    doom_canvas->setColorDepth(lgfx::color_depth_t::palette_8bit);
    doom_canvas->createPalette();
    
    doom_canvas->createSprite(240,160);
    

    __keyboard = hal.keyboard();

    __sprite_data = (unsigned short *)doom_canvas->getBuffer();

    printf("Launch DOOM\n");
    doom_main(0,0);

}
