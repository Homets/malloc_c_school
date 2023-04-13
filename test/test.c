/* ************************************************************************** */
/*                                                                            */
/*      file:                        __   __   __   __                        */
/*      test.c                        _) /__  /  \ /  \                       */
/*                                   /__ \__) \__/ \__/                       */
/*                                                                            */
/*   Authors:                                                                 */
/*      bastien.petitclerc@ecole2600.com                                      */
/*      raphael.busy@ecole2600.com                                            */
/*                                                                            */
/* ************************************************************************** */


#define _GNU_SOURCE
#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "my_secmalloc.h"
#include "my_secmalloc_private.h"


Test(log, test_log, .init=cr_redirect_stderr)
{   
    my_log("coucou %d\n",12);
}   

// Test(metadata_pool, create_clean_metadata_pool)
// {   
//     my_init_metadata_pool();
//     my_log("%p\n", metadata_pool);
// }

// Test(data_pool, create_clean_data_pool)
// {
//     my_init_metadata_pool();
//     my_init_data_pool();
//     my_log("%p\n", data_pool);
//     cr_assert(metadata_pool != NULL);
//     cr_assert(data_pool != NULL);
//     clean_metadata_pool();
//     clean_data_pool();
// }

// Test(metadata_pool, test_all_metadata_pool)
// {
//     my_init_metadata_pool();
//     int i = 0;
//     struct metadata_t *m = metadata_pool;
//     while( i < 5)
//     {
//         my_log("p_next => %p\n", m->p_next);
//         my_log("size => %d\n\n", m->p_block_pointer);
//         i = i + 1;
//         m = m->p_next;
//     }
//     clean_metadata_pool();
// }


Test(metadata_pool, check_metadatablock_edited)
{
    my_log("ptr de data %p\n", data_pool);
    void **ptr;
    void **ptr1;
    ptr = my_malloc(8);
    my_log("pointeur give => %p\n", ptr);
    ptr1 = my_malloc(8);
    my_log("\n\nptr => data2 %p\n", ptr1);

}

Test(data_pool, check_canary_well_written)
{
 
    size_t sz_data = 8;
    char *ptr = my_malloc(sz_data);

    for (int i = 0; i < 8;i++)
    {
        char s = ptr[sz_data + i];
        my_log("%c", s);
        cr_assert(s == 'X');
    }
    
}

Test(data_pool, mremap_is_correct)
{
    char *ptr = mmap(NULL, 15, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON , -1, 0);
    for (int i = 0; i < 15;i++){
        ptr[i] = 'X';
    }
    for (int i = 0; i < 15; i++){
        cr_assert(ptr[i] == 'X');
    }
    //check if data is not overwritten or pointer adress change
    ptr= mremap(ptr,15,18,0);
    if (ptr == MAP_FAILED)
    {
        my_log("error mremap\n");
    }
    for (int i = 0; i < 15; i++){
        cr_assert(ptr[i] == 'X');
    }
    int res = munmap(ptr,18);
    (void)res;
}
Test(data_pool, test_reallocation_of_data_pool)
{

    void *ptr;
    ptr = my_malloc(491522);
    my_log("\n\nptr datadata => %p\n\n", ptr);

}

//check to reallocate memory and add a metadata descriptor
// Test(data_pool, test_reallocation_of_data_pool_and_struct_add)
// {
//     metadata_size = 24;
//     metadata_pool = mmap(NULL, metadata_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

//     void **ptr;
//     ptr = &metadata_pool;
//     //init all metadata block in a linked list with all alltribute setup to null except p_next
//     for (unsigned long i = 0; i < 1;i++)
//     {
//         metadata_t *metadata = *ptr + (i * sizeof(struct metadata_t));
//         if (i < metadata_size / sizeof(struct metadata_t)){
//             metadata->p_next = NULL;
//             metadata->p_block_pointer = NULL;
//             metadata->p_block_pointer = 0;
//         }
//     }
//     struct metadata_t *metadata_available = metadata_pool;
//     if (metadata_available->p_next == NULL)
//     {
//         metadata_pool = mremap(metadata_pool,metadata_size, metadata_size + sizeof(struct metadata_t), 0);
//         metadata_size += sizeof(metadata_t);
//     }
//     metadata_t *metadata = *ptr;
//     if (metadata->p_next == NULL)
//     {
//         metadata->p_next = metadata_pool + sizeof(metadata_t);
//         metadata->p_next->p_next = NULL;
//         metadata->p_next->sz_block_size = 1000;
//         cr_assert(metadata->p_next->sz_block_size == 1000);
//     }
//     // cr_assert(clean_metadata_pool() == 0);
// }

Test(data_free, test_free)
{
    size_t sz_data = 8;
    char *ptr = my_malloc(sz_data);
    struct metadata_t *first_meta = metadata_pool;
    my_log("%p\n", first_meta->p_block_pointer);
    my_log("%ld", first_meta->p_block_pointer);
    cr_assert(first_meta->p_block_pointer = ptr);
    cr_assert(first_meta->sz_block_size = sz_data);

    my_free(ptr);
    struct metadata_t *second_meta = metadata_pool;
    my_log("%p", second_meta->p_block_pointer);
    my_log("%ld", second_meta->p_block_pointer);
    cr_assert(second_meta->p_block_pointer == NULL);
    cr_assert(second_meta->p_block_pointer == 0);

}

Test(data_free, check_canary_overwritten)
{

    size_t sz_data = 8;
    char *ptr = my_malloc(sz_data); 
    ptr[sz_data + 1] = 'A';
    
    my_free(ptr);

}


Test(test_calloc, check_calloc)
{

    size_t n = 4;
    char *ptr = my_calloc(n, sizeof(int)); 
    for (int i = 0; i < 15; i++){
        cr_assert(ptr[i] == '0');
    };
    my_free(ptr);

}

Test(test_log,test_to_write_log){
    write_log("---------------------------------------\ntest write_log\n---------------------------------------\n");
}

// Test(test_log,test_to_get_time){
//     char *str = get_time();
//     my_log("%s", str);
// }   