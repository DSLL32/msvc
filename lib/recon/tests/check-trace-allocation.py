#!/usr/bin/env python3
"""Reject libc heap activity before main in real i386 tracing probes."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]

GUARD = r'''
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern void *__libc_malloc(size_t);
extern void *__libc_calloc(size_t,size_t);
extern void *__libc_realloc(void *,size_t);
extern void __libc_free(void *);
int allocations_allowed;
static void reject_allocation(void)
{
    if (!allocations_allowed && !getenv("ALLOW_TRACE_ERROR_ALLOCATION")) {
        write(2,"allocation before main\n",23);
        _exit(91);
    }
}
void *malloc(size_t n) { reject_allocation(); return __libc_malloc(n); }
void *calloc(size_t n,size_t s) { reject_allocation(); return __libc_calloc(n,s); }
void *realloc(void *p,size_t n) { reject_allocation(); return __libc_realloc(p,n); }
void free(void *p) { reject_allocation(); __libc_free(p); }
/* Prove that each interposed allocator actually rejects pre-main use. */
static void __attribute__((constructor(102))) allocation_guard_probe(void)
{
    const char *operation=getenv("PROBE_FORBIDDEN_ALLOCATION");
    void *volatile p;
    if (!operation) return;
    if (!strcmp(operation,"malloc")) p=malloc(8);
    else if (!strcmp(operation,"calloc")) p=calloc(1,8);
    else if (!strcmp(operation,"realloc")) p=realloc(NULL,8);
    else free(NULL);
}
'''

TARGET = r'''
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
extern int allocations_allowed;
static int __attribute__((noinline)) hidden(int value) { return value+1; }
int __attribute__((noinline)) alpha(int value) { return hidden(value); }
int main(void)
{
    void *p;
    allocations_allowed=1;
    p=malloc(29); if (!p) return 92;
    p=realloc(p,61); if (!p) return 93;
    free(p); p=calloc(3,7); if (!p) return 94; free(p);
    errno=EDOM;
    if (alpha(2)!=3 || alpha(3)!=4 || errno!=EDOM) return 95;
    if (write(1,"allocation probe output\n",24)!=24) return 96;
    return 0;
}
'''


def main():
    scratch = Path.home()/'.ai-tmp/recon-trace-allocation'
    scratch.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='checks-', dir=scratch))
    guard, target = directory/'guard.c', directory/'target.c'
    guard.write_text(GUARD)
    target.write_text(TARGET)
    checks, builds = [], []

    def run(executable, name, filter_data=b'main\nalpha\nhidden\n', seen_data=b'',
            expected=('main', 'alpha', 'hidden'), status=0, extra_env=None, disabled=False):
        start = time.perf_counter()
        work = directory/(executable.name+'-'+name)
        work.mkdir()
        (work/'filter').write_bytes(filter_data)
        (work/'seen').write_bytes(seen_data)
        env = {k:v for k,v in os.environ.items() if not k.startswith('RECON_TRACE_')}
        env.update(extra_env or {})
        setup_seconds = time.perf_counter()-start
        with (work/'trace').open('wb') as output:
            env.update(RECON_TRACE_FILTER=str(work/'filter'), RECON_TRACE_SEEN=str(work/'seen'))
            if not disabled:
                env['RECON_TRACE_FD'] = str(output.fileno())
            execute_start = time.perf_counter()
            process = subprocess.run([str(executable)], env=env, pass_fds=(output.fileno(),),
                                     capture_output=True, timeout=10)
            execution_seconds = time.perf_counter()-execute_start
        check_start = time.perf_counter()
        assert process.returncode == status, (name, process.returncode, process.stderr)
        actual = (work/'trace').read_text().splitlines()
        assert actual == list(expected), (name, actual, expected)
        if status == 0:
            assert process.stdout == b'allocation probe output\n', (name, process.stdout)
            assert not process.stderr, (name, process.stderr)
        else:
            assert not process.stdout, (name, process.stdout)
            assert (b'allocation before main' if status == 91 else b'recon trace:') in process.stderr
        checks.append(dict(executable=executable.name, check=name, status='passed',
                           setup_seconds=setup_seconds, execution_seconds=execution_seconds,
                           checks_seconds=time.perf_counter()-check_start))

    for compiler in ('gcc', 'clang'):
        start = time.perf_counter()
        options = [compiler, '-m32', '-std=gnu89', '-O0', '-g', '-fno-builtin']
        guard_object, target_object = directory/(compiler+'-guard.o'), directory/(compiler+'-target.o')
        subprocess.run(options+['-c', str(guard), '-o', str(guard_object)], check=True, capture_output=True)
        subprocess.run(options+['-finstrument-functions', '-c', str(target), '-o', str(target_object)],
                       check=True, capture_output=True)
        build_seconds = time.perf_counter()-start
        for mode in ('pie', 'exec'):
            executable = directory/(compiler+'-'+mode)
            link_start = time.perf_counter()
            subprocess.run(options+['-DRECON_NATIVE_TEST', '-fPIE' if mode == 'pie' else '-no-pie',
                                    '-pie' if mode == 'pie' else '-no-pie', str(guard_object),
                                    str(target_object), str(ROOT/'src/trace.c'), '-o', str(executable)],
                           check=True, capture_output=True)
            build_seconds += time.perf_counter()-link_start
            run(executable, 'empty-prefix-no-heap')
            run(executable, 'crlf-and-final-record', filter_data=b'hidden\r\nmain\r\nalpha',
                seen_data=b'main\r\nalpha', expected=('hidden',))
            run(executable, 'all-seen-final-record', seen_data=b'main\nalpha\nhidden', expected=())
            run(executable, 'disabled-no-heap', filter_data=b'\0invalid', expected=(), disabled=True)
            for operation in ('malloc', 'calloc', 'realloc', 'free'):
                run(executable, 'guard-'+operation, expected=(), status=91,
                    extra_env={'PROBE_FORBIDDEN_ALLOCATION': operation})
            # Error diagnostics may use stdio/allocate; successful paths above cannot.
            for name, filter_data, seen_data in [
                ('empty-filter', b'', b''),
                ('embedded-zero', b'main\0\nalpha\nhidden\n', b''),
                ('blank-record', b'main\n\nalpha\nhidden\n', b''),
                ('space-in-name', b'main \nalpha\nhidden\n', b''),
                ('tab-in-name', b'main\t\nalpha\nhidden\n', b''),
                ('duplicate-filter', b'main\nmain\nalpha\nhidden\n', b''),
                ('duplicate-prefix', b'main\nalpha\nhidden\n', b'main\nmain'),
                ('unknown-prefix', b'main\nalpha\nhidden\n', b'outside'),
                ('page-boundary-final-record', b'main\n'+b'x'*(4096-5), b''),
            ]:
                run(executable, name, filter_data=filter_data, seen_data=seen_data,
                    expected=(), status=126, extra_env={'ALLOW_TRACE_ERROR_ALLOCATION': '1'})
        builds.append(dict(compiler=compiler, seconds=build_seconds))
    receipt = directory/'receipt.json'
    receipt.write_text(json.dumps(dict(status='passed', builds=builds, checks=checks), indent=2)+'\n')
    print(json.dumps(dict(status='passed', checks=len(checks), builds=builds,
                          execution_seconds=sum(check['execution_seconds'] for check in checks),
                          receipt=str(receipt))))


if __name__ == '__main__':
    main()
