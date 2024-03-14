#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024 
//看門狗計時器（Watchdog Timer）：這是一個硬體計時器，旨在監控系統的運作。
//如果系統出現故障或停止運行，看門狗計時器會在預定的時間內沒有被重設時觸發，進而執行重置操作


void set(long addr, unsigned int value);
void reset(int tick);
void cancel_reset();