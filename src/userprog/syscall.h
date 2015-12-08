#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

#define bool	_Bool
#define true	1
#define false	0
#define __bool_true_false_are_defined	1


void syscall_init (void);

void sys_halt(void);

void sys_exit(int);

pid_t sys_exec(const char *);

int sys_wait(pid_t);

bool sys_create(const char *, unsigned initial_size);
bool sys_remove(const char *);

int sys_open(const char *);
int sys_filesize(int);
int sys_read(int, void *, unsigned);
int sys_write(int, const void *, unsigned);

void sys_seek(int, unsigned);
unsigned sys_tell(int);

void sys_close(int);

bool sys_chdir (const char *);
bool sys_mkdir (const char *);
bool sys_readdir (int, char *);
bool sys_isdir (int);
int sys_inumber (int);
#endif /* userprog/syscall.h */
