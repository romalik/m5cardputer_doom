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

extern "C" void __update_sprite() {

    if(scale) {
        doom_canvas->pushRotateZoom(240.0f/2.0f,135.0f/2.0f,0.0f,1.0f,(135.0f/160.0f));
    } else {
        doom_canvas->pushSprite(0,0);
    }
        
}

extern "C" void __set_sprite_pallete(unsigned int i, unsigned char r, unsigned char g, unsigned char b) {
    printf("__set_sprite_pallete()\n");
    doom_canvas->setPaletteColor(i,r,g,b);
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

    //printf("kb_state: ");
    for(int i = 0; i<14*4; i++) {
        uint8_t v = keymap[i];
        /*
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
        */
        if(old_state[i] != kb_state[i]) {
            evt_queue.push((kb_state[i] << 7) | (v&0x7f));
        }
    }
    //printf("\n");

}

extern "C" unsigned char __get_event() {
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

#include <driver/sdmmc_host.h>
#include <esp_vfs_fat.h>

// Storage
#define RG_STORAGE_DRIVER           1                   // 0 = Host, 1 = SDSPI, 2 = SDMMC, 3 = USB, 4 = Flash
#define RG_STORAGE_HOST             SPI3_HOST   // Used by driver 1 and 2
#define RG_STORAGE_SPEED            SDMMC_FREQ_DEFAULT  // Used by driver 1 and 2
#define RG_STORAGE_ROOT             "/sd"               // Storage mount point

// SPI SD Card
#define RG_GPIO_SDSPI_MISO          GPIO_NUM_39
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_14
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_40
#define RG_GPIO_SDSPI_CS            GPIO_NUM_12
/*
static esp_err_t sdcard_do_transaction(int slot, sdmmc_command_t *cmdinfo)
{
    esp_err_t ret = sdspi_host_do_transaction(slot, cmdinfo);
    return ret;
}
*/
static esp_err_t init_sd() {
    sdmmc_host_t host_config = SDSPI_HOST_DEFAULT();
    host_config.slot = RG_STORAGE_HOST;
    host_config.max_freq_khz = RG_STORAGE_SPEED;
    host_config.do_transaction = &sdspi_host_do_transaction;
    esp_err_t err;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = RG_STORAGE_HOST;
    slot_config.gpio_cs = RG_GPIO_SDSPI_CS;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = RG_GPIO_SDSPI_MOSI,
        .miso_io_num = RG_GPIO_SDSPI_MISO,
        .sclk_io_num = RG_GPIO_SDSPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    err = spi_bus_initialize(RG_STORAGE_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) // check but do not abort, let esp_vfs_fat_sdspi_mount decide
        printf("SPI bus init failed (0x%x)", err);

    esp_vfs_fat_mount_config_t mount_config = {.max_files = 8};

    err = esp_vfs_fat_sdspi_mount(RG_STORAGE_ROOT, &host_config, &slot_config, &mount_config, NULL);
    if (err == ESP_ERR_TIMEOUT || err == ESP_ERR_INVALID_RESPONSE || err == ESP_ERR_INVALID_CRC)
    {
        printf("SD Card mounting failed (0x%x), retrying at lower speed...\n", err);
        host_config.max_freq_khz = SDMMC_FREQ_PROBING;
        err = esp_vfs_fat_sdspi_mount(RG_STORAGE_ROOT, &host_config, &slot_config, &mount_config, NULL);
    }
    return err;
}

#include <dirent.h>

void listdir(const char *name, int indent)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            printf("%*s[%s]\n", indent, "", entry->d_name);
            listdir(path, indent + 2);
        } else {
            printf("%*s- %s\n", indent, "", entry->d_name);
        }
    }
    closedir(dir);
}

#include <sys/stat.h>
#include <sys/types.h>

#include "hal/display/hal_display.hpp"


/*
extern "C" void __asan_load4_noabort(void * ptr) {

}
*/
extern "C" void my_putc(char c) {
    printf("%c", c);
}


#define MAX_MMAP_FILE_SIZE         16*1024*1024U
// 4 Kb
#define MMAP_PAGE_CHUNK_SHIFT      8
#define MMAP_PAGE_SIZE             (1UL<<(MMAP_PAGE_CHUNK_SHIFT))
#define MMAP_PAGE_POSITION_MASK    (MMAP_PAGE_SIZE-1)

#define MMAP_ARENA_N_PAGES 400

typedef struct mmap_page {
    char data[MMAP_PAGE_SIZE];
    //unsigned int next_ptr;
    char file_id;
    unsigned int chunk_idx; //merge file id and chunk idx
    unsigned int ttl;
} mmap_page_t;


mmap_page_t  mmap_arena[MMAP_ARENA_N_PAGES];

FILE * mmaped_files[16] = {NULL};

