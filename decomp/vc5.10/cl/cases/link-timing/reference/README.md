This is the completed original timing case and its immutable cached oracle.
`fixture.json` binds streams, entry trace, input, module/resource identities,
original receipt, raw inventory, typed packet and decoder provenance. The 14
original decoder source snapshots are historical identities, independent of
later worktree changes. `prepare-clock-packet.py` is an explicit offline capture
preparation command and is never invoked by regression fixtures.

The cached Linux fixture copies only needed child/provider inputs and output,
validates the actual invocation/effects/state, requires exactly two configured
clock calls at real exit, and checks status/streams/trace/artifacts. Fresh
acceptance separately decodes authenticated raw and checks full addressed GDB
state. Cached mode is explicitly rejected by that fresh path.
