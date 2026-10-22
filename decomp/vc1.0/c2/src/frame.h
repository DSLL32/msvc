#ifndef FRAME_H
#define FRAME_H
#include <recon.h>
void reset_stack_frame(void);
void initialize_stack_alignment(void);
unsigned function_stack_alignment(void);
extern unsigned stack_frame_extent;
extern int local_stack_offset;
extern unsigned parameter_stack_offset;
extern int dynamic_stack_allocation;
unsigned calculate_stack_frame_size(void);
long __fastcall reserve_local_storage(unsigned bytes);
unsigned __fastcall align_stack_storage(unsigned offset, unsigned bytes);
#endif
