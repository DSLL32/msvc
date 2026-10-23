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
    complete = json.loads((raw / 'capture.json').read_text()).get('dependency_complete', False)
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
    def negative_many(name, changes):
        originals = {filename: (raw / filename).read_bytes() for filename in changes}
        try:
            for filename, mutate in changes.items():
                (raw / filename).write_bytes(mutate(originals[filename]))
            value['files'] = enumerate_files(raw)
            manifest.write_text(json.dumps(value))
            try:
                decode(manifest)
            except (AssertionError, KeyError, ValueError, struct.error):
                passed.append(name)
            else:
                raise AssertionError('accepted mutually consistent analysis mutation: ' + name)
        finally:
            for filename, data in originals.items():
                (raw / filename).write_bytes(data)
            value['files'] = copy.deepcopy(actual['files'])
            manifest.write_text(json.dumps(value))
    def register_change(phase, register, offset, amount):
        data = (raw / ('observer/' + phase + '-context.bin')).read_bytes()
        old = struct.unpack_from('<I', data, offset)[0]
        def event_change(data):
            lines = data.decode('ascii').splitlines(keepends=True)
            return ''.join(line.replace('%s=%08x' % (register, old), '%s=%08x' % (register, old + amount))
                           if line.startswith('snapshot phase=' + phase + ' ') else line for line in lines).encode('ascii')
        return {'observer/' + phase + '-context.bin': lambda b: integer(b, offset, old + amount),
                'observer/events.txt': event_change}
    negative('unbound raw change', 'observer/invoke-after-ctype.bin', lambda b: bytes([b[0] ^ 1]) + b[1:], False)
    negative('relabelled analysis as reconstruction', 'capture.json', lambda b: b.replace(b'"baseline": true', b'"baseline": false'))
    negative('changed canonical loaded C1', 'observer/loaded-c1.bin', lambda b: b[:-1] + bytes([b[-1] ^ 1]))
    negative('wrong stdcall cleanup', 'observer/invoke-after-context.bin', lambda b: integer(b, 196, 0x31f57c))
    negative('wrong BOOL argument', 'observer/invoke-before-stack.bin', lambda b: integer(b, 8, 0))
    negative('locale reset during unload', 'observer/free-after-crt-words.bin', lambda b: integer(b, 24, 0))
    ir_gl = next(name for name in actual['files'] if name.startswith('observer/c1-output/') and name.endswith('gl'))
    negative('different captured IR', ir_gl, lambda b: b + b'changed')
    negative('LastError transition omitted', 'mktemp-access-9-after-teb.bin', lambda b: integer(b, 0x34, 0))
    negative('Win32 versus CRT stream alias', 'observer/invoke-after-parameters.bin', lambda b: integer(b, 0x20, 999))
    negative('extra child invocation', 'observer/events.txt', lambda b: b + b'invoke-call argc=26 argv=00892520 last=1 function=10612229 esp=0031f574\n')
    negative_many('mutually consistent wrong Invoke export pointer', register_change('get-invoke-after', 'eax', 176, 1))
    negative_many('mutually consistent wrong Abort export pointer', register_change('get-abort-after', 'eax', 176, 1))
    negative_many('mutually consistent wrong LoadLibrary result', register_change('load-after', 'eax', 176, 1))
    negative_many('mutually consistent LoadLibrary cleanup', register_change('load-after', 'esp', 196, 4))
    negative_many('mutually consistent GetProcAddress cleanup', register_change('get-invoke-after', 'esp', 196, 4))
    negative_many('mutually consistent FreeLibrary cleanup', register_change('free-after', 'esp', 196, 4))
    negative('spawn mode changed', 'observer/spawn-before-stack.bin', lambda b: integer(b, 0, 1))
    negative('spawn path pointer changed', 'observer/spawn-before-stack.bin', lambda b: integer(b, 4, 1))
    negative('spawn argv pointer changed', 'observer/spawn-before-stack.bin', lambda b: integer(b, 8, 1))
    if complete:
        negative('assignment trailing space changed', 'observer/putenv-2-before-assignment.bin', lambda b: b[:-2] + b'x\0')
        negative('OS and CRT command environments disagree', 'observer/putenv-2-after-os-environment.bin', lambda b: b.replace('test.obj'.encode('utf-16le'), 'tent.obj'.encode('utf-16le')))
        negative('child return status changed', 'observer/spawn-after-context.bin', lambda b: integer(b, 176, 1))
        negative('actual spawn executable changed', 'observer/spawn-executable.bin', lambda b: b[:-1] + bytes([b[-1] ^ 1]))
        negative('child output differs from final artifact', 'observer/c2-output/test.obj', lambda b: b[:-1] + bytes([b[-1] ^ 1]))
        negative('child mutates parent CRT namespace', 'observer/spawn-after-crt-environment.bin', lambda b: b.replace(b'test.obj', b'tent.obj'))
        old_first = struct.unpack_from('<I', (raw / 'observer/putenv-1-before-crt-environment.bin').read_bytes(), 8)[0]
        negative('initial putenv retains original string pointer', 'observer/putenv-1-after-crt-environment.bin', lambda b: integer(b, 8, old_first))
        negative('second putenv replaces existing string pointer', 'observer/putenv-2-after-crt-environment.bin', lambda b: integer(b, 8, 1))
    print('dependency analysis packet positive control and %d negatives pass; no native provider or acceptance' % len(passed))
