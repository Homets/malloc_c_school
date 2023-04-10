// cette définition permet d'accéder à mremap lorsqu'on inclue sys/mman.h
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
            return 0;
        }
        return ERROR_TO_CLEAN_POOL;
}

int     clean_data_pool()
{
    int res = munmap(data_pool,data_size);
    if (res == 0)
    {
        return 0;
    }
    return ERROR_TO_CLEAN_POOL;
}


char    *get_time(){
    time_t t;
    time(&t);
    char *actual_time = ctime(&t);
    // my_log("%s", actual_time);
    return actual_time;
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

//LOG FUNCTION
//FOR MALLOC
//timestamp <info | error> MALLOC size pointer <error> 
//FOR FREE
//timestamp <info | error> FREE <size desallocated if no error> pointer <error>
//FOR CALLOC
//timestamp <info | error> CALLOC <size> <ptr | error>
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
        my_log(ERROR_WRITING_LOG);
    }

    if (fd != -1){
        fsync(fd);
        close(fd);
    }

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
        char *time = get_time();
        write_log("%s ERROR MALLOC %d size equal to 0\n---------------------------------------\n",time,size);
        return ERROR_TO_ALLOCATE;
    }
    if (pool_is_create == 0){
        my_init_data_pool();
        my_init_metadata_pool();
        pool_is_create = 1;
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
        //if a metadata block is null and the block_size is greater than size param 
        if (metadata_available->block_pointer == NULL && (metadata_available->block_size == 0 || metadata_available->block_size <= size - CANARY_SZ)){

            metadata_available->block_pointer = ptr;
            metadata_available->block_size = size + CANARY_SZ;
            //write data canary
            for (int i = 0; i < CANARY_SZ;i++){
                ptr[size + i] = 'X';
            }

            char *time = get_time();
            write_log("%s INFO MALLOC %d %p\n---------------------------------------\n",time,size, ptr);
            return ptr;
        } else {
            ptr += metadata_available->block_size;
            metadata_available = metadata_available->next;
        }
    }

    //reallocate the metadata pool adding and completing one descriptor and return the data pool pointer
    metadata_pool = mremap(metadata_pool,metadata_pool_sz, metadata_pool_sz + sizeof(struct metadata), 0);
    metadata_size += sizeof(struct metadata);
    if (metadata_available->next == NULL){
        ptr += metadata_available->block_size;
        metadata_available->next = metadata_available + sizeof(struct metadata);
        metadata_available->next->block_size = size;
        metadata_available->next->block_pointer = ptr;
        char *time = get_time();
        write_log("%s INFO MALLOC %d %p\n---------------------------------------\n",time,size, ptr);
        return ptr;
    }
    
    char *time = get_time();
    write_log("%s ERROR MALLOC %d ERROR TO ALLOCATE\n---------------------------------------\n",time,size);
    return ERROR_TO_ALLOCATE;
    
}

void    my_free(void *ptr)
{
    //take pointer
    //loop in all metadata descriptor and check block_pointer to check if pointer passed in arguement is equal
    struct metadata *metadata = metadata_pool;
    while (metadata->next != NULL)
    {
            if (ptr == metadata->block_pointer){
                //check if canary is overwritten by user
                char *ptr_canary = metadata->block_pointer + metadata->block_size - CANARY_SZ;
                for (int i = 0; i < CANARY_SZ;i++){
                    if (ptr_canary[i] != 'X'){
                        char *time = get_time();
                        write_log("%s ERROR FREE %d %p ERROR CANARY IS OVERWRITTEN\n---------------------------------------\n",time,metadata->block_size, ptr);
                        exit(0);
                    }
                }

                //reset all char to 'O'
                char *ptr_erase = metadata->block_pointer;
                for (size_t j = 0; j < metadata->block_size + CANARY_SZ; j++){
                    ptr_erase[j] = '0';
                    ptr_erase++;
                }
                char *time = get_time();
                write_log("%s INFO FREE %d %p\n---------------------------------------\n",time,metadata->block_size, metadata->block_pointer);
                //dont set block_size to 0 for next malloc on this descriptor
                metadata->block_pointer = NULL;
                break;

            }
    }
    char *time = get_time();
    write_log("%s INFO FREE %d %p\n---------------------------------------\n",time,metadata->block_size, ptr);

}

void    *my_calloc(size_t nmemb , size_t size)
{
    if ((nmemb == 0 || size == 0) || (nmemb == 0 && size == 0)){
        char *time = get_time();
        write_log("%s ERROR CALLOC %d nmemb %d size INVALID ARGUMENT\n---------------------------------------\n",time,size, nmemb);
        return ERROR_TO_ALLOCATE;
    }
    if (pool_is_create == 0){
        my_init_data_pool();
        my_init_metadata_pool();
        pool_is_create = 1;
    }
    size_t total_sz = nmemb * size;
    my_log("size => %d\n",total_sz);
    void *ptr = my_malloc(total_sz);
    my_log("passe le malloc\n");
    if (ptr == NULL){
        char *time = get_time();
        write_log("%s ERROR CALLOC %d nmemb %d size\n---------------------------------------\n",time,size, nmemb);
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
