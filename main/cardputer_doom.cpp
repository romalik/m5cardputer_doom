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
#include <sys/stat.h>
#include <sys/types.h>
#include "mmap.h"
#include "spi_cache.h"


#undef FILE
#include "hal/display/hal_display.hpp"
#include "fat.h"




unsigned short * __sprite_data;

LGFX_Sprite * doom_canvas;

bool scale = true;

extern int new_frame;
extern int frame_n;

extern "C" void __update_sprite() {
    new_frame = 1;
    frame_n++;
    if(scale) {
        doom_canvas->pushRotateZoom(240.0f/2.0f,135.0f/2.0f,0.0f,1.0f,(135.0f/160.0f));
    } else {
        doom_canvas->pushSprite(0,0);
    }
        
}

extern "C" void __set_sprite_pallete(unsigned int i, unsigned char r, unsigned char g, unsigned char b) {
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
#include <driver/sdspi_host.h>

//#include <esp_vfs_fat.h>

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
#include "sdmmc_cmd.h"
#include "driver/sdmmc_defs.h"

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

    int card_handle = -1;
    sdmmc_card_t* card = (sdmmc_card_t*)malloc(sizeof(sdmmc_card_t));
    err = (*host_config.init)();
    err = sdspi_host_init_device((const sdspi_device_config_t*)&slot_config, &card_handle);

    if (card_handle != host_config.slot) {
        host_config.slot = card_handle;
    }

    err = sdmmc_card_init(&host_config, card);

    printf("SD card sector size: %d\n" , card->csd.sector_size);

    char return_code = FAT_mountVolume(card);
    printf("FAT_mountVolume retval : %d\n", return_code);

    char label[300];
    uint32_t vol_sn;
    FAT_getLabel(label, &vol_sn);

    printf("FAT_getLabel returns: %s 0x%08X\n", label, vol_sn);

/*
    
    esp_vfs_fat_mount_config_t mount_config = {.max_files = 8};

    err = esp_vfs_fat_sdspi_mount(RG_STORAGE_ROOT, &host_config, &slot_config, &mount_config, NULL);
    if (err == ESP_ERR_TIMEOUT || err == ESP_ERR_INVALID_RESPONSE || err == ESP_ERR_INVALID_CRC)
    {
        printf("SD Card mounting failed (0x%x), retrying at lower speed...\n", err);
        host_config.max_freq_khz = SDMMC_FREQ_PROBING;
        err = esp_vfs_fat_sdspi_mount(RG_STORAGE_ROOT, &host_config, &slot_config, &mount_config, NULL);
    }
    */
    return err;
}






#include "doom_iwad.h"

extern const unsigned char doom_iwad_builtin[4676420UL];


int check_fragmentation(F_FILE * file) {
    printf("Check fragmentation\n");
    cluster_size = FAT_get_cluster_size();
    sectors_per_cluster = FAT_get_sectors_per_cluster();
    printf("Cluster size: %d\n", cluster_size);

    unsigned int prev = 0;
    unsigned int cnt = 0;
    for(unsigned int p = 0; p < file->file_size; p += cluster_size) {
        unsigned int start_sect = file->file_start_sector;
        printf("Cluster at %d\n", start_sect);
        if(cnt) {
            if(start_sect != prev + sectors_per_cluster) {
                printf("Fragmentation!\n");
                return 1;
            }
        }
        int r = 0;
        if((r = _FAT_nextFileCluster(file))) {
            printf("_FAT_nextFileCluster() -> %d\n", r);
            //while(1) {}
        }
        
        
        cnt++;
        prev = start_sect;
    }
    _FAT_freset(file);
    return 0;
}

extern "C" void setup_first_cluster(F_FILE * file) {
    cluster_size = FAT_get_cluster_size();
    sectors_per_cluster = FAT_get_sectors_per_cluster();
    start_sector = file->file_start_sector;
}


extern "C" void do_defragmentation(F_FILE * file) {
    (void)file;
}

/*
extern "C" void cache_clusters(F_FILE * file) {
    printf("Cache clusters\n");
    cluster_size = FAT_get_cluster_size();
    sectors_per_cluster = FAT_get_sectors_per_cluster();
    printf("Cluster size: %d\n", cluster_size);

    start_sect_cache = (unsigned int *)malloc((1 + file->file_size / cluster_size) * sizeof(unsigned int));

    unsigned int cnt = 0;
    for(unsigned int p = 0; p < file->file_size; p += cluster_size) {
        unsigned int start_sect = file->file_start_sector;
        printf("%d : %d\n", p, start_sect);
        start_sect_cache[cnt] = start_sect;

        int r = 0;
        if((r = _FAT_nextFileCluster(file))) {
            printf("_FAT_nextFileCluster() -> %d\n", r);
            //while(1) {}
        }

        cnt++;
    }
    printf("Cache size: %d entries\n", cnt);
    _FAT_freset(file);
}
*/

