#!/usr/bin/env python3
"""Authored native entry/module/asset integration; no Wine or target cases."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

TOOL = Path(__file__).resolve().parents[1]
REPO = TOOL.parents[1]
BASE = Path.home() / '.ai-tmp/msvc2/entry-build-tests'
BASE.mkdir(parents=True, exist_ok=True)
ROOT = Path(tempfile.mkdtemp(prefix='run-', dir=BASE))
LOGS = ROOT / 'logs'
LOGS.mkdir()
results = []

def run(command, cwd=ROOT, expected=0):
    begin = time.monotonic()
    p = subprocess.run(list(map(str, command)), cwd=cwd, capture_output=True,
                       env={**os.environ, 'ENTRY_TOKEN': 'present'})
    elapsed = time.monotonic() - begin
    stem = LOGS / ('%02d' % len(results))
    stem.with_suffix('.out').write_bytes(p.stdout)
    stem.with_suffix('.err').write_bytes(p.stderr)
    results.append(dict(command=list(map(str, command)), cwd=str(cwd), status=p.returncode, seconds=elapsed))
    assert p.returncode == expected, (command, p.returncode, p.stdout[-2000:], p.stderr[-4000:])
    return p


def write(path, text):
    path = ROOT / path
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    return path


def mtimes():
    return {str(p.relative_to(ROOT)): p.stat().st_mtime_ns
            for project in ('app', 'a', 'b') for p in (ROOT / project / '.build').rglob('*.o')}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

common_sources = ' '.join(str(REPO / 'lib/recon/src' / (v+'.c')) for v in ('crt','win32','process','compat','entry','decimal','trace'))
common = f'''RECON_TOOL_DIR := {TOOL}
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST -DRECON_CRT_PROFILE_MSVCRT_510 -DRECON_DEFAULT_FMODE=0 -I{REPO}/lib/recon/include -I{REPO}/lib/recon/native
NATIVE_CFLAGS := -std=gnu89
NATIVE_CXX := clang++ -nostdlib++
NATIVE_CXXFLAGS := -std=gnu++98
NATIVE_LDLIBS := -pthread
'''
for project in ('app', 'a', 'b'):
    directory = ROOT / project
    (directory / 'src').mkdir(parents=True)
    (directory / 'harness').mkdir()
    (directory / 'assets').mkdir()
    write(project+'/binary.c', 'int binary_descriptor;\n')
    write(project+'/harness/fixture.c', 'int shared_calls,shared_ctor_calls,ImportedCounter; void entry_event(int x,unsigned y){(void)x;(void)y;} int __attribute__((stdcall)) ImportedExport(int x,int (__attribute__((stdcall)) *f)(int)){return f(x);} int fixture_provider(void){return 99;}\n')
    if project == 'app':
        shutil.copyfile(TOOL/'tests/entry/app.c', directory/'src/app.c')
        shutil.copyfile(TOOL/'tests/entry/entry.c', directory/'entry.c')
        sources = 'src/app.c'
        tail = '''RECON_ENTRY_SUPPORT_SOURCES := $(filter-out harness/fixture.c,$(BINARY_SUPPORT_SOURCES))
NATIVE_ENTRY_LINKER := $(NATIVE_CXX)
NATIVE_BIN_IMPORTS := ImportedExport=a_Export ImportedCounter=a_counter
NATIVE_BIN_FILES := catalog=assets/catalog.bin
include $(RECON_TOOL_DIR)/recon-tool.mk
$(eval $(call recon-native-module,a,../a,Export ReadCounter ReadConstructor DllEntry counter,messages=assets/catalog.bin))
$(eval $(call recon-native-module,b,../b,Export ReadCounter ReadConstructor DllEntry counter))
'''
    else:
        suffix = 'cpp' if project == 'b' else 'c'
        shutil.copyfile(TOOL/'tests/entry/module.c', directory/('src/module.'+suffix))
        sources = 'src/module.'+suffix
        aliases = [{'win32_alias':'_'+alias, 'win32_target':'_'+target,
                    'native_alias':alias, 'native_target':target}
                   for alias,target in [('MethodAlias','internal'),('DataAlias','counter')]]
        write(project+'/aliases.json',json.dumps(dict(schema=1,aliases=aliases))+'\n')
        tail = 'NATIVE_CPPFLAGS += -DMODULE_TAG='+('1' if project=='a' else '2')+'\nRECON_ALIAS_MAP := aliases.json\ninclude $(RECON_TOOL_DIR)/recon-tool.mk\n'
    write(project+'/Makefile', common+f'''BINARY_SOURCES := {sources}
BINARY_SUPPORT_SOURCES := {common_sources} harness/fixture.c
RECON_ENTRY_SOURCES := {sources}
'''+tail)
(ROOT/'app/assets/catalog.bin').write_bytes(bytes([0,1,2,255])+b'CL\r\n\0X')
(ROOT/'a/assets/catalog.bin').write_bytes(b'DLL\0catalog\xff')
# A toolchain-level coordinator; recursive recipes retain the -j4 jobserver.
write('Makefile', f'''RECON_TOOL_DIR := {TOOL}
.DEFAULT_GOAL := native-bins
include $(RECON_TOOL_DIR)/native-entry.mk
$(eval $(call recon-native-program,app,app))
''')
try:
    for project in ('app','a','b'):
        run(['make','-j4','recon-native'], ROOT/project)
    before = mtimes()
    run(['make','-j4'])
    after = mtimes()
    assert all(after[path] == stamp for path, stamp in before.items()), 'fixture object was rebuilt'
    binary = ROOT/'app/build/app'
    p = run([binary,'one','two'], ROOT/'app')
    assert p.stdout == b'entry modules assets reset PASS\n' and not p.stderr
    assert (ROOT/'app/exit.log').read_bytes()==b'Xab'
    before = mtimes(); binary_stamp = binary.stat().st_mtime_ns
    p = run(['make','-j4'])
    assert mtimes()==before and binary.stat().st_mtime_ns==binary_stamp, 'incremental no-op recompiled/relinked'
    for module in ('a','b'):
        bundle=ROOT/f'app/.build/entry/modules/{module}/bundle.o'
        names=run(['nm','-g','--defined-only',bundle]).stdout.decode()
        assert set(line.split()[-1] for line in names.splitlines()) == {module+'_'+v for v in ('Export','ReadCounter','ReadConstructor','DllEntry','counter')}
        manifest=json.loads(Path(str(bundle)+'.json').read_text())
        assert manifest['state_sections'] and manifest['constructors']==[]
        assert all(manifest['renamed'][name]==module+'_'+name for name in ('MethodAlias','DataAlias'))
        symbols=run(['nm','-P','--defined-only',bundle]).stdout.decode().splitlines()
        addresses={line.split()[0]:line.split()[2] for line in symbols if len(line.split())>=3}
        assert addresses[module+'_MethodAlias']==addresses[module+'_internal']
        assert addresses[module+'_DataAlias']==addresses[module+'_counter']
        undefined=run(['nm','--undefined-only',bundle]).stdout.decode()
        assert 'MethodAlias' not in undefined and 'DataAlias' not in undefined
    names=run(['nm','-g','--defined-only',binary]).stdout.decode()
    assert ' fixture_provider' not in names and ' recon_entry_app_main' in names
    assert ' a_main' not in names and ' b_main' not in names
    # Exact read-only assets with colliding input basenames have distinct spans.
    sections=run(['readelf','-SW',ROOT/'app/.build/entry/assets/exe_catalog.o']).stdout.decode()
    assert '.rodata.recon_asset.exe_catalog' in sections and ' WA ' not in sections
    # The project's own alias map invalidates only its private bundle/link.
    before=mtimes(); previous=binary.stat().st_mtime_ns
    aliases_path=ROOT/'a/aliases.json'; aliases=json.loads(aliases_path.read_text())
    aliases['aliases'][0].update(native_target='internal_alternative',win32_target='_internal_alternative')
    aliases_path.write_text(json.dumps(aliases)+'\n')
    run(['make','-j4'])
    after=mtimes(); changed='app/.build/entry/modules/a/bundle.o'
    assert after[changed]!=before[changed] and binary.stat().st_mtime_ns!=previous
    assert all(after[path]==stamp for path,stamp in before.items() if path!=changed), 'alias change recompiled objects or another bundle'
    symbols=run(['nm','-P','--defined-only',ROOT/changed]).stdout.decode().splitlines()
    addresses={line.split()[0]:line.split()[2] for line in symbols if len(line.split())>=3}
    assert addresses['a_MethodAlias']==addresses['a_internal_alternative']!=addresses['a_internal']
    p=run([binary,'one','two'],ROOT/'app')
    assert p.stdout==b'entry modules assets reset PASS\n' and not p.stderr
    before=mtimes(); previous=binary.stat().st_mtime_ns
    run(['make','-j4'])
    assert mtimes()==before and binary.stat().st_mtime_ns==previous, 'alias no-op rebuilt'
    # Explicit process constructor: reject default, then reject snapshot pairing.
    module=ROOT/'b/src/module.cpp'
    with module.open('a') as f:
        f.write('\n__attribute__((constructor)) static void before_main(void){++constructor_count;++shared_ctor_calls;counter=23;}\n')
    p=run(['make','-j4'], expected=2)
    assert b'declare premain explicitly' in p.stderr
    with (ROOT/'app/Makefile').open('a') as f:f.write('\nRECON_MODULE_b_CONSTRUCTORS := premain\n')
    p=run(['make','-j4'], expected=2)
    assert b'cannot be combined with a reload snapshot' in p.stderr
    with (ROOT/'app/Makefile').open('a') as f:f.write('RECON_MODULE_b_RESET := none\n')
    run(['make','-j4'])
    p=run([binary,'premain'], ROOT/'app')
    assert p.stdout==b'premain without reset PASS\n' and not p.stderr
    assert (ROOT/'app/exit.log').read_bytes()==b'Xab'
    before = mtimes(); previous=binary.stat().st_mtime_ns
    (ROOT/'a/assets/catalog.bin').write_bytes(b'DLL\0catalog\xffextra')
    run(['make','-j4'])
    assert all(mtimes()[p]==t for p,t in before.items() if '/assets/' not in p), 'asset mutation recompiled code'
    assert binary.stat().st_mtime_ns != previous
    # Explicitly selected generated shim main and missing exports cannot leak in.
    p=run(['python3',TOOL/'entry-build.py','support',ROOT/'app/.build/pic/src/app.c.o'], expected=1)
    assert b'support defines generated main' in p.stderr
    p=run(['python3',TOOL/'entry-build.py','bundle','--kind','module','--id','bad','--output',ROOT/'bad.o','--script',ROOT/'bad.ld','--export','Missing',ROOT/'a/.build/pic/src/module.c.o'],expected=1)
    assert b'required module symbol is not defined' in p.stderr
    assert not (ROOT/'bad.o').exists()
    print('PASS entry/module/assets: reuse, collision isolation, typed calls, state reset, constructors, no-op, -j4')
finally:
    (ROOT/'receipt.json').write_text(json.dumps(dict(results=results,root=str(ROOT)),indent=2)+'\n')
    print('Results:',ROOT)
