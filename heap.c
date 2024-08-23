#include "common.h"
#include "heap.h"

int placement_address = 0x800000;

// returns the physical address
u32int kmalloc(u32int sz){
    int temp = placement_address;
    placement_address += sz;

    return temp;
}

u32int kmalloc_a(u32int sz){
    if(placement_address & 0xFFF){
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }

    int temp = placement_address;
    placement_address += sz;

    return temp;
}