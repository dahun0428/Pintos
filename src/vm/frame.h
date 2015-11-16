#ifndef VM_FRAME_H
#define VM_FRAME_H
#include "threads/palloc.h"
#include "threads/pte.h"
#include <hash.h>

struct frame {

  struct list_elem felem; /* hash_elem in frame.c */

//  uintptr_t paddr;           /* physical address */
  void * vaddr;           /* virtual address */
  bool have_page;

  long long last_accessed_tick; /* need? */
};

void init_frame_list (void);
void * get_frame_single (void);
void free_frame_single (void *);
void solo_frame_single (void *);

uintptr_t get_frame_paddr (struct frame *);
void *  get_frame_vaddr (struct frame *);

#endif
