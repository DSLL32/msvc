#!/usr/bin/env python3
"""Actual analysis packet controls; no original launch or function acceptance."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import struct
import tempfile

from dependency_packet import decode, enumerate_files


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('manifest', type=Path)
args = parser.parse_args()
actual = json.loads(args.manifest.read_text())
decode(args.manifest)
scratch = Path('/root/.ai-tmp/msvc2/vc5.10/cl')
with tempfile.TemporaryDirectory(prefix='dependency-controls-', dir=scratch) as work:
    work = Path(work)
    raw = work / 'raw'
    shutil.copytree(actual['directory'], raw)
    manifest = work / 'manifest.json'
    value = copy.deepcopy(actual)
    value['directory'] = str(raw)
    manifest.write_text(json.dumps(value))
    decode(manifest)
    passed = []
    def negative(name, filename, mutate, refresh=True):
        path = raw / filename
        old = path.read_bytes()
        path.write_bytes(mutate(old))
        if refresh:
            value['files'] = enumerate_files(raw)
            manifest.write_text(json.dumps(value))
        try:
            decode(manifest)
        except (AssertionError, KeyError, ValueError, struct.error):
            passed.append(name)
        else:
            raise AssertionError('accepted analysis packet mutation: ' + name)
        finally:
            path.write_bytes(old)
            value['files'] = copy.deepcopy(actual['files'])
            manifest.write_text(json.dumps(value))
    def integer(data, offset, replacement):
        result = bytearray(data)
        struct.pack_into('<I', result, offset, replacement)
        return bytes(result)
    negative('unbound raw change', 'observer/invoke-after-ctype.bin', lambda b: bytes([b[0] ^ 1]) + b[1:], False)
    negative('relabelled analysis as reconstruction', 'capture.json', lambda b: b.replace(b'"baseline": true', b'"baseline": false'))
    negative('changed canonical loaded C1', 'observer/loaded-c1.bin', lambda b: b[:-1] + bytes([b[-1] ^ 1]))
    negative('wrong stdcall cleanup', 'observer/invoke-after-context.bin', lambda b: integer(b, 196, 0x31f57c))
    negative('wrong BOOL argument', 'observer/invoke-before-stack.bin', lambda b: integer(b, 8, 0))
    negative('locale reset during unload', 'observer/free-after-crt-words.bin', lambda b: integer(b, 24, 0))
    negative('different captured IR', 'observer/c1-output/a01900gl', lambda b: b + b'changed')
    negative('LastError transition omitted', 'mktemp-access-9-after-teb.bin', lambda b: integer(b, 0x34, 0))
    negative('Win32 versus CRT stream alias', 'observer/invoke-after-parameters.bin', lambda b: integer(b, 0x20, 999))
    negative('extra child invocation', 'observer/events.txt', lambda b: b + b'invoke-call argc=26 argv=00892520 last=1 function=10612229 esp=0031f574\n')
    print('dependency analysis packet positive control and %d negatives pass; no native provider or acceptance' % len(passed))
