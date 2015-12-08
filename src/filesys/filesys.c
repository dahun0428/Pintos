#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"
#include "threads/malloc.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();
  cache_init ();

  if (format) 
    do_format ();

  free_map_open ();

  struct thread *t = thread_current ();
  if (!strcmp (t->cur_dir, "/"))
    t->pwd = dir_open_root ();
  else
    t->pwd = dir_reopen (t->parent->pwd);
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name_, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  char *name = NULL;
  struct dir *dir = NULL; //dir_open_root ();

  path_parser (name_, &dir, &name);
  //printf("full_name: %s\n",name_);
  //printf("name: %s\n",name);
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  if (name != NULL)
    free (name);
  //printf("create %s: %s\n",name, success ? "success" : "fail");
  return success;
}

bool
filesys_mkdir (const char *name_)
{
  block_sector_t inode_sector = 0;
  off_t initial_size = 0;
  char *name = NULL;
  struct dir *dir = NULL; //dir_open_root ();

  path_parser (name_, &dir, &name);
  //printf("name: %s\n",name);
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && dir_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  if (name != NULL)
    free (name);

  //printf("mkdir %s: %s\n",name, success ? "success" : "fail");
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name_)
{
  char *name = NULL;
  struct dir *dir = NULL; //dir_open_root ();
  struct inode *inode = NULL;
  struct thread *t = thread_current ();

 // printf("cur: %s and len: %d\n", t->cur_dir, strlen (t->cur_dir));
  path_parser (name_, &dir, &name);
  
  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  if (name != NULL)
    free (name);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name_) 
{
  char *name = NULL;
  struct dir *dir = NULL; //dir_open_root ();

  if (!strcmp (name_, "/"))
    return false;

  path_parser (name_, &dir, &name);

  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  if (name != NULL)
    free (name);

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  if (!dir_add (dir_open_root (), "/", ROOT_DIR_SECTOR))
    PANIC ("HOIT");
  free_map_close ();
  printf ("done.\n");
}

