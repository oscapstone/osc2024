#pragma once

typedef struct Cmd cmd_t;
typedef void (*CmdFp)(const cmd_t*);

struct Cmd {
  const char* name;
  const char* help;
  CmdFp fp;
};

extern const cmd_t cmds[];
extern const cmd_t cmds_end[];
extern const int n_cmds;
