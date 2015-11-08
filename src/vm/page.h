#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include <hash.h>



struct page{

 struct hash_elem pelem; /* hash_elem in page.c */

 void * addr; /* address of frame ; frame number (?) */
 void * frame;

 struct thread * t;

 bool valid;
 bool accessed;
 bool dirty;
 bool swapped;
 bool stack;
 bool file;
 bool ref;
 bool modified;
 
 struct lock page_lock;
};

struct sup_page{
  
  struct hash page_list;
  unsigned stack_page_count;



};

unsigned page_hash (const struct hash_elem *, void *);
bool page_less (const struct hash_elem *, const struct hash_elem *, void *);


