#pragma once

#include "board/mini-uart.hpp"
#include "util.hpp"

namespace std {
inline void abort() {
  mini_uart_printf_sync("abort");
  prog_hang();
}
};  // namespace std
