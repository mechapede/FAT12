/* Utilities for memory management and file management. All frees are callers responsability
 *
 * Note: these function abort when detecting an error during a call
 * this behavior is acceptable for a stand alone program but is
 * insufficient for a library
 * */

#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>

/* Malloc wrapper to track allocations. Aborts program on failure */
void * xmalloc(size_t size);

/* Realloc function to track reallocations.  Aborts program on failure */
void * xrealloc(void * ptr, size_t size);

/* Free wrapper to track freeing memory */
void xfree(void * ptr);

/* Wrapper for xseek, aborts if seek fails */
void xfseek(FILE *stream, long offset, int whence);

/* Wrapper for fread, aborts if read does not read expected tokens */
size_t xfread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/* Wrapper for fwrite, aborts if write does not write expected tokens */
size_t xfwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);

#endif
