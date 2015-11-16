#include <list.h>
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "vm/frame.h"
#include "vm/page.h"

static struct list frame_list; 

#define PAGE_ADDR 0xfffff000 /* Address bits*/

void init_frame_list(void)
{
  list_init (&frame_list);
}

void *
get_frame_single (){

  void * mem_new = palloc_get_page (PAL_USER); 
  struct frame * single = (struct frame *) malloc (sizeof (struct frame));

  if (mem_new == NULL){    /* swap */
    if(!single)
      free(single);
  }

  else{
    single->vaddr = mem_new; // & PAGE_ADDR);
    single->have_page = true;
    list_push_back (&frame_list, &single->felem);
  }

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




