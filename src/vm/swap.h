#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <debug.h>
#include "devices/block.h"
#include "threads/synch.h"

struct swap_t
{
   struct block *swapblock; /* Swap Disk Block Struct */
   struct bitmap *bitmap;        /* Track Swap Slots */
   struct lock lock;         /* Lock for Swap Table */
};

struct swap_t *swap_init(void);
bool swap_read(uint32_t slot, struct swap_t *st, void *readptr);
int swap_slots_in_use(struct swap_t *st);
int swap_write(struct swap_t *st, void *writeptr);

#endif
