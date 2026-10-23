"""Typed buffered-output projections; original object gaps remain UNMODELED."""
import re
import struct
import subprocess

FIELDS = {'data': (0, 0, 4), 'cursor': (4, 4, 4), 'capacity': (8, 8, 4),
          'relocated': (12, 12, 4), 'relocation_context': (16, 16, 4),
          'stream': (20, 20, 4), 'good': (24, 24, 4), 'mode': (28, 28, 1),
          'file_descriptor': (0x120, 32, 4)}
# Encountered constructor/object/destructor identities, not section contents.
KNOWN = [(0x54038, 0x123ae, 0x23940), (0x54b40, 0x12423, 0x23939)]
ZERO = {name: 0 for name in FIELDS}
INITIALIZED = dict(ZERO, good=1, file_descriptor=-1)


def objects(fields):
    rows = fields['output_buffers']
    assert 0 < len(rows) <= len(KNOWN)
    for index, row in enumerate(rows):
        base, constructor, destructor = KNOWN[index]
        assert row['index'] == index
        assert (row['original_object_rva'], row['constructor_rva'], row['destructor_rva']) == (base, constructor, destructor)
        assert row['constructor_native_symbol'] == 'initialize_output_buffer_%x' % constructor
        assert row['destructor_native_symbol'] == 'destroy_output_buffer_%x' % destructor
        assert row['native_size'] == 36 and row['native_offset'] == 232 + index * 36
        assert row['native_field'] == 'c1_bss.output_buffers[%d]' % index
        assert row['original_unmodeled_offsets'] == [[29, 288]] and row['object_extent_unknown']
    return rows


def expected_fields(fields):
    return {'output_buffers[%d].%s' % (row['index'], name):
            (row['original_object_rva'] + offset, size, row['native_offset'] + native)
            for row in objects(fields) for name, (offset, native, size) in FIELDS.items()}


def read(data, base):
    return {name: struct.unpack_from('<' + ('B' if size == 1 else 'i' if name == 'file_descriptor' else 'I'),
                                     data, base - 0x50000 + offset)[0]
            for name, (offset, native, size) in FIELDS.items()}


def validate(fields, original, bss, current):
    models = objects(fields)
    rows = {row['name']: row for row in fields['modeled_bss']}
    for name, (address, size, native) in expected_fields(fields).items():
        row = rows[name]
        assert (row['original_rva'], row['native_offset'], row['bytes'], row['default']) == (address, native, size, 0)
        assert row['native_field'] == 'c1_bss.' + name
    wanted_file = ''.join('%x %x %x\n' % (r['original_object_rva'], r['constructor_rva'], r['destructor_rva']) for r in models)
    assert (original / 'output-buffers.txt').read_text() == wanted_file
    assert len(current['bss']['output_buffers']) == len(models)
    defaults = (original / 'compiler-bss-000.bin').read_bytes()
    for row, native in zip(models, current['bss']['output_buffers']):
        assert read(defaults, row['original_object_rva']) == ZERO
        assert read(bss, row['original_object_rva']) == native == INITIALIZED


def seeded(index, allocation, callback):
    start = allocation + index * 16
    return dict(data=start, cursor=start + 4, capacity=47 + index,
                relocated=callback, relocation_context=start, stream=start + 8,
                good=9, mode=165 + index, file_descriptor=57 + index)


def check_phase(fields, original, sequence, native, module, original_alloc, native_alloc, symbols, seed=False):
    data = (original / ('compiler-bss-%03d.bin' % sequence)).read_bytes()
    for row, value in zip(objects(fields), native['bss']['output_buffers']):
        index = row['index']
        assert read(data, row['original_object_rva']) == (seeded(index, original_alloc, module + 0x12365) if seed else ZERO)
        assert value == (seeded(index, native_alloc, symbols['initialize_state_12365']) if seed else ZERO)
    assert len(native['bss']['output_buffers']) == len(objects(fields))


