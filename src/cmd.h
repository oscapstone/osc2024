#pragma once

typedef struct Cmd Cmd_t;
typedef void (*CmdFp)(const Cmd_t*);

struct Cmd {
  const char* name;
  const char* help;
  CmdFp fp;
};

extern const Cmd_t cmds[];
extern const Cmd_t cmds_end[];
extern const int n_cmds;
