# Arena reset and repeated initialization

`CompilerArena::Reset` owns17 bytes12ab2..12ac3. It sets cursor and committed
limit to the owned reservation base, and clears committed bytes and reset_value.
The only found direct caller overwrites EAX with1 after the call, so the source
uses a void member and claims no meaningful Reset EAX result. Its real receiver,
sole formal, RET0, entry/return stack and complete state are checked independently.

Both fresh pairs now reach `initialize_default_pools`12ac3 after all six actual
arena initializations. The proof retains every member/scaler/API/Reset entry and
return in order, not just first-encounter trace entries. It checks60 native machine
FP stages, six exact signed64 conversions and rounded sizes, every provider call
and inaccessible reservation, receiver identity, return value and full state.
The restore auxiliary independently seeds all six Reset inputs with nonzero
cursor/limit pointers into their owned reservations and nonzero committed/reset
values. Only the four genuine Reset fields change; neighbors and saved state
remain intact. No reserved contents, highest-address ordering, or regular-reserve
64KiB alignment is claimed.

The new required capture also records the actual canonical HOST CRT vectors:
3 arguments and78 environment entries, distinct from Invoke's11 arguments.
The native host binds their complete ordered bytes using the existing shared
API. The real getter records those bound arrays, their owned strings, and its
NULL result. All observed array/string identities and disjointness are checked;
raw vectors remain private scratch artifacts. Earlier cases still prove only
their finite MSC_CMD_FLAGS absence contract; their evidence was not relabeled.

Both complete paired positives and strict63-function audits pass:3202 code
bytes plus16 switch-data bytes. Fresh49 Reset state/ABI,10 repeated inventory,
10 namespace and2 last-scaler mutation controls pass in3.41s. Unchanged protocol
and scaler campaigns are explicitly reused with their frozen hashes. A separate
45ms real-Make control verifies that a source-race rejection removes its generated
executable, preventing a later no-op from falsely certifying it.

The initial original target/helper build took1.556s; the added observer helper
build took1.761s. One raced native build and a subsequent no-op retry were
discarded before execution; the verified forced native link took1.470s. Current
audit takes0.032s. Original/native captures take3.673/6.898s normal and3.424/8.263s
restore. Native lifecycle fixtures pass in4.24ms excluding build. Evidence and
separate fresh/reused accounting are in `evidence/arena-reset-{normal,clear-aux}`.

Reset is accepted under the released continuation protocol. Minimal compilation
is still in progress. Initialized state remains64 known bytes and6484 UNMODELED
bytes; new semantic consumers must be modeled before execution.
