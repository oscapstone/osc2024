#pragma once

extern "C" {
// exec.S
void exec_user_prog(void* text, void* stack);
}
