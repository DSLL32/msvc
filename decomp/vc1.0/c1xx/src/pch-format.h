/* Logical on-disk PCH header, recovered at the reader's exact56-byte read. */
#ifndef PCH_FORMAT_H
#define PCH_FORMAT_H
#include <stddef.h>
#include <recon.h>
typedef struct PchHeader {
    char signature[8];
    unsigned format;
    unsigned symbol_bytes, global_bytes, expression_bytes, line_bytes;
    unsigned debug_bytes, browser_bytes;
    unsigned event_bytes, event_offset;
    long browser_offset;
    int page_count;
    unsigned reserved;
} PchHeader;
extern PchHeader pch_header; /* original0xa2310..0xa2348 */
/* These existing global names are fields of the same original object. */
#define pch_symbol_bytes (pch_header.symbol_bytes)
#define pch_expression_bytes (pch_header.expression_bytes)
#define browser_saved_position (pch_header.browser_offset)
#define permanent_page_count (pch_header.page_count)
typedef char PchHeaderSizeMustBe56[(sizeof(PchHeader) == 56) ? 1 : -1];
typedef char PchHeaderSymbolOffsetMustBe12[(offsetof(PchHeader, symbol_bytes) == 12) ? 1 : -1];
typedef char PchHeaderExpressionOffsetMustBe20[(offsetof(PchHeader, expression_bytes) == 20) ? 1 : -1];
typedef char PchHeaderBrowserOffsetMustBe44[(offsetof(PchHeader, browser_offset) == 44) ? 1 : -1];
typedef char PchHeaderPageOffsetMustBe48[(offsetof(PchHeader, page_count) == 48) ? 1 : -1];
/* The writer/reader transfer this exact1496-byte view. The149 root cells
   are proved by original6a836..6a853; relocation helpers marshal them from/to
   the canonical globals, rather than giving those globals duplicate owners. */
typedef struct PchCompilerImage {
    unsigned roots[149];
    unsigned reserved_after_roots, relocation_count, permanent_available;
    unsigned operator_count, function_directory_index, reserved_count;
    int plmn_active, plmf_active, member_full_generality;
    int member_generality_fixed, member_requested_model;
    unsigned next_symbol_index, browser_symbol_limit;
    unsigned anonymous_identifier_count, reserved_zero, hidden_frame_bytes;
    unsigned short next_debug_index, reserved_debug;
    unsigned pragma_record_number, debug_stream_offset;
    int vtordisp;
    unsigned char warning_levels[800];
    unsigned lowest_warning, highest_warning, source_file_counter;
    unsigned token_flags, parser_flags;
} PchCompilerImage;
extern PchCompilerImage pch_compiler_image;
typedef char PchCompilerImageSizeMustBe1496[(sizeof(PchCompilerImage) == 1496) ? 1 : -1];
typedef char PchCompilerImageWarningsOffsetMustBe676[(offsetof(PchCompilerImage, warning_levels) == 676) ? 1 : -1];
#endif
