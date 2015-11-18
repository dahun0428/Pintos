#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "devices/block.h"
#include <hash.h>

#define MAX_STACK_PAGE 2000


struct page
{

  struct hash_elem pelem; /* hash_elem in page.c */

  void * addr; /* address of frame ; frame number (?) ; maybe vaddr */
  void * frame_addr;

  struct thread * t;

  bool valid;
  bool accessed; /* useless */
  bool dirty; /* useless */
  bool loaded;
  bool swapped;
  bool rntly_swap;
  bool file;
  block_sector_t swap_idx;
  bool stack;
  mapid_t mapid;
  bool ref; /* useless */
  bool modified; /* useless */
  bool writable;

  struct file * cur_file;
  off_t file_ofs;
  size_t read_bytes;

  struct lock page_lock;
};

struct sup_page{

  struct hash page_table;
  unsigned stack_page_count;

};

unsigned page_hash (const struct hash_elem *, void *);
bool page_less (const struct hash_elem *, const struct hash_elem *, void *);

struct page * lazy (struct file *, off_t, uint8_t *, size_t, bool);
bool load_swap (struct page *);
bool load_lazy (struct page *);

void sup_page_init (struct sup_page *);
void page_table_clean (struct sup_page *);

void * new_stack_page (struct intr_frame *, void *, bool);
struct page * vaddr2page (struct sup_page *, void *);
struct page * mapid2page (struct sup_page *, mapid_t);
struct page * page_first (struct sup_page *, bool);

void page_unmap (mapid_t);
void page_backup (struct page *);

void page_visited (struct page *);
void page_modified (struct page *);

#endif
