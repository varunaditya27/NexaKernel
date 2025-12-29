/*
 * ===========================================================================
 * lib/cstd/string.h
 * ===========================================================================
 *
 * Standard String Library Interface
 *
 * This header provides string manipulation functions for the kernel and
 * userland applications. These are freestanding implementations that do
 * not rely on an external C library.
 *
 * Usage:
 *   #include <lib/cstd/string.h>
 *
 *   char dest[64];
 *   strcpy(dest, "Hello");
 *   size_t len = strlen(dest);
 *
 * ===========================================================================
 */

#ifndef NEXA_CSTD_STRING_H
#define NEXA_CSTD_STRING_H

#include "../../config/os_config.h"

/* ---------------------------------------------------------------------------
 * String Length Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Calculate the length of a string
 * @param s Null-terminated string
 * @return Number of characters before the null terminator
 */
size_t strlen(const char *s);

/**
 * @brief Calculate the length of a string with a maximum limit
 * @param s Null-terminated string
 * @param maxlen Maximum number of characters to scan
 * @return min(actual length, maxlen)
 */
size_t strnlen(const char *s, size_t maxlen);

/* ---------------------------------------------------------------------------
 * String Copy Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Copy a string
 * @param dst Destination buffer
 * @param src Source string (null-terminated)
 * @return Pointer to destination
 * @warning dst must have enough space for src including null terminator
 */
char *strcpy(char *dst, const char *src);

/**
 * @brief Copy up to n characters from a string
 * @param dst Destination buffer
 * @param src Source string
 * @param n Maximum characters to copy
 * @return Pointer to destination
 * @note If src is shorter than n, dst is padded with nulls
 */
char *strncpy(char *dst, const char *src, size_t n);

/* ---------------------------------------------------------------------------
 * String Comparison Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Compare two strings
 * @param s1 First string
 * @param s2 Second string
 * @return <0 if s1<s2, 0 if s1==s2, >0 if s1>s2
 */
int strcmp(const char *s1, const char *s2);

/**
 * @brief Compare up to n characters of two strings
 * @param s1 First string
 * @param s2 Second string
 * @param n Maximum characters to compare
 * @return <0 if s1<s2, 0 if s1==s2, >0 if s1>s2
 */
int strncmp(const char *s1, const char *s2, size_t n);

/* ---------------------------------------------------------------------------
 * String Concatenation Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Concatenate two strings
 * @param dst Destination string (null-terminated)
 * @param src Source string to append
 * @return Pointer to destination
 * @warning dst must have enough space for both strings
 */
char *strcat(char *dst, const char *src);

/**
 * @brief Concatenate up to n characters
 * @param dst Destination string
 * @param src Source string
 * @param n Maximum characters to append
 * @return Pointer to destination
 */
char *strncat(char *dst, const char *src, size_t n);

/* ---------------------------------------------------------------------------
 * String Search Functions
 * --------------------------------------------------------------------------- */

/**
 * @brief Find first occurrence of a character in a string
 * @param s String to search
 * @param c Character to find
 * @return Pointer to first occurrence, or NULL if not found
 */
char *strchr(const char *s, int c);

/**
 * @brief Find last occurrence of a character in a string
 * @param s String to search
 * @param c Character to find
 * @return Pointer to last occurrence, or NULL if not found
 */
char *strrchr(const char *s, int c);

/**
 * @brief Find first occurrence of a substring
 * @param haystack String to search in
 * @param needle Substring to find
 * @return Pointer to first occurrence, or NULL if not found
 */
char *strstr(const char *haystack, const char *needle);

#endif /* NEXA_CSTD_STRING_H */
