#include <hash.h>
#include <bitmap.h>
#include "devices/block.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

//static block_sector_t sector;
static struct bitmap * swap_bitmap;

#define MAX_BLOCK_PAGE 500000

void swap_init(){
//  sector = 0;
  swap_bitmap = bitmap_create (MAX_BLOCK_PAGE);
}

struct page * select_victim ()
{
  struct thread * t =  thread_current ();
  struct sup_page * sp = &t->p_hash;
  struct hash_iterator i;
  struct page * victim = NULL;

  while (victim == NULL){

    hash_first(&i, &sp->page_table);
    while (hash_next (&i))
    {
      struct page * p = hash_entry (hash_cur (&i), struct page, pelem);
      bool accessed =  pagedir_is_accessed (t->pagedir, p->addr);
      bool dirty = pagedir_is_dirty (t->pagedir, p->addr);

      if(!p->valid)
      {
//        puts("invalid");
        continue;
      }

      if (!accessed && !dirty)
        victim = p;

      else if (accessed)
      {
        pagedir_set_accessed (t->pagedir, p->addr, false);
      }

      else if (!accessed && dirty)
      {
        if (p->file)
          page_backup (p);
        pagedir_set_dirty (t->pagedir, p->addr, false);
      }

      else
        ASSERT (false);
    }
  }

  return victim;
}

void
swap_block (struct page * p)
{
  struct block * block = block_get_role (BLOCK_SWAP);
  int i;

  ASSERT (!p->swapped);

  for (i = 0 ; i < MAX_BLOCK_PAGE; i++)
  {
    if (!bitmap_test (swap_bitmap, i))
    {
      p->swap_idx = i;
      uint32_t sec_no = i * 8;
      uint32_t offset = 0;
      void * buffer = p->addr;

      for (offset = 0; offset < 8 ; offset++)
      {
        block_write (block, sec_no + offset, (void *)(buffer + BLOCK_SECTOR_SIZE * offset)); 
      }
      
      bitmap_mark (swap_bitmap, i);
      break;

    }
  }
}


void
swap_unblock (struct page * p)
{
  struct block * block = block_get_role (BLOCK_SWAP);

  ASSERT (p->swapped);
  block_sector_t idx = p->swap_idx;
  uint32_t sec_no = idx * 8;
  uint32_t offset = 0;
  void * buffer = p->frame_addr;

  for (offset = 0; offset < 8 ; offset++)
  {
    block_read (block, sec_no + offset, (void *)(buffer + BLOCK_SECTOR_SIZE * offset)); 
  }

  bitmap_reset (swap_bitmap, idx);

}
  

void
swap_clean_bit (uint32_t idx)
{
  bitmap_reset (swap_bitmap, idx);
}


