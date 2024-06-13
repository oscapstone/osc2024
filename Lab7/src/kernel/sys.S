.global call_get_pid
call_get_pid:
    mov w8, #0	
    svc #0
    ret

.global call_uart_read
call_uart_read:
    mov w8, #1	
    svc #0
    ret

.global call_uart_write
call_uart_write:
	mov w8, #2	
	svc #0
	ret

.global call_exec
call_exec:
	mov w8, #3	
	svc #0
	ret

.global call_fork
call_fork:
	mov w8, #4	
	svc #0
	ret

.global call_exit
call_exit:
	mov w8, #5	
	svc #0
	ret

.global call_mbox
call_mbox:
	mov w8, #6
	svc #0
	ret

.global call_kill
call_kill:
	mov w8, #7
	svc #0
	ret

.global call_sigreg
call_sigreg:
	mov w8, #8
	svc #0
	ret

.global call_sigkill
call_sigkill:
	mov w8, #9
	svc #0
	ret

// load general registers from stack
.macro load_all
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldp x30, x0, [sp, 16 * 15]
    msr spsr_el1, x0
    // load information when interrupted task return(used when nested interrupt occurs)
    ldp x0, x1, [sp, 16 * 16]
    msr elr_el1, x0
    msr sp_el0, x1
    // restore x0 and x1(as we just use it to restore the information of spsr and elr)
    ldp x0, x1, [sp ,16 * 0]
    add sp, sp, 16 * 17
.endm

.global call_sys_clone
call_sys_clone:
	
	mov	x8, #4					/* fork */
	svc	0x0
	ret
	/*bl uart_itoa*/
	cmp	x0, #0					/* check it's child or parent */
	beq	thread_start			/* child */
	ret

thread_start:
	/*bl uart_itoa*/
	/*mov	x29, 0*/

	/* Pick the function arg and execute.  */
	/*mov	x0, x11
	blr	x10*/
	ret

	/*load_all
	eret*/

	/* We are done, pass the return value through x0.  */
	mov	x8, #5
	svc	0x0