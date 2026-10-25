#!/usr/bin/env python3
"""CVPACK's initial data and callback contracts, separate from CFG/RET checks."""
import json
from pathlib import Path
import re
import struct
import subprocess
import sys


def output(*args):
    return subprocess.check_output(args, text=True)


def image_reader(path):
    # objdump owns PE decoding. This check only reads the declared CVPACK
    # contextual objects, including the zero-filled .bss section.
    raw = Path(path).read_bytes()
    sections = []
    for line in output('objdump', '-h', str(path)).splitlines():
        m = re.match(r'\s+\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)', line)
        if m:
            sections.append((m[1], *[int(m[i], 16) for i in (2, 3, 4)]))

    def read(address, size):
        for name, length, start, offset in sections:
            if start <= address and address + size <= start + length:
                if name == '.bss':
                    return bytes(size)
                begin = offset + address - start
                result = raw[begin:begin + size]
                assert len(result) == size, 'truncated contextual object'
                return result
        raise AssertionError('contextual address outside image: %x' % address)

    return read


def main():
    tool, case, rebuilt, map_path = sys.argv[1:]
    info = json.loads(output(tool, 'inspect', str(Path(case) / '.build/case.so')))
    functions = [f for f in info['functions'] if f['definition'] == 4]
    native = {f['native_symbol']: f for f in functions}
    policy = json.loads(Path('policy-data.json').read_text())['graph_metadata']['functions']
    linked = {}
    for line in Path(map_path).read_text().splitlines():
        m = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if m:
            linked[m[1]] = int(m[2], 16)
    # Native source inventory excludes shared shims and descriptor/case glue.
    objects = sorted(Path('.build/pic/src').glob('*.o'))
    symbols = []
    for line in output('nm', '--defined-only', '--format=posix', *map(str, objects)).splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[1] in ('T', 't'):
            symbols.append(parts[0])
    assert len(symbols) == len(set(symbols)) and set(symbols) == set(native), 'native/source registry differs'
    for f in functions:
        source = Path(policy[f['name']]['source']).read_text()
        assert '#include <recon.h>' in source
        if not f['implementation']:
            assert re.search(r'\b' + re.escape(f['native_symbol']) + r'\s*\([^{}]*\)\s*\{\s*__asm\s*\{\s*int 3\s*\}', source), 'unaccepted body must trap: ' + f['name']
    original = image_reader(Path('../../..') / info['original_path'])
    compiled = image_reader(rebuilt)
    targets = {0x10000 + f['rva']: linked[f['name']] for f in functions}
    data = json.loads(Path('data.json').read_text())
    word = lambda b: struct.unpack('<I', b)[0]
    for item in data:
        left = original(item['original'], item['size'])
        right = compiled(linked[item['symbol']], item['size'])
        if item.get('kind') == 'string-pointer-array':
            for i in range(0, item['size'], 4):
                a, b = word(left[i:i + 4]), word(right[i:i + 4])
                for n in range(4096):
                    x, y = original(a + n, 1), compiled(b + n, 1)
                    assert x == y, ('contextual string differs', item['name'], i // 4)
                    if x == b'\0':
                        break
                else:
                    raise AssertionError('unterminated diagnostic string')
        elif item.get('kind') == 'callback-table':
            stride, pointer = item['stride'], item['pointer_offset']
            assert (stride, pointer, item['size']) in ((8, 4, 256), (4, 0, 8), (8, 4, 104))
            for i in range(0, item['size'], stride):
                assert left[i:i + pointer] == right[i:i + pointer], 'callback fields differ'
                a, b = word(left[i + pointer:i + pointer + 4]), word(right[i + pointer:i + pointer + 4])
                assert a == 0 and b == 0 or a in targets and targets[a] == b, 'callback target differs'
        else:
            assert left == right, ('initial contextual data differs', item['name'])
    print('%d contextual data objects and %d application source/stub identities passed' % (len(data), len(functions)))


if __name__ == '__main__':
    main()
