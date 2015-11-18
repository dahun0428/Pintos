#ifndef VM_FRAME_H
#define VM_FRAME_H
#include "threads/palloc.h"
#include "threads/pte.h"
#include <hash.h>

struct frame {

  struct list_elem felem; /* hash_elem in frame.c */

//  uintptr_t paddr;           /* physical address */
  void * vaddr;           /* virtual address */
  struct page * apage;

  long long last_accessed_tick; /* need? */
};

void init_frame_list (void);
struct frame * get_frame_single (void);
void free_frame_single (void *);
struct frame * get_frame_struct (void *);


bool is_frame_full(void);
//struct page * select_victim();

#endif
