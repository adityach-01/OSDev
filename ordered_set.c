#include "ordered_set.h"
#include "heap.h"

// implement this function in kheap.c as per the requirement
// s8int standard_less_than_function(type_t a, type_t b);

// creation and placcement of ordered set
ordered_set_t create_ordered_set(int max_size, lessthan_function_t less_than){

    ordered_set_t ret;

    ret.array = kmalloc(max_size * sizeof(type_t));
    memset(ret.array, 0, max_size * sizeof(type_t));
    ret.size = 0;  // 0 free holes in the array
    ret.less_than = less_than;
    ret.max_size = max_size;

    return ret; 
}

void place_ordered_set(ordered_set_t *ptr, void *addr, int max_size, lessthan_function_t less_than){
    // places the array in the ordered set at the specified address
    
    ordered_set_t ret;
    ptr->array = (type_t *) addr;
    memset((u8int *)ptr->array, 0, sizeof(max_size * sizeof(type_t)));

    ptr->size = 0;
    ptr->less_than = less_than;
    ptr->max_size = max_size;
}

// destroying ordered set
void destroy_ordered_set(ordered_set_t *array){
    // kfree(array->array);
}

// inserting an element into ordered set
void insert_into_ordered_set(ordered_set_t *array, type_t item){
    int len = array->size;

    // find first index greater than item
    int index = 0;
    while(index < array->size && !array->less_than(item, array->array[index])) index++;

    // shift the elements from index to the next element and insert the item at index
    type_t cur = array->array[index];

    for(int i = index; i < len; i++){
        type_t temp = array->array[i+1];
        array->array[i+1] = cur;
        cur = temp;
    }

    array->array[index] = item;
    array->size++;
    
}

// removing index i from the ordered set
void remove_from_ordered_set(ordered_set_t *array, int index){
    // shift to i <-- i+1 from i from index to len-1;
    int len = array->size;
    
    for (int i = index; i < len; i++)
    {
        array->array[index] = array->array[index+1];
    }

    array->size--;
    
}

// get the item at index i
type_t find_in_ordered_set(ordered_set_t *array, int index){
    if(index < array->size) return array->array[index];
    else return 0;
}