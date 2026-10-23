#!/usr/bin/env python3
"""RC-only PE data/layout assertions; graph and callbacks use shared Rust."""
import hashlib
import json
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM
import reference_indexes as compare
from binary import u32

decoder = Cs(CS_ARCH_X86, CS_MODE_32)
decoder.detail = True


def audit_data(original, reconstruction, oi, ri, cfg):
    """Verify FILE/C-locale storage and signed Kanji field accesses."""
    names = compare.publics()
    report = {}
    original_iob, reconstructed_iob = 0x101d3b0, names['__iob']
    left_last = u32(original.read(0x101d630,4),0) - original_iob
    right_last = u32(reconstruction.read(names['__lastiob'],4),0) - reconstructed_iob
    assert left_last == right_last == 19 * 32
    for index in range(20):
        for offset in (12,16):
            assert original.read(original_iob+index*32+offset,4) == reconstruction.read(reconstructed_iob+index*32+offset,4)
    report['standard_streams'] = dict(count=20, sizeof_FILE=32, flag_offset=12,
                                      descriptor_offset=16, last_stream_offset=left_last,
                                      diagnostics_stream_index=1)
    left_table = u32(original.read(0x101d710,4),0)
    right_table = u32(reconstruction.read(names['__pctype'],4),0)
    left, right = original.read(left_table-2,514), reconstruction.read(right_table-2,514)
    assert left == right
    assert original.read(0x101d3a0,4) == reconstruction.read(names['___mb_cur_max'],4)
    left_decimal = original.string(u32(original.read(0x101d3a8,4),0))
    right_decimal = reconstruction.string(u32(reconstruction.read(names['__decimal_point'],4),0))
    assert left_decimal == right_decimal == '.'
    report['C_locale'] = dict(character_classes_including_EOF=257,
                              ctype_sha256=hashlib.sha256(left).hexdigest(),
                              multibyte_max=u32(original.read(0x101d3a0,4),0), decimal_point=left_decimal)
    if '_is_kanji_lead@4' in cfg['accepted']:
        functions = [next(r for r in records if r['name']=='_is_kanji_lead@4') for records in (oi,ri)]
        for pe, record, base in [(original,functions[0],0x1018558),
                                  (reconstruction,functions[1],names['_kanji_ranges'])]:
            fields = set()
            for insn in decoder.disasm(pe.read(record['start'],record['end']-record['start']),record['start']):
                for operand in insn.operands:
                    if operand.type == X86_OP_MEM and not operand.mem.base and not operand.mem.index:
                        offset = operand.mem.disp - base
                        assert offset in (0,4,8,12) and operand.size == 2 and insn.mnemonic == 'movsx'
                        fields.add(offset)
            assert fields == {0,4,8,12}
            assert pe.read(base,16) == bytes(16)
        report['KANJI_bounds'] = dict(bytes=16, element_bytes=2, signed=True,
                                      field_offsets=[0,4,8,12], range_stride=8)
    Path('build/data-layout.json').write_text(json.dumps(report,indent=2)+'\n')


def main():
    original, rebuilt, cfg, oi, ri = compare.indexes()
    audit_data(original, rebuilt, oi, ri, cfg)
    print('RC FILE/C-locale/Kanji data layout agrees; graph/callback checks are shared Rust.')


if __name__ == '__main__':
    main()
