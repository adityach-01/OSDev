#ifndef ORDERED_SET_H
#define ORDERED_SET_H

#include "common.h"

typedef void* type_t;

// defining less_tha function predicate
typedef s8int (*lessthan_function_t)(type_t, type_t);

typedef struct{
    type_t *array;  // arrao of pointers to the header of the free blocks or holes
    int size;
    int max_size;
    lessthan_function_t less_than;
}ordered_set_t;

s8int standard_less_than_function(type_t a, type_t b);

// creation and placcement of ordered set
ordered_set_t create_ordered_set(int max_size, lessthan_function_t less_than);
void place_ordered_set(ordered_set_t *ptr, void *addr, int max_size, lessthan_function_t less_than);

// destroying ordered set
void destroy_ordered_set(ordered_set_t *array);

// inserting an element into ordered set
void insert_into_ordered_set(ordered_set_t *array, type_t item);

// removing index i from the ordered set
void remove_from_ordered_set(ordered_set_t *array, int index);

// get the item at index i
type_t find_in_ordered_set(ordered_set_t *array, int index);


#endif