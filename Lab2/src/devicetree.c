#include "devicetree.h"
#include "mini_uart.h"
#include "string.h"
// #include

uint32_t swap32(uint32_t n){
  return ((n >> 24) & 0xff) | ((n >> 8) & 0xff00) |
         ((n << 8) & 0xff0000) | ((n << 24) & 0xff000000);
}


void fdt_header_b2l(fdt_header* dtb){
  if(dtb->magic == 0xd00dfeed)
    return;

  dtb->magic = swap32(dtb->magic);
  dtb->totalsize = swap32(dtb->totalsize);
  dtb->off_dt_struct = swap32(dtb->off_dt_struct);
  dtb->off_dt_strings = swap32(dtb->off_dt_strings);
  dtb->off_mem_rsvmap = swap32(dtb->off_mem_rsvmap);
  dtb->version = swap32(dtb->version);
  dtb->last_comp_version = swap32(dtb->last_comp_version);
  dtb->boot_cpuid_phys = swap32(dtb->boot_cpuid_phys);
  dtb->size_dt_strings = swap32(dtb->size_dt_strings);
  dtb->size_dt_struct = swap32(dtb->size_dt_struct);
}

void parse_dtb(void *dtb){
  uart_sends("start parsing\n");
  fdt_header* header = (fdt_header*)dtb;

  fdt_header_b2l(header);

  char *struct_block = (char *)((uintptr_t)header+header->off_dt_struct);
  char *string_block = (char *)((uintptr_t)header+header->off_dt_strings);
  char *end_ptr = (char*)((uintptr_t)header+header->off_dt_struct+header->size_dt_struct);
  uint32_t* token = (uint32_t*)struct_block;
  int padding = 0;

  fdt_prop *prop;
  fdt_prop tmprop;

  while(swap32(*token) != FDT_END){
    switch(swap32(*token)){
      case FDT_BEGIN_NODE:
        uart_sends("Node name: ");
        struct_block += 4; // 4 bytes
        uart_sends((const char*)struct_block);
        uart_sendc('\n');
        padding = 0;
        int name_len = strlen((const char*)struct_block);
        name_len++;

        padding = (4-(name_len%4))%4;
        struct_block += (name_len+padding);
        break;
      case FDT_END_NODE:
        uart_sends("node end\n");
        struct_block += 4;
        break;
      case FDT_PROP:
        // uart_sends("");
        struct_block += 4;
        prop = (fdt_prop*)struct_block;
        tmprop.len = swap32(prop->len);
        tmprop.nameoff = swap32(prop->nameoff);
        uart_sends("Property name: ");
        uart_sends(string_block+tmprop.nameoff);
        uart_sendc('\n');

        struct_block += 8;

        uart_sends("Property value: ");
        if(tmprop.len == 0){
          uart_sends("null\n");
          break;
        }
        for(int i = 0; i < tmprop.len; i++)
          uart_sendc(*struct_block++);
        uart_sendc('\n');
        padding = (4-(tmprop.len%4))%4;
        struct_block += padding; // (padding - 1)?

        break;
      case FDT_NOP:
        struct_block += 4;
        break;
    }
    token = (uint32_t*)struct_block;
  }
  if(struct_block == end_ptr)
    uart_sends("Yes\n");
  else uart_sends("No\n");
}

void fdt_traverse(void *dtb, void (*callback)(void *, char *)){
  fdt_header* header = (fdt_header*)dtb;

  fdt_header_b2l(header);

  char *struct_block = (char *)((uintptr_t)header+header->off_dt_struct);
  char *string_block = (char *)((uintptr_t)header+header->off_dt_strings);

  uint32_t* token = (uint32_t*)struct_block;
  int padding = 0;

  fdt_prop *prop;
  fdt_prop tmprop;

  while(swap32(*token) != FDT_END){
    switch(swap32(*token)){
      case FDT_BEGIN_NODE:
        struct_block += 4; // 4 bytes
        // padding = 0;
        int name_len = strlen((const char*)struct_block);
        name_len++;
        
        padding = (4-(name_len%4))%4;
        struct_block += (name_len+padding);
        break;
      case FDT_END_NODE:
        struct_block += 4;
        break;
      case FDT_PROP:
        struct_block += 4;

        prop = (fdt_prop*)struct_block;
        tmprop.len = swap32(prop->len);
        tmprop.nameoff = swap32(prop->nameoff);

        struct_block += 8;

        callback((void *)struct_block, (string_block+tmprop.nameoff));

        if(tmprop.len == 0){
          break;
        }       
        padding = (4-(tmprop.len%4))%4;
        struct_block += (padding+tmprop.len); // (padding - 1)?

        break;
      case FDT_NOP:
        struct_block += 4;
        break;
    }
    token = (uint32_t*)struct_block;
  }
}