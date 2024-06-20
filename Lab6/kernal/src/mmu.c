#include"mmu.h"
#include"peripherals/rpi_mmu.h"
#include"memalloc.h"
//pud base: (1 << 30)
//pmd base: (1 << 21)
//pte base: (1 << 12)

int set_2M_kernel_mmu(){
    unsigned long* pgd=KERNEL_PGD_BASE;
    unsigned long* pud=KERNEL_PUD_BASE;
    unsigned long* pmd1=KERNEL_PMD_1_BASE;
    unsigned long* pmd2=KERNEL_PMD_2_BASE;
    for(int i=0;i<512;i++){
        pgd[i]=0;
        pud[i]=0;
        pmd1[i]=0;
        pmd2[i]=0;
    }
    pgd[0]=((unsigned long)pud | PD_TABLE);
    pud[0]=((unsigned long)pmd1 | PD_TABLE);
    pud[1]=((unsigned long)pmd2 | PD_TABLE);
    for(int i=0;i<512;i++){
        if((i*(1 << 21)) < 0x3b400000)pmd1[i]=((unsigned long)(i*(1 << 21)) | BOOT_PMD_ATTR_NORMAL_NOCACHE);
        else pmd1[i]=((unsigned long)(i*(1 << 21)) | BOOT_PMD_ATTR_nGnRnE);
    }
    for(int i=0;i<512;i++){
        pmd2[i]=((unsigned long)(i*(1 << 21) + (1 << 30)) | BOOT_PMD_ATTR_nGnRnE);
    }
     
    return 0;

}


// run in el1
unsigned long* get_new_page(){ 
    unsigned long* new_page=fr_malloc(0x1000);
    for(int i=0;i<512;i++)new_page[i]=0;
    return new_page;
}


// run in el1,map a frame for user program
// pgd should be virtual address
int map_signle_page(unsigned long pgd, unsigned long va, unsigned long pa, unsigned long flag){ 
    if( ((pgd & 0xffff000000000000) != 0xffff000000000000) || (pgd & 0xfff != 0)){
        puts("pgd address error\r\n");
        puts("pgd address:");put_long_hex(pgd);
        puts("\r\n");
        return 1;
    }
    if((va & (~TABLE_ADDRESS_MASK)) != 0){
        puts("va wrong\r\n");
        put_long_hex(va);
        puts("\r\n");
        return 1;
    }
    if((pa & (~TABLE_ADDRESS_MASK)) != 0){
        puts("pa wrong\r\n");
        put_long_hex(pa);
        puts("\r\n");
        return 1;
    }
    if((va & 0xfff) != 0){
        puts("va isn't aligned\r\n");
        return 1;
    }
    if((pa & 0xfff) != 0){
        puts("pa isn't aligned\r\n");
        return 1;
    }
    // puts("map va:");
    // put_long_hex(va);
    // puts(" to pa:");
    // put_long_hex(pa);
    // puts("\r\n");
    unsigned long* current_table=pgd;
    for(int level=0;level<4;level++){
        unsigned int index=((va) >> (39 - level*9) & 0x1ff);
        if(index >= 512){puts("error:index\r\n");return 1;}
        if((current_table[index] & 0x1) == 0){
            if(level == 3){
            current_table[index]= pa | flag ;//| PD_KNX;
            return 0;
            }
            else{
                unsigned long* new_page=get_new_page();
                // puts("new_page:");
                // put_long_hex(new_page);
                // puts("\r\n");
                if((VIRT_TO_PHYS((unsigned long)new_page)) & (flag) != 0){puts("error:flag.\r\n");return 1;}
                current_table[index]=(VIRT_TO_PHYS((unsigned long)new_page)) | (flag);
            }
        }
        else{
            if(level == 3){
                puts("error:va is already mapped.\r\n");
                return 1;
            }
        }
        current_table= (unsigned long*) PHYS_TO_VIRT((current_table[index] & TABLE_ADDRESS_MASK));

    }
    
}

int map_pages(unsigned long pgd, unsigned long va, unsigned long pa, unsigned int size,unsigned long flag){
    for(unsigned long offset=0;offset < size ;offset+=0x1000){
        if(map_signle_page(pgd,va+offset,pa+offset,flag))return 1;
    }
    return 0;
}

void Identity_Paging_el0(){
    unsigned long* pgd_el0=get_new_page();
    puts("pgd_el0:");
    put_long_hex(pgd_el0);
    puts("\r\n");
    for(unsigned long i=0x0;i<0x80000000;i+=0x1000){
        int value=0;
        if(i<0x3b400000)value=map_signle_page(pgd_el0,i,i,USER_ATTR_NORMAL_NOCACHE);
        else value=map_signle_page(pgd_el0,i,i,USER_ATTR_nGnRnE);
        if(value)break;
    }
    puts("pgd_el0:");
    put_long_hex(pgd_el0);
    puts("\r\n");
    pgd_el0=VIRT_TO_PHYS(pgd_el0);
    asm volatile ("msr ttbr0_el1, %0" : "=r" ((pgd_el0)));

    return;
}

unsigned long va_to_pa(unsigned long pgd,unsigned long va){

    unsigned long* current_table=pgd;
    for(int level=0;level<4;level++){
        unsigned int index=((va) >> (39 - level*9) & 0x1ff);
        if((current_table[index] & 0x1) == 0 && level<3){

            puts("error:this va: ");
            put_long_hex(va);
            puts(" is not mapped.\r\n");
            return;
        }
        if(level == 3){
            unsigned long pa=(current_table[index] & TABLE_ADDRESS_MASK) | (va & 0xfff);
            puts("va:");
            put_long_hex(va);
            puts(" pa:");
            put_long_hex((current_table[index] & TABLE_ADDRESS_MASK) | (va & 0xfff));
            puts("\r\n");
            return pa;
        }
        current_table= (unsigned long*) PHYS_TO_VIRT((current_table[index] & TABLE_ADDRESS_MASK));
    }
}

int reset_pgd(unsigned long pgd,int level){
    unsigned long* table=pgd;
    for(int i=0;i<512;i++){
        if((table[i] & 0x1) != 0){
            if(level >= 3)continue;
            table=(unsigned long*) PHYS_TO_VIRT((table[i] & TABLE_ADDRESS_MASK));
            reset_pgd(table,level+1);
        }
    }
    fr_free(pgd);
    return 0;
}