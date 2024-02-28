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

#endif // CMD_H