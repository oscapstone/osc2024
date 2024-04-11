// load VBAR_EL1 with virtual vector table address
//.global irq_vector_init
//irq_vector_init:
//	adr	x0, vectors
//	msr	vbar_el1, x0
//	ret
