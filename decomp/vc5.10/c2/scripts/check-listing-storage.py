#!/usr/bin/env python3
"""Focused data-identity and nonzero code-side check; no original execution."""
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import time
from data_evidence import reader, model
from state import BINARY, ROOT, TARGET, load, sha


def check(output):
    begin = time.monotonic()
    config, image, inventory = load()
    maps = {m[1]: int(m[2], 16) for m in re.finditer(
        r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',
        (BINARY/'build/c2.map').read_text(), re.M)}
    assert '_compiling_function' not in maps, 'duplicate invented storage object'
    declaration = (BINARY/'src/c2.h').read_text()
    assert 'extern int c2_internal_listing;' in declaration
    assert 'extern C2Symbol *current_symbol;' in declaration
    value = model(config)
    objects = [r for r in value['objects'] if r['original'] == 0x490b78]
    assert objects == [{'original': 0x490b78, 'native': 'c2_internal_listing', 'kind': 'int', 'initial': 0}]
    selectors = next(r for r in value['selectors'] if r['native'] == 'c2_machine_selectors')
    assert selectors['original'] == 0x49a358
    record = next(r for r in config['driver'] if r['name'] == '@update_symbol_storage@4')
    assert record['start'] == 0x4283e3
    assert record['code_chunks'] == [[0x46bf43, 0x46c001]]
    original, rebuilt = reader(TARGET), reader(BINARY/'build/c2.exe')
    for read, table, flag in ((original, 0x49a358, 0x490b78),
                              (rebuilt, maps['_c2_machine_selectors'], maps['_c2_internal_listing'])):
        assert struct.unpack('<III', read(table+24,12)) == (ord('i'),1,flag), 'selector must alias integer flag'
    assert original(0x428481,5) == bytes.fromhex('a1780b4900')
    assert original(0x4284a6,7) == bytes.fromhex('a1780b490085c0')
    assert original(0x46bf53,13) == bytes.fromhex('8b0d1c5f49008bd6e891a3feff'), 'original current_symbol ECX/header EDX call differs'
    base = maps['@update_symbol_storage@4']
    assert rebuilt(base+0xef,7) == b'\x83\x3d'+struct.pack('<I',maps['_c2_internal_listing'])+b'\0'
    assert rebuilt(base+0x161,7) == b'\x83\x3d'+struct.pack('<I',maps['_c2_internal_listing'])+b'\0'
    at = base+0x10a
    assert rebuilt(at,6) == b'\x8b\x0d'+struct.pack('<I',maps['_current_symbol'])
    assert rebuilt(at-3,3) == bytes.fromhex('8b55dc'), 'header must be second fastcall argument EDX'
    assert rebuilt(at+6,1) == b'\xe8'
    assert at+11+struct.unpack('<i',rebuilt(at+7,4))[0] == maps['@select_symbol_section@8']
    for r in config['driver']+config['stubs']:
        assert all('compiling_function' not in s['native'] for s in r.get('state',[]))
        for state in r.get('state',[]):
            if state['original'] == '0x490b78':assert state['native'] == '(unsigned long)c2_internal_listing'
    source = (BINARY/'src/sections.c').read_text()
    flags = ['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-Wno-ignored-attributes',
             '-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),
             '-I'+str(BINARY/'src'),'-O0','-g','-ffunction-sections','-fdata-sections']
    conditions = []
    scratch = ROOT.parent/'.ai-tmp/msvc2/vc5.10/c2'
    with tempfile.TemporaryDirectory(prefix='listing-storage-',dir=scratch) as temp:
        temp = Path(temp)
        variants = [('actual',source,0),
                    ('split-flag',source.replace('C2Section *default_storage_section;',
                       'static int wrong_listing;\nC2Section *default_storage_section;').replace('if (c2_internal_listing &&','if (wrong_listing &&'),-6),
                    ('wrong-owner',source.replace('(C2FunctionSymbol *)current_symbol','(C2FunctionSymbol *)&c2_internal_listing'),-6)]
        for name, content, status in variants:
            path = temp/(name+'.c');path.write_text(content)
            exe = temp/name
            compiled = subprocess.run(flags+[str(path),str(BINARY/'scripts/check-listing-storage.c'),'-Wl,--gc-sections','-o',str(exe)],capture_output=True)
            assert compiled.returncode == 0, compiled.stderr.decode()
            run = subprocess.run([str(exe)],capture_output=True)
            assert run.returncode == status, (name,run.returncode)
            conditions.append({'name':name,'status':run.returncode,'source_sha256':sha(content.encode())})
    result = {'status':'PASS','target_sha256':sha(image),'rebuilt_sha256':sha((BINARY/'build/c2.exe').read_bytes()),
              'map_sha256':sha((BINARY/'build/c2.map').read_bytes()),'configuration_sha256':sha((BINARY/'functions.json').read_bytes()),
              'inputs':{str(p.relative_to(BINARY)):sha(p.read_bytes()) for p in [BINARY/'src/sections.c',BINARY/'src/c2.h',BINARY/'src/options.c',BINARY/'src/option-table.c',BINARY/'scripts/check-listing-storage.c',Path(__file__).resolve()]},
              'condition_variants':conditions,'seconds':time.monotonic()-begin,
              'scope':'Original/rebuilt PE and actual MAP typed storage/selector/guard/fastcall identity proof plus actual C condition test. Nonzero C tests are code-side checks, not original execution or new coverage. Historical zero-only captures remain unchanged.'}
    output.write_text(json.dumps(result,indent=2)+'\n')
    return result

if __name__ == '__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    print(json.dumps(check(p.parse_args().output.resolve()),indent=2))
