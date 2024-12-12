#include <list.h>

#include "devices/block.h"

struct swap_slot {
  struct list_elem elem;
  block_sector_t sector;
};

void initialize_swap(void);
void swap_in(size_t used_index, void *frame);
size_t swap_out(void *frame);
void swap_free(size_t used_index);