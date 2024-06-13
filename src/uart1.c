#include "uart1.h"

#include "interrupt.h"
#include "mem.h"
#include "peripherals/aux.h"
#include "peripherals/gpio.h"
#include "string.h"
#include "task.h"
#include "utli.h"
#include "vm.h"

static volatile uint32_t w_f = 0, w_b = 0;
static volatile uint32_t r_f = 0, r_b = 0;
volatile char async_uart_read_buf[MAX_BUF_SIZE];
volatile char async_uart_write_buf[MAX_BUF_SIZE];

static filesystem *uartfs;
static vnode_operations *uartfs_v_ops = (vnode_operations *)0;
static file_operations *uartfs_f_ops = (file_operations *)0;

void uart_init() {
  /* Initialize UART */
  *AUX_ENABLES |= 0x1;  // Enable mini UART
  *AUX_MU_CNTL = 0;     // Disable TX, RX during configuration
  *AUX_MU_IER = 0;      // Disable interrupt
  *AUX_MU_LCR = 3;      // Set the data size to 8 bit
  *AUX_MU_MCR = 0;      // Don't need auto flow control
  *AUX_MU_BAUD = 270;   // Set baud rate to 115200
  *AUX_MU_IIR = 6;      // No FIFO

  /* Map UART to GPIO Pins */

  // 1. Change GPIO 14, 15 to alternate function
  register uint32_t r = *GPFSEL1;
  r &= ~((7 << 12) | (7 << 15));  // Reset GPIO 14, 15
  r |= (2 << 12) | (2 << 15);     // Set ALT5
  *GPFSEL1 = r;

  // 2. Disable GPIO pull up/down (Because these GPIO pins use alternate
  // functions, not basic input-output) Set control signal to disable
  *GPPUD = 0;

  // Wait 150 cycles
  wait_cycles(150);

  // Clock the control signal into the GPIO pads
  *GPPUDCLK0 = (1 << 14) | (1 << 15);

  // Wait 150 cycles
  wait_cycles(150);

  // Remove the clock
  *GPPUDCLK0 = 0;

  // 3. Enable TX, RX
  *AUX_MU_CNTL = 3;
}

char uart_read() {
  // Check data ready field
  do {
    asm volatile("nop");
  } while (
      !(*AUX_MU_LSR & 0x01));  // bit0 : data ready; this bit is set if receive
                               // FIFO hold at least 1 symbol (data occupy)
  // Read
  char r = (char)*AUX_MU_IO;
  // Convert carrige return to newline
  return r == '\r' ? '\n' : r;
}

void uart_write(char c) {
  // Check transmitter idle field
  do {
    asm volatile("nop");
  } while (
      !(*AUX_MU_LSR & 0x20));  // bit5 : This bit is set if the
                               // transmit FIFO can accept at least one byte
  // Write
  *AUX_MU_IO = c;  // AUX_MU_IO_REG : used to read/write from/to UART FIFOs
}

void uart_send_string(const char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    uart_write(str[i]);
  }
}

void uart_puts(const char *str) {
  uart_send_string(str);
  uart_write('\r');
  uart_write('\n');
}

void uart_flush() {
  while (*AUX_MU_LSR & 0x01) {
    *AUX_MU_IO;
  }
}

void uart_int(int64_t d) {
  if (d == 0) {
    uart_write('0');
  }
  if (d < 0) {
    uart_write('-');
    d *= -1;
  }
  uint8_t tmp[10];
  int32_t total = 0;
  while (d > 0) {
    tmp[total] = '0' + (d % 10);
    d /= 10;
    total++;
  }

  for (int32_t n = total - 1; n >= 0; n--) {
    uart_write(tmp[n]);
  }
}

void uart_hex(uint32_t d) {
  uart_send_string("0x");
  uint32_t n;
  for (int32_t c = 28; c >= 0; c -= 4) {
    // get highest tetrad
    n = (d >> c) & 0xF;
    // 0-9 => '0'-'9', 10-15 => 'A'-'F'
    n += n > 9 ? 0x37 : 0x30;
    uart_write(n);
  }
}

