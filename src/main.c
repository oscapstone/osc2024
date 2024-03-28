#include "dtb.h"
#include "shell.h"
#include "uart1.h"
#include "utli.h"

#define CMD_LEN 128

extern void *_dtb_ptr;
extern void set_exception_vector_table();
enum shell_status { Read, Parse };

void main(void *arg) {
  _dtb_ptr = arg;
  shell_init();
  fdt_traverse(get_cpio_addr);
  set_exception_vector_table();
  print_cur_el();

  enum shell_status status = Read;
  while (1) {
    char cmd[CMD_LEN];
    switch (status) {
      case Read:
        shell_input(cmd);
        status = Parse;
        break;

      case Parse:
        shell_controller(cmd);
        status = Read;
        break;
    }
  }
}