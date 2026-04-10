#include <stdio.h>
#include "memory_manager.h"

int main()
{
    mem_mngr_init();

    void *ptrs[17];

    for (int i = 0; i < 17; i++)
    {
        ptrs[i] = mem_mngr_alloc(8);
        printf("ptrs[%d] = %p\n", i, ptrs[i]);
    }

    mem_mngr_print_snapshot();

    mem_mngr_leave();
    return 0;
}