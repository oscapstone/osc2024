#ifndef PAGE_FLAGS_H
#define PAGE_FLAGS_H

#include "mm.h"
#include "page_alloc.h"
#include "utils.h"

enum pageflags { PG_head, PG_slab };

#define TESTPAGEFLAG(uname, lname)                   \
    static inline int Page##uname(struct page* page) \
    {                                                \
        return test_bit(PG_##lname, &page->flags);   \
    }

#define SETPAGEFLAG(uname, lname)                        \
    static inline void SetPage##uname(struct page* page) \
    {                                                    \
        return set_bit(PG_##lname, &page->flags);        \
    }

#define CLEARPAGEFLAG(uname, lname)                        \
    static inline void ClearPage##uname(struct page* page) \
    {                                                      \
        return clear_bit(PG_##lname, &page->flags);        \
    }


TESTPAGEFLAG(Slab, slab);
SETPAGEFLAG(Slab, slab);
CLEARPAGEFLAG(Slab, slab);

TESTPAGEFLAG(Head, head);
SETPAGEFLAG(Head, head);
CLEARPAGEFLAG(Head, head);

static inline int PageTail(struct page* page)
{
    return (page->compound_head) & 1;
}

static inline int PageCompound(struct page* page)
{
    return test_bit(PG_head, &page->flags) || PageTail(page);
}

#endif /* PAGE_FLAGS_H */
