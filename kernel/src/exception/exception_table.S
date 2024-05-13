

.macro EXCEPTION_WITH_TYPE type handler
    save_all
    mov x0, \type
    mov x1, sp
    add x1, x1, 32 * 8
    bl \handler
    b exception_restore_context
.endm

exception_restore_context:
    load_all
    eret


.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    .align 7
    EXCEPTION_WITH_TYPE 0, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 1, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 2, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 3, unknown_exception_handler
    
    .align 7
    EXCEPTION_WITH_TYPE 4, svc_handler

    .align 7
    EXCEPTION_WITH_TYPE 5, irq_handler

    .align 7
    EXCEPTION_WITH_TYPE 6, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 7, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 8, svc_handler

    .align 7
    EXCEPTION_WITH_TYPE 9, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 10, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 11, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 12, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 13, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 14, unknown_exception_handler

    .align 7
    EXCEPTION_WITH_TYPE 15, unknown_exception_handler
