#!/usr/bin/env python3
"""Historical resource oracle: original execution is retired; see README.txt."""
import argparse
import hashlib
import struct


def resource_records(data):
    """Read Win32 .res headers without normalizing or changing any bytes."""
    result = []
    pos = 0
    while pos < len(data):
        size, header = struct.unpack_from('<II', data, pos)
        cursor = pos + 8

        def identifier():
            nonlocal cursor
            value, = struct.unpack_from('<H', data, cursor)
            cursor += 2
            if value == 0xffff:
                value, = struct.unpack_from('<H', data, cursor)
                cursor += 2
                return value
            chars = []
            while value:
                chars.append(value)
                value, = struct.unpack_from('<H', data, cursor)
                cursor += 2
            return ''.join(map(chr, chars))

        kind, name = identifier(), identifier()
        cursor = (cursor + 3) & ~3
        version, flags, language, resource_version, characteristics = struct.unpack_from('<IHHII', data, cursor)
        assert cursor + 16 <= pos + header and pos + header + size <= len(data)
        if size:
            payload = data[pos + header:pos + header + size]
            result.append(dict(type=kind, name=name, language=language, flags=flags,
                               bytes=size, sha256=hashlib.sha256(payload).hexdigest()))
        pos = (pos + header + size + 3) & ~3
    assert pos == len(data)
    return result



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', nargs='?', choices=['rc', 'rcpp', 'all'], default='all')
    parser.parse_args()
    parser.exit(2, 'Historical resources-rich capture is retired: its frozen preprocessor output embeds a former toolchain drive.\n'
        'Use decomp/vc1.0/rc/scripts/capture-native-boundaries.py for current RC-owned cases;\n'
        'RCPP-owned explicit capture is decomp/vc1.0/rcpp/scripts/capture-shared-prefix.py.\n'
        'Frozen historical inputs/oracles remain unchanged; no Wine process was started.\n')


if __name__ == '__main__':
    main()
