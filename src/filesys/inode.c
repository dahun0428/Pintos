#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "filesys/cache.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define SECTOR_ARRAY_MAX 128


/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t dbl_ary;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    bool is_dir;                        /* true if dir */
    uint32_t unused[124];               /* Not used. */
  };

/* following two structures are the same, just for convinience */
typedef block_sector_t indir_array[SECTOR_ARRAY_MAX]; 


typedef indir_array dbl_indir_array;
  

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    block_sector_t *dbl;
    bool is_dir;                        /* true if dir */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length)
    return pos / BLOCK_SECTOR_SIZE;
  else
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);


  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
  {   
    size_t sectors = bytes_to_sectors (length);
    disk_inode->length = length;
    disk_inode->magic = INODE_MAGIC;
    disk_inode->is_dir = false;
    size_t iter = 0;
    block_sector_t *dbl = calloc (1, sizeof (dbl_indir_array));
//    size_t check_cnt = sectors + 1 + DIV_ROUND_UP (sectors, 128);
    //size_t check_cnt = sectors;
    int se_cnt = 0;

    if (dbl == NULL)
    {
      free (disk_inode);
      return success;
    }

    for (iter = 0; iter < SECTOR_ARRAY_MAX; iter++)
      dbl[iter] = -1;
/*    if (!free_map_check (check_cnt))
    {
      free (disk_inode);
      return success;
    }*/

    if (free_map_allocate (1, &disk_inode->dbl_ary)) 
    {   
      size_t surplus_cnt = sectors % SECTOR_ARRAY_MAX; //#define SECTOR_ARRAY_MAX BLOCK_SECTOR_MAX
      size_t indir_cnt = DIV_ROUND_UP (sectors, SECTOR_ARRAY_MAX);

      if (surplus_cnt == 0)
        surplus_cnt = SECTOR_ARRAY_MAX;

      for (iter = 0; iter < indir_cnt ; iter++)
      {
        static char zeros[BLOCK_SECTOR_SIZE];
        size_t i;
        block_sector_t *indir = calloc (1, sizeof (indir_array));
        block_sector_t indir_t;

        for (i = 0; i < SECTOR_ARRAY_MAX; i++)
          indir[i] = -1;

        if (indir == NULL)
          goto free_memory;
        
        free_map_allocate (1, &indir_t);
        size_t chunk_size = (indir_cnt - iter) > 1 ? SECTOR_ARRAY_MAX : surplus_cnt;
        //printf("(create) chuck_size: %d\n",chunk_size);


        for (i = 0; i < chunk_size; i++) 
        {
          free_map_allocate (1, &indir[i]);
          block_write (fs_device, indir[i], zeros);
          se_cnt++;
        }

        block_write (fs_device, indir_t, indir);
        free (indir);
        //printf("(create) indir_sector: %d\n",indir_t);
        dbl[iter] = indir_t;
      }   
      block_write (fs_device, disk_inode->dbl_ary, dbl);
      block_write (fs_device, sector, disk_inode);
      //printf("(create) inode_sector: %d\n",sector);
      //printf("(create) dbl_sector: %d\n",disk_inode->dbl_ary);
      success = true; 
    }

free_memory:
    free (disk_inode);
    free (dbl);
  }   
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  inode->is_dir = inode->data.is_dir;
  
  inode->dbl = calloc (1, sizeof (dbl_indir_array));
  if (inode->dbl == NULL)
  {
    free (inode);
    return NULL;
  }
  block_read (fs_device, inode->data.dbl_ary, inode->dbl);
  //printf("(open) inode_sector: %d\n",inode->sector);
  //printf("(open) dbl_sector: %d\n",inode->data.dbl_ary);

  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
  {
    /* Remove from inode list and release lock. */
    list_remove (&inode->elem);

    /* Deallocate blocks if removed. */
    if (inode->removed) 
    {
      size_t sectors = bytes_to_sectors(inode->data.length);
      size_t indir_cnt = DIV_ROUND_UP (sectors, SECTOR_ARRAY_MAX), i;
      indir_array indir = {0};

      for (i = 0; i < indir_cnt; i ++)
      {
        size_t j;
        block_read (fs_device, inode->dbl[i], &indir);
        for (j = 0; j < SECTOR_ARRAY_MAX && sectors > 0; j++)
        {
          free_map_release (indir[j], 1);
          sectors--;
        }
        free_map_release (inode->dbl[i], 1);
      }
      free_map_release (inode->data.dbl_ary, 1); 
      free_map_release (inode->sector, 1);
    }

    else
    {
      block_write (fs_device, inode->data.dbl_ary, inode->dbl);
      block_write (fs_device, inode->sector, &inode->data);
    }
    free (inode->dbl);
    free (inode); 
  }
  buffer_write_behind ();
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
  void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  //printf("(read) size: %d, offset: %d\n",size, offset);
  while (size > 0) 
  {
    /* Disk sector to read, starting byte offset within sector. */
    block_sector_t offset_idx = offset / BLOCK_SECTOR_SIZE;
    block_sector_t dbl_idx = offset_idx / SECTOR_ARRAY_MAX;
    block_sector_t indir_idx = offset_idx % SECTOR_ARRAY_MAX;

    indir_array indir = {0};

    //printf("(read) dbl_idx: %d, indir_idx%d\n",dbl_idx, inode->dbl[dbl_idx]); 
    //printf("(read) dbl_sector: %d\n",inode->data.dbl_ary);

    //printf("(read) indir_idx:  %d\n",indir_idx);
    //printf("(read) sector_idx: %d\n",sector_idx);

    int sector_ofs = offset % BLOCK_SECTOR_SIZE;

    /* Bytes left in inode, bytes left in sector, lesser of the two. */
    off_t inode_left = inode_length (inode) - offset;
    int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
    int min_left = inode_left < sector_left ? inode_left : sector_left;

    /* Number of bytes to actually copy out of this sector. */
    int chunk_size = size < min_left ? size : min_left;
    if (chunk_size <= 0)
      break;

    /* allocate sector lazily */

    if (inode->dbl[dbl_idx] == -1)
    {
      size_t i;
      ASSERT (free_map_allocate (1, &inode->dbl[dbl_idx]));

      for (i = 0; i < SECTOR_ARRAY_MAX; i++)
        indir[i] = -1;
      block_write (fs_device, inode->dbl[dbl_idx], &indir);
      block_write (fs_device, inode->data.dbl_ary, inode->dbl);
    }

    else
    {
   //   cache_read_at (inode->dbl[dbl_idx], &indir, 0, BLOCK_SECTOR_SIZE);
      block_read (fs_device, inode->dbl[dbl_idx], &indir);
    }

    block_sector_t sector_idx = indir[indir_idx];
    if (inode_left > 0 && sector_idx == -1)
    {
      static char zeros[BLOCK_SECTOR_SIZE];

      ASSERT (free_map_allocate (1, &sector_idx));
      block_write (fs_device, sector_idx, zeros);
      indir[indir_idx] = sector_idx;
      block_write (fs_device, inode->dbl[dbl_idx], &indir);
    }


    cache_read_at (sector_idx, buffer + bytes_read, sector_ofs, chunk_size);

    /* Advance. */
    size -= chunk_size;
    offset += chunk_size;
    bytes_read += chunk_size;
  }
  //printf("(read) bytes_read: %d\n",bytes_read); 
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
    off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0) 
  {
    /* Sector to write, starting byte offset within sector. */
    block_sector_t offset_idx = offset / BLOCK_SECTOR_SIZE; //byte_to_sector (inode, offset);
    block_sector_t dbl_idx = offset_idx / SECTOR_ARRAY_MAX;
    block_sector_t indir_idx = offset_idx % SECTOR_ARRAY_MAX;

    ASSERT (dbl_idx < SECTOR_ARRAY_MAX && indir_idx < SECTOR_ARRAY_MAX);
    indir_array indir = {0};
  
    //printf("(write) offset: %d\n", offset);
    //printf("(write) inode_sector: %d\n",inode->sector);
    //printf("(write) dbl_sector: %d\n",inode->data.dbl_ary);
    //printf("(write) size: %d\n", size);

    if (inode->dbl[dbl_idx] == -1)
    {
      size_t i;
      ASSERT (free_map_allocate (1, &inode->dbl[dbl_idx]));

      for (i = 0; i < SECTOR_ARRAY_MAX; i++)
        indir[i] = -1;
      block_write (fs_device, inode->dbl[dbl_idx], &indir);
    }

    else
   //   cache_read_at (inode->dbl[dbl_idx], &indir, 0, BLOCK_SECTOR_SIZE);
      block_read (fs_device, inode->dbl[dbl_idx], &indir);

    block_sector_t sector_idx = indir[indir_idx];
    //printf("(write) indir_idx:  %d\n",indir_idx);
    //printf("(write) sector_idx: %d\n",sector_idx);

    int sector_ofs = offset % BLOCK_SECTOR_SIZE;

    /* Bytes left in inode, bytes left in sector, lesser of the two. */
    off_t inode_left = inode_length (inode) - offset;
    int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
    int min_left = inode_left < sector_left ? inode_left : sector_left;

    //printf("(write) inode_left: %d, sector_left: %d\n", inode_left, sector_left);

    if (inode_left <= 0)
      min_left = sector_left;

    /* Number of bytes to actually write into this sector. */
    int chunk_size = size < min_left ? size : min_left;
    //printf("(write) chunk_size: %d\n", chunk_size);
    if (chunk_size <= 0)
      break;

    /* read in file extension */
    if (inode_left <= 0 && sector_idx == -1)
    {
      static char zeros[BLOCK_SECTOR_SIZE];

      ASSERT (free_map_allocate (1, &sector_idx));
      block_write (fs_device, sector_idx, zeros);
      indir[indir_idx] = sector_idx;
      block_write (fs_device, inode->dbl[dbl_idx], &indir);
      block_write (fs_device, inode->data.dbl_ary, inode->dbl);
    }

    cache_write_at (sector_idx, buffer + bytes_written, sector_ofs, chunk_size);

    /* Advance. */
    if (inode_length (inode) < offset + chunk_size)
    {
      inode->data.length = offset + chunk_size;
      block_write (fs_device, inode->sector, &inode->data);
    }
    size -= chunk_size;
    offset += chunk_size;
    bytes_written += chunk_size;
  }

  //printf("(write) bytes_written: %d\n",bytes_written);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}


/* in inode.c */
bool inode_isdir (const struct inode *inode)
{
  return inode->is_dir;
}

bool inode_setdir (block_sector_t sector, bool isdir)
{
  struct inode_disk *disk_inode = calloc (1, sizeof *disk_inode);

  if (disk_inode == NULL)
    return false;

  block_read (fs_device, sector, disk_inode);
  disk_inode->is_dir = isdir;
  block_write (fs_device, sector, disk_inode);

  return true;
}

bool inode_open_only (const struct inode *inode)
{
  //printf("open_cnt: %d\n",inode->open_cnt);
  return inode->open_cnt <= 2;
}
