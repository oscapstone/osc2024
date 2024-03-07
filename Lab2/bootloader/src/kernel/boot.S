// A53 document:https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch32-register-descriptions/Multiprocessor-Affinity-Register?lang=en
// where this code belongs(.text section) in the compiled binary.
.section ".text.boot"

// Make _start global.
.global _start

_start:
// because of a change in firmware, this code will only run on the
// BSP core, therefore there's no need to check the cpu's id. A spin-loop on a
// non-relocated address would be otherwise bad,
1:  
  ldr x1, =0x80000
  ldr x2, =_start
  ldr w3, =__bootloader_size

// Since GPU always start at 0x80000, in order to run our bootloader, we first need to move out code from 0x80000 to address where linker declared([_start]=0x60000) 
// The address pointed by _start is 0x60000, this can verified by objdump
relocate:
  ldr x4, [x1], #8
  str x4, [x2], #8
  sub w3, w3, #1
  cbnz w3, relocate

  // Set stack pointer before code starts
  // loading code will be placed before _start, so we can use _start to grow downwards
  ldr x1, =_start
  mov sp, x1

  // clear bss
  // load bss start location to x1(64bit register)
  ldr x1, =__bss_start
  // load bss size into w2(32bit register)
  // I used w1 and got no output when I using mailbox.
  // This is because that the w1 will destroy x1(upper 32 bits->0, lower load the content)
  ldr w2, =__bss_size
// Loop until all bss set to 0
2:
  cbz w2, 3f
  // xzr is 64 bits zero register, write all 0 to x1 then x1 = x1 + 8
  // https://blog.csdn.net/boildoctor/article/details/123359325
  str xzr, [x1], #8
  // sub w2 by 1 byte(8bits)
  sub w2, w2, #1
  cbnz w2, 2b

3:
  // jump to C code, will not return in normal scenario
  bl bootloader_main - 0x20000
  // this line is for safety to send cpu0 to busy waiting if not work properly
  b  3b