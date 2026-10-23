"""Test-only encoding of synthetic/mutated native observations."""
import struct
from dependency_wire import string


def payload(fields):
    word = lambda n: struct.pack('<I', n)
    words = lambda values: b''.join(word(value) for value in values)
    def strings(value):
        return word(value['address']) + word(len(value['strings'])) + b''.join(
            word(pointer) + string(text) for pointer, text in zip(value['pointers'], value['strings']))
    def handle(value):
        position = value['position'] & ((1 << 64) - 1)
        return words([value['handle'], value['type'], value['device'] & 0xffffffff, value['device'] >> 32,
                      value['inode'] & 0xffffffff, value['inode'] >> 32, position & 0xffffffff, position >> 32])
    value = words(fields[name] for name in ('errno', 'doserrno', 'last_error', 'controlfp', 'fmode',
        'commode', 'mb_cur_max', 'mb_codepage', 'lc_codepage'))
    value += words(fields['lc_handles']) + bytes.fromhex(fields['mbctype']) + bytes.fromhex(fields['ctype'])
    value += words(fields[name] for name in ('interrupted', 'pass_running', 'pass_flags', 'pass_is_current',
        'module_is_c1', 'invoke_is_c1', 'abort_is_c1', 'provider_state', 'environment_is_initial',
        'initial_environment_is_initenv'))
    value += string(fields['pass_name']) + string(fields['pass_environment'])
    value += word(fields['argc']) + word(fields['last']) + strings(fields['argv'])
    value += strings(fields['environment']) + strings(fields['initial_environment'])
    osenv = fields['os_environment']
    raw = bytes.fromhex(osenv['bytes'])
    value += words([osenv['address'], len(raw) // 2, osenv['updates']]) + raw
    value += words(fields['file_words'])
    for stream in fields['streams']:
        value += word(stream['osfile']) + handle(stream['crt']) + handle(stream['win32'])
    value += word(len(fields['open_descriptors'])) + words(fields['open_descriptors']) + string(fields['cwd'])
    return value

