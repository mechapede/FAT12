/* Utilities for memory management and file management. All frees are callers responsability
 *
 * Note: these function abort when detecting an error during a call
 * this behavior is acceptable for a stand alone program but is
 * insufficient for a library
 * */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "utils.h"

/* Malloc wrapper to track allocations. Aborts program on failure */
void * xmalloc(size_t size) {
    assert(size);
    void * tmp = malloc(size);
    if(!tmp) {
        perror("FATAL: A malloc failed: ");
        abort();
    }
    return tmp;
}

/* Realloc function to track reallocations. Aborts program on failure */
void * xrealloc(void * ptr, size_t size) {
    assert(size);
    void * tmp = realloc(ptr,size);
    if(!tmp) {
        perror("FATAL: A realloc failed: ");
        abort();
    }
    return tmp;
}

/* Free wrapper to track freeing memory */
void xfree(void * ptr) {
    assert(ptr);
    free(ptr);
}

/* Wrapper for xseek, aborts if seek fails */
void xfseek(FILE *stream, long offset, int whence) {
    if( fseek(stream,offset, whence) ) {
        perror("FATAL: a seek failed: ");
        abort();
    }

}

/* Wrapper for fread, aborts if read does not read expected tokens */
size_t xfread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t read;
    if( (read = fread(ptr, size, nmemb, stream)) != nmemb ) {
        perror("FATAL: Read got no bytes:");
        abort();
    }
    return read;
}

/* Wrapper for fwrite, aborts if write does not write expected tokens */
size_t xfwrite(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t read;
    if( (read = fwrite(ptr, size, nmemb, stream)) != nmemb ) {
        perror("FATAL: Writing wrote no bytes:");
        abort();
    }
    return read;
}
