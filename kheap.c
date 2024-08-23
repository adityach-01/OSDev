#include "kheap.h"
#include "frame_allocator.h"
#include "paging.h"
#include "monitor.h"

extern page_directory_t *kernel_directory;
heap_t *kheap;
u32int placement_address = 0x105c88;

static int find_smallest_hole_index(u32int size, heap_t *heap, u8int page_align)
{
    // finds the smallest hold index which is greater than equal to the size
    // or which can accomodate our requested size

    int len = heap->ordered_table->size;
    int cur_head_sz = __INT_MAX__;
    int cur_head_id = -1;

    for (int i = 0; i < len; i++)
    {
        header_t *head = find_in_ordered_set(heap->ordered_table, i);

        // is this a suitable head, is its size greater than the requested size??
        if (page_align)
        {
            u32int block_start = (u32int)(head) + sizeof(header_t);
            u32int offset = 0;

            if (block_start % 0x1000 != 0)
            {
                offset = 0x1000 - block_start % 0x1000;
                block_start += offset;
            }

            if (head->size - offset >= size)
            {
                cur_head_id = i;
                break;
            }
        }
        else
        {
            if (head->size >= size)
            {
                cur_head_id = i;
            }
        }
    }

    return cur_head_id;
}

static s8int header_size_comparator(type_t a, type_t b)
{
    // here a and b are pointers to the headers of the holes
    header_t *a1 = (header_t *)a;
    header_t *a2 = (header_t *)b;

    return a1->size < a2->size ? 1 : 0;
}


heap_t *create_heap(u32int start_addr, u32int end_addr, u32int max, u8int supervisor, u8int readonly)
{   
    u32int place_old = placement_address;
    heap_t *new_heap = (heap_t *)kmalloc(sizeof(heap_t));

    ordered_set_t *new_set = (ordered_set_t *)kmalloc(sizeof(ordered_set_t));

    u32int place_new = placement_address;

    // assign pages from before to after
    for(int i = place_old; i <= place_new; i += 0x1000){
        alloc_frame(get_page(i,1,kernel_directory),1,1);
    }

    // make the start and end address page-aligned
    if (start_addr % 0x1000 != 0)
    {
        start_addr += 0x1000 - start_addr % 0x1000;
    }

    if (end_addr % 0x1000 != 0)
    {
        end_addr -= end_addr % 0x1000;
    }

    place_ordered_set(new_set, (void *)start_addr, MAX_HEAP_INDEX_ENTRIES, header_size_comparator);
    new_heap->ordered_table = new_set;

    
    start_addr += MAX_HEAP_INDEX_ENTRIES * sizeof(type_t);
    // again check for alignment

    if (start_addr % 0x1000 != 0)
    {
        start_addr += 0x1000 - start_addr % 0x1000;
    }

    // update the heap_t structure
    new_heap->starting_address = start_addr;
    new_heap->end_address = end_addr;
    new_heap->readonly = readonly;
    new_heap->max_address = max;
    new_heap->supervisor = supervisor;

    // update the index in the heap structure with one large hole
    // set the header
    // is should be noted that even is start_addr VA is not mapped into PA, accessing new_header will create a page fault which would
    // cause OS to create its mapping in the physical address space. Same goes for footer

    // monitor_write_hex((u32int)(new_heap->max_address));

    // monitor_write("Wow!");
    // monitor_put('\n');
    // Hence in heap, only header and footer of the block will have mapping into PA space in the beginning

    // monitor_write_hex((u32int)new_heap->starting_address);
    // monitor_put('\n');
    // monitor_write_hex((u32int)new_heap->end_address);
    // monitor_put('\n');

    // monitor_write("Size of header and footer - ");
    // monitor_write_dec(sizeof(header_t));
    // monitor_put('\n');
    // monitor_write_dec(sizeof(footer_t));
    // monitor_put('\n');

    header_t *new_header = (header_t *)start_addr;
    new_header->is_hole = 1;
    new_header->magic = HEAP_MAGIC;
    new_header->size = end_addr - start_addr - sizeof(header_t) - sizeof(footer_t);

    // set the footer
    footer_t *footer = (footer_t *)(end_addr - sizeof(footer_t));
    footer->header = new_header;
    footer->magic = HEAP_MAGIC;

    monitor_write("Footer addr : ");
    monitor_write_hex((u32int)footer);
    monitor_put('\n');

    insert_into_ordered_set(new_heap->ordered_table, (void *)new_header);
    
    return new_heap;
}

