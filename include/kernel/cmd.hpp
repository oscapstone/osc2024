#pragma once

int runcmd(const char*, int);

int cmd_help(int, char*[]);
int cmd_hello(int, char*[]);
int cmd_hwinfo(int, char*[]);
int cmd_reboot(int, char*[]);
int cmd_ls(int, char*[]);
int cmd_cat(int, char*[]);
int cmd_alloc(int, char*[]);

using cmd_fp = int (*)(int, char*[]);

struct Cmd {
  const char* _name;
  const char* _help;
  cmd_fp _fp;
  inline const char* name() const;
  inline const char* help() const;
  inline cmd_fp fp() const;
};

extern const Cmd cmds[];
extern const int ncmd;
extern int help_idx;

// some hack to reloc address
template <typename T>
T REL(T addr) {
  return (T)((char*)addr - (char*)cmds[help_idx]._fp + (char*)&cmd_help);
}

inline const char* Cmd::name() const {
  return REL(this->_name);
}
inline const char* Cmd::help() const {
  return REL(this->_help);
}
inline cmd_fp Cmd::fp() const {
  return REL(this->_fp);
}
