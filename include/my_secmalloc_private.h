/* ************************************************************************** */
/*                                                                            */
/*      file:                        __   __   __   __                        */
/*      my_secmalloc_private.h        _) /__  /  \ /  \                       */
/*                                   /__ \__) \__/ \__/                       */
/*                                                                            */
/*   Authors:                                                                 */
/*      bastien.petitclerc@ecole2600.com                                      */
/*      raphael.busy@ecole2600.com                                            */
/*                                                                            */
/* ************************************************************************** */

#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

//error
#define ERROR_CANARY_OVERWRITTEN "ERROR CANARY OVERWRITTEN"
#define ERROR_FREE_NO_POINTER "ERROR NO POINTER OF DATA EXIST"
#define ERROR_REALLOCATION "ERROR REALLOCATE MEMORY"
#define ERROR_TO_ALLOCATE NULL
#define ERROR_TO_CLEAN_POOL -1
#define ERROR_WRITING_LOG "ERROR FOR WRITING LOG\n"


//constant
#define CANARY_SZ 8
#define LOG_ENV_VAR "MSM_OUTPUT"

//variable globale
extern void    *metadata_pool;
extern void    *data_pool;
extern size_t  metadata_size;
extern size_t  data_size;
extern int     pool_is_create;

//struct
typedef struct      metadata_t
{
    void                  *p_block_pointer;
    size_t                sz_block_size;
    struct metadata_t     *p_next;

} metadata_t;

//func
void        *my_init_metadata_pool();

void        *my_init_data_pool();

void        clean_metadata_pool();

void        clean_data_pool();

time_t      get_time();

void        my_log(const char *fmt,...);

void        write_log(const char *fmt,...);

void        increase_metadata_pool(void *metadata_pool, size_t oldsize, size_t newsize);

void        increase_data_pool(void *data_pool, size_t oldsize, size_t newsize);

#endif
