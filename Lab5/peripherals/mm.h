#ifndef	_MM_H
#define	_MM_H

//  Heap end address: 0x1195B88
//  Allocate 1MB for stack: 0x1195B88 + 0x100000 = 0x1295B88
#define LOW_MEMORY 0x400000    // Place the bootloader sp at 0x400000
#define USER_STACK (1 << 23)    // sp for a user program.(Ranging from 0x800000 to 0x400000)



#endif  /*_MM_H */