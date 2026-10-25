# WPP386 10.0a

Fresh standardized scaffold, with no accepted application function yet.
`make -j4 native cases` builds the native i386 `libbinary.so` and one flat
`cases/0000-help` module. `make -j4 win32` checks the same App sources with original
Watcom through the shared launcher. No project-specific Wine wrapper remains.

The initial public case is bare `?`, historically proven to display real help.
Its fresh shared-backend oracle is still pending; old references remain in Git
history and are not relabelled as standardized capture evidence.

The first App frontier is private compiler `MemAlloc@0x81ba3`, size 8, called from
the CRT's FILE initializer before `main@0x43fef`. The native harness models the
actual three initializer records and local 26-byte FILE records, and consumes
allocator returns as genuine 8-byte stream-link nodes. It does not overlay the
host stdio structure or call an allocator just to manufacture a trace entry.
Later unqualified initialization/cleanup paths fail closed.

WPP's CF identity/controller qualification is being coordinated with the shared
tool owner. WCC's qualification is not inherited. Until the WPP geometry,
original-only support catalog and owned first-frontier state are reviewed, no
original capture, application acceptance or native replay pass is claimed.
Future work stops after this one help case completes.

The finite proposal is in `harness/startup-witness.json`; `cf-identity.json`
binds the original container and loader, and `help-inputs.json` specifies inputs
only. The proposed WPP protocol remains blocked by shared validation until its
separate review. Neither file supplies an output oracle or instruction admission.
