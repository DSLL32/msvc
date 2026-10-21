#ifndef WARNINGS_H
#define WARNINGS_H
/* Saved warning state is a packed sequence of number/level pairs, terminated
   by a number of -1 without a following level byte. */
#pragma pack(1)
typedef struct SavedWarning {
    short number;
    signed char level;
} SavedWarning;
#pragma pack()
extern int warning_override_count;
void __fastcall apply_warning_state(const SavedWarning *state);
void __fastcall set_warning_override(short number, signed char level);
#endif
