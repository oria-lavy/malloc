#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

///******* definitions for list **********///
typedef struct malloc_meta_data {
    size_t size;
    bool is_free;
    malloc_meta_data* next;
    malloc_meta_data* prev;
} MallocMetadata;

MallocMetadata* block_list = NULL;

///********* helper functions *********///
size_t _num_free_blocks()
{
    MallocMetadata* current_block = block_list;
    size_t num = 0;
    while (current_block != NULL)
    {
        if (current_block->is_free)
        {
            num++;
        }
        current_block = current_block->next;
    }
    return num;
}

size_t _num_free_bytes()
{
    MallocMetadata* current_block = block_list;
    size_t num = 0;
    while (current_block != NULL)
    {
        if (current_block->is_free)
        {
            num+= current_block->size;
        }
        current_block = current_block->next;
    }
    return num;
}

size_t _num_allocated_blocks()
{
    MallocMetadata* current_block = block_list;
    size_t num = 0;
    while (current_block != NULL)
    {
        num++;
        current_block = current_block->next;
    }
    return num;
}

size_t _num_allocated_bytes()
{
    MallocMetadata* current_block = block_list;
    size_t num = 0;
    while (current_block != NULL)
    {
        num+= current_block->size;
        current_block = current_block->next;
    }
    return num;
}

size_t _num_meta_data_bytes()
{
    return _num_allocated_blocks()*sizeof(MallocMetadata);
}

size_t _size_meta_data()
{
    return sizeof(MallocMetadata);
}



///********* main functions *********///

void* smalloc(size_t size)
{
    if (size == 0 || size > pow(10,8))
    {
        return NULL;
    }
    int total_size = size + _size_meta_data(); //size of entire block
    if (block_list == NULL) //NO PREV FREED BLOCKS
    {
        MallocMetadata* program_break = (MallocMetadata*)sbrk(total_size);
        if (program_break == (void*)(-1)) //sbrk failed
        {
            return NULL;
        }
        block_list = program_break;
        block_list->size = size;
        block_list->is_free = false;
        block_list->next = NULL;
        block_list->prev = NULL;
        return (void*)((char*)program_break + _size_meta_data()); //why char*????
    }

    //there are prev allocated blocks. find the first that fits
    MallocMetadata* current_block = block_list;
    MallocMetadata* prev_block = current_block; //iterating
    while (current_block != NULL)
    {
        if (current_block->is_free && current_block->size >= size) //there is already metadata
        {
            current_block->is_free = false; //use this block
            return (void*)((char*)current_block + _size_meta_data());
        }
        prev_block = current_block;
        current_block = current_block->next; //iterating on block list
    }

    //no prev allocated was big enough and free. must use sbrk
    MallocMetadata* new_block = (MallocMetadata*)sbrk(total_size);
    if (new_block == (void*)(-1)) //sbrk failed
    {
        return NULL;
    }
    new_block->size = size;
    new_block->is_free = false;
    new_block->prev = prev_block;
    new_block->next = NULL;
    prev_block->next = new_block;
    return (void*)((char*)new_block + _size_meta_data());
}

void* scalloc(size_t num, size_t size) //std::memset
{
    if (size == 0 || num == 0 || size*num > pow(10,8))
    {
        return NULL;
    }
    void* address = smalloc(size*num);
    if(address == NULL){
        return NULL;
    }
    memset(address, 0, num * size);
    return address;
}

void sfree(void* p)
{
    if(p == NULL){
        return;
    }
    MallocMetadata* p_data = (MallocMetadata*)((char*)p - sizeof(MallocMetadata));
    p_data->is_free = true;
    return;
}

void* srealloc(void* oldp, size_t size) //check if oldp is start of meta_data or start of address
{
    if(size == 0 || size > pow(10,8)){
        return NULL;
    }
    if(oldp == NULL){
        return smalloc(size);
    }
    MallocMetadata* oldp_data = (MallocMetadata*)((char*)oldp - sizeof(MallocMetadata));
    if(oldp_data->size >= size){
        return oldp;
    }
    void* address = smalloc(size);
    if(address == NULL){
        return NULL;
    }
    memmove(address, oldp, oldp_data->size);
    sfree(oldp);
    return address;
}

