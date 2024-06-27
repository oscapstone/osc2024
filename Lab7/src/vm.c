#include "vm.h"
#include "mmu.h"
#include "mini_uart.h"
#include "memory.h"
#include "string.h"
#include "thread.h"

unsigned long virt_to_phys(void* vaddr){
  return (unsigned long)vaddr - VIRT_OFFSET;
}

unsigned long phys_to_virt(void* paddr){
  return (unsigned long)paddr + VIRT_OFFSET;
}

// static void increment_vma_ref_count(vm_area_t *vma){
//   vma->ref_count++;
// }

// static void decrement_vma_ref_count(vm_area_t *vma){
//   vma->ref_count--;
//   if(vma->ref_count == 0){
//     kfree(vma);
//   }
// }

void add_vma(vm_area_t **head, unsigned long va, unsigned long pa, unsigned long sz, unsigned long flags){
  vm_area_t *new_area = (vm_area_t*)kmalloc(sizeof(vm_area_t));
  new_area->va = va & ~(PAGE_SIZE - 1);
  new_area->pa = pa & ~(PAGE_SIZE - 1);
  new_area->sz = sz % PAGE_SIZE == 0 ? sz : (sz + PAGE_SIZE) & ~(PAGE_SIZE - 1);
  new_area->flags = flags;

  if(!(*head)){
    new_area->prev = new_area;
    new_area->next = new_area;
    *head = new_area;
  }else{
    vm_area_t *cur = *head;
    do{
      if(cur->va > va){
        break;
      }
      cur = cur->next;
    }while(cur != *head);
    new_area->prev = cur->prev;
    new_area->next = cur;
    cur->prev->next = new_area;
    cur->prev = new_area;

    if(cur == *head && cur->va > va)
      *head = new_area;
  }
}

void remove_vma(vm_area_t **head, unsigned long va){
  vm_area_t *tmp = *head;
  do{
    if(tmp->va == va){
      if(tmp->next == tmp){
        *head = 0;
      }else{
        tmp->prev->next = tmp->next;
        tmp->next->prev = tmp->prev;
        if(tmp == *head)
          *head = tmp->next;
      }

      if(tmp->va != 0x80000 && tmp->va != 0x3c000000 && tmp->va != 0x100000)
        decrement_ref_count((void*)phys_to_virt((void*)tmp->pa));

      // decrement_vma_ref_count(tmp);
      kfree(tmp);
      return;
    }
    tmp = tmp->next;
  }while(tmp != *head);
  uart_sends("No such vma: 0x");
  uart_sendl(va);
  uart_sends("\n");
}

void clear_vma_list(vm_area_t **head){
  if(!(*head))
    return;
  do{
    vm_area_t *tmp = *head;
    if(tmp->next == tmp){
      *head = 0;
    }else{
      tmp->prev->next = tmp->next;
      tmp->next->prev = tmp->prev;
      *head = tmp->next;
    }
    // skip peripheral because it's not allocated by kmalloc
    if(tmp->va != 0x80000 && tmp->va != 0x3c000000 && tmp->va != 0x100000)
      decrement_ref_count((void*)phys_to_virt((void*)tmp->pa));

    // decrement_vma_ref_count(tmp);
    kfree(tmp);
  }while(*head != 0);
}

void copy_vma_list(vm_area_t **dst, vm_area_t *src){
  vm_area_t *cur = src;
  do{
    // skip peripheral
    // if(cur->va == 0x3c000000 || cur->va == 0x100000) {
    //   cur = cur->next;
    //   continue;
    // }
    // void *pm = kmalloc(cur->sz);
    // memcpy(pm, (void*)phys_to_virt((void*)cur->pa), cur->sz);
    // add_vma(dst, cur->va, virt_to_phys(pm), cur->sz, cur->flags);

    /* COW version*/
    add_vma(dst, cur->va, cur->pa, cur->sz, cur->flags);
    if(cur->va != 0x80000 && cur->va != 0x3c000000 && cur->va != 0x100000)
      increment_ref_count((void*)phys_to_virt((void*)cur->pa));
    cur = cur->next;
  }while(cur != src);
  // *dst = src;
}

void dump_vma_list(vm_area_t *head){
  vm_area_t *cur = head;
  if(!cur){
    uart_sends("vma_list is null\n");
    return;
  }
  do{
    uart_sends("va: 0x");
    uart_sendl(cur->va);
    uart_sends(" -> pa: 0x");
    uart_sendl(cur->pa);
    uart_sends(" size: ");
    uart_sendl(cur->sz);
    uart_sends(" flags: ");
    uart_sendl(cur->flags);
    uart_sends("\n");
    cur = cur->next;
  }while(cur != head);
}

