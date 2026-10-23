# System information boundary

The sole new application function is the one-byte RET at12998. Its actual entry,
caller return, empty formal inventory and unchanged state are observed separately
before GetSystemInfo; a later helper snapshot cannot substitute for this entry.
Both runs next reach the genuine named arena initializer12999.

GetSystemInfo writes the full36-byte SYSTEM_INFO at original5f508. Its original
KERNEL32 provider, resolved IAT address, parameter identity, stdcall4 cleanup,
LastError0 and x87 control027f are captured and checked. The exact output SHA256 is
f249873a246e47fe0690bcb26673eac7d0a33c622e6ed0cc42487094160c9b57.
Both normal and distinct live/saved-state runs change only these36 bytes; all
other original data/BSS/snapshot bytes and modeled native neighbors remain
unchanged. The unmodeled original four-byte gap following SYSTEM_INFO stays
unchanged and acquires no layout or equivalence claim.

Shared support4bd4606d supplies a separately tagged RSI1 profile:64 ASCII bytes of
actual reference-receipt SHA plus the36 API bytes, preceded by its four-byte tag.
The host verifies target, canonical CRT, actual environment/provider, output and
complete reference closure before loading this104-byte response. Its single API
inventory goes0→1. GlobalMemoryStatus remains denied: no MEMORYSTATUS was supplied.
This is host response transport and earns no application code credit.

The actual original captures are unchanged. Consuming the new shared provider
required only a native rebuild and both native recaptures. Historical original
receipts retain the then-current native hash honestly. The actual compiled-source
content manifest and immutable ELF bind the replacement native execution; the
profile encoder and factory have separate content snapshots. Checker-only fixes
do not manufacture another original run.

Both full positive validations and strict58-source/2878-byte ownership audits
pass. Fresh focused38 controls per run reject altered empty-entry identity,
argument/formal/stack records, API before/after bytes, errors/x87, provider, profile
count/tag/reference/payload, binding closure and neighboring state. Prior parser,
core and allocation guards are reused through frozen contract/result/controller
hashes; they were not rerun for this step.

Original build1.417s, native recorded build1.903s, audit0.028s. Original/native
captures take1.920/1.462s normal and2.723/2.333s restore. Focused controls take
47.39/61.90s separately. The cached native lifecycle fixture passes in4.43ms:
setup1.55ms, execution2.23ms, checks0.24ms, excluding builds. Minimal compilation
is still in progress; the fixture does not claim its completion.
