#ifndef KHEAP_H
#define KHEAD_H

#include "common.h"
#include "ordered_set.h"

typedef struct{
    u32int magic;
    u32int size;
    u8int is_hole;
}header_t;


typedef struct{
    u32int magic;
    header_t *header;
}footer_t;


typedef struct{
    ordered_set_t *ordered_table;
    u32int starting_address;
    u32int max_address;
    u32int end_address;
    u8int supervisor;
    u8int readonly;
}heap_t;

// some header values
#define KHEAP_START         0xC0000000
#define KHEAP_INITIAL_SIZE  0x100000
#define MAX_HEAP_INDEX_ENTRIES     0x20000
#define HEAP_MAGIC          0x123890AB
#define HEAP_MIN_SIZE       0x70000

// create new heap
heap_t *create_heap(u32int start, u32int end, u32int max, u8int supervisor, u8int readonly);

// alloc
void *alloc(u32int size, heap_t *heap, u8int page_align);

// free
void free(void *pointer, heap_t *heap);

// kernel wrapper apis
void *kmalloc(u32int sz);

u32int kmalloc_a(u32int sz);

void kfree(void *ptr);


#endif