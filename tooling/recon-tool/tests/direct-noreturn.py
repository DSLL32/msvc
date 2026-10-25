#!/usr/bin/env python3
"""Focused explicit-original DIRECT exit controls and actual native pairing."""
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import time

TOOL = Path(__file__).resolve().parents[1]
REPO = Path(os.environ.get('RECON_TOOL_TEST_REPO', '/root/msvc2'))
EXE = Path(os.environ.get('RECON_TOOL_TEST_EXE', TOOL / 'target/i686-unknown-linux-gnu/release/recon-tool'))
BROKER = Path(os.environ.get('RECON_TOOL_TEST_HELPER', TOOL / '.build/wine-broker.exe'))
WORK = Path(tempfile.mkdtemp(prefix='direct-noreturn-', dir=Path.home() / '.ai-tmp/msvc2'))
START = time.monotonic()


def win(path):
    path = Path(path).resolve()
    for base, drive in [(REPO, 'R'), (Path.home() / '.ai-tmp/msvc2', 'S')]:
        if path.is_relative_to(base):
            return drive + ':\\' + str(path.relative_to(base)).replace('/', '\\')
    raise ValueError(path)


def call(command):
    result = subprocess.run([str(x) for x in command], capture_output=True)
    if result.returncode:
        raise AssertionError(result.stdout.decode(errors='replace') + result.stderr.decode(errors='replace'))
    return result.stdout


def original(tool, *args):
    return call([EXE, 'original', 'tool', '--repo', REPO, '1.0-nt', tool, '--', *args])


source = WORK / 'target.c'
source.write_text('''#include <windows.h>
void __cdecl marker(void) { ExitProcess(9); }
void __stdcall start(void) {
 char early[2]; DWORD count;
 WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),"ready\\n",6,&count,0);
 if(GetEnvironmentVariableA("EARLY",early,sizeof(early))) ExitProcess(4);
 marker(); ExitProcess(9);
}
''')
original('CL.EXE', '/nologo', '/c', '/Od', '/G3', '/Fo' + win(WORK / 'target.obj'), win(source))
target = WORK / 'target.exe'
mapfile = WORK / 'target.map'
original('LINK.EXE', '/nologo', '/nodefaultlib', '/entry:start@0', '/subsystem:console',
         '/out:' + win(target), '/map:' + win(mapfile), win(WORK / 'target.obj'), 'KERNEL32.LIB')
