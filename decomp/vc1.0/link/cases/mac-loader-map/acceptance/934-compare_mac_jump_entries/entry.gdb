set pagination off
set confirm off
break *compare_mac_jump_entries
run -link /machine:M68K /nodefaultlib /entry:main /out:loader-map.exe /debug:none /fixed /map:loader-map.map loader-map.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/mac-loader-map-compare_mac_jump_entries-1uwv6gk5/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/mac-loader-map-compare_mac_jump_entries-1uwv6gk5/native.stderr
printf "NATIVE_MATCH compare_mac_jump_entries\n"
x/6wx $esp
bt 5
quit
