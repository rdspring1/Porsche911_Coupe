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

#include "userprog/exception.h"
#include <debug.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/init.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "vm/spage.h"
#include "vm/swap.h"

/* Stack Growth */
uint8_t * stack_bound;

/* Swap Table RDS */
struct swap_t *swaptable;

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
	void
exception_init (void) 
{
	/* These exceptions can be raised explicitly by a user program,
	   e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
	   we set DPL==3, meaning that user programs are allowed to
	   invoke them via these instructions. */
	intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
	intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
	intr_register_int (5, 3, INTR_ON, kill,
			"#BR BOUND Range Exceeded Exception");

	/* These exceptions have DPL==0, preventing user processes from
	   invoking them via the INT instruction.  They can still be
	   caused indirectly, e.g. #DE can be caused by dividing by
	   0.  */
	intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
	intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
	intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
	intr_register_int (7, 0, INTR_ON, kill,
			"#NM Device Not Available Exception");
	intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
	intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
	intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
	intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
	intr_register_int (19, 0, INTR_ON, kill,
			"#XF SIMD Floating-Point Exception");

	/* Most exceptions can be handled with interrupts turned on.
	   We need to disable interrupts for page faults because the
	   fault address is stored in CR2 and needs to be preserved. */
	intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
	void
exception_print_stats (void) 
{
	printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
	static void
kill (struct intr_frame *f) 
{
	/* This interrupt is one (probably) caused by a user process.
	   For example, the process might have tried to access unmapped
	   virtual memory (a page fault).  For now, we simply kill the
	   user process.  Later, we'll want to handle page faults in
	   the kernel.  Real Unix-like operating systems pass most
	   exceptions back to the process via signals, but we don't
	   implement them. */

	/* The interrupt frame's code segment value tells us where the
	   exception originated. */
	switch (f->cs)
	{
		case SEL_UCSEG:
			/* User's code segment, so it's a user exception, as we
			   expected.  Kill the user process.  */
			printf ("%s: dying due to interrupt %#04x (%s).\n",
					thread_name (), f->vec_no, intr_name (f->vec_no));
			//intr_dump_frame (f);
			sysexit(-1);

		case SEL_KCSEG:
			/* Kernel's code segment, which indicates a kernel bug.
			   Kernel code shouldn't throw exceptions.  (Page faults
			   may cause kernel exceptions--but they shouldn't arrive
			   here.)  Panic the kernel to make the point.  */
			intr_dump_frame (f);
			PANIC ("Kernel bug - unexpected interrupt in kernel"); 

		default:
			/* Some other code segment?  Shouldn't happen.  Panic the
			   kernel. */
			printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
					f->vec_no, intr_name (f->vec_no), f->cs);
			thread_exit ();
	}
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
	static void
page_fault (struct intr_frame *f) 
{
	bool not_present;  /* True: not-present page, false: writing r/o page. */
	bool write;        /* True: access was write, false: access was read. */
	bool user;         /* True: access by user, false: access by kernel. */
	void *fault_addr;  /* Fault address. */

	/* Obtain faulting address, the virtual address that was
	   accessed to cause the fault.  It may point to code or to
	   data.  It is not necessarily the address of the instruction
	   that caused the fault (that's f->eip).
	   See [IA32-v2a] "MOV--Move to/from Control Registers" and
	   [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
	   (#PF)". */
	asm ("movl %%cr2, %0" : "=r" (fault_addr));

	/* Turn interrupts back on (they were only off so that we could
	   be assured of reading CR2 before it changed). */
	intr_enable ();

	/* Count page faults. */
	page_fault_cnt++;

	/* Determine cause. */
	not_present = (f->error_code & PF_P) == 0;
	write = (f->error_code & PF_W) != 0;
	user = (f->error_code & PF_U) != 0;

	/* To implement virtual memory, delete the rest of the function
	   body, and replace it with code that brings in the page to
	   which fault_addr refers. */
	void* upage = pg_round_down (fault_addr);
	if(!is_user_vaddr(upage))
		kill(f);

	//printf("not_present: %d\n", not_present);
	//printf("write: %d\n", write);
	//printf("user: %d\n", user);
	//printf("Page Fault - UPAGE %p\n", upage);

	struct spage * page = spage_lookup(&thread_current()->spagedir, upage);
	if(page == NULL)
	{
		int stackdiff = ((uint8_t *) f->esp) - ((uint8_t *) fault_addr);
		upage = ((uint8_t *) stack_bound) - PGSIZE;
		//printf("Stack Difference %d\n", stackdiff);

		if(((uint8_t *) fault_addr) < stack_bound && (stackdiff <= 0 || stackdiff == 4 || stackdiff == 32))
		{
			uint8_t *kpage = frame_selector (upage, PAL_USER | PAL_ZERO);
			if (kpage != NULL) 
			{
				if (!install_page(upage, kpage, true))
				{
					palloc_free_page (kpage);
				}
				else
				{
					stack_bound = upage;

					// Add stack page to supplementary page table - rds
					struct spage * p = (struct spage *) malloc(sizeof(struct spage));
					if(p != NULL)
					{
						p->addr = upage;
						p->state = ZERO;
						p->file = NULL;
						p->ofs = 0;
						p->readonly = true;
						p->page_read_bytes = 0;
						p->page_zero_bytes = PGSIZE;
                  		lock_init(&p->spagelock);
						hash_insert (&thread_current()->spagedir, &p->hash_elem);
					}
				}
			} 
			return;
		}
		else
		{
			// Terminate User Process
			//printf("Page Fault - UPAGE %p\n", upage);
			//printf("Stack Difference %d\n", stackdiff);
			//printf("Stack: %p\n", f->esp);
			//printf("FADDR: %p\n", fault_addr);
			//printf("INSTRUCT: %p\n", f->eip);
			//ASSERT(false);
			kill(f);
		}
	}

	// Fetch data into frame and install page
	switch(page->state)
	{
		case DISK:
			{
				// READ PAGE CONTENTS FROM FILE RDS
				file_seek (page->file, page->ofs);
				/* Get a page of memory. - kernel frame */
				uint8_t *kpage = frame_selector (upage, PAL_USER);
				if (kpage != NULL)
				{
					/* Load this page. */
					if (file_read (page->file, kpage, page->page_read_bytes) != (int) page->page_read_bytes)
					{
						printf("Failed to read from file\n");
						palloc_free_page (kpage);
					}

					/* Add the page to the process's address space. */
					if (!install_page (upage, kpage, page->readonly)) 
					{
						printf("Failed to add page to process's address space\n");
						palloc_free_page (kpage);
					}
				}
			}
			break;
		case ZERO:
			{
				uint8_t *kpage = frame_selector (upage, PAL_USER | PAL_ZERO);
				if(!install_page(upage, kpage, true))
					palloc_free_page (kpage);
			}
			break;
		case MIXED:
			{
				file_seek (page->file, page->ofs);
				/* Get a page of memory. - kernel frame */
				uint8_t *kpage = frame_selector (upage, PAL_USER | PAL_ZERO);
				if (kpage != NULL)
				{
					/* Load this page. */
					if (file_read (page->file, kpage, page->page_read_bytes) != (int) page->page_read_bytes)
					{
						printf("Failed to read from file\n");
						palloc_free_page (kpage);
					}

					/* Add the page to the process's address space. */
					if (!install_page (upage, kpage, page->readonly)) 
					{
						printf("Failed to add page to process's address space\n");
						palloc_free_page (kpage);
					}
				}
			}
			break;
		case SWAP:
			{
				uint8_t *kpage = frame_selector (upage, PAL_USER);
				swap_read(page->swapindex, swaptable, (void*) kpage);
				if(!install_page(upage, kpage, true))
					palloc_free_page (kpage);
			}
			break;
		case SHARED:
			{
				ASSERT(page->state == SHARED);
			}
	}
}