void uart_hex_64(uint64_t d) {
  uart_send_string("0x");
  uint64_t n;
  for (int32_t c = 60; c >= 0; c -= 4) {
    n = (d >> c) & 0xF;
    n += (n > 9) ? 0x37 : 0x30;  // 0x30 : 0 , 0x37+10 = 0x41 : A
    uart_write(n);
  }
}

static void enable_uart_tx_interrupt() { *AUX_MU_IER |= 2; }

static void disable_uart_tx_interrupt() { *AUX_MU_IER &= ~(2); }

static void enable_uart_rx_interrupt() { *AUX_MU_IER |= 1; }

static void disable_uart_rx_interrupt() { *AUX_MU_IER &= ~(1); }

void disable_uart_interrupt() {
  disable_uart_rx_interrupt();
  disable_uart_tx_interrupt();
}

void enable_uart_interrupt() {
  enable_uart_rx_interrupt();
  *ENABLE_IRQS_1 |= 1 << 29;  // Enable mini uart interrupt, connect the
                              // GPU IRQ to CORE0's IRQ (bit29: AUX INT)
}

void uart_write_async(char c) {
  while ((w_b + 1) % MAX_BUF_SIZE == w_f) {  // full buffer -> wait
    asm volatile("nop");
  }
  OS_enter_critical();
  async_uart_write_buf[w_b++] = c;
  w_b %= MAX_BUF_SIZE;
  OS_exit_critical();
  enable_uart_tx_interrupt();
}

char uart_read_async() {
  while (r_f == r_b) {
    asm volatile("nop");
  }
  OS_enter_critical();
  char r = async_uart_read_buf[r_f++];
  r_f %= MAX_BUF_SIZE;
  OS_exit_critical();
  enable_uart_rx_interrupt();
  return r == '\r' ? '\n' : r;
}

uint32_t uart_send_string_async(const char *str) {
  uint32_t i = 0;
  while ((w_b + 1) % MAX_BUF_SIZE != w_f &&
         str[i] != '\0') {  // full buffer -> wait
    OS_enter_critical();
    async_uart_write_buf[w_b++] = str[i++];
    w_b %= MAX_BUF_SIZE;
    OS_exit_critical();
  }
  enable_uart_tx_interrupt();
  return i;
}

uint32_t uart_read_string_async(char *str) {
  uint32_t i = 0;
  while (r_f != r_b) {
    OS_enter_critical();
    str[i++] = async_uart_read_buf[r_f++];
    r_f %= MAX_BUF_SIZE;
    OS_exit_critical();
  }
  str[i] = '\0';
  enable_uart_rx_interrupt();
  return i;
}

static void uart_tx_interrupt_handler() {
  if (w_b == w_f) {  // the buffer is empty
    return;
  }
  *AUX_MU_IO = (uint32_t)async_uart_write_buf[w_f++];
  w_f %= MAX_BUF_SIZE;
  enable_uart_tx_interrupt();
}

static void uart_rx_interrupt_handler() {
  if ((r_b + 1) % MAX_BUF_SIZE == r_f) {
    return;
  }

  async_uart_read_buf[r_b++] = (char)(*AUX_MU_IO);
  r_b %= MAX_BUF_SIZE;
  enable_uart_rx_interrupt();
}

void uart_interrupt_handler() {
  if (*AUX_MU_IIR &
      0x2)  // bit[2:1]=01: Transmit holding register empty (FIFO empty)
  {
    disable_uart_tx_interrupt();
    add_task(uart_tx_interrupt_handler, UART_INT_PRIORITY);
  } else if (*AUX_MU_IIR & 0x4)  // bit[2:1]=10: Receiver holds valid byte
                                 // (FIFO hold at least 1 symbol)
  {
    disable_uart_rx_interrupt();
    add_task(uart_rx_interrupt_handler, UART_INT_PRIORITY);
  }
}

static file *uartfs_create_fd(vnode *file_node) {
  file *fp = (file *)malloc(sizeof(file));
  if (!fp) {
    return (file *)0;
  }
  fp->vnode = file_node;
  fp->f_ops = file_node->f_ops;
  fp->f_pos = 0;
  return fp;
}

