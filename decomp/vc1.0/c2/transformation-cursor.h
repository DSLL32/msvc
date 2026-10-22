#ifndef TRANSFORMATION_CURSOR_H
#define TRANSFORMATION_CURSOR_H
#include "functions.h"
/* Original830d0 is reused after expression ownership changes to machine
   records. Each phase accesses its own pointer variant of this one slot. */
typedef union TransformationCursor {
    Expression *expression;
    struct MachineInstruction *instruction;
} TransformationCursor;
extern TransformationCursor transformation_cursor;
#endif
