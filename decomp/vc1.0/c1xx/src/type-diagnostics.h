#ifndef TYPE_DIAGNOSTICS_H
#define TYPE_DIAGNOSTICS_H
#include <stddef.h>
#include <recon.h>

typedef struct TypeDiagnosticSlot {
    short handle;
    unsigned short reserved02;
    char *buffer;
    unsigned short left, right, capacity, reserved0e;
} TypeDiagnosticSlot;
/* Exhausting all 32 slots indexes -1. That prefix aliases the existing
   forced-include state; its remaining words are the slot's boundary fields. */
typedef struct TypeDiagnosticControl {
    short forced_index;
    unsigned short reserved02;
    char *source;
    char *version_text;
    int fat_names;
} TypeDiagnosticControl;
typedef union TypeDiagnosticStorage {
    TypeDiagnosticSlot slots[33];
    struct {
        TypeDiagnosticControl control;
        TypeDiagnosticSlot strings[32];
    } data;
} TypeDiagnosticStorage;
extern TypeDiagnosticStorage type_diagnostic_storage;
#define type_diagnostic_slots (type_diagnostic_storage.slots + 1)
#define forced_source_index (type_diagnostic_storage.data.control.forced_index)
#define current_forced_source (type_diagnostic_storage.data.control.source)
#define version_directive_text (type_diagnostic_storage.data.control.version_text)
#define fat_filenames (type_diagnostic_storage.data.control.fat_names)
typedef char VersionDirectiveOffsetMustBe8[(offsetof(TypeDiagnosticControl, version_text) == 8) ? 1 : -1];
typedef char FatFilenamesOffsetMustBe12[(offsetof(TypeDiagnosticControl, fat_names) == 12) ? 1 : -1];
extern unsigned next_type_diagnostic_handle;
void __stdcall append_type_diagnostic(short text, char *value);
typedef char TypeDiagnosticSlotSizeMustBe16[(sizeof(TypeDiagnosticSlot) == 16) ? 1 : -1];
typedef char TypeDiagnosticBufferOffsetMustBe4[(offsetof(TypeDiagnosticSlot, buffer) == 4) ? 1 : -1];
typedef char TypeDiagnosticLeftOffsetMustBe8[(offsetof(TypeDiagnosticSlot, left) == 8) ? 1 : -1];
typedef char TypeDiagnosticRightOffsetMustBe10[(offsetof(TypeDiagnosticSlot, right) == 10) ? 1 : -1];
typedef char TypeDiagnosticCapacityOffsetMustBe12[(offsetof(TypeDiagnosticSlot, capacity) == 12) ? 1 : -1];
typedef char TypeDiagnosticControlSizeMustBe16[(sizeof(TypeDiagnosticControl) == 16) ? 1 : -1];
typedef char ForcedSourceIndexOffsetMustBe0[(offsetof(TypeDiagnosticControl, forced_index) == 0) ? 1 : -1];
typedef char CurrentForcedSourceOffsetMustBe4[(offsetof(TypeDiagnosticControl, source) == 4) ? 1 : -1];
typedef char TypeDiagnosticStorageSizeMustBe528[(sizeof(TypeDiagnosticStorage) == 528) ? 1 : -1];
#endif
