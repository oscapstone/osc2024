#ifndef ARRAY_H
#define ARRAY_H

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]) + __must_be_array(arr))

#define __must_be_array(arr) BUILD_BUG_ON_ZERO(__same_type((arr), &(arr)[0]))

#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#endif /* ARRAY_H */
