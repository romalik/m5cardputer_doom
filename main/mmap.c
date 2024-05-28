#include "mmap.h"
#include <stddef.h>




unsigned int cluster_size = 0;
unsigned int sectors_per_cluster = 0;
unsigned int * start_sect_cache = NULL;



#if MMAP_COLLECT_STATISTICS

unsigned int page_statistics[N_PAGES];
#endif


mmap_page_t  mmap_arena[MMAP_ARENA_N_PAGES];

F_FILE * mmaped_files[16] = {NULL};

void my_mmap_init() {
    memset(mmap_arena, 0, sizeof(mmap_page_t) * MMAP_ARENA_N_PAGES);
    for(int i = 0; i<MMAP_ARENA_N_PAGES; i++) {
        mmap_arena[i].file_id = -1;
        mmap_arena[i].ttl = 0;
        //printf("mmap_arena[%d]: %p\n", i, &mmap_arena[i]);
    }

#if MMAP_COLLECT_STATISTICS
    memset(page_statistics, 0, MMAP_PAGE_SIZE*sizeof(unsigned int));
#endif

}

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
mmap_page_t * prev_page = mmap_arena;
mmap_page_t * before_prev_page = mmap_arena;

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


mmap_page_t * get_page_to_swap_out() {
    mmap_page_t * min_ttl_page = mmap_arena;
    for(int i = 1; i<MMAP_ARENA_N_PAGES; i++) {
        if(mmap_arena[i].ttl < min_ttl_page->ttl) min_ttl_page = &mmap_arena[i];
    }

    //printf("Swap out ttl 0x%08x\n", min_ttl_page->ttl);
    return min_ttl_page;
}

void swap_in_page(mmap_page_t * page, int file_id, unsigned int chunk_idx, unsigned int offset) {
    //printf("\n\n---\nswap_in_page(%d, %d, %d, %d)\n---\n", page_idx, file_id, chunk_idx, offset);
    
    /*
    fseek(mmaped_files[file_id], chunk_idx<<MMAP_PAGE_CHUNK_SHIFT, SEEK_SET);
    fread(page->data, MMAP_PAGE_SIZE, 1, mmaped_files[file_id]);
    */

    assert(MMAP_PAGE_SIZE == 512);
    
    unsigned int cluster_idx = chunk_idx / sectors_per_cluster;

    unsigned int start_sector = start_sect_cache[cluster_idx];
    unsigned int sector_to_read = start_sector + (chunk_idx % sectors_per_cluster);

#ifdef X86
    fseek(mmaped_files[file_id], chunk_idx << MMAP_PAGE_CHUNK_SHIFT, SEEK_SET);
    fread(page->data, 512, 1, mmaped_files[file_id]);
#else
    int retval = sd_read_single_block(sector_to_read, (uint8_t *)page->data);
    //int retval = FAT_read_file_sector(mmaped_files[file_id], chunk_idx, page->data);
#endif    
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


#if MMAP_COLLECT_STATISTICS
void dump_statistics() {
    DIR dir;
    F_FILE file;

    char buf[128];

    //FAT_openDir(&dir, "/");
    //FAT_fopen(&dir, &file, "acc_log.txt");


    printf("Writing statistics:\n");
    unsigned int max_reads = 0;
    for(int i = 0; i<N_PAGES; i++) {
        printf("page %d %d\n", i, page_statistics[i]);
        //FAT_fwriteString(&file, buf);
    } 
    //FAT_fsync(&file);

/*
    for(int i = 0; i<MMAP_PAGE_SIZE; i+= MMAP_PAGE_SIZE / 50) {
        unsigned int bin_avg = 0;
        for(int j = 0; j<MMAP_PAGE_SIZE / 50; j++) {
            bin_avg += read_statistics[i+j] / (MMAP_PAGE_SIZE / 50);
        }
        printf("[0x%08X - 0x%08X]: (%08d) :", i, (unsigned int)(i+MMAP_PAGE_SIZE/50), bin_avg);
        int n_stars = 100 * bin_avg / max_reads;
        for(int j = 0; j<n_stars; j++) {
            printf("*");
        }
        printf("\n");
    }
*/

    printf("Done\n");

    while(1) {}
}

#endif


unsigned int get_ptr_to_buffer(void * ptr) {
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

#if MMAP_COLLECT_STATISTICS
    page_statistics[offset / MMAP_PAGE_SIZE]++;
    static unsigned int read_n = 0;
    read_n++;
    if(read_n % 0x00100000 == 0) {
        printf("******************** read_n 0x%08X\n", read_n);

    }
    if(read_n == 0x00800000) {
        dump_statistics();
    }

#endif
    return (unsigned int)(page->data) + (offset & MMAP_PAGE_POSITION_MASK);
}




void * __remap_ptr(void * ptr, int is_store, unsigned int size, unsigned int align) {
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
