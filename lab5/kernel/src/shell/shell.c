
#include "base.h"
#include "io/uart.h"
#include "io/dtb.h"
#include "io/reboot.h"
#include "peripherals/mailbox.h"
#include "fs/cpio.h"
#include "utils/utils.h"
#include "utils/printf.h"
#include "mm/mm.h"
#include "arm/elutils.h"
#include "lib/fork.h"
#include "fs/fs.h"
#include "peripherals/timer.h"

extern char* _dtb_ptr;

// the base of memory
extern MEMORY_MANAGER mem_manager;
extern TASK_MANAGER* task_manager;

char* cpio_addr;

// for setTimeout command
#define CMD_TIMEOUT_MSG_BUF_SIZE 256
#define CMD_TIMEOUT_PACK_SIZE	10
char* timeout_msg_buf;
U32 timeout_msg_ptr;

typedef struct _SET_TIMEOUT_PACKAGE {
	U32 offset;
	U32 len;
	int timeout;
	struct _SET_TIMEOUT_PACKAGE* next;
}SET_TIMEOUT_PACKAGE;

SET_TIMEOUT_PACKAGE* timeout_packages;

void cmd_setTimeout(U32 second, const char* msg, U32 len);

void get_cpio_addr(int token, const char* name, const void *data, unsigned int size) {
	if(token == FDT_PROP && (utils_strncmp(name, "linux,initrd-start", 18) == 0)) {
		//uart_send_string("CPIO Prop found!\n");
		U64 dataContent = (U64)data;
		//uart_send_string("Data content: ");
		//uart_hex64(dataContent);
		//uart_send_string("\n");
		U32 cpioAddrBigEndian = *((U32*)dataContent);
		U32 realCPIOAddr = utils_transferEndian(cpioAddrBigEndian);
		//uart_send_string("CPIO address: 0x");
		//uart_hex64(realCPIOAddr);
		//uart_send_string("\n");
		cpio_addr = (char*) MMU_PHYS_TO_VIRT(realCPIOAddr);
	}
}

