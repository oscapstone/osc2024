
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

extern char* _dtb_ptr;

// the base of memory
extern MEMORY_MANAGER mem_manager;

// for memory testing
U32 mem_buddy_alloc(U8 order);
void mem_buddy_free(U32 frame_index);

char* cpio_addr;

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
		cpio_addr = (char*)realCPIOAddr;
	}
}

void shell() {

    
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
        }
		else if (utils_strncmp(cmd_space,"info", 4) == 0) {
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
			UPTR filePtr;	// file start pointer in cpio
			unsigned long contentSize;
			if (cpio_get(fileName, len, &filePtr, &contentSize)) {
				printf("Program %s not found.\n", fileName);
				continue;
			}

			printf("Executing %s\n", fileName);

			// TODO: executing application
			// copy to 0x90000
			memcpy((void*)filePtr, (void*)0x90000, contentSize);
			
			// Success! but will remove one day because it is ugly to run a program
			elutil_from_el1_to_el0(0x90000);
		}
		 else if (utils_strncmp(cmd_space, "async", 5) == 0) {
			printf("This will transmit A character for async uart\n");
			uart_async_write_char('A');
			uart_set_transmit_int();
		 } else if (utils_strncmp(cmd_space, "mem ", 4) == 0) {
			printf("Memory base address: 0x%x\n", mem_manager.base_ptr);
			printf("Memory size:         0x%x\n", mem_manager.size);
			printf("Levels             : %d\n", mem_manager.levels);
			for (U32 level = 0; level < mem_manager.levels; level++) {
				FREE_INFO* info = &mem_manager.free_list[level];
				printf("Level %d\n", level);
				for(int i = 0;i < info->size;i++) {
					if (info->info[i] == -1)
						break;
					printf("    frame idx     : %d\n", info->info[i]);
				}
			}
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
		 }
		 else {
			uart_send_string("Unknown command\n");
			uart_send_string(cmd_space);
			uart_send_string("\n");
		}

    }

}