extern "C" void my_mmap_init() {
    memset(mmap_arena, 0, sizeof(mmap_page_t) * MMAP_ARENA_N_PAGES);
    for(int i = 0; i<MMAP_ARENA_N_PAGES; i++) {
        mmap_arena[i].file_id = -1;
        mmap_arena[i].ttl = 0;
        printf("mmap_arena[%d]: %p\n", i, &mmap_arena[i]);
    }
}

extern "C" char * my_mmap(FILE * fd) {
    char * retval = NULL;
    printf("my_mmap(%p)\n", fd);
    for(int i = 0; i<16; i++) {
        if(mmaped_files[i] == NULL) {
            mmaped_files[i] = fd;
            retval = (char *)((0x80 | (i&0x0f)) << (8*3));
            break;
        }
    }
    printf("my_mmap() -> %p\n", retval);
    return retval;
}

extern "C" void my_unmmap(FILE * fd) {
    for(int i = 0; i<16; i++) {
        if(mmaped_files[i] == fd) {
            mmaped_files[i] = NULL;
            return;
        }
    }
}
mmap_page_t * prev_page = mmap_arena;
mmap_page_t * before_prev_page = mmap_arena;

extern "C" mmap_page_t * get_mmap_page_for_id_and_offset(int id, unsigned int chunk_idx) {
    //printf("get_mmap_page_for_id_and_offset(%d, %d)\n", id, chunk_idx);
    if(prev_page->file_id == id && prev_page->chunk_idx == chunk_idx) {
        return prev_page;
    }

    if(before_prev_page->file_id == id && before_prev_page->chunk_idx == chunk_idx) {
        return before_prev_page;
    }

    mmap_page_t * ret = NULL;

    for(int i = 0; i<MMAP_ARENA_N_PAGES; i++) {
        if(mmap_arena[i].file_id == id && mmap_arena[i].chunk_idx == chunk_idx) {
            //printf("get_mmap_page_for_id_and_offset() -> %p\n", &mmap_arena[i]);
            before_prev_page = prev_page;
            prev_page = &mmap_arena[i];
            ret = &mmap_arena[i];
            ret->ttl = 0xffffffff;
        } else {
            if(mmap_arena[i].ttl) mmap_arena[i].ttl--;
        }
    }

    return ret;
}


extern "C" mmap_page_t * get_page_to_swap_out() {
    mmap_page_t * min_ttl_page = mmap_arena;
    for(int i = 1; i<MMAP_ARENA_N_PAGES; i++) {
        if(mmap_arena[i].ttl < min_ttl_page->ttl) min_ttl_page = &mmap_arena[i];
    }

    printf("Swap out ttl 0x%08x\n", min_ttl_page->ttl);
    return min_ttl_page;
}

extern "C" void swap_in_page(mmap_page_t * page, int file_id, unsigned int chunk_idx, unsigned int offset) {
    //printf("\n\n---\nswap_in_page(%d, %d, %d, %d)\n---\n", page_idx, file_id, chunk_idx, offset);
    fseek(mmaped_files[file_id], chunk_idx<<MMAP_PAGE_CHUNK_SHIFT, SEEK_SET);
    fread(page->data, MMAP_PAGE_SIZE, 1, mmaped_files[file_id]);
    /*
    if(n_read < MMAP_PAGE_SIZE) {
        mmap_arena[page_idx].data[n_read] = 0;
        //remove me
        //added for test_mmu impl
    }
    */
    page->chunk_idx = chunk_idx;
    page->file_id = file_id;
    page->ttl = 0xffffffff;
    //mmap_arena[page_idx].next_ptr = ((unsigned int)(0xf0 | (file_id&0x0f)) << 8*3) | ((chunk_idx + 1) << MMAP_PAGE_CHUNK_SHIFT);
    //printf("swap_in_page next_ptr 0x%08x\n", mmap_arena[page_idx].next_ptr);
}

extern "C" unsigned int get_ptr_to_buffer(void * ptr) {
    int file_id = (((unsigned int)ptr & 0x0f000000) >> (8*3));
    unsigned int offset = (unsigned int)ptr & 0x00ffffff;
    unsigned int chunk_idx = offset >> MMAP_PAGE_CHUNK_SHIFT;

    //printf("get_ptr_to_buffer(%p)\n", ptr);

    mmap_page_t * page = get_mmap_page_for_id_and_offset(file_id, chunk_idx);

    if(page == NULL) {
        page = get_page_to_swap_out();
        //printf("-> [%d](%d)\n", page - mmap_arena, page->chunk_idx);
        //printf("get_ptr_to_buffer() swapout %d\n", p_idx);
        swap_in_page(page, file_id, chunk_idx, offset);
        //printf("<- [%d](%d)\n", page - mmap_arena, page->chunk_idx);
    }
    //printf("get_ptr_to_buffer() -> %lu\n", (unsigned int)(page->data) + (offset & MMAP_PAGE_POSITION_MASK));
    return (unsigned int)(page->data) + (offset & MMAP_PAGE_POSITION_MASK);
}




