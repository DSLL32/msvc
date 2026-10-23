Public cancellation transport preflight -- HOLD

Root approved actual public CTRL_BREAK for an isolated owned console/process
group; helper_main is omitted. No internal registered-callback dispatch fallback
is authorized. Native acceptance is separately held: the existing SIGQUIT shared
path calls the callback on the interrupted thread, while a real Windows control
handler uses a new thread. Relevant context/TLS/errors/FP and interruption safety
must be reviewed with the shared maintainer before choosing a bounded transport.

One explicit original-only preflight ran against the existing frozen valid
large-image COFF object and ordinary /dll /nodefaultlib /entry:E command. The
helper privately allocates a console; child CREATE_NEW_PROCESS_GROUP isolates
public GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT,childPID). No group0 signal,
shared process signal, sleeps, callback invocation, prefix remap or server reset.
The main thread is stopped at actual accepted pass_two1c700 after512bytes of
headers/padding have been written and one writable cancel.dll mapping is active.
SuspendThread previouscount0 fixes this boundary outside active CRT/allocator/
provider operations while public delivery may create a handler thread.

Observed: privateconsole allocation succeeds; real handler270f0/addTRUE is
registered and returnsTRUE. Mainthread412/childgroup640 is frozen with one
owned outputflags27/length512/capacity2MB. Public GenerateConsoleCtrlEvent(1,640)
returnsTRUE/Last0. The child exits1, but neither handler270f0 nor cleanup3d670
INT3 entry is observed and no newthread debug event arrives. cancel.dll remains
2MB. Therefore this does NOT prove the target cleanup function ran. API success
or exit1 alone cannot pass the preflight. All five required sample bytes in the
actual code mask are independently verified INT3 in validation-hold.json.

Exact events/streams/completeDLL/input hashes and helper/tool identities are in
preflight/1 and transport-preflight.json. The private Xserver readiness handshake
uses a pipe, not a sleep; its PID1660682/display:2 exited0 under own-process cleanup.
Unrelated existing Xservers remain untouched. External preflight3.735s; original
helper elapsed is in its receipt. No native fixture ran or was registered. No new
function/CRT implementation/acceptance/coverage credit, no second retry. Root and
shared maintainer received the packet and must review any transport adjustment.

Explicit tool: python3 scripts/preflight-cancellation.py
This is original-only analysis, NEVER a fixture dependency. Do not rerun it
without resolving the recorded transport issue under root review.
