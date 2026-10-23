set pagination off
set confirm off
break *compare_external_sections
run -link /machine:M68K /nodefaultlib /entry:main /out:loader-map.exe /debug:none /fixed /map:loader-map.map loader-map.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/mac-loader-map-compare_external_sections-3rqrr7vc/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/mac-loader-map-compare_external_sections-3rqrr7vc/native.stderr
printf "NATIVE_MATCH compare_external_sections\n"
x/6wx $esp
bt 5
quit
