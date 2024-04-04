#ifndef _SHELL_H
#define _SHELL_H

#include "int.h"
typedef struct cmd_s {
  char *name;
  char *help;
  void (*execute)(char *args, u32_t arg_size);
} cmd_t;

typedef struct shell_s {
  cmd_t cmds[64];
  u32_t n_cmds;
} shell_t;

void register_cmds(shell_t *s);
void shell_loop(const shell_t *shell);

#endif  // _SHELL_H
