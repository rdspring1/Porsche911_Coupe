#include <debug.h>
#include "vm/swap.h"
#include "threads/malloc.h"

#define SECTORS_PER_PAGE 8

// Function Prototype
static int find_slot(struct swap_t * st); 
static void swap_delete(struct swap_t * st, uint32_t slot);

static int find_slot(struct swap_t *st)
{
	lock_acquire(&st->lock);
	
	size_t slot;
	size_t num_bits = bitmap_size(st->bitmap);
	slot = bitmap_scan (st->bitmap, 0, num_bits, 0);
	lock_release(&st->lock);

	if (size_t == BITMAP_ERROR)
		return -1;
	else
		return (int) slot;
}

static void swap_delete(struct swap_t *st, uint32_t slot)
{
	lock_acquire(&st->lock);
	bitmap_reset(st->bitmap, slot);
	lock_release(&st->lock);
}

struct swap_t *swap_init()
{
   struct swap_t * st = (struct swap_t *) malloc(sizeof(struct swap_t));
   if(st == NULL)
      return NULL;

   struct block * swapdisk = block_get_role (BLOCK_SWAP);
   if(swapdisk == NULL)
      return NULL;

   st->swapblock = swapdisk;
   st->bitmap = *bitmap_create (block_size(swapdisk) / SECTORS_PER_PAGE);
   lock_init(&st->lock);
   return st;
}

bool swap_read(uint32_t slot, struct swap_t * st, void *readptr)
{
   uint32_t i;
   for (i = 0; i < SECTORS_PER_PAGE; ++i)
   {
      block_read (st->swapblock, slot * SECTORS_PER_PAGE + i, readptr);
   }

   swap_delete(st, slot);
   return true;
}

int swap_slots_in_use(struct swap_t *st)
{
	size_t num_bits = bitmap_size(st->bitmap);
	return (int) bitmap_count (st, 0, num_bits, 1);
}

int swap_write(struct swap_t * st, void *writeptr)
{
   int slot = find_slot(st);

   // No More Stack Space
   if(slot == -1)
      return slot;

   uint32_t i;
   for (i = 0; i < SECTORS_PER_PAGE; ++i)
   {
      block_write (st->swapblock, slot * SECTORS_PER_PAGE + i, *writeptr);
   }
   return slot;
}
