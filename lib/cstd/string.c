/*
 * ===========================================================================
 * lib/cstd/string.c
 * ===========================================================================
 *
 * Standard String Library Implementation
 *
 * This file implements standard C string functions (strlen, strcpy, strcmp, etc.)
 * for use by the kernel and userland applications. These are freestanding
 * implementations that do not rely on an external C library.
 *
 * ===========================================================================
 */

#include "string.h"
#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * String Length Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Calculate the length of a string
 */
size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

/**
 * @brief Calculate the length of a string with a maximum limit
 */
size_t strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;
    while (len < maxlen && s[len])
        len++;
    return len;
}

char *strcpy(char *dst, const char *src) {
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
    char *ret = dst;
    while (n > 0 && *src) {
        *dst++ = *src++;
        n--;
    }
    while (n > 0) {
        *dst++ = '\0';
        n--;
    }
    return ret;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strcat(char *dst, const char *src)
{
    char *ret = dst;
    while (*dst) dst++;
    while ((*dst++ = *src++));
    return ret;
}

/**
 * @brief Concatenate up to n characters
 */
char *strncat(char *dst, const char *src, size_t n)
{
    char *ret = dst;
    while (*dst) dst++;
    while (n > 0 && *src) {
        *dst++ = *src++;
        n--;
    }
    *dst = '\0';
    return ret;
}

/* ---------------------------------------------------------------------------
 * String Search Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Find first occurrence of a character in a string
 */
char *strchr(const char *s, int c)
{
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : NULL;
}

/**
 * @brief Find last occurrence of a character in a string
 */
char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c)
            last = s;
        s++;
    }
    return (c == '\0') ? (char *)s : (char *)last;
}

/**
 * @brief Find first occurrence of a substring
 */
char *strstr(const char *haystack, const char *needle)
{
    if (*needle == '\0')
        return (char *)haystack;

    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        
        if (*n == '\0')
            return (char *)haystack;
        
        haystack++;
    }
    return NULL;
}
