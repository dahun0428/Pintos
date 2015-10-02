#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/pte.h"
#include "devices/input.h"

#define ARG1(ESP) ( ( ((void *)ESP + 4)) )
#define ARG2(ESP) ( ( ((void *)ESP + 8)) )
#define ARG3(ESP) ( ( ((void *)ESP + 12)) )

#define CHECK_ARG1(ESP) ( ( (void *)ESP + 4) < PHYS_BASE )
#define CHECK_ARG2(ESP) ( ( (void *)ESP + 8) < PHYS_BASE )
#define CHECK_ARG3(ESP) ( ( (void *)ESP + 12) < PHYS_BASE )
static void syscall_handler (struct intr_frame *);

static struct file_des * fd_array[128];



void
syscall_init (void) 
{
  int i;
  for (i=2; i < 128; i++) fd_array[i] = NULL;

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f ) 
{
  int syscall_num;
  int ret;
  void * esp = f->esp;

  if(!pagedir_get_page(thread_current()->pagedir, esp)) exit(-1);

  syscall_num =  * (int *)esp; 

  switch( syscall_num) {
    case SYS_HALT:
      halt();
      break;

    case SYS_EXIT:
      if(!CHECK_ARG1(esp)) exit(-1); 
      exit( *(int *)ARG1(esp) );
      break;

    case SYS_EXEC:
      if(!CHECK_ARG1(esp)) exit(-1);
      ret = exec(* (const char **)ARG1(esp) ); 
      break;

    case SYS_WAIT:
      if(!CHECK_ARG1(esp)) exit(-1);
      ret = wait( * (pid_t *) ARG1(esp) );
      break;

    case SYS_CREATE:
      if(!CHECK_ARG2(esp)) exit(-1);
      ret = create(* (const char **) ARG1(esp),* (unsigned*) ARG2(esp) );
      break;

    case SYS_REMOVE:
      break;

    case SYS_OPEN:
      if(!CHECK_ARG1(esp)) exit(-1);
      ret = open(* (const char **) ARG1(esp) );
      break;

    case SYS_FILESIZE:
      if(!CHECK_ARG1(esp)) exit(-1);
      ret = filesize(* (int*)ARG1(esp)); 
      break;

    case SYS_READ:
      if(!CHECK_ARG3(esp)) exit(-1);
      ret = read(* (int*) ARG1(esp),* (void **)ARG2(esp),
             * (unsigned*) ARG3(esp) );
      break;

    case SYS_WRITE:
      if(!CHECK_ARG3(esp)) exit(-1);
      ret = write(* (int*) ARG1(esp),* (void **) ARG2(esp),
             * (unsigned*)  ARG3(esp) );
      break;

    case SYS_SEEK:
      if(!CHECK_ARG2(esp)) exit(-1);
      seek( *(int*) ARG1(esp),* (unsigned*) ARG2(esp));
      break;

    case SYS_TELL:
      if(!CHECK_ARG1(esp)) exit(-1);
      ret = tell(*(unsigned *) ARG1(esp));
      break;

    case SYS_CLOSE:
      if(!CHECK_ARG1(esp)) exit(-1);
      close( * (int *)ARG1(esp));
      break;

    default:
      exit(-1);
      break;

  }
  f->eax = ret;
}

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  
  struct thread *cur = thread_current ();

  if( cur->parent != NULL )
    cur->parent->child_status = status;
  printf ("%s: exit(%d)\n", thread_name() ,status);

  lock_release(&cur->child_lock);
  thread_exit();
}

pid_t exec(const char * cmd_line)
{
  int pid = -1;
  
  if(!is_user_vaddr( cmd_line )) exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, cmd_line)) exit(-1);
  if(cmd_line != NULL) pid =  process_execute(cmd_line);

  return pid;
}

int wait(pid_t pid)
{
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
  bool success = false;
  
  if(!is_user_vaddr( file )) exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, file)) exit(-1);

  if (file != NULL) 
    success = filesys_create(file, initial_size);

  else exit(-1);

  return success;
}


int open (const char *file)
{
  struct file_des * fds = calloc(1, sizeof (struct file_des));  

  if(!is_user_vaddr( file )) exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, file)) exit(-1);

  if(file == NULL) return -1;

  fds->file = filesys_open (file);

  int fd = 2;

  if (fds->file == NULL) return -1;

  for ( fd = 2 ; fd < 128; fd++){
    
    if( fd_array[fd] == NULL ) break;
    
  }

  if (fd == 128) return -1;

  fds->fd = fd;
  fd_array[fd] = fds;

  return fd;
}

int filesize(int fd){
  struct file * file_opened ;
  ASSERT(fd == fd_array[fd]->fd);
  

  if (fd_array[fd] == NULL) return -1;
  
  file_opened = fd_array[fd]->file;


  return file_length( file_opened );
}

int read(int fd, void * buffer, unsigned size)
{
  off_t ret = 0;

  if(!is_user_vaddr( buffer )) exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, buffer)) exit(-1);
  if ( (fd == 0 || (2 <= fd && fd <128) ) ==0) return -1;
  if ( fd >1 && fd_array[fd] == NULL) return -1;

  if (fd == 0){
   /* need more implementation */ 
    input_getc();
  }
  else 
    ret = file_read(fd_array[fd]->file, buffer, size);

  return ret;
}

int write(int fd, const void * buffer, unsigned size)
{
  off_t ret = 0;
  
  if(!is_user_vaddr( buffer )) exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, buffer)) exit(-1);
  if ( (fd == 1 || (2 <= fd && fd <128) ) ==0) return -1;
  if ( fd >1 && fd_array[fd] == NULL) return -1;

  
  if (fd == 1){
    putbuf(buffer, size);
  }
  else
    ret = file_write(fd_array[fd]->file, buffer, size);

  return ret;
}

void seek (int fd, unsigned position)
{
 file_seek(fd_array[fd]->file, (off_t) position); 
}

unsigned tell (int fd)
{
  return file_tell(fd_array[fd]->file);  
}

void close(int fd)
{

  if (fd == 1 || fd == 0 ) exit(-1);

  else if ( fd >= 128 || fd <  0) exit(-1);

  else if ( fd_array[fd] == NULL ) return;

  file_close(fd_array[fd]->file);
  free(fd_array[fd]);
  fd_array[fd] = NULL;
}

