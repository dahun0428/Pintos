#include "threads/palloc.h"
#include "threads/pte.h"
#include <hash.h>

struct frame {

  struct list_elem felem; /* hash_elem in frame.c */

//  uintptr_t paddr;           /* physical address */
  void * vaddr;           /* virtual address */

  long long last_accessed_tick; /* need? */
};

void * get_frame_single();


uintptr_t get_frame_paddr(struct frame *);
void *  get_frame_vaddr(struct frame *);
