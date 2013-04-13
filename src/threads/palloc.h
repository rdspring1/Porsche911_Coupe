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


#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stddef.h>
#include "lib/kernel/list.h"
#include "threads/thread.h"

/* How to allocate pages. */
enum palloc_flags
  {
    PAL_ASSERT = 001,           /* Panic on failure. */
    PAL_ZERO = 002,             /* Zero page contents. */
    PAL_KERNEL = 003,           /* Kernel page. */
    PAL_USER = 004              /* User page. */
  };

struct frame
{
	struct thread * t;
	void* upage;
	void* kpage;
};

void palloc_init (size_t user_page_limit);
void *palloc_get_page (enum palloc_flags);
void *frame_selector (void* upage, enum palloc_flags);
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);
void palloc_free_page (void *);
void palloc_free_multiple (void *, size_t page_cnt);

#endif /* threads/palloc.h */
