#include "timer.h"
#include "uart1.h"
#include "heap.h"
#include "u_string.h"

#define STR(x) #x
#define XSTR(s) STR(s)

struct list_head* timer_event_list;  // first head has nothing, store timer_event_t after it 
// 創建一個timer event
void timer_list_init(){
    INIT_LIST_HEAD(timer_event_list);
}

// cntp_ctl_el0: https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/CNTP-CTL-EL0--Counter-timer-Physical-Timer-Control-register?lang=en
void core_timer_enable(){ 
    __asm__ __volatile__(
        /*以下兩行主要用來將cntp_ctl_el0的[bit0=1]：*/
        "mov x1, 1\n\t"            // x1<-1
        "msr cntp_ctl_el0, x1\n\t" // To enable the timer’s interrupt, you need to set cntp_ctl_el0[bit:0] to 1.
                                   // cntp_ctl_el0[bit:0]: enable, Control register for the EL1 physical timer
                                   // cntp_tval_el0: Holds the timer value for the EL1 physical timer
        /*以下三行主要用來將CORE0_TIMER_IRQ_CTRL中[bit1:1]：IRQ Enabled*/
        "mov x2, 2\n\t"            // x2<-2
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t" // x1<-CORE0_TIMER_IRQ_CTRL 這裡使用 XSTR 和 STR 這兩個是為了將 CORE0_TIMER_IRQ_CTRL 這個值轉換成字符串，然後才能在組合語言中使用。
                                   // CORE0_TIMER_IRQ_CTRL=0x40000040: 這個地址為core0 timer IRQ control register的address(QA7_rev3.4: P.7)
        "str w2, [x1]\n\t"         // w2->[x1]:將w2中的值(也就是2)存到x1也就是CORE0_TIMER_IRQ_CTRL指向的位置
                                   // QA7_rev3.4.pdf p.13: Core0 Timer IRQ allows Non-secure physical timer(nCNTPNSIRQ)bit 1設為1=> 2: IRQ Enabled
                                   // 0b10 =2
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

void timer_set2sAlert(char* str)
{
    unsigned long long cntpct_el0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t": "=r"(cntpct_el0)); // tick auchor
    unsigned long long cntfrq_el0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); // tick frequency
    uart_sendline("[Interrupt][el1_irq][%s] %d seconds after booting\n", str, cntpct_el0/cntfrq_el0);
    add_timer(timer_set2sAlert,2,"2sAlert");
}


void add_timer(void *callback, unsigned long long timeout, char* args){
    timer_event_t* the_timer_event = kmalloc(sizeof(timer_event_t)); // free by timer_event_callback
    // store all the related information in timer_event
    the_timer_event->args = kmalloc(strlen(args)+1); // show在screen上的msg，加一是為了結尾的空字符。
    strcpy(the_timer_event -> args,args); //將參數 args 複製到新分配的記憶體中。
    the_timer_event->interrupt_time = get_tick_plus_s(timeout); //設定事件的觸發時間(看timeout是幾秒)，為當前的計時器 tick 加上根據秒數計算出的 tick 數。
                                                                // tick = freq * second
    the_timer_event->callback = callback;
    INIT_LIST_HEAD(&the_timer_event->listhead);

    // add the timer_event into timer_event_list (sorted)
    struct list_head* curr;
    list_for_each(curr,timer_event_list) //list_for_each-iterate over a list: 用curr指針去遍歷整條linked list
    {
        if(((timer_event_t*)curr)->interrupt_time > the_timer_event->interrupt_time) //Priority sorting
        {
            list_add(&the_timer_event->listhead,curr->prev);  // add this timer at the place just before the bigger one (sorted)
            /*在遍歷過程中，檢查當前節點 curr 轉型為 timer_event_t 指針所指向的事件的 interrupt_time 是否大於新事件 
            the_timer_event 的 interrupt_time。這是一個排序條件，
            用於確定新事件應該被插入的位置。
            換句話說，它在尋找第一個預設觸發時間比新事件(the_timer_event)晚的現有事件(curr)，為了把linked list變成：[...->the_timer_event -> curr->...]*/
            /*一旦找到了符合條件的 curr 節點，這行代碼將新事件(the_timer_event)插入到 curr 節點之前。*/
            break;
        }
    }
    // if the timer_event is the biggest, run this code block
    if(list_is_head(curr,timer_event_list))//代表新事件the_timer_event的觸發時間(interrupt time最晚)，所以直接插入tail(priority最低)
    {
        list_add_tail(&the_timer_event->listhead,timer_event_list);
    }
    // set interrupt to first event
    // 設定下一個定時器事件的中斷觸發點
    set_core_timer_interrupt_by_tick(((timer_event_t*)timer_event_list->next)->interrupt_time);
}

// cntpct_el0: ARM-v8 manual: P.D8-2183
// get cpu tick add some second
unsigned long long get_tick_plus_s(unsigned long long second){
    unsigned long long cntpct_el0=0;
    __asm__ __volatile__("mrs %0, cntpct_el0\n\t": "=r"(cntpct_el0)); // tick auchor 讀取 cntpct_el0，這是當前計時器的值，表示自啟動以來的 tick 數。
    unsigned long long cntfrq_el0=0;
    __asm__ __volatile__("mrs %0, cntfrq_el0\n\t": "=r"(cntfrq_el0)); // tick frequency 讀取 cntfrq_el0，這是計時器的頻率，用於將秒轉換為 tick 數。
    return (cntpct_el0 + cntfrq_el0*second);
}

// set timer interrupt time to [expired_time] seconds after now (relatively)
// cntp_tval_el0: 用於存儲到下一次定時器中斷前的相對時間值（以計數器的時鐘週期數表示）。當這個計數值遞減到零時，就會觸發一個物理定時器的中斷。
// 是使用clock cycle做timer(相對時間)
// cntp_cval_el0: 它儲存一個絕對的時鐘週期數，當系統計數器達到這個值時就會觸發中斷。(絕對時間)
void set_core_timer_interrupt(unsigned long long expired_time){
    __asm__ __volatile__(
        "mrs x1, cntfrq_el0\n\t"    // cntfrq_el0 -> frequency of the timer
        "mul x1, x1, %0\n\t"        // cntpct_el0 = cntfrq_el0 * seconds: relative timer to cntfrq_el0
        "msr cntp_tval_el0, x1\n\t" // Set expired time to cntp_tval_el0, which stores time value of EL1 physical timer.
    :"=r" (expired_time));
}

// Advanced Exercise 1 - Timer Multiplexing
// directly set timer interrupt time to a cpu tick  (directly)
void set_core_timer_interrupt_by_tick(unsigned long long tick){
    __asm__ __volatile__(
        "msr cntp_cval_el0, %0\n\t"  //cntp_cval_el0 -> absolute timer
    :"=r" (tick));
}

// get timer pending queue size
int timer_list_get_size(){
    int r = 0;
    struct list_head* curr;
    list_for_each(curr,timer_event_list)
    {
        r++;
    }
    return r;
}
