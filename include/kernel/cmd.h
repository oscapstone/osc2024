#pragma once

typedef struct Cmd cmd_t;
typedef void (*cmd_fp)();

struct Cmd {
  const char* name;
  const char* help;
  cmd_fp fp;
};

extern const cmd_t cmds[];
extern const cmd_t cmds_end[];
extern const int n_cmds;
