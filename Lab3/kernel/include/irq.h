#ifndef IRQ_H
#define IRQ_H

extern void irq_vector_init(void);

extern void enable_fiq(void);
extern void disable_fiq(void);
extern void enable_irq(void);
extern void disable_irq(void);
extern void enable_serror(void);
extern void disable_serror(void);
extern void enable_debug(void);
extern void disable_debug(void);
extern void enable_all_exception(void);
extern void disable_all_exception(void);

#endif /* IRQ_H */
