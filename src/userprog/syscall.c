#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
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




void
syscall_init (void) 
{

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f ) 
{
  int syscall_num;
  int ret;
  void * esp = f->esp;

  if(!pagedir_get_page(thread_current()->pagedir, esp)) sys_exit(-1);

  syscall_num =  * (int *)esp; 

  switch( syscall_num) {
    case SYS_HALT:
      sys_halt();
      break;

    case SYS_EXIT:
      if(!CHECK_ARG1(esp)) sys_exit(-1); 
      sys_exit( *(int *)ARG1(esp) );
      break;

    case SYS_EXEC:
      if(!CHECK_ARG1(esp)) sys_exit(-1);
      ret = sys_exec(* (const char **)ARG1(esp) ); 
      break;

    case SYS_WAIT:
      if(!CHECK_ARG1(esp)) sys_exit(-1);
      ret = sys_wait( * (pid_t *) ARG1(esp) );
      break;

    case SYS_CREATE:
      if(!CHECK_ARG2(esp)) sys_exit(-1);
      ret = sys_create(* (const char **) ARG1(esp),* (unsigned*) ARG2(esp) );
      break;

    case SYS_REMOVE:
      if(!CHECK_ARG1(esp)) sys_exit(-1);
      ret = sys_remove(* (const char **) ARG1(esp) );
      break;

    case SYS_OPEN:
      if(!CHECK_ARG1(esp)) sys_exit(-1);
      ret = sys_open(* (const char **) ARG1(esp) );
      break;

    case SYS_FILESIZE:
      if(!CHECK_ARG1(esp)) sys_exit(-1);
      ret = sys_filesize(* (int*)ARG1(esp)); 
      break;

    case SYS_READ:
      if(!CHECK_ARG3(esp)) sys_exit(-1);
      ret = sys_read(* (int*) ARG1(esp),* (void **)ARG2(esp),
             * (unsigned*) ARG3(esp) );
      break;

    case SYS_WRITE:
      if(!CHECK_ARG3(esp)) sys_exit(-1);
      ret = sys_write(* (int*) ARG1(esp),* (void **) ARG2(esp),
             * (unsigned*)  ARG3(esp) );
      break;

    case SYS_SEEK:
      if(!CHECK_ARG2(esp)) sys_exit(-1);
      sys_seek( *(int*) ARG1(esp),* (unsigned*) ARG2(esp));
      break;

    case SYS_TELL:
      if(!CHECK_ARG1(esp)) sys_exit(-1);
      ret = sys_tell(*(unsigned *) ARG1(esp));
      break;

    case SYS_CLOSE:
      if(!CHECK_ARG1(esp)) sys_exit(-1);
      sys_close( * (int *)ARG1(esp));
      break;

    default:
      sys_exit(-1);
      break;

  }
  f->eax = ret;
}

void sys_halt(void)
{
  shutdown_power_off();
}

void sys_exit(int status)
{
  
  struct thread *cur = thread_current ();
  struct thread *par = cur->parent;
  struct list_elem * e;
  int i;

  if( par != NULL ){
    for (e = list_begin(&par->child_list); 
          e != list_end(&par->child_list);
          e = list_next(e)){
      struct child_info * cinfo =
        list_entry(e, struct child_info, child_elem);
        if (cinfo->tid == cur->tid){
          cinfo->status = status;
        }
    }
  }

  for (e = list_begin(&cur->child_list);e != list_end(&cur->child_list);){

    struct child_info * cinfo = list_entry (e, struct child_info, child_elem);
    sys_wait(cinfo->tid);

    e = list_next(e);
    free(cinfo);
  } 

  for (i=2 ; i < 128; i ++)
    if(cur->file_des[i] != NULL) {
      sys_close(i);

      ASSERT( cur->file_des[i] == NULL );
    }

  if(cur->file != NULL) 
    file_allow_write(cur->file);

  printf ("%s: exit(%d)\n", thread_name() ,status);
  lock_release(&cur->myinfo->child_lock);
  thread_exit();
}

