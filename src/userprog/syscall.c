#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "console.h"
#include "lib/kernel/console.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/free-map.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"
#include "threads/pte.h"
#include "threads/init.h"
#include "userprog/pagedir.h"


typedef int pid_t;
struct file_descriptor
{
  int fd;
  struct file *file;
  struct list_elem elem;
  struct lock file_lock;
};
int get_new_fd (void);

static struct lock file_lock;
static void syscall_handler (struct intr_frame *);
static bool verify_user_pointer(const void *ptr);
void exit(int status);
static int write(int fd, const void *buffer, unsigned size);
static void halt(void);
static int open(const char *file);
static pid_t exec (const char *cmd_line);
static int wait (pid_t pid);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
}

int 
get_new_fd (void)
{
  static int next_fd = 2;
  return next_fd++;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if(!verify_user_pointer(f->esp) || !verify_user_pointer(f->esp + 4) || !verify_user_pointer(f->esp + 8) || !verify_user_pointer(f->esp + 12) || pagedir_get_page(thread_current()->pagedir, f->esp) == NULL)
    exit(-1);
  int syscall_number = *(int *)f->esp;
  thread_current()->esp = f->esp;
  switch(syscall_number)
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      if(!verify_user_pointer(f->esp + 4))
        exit(-1);
      exit(*(int *)(f->esp + 4));
      break;
    case SYS_EXEC:
      if(!verify_user_pointer(f->esp + 4))
        exit(-1);
      f->eax = exec((const char *)*(int *)(f->esp + 4));
      break;
    case SYS_WAIT:
      if(!verify_user_pointer(f->esp + 4))
        exit(-1);
      f->eax = wait(*(int *)(f->esp + 4));
      break;
    case SYS_CREATE:
      if(!verify_user_pointer(f->esp + 4) || !verify_user_pointer(f->esp + 8))
        exit(-1);
      f->eax = create((const char *)*(int *)(f->esp + 4), *(unsigned *)(f->esp + 8));
      break;
    case SYS_REMOVE:
      if(!verify_user_pointer(f->esp + 4))
        exit(-1);
      f->eax = remove((const char *)*(int *)(f->esp + 4));
      break;
    case SYS_OPEN:
      if(!verify_user_pointer(f->esp + 4))
        exit(-1);
      f->eax = open((const char *)*(int *)(f->esp + 4));
      break;
    case SYS_READ:
      if(!verify_user_pointer(f->esp + 4) || !verify_user_pointer(f->esp + 8) || !verify_user_pointer(f->esp + 12))
        exit(-1);
      f->eax = read(*(int *)(f->esp + 4), (void *)*(int *)(f->esp + 8), *(unsigned *)(f->esp + 12));
      break;
    case SYS_WRITE:
      if(!verify_user_pointer(f->esp + 4) || !verify_user_pointer(f->esp + 8) || !verify_user_pointer(f->esp + 12))
        exit(-1);
      f->eax = write(*(int *)(f->esp + 4), (const void *)*(int *)(f->esp + 8), *(unsigned *)(f->esp + 12));
      break;
    case SYS_FILESIZE:
      if(!verify_user_pointer(f->esp + 4))
        exit(-1);
      f->eax = filesize(*(int *)(f->esp + 4));
      break;
    case SYS_SEEK:
      if(!verify_user_pointer(f->esp + 4) || !verify_user_pointer(f->esp + 8))
        exit(-1);
      f->eax = seek(*(int *)(f->esp + 4), *(unsigned *)(f->esp + 8));
      break;

}
}

static bool
verify_user_pointer(const void *ptr)
{
  if (ptr == NULL || !is_user_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir, ptr) == NULL)
    return false;
  return true;
}

void
exit(int status)
{
  struct thread *cur = thread_current();
  char *save_ptr;
  char *name = strtok_r(cur->name, " ", &save_ptr);
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", name, status);
  thread_exit();
}

write(int fd, const void *buffer, unsigned size)
{
  if(!verify_user_pointer(buffer) || !verify_user_pointer(buffer + size))
    exit(-1);
  if (fd == 1)
    {
      lock_acquire (&file_lock);
      putbuf(buffer, size);
      lock_release (&file_lock);
      return size;
    }
  else
    {
      struct file_descriptor *fdesc = NULL;
      struct list_elem *e;
      for (e = list_begin (&thread_current ()->files); e != list_end (&thread_current ()->files); e = list_next (e))
        {
          fdesc = list_entry (e, struct file_descriptor, elem);
          if (fdesc->fd == fd)
            break;
        }
      if (fdesc == NULL)
        return -1;
      lock_acquire (&fdesc->file_lock);
      int bytes_read = file_write (fdesc->file, buffer, size);
      lock_release (&fdesc->file_lock);
      return bytes_read;
    }

}

void
halt(void)
{
  shutdown_power_off();
}

