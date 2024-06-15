int putchar(int);

char mmap_test(char * ptr) {
    while(*ptr) {
        putchar(*ptr);
        ptr++;
    }
    return 0;
}
