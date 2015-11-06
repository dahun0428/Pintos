#include "threads/palloc.h"
#include "threads/pte.h"
#include <hash.h>

struct frame {

  struct hash_elem felem; /* hash_elem in frame.c */

  uintptr_t paddr;           /* physical address */
  void * vaddr;           /* virtual address */

  long long last_accessed_tick; /* need? */
};

void * get_frame_single();


