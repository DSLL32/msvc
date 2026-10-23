set pagination off
set confirm off
run -link /nodefaultlib /force /entry:entry /subsystem:console /absolutesfrom:values.obj /out:weak.exe /map:weak.map /debug:none weak.obj > /root/.ai-tmp/vc1.0-link/absolute-weak-force/final-native.stdout 2> /root/.ai-tmp/vc1.0-link/absolute-weak-force/final-native.stderr
quit
