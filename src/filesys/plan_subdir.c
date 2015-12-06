5.3.3 Subdirectories

Implement a hierarchical name space. In the basic file system, all files live in a single directory. 
Modify this to allow directory entries to point to files or to other directories.
"pointer at dir, inode? where?"

Make sure that directories can expand beyond their original size just as any other file can.
"ok"

The basic file system has a 14-character limit on file names. You may retain this limit for individual file name components, or may extend it, at your option. You must allow full path names to be much longer than 14 characters.
"full path: absoulte or relative? if relative, what should I save more?"

Maintain a separate current directory for each process. At startup, set the root as the initial process's current directory. 
"cur_dir = root (at begin) -> in struct thread"

When one process starts another with the exec system call, the child process inherits its parent's current directory. After that, the two processes' current directories are independent, so that either changing its own current directory has no effect on the other. (This is why, under Unix, the cd command is a shell built-in, not an external program.)
"parent gives its dir to child"

Update the existing system calls so that, anywhere a file name is provided by the caller, an absolute or relative path name may used. The directory separator character is forward slash (/). You must also support special file names . and .., which have the same meanings as they do in Unix.
". and .. may be in thread.h , ans i need path parser"

Update the open system call so that it can also open directories. Of the existing system calls, only close needs to accept a file descriptor for a directory.
"write on dir -> write what?;"

Update the remove system call so that it can delete empty directories (other than the root) in addition to regular files. Directories may only be deleted if they do not contain any files or subdirectories (other than . and ..).
"of course"

You may decide whether to allow deletion of a directory that is open by a process or in use as a process's current working directory. If it is allowed, then attempts to open files (including . and ..) or create new files in a deleted directory must be disallowed.
"lets disallow"

Implement the following new system calls:
System Call: bool chdir (const char *dir)
  Changes the current working directory of the process to dir, which may be relative or absolute. Returns true if successful, false on failure.
  System Call: bool mkdir (const char *dir)
  Creates the directory named dir, which may be relative or absolute. Returns true if successful, false on failure. Fails if dir already exists or if any directory name in dir, besides the last, does not already exist. That is, mkdir("/a/b/c") succeeds only if /a/b already exists and /a/b/c does not.
  System Call: bool readdir (int fd, char *name)
  Reads a directory entry from file descriptor fd, which must represent a directory. If successful, stores the null-terminated file name in name, which must have room for READDIR_MAX_LEN + 1 bytes, and returns true. If no entries are left in the directory, returns false.
  . and .. should not be returned by readdir.

  If the directory changes while it is open, then it is acceptable for some entries not to be read at all or to be read multiple times. Otherwise, each directory entry should be read once, in any order.

  READDIR_MAX_LEN is defined in lib/user/syscall.h. If your file system supports longer file names than the basic file system, you should increase this value from the default of 14.

  System Call: bool isdir (int fd)
  Returns true if fd represents a directory, false if it represents an ordinary file.

  System Call: int inumber (int fd)
  Returns the inode number of the inode associated with fd, which may represent an ordinary file or a directory.
  An inode number persistently identifies a file or directory. It is unique during the file's existence. In Pintos, the sector number of the inode is suitable for use as an inode number.

  We have provided ls and mkdir user programs, which are straightforward once the above syscalls are implemented. We have also provided pwd, which is not so straightforward. The shell program implements cd internally.
  The pintos extract and append commands should now accept full path names, assuming that the directories used in the paths have already been created. This should not require any significant extra effort on your part.

The function I will implement:
  const char * rel2abs (const char *) // relative path to absolute path
  const char * abs2rel (const char *) // absolute path to relative path, may dir name
  const char * path_parser (const char *) // from given path, find dir name.
struct thread
{
  char * pwd; /* current path. maybe absolute path */
  /* others... */
}
