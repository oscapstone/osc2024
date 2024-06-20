#define CMD_MAX_LEN 32
#define FILENAME_MAX_LEN 32
int cmd_help();

int cmd_hello();

int shell();
int cmd_handler(char* cmd);
void flush_buffer(char *buffer, int length);