#define CWD_LEN 100
DIR cwd;
char cwd_name[CWD_LEN];




void memcheck(char * tag) {
    unsigned int heapSize = 512000;
    void * ptr = NULL;
    //We can now alloc all of the rest fo the memory.
    do
    {
        ptr = malloc(heapSize);
        heapSize -= 4;

    } while(ptr == NULL);

    free(ptr);
    printf("Heapsize at %s is %d bytes\n", tag, heapSize);
}

#define FONT_HEIGHT 16
#define FONT_WIDTH  8

int gui_select_from_list(const char ** header, int n_header, char ** list, int n) {
    int c_select = 0;
    int top = 0;
    int n_lines = (135/FONT_HEIGHT)-1 - n_header;

    doom_canvas->setFont(&AsciiFont8x16);

    while(1) {
        doom_canvas->clear();
        // draw list header
        for (int i = 0; i< n_header; i++) {
            doom_canvas->setCursor(0, i*FONT_HEIGHT);
            doom_canvas->print(header[i]);
        }

        // draw list subset
        for(int i = top; i < n; i++) {
            if(i - top > n_lines) {
                break;
            }

            if(i == c_select) {
                doom_canvas->setCursor(0, (i-top + n_header) * FONT_HEIGHT); //+1 for header
                doom_canvas->print(">");
            }

            printf("Try print i %d\n", i);
            printf("Print %s\n", list[i]);
            doom_canvas->setCursor(FONT_WIDTH * 2, (i-top + n_header) * FONT_HEIGHT); //+1 for header
            doom_canvas->print(list[i]);
        }
        
        doom_canvas->pushSprite(0, 0);

        //wait for input
        while(1){
            check_evts();
            unsigned char evt = __get_event();
            if(evt) {
                if(evt&0x80) {
                    //btn press
                } else {
                    //btn release
                    char btn = evt & 0x7f;
                    if(btn == '.') {
                        c_select++;
                        break;
                    } else if(btn == ';') {
                        c_select--;
                        break;
                    } else if(btn == K_ENT) {
                        return c_select;
                    }
                }
            }
        }

        if(c_select < 0)  c_select = n-1;
        if(c_select >= n) c_select = 0;

        if(c_select < top) {
            top = c_select;
        }
        if(c_select > top + n_lines) {
            top = c_select - n_lines;
        }

    }
}


void get_dir_list(char *** list, char ** is_dir, int * n) {


    F_FILE file;	// file object


    *n = 0;


    printf("Opened dir %s\n", FAT_getFilename());
                
    // Get number of folders and files inside the directory
    int dirItems = FAT_dirCountItems(&cwd) + 1;

    if(dirItems) {
        *list = (char **)malloc(dirItems * sizeof(char *));
        *is_dir = (char *)malloc(dirItems * sizeof(char));
        *n = dirItems;
    }
    (*list)[0] = (char *)malloc(3 * sizeof(char));
    strcpy((*list)[0], "..");
    (*is_dir)[0] = 1;

    printf("Folder has %d items:\n", dirItems);
    // Print folder content
    for(uint16_t i = 1; i < dirItems; i++){
        FAT_findNext(&cwd, &file);

        int fname_len = strlen(FAT_getFilename());

        printf("Idx: %d , %s\n", FAT_getItemIndex(&cwd), FAT_getFilename());
        (*list)[i] = (char *)malloc(fname_len + 2);
        strcpy((*list)[i], FAT_getFilename());


        if(FAT_attrIsFolder(&file)){
            printf("D, "); // Directory
            (*is_dir)[i] = 1;

            (*list)[i][fname_len] = '/';
            (*list)[i][fname_len + 1] = 0;

        }else{
            printf("F, "); // File
            (*is_dir)[i] = 0;
        }
        printf("Recorded: %d/%d %s\n", i, *n, (*list)[i]);
    }

}

/*
int navigate_to_folder(char * folder) {

    char * f_start = folder;
    char * f_end = folder;
    char * str_end = folder + strlen(folder);
    while(f_start != str_end) {
        if(*f_start == '/') {
            f_start++;
        }
        f_end = f_start;
        while(*f_end) {
            if(*f_end == '/') *f_end = 0;
            f_end++;
        }

        if(f_end != f_start) {
            printf("cd %s\n", f_start);
            int retval = FAT_openDir(&cwd, f_start);
            strncat(cwd_name, f_start, 100-strlen(cwd_name));
        }
    }
}
*/