extern "C" void * __remap_ptr(void * ptr, int is_store, unsigned int size, unsigned int align) {
    //return ptr;
    //printf("__remap_ptr check\n");
    if(((unsigned int)ptr & 0x80000000)) {
        unsigned int val = get_ptr_to_buffer(ptr);
        //printf("\n---\nTrap! %p replace with 0x%08X\n", ptr, val);
        return (void*)val;            
    } else {
        //printf("\n---\nNo trap! %p\n", ptr);
        return ptr;
    }
}



/*
static inline void do_stack_sift(void * ptr) {
   unsigned int frame;
    void * next_chunk_ptr = NULL;
    void * search_address = ptr;
    //printf("hook: %p\n", ptr);

    if((next_chunk_ptr = (void *)check_mmap_page_bound(ptr))) {
        search_address = ptr;
        ptr = next_chunk_ptr;
    }    
    int c = 0;
    if(((unsigned int)ptr & 0xf0000000) == 0xf0000000) {
        printf("\n---\nTrap! %p\n", ptr);
        for(int i = 50; i>-50; i--) {
            unsigned int * p = (unsigned int*)((unsigned int)(&(frame)) + i*4);
            printf("stack%+02d : 0x%08X\n", i, *p);
            if((unsigned int)*p == (unsigned int)search_address) {
                //unsigned int val = (unsigned int)dstr + c; c++;
                unsigned int val = get_ptr_to_buffer(ptr);
                printf("Gotcha! Replace with 0x%08X\n", val);
                *p = val;
                //return;
            }
        }
    }
}
extern "C" void __asan_handle_no_return() {
}


extern "C" void __asan_storeN_noabort(void * ptr) {
}
extern "C" void __asan_store8_noabort(void * ptr) {
}
extern "C" void __asan_store4_noabort(void * ptr) {
}
extern "C" void __asan_store2_noabort(void * ptr) {
}
extern "C" void __asan_store1_noabort(void * ptr) {
}

extern "C" void __asan_loadN_noabort(void * ptr, unsigned int size) {
    do_stack_sift(ptr);
}
extern "C" void __asan_load8_noabort(void * ptr) {
    do_stack_sift(ptr);
}
extern "C" void __asan_load4_noabort(void * ptr) {
    do_stack_sift(ptr);
}
extern "C" void __asan_load2_noabort(void * ptr) {
    do_stack_sift(ptr);
}
extern "C" void __asan_load1_noabort(void * ptr) {
    do_stack_sift(ptr);
}
*/
extern "C" char mmap_test(char * ptr);

char test_string[] = "[[this is normal memory]]\n";

#include "doom_iwad.h"

extern const unsigned char doom_iwad_builtin[4676420UL];

extern "C" void init_wad() {
    my_mmap_init();


    listdir("/sd", 0);

/*
    FILE * file = fopen("/sd/big_file", "r");
    char * mmaped_file = my_mmap(file, 0, 1000);

    printf("call mmap_test with ptr %p\n", test_string);
    mmap_test(test_string);
    printf("call mmap_test with ptr %p\n", mmaped_file);
    mmap_test(mmaped_file);
*/
#if 0
    doom_iwad = (unsigned char*)doom_iwad_builtin;
    doom_iwad_len = 4676420UL;
#else
    FILE * doom_wad_file = fopen("/sd/gdoom2.wad", "r");
    fseek(doom_wad_file, 0L, SEEK_END);
    doom_iwad_len = ftell(doom_wad_file);

    printf("WAD size: %d\n", doom_iwad_len);

    rewind(doom_wad_file);

    doom_iwad = (unsigned char *)my_mmap(doom_wad_file);
#endif
}


extern "C" void app_main(void)
{
    printf("Init SD\n");
    if(!init_sd() == ESP_OK) {
        printf("SD Init Failed!\n");
    }
    delay(500);
    init_wad();
    mkdir("/sd/doom", 0775);
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



/*
    printf("List root:\n");

    listdir("/sd", 0);
*/

/*
    printf("Write file\n");
    FILE * f = fopen("/sd/testfile", "w");
    fwrite("test text", 10, 1, f);
    fclose(f);
    listdir("/sd", 0);
*/
    printf("Launch DOOM\n");
    doom_main(0,0);

}

/*
extern "C" void app_main(void)
{
    printf("Init SD\n");
    if(!init_sd() == ESP_OK) {
        printf("SD Init Failed!\n");
    }
    delay(500);
    printf("List root:\n");
    listdir("/sd", 0);
    my_mmap_init();
    FILE * file = fopen("/sd/big_file", "r");
    char * mmaped_file = my_mmap(file);

    printf("call mmap_test with ptr %p\n", test_string);
    mmap_test(test_string);
    printf("call mmap_test with ptr %p\n", mmaped_file);
    mmap_test(mmaped_file);
    while(1);
}
*/