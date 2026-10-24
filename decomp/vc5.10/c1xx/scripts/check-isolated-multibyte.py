#!/usr/bin/env python3
"""Own original _setmbcp(-4) witness; no shared/native success is implied."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import time

from allocation_imports import export_rva, pe_read
from scanner_projection import defaults


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, message):
    if not ok:
        raise ValueError('isolated multibyte: ' + message)


def check(directory):
    directory = directory.resolve()
    started = time.perf_counter()
    path = Path(__file__).with_name('check-isolated-locale.py')
    spec = importlib.util.spec_from_file_location('multibyte_locale_prerequisite', path)
    locale = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(locale)
    prerequisite = locale.check(directory)
    trial = json.loads((directory / 'trial.json').read_text())
    require(trial['trial'] in ('_initialize_multibyte_categories','_initialize_lowercase_table','_read_env_arguments','_new_option_vector','_next_argument','_match_option','_apply_option_action','_parse_option_integer') and trial['multibyte_observers'] == {'entry_rva':0x39394,'return_rva':0x38fe9,'public_entry_rva':0x2257,'IAT_rva':0xc9184,'data_IAT_rva':0xc90c4,'codepage':-4,'next_rva':0x393ca,'flags_rva':0xcc108,'classes_rva':0xcc008,'bytes':512,'selection_support':'unclaimed until own203 captured and reviewed'}, 'wrong actual candidate/observer contract')
    provider, target = directory / 'loaded-msvcrt.bin', directory / 'loaded-target.bin'
    require(export_rva(provider, '_setmbcp') == 0x2257 and pe_read(target, 0x39398, 6) == bytes.fromhex('ff1584914c10') and pe_read(target, 0x39396, 2) == bytes.fromhex('6afc'), 'actual canonical public import/call/argument')
    require((directory / 'mb-provider-entry.bin').read_bytes() == pe_read(provider, 0x2257, 16), 'actual loaded provider entry bytes')
    text = (directory / 'events.txt').read_text()
    base = prerequisite['phases']['before']['words'][0]
    require(re.findall(r'^mb-observer canonical-export=00002257 actual=(\w+) iat=000c9184 direct-data-IAT=000c90c4 resolved=(\w+) no-query-calls=1 no-seeds=1$', text, re.M) == [('%08x' % (base+0x2257), '%08x' % (base+0x3f1c8))], 'actual code/direct-data import identity')
    entry = re.findall(r'^mb-entry seq=(\d+) pc=(\w+) esp=(\w+) return=(\w+) codepage=(-?\d+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$', text, re.M)
    returned = re.findall(r'^mb-return seq=(\d+) pc=(\w+) esp=(\w+) entry-esp=(\w+) result=(-?\d+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$', text, re.M)
    require(len(entry) == len(returned) == 1, 'public call inventory')
    entry, returned = entry[0], returned[0]
    require(int(entry[1],16) == base+0x2257 and int(entry[3],16) == int(returned[1],16) == 0x1043939e and int(entry[4]) == -4 and returned[4] == '0' and entry[0] == returned[0] and int(returned[2],16) == int(entry[2],16)+4 and returned[3] == entry[2] and entry[5:] == returned[5:], 'actual cdecl4 argument/result/RET0/nonvolatile ABI')
    observed = {}
    before_words = None
    for phase in ('mb-before','mb-after','mb-helper-return','mb-next'):
        def read(name):
            return (directory / ('locale-' + phase + '-' + name + '.bin')).read_bytes()
        words = list(struct.unpack('<18I', read('words')))
        if before_words is None:
            before_words = words
        expected = before_words[:]
        if phase != 'mb-before':
            expected[10] = 1252
        require(words == expected and words[1:11] == [1,1252,0,0,0x409,0,0,0,0,0 if phase == 'mb-before' else 1252], 'full scalars/handles and independent MB gate/codepage')
        require(read('errors') == struct.pack('<3I',0,0,203), 'own errno/DOS/Last203 preservation')
        require(read('fp') == (directory / 'locale-mb-before-fp.bin').read_bytes() and len(read('fp')) == 112, 'full112 FP preservation')
        # All current/old tables, pointers and locale result remain live. The
        # mapped ranges are observations, never inferred heap object extents.
        for name, width in (('mbctype',257),('ctype',514),('old-ctype',514),('result',len(b'English_United States.1252\0'))):
            require(len(read(name)) == width and read(name) == (directory / ('locale-next-' + name + '.bin')).read_bytes(), 'complete table/string identity and lifetime: ' + name)
        for name in ('ctype-region','old-ctype-region','result-region'):
            require(read(name) == (directory / ('locale-next-' + name + '.bin')).read_bytes(), 'pointer mapped-range identity: ' + name)
        for name, baseline in (('CRT-argv','original-crt-argv.bin'),('CRT-environment','original-crt-environment.bin'),('Invoke-argv','invoke-entry-argv.bin')):
            require(read(name) == (directory / baseline).read_bytes(), 'private ordered vector storage/content: ' + name)
        for name in ('teb','ptd'):
            require(read(name) == (directory / ('locale-mb-before-' + name + '.bin')).read_bytes(), 'actual thread/PTD lifetime: ' + name)
        for prefix, suffix in (('frontend-data-locale-','.bin'),('frontend-snapshot-locale-','.bin'),('locale-','-target-BSS.bin')):
            expected_target=(directory / (prefix+'next'+suffix)).read_bytes()
            if trial['trial'] in ('_initialize_lowercase_table','_read_env_arguments','_new_option_vector','_next_argument','_match_option','_apply_option_action','_parse_option_integer') and trial.get('test_seed') and phase=='mb-next' and prefix=='locale-':
                # A labeled later helper-entry DATA seed follows the MB helper
                # return; it is not an API effect or a relaxed neighbor guard.
                require(text.count('path-fold-seed phase=helper bytes=512 first=4303 step=37\n')==1 and (directory/'path-fold-helper-before-seed.bin').read_bytes()==bytes(512),'explicit later512-byte auxiliary handoff')
                expected_target=bytearray(expected_target);offset=0xc13f8-0xb9000
                expected_target[offset:offset+512]=struct.pack('<256H',*[(0x4303+37*i)&65535 for i in range(256)])
            require((directory / (prefix+phase+suffix)).read_bytes() == expected_target, 'complete original target live/snapshot/BSS preservation with exact labeled later seed')
        observed[phase] = dict(words=words,errors=[0,0,203],FP_sha256=sha(directory/('locale-'+phase+'-fp.bin')),table_sha256={name:sha(directory/('locale-'+phase+'-'+name+'.bin'))for name in ('mbctype','ctype','old-ctype','result')})
    require((directory/'locale-mb-before-words.bin').read_bytes() == (directory/'locale-next-words.bin').read_bytes(), 'actual helper-entry to API prestate')
    require(all(not (byte & 4) for byte in (directory/'locale-mb-after-mbctype.bin').read_bytes()[1:]), 'actual complete SBCS unsigned-byte lead domain')
    actual_defaults = b''.join(bytes(defaults()[name]) for name in ('classes','flags'))
    require(pe_read(target,0xcc008,512) == actual_defaults, 'own full semantic/default canonical table identity')
    data = json.loads((directory/'scanner-data.json').read_text())
    require(data['full_default_sha256'] == hashlib.sha256(actual_defaults).hexdigest() and data['target_sha256'] == sha(target), 'case default metadata target binding')
    compiled_hash=trial['bound_inputs']['build/c1n32_l.dll'] if trial['trial']in ('_read_env_arguments','_new_option_vector','_next_argument','_match_option','_apply_option_action','_parse_option_integer') else data['compiled_original_DLL']['artifact_sha256']
    require(sha(directory/'c1n32_l.dll') == compiled_hash and pe_read(directory/'c1n32_l.dll',data['compiled_original_DLL']['frontend_data_rva']+2028,512) == actual_defaults, 'actual frozen original-compiler DLL/contextual initializer bytes')
    if not trial.get('test_seed'):
        require((directory/'frontend-data-locale-mb-next.bin').read_bytes()[8:520] == actual_defaults, 'natural complete512B target table result')
    helpers = re.findall(r'^mb-helper-return pc=(\w+) esp=(\w+) eax=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+) error=(\d+)$',text,re.M)
    traps = re.findall(r'^trap seq=(\d+) rva=00039394 .*esp=(\w+).*stack=([0-9a-f,]+)$',text,re.M)
    require(len(helpers) == len(traps) == 1 and int(helpers[0][0],16) == int(traps[0][2].split(',')[0],16) == 0x10438fe9 and int(helpers[0][1],16) == int(traps[0][1],16)+4 and helpers[0][-1] == '203', 'actual application helper caller/RET0/error')
    return dict(kind='own original-only canonical _setmbcp(-4) Last203 witness; native success unclaimed',original_directory=str(directory),original_receipt_sha256=sha(directory/'receipt.json'),target_sha256=sha(target),provider_sha256=sha(provider),trial_sha256=sha(directory/'trial.json'),checker_sha256=sha(Path(__file__)),locale_prerequisite_checker_sha256=sha(path),phases=observed,complete_target_table_bytes=512,old_current_tables_result_vectors_errors_FP_preserved=True,internal_MB_codepage_before=0,internal_MB_codepage_after=1252,independent_MB_gate=0,actual_public_return=0,next_target_rva=0x393ca,seconds=time.perf_counter()-started)


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original',type=Path)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    result=check(args.original)
    if args.output:
        require(not args.output.exists(),'refuse validation receipt overwrite')
        args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',original_receipt_sha256=result['original_receipt_sha256'],seconds=result['seconds'],native_success_claimed=False)))
