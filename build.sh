clear
echo -e "Starting to build osdev"
echo -e "-compiling boot.asm"
nasm -f elf32 src/boot.asm -o build/boot.o
echo -e "-compiling kernelfiles"
gcc -c src/main.c -o build/main.o -ffreestanding -O2 -Wall -Wextra
echo -e "-linking kernelfiles"
gcc -T src/linker.ld -o bin/kernel32.bin build/boot.o build/main.o -ffreestanding -O2 -nostdlib -lgcc
if grub-file --is-x86-multiboot bin/kernel32.bin; then
	echo -e "-setup GRUB bootloader"
	mkdir -p isodir/boot/grub
	cp bin/kernel32.bin isodir/boot/kernel32.bin
	cp src/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o bin/kerneliso.iso isodir
	echo -e "SUCCES"
else
	echo -e "Invalid build"
fi
