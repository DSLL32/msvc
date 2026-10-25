#!/usr/bin/env python3
"""Focused make integration: discovery, one shared build, and header increments."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

TOOL_DIR = Path(__file__).resolve().parents[1]
REPO = Path(os.environ.get('RECON_TOOL_TEST_REPO', TOOL_DIR.parents[1]))
root = Path(tempfile.mkdtemp(prefix='recon-tool-make-', dir=Path.home() / '.ai-tmp'))
try:
    # Only copied headers are mutated by incremental controls.
    for directory in ['include', 'native']:
        shutil.copytree(REPO / 'lib/recon' / directory, root / 'shared' / directory)
    def write(name, text):
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        return path

    wrapper = write('commands.py', '''#!/usr/bin/env python3
import json
from pathlib import Path
import subprocess
import sys
import time
root=Path(__file__).resolve().parent
mode,*args=sys.argv[1:]
with (root/'commands.jsonl').open('a') as log:
    log.write(json.dumps([mode,*args])+'\\n')
if mode in ('native','native-cxx'):
    # These authored C++ fixtures need no C++ runtime; test driver selection
    # without requiring an unrelated i386 libstdc++ development package.
    driver=['clang++','-nostdlib++'] if mode=='native-cxx' else ['gcc']
    sys.exit(subprocess.call([*driver,*args]))
if mode=='original':
    output=next(arg[3:] for arg in args if arg.startswith('/Fo')) if '/c' in args else next(arg[5:] for arg in args if arg.startswith('/out:'))
    Path(output).write_text('original')
    if '/c' not in args:
        for arg in args:
            if arg.startswith('/map:'): Path(arg[5:]).write_text('map')
if mode=='tool' and args[0] in ('registry-export','identify-runtime'):
    Path(args[args.index('--output')+1]).write_text('[]' if args[0]=='identify-runtime' else '{}')
if mode=='tool' and args[0] in ('run','test'):
    def event(kind):
        with (root/'checks.jsonl').open('a') as log:
            log.write(json.dumps([time.monotonic_ns(),kind,args[0],args[1]])+'\\n')
    event('start')
    time.sleep(.1)
    event('end')
''')
    write('tool', f'#!/bin/sh\nexec python3 {wrapper} tool "$@"\n').chmod(0o755)
    write('graph/graph-audit.mk', '')
    write('graph/Cargo.toml', '')
    write('graph/Cargo.lock', '')
    write('graph/build-quiet.sh', '#!/bin/sh\nexit 1\n').chmod(0o755)
    write('graph/target/release/graph-audit', f'#!/bin/sh\nexec python3 {wrapper} graph "$@"\n').chmod(0o755)
    write('src/nested.h', '#define VALUE 1\n')
    write('src/local.h', '#include "nested.h"\n')
    write('src/one.c', '#include "local.h"\n#include <recon.h>\nint one(void) { return VALUE; }\n')
    write('other/one.c', 'int another(void) { return 2; }\n')
    write('template.c', 'int generated(void) { return 3; }\n')
    write('src/runtime.c', 'int original_runtime(void) { return 0; }\n')
    write('src/link.h', '#error local header must not shadow system link.h\n')
    write('support/a_b.c', '#include <link.h>\nint native_support(void) { return 0; }\n')
    write('support/a/b.c', 'int another_support(void) { return 1; }\n')
    write('harness/host.cpp', 'extern "C" int cpp_host(void) { return 7; }\n')
    write('binary.c', 'int binary_descriptor;\n')
    write('policy.json', '{}')
    write('Makefile', f'''RECON_TOOL_DIR := {TOOL_DIR}
RECON_TOOL := {root}/tool
NATIVE_CC := python3 {wrapper} native
NATIVE_CXX := python3 {wrapper} native-cxx
NATIVE_CPPFLAGS := -DINHERITED_BUILD_FLAGS -DRECON_NATIVE_TEST -Ishared/include -Ishared/native
NATIVE_CFLAGS := -std=gnu89
NATIVE_CXXFLAGS := -std=gnu++98
BINARY_SOURCES := src/one.c other/one.c generated/code.c
BINARY_SOURCE_ROOT := {root}
BINARY_SUPPORT_SOURCES := support/a_b.c support/a/b.c harness/host.cpp
ORIGINAL_SOURCES := $(BINARY_SOURCES) src/runtime.c
ORIGINAL_BUILD := original
ORIGINAL_OUTPUT := original/app.exe
ORIGINAL_MAP := original/app.map
ORIGINAL_CC := python3 {wrapper} original
ORIGINAL_LINK := python3 {wrapper} original
ORIGINAL_INCLUDE_DIRS := src shared/include
GRAPH_AUDIT_DIR := {root}/graph
RECON_AUDIT_PROFILE := example
RECON_IMAGE_BASE := 65536
RECON_POLICY := policy.json
RECON_AUDIT_FLAGS := --discovery cfg --check-ret
RECON_AUDIT_METADATA := original/metadata.json
RECON_AUDIT_PREREQUISITES := $(ORIGINAL_OUTPUT) $(ORIGINAL_MAP)
include $(RECON_TOOL_DIR)/recon-tool.mk
generated/code.c: template.c
\t@mkdir -p $(@D)
\tcp $< $@
original/metadata.json:
\t@mkdir -p $(@D)
\techo '{{}}' > $@
audit: local-audit
.PHONY: local-audit
local-audit:
\t@echo local > original/local-audit
''')
    for name in ['0000-first', '0001-second']:
        write(f'cases/{name}/local.h', '#define CASE_VALUE 1\n')
        source = 'case.c' if name == '0000-first' else 'case.cpp'
        write(f'cases/{name}/{source}', '#include \"local.h\"\n#ifndef INHERITED_BUILD_FLAGS\n#error missing binary flags\n#endif\nint case_descriptor = CASE_VALUE;\n')
    write('cases/not-a-case/case.c', '')
    write('cases/9999-no-descriptor/tests.c', '')

    def make(*args):
        result = subprocess.run(['make', '-j4', *args], cwd=root,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if result.returncode:
            raise AssertionError(result.stdout)
        return result.stdout

    def commands():
        return [json.loads(line) for line in (root/'commands.jsonl').read_text().splitlines()]

    output = make('cases')
    assert 'make[1]' not in output and 'RECON_SKIP_BINARY_BUILD' not in output, output
    initial = commands()
    common_links = [row for row in initial if '-Wl,-soname,libbinary.so' in row]
    assert len(common_links) == 1, common_links
    assert common_links[0][0] == 'native-cxx', 'C++ support needs the C++ linker'
    support_cpp = next(row for row in initial if 'harness/host.cpp' in row)
    assert support_cpp[0] == 'native-cxx' and '-std=gnu++98' in support_cpp
    assert '-std=gnu89' not in support_cpp and '-finstrument-functions' not in support_cpp
    assert all(row[0] != 'original' for row in initial)
    assert (root/'.build/support/support/a_b.c.o').is_file()
    assert (root/'.build/support/support/a/b.c.o').is_file()
    cases = [root/f'cases/{name}/.build/case.so' for name in ['0000-first', '0001-second']]
    mtimes = [path.stat().st_mtime_ns for path in cases]
    make('cases')
    assert commands() == initial, 'unchanged build ran compilers'
    # A single transitive header rebuilds only its source and the shared DSO;
    # cases retain their link output because their dependency is order-only.
    time.sleep(.02)
    write('src/nested.h', '#define VALUE 2\n')
    make('cases')
    changed = commands()[len(initial):]
    assert len(changed) == 2 and 'src/one.c' in changed[0], changed
    assert [path.stat().st_mtime_ns for path in cases] == mtimes
    before = len(commands())
    time.sleep(.02)
    write('cases/0000-first/local.h', '#define CASE_VALUE 2\n')
    output = make('cases')
    delta = commands()[before:]
    assert len(delta) == 2 and 'cases/0000-first/case.c' in delta[0], delta
    assert '-DINHERITED_BUILD_FLAGS' in delta[0] and '-finstrument-functions' in delta[0], delta
    assert cases[1].stat().st_mtime_ns == mtimes[1]
    before = len(commands())
    output = make('cases/0000-first/.build/case.so')
    assert len(commands()) == before and 'make[1]' not in output, output
    output = make('check', 'replay', 'fixtures', 'unit', 'test-all-cases')
    assert 'make[1]' not in output, output
    invocations = [row for row in commands() if row[0] == 'tool']
    expected = [[kind, f'cases/{name}'] for kind in ['run', 'test']
                for name in ['0000-first', '0001-second']]
    assert sorted(row[1:] for row in invocations) == sorted(expected), invocations
    active = peak = 0
    for _, event, _, _ in sorted(json.loads(line) for line in
                                 (root/'checks.jsonl').read_text().splitlines()):
        active += 1 if event == 'start' else -1
        peak = max(peak, active)
    assert active == 0 and peak >= 2, 'make -j4 did not overlap case checks'
    before = len(commands())
    make('cases/0000-first/check', 'RECON_RUN_FLAGS=--report selected.json')
    assert commands()[before:] == [['tool', 'run', 'cases/0000-first',
                                   '--report', 'selected.json']]
    assert not any(row[0] == 'original' for row in commands()), 'native checks ran original build'

    make('win32', 'audit')
    assert (root/'original/app.map').read_text() == 'map'
    originals = [row for row in commands() if row[0] == 'original']
    assert len(originals) == 5, originals
    for source in ['src/one.c', 'other/one.c', 'generated/code.c', 'src/runtime.c']:
        assert (root/'original/obj'/(source+'.obj')).is_file()
    assert not (root/'original/obj/binary.c.obj').exists()
    assert not (root/'original/obj/support.c.obj').exists()
    before = len(commands())
    make('win32')
    assert len(commands()) == before, 'unchanged original build ran compiler/linker'
    # Native-only API additions must not invalidate original sources. The real
    # dependency scanner sees only headers available in the original search path.
    time.sleep(.02)
    remaps = root / 'shared/native/recon-remaps.h'
    remaps.write_text(remaps.read_text() + '\nvoid recon_dependency_probe(void);\n')
    start = time.monotonic()
    make('cases', 'win32')
    elapsed = time.monotonic() - start
    delta = commands()[before:]
    assert len(delta) == 2 and delta[0][0] == 'native' and 'src/one.c' in delta[0], delta
    assert not any(row[0] == 'original' for row in delta), delta
    print(f'native-only header: one native compile/link, zero original compiles/links ({elapsed:.3f}s)')
    before = len(commands())
    time.sleep(.02)
    common = root / 'shared/include/trace-abi.h'
    common.write_text(common.read_text() + '\n#define RECON_DEPENDENCY_ABI_CONTROL 1\n')
    start = time.monotonic()
    make('cases', 'win32')
    elapsed = time.monotonic() - start
    delta = commands()[before:]
    originals = [row for row in delta if row[0] == 'original']
    assert len(originals) == 2 and 'src/one.c' in originals[0] and '/c' not in originals[1], delta
    assert len([row for row in delta if row[0].startswith('native')]) == 2, delta
    print(f'common ABI header: affected native and original compile/link ({elapsed:.3f}s)')
    before = len(commands())
    (root/'original/app.map').unlink()
    make('win32', 'audit')
    relinks = [row for row in commands()[before:] if row[0] == 'original']
    assert len(relinks) == 1 and '/out:original/app.exe' in relinks[0], relinks
    assert (root/'original/app.map').exists() and (root/'original/app.exe').exists()
    before = len(commands())
    time.sleep(.02)
    write('src/nested.h', '#define VALUE 3\n')
    make('win32')
    delta = commands()[before:]
    assert len(delta) == 2 and 'src/one.c' in delta[0] and '/c' not in delta[1], delta
    make('audit')
    graph = [row for row in commands() if row[0] == 'graph'][-1]
    assert '--profile' in graph and 'example' in graph and '--check-ret' in graph
    assert '--metadata' in graph and '--report' in graph
    assert (root/'original/local-audit').is_file()
    # The CF identity profile is a normal projection input, not runtime policy.
    profile = write('identity.json', '{}')
    before = len(commands())
    make('audit', 'RECON_CF_PROFILE=identity.json')
    projection = [row for row in commands()[before:] if row[:2] == ['tool', 'registry-export']]
    assert len(projection) == 1 and projection[0][projection[0].index('--cf-profile') + 1] == 'identity.json'
    make('audit', 'RECON_CF_PROFILE=identity.json')
    assert len([row for row in commands()[before:] if row[:2] == ['tool', 'registry-export']]) == 1
    time.sleep(.02)
    profile.write_text('{"changed":true}')
    make('audit', 'RECON_CF_PROFILE=identity.json')
    assert len([row for row in commands()[before:] if row[:2] == ['tool', 'registry-export']]) == 2
    # Runtime identification is an inherited incremental rule, not a local script.
    runtime_flags = ['RECON_AUDIT_IDENTIFY_RUNTIME=1', 'ORIGINAL_TOOLCHAIN=1.0-nt',
                     'RECON_RUNTIME_INPUTS=']
    make('audit', *runtime_flags)
    assert (root/'original/runtime.json').read_text() == '[]'
    identified = [row for row in commands() if row[:2] == ['tool', 'identify-runtime']]
    assert len(identified) == 1 and identified[0][2:4] == ['1.0-nt', 'original/app.exe']
    assert '--rebuilt-runtime' in [row for row in commands() if row[0] == 'graph'][-1]
    make('audit', *runtime_flags)
    assert [row for row in commands() if row[:2] == ['tool', 'identify-runtime']] == identified
    # Dependency scanning must terminate on ordinary include-guard cycles too.
    write('src/cycle-a.h', '#ifndef CYCLE_A_H\n#define CYCLE_A_H\n#include "cycle-b.h"\n#endif\n')
    write('src/cycle-b.h', '#ifndef CYCLE_B_H\n#define CYCLE_B_H\n#include "cycle-a.h"\n#include "nested.h"\n#endif\n')
    write('src/cycle.c', '#include "cycle-a.h"\n')
    subprocess.run(['python3', str(TOOL_DIR/'original-deps.py'), '--object-root', 'obj',
                    '--output', str(root/'cycles.mk'), 'src/cycle.c'], cwd=root, check=True)
    first = (root/'cycles.mk').read_text().splitlines()[0]
    assert all(name in first for name in ['cycle-a.h', 'cycle-b.h', 'nested.h'])
    # Selecting a named toolchain needs no local compiler/linker wrapper.
    makefile = (root/'Makefile').read_text()
    makefile = makefile.replace(f'ORIGINAL_CC := python3 {wrapper} original\n', '')
    makefile = makefile.replace(f'ORIGINAL_LINK := python3 {wrapper} original\n', '')
    (root/'Makefile').write_text('ORIGINAL_TOOLCHAIN := 1.0-nt\n' + makefile)
    dry = make('-n', 'win32')
    assert 'original tool 1.0-nt CL.EXE --' in dry and 'original tool 1.0-nt LINK.EXE --' in dry
    make('clean')
    assert not (root/'.build').exists() and not (root/'original').exists()
    assert all(not path.parent.exists() for path in cases)
    # Default reconstruction discovery includes CRT code, while harness sources
    # remain explicitly selected (some are original-only hosts or unit helpers).
    write('crt/extra.c', 'int crt_extra(void) { return 9; }\n')
    layout = (root/'Makefile').read_text()
    layout = layout.replace('BINARY_SOURCES := src/one.c other/one.c generated/code.c\n', '')
    layout = layout.replace('ORIGINAL_SOURCES := $(BINARY_SOURCES) src/runtime.c\n', '')
    (root/'Makefile').write_text(layout)
    make('cases')
    assert (root/'.build/pic/crt/extra.c.o').is_file()
    assert not (root/'.build/pic/harness/host.cpp.o').exists()
    dry = make('-n', 'win32')
    assert 'original/obj/crt/extra.c.obj' in dry
    assert 'original/obj/harness/' not in dry
    print('shared make discovery, parallel/incremental native+original, audit checks passed')
finally:
    shutil.rmtree(root)
