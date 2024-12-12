#include "vm/swap.h"

#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "lib/kernel/bitmap.h" 

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)
#define SWAP_FREE 0
#define SWAP_IN_USE 1

static struct block *swap_block;
static struct bitmap *swap_map;
static struct lock swap_lock;

void initialize_swap(void){
  swap_block = block_get_role(3);
  if(swap_block == NULL){
    PANIC("No swap block device found");
  }
  swap_map = bitmap_create(block_size(swap_block) / SECTORS_PER_PAGE);
  if(swap_map == NULL){
    PANIC("Failed to create swap bitmap");
  }
  bitmap_set_all(swap_map, SWAP_FREE);
  lock_init(&swap_lock);
}

size_t swap_out(void *frame){
  lock_acquire(&swap_lock);
  size_t swap_index = bitmap_scan_and_flip(swap_map, 0, 1, SWAP_FREE);

  if(swap_index == BITMAP_ERROR){
    PANIC("Swap partition is full");
  }
  for(size_t i = 0; i < SECTORS_PER_PAGE; i++){
    block_write(swap_block, swap_index * SECTORS_PER_PAGE + i, frame + i * BLOCK_SECTOR_SIZE);
  }
  lock_release(&swap_lock);
  return swap_index;
}

void swap_in(size_t swap_index, void *frame){
  lock_acquire(&swap_lock);
  if(bitmap_test(swap_map, swap_index) == SWAP_FREE){
    PANIC("Trying to swap in a free swap slot");
  }
  for(size_t i = 0; i < SECTORS_PER_PAGE; i++){
    block_read(swap_block, swap_index * SECTORS_PER_PAGE + i, frame + i * BLOCK_SECTOR_SIZE);
  }
  bitmap_flip(swap_map, swap_index);
  lock_release(&swap_lock);
}

void swap_free(size_t swap_index){
  lock_acquire(&swap_lock);
  if(bitmap_test(swap_map, swap_index) == SWAP_FREE){
    PANIC("Trying to free a free swap slot");
  }
  bitmap_flip(swap_map, swap_index);
  lock_release(&swap_lock);
}