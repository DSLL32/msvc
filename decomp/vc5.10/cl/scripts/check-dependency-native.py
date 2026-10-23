#!/usr/bin/env python3
"""Offline native-provider parser/transport controls; never launch an original.

The positive request is synthetic native memory, populated from the reviewed
original analysis. This checks rejection rules, not function acceptance or a
successful native application execution.
"""
import argparse
import copy
import json
import os
from pathlib import Path
import socket
import struct
import tempfile
import time

from dependency_native import decode, expectation, validate
from dependency_packet import decode as original_packet, vector
from dependency_wire import DllBroker, configuration, string
from binary import PE


from check_dependency_payload import payload


def run(manifest):
    started = time.perf_counter()
    packet = original_packet(manifest)
    raw = Path(json.loads(manifest.read_text())['directory'])
    phase = packet['phases']['invoke-before']
    phase['pass_environment'] = PE(raw / 'loaded-target.bin').string(phase['pass_words'][2])
    read = lambda name: (raw / name).read_bytes()
    # These five words are only synthetic test inputs. The old analysis did not
    # capture them; production requires the new observer's actual raw words.
    execution = struct.pack('<5I', 0, phase['pass_address'], 1, 0x4095d8, phase['pass_address'])
    expected = expectation(phase, read('observer/invoke-before-crt-environment.bin'), read('environment.bin'),
        read('observer/invoke-before-os-environment.bin'), read('observer/invoke-before-mb-state.bin'),
        read('observer/invoke-before-ctype.bin'), execution, 2)
    def observed_vector(data):
        address, entries = vector(data)
        return {'address': address, 'pointers': [pointer for pointer, text in entries],
                'strings': [text.decode('latin1') for pointer, text in entries]}
    with tempfile.TemporaryDirectory(prefix='cl-native-provider-controls-', dir='/root/.ai-tmp/msvc2') as directory:
        directory = Path(directory)
        output = directory / 'stdout.bin'
        output.write_bytes(b'test.c\r\n')
        stat = output.stat()
        fields = {**expected['scalar'], 'mbctype': expected['mbctype'], 'ctype': expected['ctype'],
            'pass_name': expected['pass_name'], 'pass_environment': expected['pass_environment'],
            'argv': observed_vector(read('observer/invoke-before-caller-argv.bin')),
            'environment': observed_vector(read('observer/invoke-before-crt-environment.bin')),
            'initial_environment': observed_vector(read('environment.bin')),
            'os_environment': {'address': 0x300000, 'updates': 1, 'bytes': expected['os_environment']},
            'file_words': phase['file0_2_words'], 'open_descriptors': [0, 1, 2], 'cwd': str(directory),
            'streams': []}
        lowio = read('observer/invoke-before-lowio-0.bin')
        flags = [lowio[36 * index + 4] for index in range(3)]
        for index in range(3):
            handle = {'handle': index + 1, 'type': 2 if index == 0 else 1,
                      'device': stat.st_dev if index else 0, 'inode': stat.st_ino if index else 0,
                      'position': 8 if index else 0}
            fields['streams'].append({'osfile': flags[index], 'crt': handle, 'win32': dict(handle)})
        encoded = payload(fields)
        assert decode(encoded) == fields
        validate(decode(encoded), expected, directory, output, flags)
        passed = []
        def negative(name, mutate=None, encoded_mutate=None):
            changed = copy.deepcopy(fields)
            if mutate: mutate(changed)
            value = payload(changed)
            if encoded_mutate: value = encoded_mutate(value)
            try:
                validate(decode(value), expected, directory, output, flags)
            except (ValueError, UnicodeError, struct.error):
                passed.append(name)
            else:
                raise AssertionError('accepted native dependency mutation: ' + name)
        negative('truncated request', encoded_mutate=lambda value: value[:-1])
        negative('unparsed request suffix', encoded_mutate=lambda value: value + b'x')
        negative('wrong last-pass ABI', lambda value: value.update(last=0))
        negative('changed error state', lambda value: value.update(last_error=0))
        negative('wrong module lease state', lambda value: value.update(provider_state=5))
        negative('wrong cached Invoke', lambda value: value.update(invoke_is_c1=0))
        negative('untyped alias', lambda value: value.update(pass_is_current=2))
        negative('wrong argument bytes', lambda value: value['argv']['strings'].__setitem__(1, '-changed'))
        negative('wrong argument alias', lambda value: value['argv']['pointers'].__setitem__(1, 1))
        negative('argument table overlaps strings', lambda value: value['argv'].update(address=value['argv']['pointers'][0]))
        negative('missing original initial environment', lambda value: value.update(initial_environment_is_initenv=0))
        negative('missing environment deep copy', lambda value: value['environment']['pointers'].__setitem__(0, value['initial_environment']['pointers'][0]))
        negative('wrong environment order', lambda value: value['environment']['strings'].reverse())
        negative('wrong OS namespace', lambda value: value['os_environment'].update(bytes='00000000'))
        negative('wrong OS update count', lambda value: value['os_environment'].update(updates=0))
        negative('wrong CRT table', lambda value: value.update(ctype='ffff' + value['ctype'][4:]))
        negative('stdin buffer no longer aliases', lambda value: value['file_words'].__setitem__(0, 1))
        negative('Win32 versus CRT handle alias', lambda value: value['streams'][2]['win32'].update(handle=9))
        negative('output position differs', lambda value: (value['streams'][1]['crt'].update(position=9), value['streams'][1]['win32'].update(position=9)))
        negative('unexpected open descriptor', lambda value: value['open_descriptors'].append(3))
        negative('wrong working directory', lambda value: value.update(cwd=str(directory.parent)))
        changed = copy.deepcopy(fields)
        changed['environment']['address'] += 4
        try:
            validate(changed, expected, directory, output, flags, previous=fields)
        except ValueError:
            passed.append('caller namespace identity changed across lifecycle')
        else:
            raise AssertionError('accepted changed namespace identity')

        identity = 'a' * 64
        config = configuration(identity, fields['pass_name'], expected['initial_environment'],
            bytes.fromhex(expected['os_environment']), bytes.fromhex(expected['mbctype']), bytes.fromhex(expected['ctype']))
        observed = []
        def callback(kind, data):
            validate(decode(data), expected, directory, output, flags)
            observed.append(kind)
            return {'status': 0, 'effect': 1, 'errno': 0, 'doserrno': 2, 'last_error': 0}
        def exchange(client, kind, data, pid=None, case_hash=identity, magic=b'CLD1'):
            data = struct.pack('<I', os.getpid() if pid is None else pid) + case_hash.encode('ascii') + data
            client.sendall(struct.pack('<4sII', magic, kind, len(data)) + data)
            return DllBroker._read(client, 24)
        # LOAD is intentionally not faked by this positive parser-only control:
        # its distinct pass cache and running state must have its own contract.
        broker = DllBroker(config, identity, [0, 1, 2], callback)
        client = broker._target
        client.settimeout(2)
        try:
            assert exchange(client, 0, config) == struct.pack('<4s5I', b'CLR1', 0, 0, 0, 0, 0)
            try: exchange(client, 2, encoded)
            except (ValueError, ConnectionError): pass
            else: raise AssertionError('out-of-order Invoke accepted')
            assert broker.errors and not observed
            passed.append('out-of-order request before callback')
        finally: broker.close()
        for name, mutation in (
                ('wrong request identity', {'case_hash': 'b' * 64}),
                ('wrong request magic', {'magic': b'WRNG'}),
                ('zero process ID', {'pid': 0}),
                ('changed actual configuration', {'data': config + b'x'})):
            broker = DllBroker(config, identity, [0, 1, 2], callback)
            broker._target.settimeout(2)
            try:
                options = dict(kind=0, data=config)
                options.update(mutation)
                try: exchange(broker._target, **options)
                except (ValueError, ConnectionError): pass
                else: raise AssertionError('invalid broker request accepted: ' + name)
                assert broker.errors
                passed.append(name)
            finally: broker.close()
        print(json.dumps({'result': 'PASS', 'kind': 'offline synthetic native requests; no acceptance',
                          'negatives': len(passed), 'seconds': time.perf_counter() - started}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    run(parser.parse_args().manifest)
