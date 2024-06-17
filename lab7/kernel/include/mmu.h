#ifndef MMU_H 
#define MMU_H 

void set_up_identity_paging();
void kernel_finer_gran();

void map_page(long*, long, long, long);
void switch_page();

long pa2va (long);
long va2pa (long);

long trans(long);
long trans_el0(long);

void setup_peripheral_identity(long*);

void three_level_translation_init();

#endif
