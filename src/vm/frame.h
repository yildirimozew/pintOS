#ifndef VM_FRAME_H
#define VM_FRAME_H
void initialize_frame_table(void);
void* allocate_frame(void);
void free_frame(void* frame);
bool add_frame(void *upage, void *kpage, bool writable);

#endif