def validate_constructor_probe(fields, project, original, observations, raw, sequence,
                               module, original_alloc, native_alloc, symbols, plan):
    models = objects(fields)
    assert observations['pre_seed']['bss']['output_buffers'] == [ZERO] * len(models)
    assert plan == {'pointer_roles': 'own snapshot interior pointers; sentinel callback initialize_state_12365 is never invoked',
                    'capacity_base': 47, 'good': 9, 'mode_base': 165, 'descriptor_base': 57, 'normal_case': False}
    assert set(observations['buffer_constructors']) == {str(row['constructor_rva']) for row in models}
    symbol_text = subprocess.check_output(['nm', '-S', '--defined-only', str(project / 'build/native/c1-host')], text=True)
    for row in models:
        index, rva = row['index'], row['constructor_rva']
        seqs = re.findall(r'^trap seq=(\d+) rva=%08x ' % rva, raw, re.M)
        assert len(seqs) == 1 and int(seqs[0]) < sequence
        observation = observations['buffer_constructors'][str(rva)]
        before = observation['before']
        bss = (original / ('compiler-bss-%03d.bin' % int(seqs[0]))).read_bytes()
        assert re.search(r'^output-buffer-seed index=%d rva=%08x allocation=%08x callback=%08x$' %
                         (index, rva, original_alloc, module + 0x12365), raw, re.M)
        assert len(before['bss']['output_buffers']) == len(models)
        for other, native in zip(models, before['bss']['output_buffers']):
            j = other['index']
            expected_original = seeded(j, original_alloc, module + 0x12365) if j == index else INITIALIZED if j < index else ZERO
            expected_native = seeded(j, native_alloc, symbols['initialize_state_12365']) if j == index else INITIALIZED if j < index else ZERO
            assert read(bss, other['original_object_rva']) == expected_original
            assert native == expected_native
        assert struct.unpack_from('<I', bss, 0x7818)[0] == original_alloc
        assert struct.unpack_from('<I', bss, 0x781c)[0] == before['bss']['cleanup_count'] == index
        original_callbacks = [module + m['destructor_rva'] for m in models[:index]] + [0] * (32 - index)
        native_callbacks = [symbols[m['destructor_native_symbol']] for m in models[:index]] + [0] * (32 - index)
        assert list(struct.unpack_from('<32I', bss, 0x7820)) == original_callbacks
        assert before['bss']['cleanup_callbacks'] == native_callbacks
        raw_data = (original / ('compiler-data-%03d.bin' % int(seqs[0]))).read_bytes()
        raw_snapshot = (original / ('compiler-snapshot-%03d.bin' % int(seqs[0]))).read_bytes()
        raw_globals = (original / ('state-%03d.bin' % int(seqs[0]))).read_bytes()
        assert struct.unpack_from('<I', raw_data, 0x1530)[0] == before['initialized']['first_call'] == 0
        assert struct.unpack_from('<I', raw_snapshot, 0x1530)[0] == before['snapshot']['first_call'] == 0x13579bdf
        assert struct.unpack_from('<I', raw_globals, 0x64)[0] == before['reuse_state'] == 0
        assert struct.unpack_from('<I', raw_globals, 0x8c)[0] == before['pass_mode'] == 1
        assert before['bss']['saved_data'] == native_alloc
        assert before['bss']['context'] == before['bss']['current_context'] == dict(object=0, value=0, saved_frame=0)
        assert before['bss']['contexts'] == [dict(object=0, value=0, saved_frame=0) for i in range(6)]
        assert struct.unpack_from('<3I', bss, 0x18b8) == struct.unpack_from('<3I', bss, 0x18c8) == (0, 0, 0)
        assert struct.unpack_from('<18I', bss, 0x1858) == (0,) * 18
        address, size = re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] ' + row['constructor_native_symbol'] + '$', symbol_text, re.M).groups()
        assert int(address, 16) <= observation['pc'] < int(address, 16) + int(size, 16)
