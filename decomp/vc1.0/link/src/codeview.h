#ifndef LINK_CODEVIEW_H
#define LINK_CODEVIEW_H
#include "pass-two.h"
#include <recon.h>
typedef struct CvModuleSegment {
    Group *group;
    Contribution *first,*last;
    struct CvModuleSegment *next;
} CvModuleSegment;
typedef struct CvModuleHeader {
    unsigned short overlay,library,segment_count,style;
} CvModuleHeader;
typedef struct CvSegment {
    unsigned short section,flags;
    unsigned long offset,size;
} CvSegment;
typedef struct CvSegmentDescriptor {
    unsigned short flags,overlay,group,frame,segment_name,class_name;
    unsigned long offset,length;
} CvSegmentDescriptor;
typedef struct CvDirectoryHeader {
    unsigned short header_size,entry_size;
    unsigned long count,next,flags;
} CvDirectoryHeader;
typedef struct CvDirectoryEntry {
    unsigned short kind,module;
    unsigned long offset,size;
} CvDirectoryEntry;
CvModuleSegment *collect_module_segments(Module *,unsigned short *,Image *);
void collect_module_publics(Image *);
void print_module_debug_info(void);
void emit_module_publics(Image *,ModuleDebugInfo *);
void emit_cv_public(SymbolTable *,External *,unsigned short);
#endif
