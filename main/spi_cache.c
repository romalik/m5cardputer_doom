#include "spi_cache.h"

void print_partition_info(const esp_partition_t * part) {
    if(!part) {
        printf("--NULL--\n");
        return;
    }

    char * p_type;
    if(part->type == ESP_PARTITION_TYPE_APP) { p_type = "APP"; }
    else if(part->type == ESP_PARTITION_TYPE_DATA) { p_type = "DAT"; }
    else { p_type = "???"; }
    printf("Partition '%s' [%s] 0x%08X sz %u\n", part->label, p_type, part->address, part->size);
}

esp_partition_t * cache_partition;
const void *flash_cache_ptr;
spi_flash_mmap_handle_t flash_cache_handle;
unsigned int cache_write_position = 512; //leave space for cache header



void init_spi_flash_cache() {
    esp_partition_iterator_t p_it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

    const esp_partition_t * last_app_partition = NULL; 

    while(p_it) {
        const esp_partition_t * part = esp_partition_get(p_it);
        

        if(part->type == ESP_PARTITION_TYPE_APP) { 
            last_app_partition = part;
        }

        print_partition_info(part);
        
        p_it = esp_partition_next(p_it);
    }

    printf("app partition:\n");
    print_partition_info(last_app_partition);

    cache_partition = (esp_partition_t *)malloc(sizeof(esp_partition_t));

    memcpy(cache_partition, last_app_partition, sizeof(esp_partition_t));
    cache_partition->address += APP_SIZE;
    cache_partition->size = FLASH_SIZE - cache_partition->address; //use up all remaining space, bye-bye vfat and spiffs!
    cache_partition->type = ESP_PARTITION_TYPE_DATA;
    strcpy(cache_partition->label, "cache");


    
    printf("cache partition:\n");
    print_partition_info(cache_partition);


    esp_partition_iterator_release(p_it);


    char header[] = "WADCACHE";



    printf("mmap partition\n");
    esp_partition_mmap(cache_partition, 0, cache_partition->size, SPI_FLASH_MMAP_DATA, &flash_cache_ptr, &flash_cache_handle);

    printf("mmaped as %p\n", flash_cache_ptr);
}


void erase_cache_partition() {
        printf("Erase cache partition..\n");

        esp_err_t r = esp_partition_erase_range(cache_partition, 0, cache_partition->size);
        if(r != ESP_OK) {
            printf("erase range fail %d\n", r);
            while(1) {}
        }
}




static_cache_region_t * cache_region_to_flash(char * data, unsigned int sz) {
    char * reg_begin = data;
    char * reg_end = data + sz;
    char * reg_data = (char *)flash_cache_ptr + cache_write_position;

    //only aligned 512-byte blocks
    assert(sz%512 == 0);
    assert(((unsigned int)data) % 512 == 0);

    char tmpbuf[MMAP_PAGE_SIZE];
    while(sz) {
        unsigned int read_now = sz;
        if(read_now > MMAP_PAGE_SIZE) {
            read_now = MMAP_PAGE_SIZE;
        }
        memcpy(tmpbuf, data, read_now);
        esp_partition_write(cache_partition, cache_write_position, tmpbuf, read_now);

        data += read_now;
        cache_write_position += MMAP_PAGE_SIZE; //roundup on last read even if read now < MMAP_PAGE_SIZE
        sz -= read_now;
    }

    return register_static_cache_region(reg_begin, reg_end, reg_data);

}


static const unsigned int crc32_table[] =
{
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
  0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
  0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
  0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
  0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
  0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
  0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
  0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
  0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
  0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
  0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
  0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
  0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
  0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
  0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
  0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
  0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
  0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
  0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
  0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
  0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
  0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
  0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
  0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
  0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
  0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
  0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
  0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
  0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
  0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
  0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
  0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
  0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
  0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
  0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};



unsigned int
xcrc32 (const char *buf, int len, unsigned int init)
{
  unsigned int crc = init;
  while (len--)
    {
      crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
      buf++;
    }
  return crc;
}


/*
cache header:

cached_length   0   4
cached_crc32    4   4
cached_name     8   32
n_regions       40  4
regions         44  xxx

*/

