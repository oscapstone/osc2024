#ifndef CMD_H
#define CMD_H

struct command {
    const char *name;
    const char *help;
    void (*func)(void);
};

extern struct command commands[];

// Commands
void help();
void hello();
void reboot();
void lshw();
void ls();
void cat();
void exec();
void clear();
void timeout();

#endif // CMD_H