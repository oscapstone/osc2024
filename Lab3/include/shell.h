#ifndef SHELL_H
#define SHELL_H

#define MAX_BUFFER_LEN 128

enum SPECIAL_CHARACTER
{
    BACK_SPACE = 127,   //del
    LINE_FEED = 10, //\n
    CARRIAGE_RETURN = 13,   //\r

    REGULAR_INPUT = 1000,
    NEW_LINE = 1001,

    UNKNOWN = -1,
};
void loop();
void shell_start();
enum SPECIAL_CHARACTER parse(char);
void command_controller(enum SPECIAL_CHARACTER, char c, char[], int *);

#endif