#include "console.h"
#include "../lib/string.h"
#include "command/all.h"
#include "io.h"

struct Console {
  struct Command commands[64];
  int num_commands;
};

struct Console console;

struct Console *console_create() {
  console.num_commands = 0;
  return &console;
}

void _readcmd(char *x);
void _print_help(struct Console *console);

void init_console(struct Console *console) { console->num_commands = 0; }

void run_console(struct Console *console) {
  char input[256];
  while (1) {
    print_string("\n>>> ");
    _readcmd(input);
    if (input[0] == '\0') {
      continue;
    }
    if (strcmp(input, "help") == 0) {
      _print_help(console);
      continue;
    }
    int i = 0;
    for (i = 0; i < console->num_commands; i++) {
      if (strcmp(input, console->commands[i].name) == 0) {
        // creaete empty argc, argv
        int argc = 0;
        char *argv[16];
        console->commands[i].function(argc, argv);
        break;
      }
    }
    print_h(i);
    if (i == console->num_commands) {
      print_string("\ncommand not found\n");
    }
  }
}

void register_command(struct Console *console, struct Command *command) {
  console->commands[console->num_commands] = *command;
  console->num_commands++;
}

void _readcmd(char *x) {
  char input_char;
  x[0] = 0;
  int input_index = 0;
  while ((input_char = read_c()) != '\n') {
    x[input_index] = input_char;
    input_index += 1;
    print_char(input_char);
  }

  x[input_index] = 0;
}

void _print_help(struct Console *console) {
  print_string("\n");

  print_string("help : print this help menu\n");
  for (int i = 0; i < console->num_commands; i++) {
    print_string(console->commands[i].name);
    print_string(" : ");
    print_string(console->commands[i].description);
    print_string("\n");
  }
}
