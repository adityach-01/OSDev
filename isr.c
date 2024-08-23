#include "common.h"
#include "isr.h"
#include "monitor.h"
#include "pic.h"

// interrupt handlers for all the interrupts
isr_t interrupt_handlers[256];


// registers the handler fro interrupt n
void register_interrupt_handler(u8int n, isr_t handler)
{
  interrupt_handlers[n] = handler;
}

// hardware interrupt handler called from irq stub
// irq stub and isr stub are different because irq are only 16, but isr may be many
// no acknowledge byte to be send in isr, but is a must in irqs

void irq_handler(registers_t *regs)
{

    send_interrupt_ack(regs->int_no);
    if (interrupt_handlers[regs->int_no] != 0)
    {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
    
}

// This gets called from our ASM interrupt handler stub.
// these are the interrupts generatede by the CPU
void isr_handler(registers_t *regs)
{
    monitor_write("recieved interrupt: ");
    monitor_write_dec(regs->int_no);
    monitor_write("CPU state : ");
    monitor_write_dec(regs->eip);
    monitor_put('\n');

    if(interrupt_handlers[regs->int_no] != 0){
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}