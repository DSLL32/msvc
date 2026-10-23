set pagination off
set confirm off
run -link /nodefaultlib /force /include:_absolute_value /entry:entry /subsystem:console /absolutesfrom:values.obj /out:weak.exe /map:weak.map /debug:none weak.obj > /root/.ai-tmp/vc1.0-link/absolute-weak-unmatched/final-native.stdout 2> /root/.ai-tmp/vc1.0-link/absolute-weak-unmatched/final-native.stderr
quit
