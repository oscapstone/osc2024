#include "irq.h"
#include "arm/sysregs.h"
#include "bool.h"
#include "def.h"
#include "entry.h"
#include "list.h"
#include "memory.h"
#include "mini_uart.h"
#include "peripheral/mini_uart.h"
#include "peripheral/timer.h"
#include "signal.h"
#include "slab.h"
#include "timer.h"
#include "utils.h"

#define alloc_irq()   (irq_task_t*)kmem_cache_alloc(irq, 0)
#define free_irq(ptr) kmem_cache_free(irq, (ptr))

static LIST_HEAD(irq_task_head);
static struct kmem_cache* irq;

typedef struct irq_task {
    irq_callback handler;    // irq task handler
    unsigned long priority;  // lower value means higher priority
    bool is_running;         // is task running
    struct list_head list;   // list node
} irq_task_t;


int irq_init(void)
{
    irq = kmem_cache_create("irq", sizeof(irq_task_t), -1);
    if (!irq)
        return 0;
    return 1;
}

static irq_task_t* create_irq_task(irq_callback handler, unsigned long priority)
{
    irq_task_t* new_irq_task = alloc_irq();

    if (!new_irq_task)
        return NULL;

    INIT_LIST_HEAD(&new_irq_task->list);

    new_irq_task->handler = handler;
    new_irq_task->is_running = false;
    new_irq_task->priority = priority;

    return new_irq_task;
}

static void insert_irq_task(irq_task_t* new_task)
{
    if (!new_task)
        return;

    irq_task_t* node = NULL;
    list_for_each_entry (node, &irq_task_head, list) {
        if (node->priority > new_task->priority)
            break;
    }

    list_add_tail(&new_task->list, &node->list);
}

#define irq_task_not_running(task) (!((task)->is_running))

static void run_irq_task(void)
{
    irq_task_t* first_task = NULL;
    while (!list_empty(&irq_task_head) &&
           irq_task_not_running(first_task = list_first_entry(
                                    &irq_task_head, irq_task_t, list))) {
        enable_all_exception();
        list_del_init(&first_task->list);
        first_task->is_running = true;
        first_task->handler();
        disable_all_exception();

        free_irq(first_task);
    }
}



const char* entry_error_type[] = {"SYNC_INVALID_EL1t",   "IRQ_INVALID_EL1t",
                                  "FIQ_INVALID_EL1t",    "ERROR_INVALID_EL1t",

                                  "SYNC_INVALID_EL1h",   "IRQ_INVALID_EL1h",
                                  "FIQ_INVALID_EL1h",    "ERROR_INVALID_EL1h",

                                  "SYNC_INVALID_EL0_64", "IRQ_INVALID_EL0_64",
                                  "FIQ_INVALID_EL0_64",  "ERROR_INVALID_EL0_64",

                                  "SYNC_INVALID_EL0_32", "IRQ_INVALID_EL0_32",
                                  "FIQ_INVALID_EL0_32",  "ERROR_INVALID_EL0_32",
                                  "SYNC_ERROR",          "SYSCALL_ERROR"};

void show_invalid_entry_message(int type,
                                unsigned long spsr,
                                unsigned long esr,
                                unsigned long elr)
{
    disable_all_exception();

    uart_send_string(entry_error_type[type]);
    uart_send_string(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter
    // D10.2.28)
    switch (esr >> ESR_ELx_EC_SHIFT) {
    case 0b000000:
        uart_send_string("Unknown");
        break;
    case 0b000001:
        uart_send_string("Trapped WFI/WFE");
        break;
    case 0b001110:
        uart_send_string("Illegal execution");
        break;
    case 0b010101:
        uart_send_string("System call");
        break;
    case 0b100000:
        uart_send_string("Instruction abort, lower EL");
        break;
    case 0b100001:
        uart_send_string("Instruction abort, same EL");
        break;
    case 0b100010:
        uart_send_string("Instruction alignment fault");
        break;
    case 0b100100:
        uart_send_string("Data abort, lower EL");
        break;
    case 0b100101:
        uart_send_string("Data abort, same EL");
        break;
    case 0b100110:
        uart_send_string("Stack alignment fault");
        break;
    case 0b101100:
        uart_send_string("Floating point");
        break;
    default:
        uart_send_string("Unknown");
        break;
    }
    // decode data abort cause
    if (esr >> ESR_ELx_EC_SHIFT == 0b100100 ||
        esr >> ESR_ELx_EC_SHIFT == 0b100101) {
        uart_send_string(", ");
        switch ((esr >> 2) & 0x3) {
        case 0:
            uart_send_string("Address size fault");
            break;
        case 1:
            uart_send_string("Translation fault");
            break;
        case 2:
            uart_send_string("Access flag fault");
            break;
        case 3:
            uart_send_string("Permission fault");
            break;
        }
        switch (esr & 0x3) {
        case 0:
            uart_send_string(" at level 0");
            break;
        case 1:
            uart_send_string(" at level 1");
            break;
        case 2:
            uart_send_string(" at level 2");
            break;
        case 3:
            uart_send_string(" at level 3");
            break;
        }
    }

    // dump registers
    uart_send_string(":\n, SPSR: 0x");
    uart_send_hex(spsr >> 32);
    uart_send_hex(spsr);
    uart_send_string(", ESR: 0x");
    uart_send_hex(esr >> 32);
    uart_send_hex(esr);
    uart_send_string(", ELR: 0x");
    uart_send_hex(elr >> 32);
    uart_send_hex(elr);
    uart_send_string("\n");

    enable_all_exception();
}

void irq_handler(void)
{
    // uart_printf("irq_handler\n");
    disable_all_exception();

    irq_task_t* task = NULL;

    unsigned int irq_pending_1 = get32(IRQ_PENDING_1);
    unsigned int core_irq_source = get32(CORE0_IRQ_SOURCE);

    if (irq_pending_1 & IRQ_PENDING_1_AUX_INT) {
        unsigned int iir = get32(AUX_MU_IIR_REG);
        unsigned int int_id = iir & INT_ID_MASK;

        if (int_id == RX_INT) {
            clear_rx_interrupt();
            task = create_irq_task(uart_rx_handle_irq, UART_IRQ_PRIORITY);
        } else if (int_id == TX_INT) {
            clear_tx_interrupt();
            task = create_irq_task(uart_tx_handle_irq, UART_IRQ_PRIORITY);
        }

    } else if (core_irq_source & CNTPNSIRQ) {
        disable_core0_timer();
        task = create_irq_task(core_timer_handle_irq, TIMER_IRQ_PRIORITY);
    } else {
        uart_send_string("Unknown pending interrupt\n");
        return;
    }

    insert_irq_task(task);
    run_irq_task();
    enable_all_exception();

    handle_sig();
    // uart_printf("irq_handler end\n");
}
