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

bool scale = true;

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

#include <driver/gpio.h>
#define digitalWrite(pin, level) gpio_set_level((gpio_num_t)pin, level)
#define digitalRead(pin) gpio_get_level((gpio_num_t)pin)

const int kb_output_list[] = {8, 9, 11};
const int kb_input_list[] = {13, 15, 3, 4, 5, 6, 7};

void kb_set_output(const int pinList[], uint8_t output)
{
    digitalWrite(pinList[0], (output & 0B00000001));
    digitalWrite(pinList[1], (output & 0B00000010));
    digitalWrite(pinList[2], (output & 0B00000100));
}


uint8_t kb_get_input(const int pinList[])
{
    uint8_t buffer = 0x00;
    uint8_t pin_value = 0x00;

    for (int i = 0; i < 7; i++) 
    {
        pin_value = (digitalRead(pinList[i]) == 1) ? 0x00 : 0x01;
        pin_value = pin_value << i;
        buffer = buffer | pin_value;
    }

    return buffer;
}


void kb_init(const int output_pins[], const int input_pins[])
{

    for (int j = 0; j<3; j++) 
    {   
        int i = output_pins[j];
        gpio_reset_pin((gpio_num_t)i);
        gpio_set_direction((gpio_num_t)i, GPIO_MODE_OUTPUT);
        gpio_set_pull_mode((gpio_num_t)i, GPIO_PULLUP_PULLDOWN);
        digitalWrite(i, 0);
    }

    for (int j = 0; j<7; j++) 
    {
        int i = input_pins[j];
        gpio_reset_pin((gpio_num_t)i);
        gpio_set_direction((gpio_num_t)i, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)i, GPIO_PULLUP_ONLY);
    }

    kb_set_output(output_pins, 0);
}

void print_bin(uint8_t v) {
    for(int i = 0; i<8; i++) {
        if(v & (1U<<(7-i))) {
            printf("1");
        } else {
            printf("0");
        }
    }
}

#define K_BKSP  0x01
#define K_TAB   0x02
#define K_FN    0x03
#define K_SHIFT 0x04
#define K_ENT   0x05
#define K_CTRL  0x06
#define K_OPT   0x07
#define K_ALT   0x08

uint8_t keymap[] = {
    '`',    '1',     '2',    '3',    '4',    '5',    '6',    '7',    '8',    '9',    '0',    '-',    '=',    K_BKSP,
    K_TAB,  'q',     'w',    'e',    'r',    't',    'y',    'u',    'i',    'o',    'p',    '[',    ']',    '\\',
    K_FN,   K_SHIFT, 'a',    's',    'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',    '\'',   K_ENT,
    K_CTRL, K_OPT,   K_ALT,  'z',    'x',    'c',    'v',    'b',    'n',    'm',    ',',    '.',    '/',    ' '    
};


char kb_state[14*4];
void kb_update_state() {
    memset(kb_state, 0, 14*4);
    for(int row = 0; row < 8; row++) {
        kb_set_output(kb_output_list, row);
        uint8_t resp = kb_get_input(kb_input_list);
        for(int col = 0; col < 7; col++) {
            if((resp & (1U<<col))) {
                int k_row = 3 - (row & 0x03);
                int k_col = col*2 + ((row&0x04)?0:1);

                kb_state[k_row*14 + k_col] = 1;
            }
        }
    }
}
#include "common_define.h"
void check_evts() {

    uint8_t old_state[14*4];
    memcpy(old_state,kb_state,14*4);

    kb_update_state();

/*
    printf("\n\n");
    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 14; col++) {
            if(kb_state[row*14 + col]) {
                printf("[#]");
            } else {
                printf("[ ]");
            }
        }
        printf("\n");
    }
*/

    printf("kb_state: ");
    for(int i = 0; i<14*4; i++) {
        uint8_t v = keymap[i];
        
        if(kb_state[i]) {
            if(v == K_ENT) {
                printf("[ENT]");
            } else if(v == K_CTRL) {
                printf("[CTRL]");
            } else if(v == K_TAB) {
                printf("[TAB]");
            } else if(v == K_BKSP) {
                printf("[BKSP]");
            } else if(v == K_FN) {
                printf("[FN]");
            } else if(v == K_SHIFT) {
                printf("[SHIFT]");
            } else if(v == K_OPT) {
                printf("[OPT]");
            } else if(v == K_ALT) {
                printf("[ALT]");
            } else {
                printf("[%c]",v);
            }
        }
        if(old_state[i] != kb_state[i]) {
            evt_queue.push((kb_state[i] << 7) | (v&0x7f));
        }
    }
    printf("\n");

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

#include "hal/display/hal_display.hpp"
extern "C" void app_main(void)
{
    printf("Startup\n");
    kb_init(kb_output_list, kb_input_list);

    printf("initCanvas\n");
    LGFX_Cardputer * _display = new LGFX_Cardputer;
    _display->init();
    _display->setRotation(1);
    doom_canvas = new LGFX_Sprite(_display);
    doom_canvas->setColorDepth(lgfx::color_depth_t::palette_8bit);
    doom_canvas->createPalette();
    
    doom_canvas->createSprite(240,160);
    

    __sprite_data = (unsigned short *)doom_canvas->getBuffer();

    printf("Launch DOOM\n");

    doom_main(0,0);

}
