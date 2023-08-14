#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>


///******* definitions for list **********///
int cookie_val = rand();
typedef struct malloc_meta_data {
    int cookie;
    size_t size;
    bool is_free;
    malloc_meta_data* add_next;
    malloc_meta_data* add_prev;
    malloc_meta_data* size_next;
    malloc_meta_data* size_prev;
} MallocMetadata;
///new file

MallocMetadata* block_list = nullptr;
MallocMetadata* first_by_size = nullptr;

///********* helper functions *********///
void is_cookie_okay(MallocMetadata* block){
    if(block == nullptr){
        return;
    }
    if(block->cookie != cookie_val){
        exit(0xdeadbeef);
    }
}

size_t _num_free_blocks()
{
    MallocMetadata* current_block = block_list;

    size_t num = 0;
    while (current_block != nullptr)
    {
        is_cookie_okay(current_block);
        if (current_block->is_free)
        {
            num++;
        }
        current_block = current_block->add_next;
    }
    return num;
}

size_t _num_free_bytes()
{
    MallocMetadata* current_block = block_list;
    size_t num = 0;
    while (current_block != nullptr)
    {
        is_cookie_okay(current_block);
        if (current_block->is_free)
        {
            num+= current_block->size;
        }
        current_block = current_block->add_next;
    }
    return num;
}

size_t _num_allocated_blocks()
{
    if(block_list == nullptr){
        return 0;
    }
    MallocMetadata* current_block = block_list;
    size_t num = 0;
    while (current_block != nullptr)
    {
        is_cookie_okay(current_block);
        if (current_block->size == 0)
        {
            current_block = current_block->add_next;
            continue;
        }
        num++;
        current_block = current_block->add_next;
    }
    return num;
}

