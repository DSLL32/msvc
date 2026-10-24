#include "directives.h"
#include <recon.h>

/* Output cursor saved by macro expansion; no prior expansion has written it. */
WCHAR *rc_macro_output_cursor=rc_string_buffer;
