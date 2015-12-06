#include <debug.h>
#include "threads/malloc.h"
#include "devices/block.h"
#include "filesys/cache.h"
#include "filesys/filesys.h"


//struct block *fs_device;
static struct buffer_cache *cache_list[BUFFER_CACHE_MAX];

/* get buffer cache idx */
struct buffer_cache * buffer_get_cache (block_sector_t sector_idx)
{
  buffer_cache_idx cache_idx = BUFFER_NOT_FOUND;


  cache_idx = buffer_evict_cache();

  cache_list[cache_idx]->sector_idx = sector_idx;
  cache_list[cache_idx]->accessed = false;
  cache_list[cache_idx]->dirty = false;
  cache_list[cache_idx]->valid = true;

  ASSERT (cache_idx != BUFFER_NOT_FOUND);

  return (cache_list[cache_idx]);
}

void cache_init (void)
{
  int i = 0;
//  fs_device = block_get_role (BLOCK_FILESYS);
  for (i = 0; i < BUFFER_CACHE_MAX ; i++)
  {
    cache_list[i] = (struct buffer_cache *) malloc (sizeof (struct buffer_cache));
    cache_list[i]->sector_idx = BUFFER_NOT_FOUND;
  //  cache_list[i]->data = calloc (BLOCK_SECTOR_SIZE, 1);

    ASSERT (cache_list[i] != NULL);
    ASSERT (cache_list[i]->data != NULL);
  }
}

/* return idx
priority: invalid idx then 2nd chance alogorithm */

buffer_cache_idx buffer_evict_cache (void)
{
  int i;
  buffer_cache_idx idx = BUFFER_NOT_FOUND;

  while (idx == BUFFER_NOT_FOUND)
  {
    for (i = 0; i < BUFFER_CACHE_MAX ; i++)
    {
      bool accessed = cache_list[i]->accessed;
      bool dirty = cache_list[i]->dirty;
      if (!cache_list[i]->valid)
      {
        idx = i;
        break;
      }

      else if (!accessed && !dirty)
      {
        idx = i;
        break;
      }

      else if (accessed)
      {
        set_buffer_accessed (cache_list[i], false);
      }

      else if (dirty)
      {
        buffer_write_single (i);
        set_buffer_dirty (cache_list[i], false);
      }

    }
  }

  return idx;
}

struct buffer_cache * buffer_find_cache (block_sector_t idx)
{
  int i;

  for (i = 0; i < BUFFER_CACHE_MAX ; i++)
    if (cache_list[i]->sector_idx == idx)
      return (cache_list[i]);

  return NULL;
}

void buffer_write_single (buffer_cache_idx idx)
{

  block_write (fs_device, cache_list[idx]->sector_idx, cache_list[idx]->data); 
  set_buffer_dirty (cache_list[idx], false);

}

void buffer_write_behind (void)
{
  size_t i;

  for (i = 0; i < BUFFER_CACHE_MAX; i++)
    if (cache_list[i]->valid && cache_list[i]->dirty)
    {
      block_write (fs_device, cache_list[i]->sector_idx, cache_list[i]->data);
      set_buffer_dirty (cache_list[i], false);
    }
}

void set_buffer_accessed (struct buffer_cache * cache, bool accessed)
{
  cache->accessed = accessed;
}

void set_buffer_dirty (struct buffer_cache * cache, bool dirty)
{
  cache->dirty = dirty;
}

