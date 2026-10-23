set pagination off
set confirm off
set exec-wrapper /usr/bin/env --argv0='R:\toolchains\watcom10.0a\.patched\WLINK.EXE'
break /root/.ai-tmp/msvc2/watcom10.0a/wlink/spawn-candidate-01/linkutil.c:48
break /root/.ai-tmp/msvc2/watcom10.0a/wlink/spawn-candidate-01/linkutil.c:49
run
set $owner = (char *)&env[0]
p sizeof(env)
p SpawnStack == $owner
p save_env == 0
p fn == DoLinkFromCommandLine
dump binary memory /root/.ai-tmp/msvc2/watcom10.0a/wlink/normal-plumbing-01/owner-before.private.bin $owner ($owner+52)
echo BEFORE-FP-BEGIN\n
info float
echo BEFORE-FP-END\n
continue
p status == 0
p (char *)&env[0] == $owner
p SpawnStack == $owner
p save_env == 0
p fn == DoLinkFromCommandLine
dump binary memory /root/.ai-tmp/msvc2/watcom10.0a/wlink/normal-plumbing-01/owner-after.private.bin $owner ($owner+52)
echo AFTER-FP-BEGIN\n
info float
echo AFTER-FP-END\n
info line *$pc
bt
quit
