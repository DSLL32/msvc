#ifndef COFF_SEGMENTS_H
#define COFF_SEGMENTS_H
#include "segments.h"
unsigned short __fastcall coff_declare_segment(Segment *segment);
typedef struct CoffSegmentLink {
    struct CoffSegmentLink *next;
    Segment *segment;
} CoffSegmentLink;
extern CoffSegmentLink *coff_segment_index[16];
void __fastcall register_coff_segment(Segment *segment);
#endif
