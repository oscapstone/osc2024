#ifndef BUILD_BUG_H
#define BUILD_BUG_H

#include "assert.h"

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int : -!!(e); }))

#define BUILD_BUG_ON_MSG(cond, msg) static_assert(!(cond), msg)

#define BUILD_BUG_ON(cond) BUILD_BUG_ON_MSG(cond, "BUILD_BUG_ON failed: " #cond)

#endif /* BUILD_BUG_H */
