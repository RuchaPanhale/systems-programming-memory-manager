/*
 * CS 551 Project "Memory manager".
 * This file needs to be turned in.	
 */

#include "memory_manager.h"

#define MEM_LIST_SIZE 6
static STRU_MEM_LIST *mem[MEM_LIST_SIZE];

/*
 * Print out the current status of the memory manager.
 * Reading this function may help you understand how the memory manager organizes the memory.
 * Do not change the implementation of this function. It will be used to help the grading.
 */
void mem_mngr_print_snapshot(void)
{
    STRU_MEM_LIST * mem_list = NULL;
    int i;

    printf("============== Memory snapshot ===============\n");

    for(i = 0; i < MEM_LIST_SIZE; i++) {
        mem_list = mem[i]; 
        if(mem_list == NULL) {
            return;
        }
        STRU_MEM_BATCH * mem_batch = mem_list->first_batch; // Get the first mem batch from the list 

        printf("mem_list %p slot_size %d batch_count %d free_slot_bitmap %p\n", 
               mem_list, mem_list->slot_size, mem_list->batch_count, mem_list->free_slots_bitmap);
        bitmap_print_bitmap(mem_list->free_slots_bitmap, mem_list->bitmap_size);

        while (NULL != mem_batch)
        {
            printf("\t mem_batch %p batch_mem %p\n", mem_batch, mem_batch->batch_mem);
            mem_batch = mem_batch->next_batch; // get next mem batch
        }
    }

    printf("==============================================\n");
}

/*
 * Initialize the memory manager
 * Initialize each list within the mem array with 1 batch of slots.
 */
void mem_mngr_init(void)
{
    for (int i = 0; i < MEM_LIST_SIZE; i++)
    {
        mem[i] = (STRU_MEM_LIST *)malloc(sizeof(STRU_MEM_LIST));
        if (mem[i] == NULL)
            return;

        mem[i]->slot_size = (i + 1) * MEM_ALIGNMENT_BOUNDARY;
        mem[i]->batch_count = 1;

        mem[i]->bitmap_size = (MEM_BATCH_SLOT_COUNT + BIT_PER_BYTE - 1) / BIT_PER_BYTE;
        mem[i]->free_slots_bitmap = (unsigned char *)malloc(mem[i]->bitmap_size);
        if (mem[i]->free_slots_bitmap == NULL)
            return;

        memset(mem[i]->free_slots_bitmap, 0xFF, mem[i]->bitmap_size);

        mem[i]->first_batch = (STRU_MEM_BATCH *)malloc(sizeof(STRU_MEM_BATCH));
        if (mem[i]->first_batch == NULL)
            return;

        mem[i]->first_batch->batch_mem = malloc(mem[i]->slot_size * MEM_BATCH_SLOT_COUNT);
        if (mem[i]->first_batch->batch_mem == NULL)
            return;

        mem[i]->first_batch->next_batch = NULL;
    }
}

/*
 * Clean up the memory manager (e.g., release all the memory allocated)
 */
void mem_mngr_leave(void)
{
    for (int i = 0; i < MEM_LIST_SIZE; i++)
    {
        if (mem[i] == NULL)
            continue;

        STRU_MEM_BATCH *curr = mem[i]->first_batch;
        while (curr != NULL)
        {
            STRU_MEM_BATCH *next = curr->next_batch;
            free(curr->batch_mem);
            free(curr);
            curr = next;
        }

        free(mem[i]->free_slots_bitmap);
        free(mem[i]);
        mem[i] = NULL;
    }
}


/*
 * Allocate a chunk of memory 	
 * @param size: size in bytes to be allocated
 * @return: the pointer to the allocated memory slot
 */

