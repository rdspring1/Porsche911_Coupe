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


#ifndef VM_SPAGE_H
#define VM_SPAGE_H

#include <debug.h>
#include <inttypes.h>
#include "lib/kernel/hash.h"
#include "filesys/off_t.h"
#include "threads/synch.h"

enum page_status
{
    SHARED,   /* Page currently stored in memory */
    SWAP,     /* Page swapped out of memory */
    DISK,     /* Page stored on the disk */
    ZERO,     /* Page completely set to zero */
	MIXED    /* Page is partially stored on the disk */
};

struct spage
{
    struct hash_elem hash_elem; /* Hash table element. */
    const void *addr;           /* User Virtual address. */
    enum page_status state;     /* Location of Page */
    bool readonly;              /* Read Only Setting */
    int swapindex;         		/* Index in the Swap Table */
	struct file * file;         /* File on the Disk */
	off_t ofs;					/* File Offset */
	size_t page_read_bytes;     /* Number of bytes read from the file */
	size_t page_zero_bytes;     /* Number of zero bytes */
    struct lock spagelock;      /* spage lock */
};

unsigned spage_hash_hash_func (const struct hash_elem *e, void *aux UNUSED);
bool spage_hash_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);
struct spage * spage_lookup (struct hash * pages, const void *address);
struct spage * spage_delete (struct hash * pages, const void *address);

#endif
