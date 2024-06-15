int putchar(int);

char mmap_test(char * ptr) {
    while(*ptr) {
        putchar(*ptr);
        ptr++;
    }
    return 0;
}
__attribute__((no_instrument_function)) char mmap_nowrap_test(char * ptr);

char mmap_nowrap_test(char * ptr) {
    while(*ptr) {
        putchar(*ptr);
        ptr++;
    }
    return 0;
}