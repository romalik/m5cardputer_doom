int putchar(int);
char mmap_test(char * ptr) {
    int ret = 0;
    int ret2 = 0;
    char * pp;
    * ptr = 123;
    //while(*ptr) {
        ret = putchar(*(char*)((unsigned int)ptr | 0x10000000));

	ret2 = putchar(ptr[3]);

	pp = &ptr[3];

	putchar(*pp);

	putchar(*ptr);
    //}
    return ret;
}