void shell() {


	// init for setTimout command
	timeout_msg_buf = kzalloc(CMD_TIMEOUT_MSG_BUF_SIZE);
	timeout_msg_ptr = 0;
	timeout_packages = NULL;

    char cmd_space[256];

    char* input_ptr = cmd_space;

	// getting CPIO addr
	fdt_traverse(get_cpio_addr);

    while (TRUE) {

        printf("# ");
        
        while (TRUE) {
			while (uart_async_empty()) {
				asm volatile("nop");
			}
            char c = uart_async_get_char();
            *input_ptr++ = c;
            if (c == '\n' || c == '\r') {
                uart_send_nstring(2, "\r\n");
                *input_ptr = '\0';
                break;
            } else {
                uart_send_char(c);
            }
        }

        input_ptr = cmd_space;
        if (utils_strncmp(cmd_space, "help", 4) == 0) {
            printf("NS shell ver 0.02\n");
            printf("help   : print this help menu\n");
			printf("hello  : print Hello World!\n");
			printf("info   : show board info\n");
			printf("reboot : reboot the device\n");
			printf("ls     : List all CPIO files\n");
			printf("cat    : Show file content\n");
			printf("dtb    : Load DTB data\n");
			printf("async  : mini uart async test\n");
        } else if (utils_strncmp(cmd_space, "hello", 4) == 0) {
			printf("Hello World!\n");
        } else if (utils_strncmp(cmd_space,"info", 4) == 0) {
			get_board_revision();
			if (mailbox_call()) {
				printf("Revision:            0x%X\n", mailbox[5]);
			} else {
				printf("Unable to query revision!\n");
			}
			get_arm_mem();
        	if (mailbox_call()) {
				printf("Memory base address: 0x%x\n", mailbox[5]);
				printf("Memory size:         0x%x\n", mailbox[6]);
			} else {
				printf("Unable to query memory info!\n");
			}
			get_serial_number();
        	if (mailbox_call()) {
				printf("Serial Number:       0x%X%X\n", mailbox[6], mailbox[5]);
			} else {
				printf("Unable to query serial!\n");
			}
		}
		 else if (utils_strncmp(cmd_space, "ls", 2) == 0) {
			cpio_ls();
		} else if (utils_strncmp(cmd_space, "cat ", 4) == 0) {
			char fileName[32];
			U32 len = 0;
			char s;
			U32 iter = 4;
			while ((s = cmd_space[iter]) == ' ') {
				iter++;
			}
			while ((s = cmd_space[iter]) != '\0') {
				fileName[len++] = cmd_space[iter++];
			}
			fileName[len] = '\0';
			len--;
			cpio_cat(fileName, len);
		} else if (utils_strncmp(cmd_space, "dtb", 3) == 0) {
			U64 dtbPtr = (U64) _dtb_ptr;
			uart_send_string("DTB address: ");
			uart_hex64(dtbPtr);
			uart_send_string("\n");
			//fdt_traverse(print_dtb);
			fdt_traverse(get_cpio_addr);
		} else if (utils_strncmp(cmd_space, "reboot", 6) == 0) {
			uart_send_string("Rebooting....\n");
			reset(1000);
		} else if (utils_strncmp(cmd_space, "exception", 9) == 0) {
			printf("\nException level: %d\n", utils_get_el());
		} else if (utils_strncmp(cmd_space, "exe ", 4) == 0) {
			char fileName[32];
			U32 len = 0;
			U32 iter = 4;
			while (cmd_space[iter] == ' ') {
				iter++;
			}
			while (cmd_space[iter] != '\0') {
				fileName[len++] = cmd_space[iter++];
			}
			fileName[--len] = '\0';

			FS_FILE* file;
			int ret = vfs_open(task_get_current_el1()->pwd, fileName, FS_FILE_FLAGS_READ, &file);
			if (ret != 0) {
				printf("Program %s not found. result = %d\n", fileName, ret);
				continue;
			}
			if (S_ISDIR(file->vnode->mode)) {
				printf("%s is directory.\n", file->vnode->name);
				continue;
			}
			unsigned long contentSize = file->vnode->content_size;
			char programName[20];
			utils_char_fill(programName, file->vnode->name, utils_strlen(file->vnode->name));
			char* buf = kmalloc(file->vnode->content_size);
			vfs_read(file, buf, contentSize);
			vfs_close(file);
			
			printf("Executing %s\n", fileName);

			TASK* user_task = task_create_user(programName, TASK_FLAGS_NONE);
			user_task->pwd = file->vnode->parent;				// Just Hard coded now, can change to shell working directory
			task_copy_program(user_task, buf, contentSize);
			kfree(buf);
			task_run_to_el0(user_task);


			// if (cpio_get(fileName, len, &filePtr, &contentSize)) {
			// 	printf("Program %s not found.\n", fileName);
			// 	continue;
			// }

			// printf("Executing %s\n", fileName);

			// TASK* user_task = task_create_user(fileName, NULL);
			// task_copy_program(user_task, filePtr, contentSize);
			// task_run_to_el0(user_task);

			// TODO: wait task
			task_wait(user_task->pid);

		} else if (utils_strncmp(cmd_space, "async", 5) == 0) {
			printf("This will transmit A character for async uart\n");
			uart_async_write_char('A');
			uart_set_transmit_int();
		} else if (utils_strncmp(cmd_space, "mem ", 4) == 0) {
			printf("Memory base address: 0x%x\n", mem_manager.base_ptr);
			printf("Memory size        : 0x%x\n", mem_manager.size);
			printf("Total Levels       : %d\n", mem_manager.levels);

			U64 free_size = 0;
			for (U32 level = 0; level < mem_manager.levels; level++) {
				FREE_INFO* info = &mem_manager.free_list[level];
				//printf("Level %d\n", level);
				for(U32 i = 0;i < info->size;i++) {
					if (info->info[i] == MEM_FREE_INFO_UNUSED)
						break;
					//printf("    frame idx     : %d\n", info->info[i]);
					free_size += (1 << level) * MEM_FRAME_SIZE;
				}
			}
			printf("Free space         : %u bytes\n", free_size);
		} else if (utils_strncmp(cmd_space, "memtest", 7) == 0) {
			printf("Memory management test\n");
			char* ptr1 = kmalloc(16);
			char* ptr2 = kmalloc(15);
			char* ptr3 = kmalloc(32);
			char* ptr4 = kmalloc(40);
			printf("kmalloc(16). ptr: %x\n", ptr1);
			printf("kmalloc(15). ptr: %x\n", ptr2);
			printf("kmalloc(32). ptr: %x\n", ptr3);
			printf("kmalloc(40). ptr: %x\n", ptr4);
			kfree(ptr4);
			kfree(ptr3);
			kfree(ptr2);
			kfree(ptr1);
		} else if (utils_strncmp(cmd_space, "svctest", 7) == 0) {
			int pid = fork();
			if (pid == 0) {
				printf("child task\n");
			} else {
				printf("parent task. pid = %d\n", pid);
			}
			printf("SVC end\n");
		} else if (utils_strncmp(cmd_space, "ps ", 3) == 0) {
			printf("Current running task: %d\n", task_manager->running);
			for (U32 i = 0; i < task_manager->running; i++) {
				printf("%4d: %s\n", task_manager->running_queue[i]->pid, task_manager->running_queue[i]->name);
			}
		} else if (utils_strncmp(cmd_space, "kill ", 5) == 0) {
			char* number_ptr = &cmd_space[5];
			int char_size = 0;
			for(int i = 5; i < 256; i++) {
				char c = cmd_space[i];
				if (c == '\0' || c == ' ')
					break;
				char_size++;
			}
			char_size--;
			pid_t pid = utils_atou_dec(number_ptr, char_size);
			task_kill(pid, -2);
		} else if (utils_strncmp(cmd_space, "setTimeout ", 11) == 0) {
			U32 len = 0;
			while (cmd_space[11 + len++] != ' ');
			if (len == 0)
				continue;
			len -= 1;
			U32 timeout = utils_str2uint_dec(&cmd_space[11 + len + 1]);
			cmd_setTimeout(timeout, &cmd_space[11], len);
		}
		 else {
			uart_send_string("Unknown command\n");
			uart_send_string(cmd_space);
			uart_send_string("\n");
		}

    }

}

