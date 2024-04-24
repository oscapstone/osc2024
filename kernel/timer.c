#include "malloc.h"
#include "string.h"
#include "timer.h"
#include "uart.h"

void add_timer(void)
{
    char *line = NULL;
    getline(&line, 0x20);

    char *message = (char *) malloc(sizeof(char) * 50);
    int second = 0;

    // message
    int i = 0;
    for (i; line[i] != ' '; i++) {
        message[i] = line[i];
    }
    message[i+1] = '\0';

    // second
    for (i; i < strlen_new(line); i++) {
        if (line[i] < '0' || line[i] > '9') {
            continue;
        }
        second = 10 * second + (line[i] - '0');
    }

    messages[timer_index] = message;
    seconds[timer_index] = second;
    timer_index++;

    // // create timer_t
    // timer_t *timer = init_timer(message, second);
    // uart_send('c');
    // // uart_send('\n');
    // append_timer(timer);
}

// timer_t *init_timer(char *msg, int second)
// {
//     // uart_send('d');
//     // timer_t *timer = (timer_t *) malloc(sizeof(timer_t));
//     // uart_send('e');
//     // uart_hex(timer);
//     // timer->second = second;
//     // uart_send('f');
//     // timer->prev = NULL;
//     // uart_send('g');
//     // timer->next = NULL;
//     // uart_send('h');
//     // timer->message = msg;   // THIS breaks
//     // uart_send('i');

//     // return timer;
// }

// void append_timer(timer_t *timer)
// {
//     // uart_send('j');
//     // if (timer_head == NULL) {
//     //     uart_send('k');
//     //     timer_head = timer;
//     //     return;
//     // }

//     // uart_send('l');
//     // timer_t *cur = timer_head;
//     // while (cur->next != NULL) {
//     //     cur = cur->next;
//     //     uart_send('m');
//     // }
//     // uart_send('n');
//     // // uart_send('\n');
//     // cur->next = timer;
//     // uart_send('o');
//     // timer->prev = cur;
//     // uart_send('p');
// }

void update_timers(void)
{
    for (int i = 0; i < timer_index; i++) {
        if (seconds[i] >= 0) {
            seconds[i]--;
        }
        if (seconds[i] == 0) {
            uart_puts(messages[i]);
        }
    }
    // timer_t *cur = timer_head;
    // if (cur == NULL) {
    //     return;
    // }

    // while (cur != NULL) {
    //     cur->second--;
    //     if (cur->second == 0) {
    //         uart_puts(cur->message);

    //         // TODO: remove this one
    //     }
    //     cur = cur->next;
    // }
}