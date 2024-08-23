#include "common.h"
#include "paging.h"
#include "frame_allocator.h"
#include "monitor.h"
#include "kheap.h"

extern u32int placement_address;
page_directory_t *current_directory;
page_directory_t *kernel_directory;
extern heap_t *kheap;

extern u32int nframes;

// takes an entry in the page table and assigns it a physical frame
// What needs to be done?
// 1. Who will access the page? User / kernel
// 2. Is the page for code or data, ie read only or writable?
// 3. Physical address of the page frame assigned? Can be obtained from the index of the free frame in bitmap

void alloc_frame(page_t *page, int is_kernel, int is_writeable)
{
   if (page->frame != 0)
   {
       return; // Frame was already allocated, return straight away.
   }
   else
   {
       u32int idx = first_frame(); // idx is now the index of the first free frame.
       if (idx == (u32int)-1)
       {
           // PANIC is just a macro that prints a message to the screen then hits an infinite loop.
           PANIC("No free frames!");
       }

       set_frame(idx * 0x1000); // this frame is now ours!
       page->present = 1; // Mark it as present.
       page->rw = (is_writeable)?1:0; // Should the page be writeable?
       page->user = (is_kernel)?0:1; // Should the page be user-mode?

       // stores the index of the frame in the page table entry, offset will be obtained from VA to get the complete PA
       page->frame = idx;
   }
}

// Function to deallocate a frame.
void free_frame(page_t *page)
{
   u32int frame;
   if (!(frame=page->frame))
   {
       return; // The given page didn't actually have an allocated frame!
   }
   else
   {
       clear_frame(frame * 0x1000); // Frame is now free again.
       page->frame = 0x0; // Page now doesn't have a frame.
   }
}


void initialise_paging()
{
   // The size of physical memory. For the moment we
   // assume it is 16MB big.
   u32int mem_end_page = 0xFFFFFFFF;

   nframes = mem_end_page / 0x1000;

   init_frame_allocator();

   // Let's make a page directory.
   kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
   memset((u8int *)kernel_directory, 0, sizeof(page_directory_t));
   current_directory = kernel_directory;

   // We need to identity map (phys addr = virt addr) from
   // 0x0 to the end of used memory, so we can access this
   // transparently, as if paging wasn't enabled.
   // NOTE that we use a while loop here deliberately.
   // inside the loop body we actually change placement_address
   // by calling kmalloc(). A while loop causes this to be
   // computed on-the-fly rather than once at the start.
    for (int i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000){
        get_page(i, 1, kernel_directory);
    }



    u32int i = 0;
    while (i < placement_address)
    {
        // Kernel code is readable but not writeable from userspace.
        alloc_frame(get_page(i, 1, kernel_directory), 1, 0);
        i += 0x1000;
    }

    for (int i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000){
       alloc_frame(get_page(i, 1, kernel_directory), 1, 1);    
    }
    // Before we enable paging, we must register our page fault handler.
    register_interrupt_handler(14, page_fault);

    // Now, enable paging!
    switch_page_directory(kernel_directory);
    kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0);


    return;
}

void switch_page_directory(page_directory_t *dir)
{
   current_directory = dir;
   asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
   u32int cr0;
   asm volatile("mov %%cr0, %0": "=r"(cr0));
   cr0 |= 0x80000000; // Enable paging!
   asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(u32int PhysicalAddress, int make, page_directory_t *dir)
{
   // Turn the PhysicalAddress into an index of the frame
   // address is the physical address.
   u32int frame_index = PhysicalAddress / 0x1000;

   // Find the page table containing this address.
   u32int index_in_page_directory = frame_index / 1024;

   // check the virtual address of that table
   if (dir->tables[index_in_page_directory]) // If this table is already assigned
   {    
        // return the pointer to the page structure
        // this is done, as each page table contains 1024 pages/frames, then which frame to choose and return its virtual address??
       return &dir->tables[index_in_page_directory]->pages[frame_index%1024];
   }
   // create an entry corresponding to this frame
   else if(make)
   {
       u32int tmp = kmalloc_a(sizeof(page_table_t));
       dir->tables[index_in_page_directory] = (page_table_t *) tmp;
       memset((u8int *)dir->tables[index_in_page_directory], 0, 0x1000);

        // assign the page directory entry corresponding to the table
       dir->tablesPhysical[index_in_page_directory] = tmp | 0x7; // PRESENT, RW, US.
       return &dir->tables[index_in_page_directory]->pages[frame_index%1024];
   }
   else
   {
       return 0;
   }
}

void page_fault(registers_t *regs)
{
   // A page fault has occurred.
   // The faulting address is stored in the CR2 register.
   u32int faulting_address = 6969;
   asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

   // The error code gives us details of what happened.
   int present   = !(regs->err_code & 0x1); // Page not present
   int rw = regs->err_code & 0x2;           // Write operation?
   int us = regs->err_code & 0x4;           // Processor was in user-mode?
   int reserved = regs->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
   int id = regs->err_code & 0x10;          // Caused by an instruction fetch?

   // Output an error message.
   monitor_write("Page fault! ( ");
   if (present) {monitor_write("present ");}
   if (rw) {monitor_write("read-only ");}
   if (us) {monitor_write("user-mode ");}
   if (reserved) {monitor_write("reserved ");}
   monitor_write(") at");
   monitor_write_hex(faulting_address);
   monitor_put('\n');
   PANIC("Page fault");
}