#!/usr/bin/env python3
"""Build real i386 shim probes and verify the explicit stream replay boundary."""
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
import spawn


SOURCE = r'''
#define RECON_SHIM_IMPL
#include <unistd.h>
#include <recon.h>

/* Exercise the real WriteFile retry and replay short-write loops deterministically. */
extern ssize_t __real_write(int,const void *,size_t);
ssize_t __wrap_write(int fd,const void *data,size_t count)
{
    static unsigned interrupted;
    if (getenv("PROBE_SHORT_WRITES") && (fd==1 || fd==2)) {
        if (!(interrupted&(1u<<fd))) {
            interrupted|=1u<<fd; errno=EINTR; return -1;
        }
        if (count>7) count=7;
    }
    return __real_write(fd,data,count);
}
int main(int argc,char **argv)
{
    const char *args[3]; int fd,result;
    if (argc!=2) return 90;
    if (!strcmp(argv[1],"child")) {
        write(1,"native\r\n\0\377",10);
        write(2,"native error\r\n",14);
        return 17;
    }
    if (!strcmp(argv[1],"redirect")) {
        fd=recon_open("redirect.bin",0x8301,0600);
        if (fd<0 || recon_free_osfhnd(1) || recon_set_osfhnd(1,recon_get_osfhandle(fd))) return 91;
    }
    if (!strcmp(argv[1],"merge") || !strcmp(argv[1],"redirect"))
        if (recon_free_osfhnd(2) || recon_set_osfhnd(2,recon_get_osfhandle(1))) return 92;
    args[0]=!strcmp(argv[1],"self")?recon_pgmptr:
        !strcmp(argv[1],"native")?argv[0]:"external.exe";
    args[1]="child"; args[2]=NULL;
    errno=0;
    result=recon_spawnve(_P_WAIT,args[0],args,NULL);
    if (!strcmp(argv[1],"failure")) return result==-1 && errno==ENOENT?0:93;
    return result;
}
'''


