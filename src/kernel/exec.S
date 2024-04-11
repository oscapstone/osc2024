.global exec_user_prog
exec_user_prog:
      msr ELR_EL1, x0
      msr SP_EL0, x1
      mov x0, 0x3c0
      msr SPSR_EL1, x0
      eret
