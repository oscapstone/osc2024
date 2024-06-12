#include "mmu.h"
#include "helper.h"
#include "utils.h"
#include "alloc.h"
#include "thread.h"
#include "mmu_regs.h"

#define PERIPHERAL_START 0x3c000000L
#define PERIPHERAL_END   0x40000000L

#define DEVICE_BASE 0x3F000000L


const int page_size = 4096;

void three_level_translation_init(){
    unsigned long *pmd_1 = (unsigned long *) 0x3000;
    for(unsigned long i=0; i<512; i++){
        unsigned long base = 0x200000L * i; // 2 * 16^5 -> 2MB
        if(base >= DEVICE_BASE){
            //map as device
            pmd_1[i] = PD_ACCESS + PD_BLOCK + base + (MAIR_IDX_DEVICE_nGnRnE << 2) + PD_KERNEL_USER_ACCESS;
        }
        else{
            //map as normal
            pmd_1[i] = PD_ACCESS + PD_BLOCK + base + (MAIR_IDX_NORMAL_NOCACHE << 2);
        }
    }

    unsigned long *pmd_2 = (unsigned long *) 0x4000;
    for(unsigned long i=0; i<512; i++){
        unsigned long base = 0x40000000L + 0x200000L * i;
        pmd_2[i] = PD_ACCESS + PD_BLOCK + base + (MAIR_IDX_NORMAL_NOCACHE << 2);
    }

    unsigned long * pud = (unsigned long *) 0x2000;
    *pud =  PD_TABLE + (unsigned long) pmd_1;
    pud[1] =  PD_TABLE + (unsigned long) pmd_2;
}

/*
void set_up_identity_paging() {
	write_sysreg(tcr_el1, TCR_CONFIG_DEFAULT);

    long attr =
        (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) |
        (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8));
    write_sysreg(mair_el1, attr);

	long* l0 = (long*)0x1000L;
	long* l1 = (long*)0x2000L;

    memset(l0, 0, 0x1000);
    memset(l1, 0, 0x1000); // clears the page table:
    
	long *p0 = (long*)0x3000;
    for (int i = 0; i < 504; i++) {
        p0[i] = (i << 21) | PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK, (1 << 6);
    }
    // [0x3F000000 ~ 0x80000000] device memory 
    for (int i = 504; i < 1024; i++) {
        p0[i] = (i << 21) | PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK;
    }

    l0[0] = (long)l1 | BOOT_PGD_ATTR;
    l1[0] = 0x3000L | PD_TABLE;
    l1[1] = 0x4000L | PD_TABLE;

    write_sysreg(ttbr0_el1, l0);
    write_sysreg(ttbr1_el1, l0);

    unsigned long sctlr = read_sysreg(sctlr_el1);
    write_sysreg(sctlr_el1, sctlr | 1);
}
*/

/*
void kernel_finer_gran() {
    // [0x00000000, 0x3F000000] normal memory 
	asm volatile ( "dsb ish;" );
	
	long* l1 = pa2va(0x2000L);
	
	asm volatile ( "dsb ish;" );

    l1[0] = 0x3000L | PD_TABLE;
    l1[1] = 0x4000L | PD_TABLE;
	asm volatile ( "dsb ish;" );
}
*/

void map_page(long * pgd, long va, long pa, long flag){
    for(int i=0; i<4; i++){
        unsigned int offset = (va >> (39 - i * 9)) & 0x1ff;
        
        if(i == 3){
            pgd[offset] = pa;
            pgd[offset] |=  PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | flag;
            return;
        }
        
        if(pgd[offset] == 0){
            unsigned long * new_page_table = my_malloc(4096);
            for(int j=0; j<4096; j++){
                ((char *) (new_page_table))[j] = 0;
            }
            pgd[offset] = va2pa(new_page_table);
            pgd[offset] |= PD_TABLE;
        }

        pgd = (long*) (((long) (pgd[offset] & ENTRY_ADDR_MASK)) + 0xffff000000000000);
    }
}

/*
void map_page(long* pt, long va, long pa, long flag) {
	if (pa < 0x3c000000) {
		uart_printf ("mapping from %llx to %llx, pt is %llx\r\n", pa, va, pt);
	}
	for (int level = 0; level < 4; level ++) {
		// uart_printf ("%d\r\n", level);
		long id = (va >> (39 - 9 * level)) & 0b111111111;
		if (pt[id] != 0) {
			if (level == 3) {
				uart_printf ("exited mapping\r\n");
			}
			pt = pa2va(pt[id] & (0xfffffffff000L));
		}
		else {
			if (level == 3) {
				pt[id] = pa | PD_ACCESS | PD_PAGE | flag |  (MAIR_IDX_NORMAL_NOCACHE << 2);
				break;
			}
			else {
				long* t = my_malloc(page_size);
				for (int i = 0; i < 4096; i ++) {
					((char*)t)[i] = 0;
				}
				pt[id] = va2pa(t) | PD_TABLE;
				pt = pa2va(pt[id] & (0xfffffffff000L));
			}
		}
	}
}
*/


void setup_peripheral_identity(long* table)
{
    unsigned long pages = (PERIPHERAL_END - PERIPHERAL_START + 4096 - 1) / 4096;
    for (int i = 0; i < pages; i++)
    {
		// uart_printf ("%d\r\n", i);
        map_page(table, PERIPHERAL_START + i * 4096, PERIPHERAL_START + i * 4096, (1 << 6));
    }
}

long pa2va(long x) {
	return x + 0xffff000000000000;
}
long va2pa(long x) {
	return x - 0xffff000000000000;
}

long trans(long x) {
	long res;
	asm volatile (
		"mov x0, %[input];"
		"AT S1E1R, x0;"
		"MRS %[res], PAR_EL1;"
		// "isb;"
		: [res] "=r" (res)
		: [input] "r" (x)
		: "x0"
	);
	if (res & (0x1)) {
		uart_printf ("trans failed, res: %x\r\n", res & 0xfff);
	}
	return (res & 0xfffffffff000) | (x & 0xfff);
}

long trans_el0(long x) {
	long res;
	asm volatile (
		"mov x0, %[input];"
		"AT S1E0R, x0;"
		"MRS %[res], PAR_EL1;"
		// "isb;"
		: [res] "=r" (res)
		: [input] "r" (x)
		: "x0"
	);
	if (res & (0x1)) {
		uart_printf ("trans failed, res: %x\r\n", res & 0xfff);
	}
	return (res & 0xfffffffff000) | (x & 0xfff);
}

extern thread* get_current();

void switch_page() {
	asm volatile(
		"mov x0, %0;"
        "dsb ish;"
        "msr ttbr0_el1, x0;"
		"dsb ish;"
        "tlbi vmalle1is;"
        "dsb ish;"
        "isb;"
        :
		: "r" (get_current() -> PGD)
		: "x0"
	);
}

