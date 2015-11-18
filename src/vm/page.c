#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "filesys/file.h"
#include <hash.h>
#include <string.h>


void page_before_swap (struct page * p);
void page_after_swap (struct page * p);
void page_unmodified (struct page * p);
  void 
page_destructor (struct hash_elem *element, void *aux UNUSED)
{

  struct page * p = hash_entry (element, struct page, pelem);
  free (p);
}

void page_free (struct page * p)
{

  /*  if (p->frame_addr != NULL)
      free_frame_single (p->frame_addr);*/

  free (p);

}
void * new_stack_page (struct intr_frame *f, void * fault_addr, bool user){

  struct thread * cur = thread_current();
  struct sup_page * sup_p = &cur->p_hash;

  if (sup_p->stack_page_count > MAX_STACK_PAGE) 
    return NULL;

  void * new_frame = get_frame_single();

  memset (new_frame, 0, PGSIZE);
  if ( !(pagedir_get_page (cur->pagedir, fault_addr) == NULL &&
        pagedir_set_page (cur->pagedir, pg_round_down (fault_addr), new_frame, true)) ) 
  {
    free_frame_single (new_frame);
    return NULL;
  }

  struct page * new_page = (struct page *) malloc (sizeof (struct page)); 
  new_page->t = cur;

  new_page->frame_addr = new_frame;
  new_page->addr = pg_round_down (fault_addr);
  new_page->valid = true;
  new_page->accessed = true;
  new_page->dirty = false;
  new_page->loaded = true;
  new_page->swapped = false;
  new_page->swap_idx = -1;
  new_page->stack = true;
  new_page->file = false;
  new_page->rntly_swap = false;
  new_page->ref = true;
  new_page->modified = false;
  new_page->writable = true;

  new_page->cur_file = NULL;

  new_page->mapid = -1;

  set_frame_page (new_page);
  lock_init (&new_page->page_lock);

  hash_insert (&sup_p->page_table, &new_page->pelem);
  sup_p->stack_page_count++;

  page_visited (new_page);

  return new_frame;
}

void sup_page_init (struct sup_page * p_hash){

  hash_init (&p_hash->page_table, page_hash, page_less, NULL);
  p_hash->stack_page_count = 0;
}

void page_table_clean (struct sup_page * sp){

  struct hash_iterator i;
  struct thread * t = thread_current ();

  hash_first(&i, &sp->page_table);
  while (hash_next (&i))
  {
    struct page * p = hash_entry (hash_cur (&i), struct page, pelem);
    if (p->swap_idx > -1)
      swap_clean_bit (p->swap_idx);
    if (p->cur_file !=NULL)
      file_close (p->cur_file);
    pagedir_clear_page (thread_current ()->pagedir, p->addr);
    free_frame_single (p->frame_addr);

  }

  hash_destroy (&sp->page_table, page_destructor);
}

struct page * lazy (struct file * file, off_t ofs, uint8_t * upage, size_t page_read_byte, bool writable)
{

  struct thread * cur = thread_current();
  struct sup_page * sup_p = &cur->p_hash;

  if (sup_p->stack_page_count > MAX_STACK_PAGE) 
    return NULL;

  struct page * new_page = (struct page *) malloc (sizeof (struct page));
  struct file * saved_file = file_reopen (file);

  if (pg_ofs (upage) != 0) return NULL;
  ASSERT (ofs % PGSIZE == 0);

  new_page->t = cur;

  new_page->frame_addr = NULL;
  new_page->addr = upage;
  new_page->valid = false;
  new_page->accessed = false;
  new_page->dirty = false;
  new_page->loaded = false;
  new_page->file = false;
  new_page->swapped = false;
  new_page->rntly_swap = false;
  new_page->swap_idx = -1;
  new_page->stack = false; /* I dont know */
  new_page->ref = false;
  new_page->modified = false;
  new_page->writable = writable;

  new_page->cur_file = saved_file;
  new_page->file_ofs = ofs;
  new_page->read_bytes = page_read_byte;

  new_page->mapid = -1;

  lock_init (&new_page->page_lock);
  hash_insert (&sup_p->page_table, &new_page->pelem);

  return new_page;
}

bool load_swap (struct page * page)
{
  struct thread * cur = thread_current();
  struct sup_page * sup_p = &cur->p_hash;

  bool writable = page->writable;

  //  printf("in load_swap %p\n",page->addr);
  page->frame_addr = get_frame_single();
  swap_unblock (page);

  if ( !(pagedir_set_page (cur->pagedir, page->addr, page->frame_addr, writable)) ) 
  {
    free_frame_single (page->frame_addr);
    return false;
  }
  page_after_swap (page);
  page_visited (page);
  set_frame_page (page);
  // puts("load_swap end");

  return true;
}

bool load_lazy (struct page * lazy_page){

  struct thread * cur = thread_current();
  struct sup_page * sup_p = &cur->p_hash;

  ASSERT ( lazy_page->loaded == false);

  //(if page in backing store);

  if (lazy_page->swapped)
    return load_swap (lazy_page);

  void * kpage = get_frame_single ();
  void * upage = lazy_page->addr;
  struct file * file = lazy_page->cur_file;//file_reopen (lazy_page->cur_file);
  off_t ofs = lazy_page->file_ofs;
  bool writable = lazy_page->writable;
  size_t read_bytes = lazy_page->read_bytes;

  //if (pg_ofs (upage) != 0) return false;
  file_seek(file, ofs);

  //  printf("file = %p\nvaddr = %p\n",file,upage);

  /* Load this page. */
  if (file_read (file, kpage, read_bytes) != (int) read_bytes)
  {
    free_frame_single (kpage);
    return false; 
  }

  memset (kpage + read_bytes, 0, PGSIZE - read_bytes);


  if ( !(//pagedir_get_page (cur->pagedir, kpage) == NULL && 
        pagedir_set_page (cur->pagedir, upage, kpage, writable)) ) 
  {
    free_frame_single (kpage);
    return false;
  }

  lazy_page->loaded = true;
  lazy_page->frame_addr = kpage;
  lazy_page->valid = true;
  set_frame_page (lazy_page);
  page_visited (lazy_page);
  //  page_after_swap (lazy_page);

  return true;
}

