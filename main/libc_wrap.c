
#include <stddef.h>
#include <stdio.h>


#define IS_MMAP_REGION_PTRS(a,b) (((unsigned int)(a) | (unsigned int)(b)) & 0x80000000)



//int __real_memcmp(const void *s1, const void *s2, size_t n);
int __wrap_memcmp(const void *s1, const void *s2, size_t n)
{
    //if(IS_MMAP_REGION_PTRS(s1, s2)) {
        const unsigned char *c1 = (const unsigned char *)s1, *c2 = (const unsigned char *)s2;
        int d = 0;

        while (n--) {
            d = (int)*c1++ - (int)*c2++;
            if (d)
                break;
        }

        return d;
    //} else {
    //    return __real_memcmp(s1,s2,n);
    //}
}

//void *__real_memcpy(void *dst, const void *src, size_t n);
void *__wrap_memcpy(void *dst, const void *src, size_t n)
{
    //if(IS_MMAP_REGION_PTRS(dst, src)) {
        const char *p = (const char *)src;
        char *q = (char *)dst;

        while (n--) {
            *q++ = *p++;
        }

        return dst;
    //} else {
    //    return __real_memcpy(dst,src,n);
    //}
}

//char *__real_strcpy(char *dst, const char *src);
char *__wrap_strcpy(char *dst, const char *src)
{
    //if(IS_MMAP_REGION_PTRS(dst, src)) {
        char *q = dst;
        const char *p = src;
        char ch;

        do {
            *q++ = ch = *p++;
        } while (ch);

        return dst;
    //} else {
    //    return __real_strcpy(dst,src);
    //}
}


int __wrap_strncmp(const char *s1, const char *s2, size_t n)
{
    //printf("__wrap_strncmp(%p,%p,%d)\n", s1, s2, n);
    //if(IS_MMAP_REGION_PTRS(s1, s2)) {
        //printf("IS MMAP!\n");
        const unsigned char *c1 = (const unsigned char *)s1;
        const unsigned char *c2 = (const unsigned char *)s2;
        unsigned char ch;
        int d = 0;

        while (n--) {
            d = (int)(ch = *c1++) - (int)*c2++;
            if (d || !ch)
                break;
        }

        return d;
    //} else {
    //    return __real_strncmp(s1,s2,n);
    //}
}

void *__wrap_memset(void *dst, int c, size_t n)
{
	char *q = dst;

	while (n--) {
		*q++ = c;
	}

	return dst;
}
char *__wrap_strncpy(char *dst, const char *src, size_t n)
{
	char *q = dst;
	const char *p = src;
	char ch;

	while (n) {
		n--;
		*q++ = ch = *p++;
		if (!ch)
			break;
	}

	/* The specs say strncpy() fills the entire buffer with NUL.  Sigh. */
	__wrap_memset(q, 0, n);

	return dst;
}