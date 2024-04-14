#include "board/mini-uart.hpp"
#include "cmd.hpp"
#include "int/timer.hpp"

struct Ctx {
  char* msg;
  uint64_t tick;
  Ctx(char* m, uint64_t t) : msg(new char[strlen(m) + 1]), tick(t) {
    memcpy(msg, m, strlen(m) + 1);
  }
  ~Ctx() {
    delete[] msg;
  }
};

void print_message(void* context) {
  auto ctx = (Ctx*)context;
  mini_uart_printf("[" PRTval "] " PRTval ": '%s'\n", FTval(get_current_time()),
                   FTval(tick2timeval(ctx->tick)), ctx->msg);
  delete ctx;
}

int cmd_setTimeout(int argc, char* argv[]) {
  if (argc <= 2) {
    mini_uart_puts("setTimeout: require at least two argument\n");
    mini_uart_puts("usage: message second [prio]\n");
    return -1;
  }
  auto msg = argv[1];
  auto tval = parseTval(argv[2]);
  auto ctx = new Ctx{msg, get_current_tick()};
  auto prio = argc >= 4 ? strtol(argv[3]) : 0;
  add_timer(tval, (void*)ctx, print_message, prio);
  return 0;
}
