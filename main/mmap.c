#include "mmap.h"
#include <stddef.h>


int64_t xx_timestamp_us() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (((uint64_t)tv.tv_sec) * 1000000LL + (uint64_t)(tv.tv_usec));
}

unsigned int cluster_size = 0;
unsigned int sectors_per_cluster = 0;
unsigned int * start_sect_cache = NULL;



#if MMAP_COLLECT_STATISTICS
#define N_PAGE_LOG 1000
unsigned int page_log[N_PAGE_LOG];
unsigned int page_log_sz = 0;
#endif


mmap_page_t  mmap_pages[MMAP_ARENA_N_PAGES];
char         mmap_arena[MMAP_ARENA_N_PAGES * MMAP_PAGE_SIZE] __attribute__((aligned(MMAP_PAGE_SIZE)));

F_FILE * mmaped_files[16] = {NULL};

void my_mmap_init() {
    printf("mmap page size %lu\n", MMAP_PAGE_SIZE);
    assert(MMAP_PAGE_SIZE == 512);
    
    printf("mmap_arena ptr %p\n", mmap_arena);
    assert(((unsigned int)mmap_arena & MMAP_PAGE_POSITION_MASK) == 0);

    memset(mmap_pages, 0, sizeof(mmap_page_t) * MMAP_ARENA_N_PAGES);
    for(int i = 0; i<MMAP_ARENA_N_PAGES; i++) {
        mmap_pages[i].file_id = -1;
        mmap_pages[i].ttl = 0;
        mmap_pages[i].data = &mmap_arena[i * MMAP_PAGE_SIZE];
        mmap_pages[i].idx = i;
        //printf("mmap_arena[%d]: %p\n", i, &mmap_arena[i]);
    }

#if MMAP_COLLECT_STATISTICS
    memset(page_log, 0, N_PAGE_LOG * sizeof(unsigned int));
    page_log_sz = 0;
#endif

}

#if MMAP_COLLECT_STATISTICS
void add_page_to_list(unsigned int page) {

    if(page_log_sz<N_PAGE_LOG) {
        page_log[page_log_sz] = page;
        page_log_sz+=2;
    }
}
int find_page_in_log(unsigned int page) {
    for(int i = 0; i<N_PAGE_LOG; i+=2) {
        if(page_log[i] == page) {
            return i;
        }
    }
    return -1;
}
#endif


