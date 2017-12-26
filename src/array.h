/*
 * Dynamically allocating arrays
 * Copyright 2018 Aidan Shafran <zelbrium@gmail.com>
 * MIT License
 */

#ifndef array_h
#define array_h

#include <stdio.h>
#include <stdbool.h>

typedef void (*ArrayFreeNotify)(void *element);

/*
 * Returns an array that can be casted to the desired
 * element type. For example, if it is an array of 32-bit
 * its, elementSize = 4, and the return can be casted
 * to int32_t *.
 *
 * Only free this with array_free.
 */
__attribute__((warn_unused_result))
void * array_new(size_t elementSize, ArrayFreeNotify freeElement);

/*
 * Frees an array returned by array_new.
 * Calls freeElement on each element.
 */
void array_free(void *array);

/*
 * Increases the capacity of the array to at least size
 * elements without changing the array length. Use this to
 * pre-allocate space if the number of elements to append
 * is known in advance. Set create = true to also increase
 * the array's length to match its new capacity (does not
 * zero the new space). This should always be called as
 * a = array_reserve(a, size);
 */
__attribute__((warn_unused_result))
void * array_reserve(void *data, size_t size, bool create);

/*
 * Appends element to array, and returns a pointer
 * to the array. This should always be called as
 * a = array_append(a, elem);
 */
__attribute__((warn_unused_result))
void * array_append(void *array, void *element);

/*
 * Inserts element to array at index, and returns a
 * pointer to the array. This should always be called as
 * a = array_append(a, elem);
 */
//void * array_insert(void *array, void *element, size_t index);

/*
 * Removes the element at index in array, reducing the
 * array's size by 1. free = true to call freeElement
 * on the removed element.
 */
void array_remove(void *array, size_t index, bool free);

/*
 * Reduces the array's size to size elements, optionally
 * freeing the elements. If size is larger than the current
 * size, nothing happens.
 */
void array_shrink(void *array, size_t size, bool free);

/*
 * Returns the size of the array.
 */
size_t array_size(void *array);

#endif /* array_h */
