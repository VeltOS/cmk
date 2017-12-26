/*
 * Dynamically allocating arrays
 * Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * MIT License
 */

#include "array.h"
#include <stdlib.h>
#include <string.h>

static const size_t kDefaultCapacity = 8;

typedef struct
{
    size_t elementSize, size, capacity;
    void (*freeElement)(void *element);
    char data[];
} Array;

void * array_new(size_t elementSize, ArrayFreeNotify freeElement)
{
    Array *arr = calloc(sizeof(Array) + (elementSize * kDefaultCapacity), 1);
    arr->elementSize = elementSize;
    arr->capacity = kDefaultCapacity;
	arr->freeElement = freeElement;
    return &arr->data;
}

static inline void * array_from_data(void *data)
{
    return data - sizeof(Array);
}

void array_free(void *data)
{
    if(data)
    {
        Array *arr = array_from_data(data);
        if(arr->freeElement)
            for(size_t i = 0; i < arr->size; ++i)
                arr->freeElement(data + (i * arr->elementSize));
        free(arr);
    }
}

void * array_reserve(void *data, size_t size, bool create)
{
    Array *arr = array_from_data(data);

    if(size > arr->capacity)
    {
        arr->capacity = size;
        arr = realloc(arr, sizeof(Array) + (arr->elementSize * arr->capacity));
    }
    
    if(create && size > arr->size)
        arr->size = size;
    
    return &arr->data;
}

void * array_append(void *data, void *element)
{
    Array *arr = array_from_data(data);
    
    if((arr->size + 1) > arr->capacity)
    {
        arr->capacity *= 2;
        if(arr->capacity < arr->size + 1)
            arr->capacity = arr->size + 1;
        arr = realloc(arr, sizeof(Array) + (arr->elementSize * arr->capacity));
    }
    
    void *pos = arr->data + (arr->elementSize * arr->size);
    if(element)
        memcpy(pos, element, arr->elementSize);
    else
        memset(pos, 0, arr->elementSize);
    
    ++arr->size;
    return &arr->data;
}

//void * array_insert(void *data, void *element, size_t index)
//{
//}

void array_remove(void *data, size_t index, bool free)
{
    Array *arr = array_from_data(data);

    if(index >= arr->size)
    {
        printf("Attempted to remove index %lu from array of size %lu\n", index, arr->size);
        return;
    }
    
    void *p = data + (arr->elementSize * index);
    if(free && arr->freeElement)
        arr->freeElement(p);
    
    // If we're not removing the last element,
    // shift everything down by 1.
    if(index < arr->size - 1)
        memmove(p, p + arr->elementSize, (arr->size - index - 1) * arr->elementSize);
    
    --arr->size;
}

void array_shrink(void *data, size_t size, bool free)
{
    Array *arr = array_from_data(data);

    if(arr->size <= size)
        return;
    
    if(free && arr->freeElement)
    {
        for(size_t i = size; i < arr->size; ++i)
            arr->freeElement(data + (i * arr->elementSize));
    }
    
    arr->size = size;
}

size_t array_size(void *data)
{
    Array *arr = array_from_data(data);
    return arr->size;
}
