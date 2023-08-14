#include <unistd.h>
#include <math.h>

void* smalloc(size_t size)
{
    if (size == 0 || size > pow(10,8))
    {
        return NULL;
    }
    void* program_break = sbrk(sizeof(char)*size);
    if (program_break == (void*)(-1)) //sbrk failed
    {
        return NULL;
    }
    return program_break;
}
