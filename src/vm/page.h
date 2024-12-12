#ifndef VM_PAGE_H
#define VM_PAGE_H
#include <stdbool.h>
#include <list.h>
#include "filesys/file.h"
#include "filesys/off_t.h"
struct sup_page_table_entry {
    void *vaddr;
    void *upage;
    bool writable;
    bool dirty;
    bool accessed;
    bool swapped;
    bool mmaped;
    int swap_index;
    struct file *file;
    off_t offset;
    size_t read_bytes;
    size_t zero_bytes;
    struct list_elem elem;
};

void add_spte(struct file *file, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, uint8_t *upage, bool writable);
struct sup_page_table_entry *get_spte_by_vaddr(uint8_t *vaddr);
void destroy_spte(struct sup_page_table_entry *spte);


#endif