struct page * vaddr2page (struct sup_page * sp, void * addr){

  struct hash_iterator i;
  void *uaddr = pg_round_down (addr);

  hash_first(&i, &sp->page_table);
  while (hash_next (&i))
  {
    struct page * p = hash_entry (hash_cur (&i), struct page, pelem);

    if (p->addr == uaddr)
      return p;
  }

  return NULL;
}

struct page * mapid2page (struct sup_page * sp, mapid_t mapping){

  struct hash_iterator i;

  hash_first(&i, &sp->page_table);
  while (hash_next (&i))
  {
    struct page * p = hash_entry (hash_cur (&i), struct page, pelem);

    if (p->mapid == mapping && p->loaded)
      return p;

  }

  return NULL;
}

struct page * page_first (struct sup_page * sp, bool user)
{
  struct hash_iterator i;


  hash_first(&i, &sp->page_table);
  if (!user)
  {
    hash_next (&i);
    return hash_entry (hash_cur (&i), struct page, pelem);
  }

  else
  {
    while (hash_next (&i))
    {
      struct page * p = hash_entry (hash_cur (&i), struct page, pelem);

      if (is_user_vaddr (p->addr))
        return p;

    }
  }

}

void page_visited (struct page * p){

  p->accessed = true;
  pagedir_set_accessed (p->t->pagedir, p->addr, true);
  p->ref = true;
}

void page_modified (struct page * p){

  p->dirty = true;
  pagedir_set_dirty (p->t->pagedir, p->addr, true);
  p->modified = true;
  page_visited (p);
}

void page_unmodified (struct page * p){

  p->dirty = false;
  pagedir_set_dirty (p->t->pagedir, p->addr, false);
  p->modified = false;
}

void page_unmap (mapid_t mapping){

  struct hash_iterator i;
  struct sup_page * sp = &thread_current ()->p_hash;
  struct page * found;
  //  bool not_finish = true;

  while (true)
  {
    hash_first(&i, &sp->page_table);
    while (hash_next (&i))
    {
      struct page * p = hash_entry (hash_cur (&i), struct page, pelem);

      if (p->mapid == mapping)
      {
        found = p;
        break;
      }
    }

    if (found == NULL)
    {
      break;
    }

    else if (pagedir_is_dirty (thread_current ()->pagedir, found->addr))
    {
      //      if (file_tell (found->cur_file) != (int) (found->file_ofs))
      //      {
      file_seek (found->cur_file, found->file_ofs);
      file_write (found->cur_file, found->addr, found->read_bytes);
      //    }
      file_close (found->cur_file);
      pagedir_clear_page (thread_current ()->pagedir, found->addr);
      free_frame_single (found->frame_addr); 
      hash_delete (&sp->page_table, &found->pelem);
      page_free (found);
    }
    else
    {
      file_close (found->cur_file);
      pagedir_clear_page (thread_current ()->pagedir, found->addr);
      free_frame_single (found->frame_addr); 
      hash_delete (&sp->page_table, &found->pelem);
      page_free (found);
    }
    found = NULL;
  }
  return;
}

void page_backup (struct page * p){

  struct hash_iterator i;
  struct sup_page * sp = &thread_current ()->p_hash;
  struct page * found;
  mapid_t mapping = p->mapid;
  //  bool not_finish = true;

  if (!pagedir_is_dirty (thread_current ()->pagedir, p->addr))
    return;

  if (mapping == -1)
  {
    file_seek (p->cur_file, p->file_ofs);
    file_write (p->cur_file, p->addr, p->read_bytes);


    page_unmodified (p);

  }

  else

    while (true)
    {
      hash_first(&i, &sp->page_table);
      while (hash_next (&i))
      {
        struct page * p = hash_entry (hash_cur (&i), struct page, pelem);

        if (p->mapid == mapping)
        {
          found = p;
          break;
        }
      }

      if (found == NULL)
      {
        break;
      }

      else
      {
        if (pagedir_is_dirty (thread_current ()->pagedir, found->addr))
        {
          file_seek (found->cur_file, found->file_ofs);
          file_write (found->cur_file, found->addr, found->read_bytes);
        }

        page_unmodified (found);
        //      pagedir_clear_page (thread_current ()->pagedir, found->addr);
      }
      found = NULL;
    }
  return;
}

void page_before_swap (struct page * p)
{
  p->valid = false;
  p->loaded = false;
  p->swapped = true;
  p->frame_addr = NULL;
  p->rntly_swap = false;
}

void page_after_swap (struct page * p)
{
  p->valid = true;
  p->loaded = true;
  p->swapped = false;
  p->rntly_swap = true;
}
/* Returns a hash value for page p. */
  unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct page *p = hash_entry (p_, struct page, pelem);
  return hash_bytes (&p->addr, sizeof p->addr);
}

/* Returns true if page a precedes page b. */
  bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
    void *aux UNUSED)
{
  const struct page *a = hash_entry (a_, struct page, pelem);
  const struct page *b = hash_entry (b_, struct page, pelem);

  return a->addr < b->addr;
}


