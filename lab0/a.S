.section ".text"

// enter busy loop
// wfe: wait for event 沒事就往下執行
// branch to _start
_start:
  wfe
  b _start
