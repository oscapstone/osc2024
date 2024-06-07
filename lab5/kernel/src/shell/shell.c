
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
#include "peripherals/irq.h"

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

struct SHELL_CMD_ARG_INFO {
	int offset;		// if offset is -1 mean it is null
	int len;
};

#define MAX_SHELL_ARGS 16

struct SHELL_INFO {
	int args;
	struct SHELL_CMD_ARG_INFO arg_info[MAX_SHELL_ARGS];
};

#define MAX_SHELL_CMD	256

static struct SHELL_INFO shell_info;
static char cmd_space[MAX_SHELL_CMD];

static void shell_parse_cmd() {
	int offset = 0;
	shell_info.args = 0;
	int arg_len = 0;

	while (cmd_space[offset] != '\0') {

		switch (cmd_space[offset])
		{
		case ' ':
			offset++;
			break;
		default:
		{
			int start_offset = offset;
			arg_len = 0;
			while(cmd_space[offset] != '\0' && cmd_space[offset] != ' ') {
				arg_len++;
				offset++;
			}
			cmd_space[offset++] = '\0';	// make the args stringable
			shell_info.arg_info[shell_info.args].offset = start_offset;
			shell_info.arg_info[shell_info.args].len = arg_len;
			shell_info.args++;
		}
			break;
		}
	}
	for (int i = shell_info.args; i < MAX_SHELL_ARGS; i++) {
		shell_info.arg_info[i].offset = -1;
	}
}

static BOOL shell_is_cmd(const char* cmd) {
	return utils_strncmp(&cmd_space[shell_info.arg_info[0].offset], cmd, utils_strlen(cmd) + 1) == 0;
}

static BOOL shell_get_args(int index, char** arg_val) {
	if (index < 0 || index > MAX_SHELL_ARGS) {
		NS_DPRINT("index out of range\n");
		return FALSE;
	}
	if (shell_info.arg_info[index].offset == -1) {
		NS_DPRINT("offset = -1\n");
		return FALSE;
	}
	*arg_val = &cmd_space[shell_info.arg_info[index].offset];
	return TRUE;
}

