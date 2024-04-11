#include "board/peripheral.hpp"

#include "util.hpp"

void set_peripheral_irq(bool enable, int bit) {
  addr_t addr;
  if (bit < 32) {
    addr = PERIPHERAL_ENABLE_IRQs_1;
  } else {
    addr = PERIPHERAL_ENABLE_IRQs_2;
    bit -= 32;
  }
  SET_CLEAR_BIT(enable, addr, bit);
}

void set_aux_irq(bool enable) {
  set_peripheral_irq(enable, 29);
}
