#include "frame.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

static struct list frame_list; 


void init_frame_list(void){
  list_init(&frame_list);
}

void *
get_frame_single (){

  void * new_page = palloc_get_page (PAL_USER | PAL_ZERO); 
  struct frame * single = (struct frame *) malloc (sizeof(struct frame));

  if (new_page == NULL){    /* swap */
    if(!single)
      free(single);
  }

  else{
    single->vaddr = new_page;
    list_push_back (&frame_list, &single->felem);
  }

  return new_page;

}

uintptr_t
get_frame_paddr(struct frame * f){
  
  return vtop(f->vaddr);
}

void *
get_frame_vaddr(struct frame * f){

  return f->vaddr;
}




