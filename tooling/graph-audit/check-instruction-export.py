#!/usr/bin/env python3
"""Focused decoded-instruction export controls; static files only, no target run."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time


def sha(data):
    return hashlib.sha256(data).hexdigest()


def semantic(value):
    return sha(json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(',', ':')).encode())


def image(code):
    b = bytearray(0x600)
    def w(at, value):
        struct.pack_into('<I', b, at, value)
    b[:2] = b'MZ'
    w(60, 0x80)
    b[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HH', b, 0x84, 0x14c, 1)
    struct.pack_into('<H', b, 0x94, 0xe0)
    struct.pack_into('<H', b, 0x98, 0x10b)
    w(0xb4, 0x400000)
    b[0x178:0x17d] = b'.text'
    for offset, value in [(8, 0x400), (12, 0x1000), (16, 0x400), (20, 0x200), (36, 0x60000020)]:
        w(0x178 + offset, value)
    b[0x200:0x600] = b'\xcc' * 0x400
    b[0x200:0x200 + len(code)] = code
    b[0x280] = 0xc3
    return bytes(b)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--scratch', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    root = args.scratch.resolve()
    assert root.is_relative_to(Path.home() / '.ai-tmp')
    root.mkdir(parents=True, exist_ok=True)
    binary = args.binary.resolve()
    results = []

    def run(name, code=b'\x90\xc3', metadata=None, mutate=None, flags=None, error=None, export=True):
        raw = image(code)
        cfg = dict(target=dict(path=str(root / 'original.exe'), sha256=sha(raw)), accepted=['_owner'], driver=[dict(name='_owner', start=0x401000, end=0x401000 + len(code))])
        runtime = [dict(name='_pending', start=0x401080, end=0x401081, pending_extent=True)]
        meta = copy.deepcopy(metadata) if metadata is not None else dict(schema=1, functions={})
        if mutate:
            mutate(cfg, runtime, meta)
        (root / 'original.exe').write_bytes(raw)
        (root / 'rebuilt.exe').write_bytes(raw)
        (root / 'config.json').write_text(json.dumps(cfg))
        (root / 'runtime.json').write_text(json.dumps(runtime))
        (root / 'metadata.json').write_text(json.dumps(meta))
        publics = [('_owner', 0x401000), ('_owner_end', 0x401000 + len(code)), ('_pending', 0x401080), ('_pending_end', 0x401081)]
        if 'code_chunks' in cfg['driver'][0]:
            publics[1] = ('_owner_end', cfg['driver'][0]['end'])
        (root / 'fixture.map').write_text(''.join(f' 0001:{v-0x400000:08x} {n} {v:08x}\n' for n, v in publics))
        command = [str(binary), '--profile', 'cl', '--repo', str(root), '--project', str(root), '--rebuilt', str(root / 'rebuilt.exe'), '--config', str(root / 'config.json'), '--map', str(root / 'fixture.map'), '--runtime', str(root / 'runtime.json'), '--rebuilt-runtime', str(root / 'runtime.json'), '--metadata', str(root / 'metadata.json')]
        command += flags if flags is not None else ['--check-ret', '--check-ownership', '--check-extents']
        if export:
            command.append('--export-instructions')
        # Existing report files must not be overwritten by an audit rejection.
        report_file = root / 'report.json'
        report_file.write_text('existing report sentinel\n')
        command += ['--report', str(report_file)]
        started = time.perf_counter()
        q = subprocess.run(command, capture_output=True, text=True)
        elapsed = time.perf_counter() - started
        if error is not None:
            assert q.returncode and error in q.stderr, (name, q.returncode, q.stderr)
            assert not q.stdout and report_file.read_text() == 'existing report sentinel\n', name
            result = None
        else:
            assert q.returncode == 0, (name, q.stderr)
            assert json.loads(report_file.read_text()) == json.loads(q.stdout)
            result = json.loads(q.stdout)
            if export:
                facts = result['decoded_instructions']
                assert facts['schema'] == 1 and 'not execution permission' in facts['semantics']
                assert set(facts['functions']) == set(result['functions'])
                assert facts['inputs']['config_sha256'] == semantic(cfg)
                assert facts['inputs']['metadata_sha256'] == semantic(meta)
                assert facts['inputs']['accepted_names_sha256'] == semantic(sorted(cfg['accepted']))
                for owner in facts['functions'].values():
                    for side in ('original', 'rebuilt'):
                        part = owner[side]
                        assert part['record_sha256'] == semantic(part['record'])
                        assert not part['record']['pending_extent']
                        starts = []
                        for row in part['instructions']:
                            address, data = row['address'], bytes.fromhex(row['bytes_hex'])
                            assert 1 <= len(data) <= 15
                            assert data == raw[0x200 + address - 0x401000:0x200 + address - 0x401000 + len(data)]
                            starts.append(address)
                        assert starts == sorted(set(starts))
        results.append(dict(case=name, accepted=q.returncode == 0, seconds=elapsed, error=error))
        return result

    baseline = run('flag off exact ordinary report', export=False)
    positive = run('paired exact instructions and semantic bindings')
    stripped = copy.deepcopy(positive)
    del stripped['decoded_instructions']
    assert stripped == baseline
    part = positive['decoded_instructions']['functions']['_owner']['original']
    assert part['effective_roots'] == [0x401000]
    assert part['checked_ret_cleanup_set'] == [0]
    assert [r['bytes_hex'] for r in part['instructions']] == ['90', 'c3']
    result = run('unchecked RET remains explicitly null', flags=[])
    assert result['decoded_instructions']['functions']['_owner']['original']['checked_ret_cleanup_set'] is None
    result = run('pending callee is only a frontier not an exported owner', code=bytes.fromhex('e87b000000c3'))
    assert list(result['decoded_instructions']['functions']) == ['_owner']
    result = run('empty checked selection grants no decoded owner', mutate=lambda c, r, m: c.update(accepted=[]))
    assert result['decoded_instructions']['functions'] == {}
    run('selected pending extent cannot be exported', mutate=lambda c, r, m: c['driver'][0].update(pending_extent=True), error='cannot export pending_extent')
    metadata = dict(schema=1, functions={'_owner': dict(original=dict(text_data=[[0x401002, 0x401003]]), rebuilt=dict(text_data=[[0x401002, 0x401003]]))})
    result = run('declared data and unreachable suffix absent', code=bytes.fromhex('eb0199c3c3'), metadata=metadata)
    assert [r['address'] for r in result['decoded_instructions']['functions']['_owner']['original']['instructions']] == [0x401000, 0x401003]
    run('branch into excluded data rejects without partial report', code=bytes.fromhex('eb0099c3c3'), metadata=metadata, error='declared text data')
    def chunks(c, r, m):
        c['driver'][0].update(end=0x401002, code_chunks=[[0x401004, 0x401005]], rebuilt_code_chunks=[[0x401004, 0x401005]])
    result = run('hot cold chunks do not export intervening gap', code=bytes.fromhex('eb029090c3'), mutate=chunks)
    part = result['decoded_instructions']['functions']['_owner']['original']
    assert [r['address'] for r in part['instructions']] == [0x401000, 0x401004]
    assert part['effective_roots'] == [0x401000]
    result = run('valid overlapping streams retained as facts', code=bytes.fromhex('ebffc3c3'))
    assert [r['address'] for r in result['decoded_instructions']['functions']['_owner']['original']['instructions']] == [0x401000, 0x401001, 0x401003]
    run('consumer overlap policy can request strict existing guard', code=bytes.fromhex('ebffc3c3'), flags=['--check-instruction-overlap'], error='enters operand')
    run('metadata root outside owner rejects', metadata=dict(schema=1, functions={'_owner': dict(original=dict(roots=[0x401030]))}), error='CFG target outside extent')
    changed = run('record evidence changes are hash bound', mutate=lambda c, r, m: c['driver'][0].update(evidence='separately reviewed witness'))
    assert changed['decoded_instructions']['functions']['_owner']['original']['record_sha256'] != positive['decoded_instructions']['functions']['_owner']['original']['record_sha256']
    run('legacy explicit rejection', flags=['--discovery', 'legacy'], error='require --discovery cfg')
    # Select integrated CVPACK by replacing the single profile argument; no
    # metadata/image parsing should occur before its explicit export denial.
    command = [str(binary), '--profile', 'cvpack', '--project', str(root), '--export-instructions']
    q = subprocess.run(command, capture_output=True, text=True)
    assert q.returncode and 'instruction export requires a standalone CFG profile' in q.stderr and not q.stdout
    results.append(dict(case='integrated CVPACK explicit rejection', accepted=False))
    args.report.write_text(json.dumps(dict(binary_sha256=sha(binary.read_bytes()), scope=__doc__, results=results), indent=2) + '\n')
    print(f'Instruction export CLI: {len(results)} controls passed.')


if __name__ == '__main__':
    main()
