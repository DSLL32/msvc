set pagination off
set confirm off
break *replace_extension
run -link /dll /nodefaultlib /entry:E /out:large.dll /debug:full /debugtype:coff,fixup /map /order:@functions.ord /stack:0x200000,0x3000 /heap:0x100000,0x2000 large.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-mapped-replace_extension-rmq4c9yf/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-mapped-replace_extension-rmq4c9yf/native.stderr
printf "NATIVE_MATCH replace_extension\n"
x/6wx $esp
bt 5
quit
