.section ".text"
_start:
    wfe // Wait for event
    b _start // Loop forever