void shell() {

	char cwd_path[256];
	cwd_path[0] = '/';
	cwd_path[1] = '\0';
	task_get_current_el1()->pwd = fs_get_root_node();

	// init for setTimout command
	timeout_msg_buf = kzalloc(CMD_TIMEOUT_MSG_BUF_SIZE);
	timeout_msg_ptr = 0;
	timeout_packages = NULL;

    U32 input_ptr = 0;

	// getting CPIO addr
	fdt_traverse(get_cpio_addr);

    while (TRUE) {

        printf("%s# ", cwd_path);
        
        input_ptr = 0;
		memzero(cmd_space, MAX_SHELL_CMD);
        while (TRUE) {
			while (uart_async_empty()) {
            	enable_interrupt();             // make sure it can be interrupt
				asm volatile("nop");
			}
            char c = uart_async_get_char();
            if (c == '\n' || c == '\r') {
                uart_send_nstring(2, "\r\n");
            	cmd_space[input_ptr] = '\0';
                break;
            } else if (c == 8 || c == 127) {	// backspace
				if (input_ptr > 0) { // is not null string
					uart_send_char('\b');
					uart_send_char(' ');
					uart_send_char('\b');
					input_ptr--;
					cmd_space[input_ptr] = '\0';
				}
			} else {
            	cmd_space[input_ptr++] = c;
                uart_send_char(c);
            }
        }
		shell_parse_cmd();

		if (shell_info.arg_info[0].offset == -1) {
			continue;
		}

        if (shell_is_cmd("help")) {
            printf("NS shell ver 0.12\n");
            printf("help   : print this help menu\n");
			printf("hello  : print Hello World!\n");
			printf("info   : show board info\n");
			printf("reboot : reboot the device\n");
			printf("ls     : List all CPIO files\n");
			printf("cat    : Show file content\n");
			printf("dtb    : Load DTB data\n");
			printf("async  : mini uart async test\n");
			printf("set    : set timeout message for lab3. format: set [msg] [second].\n");
        } else if (shell_is_cmd("hello")) {
			printf("Hello World!\n");
        } else if (shell_is_cmd("info")) {
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
		} else if (shell_is_cmd("ls")) {
			TASK* task = task_get_current_el1();
			FS_VNODE* cwd = task->pwd;

			if (shell_info.args == 1) {
				FS_VNODE* vnode = NULL;
				LLIST_FOR_EACH_ENTRY(vnode, &cwd->childs, self) {
					printf("%s\n", vnode->name);
				}
			} else {
				char* path;
				if (!shell_get_args(1, &path)) {
					printf("Failed to get path addr\n");
					continue;
				}
				FS_VNODE* target = NULL;
				if (vfs_lookup(cwd, path, &target)) {
					printf("Failed to get path: %s\n", path);
					continue;
				}
				if (S_ISDIR(target->mode)) {
					FS_VNODE* vnode = NULL;
					LLIST_FOR_EACH_ENTRY(vnode, &target->childs, self) {
						printf("%s\n", vnode->name);
					}
				} else if (S_ISREG(target->mode)) {
					printf("%s\n", target->name);
				}
			}

			//cpio_ls();
		} else if (shell_is_cmd("cd")) {
			// TODO
		} else if (shell_is_cmd("cat")) {
			if (shell_info.args == 1)
				continue;
			char *path;
			if (!shell_get_args(1, &path)) {
				printf("Failed to get path addr\n");
				continue;
			}

			FS_FILE* target = NULL;
			if (vfs_open(task_get_current_el1()->pwd, path, FS_FILE_FLAGS_RDWR, &target)) {
				printf("Failed to open file: %s\n", path);
				continue;
			}
			if (!S_ISREG(target->vnode->mode)) {
				printf("%s is not a file to read.\n", target->vnode->name);
				continue;
			}
			printf("file size: %d bytes\n", target->vnode->content_size);
			for(U32 i = 0; i < target->vnode->content_size; i++) {
				printf("%c", ((char*)target->vnode->content)[i]);
			}
			printf("\n");
			vfs_close(target);
		} else if (shell_is_cmd("dtb")) {
			U64 dtbPtr = (U64) _dtb_ptr;
			uart_send_string("DTB address: ");
			uart_hex64(dtbPtr);
			uart_send_string("\n");
			//fdt_traverse(print_dtb);
			fdt_traverse(get_cpio_addr);
		} else if (shell_is_cmd("reboot")) {
			uart_send_string("Rebooting....\n");
			reset(1000);
		} else if (shell_is_cmd("exception")) {
			printf("\nException level: %d\n", utils_get_el());
		} else if (shell_is_cmd("exe")) {

			char* filePath;
			if (!shell_get_args(1, &filePath)) {
				printf("can not parse arg 1\n");
				continue;
			}

			TASK* user_task = task_create_user("", TASK_FLAGS_NONE);
			if (task_run_program(NULL/* change this to shell cwd*/, user_task, filePath) == -1) {
				printf("Failed to execute program: %s\n", filePath);
				task_delete(user_task);
				continue;
			}
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

		} else if (shell_is_cmd("async")) {
			printf("This will transmit A character for async uart\n");
			uart_async_write_char('A');
			uart_set_transmit_int();
		} else if (shell_is_cmd("mem")) {
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
		} else if (shell_is_cmd("ps")) {
			printf("Current running task count: %d\n", task_manager->running);
			for (U32 i = 0; i < task_manager->running; i++) {
				printf("%4d: %s\n", task_manager->running_queue[i]->pid, task_manager->running_queue[i]->name);
			}
		} else if (shell_is_cmd("kill")) {
			char* number_ptr;
			if (!shell_get_args(1, &number_ptr)) {
				printf("Cannot get argv[1]\n");
				continue;
			}
			pid_t pid = utils_str2uint_dec(number_ptr);
			task_kill(pid, -2);
		} else if (shell_is_cmd("set")) {
			if (shell_info.args != 3) {
				printf("Usage: set [msg] [timeout:second]\n");
				continue;
			}
			
			char* msg;
			if (!shell_get_args(1, &msg)) {
				continue;
			}
			char* timout_str;
			if (!shell_get_args(2, &timout_str)) {
				continue;
			}


			U32 timeout = utils_str2uint_dec(timout_str);
			cmd_setTimeout(timeout, msg, utils_strlen(msg));
		} else {
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
