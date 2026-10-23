#ifndef CODEVIEW_TYPES_H
#define CODEVIEW_TYPES_H
#include <recon.h>
/* Primitive type identifiers shared by embedded and database CodeView. */
enum CodeViewPrimitive {
    CV_CHAR = 0x10, CV_SHORT = 0x11, CV_LONG = 0x12, CV_QUAD = 0x13,
    CV_UCHAR = 0x20, CV_USHORT = 0x21, CV_ULONG = 0x22, CV_UQUAD = 0x23,
    CV_REAL32 = 0x40, CV_REAL64 = 0x41, CV_REAL80 = 0x42,
    CV_RCHAR = 0x70, CV_INT2 = 0x72, CV_UINT2 = 0x73,
    CV_INT4 = 0x74, CV_UINT4 = 0x75, CV_INT8 = 0x76, CV_UINT8 = 0x77
};
#endif
