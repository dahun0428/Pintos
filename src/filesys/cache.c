#include <debug.h>
#include <string.h>
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
    cache_list[i]->accessed = false;
    cache_list[i]->dirty = false;
    cache_list[i]->valid = false;
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
    if (cache_list[i]->sector_idx == idx && cache_list[i]->valid)
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
      set_buffer_accessed (cache_list[i], false);
      cache_list[i]->valid = false;
      memset (cache_list[i]->data, 0, BLOCK_SECTOR_SIZE);
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

struct buffer_cache * cache_read_at (block_sector_t sector_idx, void *buffer, off_t sector_ofs, int chunk_size)
{
  struct buffer_cache * cache = NULL;


  cache = buffer_find_cache (sector_idx);
  if (cache == NULL)
  {
    cache = buffer_get_cache (sector_idx);
    block_read (fs_device, sector_idx, cache->data);
    set_buffer_accessed (cache, true);
  }

  memcpy (buffer, cache->data + sector_ofs, chunk_size);

  return cache;
}


struct buffer_cache * cache_write_at (block_sector_t sector_idx, void *buffer, off_t sector_ofs, int chunk_size)
{
  struct buffer_cache * cache = NULL;
  int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;

  cache = buffer_find_cache (sector_idx);
  if (cache == NULL)
  {
    cache = buffer_get_cache (sector_idx);
    //if (sector_ofs > 0 || chunk_size < sector_left) 
      block_read (fs_device, sector_idx, cache->data);
    //else
    //  memset (cache->data, 0, BLOCK_SECTOR_SIZE);
  }

  /* If the sector contains data before or after the chunk
     we're writing, then we need to read in the sector
     first.  Otherwise we start with a sector of all zeros. */
  memcpy (cache->data + sector_ofs, buffer, chunk_size);
  //block_write (fs_device, sector_idx, cache->data);
  set_buffer_accessed (cache, true);
  set_buffer_dirty (cache, true);
}
