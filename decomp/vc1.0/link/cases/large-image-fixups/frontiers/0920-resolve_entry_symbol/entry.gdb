set pagination off
set confirm off
break *resolve_entry_symbol
run -link /dll /nodefaultlib /entry:E /out:large.dll /debug:full /debugtype:coff,fixup /map /order:@functions.ord /stack:0x200000,0x3000 /heap:0x100000,0x2000 large.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-mapped-resolve_entry_symbol-la6bydxx/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-mapped-resolve_entry_symbol-la6bydxx/native.stderr
printf "NATIVE_MATCH resolve_entry_symbol\n"
x/6wx $esp
bt 5
quit
