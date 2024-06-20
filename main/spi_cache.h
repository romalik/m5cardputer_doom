#ifdef __cplusplus
extern "C" {
#endif


#ifndef SPI_CACHE_H__
#define SPI_CACHE_H__

#include "mmap.h"


typedef struct
{
  int  filepos;
  int  size;
  char name[8];
} filelump_t;


extern unsigned char * doom_iwad;
extern unsigned int doom_iwad_len;


#define APP_SIZE 2UL*1024UL*1024UL      //we are fine while size of app is less than 1M
#define FLASH_SIZE 8UL*1024UL*1024UL    //SPI Flash size 8 Mb


static_cache_region_t * cache_region_to_flash(char * data, unsigned int sz);
void init_and_maybe_rebuild_cache(char * wad_path, char * wad_name);

int check_cache_crc32();
int get_cached_path(char * path, char * name);
void init_spi_flash_cache_partition();

#endif

#ifdef __cplusplus
}
#endif
