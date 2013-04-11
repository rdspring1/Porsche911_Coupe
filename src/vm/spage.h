#ifndef VM_SPAGE_H
#define VM_SPAGE_H

#include <debug.h>
#include "lib/kernel/hash.h"

enum page_status
{
    MEMORY,   /* Page currently stored in memory */
    DISK,     /* Page stored on the disk */
    SWAP,     /* Page swapped out of memory */
    ZERO      /* Page completely set to zero */
};

struct spage
{
    struct hash_elem hash_elem; /* Hash table element. */
    const void *addr;           /* User Virtual address. */
    enum page_status state;     /* Location of Page */
    bool readonly;              /* Read Only Setting */
    uint32_t swapindex;         /* Index in the Swap Table */
};

unsigned spage_hash_hash_func (const struct hash_elem *e, void *aux UNUSED);
bool spage_hash_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);
struct spage * spage_lookup (struct hash * pages, const void *address);
struct spage * spage_delete (struct hash * pages, const void *address);

#endif
