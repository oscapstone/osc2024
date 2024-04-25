# OSC2024 Lab4
Author: jerryyyyy708 (just to make sure my code is not copied by anyone)

## Note
* lab4-kernel8-basic+adv1: more clear to show page allocate/merge/free
* lab4-kernel8-no_reserve: for demo without reserve memory -> to show allocate split and free merge
* lab4-kernel8-all: with memory allocation and reserve

## Difference with startup allocation
* without startup allocation, the buddy system is placed in bss
* with startup allocation, the buddy system is placed in the memory allocated by simple_alloc

要在runtime才會知道總共有多少memory可以用，所以會需要動態分配如frame array等等的大小(不像現在寫的時候是一開始就把可以用的空間定義在程式裡，所以可以直接在global裡面給定array大小)

## Memory Reserve
* Startup allocator is placed in kernel, which is already reserved. The buddy system is placed right after the kernel, can be reserved together.