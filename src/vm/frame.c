#include <list.h>
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

static struct list frame_list; 
static struct lock frame_lock;

void init_frame_list(void)
{
  list_init (&frame_list);
  lock_init (&frame_lock);
}

void *
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
    pagedir_clear_page (thread_current ()->pagedir, victim->addr);
    free_frame_single (victim->frame_addr);
    page_before_swap (victim);
    mem_new = palloc_get_page (PAL_USER);
//    mem_new = victim->frame_addr;

    ASSERT (mem_new != NULL);

    
/*    if(!single)
      free(single);*/
  }

  single->vaddr = mem_new; // & PAGE_ADDR);
  single->have_page = true;
  list_push_back (&frame_list, &single->felem);

  lock_release (&frame_lock);
//  puts("frame ok?");

  return mem_new;

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
      break;
    }
  }

}

void
solo_frame_single (void * vaddr){
  
  struct list_elem * e;

  for (e = list_begin (&frame_list); e != list_end (&frame_list); e = list_next (e))
  {
    struct frame * single = list_entry (e, struct frame, felem);

    if ( single->vaddr == pg_round_down (vaddr) )
    {
      single->have_page = false;
      break;
    }
  }

}

void
free_frame_all (){

  struct list_elem * e;

  for (e = list_begin (&frame_list); e != list_end (&frame_list); e = list_next (e))
  {
    struct frame * single = list_entry (e, struct frame, felem);

      palloc_free_page (single->vaddr);
      list_remove(e);
      free(single);
  }
}

uintptr_t
get_frame_paddr(struct frame * f){
  
  return vtop(f->vaddr);
}

void *
get_frame_vaddr(struct frame * f){

  return f->vaddr;
}




