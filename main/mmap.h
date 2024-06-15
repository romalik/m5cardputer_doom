#ifdef __cplusplus
extern "C" {
#endif

#ifdef X86
#include <stdio.h>
#define F_FILE FILE
#else
#undef FILE
#include "fat.h"
#endif

extern unsigned int cluster_size;
extern unsigned int sectors_per_cluster;
extern unsigned int * start_sect_cache;

#define MMAP_COLLECT_STATISTICS 0
#define MAX_MMAP_FILE_SIZE         16*1024*1024U

#define MMAP_PAGE_CHUNK_SHIFT      9
#define MMAP_PAGE_SIZE             (1UL<<(MMAP_PAGE_CHUNK_SHIFT))
#define MMAP_PAGE_POSITION_MASK    (MMAP_PAGE_SIZE-1)

//#define MMAP_ARENA_N_PAGES (120000/MMAP_PAGE_SIZE)
#define MMAP_ARENA_N_PAGES 210

typedef struct mmap_page {
    char * data;
    unsigned int chunk_idx; //merge file id and chunk idx
    unsigned int ttl;
    char file_id;
    unsigned char idx;
} mmap_page_t;


#define STATIC_CACHE_REGIONS_SIZE 5
//expected to be 512 page aligned
typedef struct static_cache_region {
    char * reg_begin;
    char * reg_end;
    char * data;
} static_cache_region_t;




#if MMAP_COLLECT_STATISTICS

#endif


extern static_cache_region_t    static_cache[STATIC_CACHE_REGIONS_SIZE];
extern mmap_page_t              mmap_pages[MMAP_ARENA_N_PAGES];
extern char                     mmap_arena[MMAP_ARENA_N_PAGES * MMAP_PAGE_SIZE];

extern F_FILE * mmaped_files[16];


void my_mmap_init();
char * my_mmap(F_FILE * fd);

void my_unmmap(FILE * fd);

static_cache_region_t * register_static_cache_region(char * reg_begin, char * reg_end, char * data);

mmap_page_t * get_mmap_page_for_id_and_offset(int id, unsigned int chunk_idx);


mmap_page_t * get_page_to_swap_out();

void swap_in_page(mmap_page_t * page, int file_id, unsigned int chunk_idx, unsigned int offset);

#if MMAP_COLLECT_STATISTICS
void dump_statistics();
#endif


unsigned int get_ptr_to_buffer(void * ptr);

void * __remap_ptr(void * ptr, int is_store, unsigned int size, unsigned int align);



#ifdef __cplusplus
}
#endif
