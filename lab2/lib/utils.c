#include "utils.h"

int align4(int n) { return n + (4 - n % 4) % 4; }
