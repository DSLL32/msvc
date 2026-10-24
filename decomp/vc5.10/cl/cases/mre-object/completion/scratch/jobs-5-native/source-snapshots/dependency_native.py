"""Decode CL's native DLL requests; data only, with no executable fallback.

All addresses are observations of this process. They are compared by ownership
and alias relationships, never mistaken for the original process's addresses.
The original-response controller supplies the corresponding typed expectations.
"""
import struct
import hashlib
from pathlib import Path

from dependency_wire import MAX_MESSAGE


class Reader:
    def __init__(self, data):
        if not isinstance(data, bytes) or len(data) > MAX_MESSAGE:
            raise ValueError('invalid native dependency payload')
        self.data, self.offset = data, 0

    def take(self, length):
        if length < 0 or length > len(self.data) - self.offset:
            raise ValueError('truncated native dependency payload')
        value = self.data[self.offset:self.offset + length]
        self.offset += length
        return value

    def word(self):
        return struct.unpack('<I', self.take(4))[0]

    def words(self, count):
        return list(struct.unpack('<%dI' % count, self.take(4 * count)))

    def text(self):
        length = self.word()
        if length > 32767:
            raise ValueError('oversized native dependency string')
        value = self.take(length)
        if b'\0' in value:
            raise ValueError('embedded NUL in native dependency string')
        return value.decode('latin1')

    def vector(self, required=True):
        address, count = self.words(2)
        if count > 4096 or (not address and (count or required)):
            raise ValueError('invalid native dependency vector')
        pointers, strings = [], []
        for unused in range(count):
            pointer = self.word()
            if not pointer:
                raise ValueError('NULL native dependency string pointer')
            pointers.append(pointer)
            strings.append(self.text())
        return {'address': address, 'pointers': pointers, 'strings': strings}

    def handle(self):
        handle, kind, device_lo, device_hi, inode_lo, inode_hi, pos_lo, pos_hi = self.words(8)
        if not handle or kind not in (1, 2):
            raise ValueError('unobserved native stream type')
        position = pos_lo | pos_hi << 32
        if position & (1 << 63):
            position -= 1 << 64
        if position < 0 and not (kind == 2 and position == -1):
            raise ValueError('unobserved native stream position')
        return {'handle': handle, 'type': kind,
                'device': device_lo | device_hi << 32,
                'inode': inode_lo | inode_hi << 32, 'position': position}

    def finish(self):
        if self.offset != len(self.data):
            raise ValueError('unused native dependency payload bytes')


def decode(data):
    reader = Reader(data)
    fields = dict(zip(('errno', 'doserrno', 'last_error', 'controlfp', 'fmode',
                       'commode', 'mb_cur_max', 'mb_codepage', 'lc_codepage'), reader.words(9)))
    fields['lc_handles'] = reader.words(6)
    fields['mbctype'] = reader.take(257).hex()
    fields['ctype'] = reader.take(514).hex()
    fields.update(zip(('interrupted', 'pass_running', 'pass_flags', 'pass_is_current',
                       'module_is_c1', 'invoke_is_c1', 'abort_is_c1', 'provider_state',
                       'environment_is_initial', 'initial_environment_is_initenv'), reader.words(10)))
    for name in ('pass_is_current', 'module_is_c1', 'invoke_is_c1', 'abort_is_c1',
                 'environment_is_initial', 'initial_environment_is_initenv'):
        if fields[name] not in (0, 1):
            raise ValueError('untyped native dependency alias observation')
    fields['pass_name'], fields['pass_environment'] = reader.text(), reader.text()
    fields['argc'], fields['last'] = reader.words(2)
    fields['argv'] = reader.vector(required=False)
    fields['environment'] = reader.vector()
    fields['initial_environment'] = reader.vector()
    os_address, os_units, updates = reader.words(3)
    if not os_address or not 2 <= os_units <= 65536:
        raise ValueError('invalid native OS environment block')
    os_bytes = reader.take(os_units * 2)
    if not os_bytes.endswith(b'\0\0\0\0'):
        raise ValueError('unterminated native OS environment block')
    # Validate UTF-16 rather than accepting malformed surrogate sequences.
    os_text = os_bytes.decode('utf-16le')
    if '\0\0' in os_text[:-2] or not os_text.endswith('\0\0'):
        raise ValueError('embedded terminator in native OS environment block')
    fields['os_environment'] = {'address': os_address, 'updates': updates,
                                'bytes': os_bytes.hex()}
    fields['file_words'] = reader.words(24)
    fields['streams'] = []
    for unused in range(3):
        fields['streams'].append({'osfile': reader.word(), 'crt': reader.handle(),
                                  'win32': reader.handle()})
    fields['open_descriptors'] = reader.words(reader.word())
    if len(fields['open_descriptors']) > 256 or fields['open_descriptors'] != sorted(set(fields['open_descriptors'])):
        raise ValueError('invalid native open-descriptor inventory')
    fields['cwd'] = reader.text()
    reader.finish()
    return fields