static vnode *uartfs_create_vnode(vnode *parent, const char *name,
                                  uint8_t type) {
  vnode *uartfs_vnode = (vnode *)malloc(sizeof(vnode));
  if (!uartfs_vnode) {
    return (vnode *)0;
  }
  strcpy(uartfs_vnode->name, name);
  uartfs_vnode->parent = parent;
  if (parent) {
    for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
      if (parent->subdirs[i] == (vnode *)0) {
        parent->subdirs[i] = uartfs_vnode;
        break;
      }
    }
  }
  for (uint32_t i = 0; i < MAX_SUBDIR_N; i++) {
    uartfs_vnode->subdirs[i] = (vnode *)0;
  }
  uartfs_vnode->mountpoint = (mount *)0;
  uartfs_vnode->v_ops = uartfs_v_ops;
  uartfs_vnode->f_ops = uartfs_f_ops;
  uartfs_vnode->type = type;
  uartfs_vnode->internal = (void *)0;

  return uartfs_vnode;
}

static int32_t uartfs_vnode_create(vnode *dir_node, vnode **target,
                                   const char *component_name) {
  uart_puts("create operation fails on uartfs");
  return -1;
}

static int32_t uartfs_vnode_lookup(vnode *dir_node, vnode **target,
                                   const char *component_name) {
  uart_puts("lookup operation fails on uartfs");
  return -1;
}

static int32_t uartfs_vnode_mkdir(vnode *dir_node, vnode **target,
                                  const char *component_name) {
  uart_puts("mkdir operation fails on uartfs");
  return -1;
}

static int32_t uartfs_file_open(vnode *file_node, file **fp) {
  *fp = uartfs_create_fd(file_node);
  if (!(*fp)) {
    return -1;
  }
  return 0;
}

static int32_t uartfs_file_close(file *fp) {
  free((void *)fp);
  return 0;
}

static int32_t uartfs_file_read(file *fp, void *buf, uint32_t len) {
  char *dst = (char *)buf;
  uint32_t i;
  for (i = 0; i < len; i++) {
    dst[i] = uart_read();
  }
  return i;
}

static int32_t uartfs_file_write(file *fp, const void *buf, uint32_t len) {
  const char *src = (const char *)buf;
  uint32_t i;
  for (i = 0; i < len; i++) {
    uart_write(src[i]);
  }
  return i;
}

static int32_t uartfs_setup_mount(filesystem *fs, mount *mount,
                                  const char *name) {
  mount->fs = fs;
  mount->root = uartfs_create_vnode((vnode *)0, name, DEVICE);
  if (!mount->root) {
    return -1;
  }
  mount->root->mountpoint = mount;
  return 0;
}

static int32_t uartfs_register_fs() {
  if (!uartfs_v_ops) {
    uartfs_v_ops = (vnode_operations *)malloc(sizeof(vnode_operations));
    if (!uartfs_v_ops) {
      return -1;
    }
    uartfs_v_ops->create =
        (vfs_create_fp_t)phy2vir((uint64_t)uartfs_vnode_create);
    uartfs_v_ops->lookup =
        (vfs_lookup_fp_t)phy2vir((uint64_t)uartfs_vnode_lookup);
    uartfs_v_ops->mkdir = (vfs_mkdir_fp_t)phy2vir((uint64_t)uartfs_vnode_mkdir);
  }

  if (!uartfs_f_ops) {
    uartfs_f_ops = (file_operations *)malloc(sizeof(file_operations));
    if (!uartfs_f_ops) {
      return -1;
    }
    uartfs_f_ops->open = (vfs_open_fp_t)phy2vir((uint64_t)uartfs_file_open);
    uartfs_f_ops->close = (vfs_close_fp_t)phy2vir((uint64_t)uartfs_file_close);
    uartfs_f_ops->read = (vfs_read_fp_t)phy2vir((uint64_t)uartfs_file_read);
    uartfs_f_ops->write = (vfs_write_fp_t)phy2vir((uint64_t)uartfs_file_write);
  }
  return 0;
}

filesystem *uartfs_init() {
  if (!uartfs) {  // if the uartfs has not been initialized
    uartfs = (filesystem *)malloc(sizeof(filesystem));
    strcpy(uartfs->name, "uartfs");
    uartfs->setup_mnt = (vfs_setup_mount)phy2vir((uint64_t)uartfs_setup_mount);
    uartfs->register_fs =
        (vfs_register_fs)phy2vir((uint64_t)uartfs_register_fs);
  }
  return uartfs;
}