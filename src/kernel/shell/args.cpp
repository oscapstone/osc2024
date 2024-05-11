#include "shell/args.hpp"

#include "string.hpp"

Args::Args(const char* buf_, int len) {
  auto buf = new char[len + 1];
  memcpy(buf, buf_, len);
  buf[len] = 0;

  int argc = 1;
  for (int i = 0; i < len; i++)
    if (buf[i] == ' ') {
      buf[i] = '\0';
      argc++;
    }

  auto argv = new char*[argc + 1];
  argv[0] = buf;
  for (int i = 1; i < argc; i++)
    argv[i] = argv[i - 1] + strlen(argv[i - 1]) + 1;
  argv[argc] = nullptr;

  init(argv);

  delete[] buf;
  delete[] argv;
}
