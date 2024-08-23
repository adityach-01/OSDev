#ifndef IO_HEADER
#define IO_HEADER

#include "common.h"

void outb(u16int port, u8int value);
u8int inb(u16int port);
u16int inw(u16int port);
void enable_interrupts();
void disable_interrupts();

#endif