void shell_setTimeout_callback() {

	if (!timeout_packages) {
		return;
	}

	SET_TIMEOUT_PACKAGE* package = timeout_packages;

	int cur_timeout = package->timeout;

	while (package) {
		package->timeout -= cur_timeout;

		if (package->timeout <= 0) {
			uart_send_string("[SETTIMEOUT] msg: ");
			for (U32 i = 0; i < package->len; i++) {
				U32 offset_ptr = package->offset + i;
				uart_send_char(timeout_msg_buf[offset_ptr]);
				if (offset_ptr >= CMD_TIMEOUT_MSG_BUF_SIZE) {
					offset_ptr -= CMD_TIMEOUT_MSG_BUF_SIZE;
				}
			}
			uart_send_string("\n");
			SET_TIMEOUT_PACKAGE* toFree = package;
			package = package->next;
			timeout_packages = package;
			kfree(toFree);
			continue;
		}

		package = package->next;
	}
}

void cmd_setTimeout(U32 second, const char* msg, U32 len) {

	SET_TIMEOUT_PACKAGE* package = kzalloc(sizeof(SET_TIMEOUT_PACKAGE));
	package->offset = timeout_msg_ptr;
	package->len = len;
	package->timeout = second;
	package->next = NULL;

	for (U32 i = 0; i < len; i++) {
		timeout_msg_buf[timeout_msg_ptr++] = msg[i];
		if (timeout_msg_ptr >= CMD_TIMEOUT_MSG_BUF_SIZE) {
			timeout_msg_ptr -= CMD_TIMEOUT_MSG_BUF_SIZE;
		}
	}

	timer_add(shell_setTimeout_callback, second * 1000);
	if (!timeout_packages) {
		timeout_packages = package;	
	} else {
		SET_TIMEOUT_PACKAGE* current_package = timeout_packages;
		SET_TIMEOUT_PACKAGE* next_package = timeout_packages->next;

		if (current_package->timeout > package->timeout) {
			package->next = current_package;
			timeout_packages = package;
			return;
		}

		while (next_package) {
			if (current_package->timeout < (int) second && next_package->timeout >= (int) second) {
				current_package->next = package;
				package->next = next_package;
				return;
			}
			current_package = next_package;
			next_package = next_package->next;
		}
		current_package->next = package;
	}


	// overflow
}
