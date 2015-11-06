#include "frame.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"

static struct hash frame_hash; 



void *
get_frame_single (){

  void * new_page = palloc_get_page (PAL_USER); 
  struct frame * single = (struct frame *) malloc (struct frame);

  if (new_page == NULL){
    if(!single)
      free(single);
  }

  else{
    single->vaddr = new_page;
    single->paddr = vtop(new_page); 

    hash_insert (&frame_hash, single->elem);
  }

  return new_page;

}

uintptr_t
get_frame_paddr(struct frame * f){
  
  return f->paddr;
}

void *
get_frame_vaddr(struct frame * f){

  return f->vaddr;
}




