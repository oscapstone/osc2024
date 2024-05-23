//------------------------------------------------------------------------------
// fn signal_handler_wrapper()
//------------------------------------------------------------------------------
signal_handler_wrapper:
    ldr x0, [sp]
    blr x0
    mov x8, 64
    svc 0

.global signal_handler_wrapper
.type signal_handler_wrapper function
