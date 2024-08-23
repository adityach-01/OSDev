#include "common.h"
#include "io.h"
#include "pic.h"

// driver functions for the 8259A driver
// uses the 8259A legacy PIC
void configure_PIC(u32int offset1, u32int offset2){
    // remap the irq table by configuring the PICs
    // Remap the irq table.
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, offset1);
    outb(0xA1, offset2);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // init the masks of the master and slave PIC
    // mask all the interrupts initially to allow uninterrupted booting of CPU
    outb(0x21, 0xff);
    outb(0xA1, 0xff);
}

void set_PIC_mask(u32int IRQline){
    u8int port, data;

    if(IRQline < 8){
        port = PIC1_DATA;
    }
    else{
        port = PIC2_DATA;
        IRQline -= 8;
    }

    data = inb(port) | (1<<IRQline);
    outb(port,data);
}

void remove_PIC_mask(u32int IRQline){
    u8int port, data;

    if(IRQline < 8){
        port = PIC1_DATA;
    }
    else{
        port = PIC2_DATA;
        IRQline -= 8;
    }

    data = inb(port) & ~(1<<IRQline);
    outb(port,data);

}

void send_interrupt_ack(u32int int_no){
    // 32 to 39 raised by master
    // 40 to 47 raised by slave
    int irq_no = int_no - PIC_REMAP_OFFSET;
    if(irq_no >= 8){
        // if raised by slave, ack both slave and master
        outb(PIC2_CMD, ACK_BYTE);
    }

    // else only ack master
    outb(PIC1_CMD, ACK_BYTE);
}