#include "files.h"
#include "types.h"
#include "image.h"
#include "pdb.h"
#include "strip.h"
#include "output.h"
#include "oldsymbols.h"
#include <recon.h>

/* Original reads an unwritten WORD at entry ESP-14 to choose storage.
   One natural two-module run observed 0 (resize) then 0x7e (reuse). The
   varying control defect remains unsupported; no native branch is invented. */
void convert_old_barray(TypeRecord *entry) { __asm { int 3 } }
