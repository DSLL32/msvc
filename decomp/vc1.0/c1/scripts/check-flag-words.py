#!/usr/bin/env python3
"""Reject prior flag identity mistakes using private canonical PE mutations."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time
from binary import PE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

BINARY = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    options = parser.parse_args()
    directory = options.directory.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    began = time.perf_counter()
    original = PE(BINARY/'build/c1.exe')
    baseline = json.loads((BINARY/'build/flag-words.json').read_text())
    assert baseline['status'] == 'pass'
    assert baseline['rebuilt_sha256'] == hashlib.sha256(original.data).hexdigest()
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    results = []
    for name, function, mutation in [
            ('scanner-parser-word', '_assign_allocated_text@4', 'word'),
            ('old-tag-bit-mask', '_begin_tag_definition@12', 'mask'),
            ('upper-halfword-offset', '_declare_named_entity@16', 'offset')]:
        row = next(r for r in baseline['raw'] if r['side'] == 'rebuilt' and
                   r['function'] == function and r['word'] == 'scanner_flags')
        instruction = next(decoder.disasm(bytes.fromhex(row['bytes']), row['address'], count=1))
        data = bytearray(original.data)
        where = original.offset(row['address'])
        if mutation == 'mask':
            assert instruction.mnemonic == 'test' and instruction.operands[1].imm == 8
            data[where+instruction.imm_offset] = 0x80
        else:
            address = instruction.operands[0].mem.disp
            if mutation == 'word':
                address += baseline['addresses'][1]['parser_flags']-baseline['addresses'][1]['scanner_flags']
            else:
                assert row['offset'] == 2
                address -= 2
            assert instruction.disp_size == 4
            offset = where+instruction.disp_offset
            data[offset:offset+4] = struct.pack('<I', address)
        image = directory/(name+'.exe')
        image.write_bytes(data)
        test_start = time.perf_counter()
        result = subprocess.run([sys.executable, str(BINARY/'scripts/audit-flag-words.py'),
                                 '--rebuilt', str(image), '--report', str(directory/(name+'.json'))],
                                cwd=BINARY, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (directory/(name+'.log')).write_bytes(result.stdout)
        assert result.returncode != 0 and b'flag-word or mask identity differs' in result.stdout, (name,result.stdout)
        results.append(dict(name=name,status='rejected',function=function,
                            original_instruction=row,seconds=time.perf_counter()-test_start))
    receipt = dict(status='pass',tests=results,seconds=time.perf_counter()-began,
                   executable_sha256=hashlib.sha256(original.data).hexdigest(),
                   policy='Private PE mutations; source, original image and case oracles unchanged.')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Three flag word/mask/offset mutations rejected:',directory/'receipt.json')


if __name__ == '__main__':
    main()
