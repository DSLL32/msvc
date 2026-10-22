#ifndef FRAME_H
#define FRAME_H
void reset_stack_frame(void);
void initialize_stack_alignment(void);
unsigned function_stack_alignment(void);
extern unsigned stack_frame_extent;
extern int local_stack_offset;
extern unsigned maximum_stack_alignment;
#endif
