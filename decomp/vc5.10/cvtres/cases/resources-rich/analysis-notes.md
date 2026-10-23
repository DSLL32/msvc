# Resource conversion analysis

The x86 resource case is complete: fourteen application functions, 5,614 extent
bytes, exact canonical COFF output. Unreached CE and error paths remain
unaccepted; declarations and analysis alone do not count as acceptance.

The x86 invocation is `/nologo /machine:ix86 /readonly /out:rich.obj rich.res`.
Its output is deterministic: three COFF sections, zero timestamp, no normalization.
The sections are `.debug$S`, `.rsrc$01` and `.rsrc$02`; resource data is aligned to
8 bytes normally and 4 bytes with `/windowsce`. `/readonly` suppresses
`IMAGE_SCN_MEM_WRITE` in the resource-section headers; it does not modify input.

`seek_file` at `01001af6..01001b30` has stdcall cleanup12. It calls `fseek`,
tests specifically for -1, reports error1105/exits on that condition, then calls
the application tell wrapper at `01001ac9..01001af6` (cleanup4). The tell wrapper
returns `ftell`, reporting error1104/exiting if it returns -1.

Other directly evidenced utility boundaries are allocation `01001a4c..01001a7a`
(cleanup4, `calloc(1,size)`), nullable free `01001a7a..01001a91` (cleanup4,
returns0), exact byte read `01001a91..01001ac9` (cleanup12), exact byte write
`01001b30..01001b68` (cleanup12), and 1024-byte buffered copy
`01001b68..01001bb5` (cleanup12). Read/write return0 after an exact operation;
short operations report1103/1106 and exit. Copy uses the mutable BSS buffer at
`01004140`, reads/writes through these wrappers and returns the copied count.

The resource header allocated during parsing is24 bytes: DWORD data size/header
size/data version; WORD memory flags/language; DWORD version/characteristics.
Identifier reading `01001c39..01001d49` (cleanup4) distinguishes ordinalFFFF+WORD
from UTF16 names and interns names in a linked list. Its identifier record has
kind at0, an ordinal/link union at4, string offset at8, encoded byte count at12,
character count at14, UTF16 characters at16. Ordinal allocation is20 bytes;
named allocation is20+2*character-count. The parser reads a local256-WCHAR buffer
without an explicit bound check; retain the original logic when reconstructed.

The resource insertion routine `01001d49..01001fdf` has cleanup12. Its initial
comparison reads the union's low WORD even for named identifiers before falling
back to `wcscmp`; do not silently simplify that branch away. Type nodes are24
bytes, language/resource nodes32 bytes. Parsing skips ordinal type17
(DLGINCLUDE), explaining18 emitted entries from19 non-null input records.

Directory initialization `010030b4..010030e9` has cleanup24 and returns its
first pointer. It assigns characteristics, packed major/minor version, timestamp,
named and ID counts. Data-entry initialization `010030e9..0100310a` has
cleanup20 and likewise returns its pointer after assigning four DWORDs.

The converter's zero-fill pointer at `01004548` can be passed with lengths up to7.
The name-string byte counter is at `0100454e`, so that adjacency must be modeled
explicitly if the full function's padding logic reads it. Do not assume every
byte beyond the first DWORD is an independent constant zero.

The queued ARM/WindowsCE reference is separate. It genuinely transforms bitmap
2/104/1033 from232 bytes/24bpp to88 bytes/2bpp, preserving all18 resource identities.
It reaches the pixel/color conversion region skipped by ordinary x86. No CE
function is accepted merely from this original-only observation.
