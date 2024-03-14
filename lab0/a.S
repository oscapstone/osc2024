.section ".text"
_start:
  mrs x0, mpidr_el1
  and x0, x0, #0xff
  cbz x0, master
  b proc_hang

proc_hang:
  wfe
  b proc_hang

master:
  b proc_hang