raw = target.read_bytes()
pe = struct.unpack_from('<I', raw, 0x3c)[0]
base = struct.unpack_from('<I', raw, pe + 24 + 28)[0]
symbols = {m[1]: int(m[2], 16) - base for m in re.finditer(
    r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', mapfile.read_text(), re.M)}
entry, marker = symbols['_start@0'], symbols['_marker']
probe = WORK / 'probe.c'
probe.write_text('''#include "recon-original-direct.h"
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *probe) {
 char mode[16]; GetEnvironmentVariableA("MODE",mode,sizeof(mode));
 if(mode[0]=='r') recon_original_finish(probe,1);
 ExitProcess(mode[0]=='w'?5:4);
}
''')
helper = WORK / 'probe.dll'
call([EXE, 'original', 'build-direct', probe, '--repo', REPO, '--output', helper])
case = WORK / 'native/cases/0000-exit'
(case / '.build').mkdir(parents=True)
native = case.parents[1]
(native / '.build').mkdir()
(native / 'app.c').write_text('''#include <unistd.h>
#include <stdlib.h>
void fixture_marker(void) { exit(9); }
void fixture_start(void) { write(1,"ready\\n",6);fixture_marker();exit(9); }
''')
(case / 'case.c').write_text('''#include "recon-tool.h"
#include "recon-native.h"
#include <stdlib.h>
extern void fixture_start(void),fixture_marker(void);
static void callback(void) {
#ifdef RETURNING
 recon_native_entry_finish(1);
#else
 exit(4);
#endif
}
static void invoke(void *unused) {(void)unused;fixture_start();}
static int direct(void) {return recon_native_at_entry(fixture_marker,callback,invoke,0)==1;}
static const uint32_t order[]={%d,%d};
const ReconCase case_descriptor={.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),
 .name="noreturn",.description="authored DIRECT exit control",.execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=direct,
 .first_entry_order=order,.first_entry_count=2};
''' % (entry, marker))
records = []
for name, native_name in [('_start@0', 'fixture_start'), ('_marker', 'fixture_marker')]:
    start = symbols[name]
    end = min(x for x in symbols.values() if x > start)
    records.append('{.name="%s",.entry_rva=%d,.code=(const ReconExtent[]){{%d,%d}},.code_count=1,.definition=RECON_APP,.implementation=RECON_IMPLEMENTED,.native_symbol="%s"}' % (name, start, start, end, native_name))
for name, api in [('_ExitProcess@4', 'ExitProcess'), ('_WriteFile@20', 'WriteFile'),
                  ('_GetStdHandle@4', 'GetStdHandle'), ('_GetEnvironmentVariableA@12', 'GetEnvironmentVariableA')]:
    start = symbols[name]
    records.append('{.name="%s",.entry_rva=%d,.code=(const ReconExtent[]){{%d,%d}},.code_count=1,.definition=RECON_IMPORT_NAME,.import_dll="KERNEL32.dll",.import_name="%s"}' % (name, start, start, start + 6, api))
(native / 'binary.c').write_text('#include "recon-tool.h"\nstatic const ReconFunction f[]={' + ','.join(records) + '};\nconst ReconBinary binary_descriptor={.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconBinary),.name="authored-exit",.original_path="' + str(target) + '",.original_sha256="' + hashlib.sha256(raw).hexdigest() + '",.original_entry_rva=' + str(entry) + ',.functions=f,.function_count=' + str(len(records)) + '};\n')
flags = ['gcc', '-m32', '-fPIC', '-g', '-O0', '-I' + str(TOOL / 'include'),
         '-I' + str(REPO / 'lib/recon/include'), '-DRECON_NATIVE_TEST']
objects = []
for source, instrument in [(native / 'app.c', True), (native / 'binary.c', False),
                           (REPO / 'lib/recon/src/trace.c', False), (TOOL / 'native/entry-hook.c', False)]:
    obj = native / '.build' / (source.stem + '.o')
    call([*flags, *(['-finstrument-functions'] if instrument else []), '-c', source, '-o', obj])
    objects.append(obj)
call(['gcc', '-m32', '-shared', '-Wl,-Bsymbolic-functions', '-Wl,-soname,libbinary.so', *objects, '-ldl', '-o', native / '.build/libbinary.so'])


def build_case(returning=False):
    call([*flags, *(['-DRETURNING'] if returning else []), '-shared', case / 'case.c',
          native / '.build/libbinary.so', '-Wl,-rpath,$ORIGIN/../../../.build', '-o', case / '.build/case.so'])


build_case()
results = []
for label, mode, early, noreturn, expected, passed in [
    ('exit4', 'exit', None, True, 4, True),
    ('wrong5', 'wrong', None, True, 4, False),
    ('before-callback', 'exit', '1', True, 4, False),
    ('returned-under-noreturn', 'return', None, True, 4, False),
    ('ordinary-return', 'return', None, False, 0, True),
]:
    data = WORK / (label + '-input.json')
    data.write_text(json.dumps({'args': ['authored'], 'env': {'MODE': mode, 'EARLY': early},
                                'stdout': 'text:ready\n', 'stderr': 'text:', 'exit_code': expected}))
    fixture = WORK / (label + '-input.bin')
    call([EXE, 'capture', 'pack', data, fixture])
    output, report = WORK / (label + '.bin'), WORK / (label + '.json')
    command = [EXE, 'original', 'capture', case, '--repo', REPO, '--capture', fixture,
               '--helper', BROKER, '--direct-helper', helper, '--marker-rva', marker,
               '--progress', '--output', output, '--report', report]
    if noreturn:
        command.append('--direct-noreturn')
    started = time.monotonic()
    result = subprocess.run([str(x) for x in command], capture_output=True)
    (WORK / (label + '.log')).write_bytes(result.stdout + result.stderr)
    assert (result.returncode == 0) == passed, (label, result.stdout, result.stderr)
    assert output.exists() == passed
    if not passed:
        fragment = {'wrong5': b'nonreturn DIRECT exit differs',
                    'before-callback': b'lacks actual callback-entry proof',
                    'returned-under-noreturn': b'typed original callback assertions failed'}[label]
        assert fragment in result.stdout + result.stderr, (label, result.stdout, result.stderr)
    results.append({'label': label, 'expected_pass': passed, 'elapsed_ms': (time.monotonic() - started) * 1000})
    if label == 'exit4':
        receipt = json.loads(report.read_text())
        assert receipt['terminal'] == 'exit:4' and receipt['first_entry_order'] == [entry, marker]
        proof = receipt['direct_noreturn']
        assert proof['callback_observed'] and all(proof[k] for k in ['callback_pc', 'pid', 'tid'])
        assert proof['expected_exit'] == proof['actual_exit'] == 4
        command = [EXE, 'run', case, '--capture', output, '--original-report', report]
        call(command)
        build_case(returning=True)
        bad = subprocess.run([str(x) for x in command], capture_output=True)
        assert bad.returncode and b'nonreturn DIRECT callback returned' in bad.stdout + bad.stderr
        build_case()
(WORK / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
print('DIRECT nonreturn/ordinary-return original controls and native pair PASS:', WORK,
      'elapsed %.3f s' % (time.monotonic() - START))
