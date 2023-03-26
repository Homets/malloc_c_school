#define _GNU_SOURCE
#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "my_secmalloc.h"
#include "my_secmalloc_private.h"

Test(mmap, test_simple_mmap)
{
    printf("Ici on fait un test simple de mmap\n");
    void *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON , -1, 0);
    cr_expect(ptr != NULL);
    int res = munmap(ptr, 4096);
    cr_expect(res == 0);
}


Test(log, test_log, .init=cr_redirect_stderr)
{   
    my_log("coucou %d\n",12);
    cr_assert_stderr_eq_str("coucou 12\n");

}

Test(metadata_pool, create_clean_metadata_pool)
{   
    my_init_metadata_pool();
    my_log("%p\n", metadata_pool);
}

Test(data_pool, create_clean_data_pool)
{
    my_init_metadata_pool();
    my_init_data_pool();
    my_log("%p\n", data_pool);
    cr_assert(metadata_pool != NULL);
    cr_assert(data_pool != NULL);
    cr_assert(clean_metadata_pool() == 0);
    cr_assert(clean_data_pool() == 0);
}

Test(metadata_pool, test_all_metadata_pool)
{
    my_init_metadata_pool();
    int i = 0;
    struct metadata *m = metadata_pool;
    while( i < 5)
    {
        my_log("next => %p\n", m->next);
        my_log("size => %d\n\n", m->block_size);
        i = i + 1;
        m = m->next;
    }
    cr_assert(clean_metadata_pool() == 0);
}


Test(metadata_pool, check_metadatablock_edited)
{
    my_init_metadata_pool();
    my_init_data_pool();
    my_log("ptr de data %p\n", data_pool);
    void **ptr;
    void **ptr1;
    ptr = my_malloc(8);
    my_log("pointeur give => %p\n", ptr);
    ptr1 = my_malloc(8);
    my_log("\n\nptr => data2 %p\n", ptr1);

    cr_assert(clean_metadata_pool() == 0);
    cr_assert(clean_data_pool() == 0);
}

Test(data_pool, check_canary_well_written)
{
    my_init_metadata_pool();
    my_init_data_pool();
    size_t sz_data = 8;
    char *ptr = my_malloc(sz_data);

    for (int i = 0; i < 8;i++)
    {
        char s = ptr[sz_data + i];
        my_log("%c", s);
        cr_assert(s == 'X');
    }

    cr_assert(clean_metadata_pool() == 0);
    cr_assert(clean_data_pool() == 0);
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
    printf("%d", res);
}

Test(data_pool, test_reallocation_of_data_pool)
{
    my_init_metadata_pool();
    my_init_data_pool();
    void *ptr;
    ptr = my_malloc(314405);
    my_log("\n\nptr datadata => %p\n\n", ptr);

    cr_assert(clean_metadata_pool() == 0);
    cr_assert(clean_data_pool() == 0);
}