/*******************************************************
* Entry of PGD, PUD, PMD which point to a page table
*
* +-----+------------------------------+---------+--+
* |     | next level table's phys addr | ignored |11|
* +-----+------------------------------+---------+--+
*      47                             12         2  0
*
* Entry of PUD, PMD which point to a block
*
* +-----+------------------------------+---------+--+
* |     |  block's physical address    |attribute|01|
* +-----+------------------------------+---------+--+
*      47                              n         2  0
*
* Entry of PTE which points to a page
*
* +-----+------------------------------+---------+--+
* |     |  page's physical address     |attribute|11|
* +-----+------------------------------+---------+--+
*      47                             12         2  0
*
* Invalid entry
*
* +-----+------------------------------+---------+--+
* |     |  page's physical address     |attribute|*0|
* +-----+------------------------------+---------+--+
*      47                             12         2  0
*******************************************************/

pte_t* walk(pagetable_t pgt, unsigned long vaddr, int alloc){
  for(int level = 3; level > 0; level--){
    pte_t* pte = &pgt[((vaddr >> (12 + 9*level)) & 0x1ff)];
    if(((*pte) & 0x1) == 0){
      if(!alloc)
        return 0;
      void* new_pgt = alloc_pages(0);
      memset((void*)new_pgt, 0, PAGE_SIZE);
      *pte = virt_to_phys(new_pgt)|PD_TABLE|PD_ACCESS|(MAIR_IDX_NORMAL_NOCACHE << 2);
    }
    pgt = (pagetable_t)phys_to_virt((void*)(*pte & 0xfffffffff000));
  }
  return &pgt[(vaddr >> 12) & 0x1ff];
}


int map_pages(pagetable_t pgt, unsigned long va, unsigned long pa, unsigned long sz, unsigned long attr){
  if(sz == 0)
    return -1;

  pte_t* pte;
  pagetable_t pgd = (pagetable_t)phys_to_virt((void*)pgt);
  va = va & ~(PAGE_SIZE - 1);
  pa = pa & ~(PAGE_SIZE - 1);

  for(unsigned int i = 0; i < sz; i += PAGE_SIZE){
    pte = walk(pgd, va+i, 1);
    if(pte == 0){
      uart_sends("walk failed\n");
      return -1;
    }
    *pte = (pa+i)|PD_PAGE|PD_ACCESS|(MAIR_IDX_NORMAL_NOCACHE << 2)|attr; // should set user access?
  }
  return 0;
}

void copy_pagetable(pagetable_t dst, pagetable_t src, int level){
  pagetable_t dst_pgd = (pagetable_t)phys_to_virt((void*)dst);
  pagetable_t src_pgd = (pagetable_t)phys_to_virt((void*)src);
  for(int i = 0; i < 512; i++){
    if(src_pgd[i] & 0x1){
      if(level != 3){
        void* new_pgt = alloc_pages(0);
        memset((void*)new_pgt, 0, PAGE_SIZE);
        dst_pgd[i] = virt_to_phys(new_pgt)|PD_TABLE|PD_ACCESS|(MAIR_IDX_NORMAL_NOCACHE << 2);
        copy_pagetable((pagetable_t)((void*)(dst_pgd[i] & 0xfffffffff000)), (pagetable_t)((void*)(src_pgd[i] & 0xfffffffff000)), level+1);
      }
      else {
        // increment_ref_count((void*)phys_to_virt((void*)(src_pgd[i] & 0xfffffffff000)));
        src_pgd[i] |= PD_RDONLY; // set to read-only
        dst_pgd[i] = src_pgd[i];
      }
    }
  }
}

void clear_pagetable(pagetable_t pgt, int level){
  pagetable_t pagetable = (pagetable_t)phys_to_virt((void*)pgt);
  for(int i = 0; i < 512; i++){
    pte_t* pte = &pagetable[i];
    if(*pte & 0x1){
      if(level < 2)
        clear_pagetable((pagetable_t)((void*)(*pte & 0xfffffffff000)), level + 1);
      kfree((void*)phys_to_virt((void*)(*pte & 0xfffffffff000)));
      *pte = 0;
    }
  }
}

