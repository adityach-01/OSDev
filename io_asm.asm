global outb
global inb
global inw
global enable_interrupts
global disable_interrupts

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret


outb:
    mov al, [esp + 8]
    mov dx, [esp + 4]
    out dx, al
    ret
    
inb:
    mov dx, [esp + 4]
    in al, dx
    ret


;inw:
;    mov dx, [esp + 4]
;    inw ax, dx
;    ret