def main():
    scratch = Path.home()/'.ai-tmp/recon-spawn-streams'
    scratch.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='checks-', dir=scratch))
    source = directory/'target.c'
    source.write_text(SOURCE)
    builds, checks = [], []
    stdout = b'captured\r\n\0\xff\x1a'+bytes(range(256))*257
    stderr = b'error\r\n\0\xfe'+bytes(range(255, -1, -1))*97

    def run(executable, name, mode, callback, expected_status=0,
            expected_stdout=b'', expected_stderr=b'', short=False, error=None,
            extra_env=None, files=None, expected_requests=1, broker_error=True):
        setup_start = time.perf_counter()
        work = directory/(executable.name+'-'+name)
        work.mkdir()
        env = {k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
        env.update(extra_env or {})
        for filename, data in (files or {}).items():
            (work/filename).write_bytes(data)
        if short:
            env['PROBE_SHORT_WRITES'] = '1'
        broker = spawn.SpawnBroker(callback)
        env.update(broker.environment())
        setup_seconds = time.perf_counter()-setup_start
        execute_start = time.perf_counter()
        try:
            process = subprocess.run([str(executable), mode], cwd=work, env=env,
                                     pass_fds=(broker.fileno(),), capture_output=True, timeout=10)
        finally:
            broker.close()
        execution_seconds = time.perf_counter()-execute_start
        check_start = time.perf_counter()
        assert process.returncode == expected_status, (name, process.returncode, process.stderr)
        assert process.stdout == expected_stdout, (name, len(process.stdout))
        if error:
            assert error in process.stderr, (name, process.stderr)
            assert bool(broker.errors) == broker_error, (name, broker.errors)
            assert not broker.requests, name
        else:
            assert process.stderr == expected_stderr, (name, len(process.stderr))
            broker.check()
            assert len(broker.requests) == expected_requests, name
        if mode == 'redirect':
            assert (work/'redirect.bin').read_bytes() == stdout+stderr, name
        checks.append(dict(compiler=executable.name, check=name, status='passed',
                           setup_seconds=setup_seconds, execution_seconds=execution_seconds,
                           checks_seconds=time.perf_counter()-check_start))

    def replay(request):
        assert request['mode'] == 0 and request['file'] == 'external.exe'
        assert request['argv'] == ['external.exe', 'child']
        return dict(status=0, stdout=stdout, stderr=stderr)

    def reject(request):
        raise AssertionError('unknown invocation or input')

    def captured_failure(request):
        assert request['mode'] == 0 and request['file'] == 'external.exe'
        assert request['argv'] == ['external.exe', 'child']
        assert request['cwd'].startswith(str(directory))
        return dict(status=-1, errno=2)

    for compiler in sys.argv[1:] or ['gcc', 'clang']:
        executable = directory/Path(compiler).name
        start = time.perf_counter()
        command = [compiler, '-m32', '-std=gnu89', '-O0', '-g', '-Wall', '-Wextra',
                   '-DRECON_NATIVE_TEST', '-I'+str(ROOT/'include'), '-I'+str(ROOT/'native'),
                   str(source), *(str(ROOT/'src'/name) for name in ('crt.c','win32.c','process.c')),
                   '-Wl,--wrap=write', '-o', str(executable)]
        subprocess.run(command, check=True, capture_output=True)
        builds.append(dict(compiler=compiler, seconds=time.perf_counter()-start))
        run(executable, 'binary-streams-status-zero', 'mock', replay,
            expected_stdout=stdout, expected_stderr=stderr)
        run(executable, 'short-writes-and-eintr', 'mock', replay, short=True,
            expected_stdout=stdout, expected_stderr=stderr)
        run(executable, 'merged-logical-handles', 'merge', replay, expected_stdout=stdout+stderr)
        run(executable, 'redirected-logical-handles', 'redirect', replay)
        run(executable, 'legacy-integer', 'mock', lambda request: 37, expected_status=37)
        run(executable, 'real-native', 'native', lambda request: None, expected_status=17,
            expected_stdout=b'native\r\n\0\xff', expected_stderr=b'native error\r\n')
        run(executable, 'captured-enoent', 'failure', captured_failure)
        run(executable, 'captured-enoent-streams', 'failure',
            lambda request: dict(status=-1, errno=2, stdout=stdout, stderr=stderr),
            expected_stdout=stdout, expected_stderr=stderr)
        run(executable, 'preflight-missing-no-request', 'failure', reject,
            extra_env={'RECON_SPAWN_REQUIRE_EXISTS': '1'}, expected_requests=0)
        run(executable, 'preflight-existing-pe-replayed', 'mock', replay,
            extra_env={'RECON_SPAWN_REQUIRE_EXISTS': '1'}, files={'external.exe': b'MZ'+b'\0'*30},
            expected_stdout=stdout, expected_stderr=stderr)
        run(executable, 'preflight-virtual-self-route', 'self', lambda request: None,
            extra_env={'RECON_SPAWN_REQUIRE_EXISTS': '1', 'RECON_SELF_SPAWN': '1',
                       'RECON_PROGRAM_PATH': r'Z:\virtual\stream-probe.exe'}, expected_status=17,
            expected_stdout=b'native\r\n\0\xff', expected_stderr=b'native error\r\n')
        run(executable, 'preflight-invalid-config', 'mock', reject, expected_status=125,
            extra_env={'RECON_SPAWN_REQUIRE_EXISTS': '0'},
            error=b'RECON_SPAWN_REQUIRE_EXISTS must be 1', broker_error=False)
        run(executable, 'unknown-dependency', 'mock', reject, expected_status=125,
            error=b'spawn broker rejected request')
        for name, response in [('bool-status', True), ('negative-status', -1),
                               ('large-status', 256), ('missing-stream', {'status': 0}),
                               ('text-stream', dict(status=0, stdout='text', stderr=b'')),
                               ('null-status', dict(status=None, stdout=b'', stderr=b'')),
                               ('failure-no-errno', dict(status=-1, stdout=b'', stderr=b'')),
                               ('failure-zero-errno', dict(status=-1, errno=0)),
                               ('failure-large-errno', dict(status=-1, errno=4096)),
                               ('failure-bool-errno', dict(status=-1, errno=True)),
                               ('failure-unknown-key', dict(status=-1, errno=2, unexpected=1)),
                               ('failure-wrong-status', dict(status=-2, errno=2)),
                               ('failure-text-stream', dict(status=-1, errno=2, stderr='text'))]:
            run(executable, name, 'mock', lambda request, response=response: response,
                expected_status=125, error=b'spawn broker rejected request')
        limit = spawn.MAX_STREAM_BYTES
        try:
            spawn.MAX_STREAM_BYTES = 1
            run(executable, 'oversized-callback-streams', 'mock', replay, expected_status=125,
                error=b'spawn broker rejected request')
        finally:
            spawn.MAX_STREAM_BYTES = limit

        # An independent malicious peer verifies the C side's protocol bounds.
        for name, response, diagnostic in [
            ('wire-magic', struct.pack('<4sII', b'BAD!', 1, 0), b'rejected request'),
            ('wire-action', struct.pack('<4sII', b'RSP1', 5, 0), b'rejected request'),
            ('wire-status', struct.pack('<4sII', b'RSP1', 1, 256), b'invalid spawn response status'),
            ('wire-native-status', struct.pack('<4sII', b'RSP1', 0, 1), b'invalid spawn response status'),
            ('wire-size', struct.pack('<4sII2I', b'RSP1', 3, 0, 64*1024*1024, 1), b'too large'),
            ('wire-truncated', struct.pack('<4sII2I', b'RSP1', 3, 0, 1, 0), b'cannot read spawn response'),
            ('wire-failure-status', struct.pack('<4sII', b'RSP1', 4, 0), b'invalid spawn response status'),
            ('wire-failure-errno-zero', struct.pack('<4siiI', b'RSP1', 4, -1, 0), b'invalid spawn response errno'),
            ('wire-failure-errno-large', struct.pack('<4siiI', b'RSP1', 4, -1, 4096), b'invalid spawn response errno'),
        ]:
            start = time.perf_counter()
            server, target = socket.socketpair()
            errors = []

            def serve():
                try:
                    header = spawn.SpawnBroker._read(server, 8)
                    _, length = struct.unpack('<4sI', header)
                    spawn.SpawnBroker._read(server, length)
                    server.sendall(response)
                    server.shutdown(socket.SHUT_WR)
                except Exception as error:
                    errors.append(str(error))

            thread = threading.Thread(target=serve)
            thread.start()
            env = {k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
            env['RECON_SPAWN_FD'] = str(target.fileno())
            process = subprocess.run([str(executable), 'mock'], env=env,
                                     pass_fds=(target.fileno(),), capture_output=True, timeout=10)
            thread.join(timeout=1)
            target.close()
            server.close()
            assert not thread.is_alive() and not errors, (name, errors)
            assert process.returncode == 125 and diagnostic in process.stderr, (name, process)
            assert not process.stdout, name
            checks.append(dict(compiler=executable.name, check=name, status='passed',
                               seconds=time.perf_counter()-start))
    receipt = directory/'receipt.json'
    receipt.write_text(json.dumps(dict(status='passed', builds=builds, checks=checks), indent=2)+'\n')
    print(json.dumps(dict(status='passed', checks=len(checks), builds=builds,
                          execution_seconds=sum(check.get('execution_seconds', check.get('seconds', 0))
                                                for check in checks), receipt=str(receipt))))


if __name__ == '__main__':
    main()
