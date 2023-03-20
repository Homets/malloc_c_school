// cette définition permet d'accéder à mremap lorsqu'on inclue sys/mman.h
#define _GNU_SOURCE
#define _ISOC99_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>

#include "my_secmalloc.h"
#include "my_secmalloc_private.h"

//global pool
void    *metadata_pool = NULL;
void    *data_pool = NULL;


void    *my_init_metadata_pool()
{
    metadata_pool = mmap(NULL, 3144, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    
    void **ptr;
    ptr = &metadata_pool;

    //init all metadata block in a linked list with all alltribute setup to null except next
    for (int i = 0; i < 131;i++)
    {
        struct metadata *metadata = *ptr + (i * sizeof(struct metadata));
        if (i < 130){
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
    data_pool = mmap(NULL,314400, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    return data_pool;

}


int    clean_metadata_pool()
{
        int res = munmap(metadata_pool,3144);
        if (res == 0)
        {   
            my_log("munmap success\n");
            return 0;
        }
        return -1;
}

int     clean_data_pool()
{
    int res = munmap(data_pool,314400);
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

void    *my_malloc(size_t size)
{  
    if (size == 0){
        my_log("You need to specify a size");
        return 0;
    }
    //var used to add size of a block every time a metadata is already taken for avoid the the allocation of already used memory
    char *ptr; 
    ptr = data_pool;

    //search a descriptor block available
    struct metadata *metadata_available = metadata_pool;
    while (metadata_available->next != NULL){
        if (metadata_available->block_pointer == NULL){
            metadata_available->block_pointer = ptr;
            metadata_available->block_size = size + 8; //8 is canary size
            //write data on canary
            for (int i = 0; i < 8;i++){
                ptr[size + i] = 'X';
            }
            return ptr;
        } else {
            ptr += metadata_available->block_size;
            metadata_available = metadata_available->next;
        }
    }
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