int
open (const char *file)
{
  struct file *f = NULL;
  struct file_descriptor *fd = NULL;
  int f_d = -1;
  if(!verify_user_pointer(file)){
    exit(-1);
  }
  if (file != NULL)
    {
      lock_acquire(&file_lock);
      f = filesys_open (file);
      lock_release(&file_lock);
      if (f != NULL)
        {
          fd = malloc (sizeof (struct file_descriptor));
          if (fd == NULL)
            {
              file_close (f);
              return -1;
            }
          lock_acquire (&file_lock);
          f_d = get_new_fd ();
          fd->fd = f_d;
          fd->file = f;
          lock_init (&fd->file_lock);
          list_push_back (&thread_current ()->files, &fd->elem);
          lock_release (&file_lock);
        }
    }

  return f_d;
}

pid_t
exec (const char *cmd_line)
{
  if (cmd_line == NULL || verify_user_pointer(cmd_line) == false || verify_user_pointer(cmd_line + 4) == false)
    exit(-1);
  lock_acquire (&file_lock);
  tid_t tid = process_execute (cmd_line);
  lock_release (&file_lock);
  if (tid == TID_ERROR)
    return -1;
  return tid;
}


int
wait (pid_t pid)
{
  return process_wait (pid);
}

bool
create (const char *file, unsigned initial_size)
{
  if (file == NULL || verify_user_pointer(file) == false)
    exit(-1);
  lock_acquire(&file_lock);
  bool result = filesys_create (file, initial_size);
  lock_release(&file_lock);
  
  return result;
}

bool
remove (const char *file)
{
  if (file == NULL)
    return false;

  lock_acquire(&file_lock);
  bool result = filesys_remove (file);
  lock_release(&file_lock);
  return result;
}

int
read (int fd, void *buffer, unsigned size)
{
  if(!verify_user_pointer(buffer) || !verify_user_pointer(buffer + size))
    exit(-1);
  if (fd == 0)
    {
      lock_acquire (&file_lock);
      unsigned i;
      for (i = 0; i < size; i++)
        {
          if (( (char *) buffer)[i] == '\0')
            break;
        }
      lock_release (&file_lock);
      return i;
    }
  else
    {
      struct file_descriptor *fdesc = NULL;
      struct list_elem *e;
      for (e = list_begin (&thread_current ()->files); e != list_end (&thread_current ()->files); e = list_next (e))
        {
          fdesc = list_entry (e, struct file_descriptor, elem);
          if (fdesc->fd == fd)
            break;
        }
      if (fdesc == NULL)
        return -1;
      int bytes_read = file_read (fdesc->file, buffer, size);
      return bytes_read;
    }
}

int 
filesize (int fd)
{
  struct file_descriptor *fdesc = NULL;
  struct list_elem *e;
  for (e = list_begin (&thread_current ()->files); e != list_end (&thread_current ()->files); e = list_next (e))
    {
      fdesc = list_entry (e, struct file_descriptor, elem);
      if (fdesc->fd == fd)
        break;
    }
  if (fdesc == NULL)
    return -1;
  lock_acquire (&file_lock);
  int size = file_length (fdesc->file);
  lock_release (&file_lock);
  return size;
}

int
seek (int fd, unsigned position)
{
  struct file_descriptor *fdesc = NULL;
  struct list_elem *e;
  for (e = list_begin (&thread_current ()->files); e != list_end (&thread_current ()->files); e = list_next (e))
    {
      fdesc = list_entry (e, struct file_descriptor, elem);
      if (fdesc->fd == fd)
        break;
    }
  if (fdesc == NULL)
    return -1;
  lock_acquire (&fdesc->file_lock);
  file_seek (fdesc->file, position);
  lock_release (&fdesc->file_lock);
  return 0;
}

unsigned
tell (int fd)
{
  struct file_descriptor *fdesc = NULL;
  struct list_elem *e;
  for (e = list_begin (&thread_current ()->files); e != list_end (&thread_current ()->files); e = list_next (e))
    {
      fdesc = list_entry (e, struct file_descriptor, elem);
      if (fdesc->fd == fd)
        break;
    }
  if (fdesc == NULL)
    return -1;
  lock_acquire (&fdesc->file_lock);
  unsigned position = file_tell (fdesc->file);
  lock_release (&fdesc->file_lock);
  return position;
}

void
close (int fd)
{
  struct file_descriptor *fdesc = NULL;
  struct list_elem *e;
  for (e = list_begin (&thread_current ()->files); e != list_end (&thread_current ()->files); e = list_next (e))
    {
      fdesc = list_entry (e, struct file_descriptor, elem);
      if (fdesc->fd == fd)
        break;
    }
  if (fdesc == NULL)
    return;
  lock_acquire (&file_lock);
  file_close (fdesc->file);
  lock_release (&file_lock);
  list_remove (&fdesc->elem);
  free (fdesc);
}