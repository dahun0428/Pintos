#include <list.h>
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

#define FRAME_MAX 128000

static struct list frame_list; 
static uint32_t frame_cnt = 0;
static struct lock frame_lock;

void init_frame_list(void)
{
  list_init (&frame_list);
  lock_init (&frame_lock);
}

bool is_frame_full(void)
{
  return !(FRAME_MAX > frame_cnt);
}

struct frame *
get_frame_single (){

  lock_acquire (&frame_lock);
  void * mem_new = palloc_get_page (PAL_USER); 
  struct frame * single = (struct frame *) malloc (sizeof (struct frame));
  struct list_elem * e;

  if (mem_new == NULL){    /* swap */
    struct page * victim = select_victim ();

    //   printf("victim addr = %p\n",victim->addr);
    ASSERT (victim != NULL);
    //  go to swapblock (victim) function
    swap_block (victim);
    pagedir_clear_page (victim->t->pagedir, victim->addr);
    free_frame_single (victim->frame_addr);
    page_before_swap (victim);
    mem_new = palloc_get_page (PAL_USER);
    //    mem_new = victim->frame_addr;

    ASSERT (mem_new != NULL);


    /*    if(!single)
          free(single);*/
  }

  single->vaddr = mem_new; // & PAGE_ADDR);
  single->apage = NULL;
  list_push_back (&frame_list, &single->felem);
  frame_cnt++;

  lock_release (&frame_lock);
  //  puts("frame ok?");

  return single;

}

void
free_frame_single (void * vaddr){

  struct list_elem * e;

  for (e = list_begin (&frame_list); e != list_end (&frame_list); e = list_next (e))
  {
    struct frame * single = list_entry (e, struct frame, felem);


    if ( single->vaddr == pg_round_down (vaddr) )
    {
      palloc_free_page (single->vaddr);
      list_remove(e);
      free(single);
      frame_cnt--;
      break;
    }
  }

}


/*struct page * select_victim ()
  {
//  struct hash_iterator i;
struct list_elem * e;
struct page * victim = NULL;

while (victim == NULL){

puts("???");
e = list_begin (&frame_list);
while (true)
{
struct frame * single = list_entry (e, struct frame, felem);
struct page * p = single->apage;
bool accessed =  pagedir_is_accessed (p->t->pagedir, p->addr);
bool dirty = pagedir_is_dirty (p->t->pagedir, p->addr);

ASSERT (single->apage->valid);

if (e == list_end (&frame_list))
{
puts("WHYNO");
break;
}

if(!p->valid)
{
puts("invalid");
continue;
}

if (!accessed && !dirty && !p->rntly_swap)
{
victim = p;
break;
}

else if (accessed && !p->rntly_swap)
{
pagedir_set_accessed (p->t->pagedir, p->addr, false);
}

else if (!accessed && dirty && !p->rntly_swap)
{
if (p->file)
page_backup (p);
pagedir_set_dirty (p->t->pagedir, p->addr, false);
}

else if (p->rntly_swap)
p->rntly_swap = false;
else
ASSERT (false);

list_next (e);
}
puts("???");
}

return victim;
}*/


