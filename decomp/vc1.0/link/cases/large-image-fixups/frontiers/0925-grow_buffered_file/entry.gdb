set pagination off
set confirm off
break *grow_buffered_file
run -link /dll /nodefaultlib /entry:E /out:extra.dll /debug:full /debugtype:coff,fixup /map /order:@extra.ord /stack:0x200000,0x3000 /heap:0x100000,0x2000 extra.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-extra-grow_buffered_file-x0oove86/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-extra-grow_buffered_file-x0oove86/native.stderr
printf "NATIVE_MATCH grow_buffered_file\n"
x/6wx $esp
bt 5
quit