def argument_storage(vector):
    """Prove the reached caller's contiguous live strings and disjoint argv."""
    address, pointers, strings = (vector[name] for name in ('address', 'pointers', 'strings'))
    if not address or not pointers or len(pointers) != len(strings):
        raise ValueError('missing actual argument storage')
    offsets, length = [], 0
    for pointer, text in zip(pointers, strings):
        if pointer != pointers[0] + length:
            raise ValueError('argument strings are not contiguous')
        offsets.append(length)
        length += len(text.encode('latin1')) + 1
    if not (address + 4 * (len(pointers) + 1) <= pointers[0] or pointers[0] + length <= address):
        raise ValueError('argument vector overlaps live strings')
    return offsets


def stream_roles(fields):
    """Resolve CRT and Win32 aliases separately, retaining their observed roles."""
    streams = fields['streams']
    if fields['open_descriptors'] != [0, 1, 2]:
        raise ValueError('unexpected open target descriptor')
    roles = []
    for index, stream in enumerate(streams):
        words = fields['file_words'][index * 8:index * 8 + 8]
        pointer, count, base, flags, descriptor, character, capacity, temporary = words
        if pointer != base or count or temporary or descriptor != index or character:
            raise ValueError('unobserved FILE fields/live buffer')
        if index == 0:
            if not pointer or flags != 0x101 or capacity != 4096:
                raise ValueError('stdin provider storage differs')
        elif pointer or flags != 2 or capacity:
            raise ValueError('output FILE state differs')
        crt, win32 = stream['crt'], stream['win32']
        if crt != win32:
            raise ValueError('CRT/Win32 standard handle roles differ')
        if crt['type'] != (2 if index == 0 else 1):
            raise ValueError('standard stream kind differs')
        roles.append({'osfile': stream['osfile'], 'position': crt['position']})
    left, right = streams[1]['crt'], streams[2]['crt']
    if (left['device'], left['inode']) != (right['device'], right['inode']):
        raise ValueError('stdout/stderr do not alias the same file')
    return roles


def expectation(snapshot, crt_vector, initial_vector, os_bytes, mb_bytes, ctype_bytes, execution_words, kind):
    """Project already verified original bytes into a native request contract.

    This is deliberately independent of case/path normalization. A controller
    must verify the original raw manifest, masked trial and lifecycle before
    calling it; analysis packets alone never authorize native success.
    """
    from dependency_packet import vector
    digest = lambda value: hashlib.sha256(value).hexdigest()
    if digest(crt_vector) != snapshot['crt_environment_sha256'] or \
       digest(os_bytes) != snapshot['os_environment_sha256'] or \
       digest(mb_bytes) != snapshot['mb_state_sha256'] or \
       digest(ctype_bytes) != snapshot['ctype_sha256']:
        raise ValueError('original native-request expectation is not raw-bound')
    unused, entries = vector(crt_vector)
    unused, initial = vector(initial_vector)
    if len(entries) != snapshot['crt_environment_count'] or len(mb_bytes) != 261 or len(ctype_bytes) != 514:
        raise ValueError('incomplete original native-request expectation')
    if len(execution_words) != 20:
        raise ValueError('missing actual execution globals observation')
    interrupted, active_pass, pass_running, driver, current_pass = struct.unpack('<5I', execution_words)
    if interrupted or active_pass != snapshot['pass_address'] or not driver or current_pass != active_pass or \
       pass_running != int(kind == 2):
        raise ValueError('original execution globals/aliases differ')
    names = ('fmode', 'commode', 'mb_cur_max', 'mb_codepage', 'lc_codepage', 'lc_handles')
    scalar = {**snapshot['errors'], **{name: snapshot[name] for name in names}}
    # This observed case uses x87CW027f: all six exceptions masked, precision53,
    # rounding-nearest, affine infinity. _controlfp represents those as9001f.
    if snapshot['x87_control_word'] != 0x27f or kind not in (1, 2, 3):
        raise ValueError('unreviewed native request FP state or lifecycle')
    scalar.update(controlfp=0x9001f, interrupted=interrupted, pass_running=pass_running,
                  pass_flags=snapshot['pass_words'][4], pass_is_current=1,
                  module_is_c1=int(kind != 1), invoke_is_c1=int(kind != 1),
                  abort_is_c1=int(kind != 1), provider_state={1: 2, 2: 3, 3: 4}[kind],
                  environment_is_initial=0, initial_environment_is_initenv=1,
                  argc=26 if kind == 2 else 0, last=int(kind == 2))
    pointers = snapshot['argument_pointers']
    offsets = [pointer - pointers[0] for pointer in pointers]
    expected = {'scalar': scalar, 'mbctype': mb_bytes[4:].hex(), 'ctype': ctype_bytes.hex(),
                'pass_name': snapshot['argv'][0], 'pass_environment': snapshot['pass_environment'],
                'argv': snapshot['argv'] if kind == 2 else [], 'argv_offsets': offsets if kind == 2 else [],
                'environment': [text.decode('latin1') for pointer, text in entries],
                'initial_environment': [text.decode('latin1') for pointer, text in initial],
                'os_environment': os_bytes.hex(), 'os_updates': 1,
                'streams': []}
    if snapshot['pass_words'][4] != 15 or expected['environment'] != expected['initial_environment']:
        raise ValueError('unreviewed pass/environment contract')
    for index in range(3):
        observed = snapshot['streams']['crt'][index]
        flags = snapshot['open_descriptors'][index]
        if flags['fd'] != index or observed != snapshot['streams']['win32'][index]:
            raise ValueError('original standard stream roles differ')
        # Raw original lowio flags are separately captured by the controller;
        # their exact values must be supplied, not inferred from FILE flags.
        expected['streams'].append({'type': observed['type'],
                                    'position': observed['position_low'] | observed['position_high'] << 32})
    return expected


