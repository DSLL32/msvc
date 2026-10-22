/* Precompiled-header event playback and include ownership. */
#include "pch.h"
#include "preprocessor.h"

PchRecord *pch_record_cursor; /* original 0x5b568 */
short pch_include_count = -1; /* original 0x6de1c */
unsigned short pch_include_page_count; /* original 0x6de20 */
int pch_macro_changed; /* original 0x6de24 */
int pch_rebuild_on_macro_change; /* original 0x6de28 */

/* Original 0x45380..0x4549a. During PCH playback, includes and their
   macro-presence dependencies are consumed from the recorded event stream. */
int __stdcall pch_skip_include(char *filename)
{
    short previous_count;
    switch (pch_mode) {
    case 0:
        return 0;
    case 1:
        previous_count = pch_include_count--;
        if (!previous_count) finish_pch_capture();
        else pch_record_string(2, permanent_page_count, filename);
        if (options.pch_auto_file && compare_filenames(options.pch_auto_file, filename))
            pch_include_count = 0;
        return 0;
    case 2:
        if (pch_record_cursor->kind != 2 ||
            !compare_filenames((char *)(pch_record_cursor + 1), filename))
            return pch_replay_mismatch();
        ++pch_include_count;
        pch_include_page_count = (unsigned short)pch_record_cursor->value;
        for (;;) {
            pch_record_cursor = (PchRecord *)((char *)(pch_record_cursor + 1) +
                                            pch_record_cursor->payload_bytes);
            if (pch_record_cursor->kind != 5) return 1;
            if (pch_macro_defined((char *)(pch_record_cursor + 1)) != pch_record_cursor->value) {
                pch_macro_changed = 1;
                if (pch_rebuild_on_macro_change) return pch_replay_mismatch();
            }
        }
    }
    return pch_replay_mismatch();
}
