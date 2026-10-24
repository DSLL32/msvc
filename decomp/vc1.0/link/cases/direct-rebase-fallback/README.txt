Direct REBASE fallback relocation lifecycle

The real accepted rebase_main assigns fallback_process_relocations only when
NTDLL/LdrProcessRelocationBlock resolution fails. Actual public runs resolve
the provider. This case calls the genuine typed callback directly through the
existing external-driver seam; it makes no public CLI reach or provider-failure
claim. The callback has four stdcall formals, RET16 and no callees.

One untouched-original baseline, one masked first frontier and one fresh masked
whole-body/native pair are retained. All four cumulative calls agree on actual
formals, complete 4096-byte caller page, distinct16-byte relocation payload,
64 guard bytes, errors and strict architectural x87 state. Return-pointer owner
and offset, ESP+20 and nonvolatiles agree. The external driver keeps both
allocations live through the real CRT continuation and reports logical offsets;
no address is normalized in target output. No machine code is executed.

The phases are count0; positive18004; negative18004 on the actual preceding
page; and one type6 record returning NULL before any page write. Valid phases
exercise ABSOLUTE/HIGH/LOW/HIGHLOW/HIGHADJ with its following signed adjustment
word, and MIPS_JMPADDR. The bundled original headers and actual six-way reader
establish these encodings. The independent integer report oracle checks every
page/record/guard/error byte. Original/fresh/native reports are exactly20916B,
streams empty, exit0 and no target-created files. No output normalization,
FP/error qualifications, new CRT implementation or shared behavioral shim.

The readable C switch occupies the original204-byte body; its six-target24-byte
text table is semantic data. Twelve trailing alignment bytes are excluded.
All91 cached native entry/output cases, new output-only case, six cache controls,
17 actual-packet controls, original compiler,840 CFG graphs,820 callback routines,
514 contextual definitions and41 ECX routines pass. Source hashes before/after
the final native build/suite are equal, bound to shared READY f086d3fd2 (only
three additive SDK constants since the preceding checkpoint).

Timing is single-sample and split in timings.json. Final native+trace build
15.795s, cache1.123s,91-case process10.214s. New traced case total.208944s includes
.187529s trace preparation and4.140ms direct ELF; output-only total.010486s
includes1.916ms ELF. Capture/GDB/compiler/host audits are separate from fixtures.
Fixtures use immutable owned references and never invoke Wine or a debugger.

Current totals:1002frontiers/803application functions (759public,44direct),
16historical CRT overrides separate. Application-only extent229595/281634
(81.5225%); support-inclusive260534/281634 (92.5080%). Partial known target-body
inventory194443/195120;11655unaccepted unclassified bytes remain, not CRT by
default. Full nonCRT denominator remains unavailable. These are extent/static
instruction measures, not branch coverage.

analysis/proof.json binds canonical body/table/header/source/owner evidence;
reference/ is the untouched baseline; acceptance/ holds raw fresh states,
controllers and compact hash inventories. first-frontier.json/fresh-pair.json,
checks.json, audits.json and native-validation.json record separate gates.
Large transient build/auditor logs remain at their ordinary scratch paths and
are referenced by receipts rather than bundled with dependencies.
