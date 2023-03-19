#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

/*
 * Ici vous pourrez faire toutes les déclarations de variables/fonctions pour votre usage interne
 * */

extern void *metadata_pool;
extern void *data_pool;


struct      metadata
{
    void                 *block_pointer;
    unsigned long       block_size;
    struct metadata     *next;

};


void        *my_init_metadata_pool();

void        *my_init_data_pool();

int         clean_metadata_pool();

int         clean_data_pool();

void        my_log(const char *fmt,...);

void        increase_metadata_pool(void *metadata_pool, size_t oldsize, size_t newsize);

void        increase_data_pool(void *data_pool, size_t oldsize, size_t newsize);

#endif
