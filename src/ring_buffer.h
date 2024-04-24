#ifndef _DEF_RING_BUFFER
#define _DEF_RING_BUFFER

typedef struct {
    int size;
    int start;
    int end;
    char *ele;
} rbuffer_t;

void rb_init(rbuffer_t *buf, int size);
int rb_empty(rbuffer_t *buf);
int rb_full(rbuffer_t *buf);
void rb_write(rbuffer_t *buf, char c);
char rb_read(rbuffer_t *buf);
void rb_print(rbuffer_t *buf);

// // https://zh.wikipedia.org/zh-tw/%E7%92%B0%E5%BD%A2%E7%B7%A9%E8%A1%9D%E5%8D%80
// typedef struct {
//     int size;
//     int start;
//     int end;
//     char *els;
// } ring_buffer_t;

// void rb_init(ring_buffer_t *, int);
// void rb_print(ring_buffer_t *);
// int rb_is_full(ring_buffer_t *);
// int rb_is_empty(ring_buffer_t *);
// int rb_incr(ring_buffer_t *, int);
// void rb_write(ring_buffer_t *, char);
// char rb_read(ring_buffer_t *);

#endif
