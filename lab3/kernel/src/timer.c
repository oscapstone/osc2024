#include "timer.h"
#include "uart1.h"
#include "heap.h"
#include "u_string.h"

#define STR(x) #x
#define XSTR(s) STR(s)

struct list_head* timer_event_list;  // first head has nothing, store timer_event_t after it 

void timer_list_init(){
    INIT_LIST_HEAD(timer_event_list);
}

void core_timer_enable(){
    __asm__ __volatile__(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t" // cntp_ctl_el0[0]: enable timer, Control register for the EL1 physical timer.
                                   // cntp_tval_el0: Holds the timer value for the EL1 physical timer
        "mov x2, 2\n\t"            // x2 的32位元形式= w2
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t" // CORE0_TIMER_IRQ_CTRL的地址載入到x1
                                   // CORE0_TIMER_IRQ_CTRL 這個registe控制timer enable/disable
                                   // w2 存到 x1指向的地址=CORE0_TIMER_IRQ_CTRL的內容
        "str w2, [x1]\n\t"         // 取消core 0 timer interrupt的disable=enable timer
    );
}

void core_timer_disable()
{
    __asm__ __volatile__(
        "mov x2, 0\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t"         // QA7_rev3.4.pdf: Mask all timer interrupt
    );
}

void core_timer_handler(){
    // 檢查event list中有沒有待處理的事件
    // 若無 disable interrupt
    // 若有 執行timer event callback  
    if (list_empty(timer_event_list))
    {
        set_core_timer_interrupt(10000); // disable timer interrupt (set a very big value)
        return;
    }
    timer_event_callback((timer_event_t *)timer_event_list->next); // do callback and set new interrupt
}

void timer_event_callback(timer_event_t * timer_event){
    list_del_entry((struct list_head*)timer_event); // delete the event in queue
    free(timer_event->args);                        // free the event's space
    free(timer_event);
    ((void (*)(char*))timer_event-> callback)(timer_event->args);  // call the event

    // set queue linked list to next time event if it exists
    if(!list_empty(timer_event_list))
    {
        set_core_timer_interrupt_by_tick(((timer_event_t*)timer_event_list->next)->interrupt_time);
    }
    else
    {
        set_core_timer_interrupt(10000);  // disable timer interrupt (set a very big value)
    }
}

// except handler
void timer_set2sAlert(char* str)
{
    unsigned long long cntpct_el0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t": "=r"(cntpct_el0)); // timer 計數值
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); // timer 頻率
    uart_sendline("[Interrupt][el1_irq][%s] %d seconds after booting\n", str, cntpct_el0/cntfrq_el0); // 自系統啟動以來經過的秒數
    add_timer(timer_set2sAlert,2,"2sAlert"); // 設定下一個timer interrupt 2s後觸發
}


void add_timer(void *callback, unsigned long long timeout, char* args){
    // callback:函式????  timeout:等待的秒數 args:????
    timer_event_t* the_timer_event = kmalloc(sizeof(timer_event_t)); // free by timer_event_callback
    // 動態分配記憶體給timer event儲存所有關於timer event的相關訊息
    the_timer_event->args = kmalloc(strlen(args)+1);
    //分配記憶體給args，多出一格放終結符號'\0'
    strcpy(the_timer_event -> args,args);
    // args複製到剛剛分配記憶體空間的the_timer_event -> args
    the_timer_event->interrupt_time = get_tick_plus_s(timeout);
    // get_tick_plus_s計算現在時刻+經過timeout秒後的時間=要發出interrupt的時間
    // 把這個時間儲存在the_timer_event->interrupt_time
    the_timer_event->callback = callback;
    // 把callback存到the_timer_event->callback
    INIT_LIST_HEAD(&the_timer_event->listhead);
    // 初始化這個timer event的head
    // next prev指向自己形成空node

    // add the timer_event into timer_event_list (sorted)
    struct list_head* curr;
    // curr指向list head
    list_for_each(curr,timer_event_list)
    // 走訪全部的node
    {
        if(((timer_event_t*)curr)->interrupt_time > the_timer_event->interrupt_time)
        // 在list中若找到某event的interrupt更晚
        {
            list_add(&the_timer_event->listhead,curr->prev);  // add this timer at the place just before the bigger one (sorted)
            // 就把the_timer_event插入到某event之前
            break;
        }
    }
    
    if(list_is_head(curr,timer_event_list))
    // 若走完list沒有發現更晚的event或list為空
    {
        list_add_tail(&the_timer_event->listhead,timer_event_list);
        // the_timer_event放到list tail
    }
    // set interrupt to first event
    set_core_timer_interrupt_by_tick(((timer_event_t*)timer_event_list->next)->interrupt_time);
    // list中找到下一個即將觸發的event
    // set_core_timer_interrupt_by_tick設置核心計時器下一次中斷的觸發時間
}

// 現在時刻+指定秒數=計算出的cpu tick 
unsigned long long get_tick_plus_s(unsigned long long second){ // second:指定的秒數
    unsigned long long cntpct_el0=0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t": "=r"(cntpct_el0)); // 讀取當前的cpu tick
    unsigned long long cntfrq_el0=0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); // 讀取tick frequency
    return (cntpct_el0 + cntfrq_el0*second);
    // 計算將來要發出interrupt的cpu tick
}

// set timer interrupt time to [expired_time] seconds after now (relatively)
void set_core_timer_interrupt(unsigned long long expired_time){
    __asm__ __volatile__(
        "mrs x1, cntfrq_el0\n\t"    // cntfrq_el0 -> frequency of the timer
        "mul x1, x1, %0\n\t"        // cntpct_el0 = cntfrq_el0 * seconds: relative timer to cntfrq_el0
        "msr cntp_tval_el0, x1\n\t" // Set expired time to cntp_tval_el0, which stores time value of EL1 physical timer.
    :"=r" (expired_time));
}

// 設定核心計時器的中段觸發時間(明確的時間點)
// 使用tick值幫助設定
void set_core_timer_interrupt_by_tick(unsigned long long tick){
    __asm__ __volatile__(
        "msr cntp_cval_el0, %0\n\t"  //cntp_cval_el0 -> absolute timer
    :"=r" (tick));
    // tick的值移動到cntp_cval_el0 暫存器中
    // 注目!當cntp_cval_el0(標準答案)與cntpct_el0(實際計數)相等時
    // 觸發timer interrupt   
}

// 計算event數量
int timer_list_get_size(){
    int r = 0;
    struct list_head* curr;
    list_for_each(curr,timer_event_list)
    {
        r++;
    }
    return r;
}
