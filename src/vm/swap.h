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

#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <debug.h>
#include "devices/block.h"
#include "threads/synch.h"

struct swap_t
{
	struct block * swapblock; /* Swap Disk Block Struct */
	uint32_t * bitmap;        /* Track Swap Slots */
	int size;            /* Number of Swap Slots */
	uint32_t inuse;           /* Number of Used Swap Slots */
	struct lock lock;         /* Lock for Swap Table */
};

struct swap_t * swap_init(void);
bool swap_read(uint32_t slot, struct swap_t * st, void* readptr);
int swap_write(struct swap_t * st, void* writeptr);

#endif
