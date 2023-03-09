#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

/*
 * Ici vous pourrez faire toutes les déclarations de variables/fonctions pour votre usage interne
 * */

struct      metadata
{
    int                 block_pointer;
    int                 block_state; //0 allocated | 1 free
    unsigned long       block_size;

};


void*       my_init_pool();

int        clean_metadata_pool(void *metadata_pool);

void        my_log(const char *fmt,...);

void        increase_metadata_pool(void *metadata_pool, size_t oldsize, size_t newsize);

void        increase_data_pool(void *data_pool, size_t oldsize, size_t newsize);

#endif
