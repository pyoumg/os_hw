#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "userprog/gdt.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/flags.h"
#include "threads/vaddr.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
static void syscall_handler(struct intr_frame*);

void
syscall_init(void)
{
	intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame* f UNUSED)
{
	uint32_t call_num = *(uint32_t*)(f->esp);
	check_address(f->esp);
	
	switch (call_num) {
	case SYS_HALT:                   /* Halt the operating system. */
		halt();
		break;
	case SYS_EXIT:                   /* Terminate this process. */
		check_address(f->esp + 4);
		exit((int)*(uint32_t*)(f->esp + 4));
		break;
	case SYS_EXEC:
		check_address(f->esp + 4);/* Start another process. */
		f->eax = exec((char*)*(uint32_t*)(f->esp + 4));
		break;
	case SYS_WAIT:                   /* Wait for a child process to die. */
		check_address(f->esp + 4);
		f->eax = wait((pid_t) * (uint32_t*)(f->esp + 4));//어려우니까 내일 도전
		break;
	case SYS_READ:                   /* Read from a file. */
		//stdin
		check_address(f->esp + 4);
		check_address(f->esp + 8);
		check_address(f->esp + 12);
		f->eax = read((int)*(uint32_t*)(f->esp + 4), (void*)*(uint32_t*)(f->esp + 8), (unsigned)*(uint32_t*)(f->esp + 12));
		break;
	case SYS_WRITE:                  /* Write to a file. */
		//stdout
		check_address(f->esp + 4);
		check_address(f->esp + 8);
		check_address(f->esp + 12);
		f->eax = write((int)*(uint32_t*)(f->esp + 4), (void*)*(uint32_t*)(f->esp + 8), (unsigned)*(uint32_t*)(f->esp + 12));
		break;
	case SYS_FIBO:
		check_address(f->esp + 4);
		f->eax = fibonacci((int)*(uint32_t*)(f->esp + 4));
		break;
	case SYS_MAX:
		check_address(f->esp + 4);
		check_address(f->esp + 8);
		check_address(f->esp + 12);
		check_address(f->esp + 16);
		f->eax = max_of_four_int((int)*(uint32_t*)(f->esp + 4), (int)*(uint32_t*)(f->esp + 8), (int)*(uint32_t*)(f->esp + 12), (int)*(uint32_t*)(f->esp + 16));
		break;
	default:
		exit(-1);
		break;
	}


	//debug_backtrace_all();
	//printf("system call!\n");
	//thread_exit(); 
}
void halt(void) {
	shutdown_power_off();
}

void exit(int status) {
	char name[17] = { '\0' };
	struct thread* t = thread_current();

	for (int i = 0; i < 16; i++) {
		if (t->name[i] != ' ' && t->name[i] != '\0') {
			name[i] = t->name[i];
		}
		else {
			name[i] = '\0';
			break;
		}
	}

	printf("%s: exit(%d)\n", name, status);
	t->exit_status = status;
	thread_exit();

}

pid_t exec(const char* cmd_line) {
	/* Open executable file. */
	struct file* file = NULL;
	char argv[50];
	for (int i = 0; i < 50; i++) {
		if (cmd_line[i] == ' ' || cmd_line[i] == '\0') {
			argv[i] = '\0';
			break;
		}
		else {
			argv[i] = cmd_line[i];
		}
	}
	file = filesys_open(argv);
	if (file == NULL)
	{
		return -1;
	}

	return process_execute(cmd_line);
}
int wait(pid_t pid) {
	return process_wait(pid);
}
//return: the number of bytes actually written(or read)
int read(int fd, void* buffer, unsigned size) {
	//stdin

	unsigned i;

	if (fd != 0)
		return -1;
	for (i = 0; i < size; i++) {
		*(uint8_t*)buffer = input_getc();
		buffer += 1;
	}
	*(uint8_t*)buffer = '\0';
	return size;
}
int write(int fd, const void* buffer, unsigned size) {
	//stdout 
	if (fd != 1)
		return -1;

	putbuf(buffer, size);
	return size;
}
int fibonacci(int n) {
	int a = 1, b = 1,c;
	if (n < 0)
		return -1;
	else if (n == 0)
		return 0;
	else if (n<=2)
		return a;


	for (int i = 0; i < n-2; i++) {
		c = a + b;
		a = b;
		b = c;
	}
	return c;
}
int max_of_four_int(int a, int b, int c, int d) {
	int max = a;
	if (b > max)
		max = b;
	if (c > max)
		max = c;
	if (d > max)
		max = d;
	return max;
}
void
check_address(void* address) {//input: vitual address 
   /*Verify the validity of a user-provided pointer.*/
	struct thread* t = thread_current();

	if (!address || !is_user_vaddr(address)) {//NULL or not below PHYS_BASE
		exit(-1);
	}
	else if ((uint32_t)address < 0x8048000 || !pagedir_get_page(t->pagedir, address)) {//unmapped
		exit(-1);
	}

}
