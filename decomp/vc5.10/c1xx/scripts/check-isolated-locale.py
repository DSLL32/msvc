#!/usr/bin/env python3
"""Read-only original isolated locale witness; grants no native/API acceptance."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import time

from allocation_imports import export_rva, pe_read
from scaler_state import fp, semantic


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, message):
    if not ok:
        raise ValueError('isolated locale: ' + message)


def vector(blob):
    require(len(blob) >= 12, 'private vector header absent')
    root, count = struct.unpack_from('<2I', blob)
    require(root and count < 4096, 'private vector bounds')
    offset = 8
    pointers = []
    for _ in range(count):
        require(offset + 8 <= len(blob), 'private vector entry absent')
        pointer, length = struct.unpack_from('<2I', blob, offset)
        offset += 8
        require(pointer and 0 < length <= 65536 and offset + length <= len(blob), 'private vector string bounds')
        text = blob[offset:offset + length]
        require(text[-1:] == b'\0' and b'\0' not in text[:-1], 'private vector terminator')
        offset += length
        pointers.append(pointer)
    require(offset == len(blob), 'private vector trailing bytes')
    return root, count, pointers


def checksum_tables():
    low, high, update = bytearray(256), bytearray(256), bytearray(256)
    for value in range(256):
        for bit in range(8):
            for tap in (0, 1, 3, 6, 8, 13, 15):
                shift = tap + bit
                if shift >= 16:
                    break
                destination = low if shift < 8 else high
                destination[value] ^= ((value >> (shift % 8)) & 1) << bit
        value_now = value
        for bit in range(1, 8):
            value_now ^= (value_now << 1) & (1 << bit)
            if bit > 2:
                value_now ^= (value_now << 3) & (1 << bit)
        update[value] = value_now
    return bytes(update + high + low)


def check(directory):
    start = time.perf_counter()
    directory = directory.resolve()
    receipt = json.loads((directory / 'receipt.json').read_text())
    require((receipt['kind'], receipt['mode'], receipt['exit']) == ('explicit-original-capture', 'masked', 125), 'wrong capture role')
    # Authenticate every captured artifact, including helper/compiler snapshots.
    for name, row in receipt['artifacts'].items():
        path = directory / name
        require(Path(name).name == name and path.is_file() and path.stat().st_size == row['size'] and sha(path) == row['sha256'], 'capture artifact drift: ' + name)
    trial = json.loads((directory / 'trial.json').read_text())
    require(trial['trial'] in ('_initialize_checksums','_initialize_multibyte_categories','_initialize_lowercase_table','_read_env_arguments','_new_option_vector','_next_argument','_match_option','_apply_option_action','_parse_option_integer','_match_option_suffix','_append_option_vector'), 'not a genuine encountered checksum/scanner candidate')
    for name, digest in trial['case_inputs'].items():
        require(sha(directory / Path(name).name) == digest, 'case input drift: ' + name)
    schema = json.loads((directory / 'locale-schema.json').read_text())
    require(sha(directory / 'dll-capture.exe') == receipt['helper_sha256'], 'actual compiled helper identity')
    provider, target = directory / 'loaded-msvcrt.bin', directory / 'loaded-target.bin'
    require(sha(provider) == receipt['canonical_crt_sha256'] == schema['provider_sha256'] and provider.stat().st_size == schema['provider_bytes'], 'actual provider hFile identity')
    require(sha(target) == receipt['target_sha256'] == schema['target_sha256'], 'actual target hFile identity')
    transport = (directory / 'locale-observer.inc').read_bytes()
    transport_refinement = None
    if hashlib.sha256(transport).hexdigest() != schema['transport_source']['sha256']:
        # The numeric observer adds only these immutable export-cache entries.
        # Removing exactly that addition must reproduce the authenticated old
        # observer byte for byte; every locale read/phase remains unchanged.
        added = b',{"isspace",0x1c0be,0},{"isdigit",0x1c065,0},{"atol",0x5d30,0}'
        require(trial['trial'] in ('_parse_option_integer','_match_option_suffix','_append_option_vector') and trial.get('integer_observers') and
                transport.count(added) == 1, 'unknown compiled observer extension')
        prior = transport.replace(added, b'', 1)
        require(hashlib.sha256(prior).hexdigest() == schema['transport_source']['sha256'],
                'observer delta exceeds three immutable numeric export entries')
        for name, rva in (('isspace', 0x1c0be), ('isdigit', 0x1c065), ('atol', 0x5d30)):
            require(export_rva(provider, name) == rva, 'numeric canonical export identity: ' + name)
        transport_refinement = dict(kind='exact three immutable export-cache entries only; no locale read/phase change',
                                    compiled_sha256=hashlib.sha256(transport).hexdigest(),
                                    prior_sha256=hashlib.sha256(prior).hexdigest())
    for name, rva in schema['exports'].items():
        require(export_rva(provider, name) == rva, 'canonical export identity: ' + name)
    for row in schema['canonical_instructions']:
        raw = bytes.fromhex(row['bytes'])
        require(pe_read(provider, row['rva'], len(raw)) == raw, 'canonical CRT instruction identity')
    call = schema['call']
    require(pe_read(target, call['call_instruction_rva'], 6) == bytes.fromhex(call['call_bytes']) and pe_read(target, call['arguments'][1] - 0x10400000, 1) == b'\0', 'actual call/literal identity')
    text = (directory / 'events.txt').read_text()
    entry = re.findall(r'^locale-entry seq=(\d+) pc=(\w+) esp=(\w+) return=(\w+) category=(\d+) argument=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$', text, re.M)
    returned = re.findall(r'^locale-return seq=(\d+) pc=(\w+) esp=(\w+) entry-esp=(\w+) result=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$', text, re.M)
    require(len(entry) == len(returned) == 1, 'public call inventory')
    entry, returned = entry[0], returned[0]
    base = int(re.search(r'^module name=MSVCRT.dll actual=(\w+) ', text, re.M | re.I)[1], 16)
    require(int(entry[1], 16) == base + schema['exports']['setlocale'] and int(entry[3], 16) == int(returned[1], 16) == 0x10400000 + call['caller_return_rva'], 'public entry/return PCs')
    require(int(entry[4]) == 2 and int(entry[5], 16) == call['arguments'][1] and int(returned[2], 16) == int(entry[2], 16) + 4 and returned[3] == entry[2] and returned[0] == entry[0] and entry[6:] == returned[5:], 'cdecl arguments/RET0/nonvolatile ABI')
    expected_frontier={'_append_option_vector':'000397db','_match_option_suffix':'000100ea','_parse_option_integer':'00039757','_apply_option_action':'00039779','_match_option':'00039632','_next_argument':'0003944a','_read_env_arguments':'0000682f','_initialize_lowercase_table':'00039413','_initialize_multibyte_categories':'000393ca'}.get(trial['trial'],'00039394')
    expected_frontiers=[expected_frontier]
    if trial['trial']=='_new_option_vector':
        expected_frontiers=['00039428']if trial.get('test_seed')else[]
        if not trial.get('test_seed'):
            public=struct.unpack('<12I',(directory/'signature-entry-registers.bin').read_bytes())
            provider_file=directory/'loaded-mspdb50.bin'
            require(sha(provider_file)=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'and export_rva(provider_file,'SigForPbCb')==0xf22d and public[1]==public[11]+0xf22d and list(public[7:11])==[0x1043909f,0x104d6620,3,0xffffffff]and(directory/'signature-input.bin').read_bytes()==b'CPP\0'and(directory/'signature-provider-entry.bin').read_bytes()==pe_read(provider_file,0xf22d,53)and re.findall(r'^dependency-frontier module=MSPDB50.dll rva=0000f22d seq=(\d+)$',text,re.M)==[str(public[0])],'authenticated actual signature entry only, no provider execution')
    require('locale-observer canonical-export=00005729 actual=%08x iat=000c914c no-query-calls=1 no-seeds=1' % (base + 0x5729) in text and re.findall(r'^frontier rva=(\w+)$', text, re.M) == expected_frontiers, 'isolated transport/final target frontier')
    require((directory / 'locale-provider-entry.bin').read_bytes() == pe_read(provider, 0x5729, 16), 'actual loaded provider entry bytes')
    observed, private = {}, {}
    first_words = None
    for phase in ('before', 'after', 'next'):
        def read(suffix):
            return (directory / ('locale-' + phase + '-' + suffix + '.bin')).read_bytes()
        words = struct.unpack('<18I', read('words'))
        errors = struct.unpack('<3I', read('errors'))
        require(words[0] == base and words[12] == base + 0x3fba8 and words[16] == base + 0x3fbaa, 'provider/static/old-table identity')
        require(errors == (0, 0, 203), 'actual errno/DOS/LastError')
        require(len(read('mbctype')) == 257 and len(read('ctype')) == len(read('old-ctype')) == 514, 'full table widths including EOF')
        require(len(read('fp')) == 112, 'complete FLOATING_SAVE_AREA')
        raw_teb, raw_ptd = read('teb'), read('ptd')
        require(len(raw_teb) == 4096 and len(raw_ptd) == 16 and struct.unpack_from('<2I', raw_ptd, 8) == errors[:2] and struct.unpack_from('<I', raw_teb, 0x34)[0] == errors[2], 'raw errors storage')
        states = re.findall(r'^locale-state phase=' + phase + r' teb=(\w+) tid=(\d+) tls-index=(\d+) ptd=(\w+) errno=(\d+) doserrno=(\d+) LastError=(\d+)$', text, re.M)
        require(len(states) == 1, 'actual TEB/PTD observation inventory')
        teb, tid, tls, ptd = int(states[0][0], 16), int(states[0][1]), int(states[0][2]), int(states[0][3], 16)
        require(tls < 64 and struct.unpack_from('<I', raw_teb, 0x18)[0] == teb and struct.unpack_from('<I', raw_teb, 0x24)[0] == tid and struct.unpack_from('<I', raw_teb, 0xe10 + tls * 4)[0] == ptd and tuple(map(int, states[0][4:])) == errors, 'actual thread/TLS/PTD identity')
        if first_words is None:
            first_words = words
        regions = {}
        for name, pointer, size in (('ctype', words[11] - 2, 514), ('old-ctype', words[16] - 2, 514), ('result', words[17], len(read('result')))):
            region = struct.unpack('<7I', read(name + '-region'))
            regions[name] = list(region)
            if pointer:
                require(region[4] == 0x1000 and region[0] <= pointer and pointer + size <= region[0] + region[3] and region[1] and region[5] in (4, 8), 'observed mapped range: ' + name)
            else:
                require(region == (0,) * 7 and not size, 'absent result region')
        private[phase] = {}
        for name, baseline in (('CRT-argv', 'original-crt-argv.bin'), ('CRT-environment', 'original-crt-environment.bin'), ('Invoke-argv', 'invoke-entry-argv.bin')):
            blob = read(name)
            require(blob == (directory / baseline).read_bytes(), 'private ordered vector/storage drift: ' + name)
            root, count, _ = vector(blob)
            if name == 'CRT-argv':
                require((root, count) == (words[14], words[13]), 'actual HOST CRT argv storage')
            if name == 'CRT-environment':
                require(root == words[15], 'actual CRT environ storage')
            private[phase][name] = dict(count=count, sha256=hashlib.sha256(blob).hexdigest())
        observed[phase] = dict(words=list(words), errors=list(errors), fp=semantic(fp(read('fp'))), result=read('result').decode('ascii'), mapped_regions=regions, table_sha256={name: hashlib.sha256(read(name)).hexdigest() for name in ('mbctype', 'ctype', 'old-ctype')})
    before, after, later = (observed[p] for p in ('before', 'after', 'next'))
    require(before['words'][1:11] == [1] + [0] * 9 and before['words'][11] == base + 0x3fbaa and not before['words'][17] and before['result'] == '', 'exact isolated initial locale state')
    require((directory / 'locale-before-ctype.bin').read_bytes() == pe_read(provider, 0x3fba8, 514), 'static initial ctype content identity')
    expected = before['words'][:]
    expected[2], expected[5], expected[11], expected[17] = 1252, 0x409, after['words'][11], int(returned[4], 16)
    require(after['words'] == expected and after['words'][11] != before['words'][11] and after['words'][17] and after['result'] == 'English_United States.1252\0', 'actual isolated transition')
    require(after == later and before['fp'] == after['fp'] and before['errors'] == after['errors'], 'return-to-next lifetime/FP/error stability')
    for phase in ('after', 'next'):
        require((directory / ('locale-' + phase + '-mbctype.bin')).read_bytes() == (directory / 'locale-before-mbctype.bin').read_bytes() and (directory / ('locale-' + phase + '-old-ctype.bin')).read_bytes() == (directory / 'locale-before-ctype.bin').read_bytes(), 'MB/retained-old-table lifetime')
        for prefix, suffix in (('frontend-data-locale-', '.bin'), ('frontend-snapshot-locale-', '.bin'), ('locale-', '-target-BSS.bin')):
            require((directory / (prefix + phase + suffix)).read_bytes() == (directory / (prefix + 'before' + suffix)).read_bytes(), 'full original target live/snapshot/BSS mutation')
    require(all((directory / (prefix + 'before.bin')).stat().st_size == 8520 for prefix in ('frontend-data-locale-', 'frontend-snapshot-locale-')), 'complete original live/saved widths')
    require((directory / 'locale-before-fp.bin').read_bytes() == (directory / 'locale-after-fp.bin').read_bytes() == (directory / 'locale-next-fp.bin').read_bytes(), 'raw112 FP changed across API')
    require((directory / 'checksum-taps-loaded.bin').read_bytes() == pe_read(target, 0xd5c58, 32) == struct.pack('<8i', 0, 1, 3, 6, 8, 13, 15, -1), 'encountered polynomial tap identities')
    tables = checksum_tables()
    require((directory / 'checksum-tables-checksum-return.bin').read_bytes() == tables and (directory / 'checksum-return-BSS.bin').read_bytes() == (directory / 'locale-before-target-BSS.bin').read_bytes(), 'complete original checksum output/API prestate')
    for phase in ('before', 'after', 'next'):
        bss = (directory / ('locale-' + phase + '-target-BSS.bin')).read_bytes()
        require(len(bss) == 63132 and bss[0xbe970 - 0xb9000:0xbec70 - 0xb9000] == tables, 'full checksum tables to next target')
    return dict(**({'transport_refinement': transport_refinement} if transport_refinement else {}), kind='original-only isolated canonical setlocale witness; no native success or function acceptance', original_directory=str(directory), original_receipt_sha256=sha(directory / 'receipt.json'), provider_sha256=sha(provider), target_sha256=sha(target), trial_sha256=sha(directory / 'trial.json'), schema_sha256=sha(directory / 'locale-schema.json'), checker_sha256=sha(Path(__file__)), phases=observed, private_vectors=private, complete_original_checksum_bytes=768, pointer_ownership='actual pointers/static-provider identities/mapped regions only; heap object extents UNPROVEN', full112_FP_unchanged=True, target_live_snapshot_BSS_unchanged=True, next_target_rva=int(expected_frontier,16), artifacts_checked=len(receipt['artifacts']), seconds=time.perf_counter() - start)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = check(args.original)
    if args.output:
        require(not args.output.exists(), 'refuse to overwrite an existing validation receipt')
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(status='PASS', original_receipt_sha256=result['original_receipt_sha256'], artifacts_checked=result['artifacts_checked'], seconds=result['seconds'], native_success_claimed=False)))
