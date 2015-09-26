#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

#define bool	_Bool
#define true	1
#define false	0
#define __bool_true_false_are_defined	1

void syscall_init (void);
/*
void halt(void);

void exit(int);

pid_t exec(const char *);

int wait(pid_t);

bool create(const char *, unsigned initial_size);
bool remove(const char *);

int open(const char *);
int filesize(int);
int read(int, void *, unsigned);
int write(int, const void *, unsigned);

void seek(int, unsigned);
unsigned tell(int);

void close(int);**/

#endif /* userprog/syscall.h */
