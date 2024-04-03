#ifndef __MEM_H__
#define __MEM_H__

extern uint32_t _end;

#define HEAP_END &_end + 0x1000

#define USER_PROCESS_SP 0x200000
#define USER_START_ADDR 0x100000

#endif