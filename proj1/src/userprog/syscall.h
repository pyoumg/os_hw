#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
typedef int pid_t;
void syscall_init (void);
void halt(void);
void exit(int status);
pid_t exec(const char* cmd_line);
int wait(pid_t pid);
int read(int fd, void* buffer, unsigned size);
int write(int fd, const void* buffer, unsigned size);
void check_address(void* address);
int max_of_four_int(int a, int b, int c, int d);
int fibonacci(int n);
#endif /* userprog/syscall.h */
