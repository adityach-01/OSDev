#ifndef PIC_CONTORLLER_HEADER
#define PIC_CONTORLLER_HEADER

#include "common.h"
 
// pic1 is the master and pic2 is the slave
#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

#define PIC1_CMD 0x20
#define PIC2_CMD 0xA1

// #define PIC1_COMMAND
// #define PIC2_COMMAND
#define PIC_REMAP_OFFSET 0x20


#define ACK_BYTE 0x20

void configure_PIC();
void set_PIC_mask(u32int IRQline);
void remove_PIC_mask(u32int IRQline);
void send_interrupt_ack(u32int int_no);
#endif