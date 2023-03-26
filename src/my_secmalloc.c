// cette définition permet d'accéder à mremap lorsqu'on inclue sys/mman.h
#define _GNU_SOURCE
#define _ISOC99_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>
#include <stdlib.h>

#include "my_secmalloc.h"
#include "my_secmalloc_private.h"

//global variable
void    *metadata_pool = NULL;
void    *data_pool = NULL;
size_t metadata_size = 0;
size_t data_size = 0;


void    *my_init_metadata_pool()
{
    metadata_size = 3144;
    metadata_pool = mmap(NULL, metadata_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    
    void **ptr;
    ptr = &metadata_pool;
    //init all metadata block in a linked list with all alltribute setup to null except next
    for (unsigned long i = 0; i < 131;i++)
    {
        struct metadata *metadata = *ptr + (i * sizeof(struct metadata));
        if (i < metadata_size / sizeof(struct metadata)){
            metadata->next = *ptr + ((i + 1) * sizeof(struct metadata));
            metadata->block_pointer = NULL;
            metadata->block_size = 0;
        }
    }

    // my_log("metadata pool and chained list setup");
    return metadata_pool;
}

void    *my_init_data_pool()
{
    data_size = 314400;
    data_pool = mmap(NULL,data_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    return data_pool;

}

int    clean_metadata_pool()
{
        int res = munmap(metadata_pool,metadata_size);
        if (res == 0)
        {   
            my_log("munmap success\n");
            return 0;
        }
        return -1;
}

int     clean_data_pool()
{
    int res = munmap(data_pool,data_size);
    if (res == 0)
    {
        my_log("munmap success\n");
        return 0;
    }
    return -1;

}


//Log without heap allocation
void    my_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    size_t sz = vsnprintf(NULL,0,fmt,ap);
    va_end(ap);
    char *buf = alloca(sz + 2);
    va_start(ap,fmt);
    vsnprintf(buf, sz + 1,fmt,ap);
    va_end(ap);
    write(2, buf, strlen(buf));
}


void     check_data_pool_size(size_t size)
{
    struct metadata *ptr = metadata_pool;
    size_t size_copy = data_size;
    size_t increment_size = 0;
    //check if data pool size is enough 
    while (ptr->next != NULL){
        increment_size += ptr->block_size;
        ptr = ptr->next;
    }
    if (increment_size + size > size_copy){
        my_log("on augmente la taille\n");
        data_pool = mremap(data_pool, size_copy, increment_size+ size, 0);
        if (data_pool == MAP_FAILED){
            my_log("error reallocate memory");
        }
        data_size = data_size + size;
    }
}


void    *my_malloc(size_t size)
{  
    if (size == 0){
        my_log("You need to specify a size");
        return 0;
    }
    //var used to add size of a block every time a metadata is already taken for avoid the the allocation of already used memory
    char *ptr; 
    ptr = data_pool;
    unsigned long metadata_pool_sz = metadata_size; //size will be used for mremap if all metadata block are already used

    //check data length
    check_data_pool_size(size);
    //search a descriptor block available
    struct metadata *metadata_available = metadata_pool;
    while (metadata_available->next != NULL){
        metadata_pool_sz += sizeof(struct metadata);
        if (metadata_available->block_pointer == NULL){
            metadata_available->block_pointer = ptr;
            metadata_available->block_size = size + 8; //8 is canary size
            //write data canary
            for (int i = 0; i < 8;i++){
                ptr[size + i] = 'X';
            }
            return ptr;
        } else {
            ptr += metadata_available->block_size;
            if (metadata_available-> next == NULL){
                metadata_available->next = metadata_available + sizeof(struct metadata);
                metadata_available->next->block_size = size;
                metadata_available->next->block_pointer = ptr;
            }
            metadata_available = metadata_available->next;
        }
    }
    metadata_pool = mremap(metadata_pool,metadata_pool_sz, metadata_pool_sz + sizeof(struct metadata), 0);
    metadata_size += sizeof(struct metadata);
    //create new descriptor and the allocation
    
    return NULL;
}

void    my_free(void *ptr)
{
    (void) ptr;
}

void    *my_calloc(size_t nmemb, size_t size)
{
    (void) nmemb;
    (void) size;
    return NULL;
}

void    *my_realloc(void *ptr, size_t size)
{
    (void) ptr;
    (void) size;
    return NULL;

}

#ifdef DYNAMIC
/*
 * Lorsque la bibliothèque sera compilé en .so les symboles malloc/free/calloc/realloc seront visible
 * */

void    *malloc(size_t size)
{
    return my_malloc(size);
}
void    free(void *ptr)
{
    my_free(ptr);
}
void    *calloc(size_t nmemb, size_t size)
{
    return my_calloc(nmemb, size);
}

void    *realloc(void *ptr, size_t size)
{
    return my_realloc(ptr, size);
}

#endif
