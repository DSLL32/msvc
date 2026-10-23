# Early integer switch parser — review packet

Candidate process_early_option owns132 hot bytes38d56..38dda and84 cold bytes
720fc..72150. The distinct16-byte four-DWORD jump table38de0..38df0 is record.shared
plus original.text_data, not code_chunks. Exact targets are38d70/72116/38dc6/7212a.
The preceding six alignment bytes remain masked. Common strict Rust audit reports
216candidate code bytes and3288whole-trial code bytes, excluding16data bytes.

Own mask extension is two small loops: prepare-trial appends each shared interval
with kind=text_data,entry0; existing debugger restores every interval from its
actual relocated loaded snapshot. Immutable config, candidate and Rust consume
the exact same owned16bytes. Full751216-byte mask/readback comparison still checks
every byte; direct table checks bind shape/targets/file backing and padding.
accept.py records code_chunks excluding text_data and separate shared_text_data
with zero code credit. Shared ranges never become entry roots.

Source is an ordinary switch/state loop with three character prefix comparisons
for -Bm, -ZB and -Zm, atoi at the public CRT boundary, and -Zm zero fallback100.
No reconstruction data table is copied. Actual loaded defaults for three int
fields are0/0/100 atcd09c/cd4ac/cdc28. All12bytes join native typed snapshot/restore;
nonzero live43/-47/53 and saved31/-37/41 prove separation. Initialized149semantic
bytes+51native record padding=200native bytes;8371original bytes stay unmodeled.

Every one of25 real argv parser calls is observed at entry and its actual stack
return, with all formals, owned argv pointer, state0/result0, RET0, EDI/ESI/EBX,
LastError203 andCW027f. Native observations use real GDB breakpoints; original
one-shot caller breakpoint restores the saved instruction, with no forced return.
All inputs are the captured minimal case; no invented -Bm/-Zm auxiliary calls.
Only actual -ZB64 changes a live setting. Unchanged full original BSS, full raw
initialized image except that4-byte store, saved8520-byte snapshot and every
modeled native neighbor constrain the final named void hook frontier.

The proof packet early-switches-trials.json contains all three immutable pairs,
current source/controller/checker hashes, affected controls and accounting.
Natural original7.48s/native GDB4.19s; restore originals7.83/8.14s and natives
4.02/4.51s. These are debugger observation costs, not fixture execution times.
Three direct common Rust controls reject missing/wrong data ownership and a
crossing code/data declaration. Ten direct exact-table controls reject range,
entry/root, owner, target, snapshot/readback and padding changes; remaining fresh
controls cover actual parser ABI/settings and each live/saved/restored int.

Promotion is HELD pending the orchestrator's first text-data transport review.
No new application coverage has been granted by this packet.
