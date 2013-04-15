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


#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "threads/loader.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "vm/spage.h"
#include "vm/swap.h"

/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.  See malloc.h for an allocator that
   hands out smaller chunks.

   System memory is divided into two "pools" called the kernel
   and user pools.  The user pool is for user (virtual) memory
   pages, the kernel pool for everything else.  The idea here is
   that the kernel needs to have memory for its own operations
   even if user processes are swapping like mad.

   By default, half of system RAM is given to the kernel pool and
   half to the user pool.  That should be huge overkill for the
   kernel pool, but that's just fine for demonstration purposes. */

// Page Eviction Algorithm
static size_t page_idx;
struct frame * frame_eviction(struct frame** framelist, enum palloc_flags flags, size_t page_cnt);
struct lock fevict;

void write_dirty_page(bool accessed, struct frame * f, struct spage * page);

// EXTERN - RDS
/* Swap Table RDS */
struct swap_t *swaptable;


/* A memory pool. */
struct pool
  {
    struct lock lock;                   /* Mutual exclusion. */
    struct bitmap *used_map;            /* Bitmap of free pages. */
    uint8_t *base;                      /* Base of pool. */
	size_t size;						/* Size of pool */
	size_t index;							/* Index for framelist */
    struct frame** framelist;           /* Array of frames */
  };

/* Two pools: one for kernel data, one for user pages. */
static struct pool kernel_pool, user_pool;

static void init_pool (struct pool *, void *base, size_t page_cnt,
                       const char *name);
static bool page_from_pool (const struct pool *, void *page);

/* Initializes the page allocator.  At most USER_PAGE_LIMIT
   pages are put into the user pool. */
void
palloc_init (size_t user_page_limit)
{
  // Initialize Frame Eviction - RDS
  lock_init(&fevict);

  /* Free memory starts at 1 MB and runs to the end of RAM. */
  uint8_t *free_start = ptov (1024 * 1024);
  uint8_t *free_end = ptov (init_ram_pages * PGSIZE);
  size_t free_pages = (free_end - free_start) / PGSIZE;
  size_t user_pages = free_pages / 2;
  size_t kernel_pages;
  if (user_pages > user_page_limit)
    user_pages = user_page_limit;
  kernel_pages = free_pages - user_pages;

  /* Give half of memory to kernel, half to user. */
  init_pool (&kernel_pool, free_start, kernel_pages, "kernel pool");
  init_pool (&user_pool, free_start + kernel_pages * PGSIZE, user_pages, "user pool");

  // Initialize Frame List - rds
  user_pool.framelist = (struct frame **) calloc(user_pages, sizeof(struct frame *));
  user_pool.index = 0;
}


void *frame_selector (void* upage, enum palloc_flags flags)
{
	struct frame * f = (struct frame *) malloc(sizeof(struct frame));
	f->t = thread_current();
	f->upage = upage;
	f->kpage = palloc_get_multiple(flags, 1);

	lock_acquire(&fevict);
	//free(user_pool.framelist[page_idx - 1]);
	user_pool.framelist[page_idx - 1] = f;
	lock_release(&fevict);
	return f->kpage;
}

/* Obtains and returns a group of PAGE_CNT contiguous free pages.
   If PAL_USER is set, the pages are obtained from the user pool,
   otherwise from the kernel pool.  If PAL_ZERO is set in FLAGS,
   then the pages are filled with zeros.  If too few pages are
   available, returns a null pointer, unless PAL_ASSERT is set in
   FLAGS, in which case the kernel panics. */
void *
palloc_get_multiple (enum palloc_flags flags, size_t page_cnt)
{
  struct pool *pool = flags & PAL_USER ? &user_pool : &kernel_pool;
  void *pages;

  if (page_cnt == 0)
    return NULL;

  lock_acquire (&pool->lock);
  page_idx = bitmap_scan_and_flip (pool->used_map, 0, page_cnt, false);
  lock_release (&pool->lock);

  if (page_idx != BITMAP_ERROR)
    pages = pool->base + PGSIZE * page_idx;
  else
    pages = NULL;

  if (pages != NULL) 
    {
      if (flags & PAL_ZERO)
	  {
        memset (pages, 0, PGSIZE * page_cnt);
	  }
    }
  else 
    {
      if (flags & PAL_ASSERT)
        PANIC ("palloc_get: out of pages");

	  // When the frame_table is full, panic the kernel (RDS)
	  // Implemenent Page EVICTION
	  if(flags & PAL_USER)
      {
		  struct frame * f = frame_eviction(pool->framelist, flags, page_cnt);
		  return f->kpage;
	  }
    }

  return pages;
}