char * my_mmap(F_FILE * fd) {
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

void my_unmmap(FILE * fd) {
    for(int i = 0; i<16; i++) {
        if(mmaped_files[i] == fd) {
            mmaped_files[i] = NULL;
            return;
        }
    }
}
mmap_page_t * prev_page = mmap_pages;
mmap_page_t * before_prev_page = mmap_pages;

mmap_page_t * get_mmap_page_for_id_and_offset(int id, unsigned int chunk_idx) {
    //printf("get_mmap_page_for_id_and_offset(%d, %d)\n", id, chunk_idx);
    if(prev_page->file_id == id && prev_page->chunk_idx == chunk_idx) {
        return prev_page;
    }

    if(before_prev_page->file_id == id && before_prev_page->chunk_idx == chunk_idx) {
        return before_prev_page;
    }

    mmap_page_t * ret = NULL;

    for(int i = 0; i<MMAP_ARENA_N_PAGES; i++) {
        if(mmap_pages[i].file_id == id && mmap_pages[i].chunk_idx == chunk_idx) {
            //printf("get_mmap_page_for_id_and_offset() -> %p\n", &mmap_arena[i]);
            before_prev_page = prev_page;
            prev_page = &mmap_pages[i];
            ret = &mmap_pages[i];
            ret->ttl = 0xffffffff;
        } else {
            if(mmap_pages[i].ttl) mmap_pages[i].ttl--;
        }
    }

    return ret;
}


mmap_page_t * get_page_to_swap_out() {
    mmap_page_t * min_ttl_page = mmap_pages;
    for(int i = 1; i<MMAP_ARENA_N_PAGES; i++) {
        if(mmap_pages[i].ttl < min_ttl_page->ttl) min_ttl_page = &mmap_pages[i];
    }

    //printf("Swap out ttl 0x%08x\n", min_ttl_page->ttl);
    return min_ttl_page;
}

void swap_in_page(mmap_page_t * page, int file_id, unsigned int chunk_idx, unsigned int offset) {


    
    unsigned int cluster_idx = chunk_idx / sectors_per_cluster;

    unsigned int start_sector = start_sect_cache[cluster_idx];
    unsigned int sector_to_read = start_sector + (chunk_idx % sectors_per_cluster);


    
    int retval = sd_read_single_block(sector_to_read, (uint8_t *)page->data);
    

    page->chunk_idx = chunk_idx;
    page->file_id = file_id;
    page->ttl = 0xffffffff;

}


#if MMAP_COLLECT_STATISTICS
void dump_statistics() {
}

#endif

int n_swaps = 0;

unsigned int get_ptr_to_buffer(void * ptr) {
    int file_id = (((unsigned int)ptr & 0x0f000000) >> (8*3));
    unsigned int offset = (unsigned int)ptr & 0x00ffffff;
    unsigned int chunk_idx = offset >> MMAP_PAGE_CHUNK_SHIFT;

    //printf("get_ptr_to_buffer(%p)\n", ptr);
    


    mmap_page_t * page = get_mmap_page_for_id_and_offset(file_id, chunk_idx);


    //int retval = FAT_read_file_sector(mmaped_files[file_id], chunk_idx, page->data);
    if(page == NULL) {
        n_swaps++;
        page = get_page_to_swap_out();
        //printf("-> [%d](%d)\n", page - mmap_arena, page->chunk_idx);
        //printf("get_ptr_to_buffer() swapout %d\n", p_idx);
        swap_in_page(page, file_id, chunk_idx, offset);

        //printf("<- [%d](%d)\n", page - mmap_arena, page->chunk_idx);
    }
    //printf("get_ptr_to_buffer() -> %lu\n", (unsigned int)(page->data) + (offset & MMAP_PAGE_POSITION_MASK));

    return (unsigned int)(page->data) | (offset & MMAP_PAGE_POSITION_MASK);
}

int new_frame = 0;

int frame_n = 0;

int avg_remap = 0;
int max_remap = 0;
int min_remap = 100000;

int n_fast = 0;

int n_remap   = 0;

unsigned int read_chunk_idx = 0;

#define MMAP_PAGE_POSITION_MASK_INV (~MMAP_PAGE_POSITION_MASK)

unsigned int prev_ptr = 0;
unsigned int prev_result = 0;

void * __remap_ptr(void * ptr, int is_store, unsigned int size, unsigned int align) {
    //printf("__remap_ptr check\n");
    if(((unsigned int)ptr & 0x80000000)) {
        unsigned int val;
#if MMAP_COLLECT_STATISTICS
        uint64_t start_search = xx_timestamp_us();
#endif

        if((prev_ptr & MMAP_PAGE_POSITION_MASK_INV) == (((unsigned int)ptr) & MMAP_PAGE_POSITION_MASK_INV)) {
            val = ((prev_result & MMAP_PAGE_POSITION_MASK_INV) | (((unsigned int)ptr) & MMAP_PAGE_POSITION_MASK));
            n_fast++;
        } else {
            val = get_ptr_to_buffer(ptr);
            read_chunk_idx = ((unsigned int)ptr & 0x00ffffff) >> MMAP_PAGE_CHUNK_SHIFT;
        }
        prev_ptr = (unsigned int)ptr;
        prev_result = val;

#if MMAP_COLLECT_STATISTICS
        uint64_t end_search = xx_timestamp_us();

        int remap_time = (int)(end_search - start_search);

        avg_remap = (int)(((uint64_t)remap_time + ((uint64_t)n_remap) * ((uint64_t)avg_remap)) / ((uint64_t)n_remap + 1LL));

        n_remap++;
        if(remap_time > max_remap) {
            max_remap = remap_time;
        }
        if(remap_time < min_remap) {
            min_remap = remap_time;
        }

        static int log_idx = 0;
        if(page_log[log_idx] != read_chunk_idx) {
            log_idx = find_page_in_log(read_chunk_idx);
        }

        if(log_idx >= 0) {
            page_log[log_idx + 1]++;
        } else {
            log_idx = page_log_sz;
            add_page_to_list(read_chunk_idx);
            page_log[page_log_sz - 1] = 1;
        }

        if(page_log_sz == N_PAGE_LOG || new_frame) {
            printf("Frame %d Remap stats: avg %d min %d max %d n_swaps %d n_fast %d n %d\n", frame_n, avg_remap, min_remap, max_remap, n_swaps, n_fast, n_remap);
            printf("ACCESS_AT %d ", frame_n);
            for(int i = 0; i<page_log_sz; i++) {
                printf("%u ", page_log[i]);
            }
            printf("\n");
            n_remap = 0;
            avg_remap = 0;
            max_remap = 0;
            min_remap = 100000;
            n_swaps = 0;
            n_fast = 0;
            new_frame = 0;

            page_log_sz = 0;
            memset(page_log, 0, N_PAGE_LOG * sizeof(unsigned int));
        }
#endif

        //printf("\n---\nTrap! %p replace with 0x%08X\n", ptr, val);
        return (void*)val;            
    } else {
        //printf("\n---\nNo trap! %p\n", ptr);
        return ptr;
    }
}
