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
#include "filesys/inode.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
static void syscall_handler(struct intr_frame*);
typedef int32_t off_t;
/* An open file. */
struct file
{
	struct inode* inode;        /* File's inode. */
	off_t pos;                  /* Current position. */
	bool deny_write;            /* Has file_deny_write() been called? */
};



void
syscall_init(void)
{
	lock_init(&filesys_lock);
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
		f->eax = wait((pid_t) * (uint32_t*)(f->esp + 4));
		break;
	case SYS_READ:                   /* Read from a file. */
		check_address(f->esp + 4);
		check_address(f->esp + 8);
		check_address(f->esp + 12);
		f->eax = read((int)*(uint32_t*)(f->esp + 4), (void*)*(uint32_t*)(f->esp + 8), (unsigned)*(uint32_t*)(f->esp + 12));
		break;
	case SYS_WRITE:                  /* Write to a file. */
		check_address(f->esp + 4);
		check_address(f->esp + 8);
		check_address(f->esp + 12);
		f->eax = write((int)*(uint32_t*)(f->esp + 4), (void*)*(uint32_t*)(f->esp + 8), (unsigned)*(uint32_t*)(f->esp + 12));
		break;
	case SYS_CREATE:
		check_address(f->esp + 4);
		check_address(f->esp + 8);
		f->eax = create((const char*)*(uint32_t*)(f->esp + 4), (unsigned)*(uint32_t*)(f->esp + 8));
		break;
	case SYS_REMOVE:
		check_address(f->esp + 4);
		f->eax = remove((const char*)*(uint32_t*)(f->esp + 4));
		break;
	case SYS_OPEN:
		check_address(f->esp + 4);
		f->eax = open((const char*)*(uint32_t*)(f->esp + 4));
		break;
	case SYS_CLOSE:
		check_address(f->esp + 4);
		close((int)*(uint32_t*)(f->esp + 4));
		break;
	case SYS_FILESIZE:
		check_address(f->esp + 4);
		f->eax = filesize((int)*(uint32_t*)(f->esp + 4));
		break;
	case SYS_SEEK:
		check_address(f->esp + 4);
		check_address(f->esp + 8);
		seek((int)*(uint32_t*)(f->esp + 4), (unsigned)*(uint32_t*)(f->esp + 8));
		break;
	case SYS_TELL:
		check_address(f->esp + 4);
		f->eax = tell((int)*(uint32_t*)(f->esp + 4));
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


}
void halt(void) {
	shutdown_power_off();
}

void exit(int status) {
	char name[17] = { '\0' };
	struct thread* t = thread_current();
	struct list_elem* child = list_begin(&(t->child_list));
	struct list_elem* child_end = list_end(&(t->child_list));
	struct thread* temp;

	for (int i = 0; i < 16; i++) {
		if (t->name[i] != ' ' && t->name[i] != '\0') {
			name[i] = t->name[i];
		}
		else {
			name[i] = '\0';
			break;
		}
	}
	//file close
	for (int i = 3; i < 129; i++) {
		if (t->fd_table[i]) {
			file_close(t->fd_table[i]);
			t->fd_table[i] = NULL;
		}
	}
	//wait child thread
	for (child; child != NULL && child != child_end; child = child->next) {
		temp = list_entry(child, struct thread, child_elem);
		process_wait(temp->tid);
	}


	printf("%s: exit(%d)\n", name, status);
	t->exit_status = status;
	thread_exit();

}

pid_t exec(const char* cmd_line) {
	/* Open executable file. */
	struct thread* t = thread_current();

	pid_t pid= process_execute(cmd_line);
	struct thread* t_child = get_child_process(pid);
	if (t_child == NULL)
		return -1;
	sema_down(&(t->sema.load));
	if (t_child->is_load == 0)
		return -1;
	return pid;
}
int wait(pid_t pid) {
	return process_wait(pid);
}
//return: the number of bytes actually written(or read)
int read(int fd, void* buffer, unsigned size) {

	unsigned i;
	check_address(buffer);
	lock_acquire(&filesys_lock);
	struct thread* t = thread_current();
	if (fd == 0) {
		for (i = 0; i < size; i++) {
			*(uint8_t*)buffer = input_getc();
			buffer += 1;
		}
		*(uint8_t*)buffer = '\0';

	}

	else if (fd > 2) {
		if (!(t->fd_table[fd])) {//NULL
			lock_release(&filesys_lock);
			exit(-1);
		}
		size= file_read(t->fd_table[fd], buffer, size);
	}
	lock_release(&filesys_lock);
	return size;
}
int write(int fd, const void* buffer, unsigned size) {
	int write_size = -1;
	check_address(buffer);
	struct thread* t = thread_current();
	lock_acquire(&filesys_lock);
	if (fd == 1) {
		putbuf(buffer, size);
		write_size = size;
	}
	else if (fd > 2) {
		if (!(t->fd_table[fd])) {
			lock_release(&filesys_lock);
			exit(-1);
		}
		if (t->fd_table[fd]->deny_write) {//write ok
			file_deny_write(t->fd_table[fd]);
		}
		write_size = file_write(t->fd_table[fd], buffer, size);
	}
	lock_release(&filesys_lock);
	//printf("write size%d\n", write_size);
	return write_size;
}


/*project2*/

bool create(const char* file, unsigned initial_size) {
	if (!file)
		exit(-1);
	return filesys_create(file, initial_size);

}

bool remove(const char* file) {
	if (!file)
		exit(-1);
	return filesys_remove(file);
}
//get fd and return fd
int open(const char* file) {
	int fd = -1;
	struct thread* t = thread_current();
	if (!file)
		exit(-1);
	lock_acquire(&filesys_lock);
	struct file* f = filesys_open(file);
	if (f != NULL) {
		for (int i = 3; i < 129; i++) {
			if (t->fd_table[i] == NULL) {
				if (syscall_strcmp(thread_name(), file)==0) {
					file_deny_write(f);
				}
				t->fd_table[i] = f;
				fd = i;
				break;
			}
		}
		lock_release(&filesys_lock);
		return fd;
	}
	else {
		lock_release(&filesys_lock);
		return -1;
	}
}
void close(int fd) {
	struct thread* t = thread_current();
	if (t->fd_table[fd]) {
		process_close_file(fd);
	}
	else
		exit(-1);
}
int filesize(int fd) {
	struct thread* t = thread_current();
	if (!(t->fd_table[fd]))//NULL
		exit(-1);
	return file_length(t->fd_table[fd]);
}
void seek(int fd, unsigned position) {
	struct thread* t = thread_current();
	if (!(t->fd_table[fd]))//NULL (position is unsigned)
		exit(-1);
	file_seek(t->fd_table[fd], position);
}
unsigned tell(int fd) {
	struct thread* t = thread_current();
	if (!(t->fd_table[fd]))//NULL
		exit(-1);
	return file_tell(t->fd_table[fd]);
}

int fibonacci(int n) {
	int a = 1, b = 1, c;
	if (n < 0)
		return -1;
	else if (n == 0)
		return 0;
	else if (n <= 2)
		return a;


	for (int i = 0; i < n - 2; i++) {
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
int syscall_strcmp(char* str1, char* str2) {//same:0 else:-1 (str2 is shorter or same than str1)
	int flag = 0;
	for (int i = 0; i < 15; i++) {
		if (str2[i] == '\0')
			break;
		if (str1[i] != str2[i]) {
			flag = -1;
			break;
		}

	}
	return flag;
}