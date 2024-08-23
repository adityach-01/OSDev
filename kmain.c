#include "common.h"
#include "io.h"
#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "pic.h"
#include "isr.h"
#include "paging.h"

void keyboard_interrupt_handler(registers_t *regs){
    monitor_write("Key pressed!!");

    volatile u8int scancode = inb(0x60);

    // convert the scancode into ascii
    monitor_write("Scancode: ");
    monitor_write_hex(scancode);
    monitor_put('\n');

    // convert this scancode into ascii character and print the character
    // outb(0x20, 0x20);

}

int kmain(struct multiboot *mboot_ptr){

    init_descriptor_tables();
    monitor_clear();

    enable_interrupts();

    // configure the PIC and remap the IRQ lines
    configure_PIC(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    // remove_PIC_mask(1);
    // remove_PIC_mask(0);
    register_interrupt_handler(IRQ1, &keyboard_interrupt_handler);

    monitor_write("Hello World!!");
    monitor_put('\n');

    // asm volatile ("int $0x3");
    // asm volatile ("int $0x4");
    // init_timer(50);

    // code to generate page fault
    // u32int *ptr = (u32int*)0xA0000000;
    // u32int do_page_fault = *ptr;

    /*
    // working kernel heap
    u32int a = kmalloc(8);
    initialise_paging();    

    u32int b = kmalloc(8);
    u32int c = kmalloc(8);
    int *ptr = (int *)c;
    *ptr = 10;
    monitor_write("a: ");
    monitor_write_hex(a);
    monitor_write(", b: ");
    monitor_write_hex(b);
    monitor_write("\nc: ");
    monitor_write_hex(c);
    
    monitor_write("Pointer value : ");
    monitor_write_dec(*ptr);
    monitor_put('\n');
    kfree(c);
    kfree(b);
    u32int d = kmalloc(12);
    monitor_write(", d: ");
    monitor_write_hex(d);
    */

    for(;;) asm("hlt");

    return 0;
}