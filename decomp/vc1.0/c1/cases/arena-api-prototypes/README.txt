Legacy arena API declaration/prototype workflow
==============================================
One source exports named-based entry links, typed callbacks and pointer-to-array
row views, with an old-style void* prototype/typed object-pointer definition.
Untouched normal /Z7 /Od /G5 /W4 compile/link and checksum exit0; exact C4131
warning. Callbacks have matching C calling conventions and parameter types.

Public /Zg prints six complete prototype rows but exits1 after original C2213
for the same valid named base, followed by C4131. CL exits2. It creates no
artifacts and has no backend/runtime success claim. Original prototype mode
leaves pointer model0, while its named-base validator accepts model1/2 only.
Independent raw instructions and two matching captures prove that limitation.
No type-model repair, injected private flag, path/pointer normalization or
diagnostic suppression occurs. Prototype stdout and diagnostic stderr remain
separate; normal diagnostics retain original CL descriptor alias.

Initial __far/__loadds draft was rejected and is retained as failed preflight.
Those unsupported forms were removed before source/transport freeze. Fixtures
are native only; all original validation is explicit captured evidence.

Complete at971 entries: two whole functions,217frontend extent bytes.
All39 native phases,45 rejections, six graph/ABI/mask checks, three flag
mutations,289state/alias checks and both grammars pass. Full validation.json
binds raw captures, original limitation, entry histories and compiled inputs.
