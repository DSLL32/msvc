"""Exercise graph/extent safeguards, optionally comparing a saved real audit."""
import argparse
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import struct


def load(path):
    spec = importlib.util.spec_from_file_location('checked_audit', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Image:
    def __init__(self, code, patches=()):
        self.data = bytearray(b'\xcc' * 0x2000)
        self.data[:len(code)] = code
        for address, data in patches:
            self.data[address-0x1000:address-0x1000+len(data)] = data

    def read(self, address, size):
        if not 0x1000 <= address < 0x1000+len(self.data):
            raise ValueError('unbacked address')
        return self.data[address-0x1000:address-0x1000+size]


def relative(opcode, target, address):
    return opcode + struct.pack('<I', (target-address-5) & 0xffffffff)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, help='also run synthetic checks against the previous script')
    parser.add_argument('--reference', type=Path, help='saved original/rebuilt per-function edge records')
    args = parser.parse_args()
    audit = load(Path(__file__).with_name('audit-graph.py'))
    modules = [audit] + ([load(args.baseline)] if args.baseline else [])
    checks = []

    def check(name, code, expected, end=None, patches=(), truncated=False):
        image = Image(code, patches)
        record = dict(name='function', start=0x1000, end=0x1000+(len(code) if end is None else end))
        records = [record, dict(name='callee', start=0x3000, end=0x3001),
                   dict(name='other', start=0x4000, end=0x4001),
                   dict(name='later_alias', start=0x3000, end=0x3001)]
        for module in modules:
            try:
                edges = module.edges(image, records, record)
            except ValueError as error:
                assert truncated and 'extent ends' in str(error), (name, error)
            else:
                assert not truncated, name+': truncated operand accepted'
                assert edges == set(expected), (name, edges, expected)
        checks.append(name)

    call = lambda target, address: relative(b'\xe8', target, address)
    jump = lambda target, address: relative(b'\xe9', target, address)
    check('direct call and first alias', call(0x3000, 0x1000)+b'\xc3', ['callee'])
    check('changed callee', call(0x4000, 0x1000)+b'\xc3', ['other'])
    check('tail jump excludes fallthrough', jump(0x3000, 0x1000)+call(0x4000, 0x1005)+b'\xc3', ['callee'])
    check('conditional paths', b'\x74\x06'+call(0x3000, 0x1002)+b'\xc3'+call(0x4000, 0x1008)+b'\xc3', ['callee', 'other'])
    check('indirect call', b'\xff\xd0\xc3', ['indirect'])
    check('indirect jump excludes fallthrough', b'\xff\xe0'+call(0x3000, 0x1002)+b'\xc3', ['indirect'])
    check('prefixed return', b'\xf3\xc3'+call(0x3000, 0x1002)+b'\xc3', [])
    check('int3 excludes fallthrough', b'\xcc'+call(0x3000, 0x1001), [])
    check('transactional branch group', b'\xc7\xf8'+struct.pack('<I', 0x3000-0x1006)+call(0x4000, 0x1006)+b'\xc3', ['callee', 'other'])
    check('overlapping instruction streams', bytes.fromhex('7401b8e8000000eb02ccccc3'), ['0xeb001008'])

    switch = bytearray(b'\xcc'*64)
    switch[:7] = b'\xff\x24\x8d'+struct.pack('<I', 0x1010)
    switch[16:28] = struct.pack('<III', 0x1020, 0x1030, 0)
    switch[32:38] = call(0x3000, 0x1020)+b'\xc3'
    switch[48:54] = call(0x4000, 0x1030)+b'\xc3'
    check('embedded switch data and both arms', switch, ['callee', 'other'])
    switch[16:28] = struct.pack('<III', 0, 0, 0)
    check('unresolved switch stays indirect', switch, ['indirect'])

    handlers = bytearray(b'\xcc'*64)
    handlers[:6] = b'\x68'+struct.pack('<I', 0x2000)+b'\xc3'
    handlers[16:22] = call(0x4000, 0x1010)+b'\xc3'
    handlers[32:38] = call(0x3000, 0x1020)+b'\xc3'
    scope = struct.pack('<III', 0xffffffff, 0x1010, 0x1020)
    check('disconnected SEH filter and handler', handlers, ['callee', 'other'], patches=[(0x2000, scope)])
    check('scope with invalid filter ignored', handlers, [], patches=[(0x2000, struct.pack('<III', 0xffffffff, 0x4000, 0x1020))])
    memory = bytearray(handlers)
    memory[:6] = b'\xa1'+struct.pack('<I', 0x2000)+b'\xc3'
    check('memory displacement is not scope immediate', memory, [], patches=[(0x2000, scope)])
    handlers[:7] = b'\xc3\x68'+struct.pack('<I', 0x2000)+b'\xc3'
    check('SEH prefix scan includes bytes after return', handlers, ['callee', 'other'], patches=[(0x2000, scope)])

    check('truncated ret operand rejected', b'\xc2\x08\x00', [], end=2, truncated=True)
    check('truncated call operand rejected', call(0x3000, 0x1000), [], end=4, truncated=True)
    check('instruction across batch boundary', b'\x90'*255+b'\xc2\x08\x00', [])
    check('truncation across batch boundary rejected', b'\x90'*255+b'\xc2\x08\x00', [], end=257, truncated=True)

    # Prove main() still rejects unequal original/rebuilt graphs, rather than
    # merely checking the extraction helper's return value.
    original = Image(call(0x3000, 0x1000)+b'\xc3')
    rebuilt = Image(call(0x4000, 0x1000)+b'\xc3')
    record = dict(name='function', start=0x1000, end=0x1006)
    records = [record, dict(name='callee', start=0x3000, end=0x3001)]
    old_indexes = audit.indexes
    audit.indexes = lambda: (original, rebuilt, dict(driver=[record], accepted=['function']), records, records)
    try:
        with contextlib.redirect_stdout(io.StringIO()) as output:
            try:
                audit.main()
            except SystemExit as error:
                assert error.code == 1
            else:
                raise AssertionError('main accepted mismatching callees')
        assert 'original only' in output.getvalue()
    finally:
        audit.indexes = old_indexes
    checks.append('main rejects changed function graph')

    compared = 0
    if args.reference:
        original, rebuilt, config, oi, ri = audit.indexes()
        rows = []
        for side, pe, records in [('original', original, oi), ('rebuilt', rebuilt, ri)]:
            names = audit.address_names(records)
            for record in records:
                if record['name'] in set(config['accepted']):
                    rows.append(dict(side=side, name=record['name'], start=record['start'], end=record['end'],
                                     edges=sorted(audit.edges(pe, records, record, names))))
        assert rows == json.loads(args.reference.read_text()), 'real per-function graphs changed'
        compared = len(rows)
    print(json.dumps(dict(status='passed', checks=checks, implementations=len(modules), real_graphs=compared)))


if __name__ == '__main__':
    main()
