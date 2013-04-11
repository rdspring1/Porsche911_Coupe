#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <debug.h>
#include "devices/block.h"

struct swap_t
{
   struct block * swapblock; /* Swap Disk Block Struct */
   uint32_t * bitmap;        /* Track Swap Slots */
   uint32_t size;            /* Number of Swap Slots */
   uint32_t inuse;           /* Number of Used Swap Slots */
};

int swap_read(uint32_t slot, struct swap_t * st, void** readptr);
int swap_write(uint32_t slot, struct swap_t * st, void** writeptr);
int swap_load(struct swap_t * st);
int swap_delete(struct swap_t * st, uint32_t slot);
struct swap_t * swap_init(void);

#endif
