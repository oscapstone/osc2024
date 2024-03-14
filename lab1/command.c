#include "mbox.h"
#include "string.h"
#include "uart.h"

void input_buffer_overflow_message(char cmd[]) {
  uart_puts("Follow command: \"");
  uart_puts(cmd);
  uart_puts("\"... is too long to process.\n");

  uart_puts("The maximum length of input is 64.");
}

void command_help() {
  uart_puts("\n");
  uart_puts("\033[32m\tValid Command:\n");
  uart_puts("\thelp:   print this help.\n");
  uart_puts("\thello:  print \"Hello World!\".\n");
  uart_puts("\treboot: reboort the device.\n");
  uart_puts("\tinfo:   info of revision and ARM memory.\n");
  uart_puts("\tclear:  clear the screen\n\033[0m");
  uart_puts("\n");
}

void command_hello() { uart_puts("Hello World!\n"); }

void command_not_found(char *s) {
  uart_puts("Err: command ");
  uart_puts(s);
  uart_puts(" not found, try <help>\n");
}

void command_reboot() {
  uart_puts("Start Rebooting...\n"); // 顯示重啟提示信息

  // PM_RSTC（Power Management, Reset Controller）
  // 設置PM_RSTC寄存器來觸發重啟。PM_RSTC是“電源管理重啟控制器”寄存器的地址。
  *PM_RSTC = PM_PASSWORD | 0x20; // 寫入密碼和重啟命令

  // PM_WDOG（Power Management, Watchdog Timer）
  // 設置PM_WDOG寄存器來定義看門狗計時器的超時時間。PM_WDOG是“電源管理看門狗計時器”寄存器的地址。
  *PM_WDOG =
      PM_PASSWORD | 100; // 寫入密碼和看門狗超時時間（通常以某種時鐘週期為單位）
}

void command_info() {
  get_board_revision();
  get_ARM_memory();
}

void command_clear() {
  // 發送特定的ANSI轉義序列到UART來清除終端螢幕並重置游標位置

  uart_puts("\033[2J"); // \033是ESC鍵的八進位代碼，用於引導ANSI轉義序列
                        // [2J是ANSI轉義序列，用於清除整個螢幕內容。
                        // 2指明清除模式，J是清除螢幕的指令。

  uart_puts(
      "\033[H"); // [H是將游標移動到螢幕左上角（即主頁位置）的轉義序列。
                 // 在清除螢幕後進行此操作是為了從一個乾淨的狀態開始輸出新的內容。

  uart_puts(
      "\033[3J"); // [3J是用於清除終端的滾動回顧緩衝區（如果可用的話）的轉義序列。
                  // 這意味著清除所有已經滾動出螢幕的文字，實際效果取決於終端的實現。
}