// Second Chance Page Replacement Algorithm
struct frame * frame_eviction(struct frame** framelist, enum palloc_flags flags, size_t page_cnt)
{
    lock_acquire(&fevict);
    bool found = false;
    struct frame * f = NULL;
	while(!found)
	{		
		f = framelist[user_pool.index];
        struct spage * page = spage_lookup(&f->t->spagedir, f->upage);
		bool accessed = pagedir_is_accessed(f->t->pagedir, f->upage);
		bool dirty = pagedir_is_dirty(f->t->pagedir, f->upage);

   		lock_try_acquire(&page->spagelock);
		if(accessed && dirty) // Used and Modified
		{
         	write_dirty_page(true, f, page);
		}
		else if(!accessed && dirty) // Not Used but Modified
		{
         	write_dirty_page(false, f, page);
		}
		else if(accessed && !dirty) // Used but Not Modified
		{
         	pagedir_set_accessed(f->t->pagedir, f->upage, false);
		}
		else if(!accessed && !dirty) // Not Used or Modified
		{
         	found = true;
			page_idx = user_pool.index + 1;
		    pagedir_clear_page(f->t->pagedir, f->upage); 

	  		if(flags & PAL_ZERO)
            	memset (f->kpage, 0, PGSIZE * page_cnt);
		}
   		lock_release(&page->spagelock);

		++user_pool.index;
		if(user_pool.index == user_pool.size-1)
			user_pool.index = 0;
	}
    lock_release(&fevict);
	return f;
}

void write_dirty_page(bool accessed, struct frame * f, struct spage * page)
{
   lock_release(&fevict);

   page->state = SWAP;
   page->swapindex = swap_write(swaptable, f->kpage);

   // Panic Kernel if no more swap space
   ASSERT(page->swapindex != -1);

   pagedir_set_dirty(f->t->pagedir, f->upage, false);
   if(accessed)
      pagedir_set_accessed(f->t->pagedir, f->upage, false);

   lock_acquire(&fevict);
}

/* Obtains a single free page and returns its kernel virtual
   address.
   If PAL_USER is set, the page is obtained from the user pool,
   otherwise from the kernel pool.  If PAL_ZERO is set in FLAGS,
   then the page is filled with zeros.  If no pages are
   available, returns a null pointer, unless PAL_ASSERT is set in
   FLAGS, in which case the kernel panics. */
void *
palloc_get_page (enum palloc_flags flags) 
{
  return palloc_get_multiple (flags, 1);
}

/* Frees the PAGE_CNT pages starting at PAGES. */
void
palloc_free_multiple (void *pages, size_t page_cnt) 
{
  bool upool = false;
  struct pool *pool;
  size_t page_idx;

  ASSERT (pg_ofs (pages) == 0);
  if (pages == NULL || page_cnt == 0)
    return;

  if (page_from_pool (&kernel_pool, pages))
  {
    pool = &kernel_pool;
  }
  else if (page_from_pool (&user_pool, pages))
  { 
    upool = true;
    pool = &user_pool;
  }
  else
  {
    NOT_REACHED ();
  }

  page_idx = pg_no (pages) - pg_no (pool->base);

  if(upool && page_idx > 0)
  {
  	 lock_acquire(&fevict);
     free(pool->framelist[page_idx - 1]);
     pool->framelist[page_idx - 1] = NULL;

     #ifndef NDEBUG
     memset (pages, 0xcc, PGSIZE * page_cnt);
     #endif

     ASSERT (bitmap_all (pool->used_map, page_idx, page_cnt));
     bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);

  	 lock_release(&fevict);
	 return;
  }

#ifndef NDEBUG
  memset (pages, 0xcc, PGSIZE * page_cnt);
#endif

  ASSERT (bitmap_all (pool->used_map, page_idx, page_cnt));
  bitmap_set_multiple (pool->used_map, page_idx, page_cnt, false);
}

/* Frees the page at PAGE. */
void
palloc_free_page (void *page) 
{
  palloc_free_multiple (page, 1);
}

/* Initializes pool P as starting at START and ending at END,
   naming it NAME for debugging purposes. */
static void
init_pool (struct pool *p, void *base, size_t page_cnt, const char *name) 
{
  /* We'll put the pool's used_map at its base.
     Calculate the space needed for the bitmap
     and subtract it from the pool's size. */
  size_t bm_pages = DIV_ROUND_UP (bitmap_buf_size (page_cnt), PGSIZE);
  if (bm_pages > page_cnt)
    PANIC ("Not enough memory in %s for bitmap.", name);
  page_cnt -= bm_pages;

  printf ("%zu pages available in %s.\n", page_cnt, name);

  /* Initialize the pool. */
  lock_init (&p->lock);
  p->used_map = bitmap_create_in_buf (page_cnt, base, bm_pages * PGSIZE);
  p->base = base + bm_pages * PGSIZE;
  p->size = page_cnt;
}

/* Returns true if PAGE was allocated from POOL,
   false otherwise. */
static bool
page_from_pool (const struct pool *pool, void *page) 
{
  size_t page_no = pg_no (page);
  size_t start_page = pg_no (pool->base);
  size_t end_page = start_page + bitmap_size (pool->used_map);

  return page_no >= start_page && page_no < end_page;
}