size_t _num_allocated_bytes()
{
    MallocMetadata* current_block = block_list;
    size_t num = 0;
    while (current_block != NULL)
    {
        is_cookie_okay(current_block);
        num += current_block->size;
        current_block = current_block->add_next;
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


MallocMetadata* get_first_md_by_size() {
//    if (block_list == nullptr)
//    {
//        return nullptr;
//    }
//    MallocMetadata* current_block = block_list;
//    is_cookie_okay(current_block);
//    while (current_block->size_prev != nullptr)
//    {
//        is_cookie_okay(current_block);
//        current_block = current_block->size_prev;
//    }
//    return current_block;
    return first_by_size;
}

void remove_from_list(MallocMetadata* block){ ///only for mmap
    is_cookie_okay(block);
    if(block->add_next == nullptr && block->add_prev == nullptr){
        block_list = nullptr;
        return;
    }
    else if(block->add_prev == nullptr){
        is_cookie_okay(block->add_next);
        block->add_next->add_prev = nullptr;
        block_list = block->add_next;
        return;
    }
    else if(block->add_next == nullptr){
        is_cookie_okay(block->add_prev);
        block->add_prev->add_next = nullptr;
        return;
    }
    else{
        is_cookie_okay(block->add_next);
        is_cookie_okay(block->add_prev);
        block->add_prev->add_next = block->add_next;
        block->add_next->add_prev = block->add_prev;
        return;
    }
}

void add_to_list_by_size(MallocMetadata* new_block){
    is_cookie_okay(new_block);
    size_t size = new_block->size;
    MallocMetadata* first_md_by_size = get_first_md_by_size();
    is_cookie_okay(first_md_by_size);
    if(first_md_by_size == nullptr){
        first_by_size = new_block;
        new_block->size_next = nullptr;
        new_block->size_prev = nullptr;
        return;
    }
    if (first_md_by_size->size > size) //new_block should be first in size
    {
        first_md_by_size->size_prev = new_block;
        new_block->size_next = first_md_by_size;
        new_block->size_prev = nullptr;
        first_by_size = new_block;
        return;
    }

    MallocMetadata* current_block_size = first_md_by_size;
    MallocMetadata* prev_block_size = current_block_size;

    while(current_block_size != NULL && current_block_size->size < size){
        is_cookie_okay(current_block_size);
        prev_block_size = current_block_size;
        current_block_size = current_block_size->size_next;
    }
    is_cookie_okay(new_block);
    is_cookie_okay(prev_block_size);
    is_cookie_okay(current_block_size);
    if (current_block_size == nullptr)
    {
        prev_block_size->size_next = new_block;
        new_block->size_prev = prev_block_size;
        new_block->size_next = nullptr;
    }
    else if (current_block_size->size == size)
    {
        if (new_block < current_block_size) //before current
        {
            new_block->size_prev = prev_block_size;
            current_block_size->size_prev = new_block;
            new_block->size_next = current_block_size;
            prev_block_size->size_next = new_block;
            return;
        }
        //after current
        while(current_block_size->size_next != nullptr && current_block_size->size_next->size == current_block_size->size && new_block > current_block_size->size_next) //by size then by address
        {
            is_cookie_okay(current_block_size); /////to continue checking cookie if needed
            prev_block_size = current_block_size;
            current_block_size = current_block_size->size_next;
        }
        if(current_block_size->size_next == nullptr){
            current_block_size->size_next = new_block;
            new_block->size_prev = current_block_size;
            new_block->size_next = nullptr;
            return;
        }
        is_cookie_okay(current_block_size->size_next);
        new_block->size_next = current_block_size->size_next;
        current_block_size->size_next = new_block;
        new_block->size_prev = current_block_size;
        current_block_size->size_next->size_prev = new_block;
        return;
    }
    else /// its bigger than prev but smaller the current
    {
        is_cookie_okay(current_block_size);
        is_cookie_okay(new_block->add_next);
        new_block->size_prev = prev_block_size;
        current_block_size->size_prev = new_block;
        new_block->size_next = current_block_size;
        prev_block_size->size_next = new_block;
        return;
    }
    return;
}

void remove_from_list_by_size(MallocMetadata* block){ //check what about block_list?
    is_cookie_okay(block);
    MallocMetadata* prev_block_size = block->size_prev;
    if (prev_block_size == nullptr && block->size_next != nullptr) //block is the first in size
    {
        is_cookie_okay(block->size_next);
        block->size_next->size_prev = nullptr; //now block->size_next is the first by size
        first_by_size = block->size_next;
        return;
    }
    if(prev_block_size == nullptr && block->size_next == nullptr){
        first_by_size = nullptr;
        return;
    }
    is_cookie_okay(prev_block_size);
    prev_block_size->size_next = block->size_next;
    if (block->size_next != nullptr)
    {
        is_cookie_okay(block->size_next);
        block->size_next->size_prev = prev_block_size;
    }
    return;
}

MallocMetadata* combine_with_next_add(MallocMetadata* block){
    is_cookie_okay(block);
    if (block->add_next == NULL || block->add_next->is_free == false)
    {
        return block; //maybe return null to know that didnt do anything?
    }
    MallocMetadata* next_add_block = block->add_next;
    is_cookie_okay(next_add_block);
    size_t new_size = block->size + next_add_block->size + _size_meta_data(); //last because we delete the second MD
    block->size = new_size;
    block->add_next = next_add_block->add_next;
    if (next_add_block->add_next != NULL)
    {
        next_add_block->add_next->add_prev = block;
    }
    remove_from_list_by_size(next_add_block);
    remove_from_list_by_size(block);
    add_to_list_by_size(block);
    return block;
}

MallocMetadata* combine_with_prev_add(MallocMetadata* block){
    is_cookie_okay(block);
    if (block->add_prev == nullptr || block->add_prev->is_free == false)
    {
        return block; //same here
    }
    MallocMetadata* prev_add_block = block->add_prev;
    block->is_free = true;
    return combine_with_next_add(prev_add_block);
}

MallocMetadata* combine_next_and_prev_add(MallocMetadata* block){
    return combine_with_next_add(combine_with_prev_add(block)); //hope this is okay. maybe like in matam to send derefernce??
}

void split_block(MallocMetadata* block_to_split, size_t size){
    is_cookie_okay(block_to_split);
    if(block_to_split->size < 128 + _size_meta_data() + size){
        ///not large enough
        return;
    }
    MallocMetadata* new_block = (MallocMetadata*)((char*)block_to_split + _size_meta_data() + size);
    new_block->size = block_to_split->size - size - _size_meta_data();
    new_block->add_next = block_to_split->add_next;
    new_block->add_prev = block_to_split; ///check if needs to cast
    new_block->is_free = true;
    new_block->cookie = cookie_val;
    combine_with_next_add(new_block);
    block_to_split->size = size;
    block_to_split->add_next = new_block;
    if (block_to_split->add_next != nullptr)
    {
        block_to_split->add_next->add_prev = new_block;
    }
    add_to_list_by_size(new_block);
    remove_from_list_by_size(block_to_split);
    add_to_list_by_size(block_to_split);
    return;
}

bool do_mmap(size_t size){
    return size >= 128 * 1024;
}


bool is_mmapped(MallocMetadata* block){ //for srealloc
    is_cookie_okay(block);
    return do_mmap(block->size);
}



MallocMetadata* get_wilderness_block()
{
    MallocMetadata* current_block = block_list;
    is_cookie_okay(current_block);
    if(current_block == nullptr)
    {
        return nullptr;
    }
    while (current_block->add_next != nullptr)
    {
        is_cookie_okay(current_block);
        current_block = current_block->add_next;
    }
    return current_block;
}
void add_to_list_by_address(MallocMetadata* new_block){
    is_cookie_okay(new_block);
    if(block_list == nullptr){
        block_list = new_block;
        new_block->add_next = nullptr;
        new_block->size_next = nullptr;
        new_block->add_prev = nullptr;
        new_block->size_prev = nullptr;
        return;
    }
    MallocMetadata* current_block = block_list;
    MallocMetadata* prev_block = current_block;
    while(current_block != nullptr && current_block < new_block){
        is_cookie_okay(current_block);
        prev_block = current_block;
        current_block = current_block->add_next;
    }
    is_cookie_okay(prev_block);
    is_cookie_okay(current_block);
    if(current_block == nullptr){
        prev_block->add_next = new_block;
        new_block->add_prev = prev_block;
        new_block->add_next = nullptr;
        return;
    }
    else{
        prev_block->add_next = new_block;
        new_block->add_prev = prev_block;
        new_block->add_next = current_block;
        current_block->add_prev = new_block;
        return;
    }
}

///********* main functions *********///

void* smalloc(size_t size)
{
    if (size == 0 || size > pow(10,8))
    {
        return NULL;
    }
    if (do_mmap(size))
    {
        void* address = mmap(nullptr, size + _size_meta_data(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(address == (void*)(-1)){
            return nullptr;
        }
        MallocMetadata* add_meta_data = (MallocMetadata*)address;
        add_meta_data->size = size;
        add_meta_data->is_free = false;
        add_meta_data->cookie = cookie_val;
        add_to_list_by_address(add_meta_data);
        add_to_list_by_size(add_meta_data);
        return (void*)((char*)address + _size_meta_data());
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
        block_list->cookie = cookie_val;
        block_list->size_prev = nullptr;
        block_list->size_next = nullptr;
        block_list->add_next = nullptr;
        block_list->add_prev = nullptr;
        block_list->is_free = false;
        first_by_size = program_break;
        return (void*)((char*)block_list + _size_meta_data()); //why char*????
    }

    MallocMetadata* current_block_size = get_first_md_by_size();
    while (current_block_size != NULL)
    {
        is_cookie_okay(current_block_size);
        if (current_block_size->size >= size && current_block_size->is_free) //there is already metadata
        {
            current_block_size->is_free = false;
            split_block(current_block_size, size);

            return (void*)((char*)current_block_size + _size_meta_data());
        }
        current_block_size = current_block_size->size_next; //iterating on block list
    }
    //no free block was large enough. ch 3
    MallocMetadata* wilderness = get_wilderness_block(); //must not be null
    is_cookie_okay(wilderness);
    if (wilderness->is_free) //enlarge wilderness
    {
        size_t new_size = size - wilderness->size;
        void* program_break = sbrk(new_size);
        if (program_break == (void*)(-1)) //sbrk failed
        {
            return NULL;
        }
        wilderness->size+=new_size;
        wilderness->is_free = false;
        remove_from_list_by_size(wilderness);
        add_to_list_by_size(wilderness);
        return (void*)((char*)wilderness + _size_meta_data());
    }

    MallocMetadata* prev_block_add = wilderness;
    MallocMetadata* new_block = (MallocMetadata*)sbrk(total_size);
    if (new_block == (void*)(-1)) //sbrk failed
    {
        return NULL;
    }

    new_block->size = size;
    new_block->cookie = cookie_val;
    new_block->is_free = false;
    prev_block_add->add_next = new_block;
    new_block->add_next = NULL;
    new_block->add_prev = prev_block_add;
    add_to_list_by_size(new_block);

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
    MallocMetadata* p_data = (MallocMetadata*)((char*)p - _size_meta_data());
    is_cookie_okay(p_data);
    if(is_mmapped(p_data)){
        remove_from_list(p_data);
        munmap((void*)p_data, p_data->size + _size_meta_data());
        return;
    }
    p_data->is_free = true;
    combine_next_and_prev_add(p_data);
    return;
}

void* srealloc(void* oldp, size_t size) //check if oldp is start of meta_data or start of address /////////////////////i didnt change anything here
{
    if(size == 0 || size > pow(10,8)){
        return NULL;
    }
    if(oldp == NULL){
        return smalloc(size);
    }
    MallocMetadata* oldp_data = (MallocMetadata*)((char*)oldp - _size_meta_data());
    is_cookie_okay(oldp_data);
    size_t orig_size = oldp_data->size;
    ///a. Try to reuse the current block without any merging

    if(orig_size >= size){
        split_block(oldp_data, size);
        return oldp;
    }

    ///b. Try to merge with the adjacent block with the lower address.
    is_cookie_okay(oldp_data->add_prev);
    if(oldp_data->add_prev != nullptr && oldp_data->add_prev->is_free && oldp_data->add_prev->size + orig_size + _size_meta_data() >= size) {
        MallocMetadata* address = combine_with_prev_add(oldp_data);
        is_cookie_okay(address);
        address->is_free = false;
        memmove((void*)((char*)address + _size_meta_data()), oldp, orig_size);
        split_block(address, size);
        address->is_free = false;
        return (void*)((char*)address + _size_meta_data());
    }

    ///c. If the block is the wilderness chunk, enlarge it.
    if(oldp_data->add_next == nullptr){
        MallocMetadata* address = combine_with_prev_add(oldp_data);
        is_cookie_okay(address);
        size_t size_to_add = size - address->size;
        if ((MallocMetadata*)sbrk(size_to_add) == (void*)(-1)) //sbrk failed
        {
            return NULL;
        }
        address->size = size;
        address->is_free = false;
        remove_from_list_by_size(address);
        add_to_list_by_size(address);
        memmove((void*)((char*)address + _size_meta_data()), oldp, oldp_data->size);
        split_block(address, size);
        address->is_free = false;
        return (void*)((char*)address + _size_meta_data());
    }
    ///d. Try to merge with the adjacent block with the higher address.
    is_cookie_okay(oldp_data->add_next);
    if(oldp_data->add_next->is_free && oldp_data->add_next->size + orig_size + _size_meta_data() >= size){
        MallocMetadata* address = combine_with_next_add(oldp_data);
        memmove((void*)((char*)address + _size_meta_data()), oldp, oldp_data->size);
        split_block(address, size);
        address->is_free = false;
        return (void*)((char*)address + _size_meta_data());
    }
    ///e. Try to merge all those three adjacent blocks together.
    if(oldp_data->add_next->is_free && oldp_data->add_prev->is_free
       && orig_size + oldp_data->add_prev->size + oldp_data->add_next->size + 2*_size_meta_data() >= size){
        MallocMetadata* address = combine_next_and_prev_add(oldp_data);
        memmove((void*)((char*)address + _size_meta_data()), oldp, oldp_data->size);
        split_block(address, size);
        address->is_free = false;
        return (void*)((char*)address + _size_meta_data());
    }
    /** f. If the wilderness chunk is the adjacent block with the higher address:

    i. Try to merge with the lower and upper blocks (such as in e), and enlarge the
    wilderness block as needed.
    ii. Try to merge only with higher address (the wilderness chunk), and enlarge it as
    needed. */
    if(oldp_data->add_next->add_next == nullptr && oldp_data->add_next->is_free) {
        MallocMetadata *address = combine_next_and_prev_add(oldp_data);
        is_cookie_okay(address);
        size_t size_to_add = size - address->size;
        if ((MallocMetadata*)sbrk(size_to_add) == (void*)(-1)) //sbrk failed
        {
            return NULL;
        }
        address->size = size;
        address->is_free = false;
        remove_from_list_by_size(address);
        add_to_list_by_size(address);
        memmove((void*)((char*)address + _size_meta_data()), oldp, oldp_data->size);
        return (void*)((char*)address + _size_meta_data());
    }

    /** g. Try to find a different block that’s large enough to contain the request (don’t forget
    that you need to free the current block, therefore you should, if possible, merge it
    with neighboring blocks before proceeding). */
    sfree(oldp);
    void* address = smalloc(size);
    if(address == NULL){
        return NULL;
    }
    memmove(address, oldp, orig_size);
    return address;
}


