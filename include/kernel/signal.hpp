#pragma once

#include "ds/list.hpp"
#include "int/exception.hpp"
#include "mm/mmu.hpp"

struct Kthread;

#define SIGKILL 9

constexpr int NSIG = 32;
using signal_handler = void (*)(int);

struct SigAction {
  bool in_kernel;
  signal_handler handler;
};

struct SignalItem : ListItem {
  int signal;
  SignalItem(int sig) : ListItem{}, signal(sig) {}
};

struct Signal {
  Kthread* cur;
  ListHead<SignalItem> list;
  SigAction actions[NSIG];
  uint64_t stack_addr = INVALID_ADDRESS;

  Signal(Kthread*);
  Signal(Kthread*, const Signal& other);
  Signal(const Signal&) = delete;

  void regist(int signal, signal_handler handler);
  void setall(signal_handler handler);
  void operator()(int signal);
  void handle(TrapFrame* frame = nullptr);
};

void signal_handler_nop(int);
void signal_handler_kill(int);

void signal_kill(int pid, int signal);
void signal_return(TrapFrame* frame);
void el0_sig_return();
