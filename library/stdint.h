#ifndef STDINT_H
#define STDINT_H

#ifndef __bool_defined
typedef int                          bool;
#define __bool_defined
#endif

#define true 1
#define false 0


#ifndef __uint32_t_defined
typedef unsigned int                 uint32_t;
#define __uint32_t_defined
#endif

#ifndef __uint64_t_defined
typedef unsigned long long int       uint64_t;
#define __uint64_t_defined
#endif




#endif