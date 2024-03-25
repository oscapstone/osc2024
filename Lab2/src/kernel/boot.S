// A53 document:https://developer.arm.com/documentation/ddi0500/j/System-Control/AArch32-register-descriptions/Multiprocessor-Affinity-Register?lang=en
// where this code belongs(.text section) in the compiled binary.
.section ".text.boot"

// Make _start global.
.global _start

_start:
  // only the primary core runs (core 0), and the secondary cores are awaiting in a spin loop.
  // mpidr_el1: 64 bits Multiprocessor Affinity Register
  mrs x1, mpidr_el1 // Read MPIDR_EL1 into X1
  // Gat value [2:0] which is the CPU id(it's hard to understand in official spec, it's should try to sat that if 0x0,0x1 will invidually appear when it's switch to core 0 and 1 )
  // 0xFF should also OK?
  and x1, x1, #0xFF
  // If it's not core 0, send to busy waiting(Compare Branch Zero?)
  // xf(b) : branches to the first found label "x" searching "forward" for "f" or "backward" for "b".(GNU specific) see:https://stackoverflow.com/questions/27353096/1b-and-1f-in-gnu-assembly
  cbz x1, 1f

// if it's not cpu0
halt: 
  // Wait for event
  // When the C function returns, it enters the halt procedure where it loops forever doing nothing.
  wfe
  b halt

  // if it's cpu0
1:  
  // store the address of _dtb_addr
  // then load the dtb loading address(x0) into x1(_dtb_addr)
  ldr x1, =_dtb_addr
  str x0, [x1]
  // Set stack pointer before code starts
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
  bl main
  // this line is for safety to send cpu0 to busy waiting if not work properly
  b  halt
//_dtb_addr is in data section
.section .data
.global _dtb_addr
// make it 64bits
_dtb_addr: 
  .quad 0x0	