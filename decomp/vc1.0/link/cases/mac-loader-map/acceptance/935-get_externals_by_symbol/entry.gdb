set pagination off
set confirm off
break *get_externals_by_symbol
run -link /machine:M68K /nodefaultlib /entry:main /out:loader-map.exe /debug:none /fixed /map:loader-map.map loader-map.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/mac-loader-map-get_externals_by_symbol-6162ggek/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/mac-loader-map-get_externals_by_symbol-6162ggek/native.stderr
printf "NATIVE_MATCH get_externals_by_symbol\n"
x/6wx $esp
bt 5
quit