def validate(fields, expected, cwd, output_file, lowio_flags, previous=None):
    """Reject unknown ABI, state, stream or pointer-ownership inputs before effects."""
    for name, value in expected['scalar'].items():
        if fields[name] != value:
            raise ValueError('native dependency input differs: ' + name)
    for name in ('mbctype', 'ctype', 'pass_name', 'pass_environment'):
        if fields[name] != expected[name]:
            raise ValueError('native dependency input differs: ' + name)
    if fields['argv']['strings'] != expected['argv']:
        raise ValueError('native dependency argv differs')
    if expected['argv']:
        if argument_storage(fields['argv']) != expected['argv_offsets']:
            raise ValueError('native dependency argv storage offsets differ')
    elif fields['argv']['address'] or fields['argv']['pointers']:
        raise ValueError('unexpected lifecycle argument storage')
    for name in ('environment', 'initial_environment'):
        value = fields[name]
        if value['strings'] != expected[name] or len(set(value['pointers'])) != len(value['pointers']):
            raise ValueError('native dependency namespace/order/ownership differs: ' + name)
    current, initial = fields['environment'], fields['initial_environment']
    if current['address'] == initial['address'] or set(current['pointers']) & set(initial['pointers']):
        raise ValueError('initial CRT environment was not deep-copied')
    os_environment = fields['os_environment']
    if os_environment['bytes'] != expected['os_environment'] or os_environment['updates'] != expected['os_updates']:
        raise ValueError('native dependency OS namespace/order differs')
    if fields['cwd'] != str(Path(cwd).resolve()):
        raise ValueError('native dependency working directory differs')
    roles = stream_roles(fields)
    reference = Path(output_file).stat()
    for index, (observed, wanted) in enumerate(zip(fields['streams'], expected['streams'])):
        if observed['osfile'] != lowio_flags[index] or observed['crt']['type'] != wanted['type']:
            raise ValueError('native dependency lowio flags/handle kind differs')
        if index:
            handle = observed['crt']
            if handle['position'] != wanted['position'] or \
               (handle['device'], handle['inode']) != (reference.st_dev, reference.st_ino):
                raise ValueError('native dependency output role/position differs')
    if previous is not None:
        for name in ('environment', 'initial_environment', 'os_environment', 'file_words', 'streams', 'open_descriptors'):
            if fields[name] != previous[name]:
                raise ValueError('native DLL lifecycle changed caller-owned state: ' + name)
    return {'argument_offsets': expected['argv_offsets'], 'streams': roles,
            'environment_strings': len(current['strings']), 'initial_strings_deep_copied': True}
