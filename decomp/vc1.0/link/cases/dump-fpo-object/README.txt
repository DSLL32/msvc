FPO and symbol-map object route

The unmodified original compiler hello-debug.obj from the shared debug-DLL fixture
is frozen as debug.obj. Two explicit untouched-original-only captures of
-dump /fpo /map /out:dump.txt debug.obj agree exactly:1043 output bytes, streams
and exit0; no normalization. This exercises multiple .debug$F COMDAT sections and
the locator's no-optional-header branch. No new function frontier is required
beyond dump-fpo-map. A fresh masked-original/native GDB terminal pair and cached
native empty-new-entry trace both pass. See validation.json.
