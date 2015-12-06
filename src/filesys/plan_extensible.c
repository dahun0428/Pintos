5.3.2 Indexed and Extensible Files

The basic file system allocates files as a single extent, making it vulnerable to external fragmentation, that is, it is possible that an n-block file cannot be allocated even though n blocks are free. Eliminate this problem by modifying the on-disk inode structure. 
"Actually I should modify on-disk structure, or inode. it needs thoughts.

In practice, this probably means using an index structure with direct, indirect, and doubly indirect blocks. 
"double indirect, 128 * 128 * BLOCK_SIZE;

You are welcome to choose a different scheme as long as you explain the rationale for it in your design documentation, and as long as it does not suffer from external fragmentation (as does the extent-based file system we provide).

You can assume that the file system partition will not be larger than 8 MB. You must support files as large as the partition (minus metadata). Each inode is stored in one disk sector, limiting the number of block pointers that it can contain. Supporting 8 MB files will require you to implement doubly-indirect blocks.
"ok i will try.."

An extent-based file can only grow if it is followed by empty space, but indexed inodes make file growth possible whenever free space is available. Implement file growth. In the basic file system, the file size is specified when the file is created. In most modern file systems, a file is initially created with size 0 and is then expanded every time a write is made off the end of the file. Your file system must allow this.
"with what? max size, block index list.. etc "

There should be no predetermined limit on the size of a file, except that a file cannot exceed the size of the file system (minus metadata). This also applies to the root directory file, which should now be allowed to expand beyond its initial limit of 16 files.
"does limit means the number of files..? or file size..?"

User programs are allowed to seek beyond the current end-of-file (EOF). The seek itself does not extend the file. Writing at a position past EOF extends the file to the position being written, and any gap between the previous EOF and the start of the write must be filled with zeros. A read starting from a position past EOF returns no bytes.
"got it"

Writing far beyond EOF can cause many blocks to be entirely zero. Some file systems allocate and write real data blocks for these implicitly zeroed blocks. Other file systems do not allocate these blocks at all until they are explicitly written. The latter file systems are said to support "sparse files." You may adopt either allocation strategy in your file system.
"got it"

---

Functions i will fix
{
 bool inode_create ();  /* sequential to random */
      inode_open ();    /* read data; and read dbl_ary idx */
      free_map_release () or inode_close ();
      /* actually, i should modify all functions 
         that using byte_to_sector (maybe) or "cnt" variable */
      inode_read () and inode_write ();

      /* following functions with access past EOF */
      seek ();
      write ();
      read ();     

}

struct inode_disk
  {
//    block_sector_t start;               /* First data sector. */
    block_sector_t dbl_ary;
    
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[125];               /* Not used. */
  };
/* additional variable:
   bool dir */


struct inode
{
  block_sector_t **dbl = read frome dbl_ary;

  /* others... */
}

off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
    off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  struct buffer_cache * cache = NULL;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0) 
  {
    /* Sector to write, starting byte offset within sector. */
    block_sector_t offset_idx = offset / BLOCK_SECTOR_SIZE; //byte_to_sector (inode, offset);
    block_sector_t dbl_idx = offset_idx / SECTOR_ARRAY_MAX;
    block_sector_t indir_idx = offset_idx % SECTOR_ARRAY_MAX;

    indir_array indir = {0};
  
    //printf("(write) offset: %d\n", offset);
    //printf("(write) inode_sector: %d\n",inode->sector);
    //printf("(write) dbl_sector: %d\n",inode->data.dbl_ary);
    block_read (fs_device, inode->dbl[dbl_idx], &indir);
    block_sector_t sector_idx = indir[indir_idx];
    //printf("(write) indir_idx:  %d\n",indir_idx);
    //printf("(write) sector_idx: %d\n",sector_idx);

    int sector_ofs = offset % BLOCK_SECTOR_SIZE;

    /* Bytes left in inode, bytes left in sector, lesser of the two. */
    off_t inode_left = inode_length (inode) - offset;
    int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
    int min_left = inode_left < sector_left ? inode_left : sector_left;

    /* Number of bytes to actually write into this sector. */
    int chunk_size = size < min_left ? size : min_left;
    if (chunk_size <= 0)
      break;

    cache = buffer_find_cache (sector_idx);
    if (cache == NULL)
    {
      cache = buffer_get_cache (sector_idx);
      if (sector_ofs > 0 || chunk_size < sector_left) 
        block_read (fs_device, sector_idx, cache->data);
      else
        memset (cache->data, 0, BLOCK_SECTOR_SIZE);

    }

    /* If the sector contains data before or after the chunk
       we're writing, then we need to read in the sector
       first.  Otherwise we start with a sector of all zeros. */
    memcpy (cache->data + sector_ofs, buffer + bytes_written, chunk_size);
//    block_write (fs_device, sector_idx, cache->data);
    set_buffer_accessed (cache, true);
    set_buffer_dirty (cache, true);


    /* Advance. */
    size -= chunk_size;
    offset += chunk_size;
    bytes_written += chunk_size;
  }

  //printf("(write) bytes_written: %d\n",bytes_written);
  return bytes_written;
}
