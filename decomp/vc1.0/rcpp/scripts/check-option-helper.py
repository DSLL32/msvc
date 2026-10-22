#!/usr/bin/env python3
"""Minimal direct paired case; the production preprocessor has no caller."""
import concurrent.futures
import hashlib
import json
import re
import subprocess
from pathlib import Path
from compare import HERE, SCRATCH, indexes, mask, wt

NAME = '_rc_option_contains_character@8'

def compare_direct(count=None):
    cfg, original, reconstructed, oi, ri = indexes()
    accepted = cfg['accepted'] if count is None else cfg['accepted'][:count]
    runs = {}
    directory = SCRATCH/'option-helper'
    directory.mkdir(exist_ok=True)

    def run(label, pe, records):
        binary = directory/(label+'.exe')
        mask(pe, records, accepted, binary)
        function = next(f for f in records if f['name'] == NAME)
        stem = directory/label
        args = subprocess.list2cmdline([wt.windows(binary),
            '%x' % (function['start']-pe.base)])
        subprocess.run(['wine', 'build/debug-run.exe', wt.windows(HERE/'build/option-check.exe'),
            args, wt.windows(str(stem)+'.stdout'), wt.windows(str(stem)+'.stderr'),
            wt.windows(str(stem)+'.events')], env=wt.environment(), check=True,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=45)
        events = Path(str(stem)+'.events').read_text()
        stdout = Path(str(stem)+'.stdout').read_bytes()
        stderr = Path(str(stem)+'.stderr').read_bytes()
        if NAME not in accepted:
            match = re.fullmatch(r'exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+) first 1\nexit 99\n', events)
            assert match, events
            address = (int(match[1],16)+int(match[2],16)) & 0xffffffff
            mapped = re.fullmatch(rb'mapped ([0-9a-f]{8})\r\n', stdout)
            assert mapped and not stderr, (stdout,stderr)
            assert address == int(mapped[1],16)+function['start']-pe.base, (label,hex(address),function)
            result = NAME
        else:
            assert events == 'exit 0\n', events
            assert re.fullmatch(rb'mapped [0-9a-f]{8}\r\n10 examples and stdcall ABI match\r\n', stdout) and not stderr
            result = 'exit:0'
        return label, dict(result=result, events=events, stdout=stdout.decode('ascii'),
            stderr=stderr.decode('ascii'), binary_sha256=hashlib.sha256(pe.data).hexdigest(),
            rva=function['start']-pe.base)

    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        for label, result in pool.map(lambda args: run(*args),
                [('original',original,oi),('reconstructed',reconstructed,ri)]):
            runs[label] = result
    assert runs['original']['result'] == runs['reconstructed']['result']
    result = runs['original']['result']
    path = HERE/'option-helper-validation.json'
    report = json.loads(path.read_text()) if path.exists() else {}
    report['scope'] = 'direct leaf case; no production caller or public option reachability claimed'
    report['samples'] = 10
    report['abi'] = 'stdcall(const char *, int), low eight bits of second argument; exact int 0/1; 8-byte callee cleanup and EBX/ESI/EDI/EBP preservation'
    report['frontier' if result == NAME else 'accepted'] = runs
    path.write_text(json.dumps(report,indent=2)+'\n')
    print('option-helper: original: %s; reconstruction: %s' % (result,result), flush=True)
    return result

if __name__ == '__main__':
    cfg = json.loads(Path('functions.json').read_text())
    if NAME in cfg['accepted']:
        assert compare_direct(cfg['accepted'].index(NAME)) == NAME
        assert compare_direct() == 'exit:0'
    else:
        assert compare_direct() == NAME
