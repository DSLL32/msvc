set pagination off
set confirm off
break *resource_error_prefix
run -link /nodefaultlib /entry:main /subsystem:console /debug:none /out:identity.exe test.obj identity.res >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/resource-identity-duplicate-resource_error_prefix-kpd7xj30/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/resource-identity-duplicate-resource_error_prefix-kpd7xj30/native.stderr
printf "NATIVE_MATCH resource_error_prefix\n"
x/6wx $esp
bt 5
quit
