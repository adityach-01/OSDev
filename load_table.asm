global gdt_load
global idt_load

gdt_load:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush  
.flush:
    ret


idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

