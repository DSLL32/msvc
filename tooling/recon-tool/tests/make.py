#!/usr/bin/env python3
"""Focused make integration: discovery, one shared build, and header increments."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

TOOL_DIR = Path(__file__).resolve().parents[1]
root = Path(tempfile.mkdtemp(prefix='recon-tool-make-', dir=Path.home() / '.ai-tmp'))
try:
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
root=Path(__file__).resolve().parent
mode,*args=sys.argv[1:]
with (root/'commands.jsonl').open('a') as log:
    log.write(json.dumps([mode,*args])+'\\n')
if mode=='native':
    sys.exit(subprocess.call(['gcc',*args]))
if mode=='original':
    output=next(arg[3:] for arg in args if arg.startswith('/Fo')) if '/c' in args else next(arg[5:] for arg in args if arg.startswith('/out:'))
    Path(output).write_text('original')
if mode=='tool' and args[0]=='registry-export':
    Path(args[args.index('--output')+1]).write_text('{}')
''')
    write('tool', f'#!/bin/sh\nexec python3 {wrapper} tool "$@"\n').chmod(0o755)
    write('graph/graph-audit.mk', '')
    write('graph/Cargo.toml', '')
    write('graph/Cargo.lock', '')
    write('graph/build-quiet.sh', '#!/bin/sh\nexit 1\n').chmod(0o755)
    write('graph/target/release/graph-audit', f'#!/bin/sh\nexec python3 {wrapper} graph "$@"\n').chmod(0o755)
    write('src/nested.h', '#define VALUE 1\n')
    write('src/local.h', '#include "nested.h"\n')
    write('src/one.c', '#include "local.h"\nint one(void) { return VALUE; }\n')
    write('other/one.c', 'int another(void) { return 2; }\n')
    write('template.c', 'int generated(void) { return 3; }\n')
    write('src/runtime.c', 'int original_runtime(void) { return 0; }\n')
    write('support.c', 'int native_support(void) { return 0; }\n')
    write('binary.c', 'int binary_descriptor;\n')
    write('policy.json', '{}')
    write('Makefile', f'''RECON_TOOL_DIR := {TOOL_DIR}
RECON_TOOL := {root}/tool
NATIVE_CC := python3 {wrapper} native
BINARY_SOURCES := src/one.c other/one.c generated/code.c
BINARY_SUPPORT_SOURCES := support.c
ORIGINAL_SOURCES := $(BINARY_SOURCES) src/runtime.c
ORIGINAL_BUILD := original
ORIGINAL_OUTPUT := original/app.exe
ORIGINAL_CC := python3 {wrapper} original
ORIGINAL_LINK := python3 {wrapper} original
ORIGINAL_INCLUDE_DIRS := src
GRAPH_AUDIT_DIR := {root}/graph
RECON_AUDIT_PROFILE := example
RECON_IMAGE_BASE := 65536
RECON_POLICY := policy.json
RECON_AUDIT_FLAGS := --discovery cfg --check-ret
RECON_AUDIT_METADATA := original/metadata.json
RECON_AUDIT_PREREQUISITES := $(ORIGINAL_OUTPUT)
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
        write(f'cases/{name}/{source}', '#include \"local.h\"\nint case_descriptor = CASE_VALUE;\n')
        write(f'cases/{name}/Makefile', f'''RECON_TOOL_DIR := {TOOL_DIR}
CASE_CC := python3 {wrapper} native
CASE_CXX := python3 {wrapper} native
CASE_CPPFLAGS = -I$(CASE_DIR) -DCASE_EXPECTED_{name.replace('-', '_')}
include $(RECON_TOOL_DIR)/case.mk
''')
    write('cases/not-a-case/Makefile', 'all:\n\tfalse\n')
    write('cases/9999-no-makefile/case.c', '')

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
    assert all(row[0] != 'original' for row in initial)
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
    assert '-DCASE_EXPECTED_0000_first' in delta[0] and '-DCASE_EXPECTED_0001_second' not in delta[0], delta
    assert cases[1].stat().st_mtime_ns == mtimes[1]
    before = len(commands())
    subprocess.run(['make', '-C', str(root/'cases/0000-first'), '-j4'], check=True,
                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    assert len(commands()) == before, 'standalone case rebuilt unchanged library'
    output = make('check', 'unit')
    assert 'make[1]' not in output, output
    invocations = [row for row in commands() if row[0] == 'tool']
    assert [row[1] for row in invocations] == ['run', 'test'] or [row[1] for row in invocations] == ['test', 'run']
    assert all(row[2:] == ['cases/0000-first', 'cases/0001-second'] for row in invocations)
    assert not any(row[0] == 'original' for row in commands()), 'native checks ran original build'

    make('win32')
    originals = [row for row in commands() if row[0] == 'original']
    assert len(originals) == 5, originals
    for source in ['src/one.c', 'other/one.c', 'generated/code.c', 'src/runtime.c']:
        assert (root/'original/obj'/(source+'.obj')).is_file()
    assert not (root/'original/obj/binary.c.obj').exists()
    assert not (root/'original/obj/support.c.obj').exists()
    before = len(commands())
    make('win32')
    assert len(commands()) == before, 'unchanged original build ran compiler/linker'
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
    # Dependency scanning must terminate on ordinary include-guard cycles too.
    write('src/cycle-a.h', '#include "cycle-b.h"\n')
    write('src/cycle-b.h', '#include "cycle-a.h"\n#include "nested.h"\n')
    write('src/cycle.c', '#include "cycle-a.h"\n')
    subprocess.run(['python3', str(TOOL_DIR/'original-deps.py'), '--object-root', 'obj',
                    '--output', str(root/'cycles.mk'), 'src/cycle.c'], cwd=root, check=True)
    first = (root/'cycles.mk').read_text().splitlines()[0]
    assert all(name in first for name in ['cycle-a.h', 'cycle-b.h', 'nested.h'])
    make('clean')
    assert not (root/'.build').exists() and not (root/'original').exists()
    assert all(not path.parent.exists() for path in cases)
    print('shared make discovery, parallel/incremental native+original, audit checks passed')
finally:
    shutil.rmtree(root)
