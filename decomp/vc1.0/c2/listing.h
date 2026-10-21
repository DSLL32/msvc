#ifndef LISTING_H
#define LISTING_H
#include "segments.h"
extern int listing_enabled;
extern unsigned listing_column;
extern Segment *listed_segment;
void __fastcall write_segment_listing(Segment *segment, int definition);
int __fastcall close_segment_listing(Segment *segment);
char *__fastcall listing_segment_name(Segment *segment);
#endif
