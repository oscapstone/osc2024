#include "headers/shell.h"
#include "headers/utils.h"
#include "headers/command.h"
#include "headers/uart.h"

void read_command(char *buffer)
{
    int ind = 0;
    while(1)
    {
        buffer[ind] = receive();
        if(buffer[ind] == 127 && ind>0){ send(127); ind--; continue;}
        else if(buffer[ind] == 127 && ind==0){continue;}
        else send(buffer[ind]);

        if(buffer[ind] == '\n')
        {
            buffer[ind] = '\0';
            // buffer[ind+1] = '\n';
            break;
        }
        ind++;
    }
}

void parsing(char *buffer)
{
    char *command = buffer;
    char *parameters[MAX_PARAM_NUMBER];
    int num_param = 0;
    int len = strlen(buffer);
    for(int i=0 ; i<len ; i++)
    {
        if(command[i] == ' ')
        {
            command[i] = '\0';
            parameters[num_param++] = command+i+1;
        }
    }

    display("\r");
    exec(command, parameters, num_param);
}

void exec(char *command, char **parameters, int num_param)
{
    if(strcmp(command, COMMAND_HELP))           help();
    else if(strcmp(command, COMMAND_HELLO))     hello();
    else if(strcmp(command, COMMAND_INFO))      info();
    else if(strcmp(command, COMMAND_REBOOT))    reboot();
    else if(strcmp(command, COMMAND_LS))        ls();
    else if(strcmp(command, COMMAND_CAT))       cat(parameters[0]);
    else if(strcmp(command, COMMAND_MALLOC))    malloc(parameters[0]);
    else undefined();
}

void char2ascii(char ch, char *ascii)
{
    int x = ch;
    ascii[2] = x%10+'0';
    x /= 10;
    ascii[1] = x%10+'0';
    x /= 10;
    ascii[0] = x%10+'0';
}

void shell()
{
    display("Welcome! Try \"help\" to get the instruction menu~~~\n");
    while(1)
    {
        display("# ");
        char buffer[MAX_BUFFER_SIZE];
        read_command(buffer);
        parsing(buffer); 
    }
}