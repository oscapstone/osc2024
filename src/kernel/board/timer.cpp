#include "board/timer.hpp"

#include "mm/mmu.hpp"

void set_core0_timer_irq_ctrl(bool enable, int bit) {
  SET_CLEAR_BIT(enable, pa2va(CORE0_TIMER_IRQ_CTRL), bit);
}
