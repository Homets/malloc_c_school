/* ************************************************************************** */
/*                                                                            */
/*      file:                        __   __   __   __                        */
/*      my_secmalloc.c                _) /__  /  \ /  \                       */
/*                                   /__ \__) \__/ \__/                       */
/*                                                                            */
/*   Authors:                                                                 */
/*      bastien.petitclerc@ecole2600.com                                      */
/*      raphael.busy@ecole2600.com                                            */
/*                                                                            */
/* ************************************************************************** */


#define _GNU_SOURCE
#define _ISOC99_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


#include "my_secmalloc.h"
#include "my_secmalloc_private.h"

//global variable
void    *metadata_pool = NULL;
void    *data_pool = NULL;
size_t  metadata_size = 0;
size_t  data_size = 0;
int     pool_is_create = 0;

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


void    *my_init_metadata_pool()
{
    metadata_size = 12288;
    metadata_pool = mmap(NULL, metadata_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    void **ptr;
    ptr = &metadata_pool;
    //init all metadata block in a linked list with all alltribute setup to null except p_next
    for (unsigned long i = 0; i < metadata_size / sizeof(struct metadata_t);i++)
    {
        struct metadata_t *metadata = *ptr + (i * sizeof(struct metadata_t));   
        if (i < metadata_size / sizeof(struct metadata_t) -1 ){
            metadata->p_next = *ptr + ((i + 1) * sizeof(struct metadata_t));
            metadata->p_block_pointer = NULL;
            metadata->sz_block_size = 0;
        } else {
            metadata->p_next = NULL;
            metadata->p_block_pointer = NULL;
            metadata->sz_block_size = 0;
        }
    }

    return metadata_pool;
}

void    *my_init_data_pool()
{
    data_size = 409600;
    data_pool = mmap(NULL,data_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    int lock = mlockall(MCL_CURRENT | MCL_FUTURE);
    if (lock != 0){
        time_t time = get_time();
        write_log("%d UNABLE TO UNLOCK MEMORY!\n---------------------------------------\n",time);
    }
    return data_pool;

}

//clean metadata and data and perform metadata analysis to check if memory leak is present by checking block pointer
void    clean_metadata_pool(void)
{   
    struct metadata_t *metadata = metadata_pool;
    while (metadata->p_next != NULL)
    {
        if (metadata->p_block_pointer != NULL){
            time_t time = get_time();
            write_log("%d MEMORY LEAK!\nAddress => %p\nSize => %d\n---------------------------------------\n",time,metadata->p_block_pointer, metadata->p_block_pointer);
        }
        metadata = metadata->p_next;
    }
    if (metadata->p_block_pointer != NULL && metadata->p_next == NULL){
            time_t time = get_time();
            write_log("%d MEMORY LEAK!\nAddress => %p\nSize => %d\n---------------------------------------\n",time,metadata->p_block_pointer, metadata->p_block_pointer);
    }

    munmap(metadata_pool,metadata_size);
    int munlock = munlockall();
    if (munlock != 0){
        time_t time = get_time();
        write_log("%d UNABLE TO UNLOCK MEMORY!\n---------------------------------------\n",time);
    }
}

void     clean_data_pool()
{
    munmap(data_pool,data_size);
}

time_t    get_time(){
    time_t t = time(&t);

    return t;
}

//LOG FUNCTION
//for malloc
//timestamp <info | error> MALLOC size pointer <error> 
//for free
//timestamp <info | error> FREE <size desallocated if no error> pointer <error>
//for calloc
//timestamp <info | error> CALLOC <size> <ptr | error>
//for realloc
//timestamp <info | error> REALLOC <old size> <new size> <ptr | error>
void    write_log(const char *fmt,...){

    const char *log_file_path = secure_getenv(LOG_ENV_VAR); 
    
    va_list ap;
    va_start(ap,fmt);
    size_t sz = vsnprintf(NULL,0,fmt,ap);
    va_end(ap);
    char *buf = alloca(sz);
    va_start(ap,fmt);
    vsnprintf(buf, sz + 1,fmt,ap);
    va_end(ap);
    // write log file
    int fd = open(log_file_path, O_RDWR | O_CREAT | O_APPEND);
    if (fd == -1){
        my_log(ERROR_WRITING_LOG);
    }   
    ssize_t write_fd = write(fd, buf, strlen(buf));
    if (write_fd == -1){
        write_log("%sn---------------------------------------\n", ERROR_WRITING_LOG);
    }

    if (fd != -1){
        fsync(fd);
        close(fd);
    }

}

//align a value with 4096 because mremap need aligned page
size_t   get_aligned_size(size_t size)
{
        while (size % ALIGNED_SIZE != 0)
            size++;
        return size;
}

//this function will check if the size of 
void     check_data_pool_size(size_t size)
{
    size_t sz_aligned_size = get_aligned_size(size);
    struct metadata_t *ptr = metadata_pool;
    size_t sz_size_copy = data_size;   
    size_t sz_increment_size = 0;
    //check if data pool size is enough 
    while (ptr->p_next != NULL){
        sz_increment_size += ptr->sz_block_size;
        ptr = ptr->p_next;
    }
    if (sz_increment_size + sz_aligned_size > sz_size_copy){
        data_pool = mremap(data_pool, sz_size_copy, sz_increment_size + sz_aligned_size, 0);
        if (data_pool == MAP_FAILED){
            time_t time = get_time();
            write_log("%lu ERROR INCREASE DATA POOL SIZE\n---------------------------------------\n",time,sz_aligned_size);

        }
        data_size = data_size + sz_aligned_size;
        time_t time = get_time();
        write_log("%lu INFO INCREASE DATA POOL SIZE\n---------------------------------------\n",time,sz_aligned_size);

    }
}

void    *my_malloc(size_t size)
{   
    if (size == 0){
        time_t time = get_time();
        write_log("%lu ERROR MALLOC %d size equal to 0\n---------------------------------------\n",time,size);
        return ERROR_TO_ALLOCATE;
    }
    if (pool_is_create == 0){
        my_init_data_pool();
        my_init_metadata_pool();
        atexit(clean_metadata_pool);
        atexit(clean_data_pool);
        pool_is_create = 1;

    }
    //var used to add size of a block every time a metadata is already taken for avoid the the allocation of already used memory
    char *ptr; 
    ptr = data_pool;
    unsigned long metadata_pool_sz = metadata_size; //size will be used for mremap if all metadata block are already used

    //check data length
    check_data_pool_size(size);
    //search a descriptor block available
    metadata_t *metadata_available = metadata_pool;
    while (metadata_available->p_next != NULL){
        metadata_pool_sz += sizeof(struct metadata_t);
        //if a metadata block is null and the sz_block_size is greater than size param 
        if (metadata_available->p_block_pointer == NULL &&  metadata_available->sz_block_size <= size - CANARY_SZ){
            metadata_available->p_block_pointer = ptr;
            metadata_available->sz_block_size = size + CANARY_SZ;
            //write data canary
            for (int i = 0; i < CANARY_SZ;i++){
                ptr[size + i] = 'X';
            }

            time_t time = get_time();
            write_log("%lu INFO MALLOC %d %p\n---------------------------------------\n",time,size, ptr);
            return ptr;
        } else {
            ptr += metadata_available->sz_block_size;
            metadata_available = metadata_available->p_next;
        }
    }

    //reallocate the metadata pool adding and completing one descriptor and return the data pool pointer
    metadata_pool = mremap(metadata_pool,metadata_pool_sz, metadata_pool_sz + sizeof(struct metadata_t), 0);
    metadata_size += sizeof(metadata_t);
    if (metadata_available->p_next == NULL){
        ptr += metadata_available->sz_block_size;
        metadata_available->p_next = metadata_available + sizeof(struct metadata_t);
        metadata_available->p_next->sz_block_size = size;
        metadata_available->p_next->p_block_pointer = ptr;
        time_t time = get_time();
        write_log("%lu INFO MALLOC %d %p\n---------------------------------------\n",time,size, ptr);
        return ptr;
    }
    
    time_t time = get_time();
    write_log("%lu ERROR MALLOC %d\n ERROR TO ALLOCATE\n---------------------------------------\n",time,size);

    return ERROR_TO_ALLOCATE;
    
}

void    my_free(void *ptr)
{   
    //take pointer
    //loop in all metadata descriptor and check p_block_pointer to check if pointer passed in arguement is equal
    if (ptr == NULL){
        return;
    }
    struct metadata_t *metadata = metadata_pool;
    while (metadata->p_next != NULL)
    {
            if (ptr == metadata->p_block_pointer){

                //check if canary is overwritten by user
                char *ptr_canary = metadata->p_block_pointer + metadata->sz_block_size - CANARY_SZ;
                for (int i = 0; i < CANARY_SZ;i++){
                    if (ptr_canary[i] != 'X'){
                        time_t time = get_time();
                        write_log("%lu ERROR FREE %d %p\n ERROR CANARY IS OVERWRITTEN\n---------------------------------------\n",time,metadata->sz_block_size, ptr);
                        exit(0);
                    }
                }

                time_t time = get_time();
                write_log("%lu INFO FREE %d %p\n---------------------------------------\n",time,metadata->sz_block_size, metadata->p_block_pointer);
                //dont set sz_block_size to 0 for p_next malloc on this descriptor
                metadata->p_block_pointer = NULL;
                break;

            }
            
            metadata = metadata->p_next;
    }
}

void    *my_calloc(size_t nmemb , size_t size)
{
    if ((nmemb == 0 || size == 0) || (nmemb == 0 && size == 0)){
        time_t time = get_time();
        write_log("%lu ERROR CALLOC %d nmemb %d size\n INVALID ARGUMENT\n---------------------------------------\n",time,size, nmemb);
        return ERROR_TO_ALLOCATE;
    }
    if (pool_is_create == 0){
        my_init_data_pool();
        my_init_metadata_pool();
        pool_is_create = 1;
    }
    size_t total_sz = nmemb * size;
    void *ptr = my_malloc(total_sz);
    if (ptr == NULL){
        time_t time = get_time();
        write_log("%lu ERROR CALLOC %d nmemb %d size\n---------------------------------------\n",time,size, nmemb);
        return ERROR_TO_ALLOCATE;
    }
    //write all byte allocate with 0
    char *ptr_write = ptr;
    for (size_t i = 0;i < total_sz;i++){
        ptr_write[i] = '0';
    }

    return ptr;
}

void    *my_realloc(void *ptr, size_t size)
{

    time_t time = get_time();
    write_log("%lu INFO REALLOC %d\n size equal to 0\n---------------------------------------\n",time,size);
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