void dump_pagetable(pagetable_t pgt, int level) {
  for (int i = 0; i < 512; i++) { // Assuming 512 entries in the page table
    pte_t *pte = &pgt[i];
    if (*pte & 0x1) {
      uart_sends("=====================\nLevel ");
      uart_sendi(level);
      uart_sends("\n");
      uart_sends("-------------------------\n");
      uart_sends("Entry ");
      uart_sendi(i);
      uart_sends(": ");
      uart_sendl((unsigned long)*pte);
      uart_sendc('\n');
      if(level <= 2)
        dump_pagetable((pagetable_t)phys_to_virt((void *)(*pte & 0x0000fffffffff000)), level+1);
    }
  }
}

void page_fault_handler(){
  thread *t = get_current();
  unsigned long addr;
  asm volatile("mrs %0, FAR_EL1" : "=r"(addr));

  vm_area_t *vma = t->vma_list;
  do{
    if(!vma){
      uart_sends("vma_list is null\n");
      break;
    }
    if(addr >= vma->va && addr < (vma->va + vma->sz)){
      pte_t *pte = walk((pagetable_t)phys_to_virt((void*)t->regs.pgd), addr, 0);
      if(pte && (*pte & PD_RDONLY)){ // Copy-on-write fault or Permission fault
        if(vma->flags & 2){ // Copy-on-write fault
          frame_t *frame = get_frame(phys_to_virt((void*)vma->pa));
          if(frame->ref_count > 1){
            uart_sends("[Copy-on-write fault]: 0x");
            uart_sendl(addr);
            uart_sends(".\n");
            // frame->ref_count--;
            void *new_page = kmalloc(vma->sz);
            memcpy(new_page, (void*)phys_to_virt((void*)vma->pa), vma->sz);
            if(vma->va == 0xffffffffb000)
              t->stack = new_page;
            unsigned long tmp_va = vma->va, tmp_sz = vma->sz;
            int tmp_flags = vma->flags;
            unsigned long new_page_pa = virt_to_phys(new_page);
            remove_vma(&t->vma_list, vma->va);
            add_vma(&t->vma_list, tmp_va, new_page_pa, tmp_sz, tmp_flags);
            unsigned long va = addr & ~(PAGE_SIZE - 1);
            unsigned long pa = new_page_pa + (va - vma->va);
            // *pte = pa | ((*pte & 0xfff) & ~PD_RDONLY);
            *pte = pa | (*pte & 0xf7f);
            // void *new_page = kmalloc(PAGE_SIZE);
            // memcpy(new_page, (void*)phys_to_virt((void*)(*pte & 0xfffffffff000)), PAGE_SIZE);
            // *pte = virt_to_phys(new_page)|((*pte & 0xfff) & ~PD_RDONLY);
          }
          else{
            uart_sends("[Permission fault]: 0x");
            uart_sendl(addr);
            uart_sends(".\n");
            if((*pte & 0xfffffffff000) >= vma->pa && (*pte & 0xfffffffff000) < (vma->pa + vma->sz))
              *pte &= ~PD_RDONLY;
            else{
              unsigned long va = addr & ~(PAGE_SIZE - 1);
              unsigned long pa = vma->pa + (va - vma->va);
              *pte = pa | (*pte & 0xf7f);
            }
          }

          // update TLB
          asm volatile(
            "dsb ish;"
            "msr ttbr0_el1, %0;"
            "tlbi vmalle1is;"
            "dsb ish;"
            "isb;" :: "r" (t->regs.pgd)
          );

          return;
        }
        else{ // Permission fault (Segmentation fault)
          uart_sends("Write to read-only page.\n");
          break;
        }
      }
      else{ // Translation fault
        unsigned long va = addr & ~(PAGE_SIZE - 1);
        unsigned long pa = vma->pa + (va - vma->va);
        unsigned long attr = 0;
        // xwr
        if(!(vma->flags & 4)) attr |= PD_UNX;
        if(!(vma->flags & 2)) attr |= PD_RDONLY;
        if((vma->flags & 1)) attr |= USER_ACCESS;
        if(map_pages((pagetable_t)t->regs.pgd, va, pa, PAGE_SIZE, attr) == 0){
          uart_sends("[Translation fault]: 0x");
          uart_sendl(addr);
          uart_sends(".\n");
        }
        return;
      }
    }
    vma = vma->next;
  }while(vma != t->vma_list);
  uart_sends("error: 0x");
  uart_sendl(addr);
  uart_sends(".\n");
  uart_sends("[Segmentation fault]: Kill Process.\n");
  thread_exit();
}