// add some entries to heap index also needs to be done
static void expand_heap(u32int new_size, heap_t *heap)
{
    // make sure that new address is page alignes
    new_size = new_size - new_size % 0x1000;

    // sanity checks
    u32int old_size = heap->end_address - heap->starting_address;
    if (heap->starting_address + new_size > heap->max_address)
        new_size = heap->max_address - heap->starting_address;
    if (new_size <= old_size)
        return;

    u32int additional_addr = heap->end_address;
    heap->end_address = heap->starting_address + new_size;

    // create a mapping for these addresses as well
    while (additional_addr < heap->end_address)
    {
        alloc_frame(get_page(additional_addr, 1, kernel_directory), (heap->supervisor) ? 1 : 0, (heap->readonly) ? 0 : 1);
        additional_addr += 0x1000;
    }

    return;
}

// remove some entries from heap index also needs to be done
static u32int contract_heap(u32int new_size, heap_t *heap)
{
    // sanity checks
    u32int offset = 0x1000 - new_size % 0x1000;
    new_size = new_size + offset;

    u32int old_size = heap->end_address - heap->starting_address;
    if (new_size > old_size)
        return;

    // dont contract too far
    if (new_size < HEAP_MIN_SIZE)
        new_size = HEAP_MIN_SIZE;

    // free the pages allocated to those addresses
    u32int additional_addr = heap->end_address;
    heap->end_address = heap->starting_address + new_size;

    while (additional_addr > heap->end_address)
    {
        free_frame(get_page(additional_addr, 0, kernel_directory));
        additional_addr -= 0x1000;
    }

    return new_size;
}

// handles heap expansion and inserting new header into the index table
static void heap_expansion_handler(u32int size, heap_t *heap, u8int page_align)
{
    u32int prev_end = heap->end_address;
    u32int prev_size = heap->end_address - heap->starting_address;
    u32int start_addr = heap->starting_address;

    u32int new_size = size + sizeof(header_t) + sizeof(footer_t);
    expand_heap(prev_size + new_size, heap);

    // check if the last header is free
    // if free then merge that header with this created block
    u32int merge = -1;
    for (int i = 0; i < heap->ordered_table->size; i++)
    {
        header_t *head = find_in_ordered_set(heap->ordered_table, i);
        u32int header_int = (u32int)head;

        if (header_int + sizeof(header_t) + sizeof(footer_t) + head->size == prev_end)
        {
            if (head->is_hole == 1)
                merge = i;
        }
    }

    if (merge != -1)
    {
        // get the header and update the size of the block
        header_t *head = find_in_ordered_set(heap->ordered_table, merge);
        u32int old_size = head->size;
        head->size += new_size;

        // make a new footer at the end of the memory and update the values
        u32int end_footer_addr = prev_end + new_size - sizeof(footer_t);
        footer_t *new_footer = (footer_t *)end_footer_addr;
        new_footer->header = head;
        new_footer->magic = HEAP_MAGIC;

        // get the previous footer and free the footer
        // should it be done??
        // no need to free as this has not been alloced
        // u32int old_footer = (u32int)(head) + sizeof(header_t) + old_size;
        // footer_t *old_footer_ptr = (footer_t *)(old_footer);
        // free((void *)(old_footer), heap);
    }
    else
    {
        // else create a new header and add to the list
        // add this new header pointer to our list
        u32int new_block_start = prev_end;
        u32int new_block_end = prev_end + sizeof(header_t) + size;

        header_t *new_header = (header_t *)new_block_start;
        new_header->is_hole = 1;
        new_header->magic = HEAP_MAGIC;
        new_header->size = size;

        footer_t *new_footer = (footer_t *)new_block_end;
        new_footer->header = new_header;
        new_footer->magic = HEAP_MAGIC;

        insert_into_ordered_set(heap->ordered_table, (type_t)(new_header));
    }
}

