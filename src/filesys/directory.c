#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"

/* A directory. */
struct dir 
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
  };

/* A single directory entry. */
struct dir_entry 
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
  };

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  return (inode_create (sector, entry_cnt * sizeof (struct dir_entry))
      && inode_setdir (sector, true));
}


/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode) 
{
  struct dir *dir = calloc (1, sizeof *dir);
  //inode_open_only (inode);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      dir->pos = 0;
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL; 
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir) 
{
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir) 
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir) 
{
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
  {

    //printf("(lookup) e.name: %s\n", e.name);
    if (e.in_use && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
  }
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else
    *inode = NULL;

  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  //printf("(dir-add) e.name: %s\n", e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;

 done:
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name) 
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);
  if (inode == NULL)
    goto done;

  if (inode_isdir (inode) && !dir_isempty (inode))
    goto done;

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) 
    goto done;

  /* Remove inode. */
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) 
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        } 
    }
  return false;
}

static bool 
is_absolute_path (const char * path)
{
  if (*path == '/')
    return true;

  return false;
}

/* parsing path, then return struct dir * to path_dir,
   and file name pointer to file_name */
void path_parser (const char *path, struct dir **path_dir, char ** file_name)
{
  struct thread *t = thread_current ();
  char *split_path;
  char *token = NULL, *ptr = NULL;
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;
  char *stack_name[202] = {0};
  int stack_top = -1;

  //printf("cur: %s and len: %d\n", t->cur_dir, strlen (t->cur_dir));
  if (is_absolute_path (path))
  {
    split_path = calloc (1, strlen (path) + 1);
    if (split_path == NULL)
      return;
    strlcpy (split_path, path, strlen(path) + 1); 
  }

  else if (!strcmp (path, "."))
  {
    split_path = calloc (1, strlen (t->cur_dir) + 1);
    if (split_path == NULL)
      return;
    strlcpy (split_path, t->cur_dir, strlen(t->cur_dir) + 1); 
   
  }

  else 
  {
    split_path = calloc (1, strlen (t->cur_dir) + strlen (path) + 2);
    if (split_path == NULL)
      return;
    strlcpy (split_path, t->cur_dir, strlen (t->cur_dir) + 1);
    if (strcmp (t->cur_dir, "/"))
      strlcat (split_path, "/", strlen (split_path) + strlen ("/")+1);
    strlcat (split_path, path, strlen (t->cur_dir) + strlen (path) + 2);
    //printf("cur->dir:  %s\n",t->cur_dir);
    //printf("path: %s\n",path);
  }

  //printf("split_path: %s\n",split_path);
  token = strtok_r (split_path, "/", &ptr);

  /* path: "/" */
  if (token == NULL)
  {
    free (split_path);
    *file_name = calloc (1, strlen ("/") + 1);
    strlcpy (*file_name, "/", strlen ("/") + 1);
    *path_dir = dir;
    return ;
  }

  while (token)
  {
    //printf ("token_name: %s\n", token);
    if (!strcmp (token, "."))
      goto meet_cur;

    else if (!strcmp (token, ".."))
    {
      stack_name[stack_top--] = NULL;    // pop
      if (stack_top < -1)
        stack_top = -1;
      goto meet_cur;
    }

    stack_name[++stack_top] = token;
    //printf ("loop_stack_name: %s\n", stack_name[stack_top]);
meet_cur:
    token = strtok_r (NULL, "/", &ptr);
  }

  //  *path_dir = dir;
  *file_name = calloc (1, strlen (stack_name[stack_top]) + 1); 
  //printf ("stack_name: %s\n", stack_name[stack_top]);
  strlcpy (*file_name, stack_name[stack_top], strlen (stack_name[stack_top]) + 1);
  //printf ("file_name: %s\n", *file_name);
//  stack_name[stack_top--] = NULL;

  int going_path = 0;
  while (going_path < stack_top)
  {
    //printf("stack_name: %s\n",stack_name[going_path]);
    if (!dir_lookup (dir, stack_name[going_path], &inode))
    {
      //printf("going: %d, top: %d\n",going_path,stack_top);
      free (split_path);
      free (*file_name);
      *file_name = NULL;
      *path_dir = NULL;
      return;
    }
    dir_close (dir);
    dir = dir_open (inode);
    going_path++;
//    stack_name[stack_top--] = NULL;
  }
  *path_dir = dir;
  ASSERT (dir != NULL);
  //printf ("file_name: %s\n", *file_name);

  free (split_path);
}

bool dir_change (const char *path)
{
  struct dir *dir = NULL;
  char *name, *new_path = NULL;
  struct thread *t = thread_current ();

  if (is_absolute_path (path))
  {
    new_path = calloc (1, strlen (path) + 1);
    if (new_path == NULL)
      return false;
    strlcpy (new_path, path, strlen(path) + 1); 
  }

  else if (!strcmp (path, "."))
  {
    new_path = calloc (1, strlen (t->cur_dir) + 1);
    if (new_path == NULL)
      return false;
    strlcpy (new_path, t->cur_dir, strlen(t->cur_dir) + 1); 
  }

  else 
  {
    new_path = calloc (1, strlen (t->cur_dir) + strlen (path) + 2);
    if (new_path == NULL)
      return false;
    strlcpy (new_path, t->cur_dir, strlen (t->cur_dir) + 1);
    if (strcmp (t->cur_dir, "/"))
      strlcat (new_path, "/", strlen (new_path) + strlen ("/")+1);
    strlcat (new_path, path, strlen (t->cur_dir) + strlen (path) + 2);
  //printf("cur_path: %s\n",t->cur_dir);
  //printf("rel_path: %s\n",path);
  }

  path_parser (path, &dir, &name);

  if (dir == NULL)
  {
    free (new_path);
    return false;
  }

  struct inode *inode = NULL;

  if (!dir_lookup (dir, name, &inode))
  {
    free (new_path);
    return false;
  }

  free (t->cur_dir);
  t->cur_dir = calloc (1, strlen (new_path) + 1);
  strlcpy (t->cur_dir, new_path, strlen (new_path ) + 1);
  free (new_path);
  dir_close (t->pwd);
  t->pwd = dir;

  return true;
}

bool
dir_isempty (struct inode *inode)
{
  struct dir_entry e;
  off_t ofs;

  for (ofs = 0; inode_read_at (inode, &e, sizeof e, ofs) == sizeof e; ofs += sizeof e) 
    if (e.in_use)
      return false;

  return true;
}
