#include <stdio.h>

int main() {
  const char *uart_path = "/dev/tty.usbserial-0001";

  FILE *uart_ptr = fopen(uart_path, "wr");
  FILE *kernel_img_ptr = fopen("", "r");

  fseek(kernel_img_ptr, 0L, SEEK_END);
  size_t image_size = ftell(kernel_img_ptr);
  fseek(kernel_img_ptr, 0L, SEEK_SET);

  fwrite(&image_size, sizeof(size_t), 1, uart_ptr);

  char msg[256] = {};
  fread(&msg, 256, 1, uart_ptr);
  puts(msg);

  return 0;
}
