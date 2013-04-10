#include <debug.h>
#include "vm/swap.h"
#include "threads/malloc.h"

#define PAGESECTORSIZE 8

struct swap_t * swap_init()
{
   struct swap_t * st = (struct swap_t *) malloc(sizeof(struct swap_t));
   if(st == NULL)
      return NULL;

   struct block * swapdisk = block_get_role (BLOCK_SWAP);
   if(swapdisk == NULL)
      return NULL;

   st->swapblock = swapdisk;
   st->size = block_size(swapdisk) / PAGESECTORSIZE;
   st->bitmap = (int *) calloc(st->size, sizeof(uint32_t));
   st->inuse = 0;
   return st;
}

int swap_read(uint32_t slot, struct swap_t * st, void** readptr)
{
   if(slot >= st->size)
      return 0;

   uint32_t i;
   for (i = 0; i < PAGESECTORSIZE; ++i)
   {
      block_read (st->swapblock, slot * PAGESECTORSIZE + i, *readptr);
   }
   return 1;
}

int swap_write(uint32_t slot, struct swap_t * st, void** writeptr)
{
   if(slot >= st->size)
      return 0;

   uint32_t i;
   for (i = 0; i < PAGESECTORSIZE; ++i)
   {
      block_write (st->swapblock, slot * PAGESECTORSIZE + i, *writeptr);
   }
   return 1;
}

int swap_load(struct swap_t * st)
{
  uint32_t i;
  for (i = 0; i < st->size; i++)
  {
    	if (st->bitmap[i] == 0) {
			st->bitmap[i] = 1;
         ++st->inuse;
			return i;
		}
  }
  return -1; // completely filled array
}

int swap_delete(struct swap_t * st, uint32_t slot)
{
   if(slot >= st->size)
      return 0;

   st->bitmap[slot] = 0;
   ++st->inuse;
   return 1;
}