void *alloc(u32int size, heap_t *heap, u8int page_align)
{
    int index = find_smallest_hole_index(size, heap, page_align);

    // if no such hole is present
    if (index == -1)
    {
        // save some previous data
        heap_expansion_handler(size, heap, page_align);

        return alloc(size, heap, page_align);
    }

    header_t *head = find_in_ordered_set(heap->ordered_table, index);
    u32int original_size = head->size;

    u32int head_addr = (u32int)head;

    u32int block_addr = head_addr + sizeof(header_t);
    u32int offset = 0;

    // page align block_addr
    // and create a new hole before the page aligned hole
    u8int delete_header = 1;

    if (page_align & (block_addr % 0x1000 != 0))
    {

        offset = 0x1000 - block_addr % 0x1000;
        block_addr += offset;

        delete_header = 0;

        // create a new hole
        u32int new_temp_footer_int = block_addr - sizeof(header_t) - sizeof(footer_t);
        u32int new_temp_header_int = block_addr - sizeof(header_t);

        // update original header size
        head->is_hole = 1;
        head->magic = HEAP_MAGIC;
        head->size = offset - sizeof(header_t) - sizeof(footer_t);

        // update new footer
        footer_t *new_temp_footer = (footer_t *)(new_temp_footer_int);
        new_temp_footer->header = head;
        new_temp_footer->magic = HEAP_MAGIC;

        // update head and orignal size
        head = (header_t *)(new_temp_header_int);
        original_size -= offset;
        head_addr = (u32int)(head);
    }

    u8int isSplit = 1;
    // check if it is possible to split this block
    if (original_size <= size + sizeof(header_t) + sizeof(footer_t))
    {
        isSplit = 0;

        // update the header of the block
        head->is_hole = 0;
        head->magic = HEAP_MAGIC;
        head->size = original_size;

        if (delete_header)
            remove_from_ordered_set(heap->ordered_table, index);

        // now return the pointer
        return (void *)(block_addr);
    }

    // now split the block
    // find the addr of new footer and new header
    u32int new_footer_addr = block_addr + size;
    u32int new_header_addr = block_addr + size + sizeof(footer_t);
    u32int old_footer_addr = block_addr + original_size;

    footer_t *new_footer = (footer_t *)(new_footer_addr);
    header_t *new_header = (header_t *)(new_header_addr);
    footer_t *old_footer = (footer_t *)(old_footer_addr);
    header_t *old_header = head;

    // fill out the values in the structures
    old_header->is_hole = 0;
    old_header->magic = HEAP_MAGIC;
    old_header->size = size;

    new_footer->header = old_header;
    new_footer->magic = HEAP_MAGIC;

    new_header->is_hole = 1;
    new_header->magic = HEAP_MAGIC;
    new_header->size = original_size - (size + sizeof(header_t) + sizeof(footer_t));

    old_footer->magic = HEAP_MAGIC;
    old_footer->header = new_header;

    // remove the old index from the list
    if (delete_header)
        remove_from_ordered_set(heap->ordered_table, index);

    // add the new created block in the list
    insert_into_ordered_set(heap->ordered_table, (type_t)(new_header));

    // return the malloced block address
    return (void *)(block_addr);
}

