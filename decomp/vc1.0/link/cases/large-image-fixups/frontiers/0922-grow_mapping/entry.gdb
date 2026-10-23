set pagination off
set confirm off
break *grow_mapping
run -link /dll /nodefaultlib /entry:E /out:large.dll /debug:full /debugtype:coff,fixup /map /order:@functions.ord /stack:0x200000,0x3000 /heap:0x100000,0x2000 large.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-mapped-grow_mapping-we3h5pgl/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-mapped-grow_mapping-we3h5pgl/native.stderr
printf "NATIVE_MATCH grow_mapping\n"
x/6wx $esp
bt 5
quit