void * mem_mngr_alloc(size_t size)
{
    if (size == 0)
        return NULL;

    size_t aligned_size = SLOT_ALLINED_SIZE(size);

    if (aligned_size > MEM_LIST_SIZE * MEM_ALIGNMENT_BOUNDARY)
        return NULL;

    int list_index = (aligned_size / MEM_ALIGNMENT_BOUNDARY) - 1;
    STRU_MEM_LIST *list = mem[list_index];

    if (list == NULL)
        return NULL;

    int free_pos = bitmap_find_first_bit(list->free_slots_bitmap, list->bitmap_size, 1);

    if (free_pos == BITMAP_OP_ERROR)
    {
        STRU_MEM_BATCH *new_batch = (STRU_MEM_BATCH *)malloc(sizeof(STRU_MEM_BATCH));
        if (new_batch == NULL)
            return NULL;

        new_batch->batch_mem = malloc(list->slot_size * MEM_BATCH_SLOT_COUNT);
        if (new_batch->batch_mem == NULL)
        {
            free(new_batch);
            return NULL;
        }
        new_batch->next_batch = NULL;

        STRU_MEM_BATCH *curr = list->first_batch;
        while (curr->next_batch != NULL)
            curr = curr->next_batch;
        curr->next_batch = new_batch;

        int old_batch_count = list->batch_count;
        list->batch_count++;

        int old_bitmap_size = list->bitmap_size;
        int total_slots = list->batch_count * MEM_BATCH_SLOT_COUNT;
        int new_bitmap_size = (total_slots + BIT_PER_BYTE - 1) / BIT_PER_BYTE;

        unsigned char *new_bitmap = (unsigned char *)malloc(new_bitmap_size);
        if (new_bitmap == NULL)
            return NULL;

        memset(new_bitmap, 0, new_bitmap_size);
        memcpy(new_bitmap, list->free_slots_bitmap, old_bitmap_size);

        for (int i = old_batch_count * MEM_BATCH_SLOT_COUNT; i < total_slots; i++)
            bitmap_set_bit(new_bitmap, new_bitmap_size, i);

        free(list->free_slots_bitmap);
        list->free_slots_bitmap = new_bitmap;
        list->bitmap_size = new_bitmap_size;

        free_pos = bitmap_find_first_bit(list->free_slots_bitmap, list->bitmap_size, 1);
        if (free_pos == BITMAP_OP_ERROR)
            return NULL;
    }

    bitmap_clear_bit(list->free_slots_bitmap, list->bitmap_size, free_pos);

    int batch_index = free_pos / MEM_BATCH_SLOT_COUNT;
    int slot_index = free_pos % MEM_BATCH_SLOT_COUNT;

    STRU_MEM_BATCH *batch = list->first_batch;
    for (int i = 0; i < batch_index; i++)
    {
        if (batch == NULL)
            return NULL;
        batch = batch->next_batch;
    }

    if (batch == NULL)
        return NULL;

    return (void *)((char *)batch->batch_mem + slot_index * list->slot_size);
}



/*
 * Free a chunk of memory pointed by ptr
 * Print out any error messages
 * @param: the pointer to the allocated memory slot
 */
void mem_mngr_free(void * ptr)
{
    if (ptr == NULL)
        return;

    for (int i = 0; i < MEM_LIST_SIZE; i++)
    {
        STRU_MEM_LIST *list = mem[i];
        if (list == NULL)
            continue;

        STRU_MEM_BATCH *batch = list->first_batch;
        int batch_num = 0;

        while (batch != NULL)
        {
            char *batch_start = (char *)batch->batch_mem;
            char *batch_end = batch_start + list->slot_size * MEM_BATCH_SLOT_COUNT;
            char *target = (char *)ptr;

            if (target >= batch_start && target < batch_end)
            {
                int offset = (int)(target - batch_start);

                if (offset % list->slot_size != 0)
                {
                    printf("Error: ptr is not the starting address of any slot.\n");
                    return;
                }

                int slot_index = offset / list->slot_size;
                int global_pos = batch_num * MEM_BATCH_SLOT_COUNT + slot_index;

                int bit = bitmap_bit_is_set(list->free_slots_bitmap, list->bitmap_size, global_pos);

                if (bit == 1)
                {
                    printf("Error: double free detected.\n");
                    return;
                }

                bitmap_set_bit(list->free_slots_bitmap, list->bitmap_size, global_pos);
                return;
            }

            batch = batch->next_batch;
            batch_num++;
        }
    }

    printf("Error: ptr is outside of memory managed by the manager.\n");
}