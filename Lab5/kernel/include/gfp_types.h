#ifndef GFP_TYPES_H
#define GFP_TYPES_H


typedef unsigned int gfp_t;

enum {
    ___GFP_ZERO_BIT,
    ___GFP_COMP_BIT,
};

#define BIT(nr)    (1 << (nr))
#define __GFP_ZERO BIT(___GFP_ZERO_BIT)
#define __GFP_COMP BIT(___GFP_COMP_BIT)

#endif /* GFP_TYPES_H */
