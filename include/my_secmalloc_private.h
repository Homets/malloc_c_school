#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

/*
 * Ici vous pourrez faire toutes les déclarations de variables/fonctions pour votre usage interne
 * */

//error
#define ERROR_CANARY_OVERWRITTEN "ERROR CANARY OVERWRITTEN"
#define ERROR_FREE_NO_POINTER "ERROR NO POINTER OF DATA EXIST"
#define ERROR_TO_ALLOCATE NULL
#define ERROR_TO_CLEAN_POOL -1
#define ERROR_WRITING_LOG "ERROR FOR WRITING LOG"


//constant
#define CANARY_SZ 8
#define LOG_ENV_VAR "MSM_OUTPUT"

//variable global
extern void *metadata_pool;
extern void *data_pool;
extern size_t metadata_size;
extern size_t data_size;

//struct
struct      metadata
{
    void                *block_pointer;
    size_t              block_size;
    struct metadata     *next;

};

//func
void        *my_init_metadata_pool();

void        *my_init_data_pool();

int         clean_metadata_pool();

int         clean_data_pool();

void        my_log(const char *fmt,...);

void        write_log(const char *fmt,...);

void        increase_metadata_pool(void *metadata_pool, size_t oldsize, size_t newsize);

void        increase_data_pool(void *data_pool, size_t oldsize, size_t newsize);

#endif
