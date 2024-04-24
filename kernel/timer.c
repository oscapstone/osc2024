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

    // create timer_t
    timer_t *timer = init_timer(message, second);
    append_timer(timer);
}

timer_t *init_timer(char *msg, int second)
{
    timer_t *timer = (timer_t *) malloc(sizeof(timer_t));

    timer->message = msg;
    timer->second = second;
    timer->prev = NULL;
    timer->next = NULL;

    return timer;
}

void append_timer(timer_t *timer)
{
    if (timer_head == NULL) {
        timer_head = timer;
        return;
    }

    timer_t *cur = timer_head;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = timer;
    timer->prev = cur;
}

void update_timers(void)
{
    timer_t *cur = timer_head;
    if (cur == NULL) {
        return;
    }

    while (cur != NULL) {
        cur->second--;
        if (cur->second == 0) {
            uart_puts(cur->message);

            // TODO: remove this one
        }
        cur = cur->next;
    }
}