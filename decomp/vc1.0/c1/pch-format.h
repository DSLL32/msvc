/* Native PCH configuration record. This is a wire layout assembled from
   semantic options, not a copy of the compiler data segment. */
#ifndef PCH_FORMAT_H
#define PCH_FORMAT_H
#include <stddef.h>
typedef struct PchSettingsImage {
    int z_E;
    int extensions_enabled;
    int option_Gr;
    int reserved_calling_convention;
    int japanese_identifiers;
    int option_J;
    int compatibility_model[6];
    int option_MD;
    int option_M;
    int option_MT;
    int option_Gc;
    int option_PLMF;
    int option_PLMN;
    int debug_info;
    int option_NoComdat;
    int option_GA;
    int option_GD;
    int option_GEf;
    int option_Gq;
    int option_GW;
    int option_Gw;
    int option_Yd;
    unsigned short object_name_bytes, browser_name_bytes;
    int option_Gn;
    int plmf_seen;
    int plmn_seen;
    int native_caller_seen;
    int option_EP;
    int option_pf;
    int skip_fault_protection;
    int option_Bu;
    int option_Brepro;
    int option_ide;
    int option_Oi;
    int option_Ol;
    int option_Oq;
    int option_Gs;
    int reserved_optimization;
    int reserved_sentinel;
    int option_Gd;
    int option_Gz;
    int default_string_pooling;
    int option_G1;
    int option_G2;
    int option_G3;
    int option_G4;
    int option_qc;
    int option_Op;
    int option_Ob;
    int option_Oy;
    int option_Oa;
    int option_Ow;
    int option_Oz;
    int browser_enabled;
    int browser_full;
    int optimization_flags;
    int thread_storage;
    int line_records;
    char database_path[256];
    char pch_object_name[256];
} PchSettingsImage;
typedef char PchSettingsImage_size[sizeof(PchSettingsImage) == 764 ? 1 : -1];

/* Expanded in the original snapshot/write functions to preserve their graph.
   index_ is an unsigned local supplied by those functions. */
extern int pch_compatibility[6];
extern int pch_optimization_reserved, pch_optimization_sentinel;
#define CAPTURE_PCH_SETTINGS(image_, source_, database_, object_, index_) do { \
    image_.z_E = source_.z_E; \
    image_.extensions_enabled = source_.extensions_enabled; \
    image_.option_Gr = source_.option_Gr; \
    image_.reserved_calling_convention = 0; \
    image_.japanese_identifiers = source_.japanese_identifiers; \
    image_.option_J = source_.option_J; \
    image_.compatibility_model[0] = pch_compatibility[0]; \
    image_.compatibility_model[1] = pch_compatibility[1]; \
    image_.compatibility_model[2] = pch_compatibility[2]; \
    image_.compatibility_model[3] = pch_compatibility[3]; \
    image_.compatibility_model[4] = pch_compatibility[4]; \
    image_.compatibility_model[5] = pch_compatibility[5]; \
    image_.option_MD = source_.option_MD; \
    image_.option_M = source_.option_M; \
    image_.option_MT = source_.option_MT; \
    image_.option_Gc = source_.option_Gc; \
    image_.option_PLMF = source_.option_PLMF; \
    image_.option_PLMN = source_.option_PLMN; \
    image_.debug_info = source_.debug_info; \
    image_.option_NoComdat = source_.option_NoComdat; \
    image_.option_GA = source_.option_GA; \
    image_.option_GD = source_.option_GD; \
    image_.option_GEf = source_.option_GEf; \
    image_.option_Gq = source_.option_Gq; \
    image_.option_GW = source_.option_GW; \
    image_.option_Gw = source_.option_Gw; \
    image_.option_Yd = source_.option_Yd; \
    image_.object_name_bytes = pch_object_name_bytes; \
    image_.browser_name_bytes = pch_browser_name_bytes; \
    image_.option_Gn = source_.option_Gn; \
    image_.plmf_seen = plmf_pragma_seen; \
    image_.plmn_seen = plmn_pragma_seen; \
    image_.native_caller_seen = native_caller_pragma_seen; \
    image_.option_EP = source_.option_EP; \
    image_.option_pf = source_.option_pf; \
    image_.skip_fault_protection = skip_fault_protection; \
    image_.option_Bu = source_.option_Bu; \
    image_.option_Brepro = source_.option_Brepro; \
    image_.option_ide = source_.option_ide; \
    image_.option_Oi = source_.option_Oi; \
    image_.option_Ol = source_.option_Ol; \
    image_.option_Oq = source_.option_Oq; \
    image_.option_Gs = source_.option_Gs; \
    image_.reserved_optimization = pch_optimization_reserved; \
    image_.reserved_sentinel = pch_optimization_sentinel; \
    image_.option_Gd = source_.option_Gd; \
    image_.option_Gz = source_.option_Gz; \
    image_.default_string_pooling = default_string_pooling; \
    image_.option_G1 = source_.option_G1; \
    image_.option_G2 = source_.option_G2; \
    image_.option_G3 = source_.option_G3; \
    image_.option_G4 = source_.option_G4; \
    image_.option_qc = source_.option_qc; \
    image_.option_Op = source_.option_Op; \
    image_.option_Ob = source_.option_Ob; \
    image_.option_Oy = source_.option_Oy; \
    image_.option_Oa = source_.option_Oa; \
    image_.option_Ow = source_.option_Ow; \
    image_.option_Oz = source_.option_Oz; \
    image_.browser_enabled = browser_enabled; \
    image_.browser_full = browser_full; \
    image_.optimization_flags = precompiled_option_flags; \
    image_.thread_storage = default_thread_storage; \
    image_.line_records = source_.line_records; \
    for (index_ = 0; index_ < 256; ++index_) { \
        image_.database_path[index_] = database_[index_]; \
        image_.pch_object_name[index_] = object_[index_]; \
    } \
} while (0)

