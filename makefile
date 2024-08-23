OBJECTS = loader.o kmain.o common.o monitor.o io_asm.o descriptor_tables.o load_table.o interrupt.o isr.o timer.o pic.o paging.o frame_allocator.o kheap.o ordered_set.o
CC = gcc
CFLAGS = -m32 -nostartfiles -fno-builtin -fno-stack-protector -ffreestanding -Wall -Wextra -c
LDFLAGS = -T link.ld -melf_i386
LIBPATH = -L/usr/lib/gcc/x86_64-pc-linux-gnu/13.2.1/libgcc.a
LIBFLAGS = -lgcc -lc
AS = nasm
ASFLAGS = -f elf

all : kernel.elf

kernel.elf: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

os.iso: kernel.elf
	cp kernel.elf iso/boot/kernel.elf
	genisoimage -R \
				-b boot/grub/stage2_eltorito \
				-no-emul-boot \
				-boot-load-size 4\
				-A os  \
				-input-charset utf8  \
				-quiet   \
				-boot-info-table   \
				-o os.iso     \
				iso

run: os.iso
	bochs -f bochsrc.txt -q 


%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf *.o kernel.elf os.iso