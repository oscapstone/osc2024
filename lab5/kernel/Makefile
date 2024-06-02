GCC		= aarch64-linux-gnu-gcc
G++		= aarch64-linux-gnu-g++
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

# define:
# NS_DEBUG: enable debug output 

# nostdlib: No standard library
CFLAGS	= -Wall -Wextra -Wpedantic -ffreestanding \
		  -nostdlib -nostartfiles \
		  -mgeneral-regs-only\
		  -fno-stack-protector\
		  -O0\
		  -I src\
		  #-D NS_DEBUG

BIN_DIR = bin

OBJ_DIR = bin-int

# object files
OBJ_FILES = $(addprefix $(OBJ_DIR)/, src/start.o src/main.o \
				src/io/uart.o src/io/dtb.o src/io/reboot.o src/io/exception.o src/io/exception_c.o\
				src/utils/utils.o src/utils/utilsASM.o src/utils/printf.o src/utils/fifo_buffer.o src/utils/link_list.o \
				src/shell/shell.o \
				src/fs/cpio.o src/fs/fs.o src/fs/rootfs.o src/fs/initramfs.o \
				src/peripherals/gpio.o src/peripherals/irq.o src/peripherals/timer.o src/peripherals/mailbox.o src/peripherals/irq_s.o\
				src/mm/mmutilASM.o src/mm/mm.o\
				src/arm/elutilsASM.o src/arm/mmu.o\
				src/proc/task.o src/proc/task_s.o src/proc/signal.o src/proc/worker.o \
				src/syscall/syscall.o src/syscall/sys_getpid.o src/syscall/sys_uartread.o src/syscall/sys_uartwrite.o src/syscall/sys_exec.o src/syscall/sys_fork.o src/syscall/sys_exit.o src/syscall/sys_mbox_call.o src/syscall/sys_kill.o\
				src/syscall/sys_open.o src/syscall/sys_close.o src/syscall/sys_signal.o src/syscall/sys_mmap.o \
				src/lib/fork.o src/lib/getpid.o\
				)

# the directories of the output files
OUT_DIRS = $(sort $(dir $(OBJ_FILES)))

QEMU_DEVICE = /dev/pts/5

all : kernel.img

gdb-run :
	gdb-multiarch -ex 'set architecture aarch64' -ex 'file bin/kernel.elf' -ex 'target remote localhost:1234'

upload : all
	python3 uploader_qemu.py $(QEMU_DEVICE)
	sudo screen -L -Logfile screenlog.txt $(QEMU_DEVICE) 115200

kernel.img : $(OBJ_FILES) linker.ld $(BIN_DIR)
	$(LD) --verbose $(OBJ_FILES) -T linker.ld -o $(BIN_DIR)/kernel.elf
	$(OBJCOPY) -O binary $(BIN_DIR)/kernel.elf $@

clean :
	rm -rf $(OBJ_FILES) kernel.img $(BIN_DIR)/kernel.elf

$(OBJ_DIR)/%.o : %.S | $(OUT_DIRS)
	$(GCC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o : %.c $(OUT_DIRS)
	$(GCC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o : %.cpp $(OUT_DIRS)
	$(G++) $(CFLAGS) -c $< -o $@

# directories to be make
$(OUT_DIRS):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@





