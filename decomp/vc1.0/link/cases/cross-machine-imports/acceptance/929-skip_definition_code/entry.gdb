set pagination off
set confirm off
break *skip_definition_code
run -lib /machine:i860 /def:cross.def /name:cross.dll /debugtype:both /out:i860.lib >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/import-archive-i860-skip_definition_code-dtyk9_m_/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/import-archive-i860-skip_definition_code-dtyk9_m_/native.stderr
printf "NATIVE_MATCH skip_definition_code\n"
x/6wx $esp
bt 5
quit
