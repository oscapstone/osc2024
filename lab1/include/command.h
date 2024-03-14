#ifndef COMMAND_H
#define COMMAND_H

void command_help ();
void command_hello ();
void command_reboot ();
void command_cancel_reset ();

void command_info ();
void command_clear ();

void input_buffer_overflow_message ( char [] );
void command_not_found ();
#endif