pid_t sys_exec(const char * cmd_line)
{
  int pid = -1;
  char *cmd_copy;
  
  if(!is_user_vaddr( cmd_line ))   sys_exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, cmd_line)) sys_exit(-1);
  if(cmd_line == NULL) return -1;

  pid = process_execute (cmd_line);
 
  return pid;
}

int sys_wait(pid_t pid)
{
  return process_wait(pid);
}

bool sys_create (const char *file, unsigned initial_size)
{
  bool success = false;
  
  if(!is_user_vaddr( file )) sys_exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, file)) sys_exit(-1);

  if (file != NULL) 
    success = filesys_create(file, initial_size);

  else sys_exit(-1);

  return success;
}

bool sys_remove(const char *file)
{
  bool success = false;
  
  if(!is_user_vaddr( file )) sys_exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, file)) sys_exit(-1);

  if (file != NULL) 
    success = filesys_remove(file);

  else sys_exit(-1);

  return success;
  
}

int sys_open (const char *file)
{
  struct file_des * fds;
  struct thread * t = thread_current();
  if(!is_user_vaddr( file )) sys_exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, file)) sys_exit(-1);

  if(file == NULL) return -1;

  fds = calloc(1, sizeof (struct file_des));  

  if (fds == NULL)
    sys_exit(-1);

  fds->file = filesys_open (file);

  int fd = 2;


  if (fds->file == NULL) return -1;

  for ( fd = 2 ; fd < 128; fd++){
    
    if( t->file_des[fd] == NULL ) break;
    
  }

  if (fd == 128) return -1;

  fds->fd = fd;
  t->file_des[fd] = fds;
  
/*  if (strcmp (file, t->name) == 0)
    file_deny_write(t->file_des[fd]->file); */


  return fd;
}

int sys_filesize(int fd){
  struct file * file_opened ;
  struct thread * t = thread_current();
  ASSERT(fd == t->file_des[fd]->fd);

  if (t->file_des[fd] == NULL) return -1;
  
  file_opened = t->file_des[fd]->file;

  return file_length( file_opened );
}

int sys_read(int fd, void * buffer, unsigned size)
{
  off_t ret = 0;
  struct thread * t = thread_current();

  if(!is_user_vaddr( buffer )) sys_exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, buffer)) sys_exit(-1);
  if ( (fd == 0 || (2 <= fd && fd <128) ) ==0) return -1;
  if ( fd >1 && t->file_des[fd] == NULL) return -1;


  if (fd == 0)
    while(size != ret) {
      *( (char*) buffer++) = input_getc ();
      ret++;
    }

  else 
    ret = file_read(t->file_des[fd]->file, buffer, size);


  return ret;
}

int sys_write(int fd, const void * buffer, unsigned size)
{
  off_t ret = 0;
  struct thread * t = thread_current();
  
  if(!is_user_vaddr( buffer )) sys_exit(-1);
  if(!pagedir_get_page(thread_current()->pagedir, buffer)) sys_exit(-1);
  if ( (fd == 1 || (2 <= fd && fd <128) ) ==0) return -1;
  if ( fd >1 && t->file_des[fd] == NULL) return -1;

  
  if (fd == 1){
    putbuf(buffer, size);
  }
  else
    ret = file_write(t->file_des[fd]->file, buffer, size);

  return ret;
}

void sys_seek (int fd, unsigned position)
{
  struct thread * t = thread_current();
  file_seek(t->file_des[fd]->file, (off_t) position); 
}

unsigned sys_tell (int fd)
{
  struct thread * t = thread_current();
  return file_tell(t->file_des[fd]->file);  
}

void sys_close(int fd)
{
  struct thread * t = thread_current();

  if (fd == 1 || fd == 0 ) sys_exit(-1);

  else if ( fd >= 128 || fd <  0) sys_exit(-1);

  else if ( t->file_des[fd] == NULL ) return;

 // file_allow_write(t->file_des[fd]->file);
  file_close(t->file_des[fd]->file);
  free(t->file_des[fd]);
  t->file_des[fd] = NULL;
}

