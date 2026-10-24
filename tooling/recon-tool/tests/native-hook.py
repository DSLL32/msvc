#!/usr/bin/env python3
"""Exercise one shared hook instance through a real i386 dlopened DSO."""
from pathlib import Path
import subprocess
import tempfile
import time

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='recon-native-hook-', dir=Path.home()/'.ai-tmp') as directory:
    path = Path(directory)
    started = time.perf_counter()
    subprocess.run(['gcc', '-m32', '-fPIC', '-O0', '-g', '-I'+str(root/'include'),
                    '-c', root/'native/entry-hook.c', '-o', path/'hook.o'], check=True)
    subprocess.run(['gcc', '-m32', '-shared', '-fPIC', '-O0', '-g', '-finstrument-functions',
                    '-Wl,-Bsymbolic-functions', '-Dmain=hook_test_main', '-I'+str(root/'include'),
                    root/'tests/native-hook.c', path/'hook.o', '-o', path/'test.so'], check=True)
    source = path/'load.c'
    source.write_text('''#include <dlfcn.h>
#include <assert.h>
int main(int argc,char **argv) {
    void *module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    int (*test)(void);
    (void)argc;assert(module);test=dlsym(module,"hook_test_main");assert(test);
    return test();
}
''')
    subprocess.run(['gcc', '-m32', source, '-ldl', '-o', path/'load'], check=True)
    run = time.perf_counter()
    result = subprocess.run([path/'load', path/'test.so'], stderr=subprocess.PIPE, text=True)
    assert result.returncode == 0, result.stderr
    assert result.stderr == 'recon-tool: native entry hook failed\n', result.stderr
    print('native entry hook PASS: build %.3fs, execution %.3fs' %
          (run-started, time.perf_counter()-run))
