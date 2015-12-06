#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H
#include "filesys/filesys.h"
#include "devices/block.h"


#define BUFFER_NOT_FOUND 65
#define BUFFER_CACHE_MAX 64

typedef uint32_t buffer_cache_idx;

/* buffer_cache unit structure */
struct buffer_cache
  {
    char data[BLOCK_SECTOR_SIZE];
    block_sector_t sector_idx;
    bool accessed;
    bool dirty;
    bool valid;
  };

void cache_init (void);

struct buffer_cache * buffer_get_cache (block_sector_t);
buffer_cache_idx buffer_evict_cache (void);
struct buffer_cache * buffer_find_cache (block_sector_t);

void buffer_write_single (buffer_cache_idx);
void buffer_write_behind (void);

void set_buffer_accessed (struct buffer_cache *, bool);
void set_buffer_dirty (struct buffer_cache *, bool);

#endif /* filesys/cache.h */


