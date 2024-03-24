.section ".text.boot"

.global _start
_start:
		mrs		x0, mpidr_el1	//mrs: move processorID to ARM register from mpidr_el1 register
		and		x0,x0, #0x3		//check processor ID
		cbz		x0, cleanbss		//if processor ID = 0(we want only one core) , branch to cleanbss

proc_hang:
		wfe //wfe, wait for event, low-power state, increase energy efficiency
		b		proc_hang

cleanbss:
		ldr		x0, =_start		//load 32bit to register  , = : address's value
		mov		sp, x0
		//clear bss
		ldr		x0, =bss_begin
		ldr		x1, =bss_size

memzero:
		cbz		x1, master		//if bss_size = 0 , branch to master
		str		xzr,[x0],#8		//write zero register(xzr)'s value to x0's addr , and x0=x0+8
		sub		x1, x1, #1
		cbnz	x1, memzero		//if bss_size != 0 , branch memzero(repeat clean)

master:
		bl		kernel_main
		b 		proc_hang
