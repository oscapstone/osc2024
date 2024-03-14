#include "shell.h"
#include "string.h"
#include "command.h"
#include "uart.h"


void welcome_msg() {
    command_clear();
    command_info();
    uart_puts(
        "*** YADOS 0.01 for OSC 2024 ***\n"
        "Hello, there!\n"
        "Hopefully this will be Yet Another Dope OS!\n\n"
    );
}

void shell_start () {
    int buffer_counter = 0;
    char input_char;
    char buffer[MAX_BUFFER_LEN];
    enum SPECIAL_CHARACTER input_parse;

    strset (buffer, 0, MAX_BUFFER_LEN);   

    // new line head
    uart_puts("# ");

    // read input
    while(1) {
        input_char = uart_getc();
        input_parse = parse ( input_char );
        command_controller ( input_parse, input_char, buffer, &buffer_counter);
    }
}

enum SPECIAL_CHARACTER parse ( char c ) {
    if ( !(c < 128 && c >= 0) )
        return UNKNOWN;

    if ( c == BACK_SPACE )
        return BACK_SPACE;
    
    if ( c == DELETE )
        return DELETE;
    else if ( c == LINE_FEED || c == CARRIAGE_RETURN )
        return NEW_LINE;
    else
        return REGULAR_INPUT;    
}

void command_controller ( enum SPECIAL_CHARACTER input_parse, char c, char buffer[], int * counter ) {   
    if ( input_parse == UNKNOWN )
        return;
    
    // Special key
    if ( input_parse == BACK_SPACE) {
        if (  (*counter) > 0 ) {
            uart_send('\b');
            uart_send(' ');
            uart_send('\b');
            (*counter) --;
        }
    }
    if ( input_parse == DELETE ) {   
        if (  (*counter) > 0 ) {
            uart_send('\b');
            uart_send(' ');
            uart_send('\b');
            (*counter) --;
        }
    }
    else if ( input_parse == NEW_LINE ) {
        uart_send(c);
        if ( (*counter) == MAX_BUFFER_LEN ) {
            input_buffer_overflow_message(buffer);
        }
        else if (strlen(buffer) > 0) {
            buffer[(*counter)] = '\0';

            if ( !strcmp(buffer, "help" ) ) 
                command_help();
            else if ( !strcmp(buffer, "hello" ) )
                command_hello();
            else if ( !strcmp(buffer, "reboot" ) ) 
                command_reboot();
            else if ( !strcmp(buffer, "info" ) ) 
                command_info();
            else if ( !strcmp(buffer, "clear" ) ) 
                command_clear();
            else if ( !strcmp(buffer, "cc"))
                command_cancel_reset();
            else 
                command_not_found();
        }
            
        (*counter) = 0;
        strset (buffer, 0, MAX_BUFFER_LEN); 

        // new line head;
        uart_puts("# ");
    } else if ( input_parse == REGULAR_INPUT ) {
        uart_send(c);
        if ( *counter < MAX_BUFFER_LEN) {
            buffer[*counter] = c;
            (*counter) ++;
        }
    }
}