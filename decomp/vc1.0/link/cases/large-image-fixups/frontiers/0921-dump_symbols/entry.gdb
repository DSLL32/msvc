set pagination off
set confirm off
break *dump_symbols
run -link /dll /nodefaultlib /entry:E /out:large.dll /debug:full /debugtype:coff,fixup /map /order:@functions.ord /stack:0x200000,0x3000 /heap:0x100000,0x2000 large.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-mapped-dump_symbols-w8hirsov/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-mapped-dump_symbols-w8hirsov/native.stderr
printf "NATIVE_MATCH dump_symbols\n"
x/6wx $esp
bt 5
quit