static_cache_region_t * cache_lumps_between(char * start, char * end) {
    char * f_start_ptr = NULL;
    char * f_end_ptr = NULL;

    char namebuf[9];
    namebuf[8] = 0;

    printf("Cache region [%s - %s]\n", start, end);

    int f_start_num = W_GetNumForName(start);
    printf("start lump:\n");
    //find first actual lump
    while(1) {
        const filelump_t* fl = FindLumpByNum(f_start_num);
        if(!fl) {
            printf("Lump for cache not found\n");
            abort();
        }

        memcpy(namebuf, fl->name, 8);

        if(fl->filepos && fl->size) {
            f_start_ptr = (char *)doom_iwad + fl->filepos;
            printf("select '%.8s'\n", namebuf);
            break;
        }
        printf("skip '%.8s'\n", namebuf);
        f_start_num++;
    }

    int f_end_num = W_GetNumForName(end)+1;
    //find first actual lump
    printf("first next lump:\n");
    while(1) {
        const filelump_t* fl = FindLumpByNum(f_end_num);
        if(!fl) {
            printf("end of file\n");
            f_end_ptr = (char *)doom_iwad + doom_iwad_len;
            break;
        }

        memcpy(namebuf, fl->name, 8);

        if(fl->filepos && fl->size) {
            f_end_ptr = (char *)doom_iwad + fl->filepos;
            printf("select '%.8s'\n", namebuf);
            break;
        }
        printf("skip '%.8s'\n", namebuf);
        f_end_num++;
    }

    //round down
    f_start_ptr = (char *) (  512*(((unsigned int)f_start_ptr)/512)  );

    unsigned int f_size = f_end_ptr - f_start_ptr;
    f_size = 512*(f_size/512) + ((f_size%512)?512:0);

    printf("Region size %d\n", f_size);
    //~~~~ Read WAD chunks here ~~~~
    return cache_region_to_flash(f_start_ptr, f_size);

}

char * wad_regions_to_cache[] = {
    "P_START", "P_END",
    "F_START", "F_END",
    "PLAYPAL", "COLORMAP",
    "TEXTURE1", "PNAMES",
    NULL
};

void init_and_maybe_rebuild_cache(char * wad_path) {
    init_spi_flash_cache();

    unsigned int *cached_length    = (unsigned int *)(flash_cache_ptr);
    unsigned int *cached_crc32     = (unsigned int *)(flash_cache_ptr + 4);
    char         *cached_wad_name  = (char *)        (flash_cache_ptr + 8);
    unsigned int *cached_n_regions = (unsigned int *)(flash_cache_ptr + 40);
    char         *cached_regions   = (char *)        (flash_cache_ptr + 44);
    char         *cached_data      = (char *)        (flash_cache_ptr + 512);


    printf("Cache header: length %u crc32 0x%08x wad_name '%.8s' n_regs %d\n", *cached_length, *cached_crc32, cached_wad_name, *cached_n_regions);


    //check cache validity

    printf("check cache validity\n");
    int need_rebuild_cache = 0;

    if(strncmp(cached_wad_name, wad_path, strlen(wad_path))) {
        printf("wad name mismatch\n");
        need_rebuild_cache = 1;
    } else if(*cached_crc32 != xcrc32(cached_data, *cached_length, 0)) {
        printf("crc32 mismatch\n");
        need_rebuild_cache = 1;
    }

    need_rebuild_cache = 1;


    if(need_rebuild_cache) {
        printf("Rebuilding cache\n");
        erase_cache_partition();


        char header[512];
        memset(header, 0, 512);

        unsigned int cache_length = 0;
        unsigned int cache_crc    = 0;

        char ** w_reg = wad_regions_to_cache;

        int n_regs = 0;
        while(*w_reg) {
            static_cache_region_t * cache_region;        
            cache_region = cache_lumps_between(*w_reg, *(w_reg+1));

            *(unsigned int *)(header + 44 + n_regs*12) = (unsigned int)cache_region->reg_begin;
            *(unsigned int *)(header + 48 + n_regs*12) = (unsigned int)cache_region->reg_end;
            *(unsigned int *)(header + 52 + n_regs*12) = (unsigned int)cache_region->data;

            cache_length += cache_region->reg_end - cache_region->reg_begin;

            n_regs++;
            w_reg += 2;
        }


        cache_crc = xcrc32(cached_data, cache_length, 0);

        printf("Overall cache length %d\n", cache_length);

        
        *(unsigned int *)(header)      = cache_length;
        *(unsigned int *)(header + 4)  = cache_crc;
        strcpy(header + 8, wad_path);

        *(unsigned int *)(header + 40) = n_regs;

        esp_partition_write(cache_partition, 0, header, 512);

    } else {
        printf("Cache valid\n");
        printf("Setup mmap\n");
        for(int i = 0; i < *cached_n_regions; i++) {
            char * reg_begin = (char*) ( *(unsigned int *)(cached_regions + 0 + i*(4*3)) );
            char * reg_end   = (char*) ( *(unsigned int *)(cached_regions + 4 + i*(4*3)) );
            char * reg_data  = (char*) ( *(unsigned int *)(cached_regions + 8 + i*(4*3)) );

            register_static_cache_region(reg_begin, reg_end, reg_data);
        }
    }


}
