#include <list.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include <string.h>
#include "vm/page.h"


struct frame {
    bool is_allocated;
    void *phys_base;
    struct thread *thread;
    struct list_elem elem; 
    bool writable;
};

void initialize_frame_table(void);
void *allocate_frame();
void free_frame(void *phys_base);
struct frame *evict_frame(void);

static struct list frame_table;
static struct lock frame_lock;

void initialize_frame_table(void) {
    list_init(&frame_table);
    void *phys_base = palloc_get_page(PAL_USER);
    lock_init(&frame_lock);
    while (phys_base != NULL) {
        struct frame *f = malloc(sizeof(struct frame));
        f->is_allocated = false;
        f->phys_base = phys_base;
        f->thread = NULL;
        list_push_back(&frame_table, &f->elem);
        phys_base = palloc_get_page(PAL_USER);
    }
}

void *allocate_frame() {
    lock_acquire(&frame_lock);
    struct list_elem *e;
    for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
        struct frame *f = list_entry(e, struct frame, elem);
        if (!f->is_allocated) {
            f->is_allocated = true;
            f->thread = thread_current();
            lock_release(&frame_lock);
            return f->phys_base;
        }
    }
    lock_release(&frame_lock);
    return (evict_frame())->phys_base;
}

void free_frame(void *phys_base) {
    lock_acquire(&frame_lock);
    struct list_elem *e;
    for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
        struct frame *f = list_entry(e, struct frame, elem);
        if (f->phys_base == phys_base) {
            f->is_allocated = false;
            f->thread = NULL;
            memset(phys_base, 0, PGSIZE);
            lock_release(&frame_lock);
            return;
        }
    }
    lock_release(&frame_lock);
    PANIC("No frame found to free");
}


static struct frame *get_frame(void *phys_base) {
    struct list_elem *e;
    for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
        struct frame *f = list_entry(e, struct frame, elem);
        if (f->phys_base == phys_base) {
            return f;
        }
    }
    return NULL;
}

bool add_frame(void *upage, void *kpage, bool writable) {
    lock_acquire(&frame_lock);
    struct thread *t = thread_current();    
    struct frame *frame = get_frame(kpage);

    bool result = (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));

    if (result) {
        frame->is_allocated = true;
        frame->thread = t;
        frame->writable = writable;
    }

    lock_release(&frame_lock);
    return result;
}

struct frame *evict_frame(void) {
    struct list_elem *e;
    for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
        struct frame *f = list_entry(e, struct frame, elem);
        if (f->thread == thread_current()) {
            if (pagedir_is_dirty(f->thread->pagedir, f->phys_base)) {
                struct sup_page_table_entry *p = get_spte_by_vaddr(f->phys_base);
                    file_write_at(p->file, f->phys_base, PGSIZE, p->offset);
                } else {
                    swap_out(f->phys_base);
                }
            }
            pagedir_clear_page(f->thread->pagedir, f->phys_base);
            f->is_allocated = false;
            f->thread = NULL;
            return f;
        }
        PANIC("No frame found to evict");
    }