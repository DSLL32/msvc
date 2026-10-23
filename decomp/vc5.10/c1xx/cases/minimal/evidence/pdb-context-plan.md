# First PDB context

`3894f..3897a` returns cdecl `int` zero. If `restore_mode` at `cb06c`
is zero, it clears six members at `cb030 + {0,4,8,12,20,24}` in that
instruction order (20/24 precede 12). Otherwise it preserves them.
These members are outside both datafe snapshot and BSS reset regions.
The native object projects 24 bytes; original offset +16 remains unmodeled.

Roles are grounded in this C1XX image, independently of the C1 counterpart:

- +0 PDB*: receiver `3bac2` passes itself as PDB::OpenEx output through
  `3bb2f`; `3bbb7` calls IAT `c921c`, and `3bafc` uses the result as PDB.
- +4 NameMap*: selected context `cb000` supplies output address `this+4`
  to imported NameMap::open (`3bc3a..3bc43`, IAT `c9224`).
- +8 TPI/types*: `829d8` loads `cb038` and passes it to imported
  TypesQueryTiForCVRecordEx (IAT `c9228`). `a0aca` also opens through
  the PDB virtual method into `this+8`.
- +12 MREngine*: `3bc88` takes context in ECX, `3bcb7` passes `this+12`
  to MREngine::FOpen (IAT `c922c`). Its separate +16 output is unmodeled.
- +20 opaque owned PdbStreamState*: context method `a0ca6` passes
  `this+20` to `a0d8d`, which allocates 16 bytes, constructs that object,
  opens a PDB stream through the PDB virtual method and returns the
  owned pointer (`a0dc7..a0dcd`). Context method `a0b0d` receives the
  actual `cb030` context from `82929..8292e`, passes this member as ECX
  to `a1683`, and clears the pointer. Pointee layout is not modeled.
- +24 int read_mode: context method `3bb2f` sets zero at `3bb48`; cold
  `73474..7348a` tests the mode with strchr and stores one when found.

Normal masked capture must reach callback23 after registering the actual
PDB destructor. Explicit auxiliary pairs seed five never-dereferenced
owned-snapshot interior pointer identities and read_mode=37 before BSS
reset, then select reuse=0/1 at the real PDB constructor entry. They prove
persistent state survives BSS reset, reset/preserve branches, cleanup identity,
zero return, exact storage map and untouched original +16 gap. Auxiliary
cases do not authorize acceptance; the natural pair alone does that.
