void memzero(void* start, void* end) {
  for (long* i = start; i != end; i++)
    *i = 0;
}

void kernel_main() {
  for (;;)
    ;
}