typedef struct PchCompilerImage {
    unsigned roots[105]; /* ordered permanent-arena roots, all relocated */
    unsigned reserved_after_roots, relocation_count, available;
    unsigned reserved_string_count, function_record_index, reserved_record_count;
    int active_plmf, active_plmn;
    unsigned next_symbol_index, browser_symbol_limit;
    unsigned reserved_one, reserved_zero, symbol_offset;
    unsigned short next_debug_index, reserved_debug;
    unsigned pragma_record_number, debug_type_bytes, reserved_debug_bytes;
    unsigned char warning_levels[800];
    unsigned lowest_warning, highest_warning, source_file_counter;
    unsigned scanner_flags, parser_flags;
} PchCompilerImage;
typedef char PchCompilerImage_size[sizeof(PchCompilerImage) == 1308 ? 1 : -1];

/* Signature and stream extents precede the settings image at offset zero. */
typedef struct PchHeader {
    char signature[8];
    unsigned format;
    unsigned symbol_bytes, global_bytes, expression_bytes, line_bytes;
    unsigned debug_bytes, browser_bytes;
    unsigned event_checksum, event_offset;
    long browser_offset;
    unsigned page_count, reserved;
} PchHeader;
typedef char PchHeader_size[sizeof(PchHeader) == 56 ? 1 : -1];
typedef char PchSettings_compatibility_offset[offsetof(PchSettingsImage, compatibility_model) == 24 ? 1 : -1];
typedef char PchSettings_name_lengths_offset[offsetof(PchSettingsImage, object_name_bytes) == 108 ? 1 : -1];
typedef char PchSettings_database_offset[offsetof(PchSettingsImage, database_path) == 252 ? 1 : -1];
typedef char PchSettings_object_offset[offsetof(PchSettingsImage, pch_object_name) == 508 ? 1 : -1];
typedef char PchCompiler_warnings_offset[offsetof(PchCompilerImage, warning_levels) == 488 ? 1 : -1];
typedef char PchCompiler_source_counter_offset[offsetof(PchCompilerImage, source_file_counter) == 1296 ? 1 : -1];
typedef char PchHeader_checksum_offset[offsetof(PchHeader, event_checksum) == 36 ? 1 : -1];
typedef char PchHeader_pages_offset[offsetof(PchHeader, page_count) == 48 ? 1 : -1];
#endif
