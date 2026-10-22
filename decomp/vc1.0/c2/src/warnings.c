#include "c2.h"
#include "warnings.h"
#include <recon.h>
int warning_override_count;
/* Original [000592f0,00059350). A null pointer leaves prior state intact. */
void __fastcall apply_warning_state(const SavedWarning *state)
{
    if (state) {
        warning_override_count = 0;
        while (state->number != -1) {
            set_warning_override(state->number, state->level);
            ++state;
        }
    }
}
