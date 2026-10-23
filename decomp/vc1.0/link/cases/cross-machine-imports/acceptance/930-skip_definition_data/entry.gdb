set pagination off
set confirm off
break *skip_definition_data
run -lib /machine:i860 /def:cross.def /name:cross.dll /debugtype:both /out:i860.lib >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/import-archive-i860-skip_definition_data-z86k3lpd/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/import-archive-i860-skip_definition_data-z86k3lpd/native.stderr
printf "NATIVE_MATCH skip_definition_data\n"
x/6wx $esp
bt 5
quit
