# Option action candidate

`apply_option_action` owns 219 primary bytes at `39632..3970d` and 154 cold
bytes at `72589..72623`, totaling 373 code bytes. Its eight-way table and
39-byte selector occupy exactly 71 separate data bytes at `39710..39757`.
The three preceding padding bytes remain masked. Both original and rebuilt
metadata identify the table; it has no function or code credit.

The complete source dispatches on the actual action byte: 1/5 set or clear an
integer, 22 stores a borrowed or duplicated string, 23 processes the encountered
letter-option records, 24 calls the named integer parser, 26 appends to the
named vector helper, and 27 diagnoses an already assigned string or stores it.
The full cold diagnostic/duplication paths remain in the source and graph.
Only genuine pending helpers contain INT3. No new data or fields are modeled.

Both ordinary cases execute three matches and three action entries. The first
two actions return after storing borrowed Invoke argv[2] as the IL prefix and
argv[4] as the source filename. The third action reaches the genuine
`parse_option_integer(const char *value)` trap with borrowed argv[6], `"1"`.
All six iterator calls return borrowed inputs with correct count/cursor changes,
stack cleanup and preserved nonvolatile registers. The final argc is 20 and the
cursor is Invoke root+24. Action EAX is not assigned semantic return meaning;
the actual callers discard it.

The normal case executes three authenticated public `SigForPbCb` calls: CPP,
minimal.cpp and -f. Restore skips CPP and executes the latter two with its own
actual seeds/results. Each real provider/shim entry and RET records all three
physical arguments, including the provider's mutation of its private third
slot, result, caller, errors, full original FP/native architectural FP, locale
tables/owners, private CRT vectors, target/saved state and owned 32KiB vector
storage. The caller immediately discards the private slots and consumes EAX.
Shared stateless CRC support supplies no application code credit.

The persistent observer adapts the C1 mechanism with independently established
C1XX addresses, literal, complete inputs and results. Original first/loop outer
returns are 390b3/390d1 and matcher returns 390be/390dc. Inner returns are
3957b/395bd; action returns are 39589/395cb. CRC returns are 3909f for initial
CPP, 395af for the fetched value and 39565 for the original option string.
The first legacy prefix files are aliases of the same actual physical phase;
later iterations have distinct indexed files. No API-only reference was run.

The decoder checks every complete native guard and original BSS/8520-byte
initialized/saved image at each new phase, allowing only the encountered typed
stores. Unknown initialized bytes6031 remain unmodeled and are compared only
within the original. Existing typed snapshot/reset tests remain in both cases.
Both strict current audits and complete immutable paired decoders pass; 62
normal and 60 restore focused mutations reject. They cover later ABI, ownership,
unknown-neighbor, callback, numeric-frontier, repeated API and table corruption.
Unchanged earlier controls retain their pinned matcher packet provenance.

Original captures took60.639/55.483s; native GDB observations10.891/11.629s.
Full offline validation took11.343/11.042s and focused controls8.462/8.368s.
The separate cached lifecycle fixture took13.760ms total, including2.007ms
execution and no Wine. Compile/link and GDB phase accounting are recorded in
`option-action-trial.json`; these rich observations are not fixture timings.

Independent review passed and actual normal promotion completed with
accepted_count93. Totals are89 application functions/6726 application bytes plus
historical CRT451B/import12B, or7189/751216 whole code-section bytes. The
denominator includes CRT and is not non-CRT coverage. The numeric parser and
other pending helpers remain unaccepted.

Read-only revalidation of both frozen roles invokes no binaries:

```
python3 scripts/check-option-action-history.py
```
