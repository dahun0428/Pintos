#ifndef VM_SWAP_H
#define VM_SWAP_H
#include <bitmap.h>
#include "devices/block.h"

#define SWAP_PAGE_MAX 250000 /* maximum user memory * 2 / page */



struct page * select_victim (void);
void swap_init (void);

void swap_block (struct page *);
void swap_unblock (struct page *);

void swap_clean_bit (uint32_t);
#endif