void select_wad_path(F_FILE * wad_file, char * name) {
    char cached_path[CWD_LEN];
    int cached_path_success = 0;
    if(get_cached_path(cached_path, name)) {
        printf(" >>> Cached path : %s %s\n", cached_path, name);
        int rval = FAT_openDir(&cwd, cached_path);
        if(rval == FR_OK) {
            strcpy(cwd_name, cached_path);
            cached_path_success = 1;
        }
        
    } else {
        printf(" >>> No cached WAD\n");

    }

    int cached_selected = 0;
    // Continue with cached file dialog
    if(cached_path_success) {
        const char * gui_header[3];
        gui_header[0] = "    === Use previous? ===";
        gui_header[1] = cwd_name;
        gui_header[2] = name;

        char * select_list[2];
        select_list[0] = "Yes";
        select_list[1] = "No";

        int sel = gui_select_from_list(gui_header, 3, select_list, 2);
        if(sel == 0) {
            cached_selected = 1;
        }
    }

    // Select WAD file
    if(!cached_selected) {
        const char * gui_header[2];
        gui_header[0] = "     === Select WAD ===";

        int selected = 0;

        while(!selected) {
            printf("cwd: %s\n", cwd_name);
            gui_header[1] = cwd_name;

            char ** select_list = NULL;
            char * is_dir = NULL;
            int n_files = 0;

            get_dir_list(&select_list, &is_dir, &n_files);

            for(int i = 0; i<n_files; i++) {
                printf("%d : %s\n", i, select_list[i]);
            }

            int sel = gui_select_from_list(gui_header, 2, select_list, n_files);

            if(is_dir[sel]) {
                if(sel == 0) {
                    FAT_dirBack(&cwd);
                    if(strlen(cwd_name) > 1) {
                        int n = strlen(cwd_name) - 2;
                        while(n >= 0) {
                            if(cwd_name[n] == '/') {
                                cwd_name[n+1] = 0;
                                break;
                            }
                            n--;
                        }
                    }
                } else {
                    strncat(cwd_name, select_list[sel], 100-strlen(cwd_name));
                    FAT_openDir(&cwd, cwd_name);
                }
                
            } else {
                strcpy(name, select_list[sel]);

                selected = 1;
            }



            for(int i = 0; i<n_files; i++) {
                free(select_list[i]);
            }
            free(select_list);
            free(is_dir);
        }
    }

    printf("Selected %s %s\n", cwd_name, name);

    int retval = FAT_fopen(&cwd, wad_file, name);

    if(retval != FR_OK) {
        printf("fopen failed!\n");
    }

    //while(1) {}

}

void canvas_init() {
    LGFX_Cardputer * _display = new LGFX_Cardputer;
    _display->init();
    _display->setRotation(1);
    doom_canvas = new LGFX_Sprite(_display);
    doom_canvas->setColorDepth(lgfx::color_depth_t::palette_8bit);
    doom_canvas->createPalette();
    doom_canvas->createSprite(240,160);
    __sprite_data = (unsigned short *)doom_canvas->getBuffer();
}

extern "C" void init_wad(F_FILE * doom_wad_file, char * wad_name) {



#if 0
    doom_iwad = (unsigned char*)doom_iwad_builtin;
    doom_iwad_len = sizeof(doom_iwad_builtin);
#else



    doom_iwad_len = doom_wad_file->file_size;

    printf("WAD size: %d\n", doom_iwad_len);

    int is_fragmented = check_fragmentation(doom_wad_file);


    if(is_fragmented) {
        const char * gui_header[5];
        char * select_list[1];

        gui_header[0] = "  ======== ERROR ======== ";
        gui_header[1] = "File fragmented";
        gui_header[2] = "Defragmentation";
        gui_header[3] = "not implemented yet";
        gui_header[4] = "Try to reupload the WAD file ";

        select_list[0] = "OK";

        gui_select_from_list(gui_header, 5, select_list, 1);
        esp_restart();
    }

    setup_first_cluster(doom_wad_file);


    //cache_clusters(doom_wad_file);



    //rewind(doom_wad_file);

    doom_iwad = (unsigned char *)my_mmap(doom_wad_file);

    init_and_maybe_rebuild_cache(cwd_name, wad_name);

#endif
}
extern "C" void app_main(void)
{
    F_FILE wad_file;
    printf("MMAP_PAGE_SIZE: %lu\n", MMAP_PAGE_SIZE);
    memcheck("Start");
    printf("Init SD\n");
    if(!init_sd() == ESP_OK) {
        printf("SD Init Failed!\n");
    }
    delay(500);


    FAT_openDir(&cwd, "/");
    strcpy(cwd_name, "/");
    
    init_spi_flash_cache_partition();
    my_mmap_init();
    kb_init(kb_output_list, kb_input_list);
    canvas_init();
    
 
    char wad_name[32];

    select_wad_path(&wad_file, wad_name);

    init_wad(&wad_file, wad_name);


    printf("Launch DOOM\n");
    doom_main(0,0);

}
