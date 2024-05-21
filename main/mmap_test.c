char mmap_test(char * ptr) {
    //while(*ptr) {
        putchar(*(char*)((unsigned int)ptr | 0x10000000));
    //}
    return 0;
}