static u8int merge_block(header_t *prev, header_t *cur, heap_t *heap)
{
    // check if previous is valid and free
    footer_t *prev_footer = (footer_t *)((u32int)(prev) + sizeof(header_t) + prev->size);
    footer_t *cur_footer = (footer_t *)((u32int)(cur) + sizeof(header_t) + cur->size);

    // check the validity of header and footer
    if (prev->magic != HEAP_MAGIC || prev_footer->magic != HEAP_MAGIC)
        return 0;
    if (prev->is_hole == 0)
        return 0;

    // can be merged
    // if valid, update the size of the header and footer
    prev->size += sizeof(header_t) + sizeof(footer_t) + cur->size;
    cur_footer->header = prev;

    // remove the current block from the list
    int len = heap->ordered_table->size;
    for (int i = 0; i < len; i++)
    {
        header_t *head = find_in_ordered_set(heap->ordered_table, i);
        if (head == cur)
        {
            remove_from_ordered_set(heap->ordered_table, i);
            break;
        }
    }

    // merge successful
    return 1;
}

void free(void *pointer, heap_t *heap)
{
    if (pointer == 0)
        return;

    u32int cur_header_int = (u32int)((u32int)pointer - sizeof(header_t));
    header_t *cur_header = (header_t *)(cur_header_int);

    u32int cur_footer_int = (u32int)((u32int)pointer + cur_header->size);
    footer_t *cur_footer = (footer_t *)(cur_footer_int);

    if (cur_header->magic != HEAP_MAGIC || cur_footer->magic != HEAP_MAGIC)
    {
        monitor_write("Heap is Corrupted as magic values have been over written!!");
        return;
    }

    cur_header->is_hole = 1;

    // form the next and prev headers and footers
    footer_t *prev_footer = (footer_t *)(cur_header_int - sizeof(footer_t));
    header_t *prev_header = prev_footer->header;

    header_t *next_header = (header_t *)(cur_footer_int + sizeof(footer_t));
    footer_t *next_footer = (footer_t *)((u32int)next_header + next_header->size + sizeof(header_t));

    // previous block must stay in the index table
    if (merge_block(prev_header, cur_header, heap))
    {
        cur_header = prev_header;
        cur_footer->header = cur_header;
    }
    else
    {
        insert_into_ordered_set(heap->ordered_table, (type_t)(cur_header));
    }

    if (merge_block(cur_header, next_header, heap))
    {
        cur_footer = next_footer;
        cur_footer->header = cur_header;
    }

    // now perform heap contraction
    if ((u32int)(cur_footer) + sizeof(footer_t) == heap->end_address)
    {
        u32int old_length = heap->end_address - heap->starting_address;

        u32int new_length = contract_heap((u32int)(cur_header)-heap->starting_address, heap);

        // if entire block is not removed
        if (cur_header->size - (old_length - new_length) > 0)
        {
            // make new footer and update it
            u32int new_size = cur_header->size - (old_length - new_length);

            cur_header->size = new_size;
            cur_header->magic = HEAP_MAGIC;
            cur_header->is_hole = 1;

            footer_t *new_footer = (footer_t *)((u32int)(cur_header) + sizeof(header_t) + cur_header->size);
            new_footer->header = cur_header;
            new_footer->magic = HEAP_MAGIC;
        }
        else
        {
            // completely removed
            // remove this free block from the index table
            int len = heap->ordered_table->size;
            for (int i = 0; i < len; i++)
            {
                header_t *head = find_in_ordered_set(heap->ordered_table, i);
                if (head == cur_header)
                {
                    remove_from_ordered_set(heap->ordered_table, i);
                    break;
                }
            }
        }
    }
}

// returns the physical address
void *kmalloc(u32int sz){
    if(kheap == 0){
        int temp = placement_address;
        placement_address += sz;

        return (void *)temp;
    }

    return alloc(sz, kheap, 0);
}

void kfree(void *ptr){
    if(kheap != 0){
        free(ptr, kheap);
    }
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