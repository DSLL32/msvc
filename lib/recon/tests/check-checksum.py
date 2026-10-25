"""Actual public oracle plus small independent DWORD-sum edge controls."""
import hashlib
import pathlib
import struct
import subprocess
import sys
import tempfile
import time

start = time.monotonic()
binary = str(pathlib.Path(sys.argv[1]).resolve())
actual = pathlib.Path(sys.argv[2]).read_bytes()[:18320]
master = pathlib.Path(sys.argv[3]).read_bytes()
assert hashlib.sha256(actual).hexdigest() == 'c30b56b624df1f89fdfbbbe33bba6263a0dfa5d4806d579bca8e54e58100572c'
assert hashlib.sha256(master).hexdigest() == '05eb75a92370b4c76cb08a4a37e8b8c6f655a58a6aad23860b60a0e73966a7de'
nt = struct.unpack_from('<I', actual, 60)[0]
field = nt+0x58

def checksum(data):
    cleared = bytearray(data)
    cleared[field:field+4] = bytes(4)
    padded = cleared+bytes((-len(cleared)) % 4)
    total = sum(struct.unpack('<%dI' % (len(padded)//4), padded))
    while total >> 16:
        total = (total & 0xffff)+(total >> 16)
    return (total+len(data)) & 0xffffffff

assert struct.unpack_from('<I', actual, field)[0] == 0x10c76
assert checksum(actual) == checksum(master) == struct.unpack_from('<I', master, field)[0] == 0x4b2a
changed = bytearray(actual)
struct.pack_into('<I', changed, field, 0xdeadbeef)
vectors = [actual, master, bytes(changed), actual+b'\xa7', actual[:1024]+b'\xff'*(65537-1024)]
count = 0
with tempfile.TemporaryDirectory() as directory:
    path = pathlib.Path(directory, 'mapped.bin')
    def invoke(data, mode, reject=False):
        global count
        backing = data+b'\xa5'*(2097152-len(data))
        path.write_bytes(backing)
        old = struct.unpack_from('<I', data, field)[0]
        new = 0 if reject else checksum(data)
        result = subprocess.run([binary, str(path), str(len(data)), str(old), str(new), str(nt), mode], capture_output=True)
        assert result.returncode == 0 and not result.stdout, (mode, result)
        if reject:
            assert b'unsupported native operation:' in result.stderr, result.stderr
        else:
            assert not result.stderr, result.stderr
        assert path.read_bytes() == backing
        count += 1
    for data in vectors:
        for mode in ('', 'live'):
            invoke(data, mode)
    invoke(actual, 'adjacent')
    for mode in ('null-base', 'interior', 'unmapped', 'short', 'long', 'null-output',
                 'image-output', 'tail-output', 'same-output', 'partial-output', 'wrapped-output'):
        invoke(actual, mode, True)
    for offset, value in ((0, b'NZ'), (60, struct.pack('<I', 0xfffffffc)),
                          (nt, b'XX'), (nt+20, struct.pack('<H', 96)),
                          (nt+24, struct.pack('<H', 0x20b))):
        bad = bytearray(actual)
        bad[offset:offset+len(value)] = value
        invoke(bytes(bad), 'bad-header', True)
print('actual18320B/0x4b2a oracle; old-field/odd/carries/NT alias/outputs/tails '
      'x3 errors xempty/live108FP;16 no-write denials PASS %.6fs (%d controls)' %
      (time.monotonic()-start, count))
