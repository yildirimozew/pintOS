#include "vm/page.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include <stdbool.h>
#include <stdio.h>

void add_spte(struct file *file, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, uint8_t *upage, bool writable){
  struct sup_page_table_entry *spte = malloc(sizeof(struct sup_page_table_entry));
  spte->file = file;
  spte->offset = ofs;
  spte->read_bytes = read_bytes;
  spte->zero_bytes = zero_bytes;
  spte->upage = upage;
  spte->writable = writable;

  struct thread *cur = thread_current();
  list_push_back(&cur->sup_page_table, &spte->elem);
}

struct sup_page_table_entry *get_spte_by_vaddr(uint8_t *vaddr){
  struct thread *cur = thread_current();
  struct list_elem *e;
  for(e = list_begin(&cur->sup_page_table); e != list_end(&cur->sup_page_table); e = list_next(e)){
    struct sup_page_table_entry *spte = list_entry(e, struct sup_page_table_entry, elem);
    if(spte->upage == vaddr){
      return spte;
    }
  }
  return NULL;
}

void destroy_spte(struct sup_page_table_entry *spte){
  list_remove(&spte->elem);
  free(spte);
}