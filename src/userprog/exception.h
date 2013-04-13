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

#ifndef USERPROG_EXCEPTION_H
#define USERPROG_EXCEPTION_H

/* Page fault error code bits that describe the cause of the exception.  */
#define PF_P 0x1    /* 0: not-present page. 1: access rights violation. */
#define PF_W 0x2    /* 0: read, 1: write. */
#define PF_U 0x4    /* 0: kernel, 1: user process. */

void exception_init (void);
void exception_print_stats (void);

#endif /* userprog/exception.h */
