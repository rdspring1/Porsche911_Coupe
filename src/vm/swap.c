// 4/12/2013
//Name1: Ryan Spring
//EID1: rds2367
//CS login: rdspring
//Email: rdspring1@gmail.com
//Unique Number: 53426
//
//Name2: Jimmy Kettler
//EID2: jbk97 
//CS login: jimmyk3t
//Email: jimmy.kettler@gmail.com
//Unique Number: 53426
//
//Name3: Benjamin Holder
//EID3: bdh874
//CS login: bdh874
//Email: benjamin.holder@utexas.edu
//Unique Number: 53430

#include <debug.h>
#include "stdio.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

#define PAGESECTORSIZE 8

// Function Prototype
int findslot(struct swap_t * st); 
void swap_delete(struct swap_t * st, uint32_t slot);

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
   st->bitmap = (uint32_t *) calloc(st->size, sizeof(uint32_t));
   st->inuse = 0;
   lock_init(&st->lock);
   return st;
}

bool swap_read(uint32_t slot, struct swap_t * st, void* readptr)
{
   uint32_t i;
   for (i = 0; i < PAGESECTORSIZE; ++i)
   {
      block_read (st->swapblock, slot * PAGESECTORSIZE + i, readptr);
	  readptr = (void*) (((uint8_t*) readptr) + BLOCK_SECTOR_SIZE);
   }

   swap_delete(st, slot);
   return true;
}

int swap_write(struct swap_t * st, void* writeptr)
{
   int slot = findslot(st);

   // No More Stack Space
   if(slot == -1)
      return slot;

   uint32_t i;
   for (i = 0; i < PAGESECTORSIZE; ++i)
   {
      block_write (st->swapblock, slot * PAGESECTORSIZE + i, writeptr);
	  writeptr = (void*) (((uint8_t*) writeptr) + BLOCK_SECTOR_SIZE);
   }
   return slot;
}

void swap_delete(struct swap_t * st, uint32_t slot)
{
   lock_acquire (&st->lock);
   st->bitmap[slot] = 0;
   --st->inuse;
   lock_release(&st->lock);
}

int findslot(struct swap_t * st) 
{
   lock_acquire (&st->lock);
   int slot;
   bool found = false;
   for (slot = 0; slot < st->size && !found; slot++)
   {
	   if (st->bitmap[slot] == 0) {
		   st->bitmap[slot] = 1;
           ++st->inuse;
           found = true;
	   }
   }
   lock_release(&st->lock);

   if(found)
      return slot-1;
   else
      return -1;
}
