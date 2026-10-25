"""Offline ownership controls for ordinary flat-image masking."""
from pathlib import Path
import sys
import struct
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'watcom'))
from debugger import layout_evidence, mask_image, sha
from broker import Broker
from ipc import ProtocolError


def row(name, rva, definition, implementation, code, data=()):
    return dict(name=name, rva=rva, definition=definition,
                implementation=implementation, code=code, text_data=list(data))


class MaskTests(unittest.TestCase):
    def setUp(self):
        self.original = bytes(range(128))
        self.layout = dict(code=[[0, 96]], text_data=[[4, 8]])
        self.registry = [row('crt', 16, 3, 0, [[16, 24]], [[8, 10]]),
                         row('accepted', 32, 4, 1, [[32, 40]]),
                         row('pending', 64, 4, 0, [])]

    def test_preserves_globals_text_data_and_real_restored_code(self):
        masked, apps, restored = mask_image(self.original, self.registry, self.layout, 16)
        expected = bytearray(b'\xcc' * 96 + self.original[96:])
        for start, end in [[4, 10], [16, 24], [32, 40]]:
            expected[start:end] = self.original[start:end]
        self.assertEqual(masked, expected)
        self.assertEqual(set(apps), {32, 64})
        self.assertEqual(masked[64], 0xcc)
        self.assertEqual([r['name'] for r in restored], ['crt', 'accepted'])

    def test_unknown_or_stub_extent_does_not_authorize_original_code(self):
        self.registry.append(row('unknown', 48, 0, 0, [[48, 56]]))
        self.registry[-2]['code'] = [[64, 72]]
        masked, _, _ = mask_image(self.original, self.registry, self.layout, 16)
        self.assertEqual(masked[48:56], b'\xcc' * 8)
        self.assertEqual(masked[64:72], b'\xcc' * 8)

    def test_pending_entry_cannot_be_restored_as_runtime_or_text_data(self):
        self.registry[0]['code'] = [[16, 68]]
        with self.assertRaises(ProtocolError):
            mask_image(self.original, self.registry, self.layout, 16)
        self.registry[0]['code'] = [[16, 24]]
        self.layout['text_data'] = [[64, 68]]
        with self.assertRaises(ProtocolError):
            mask_image(self.original, self.registry, self.layout, 16)

    def test_exit_wire_keeps_full_dword_including_259(self):
        class Client:
            def __init__(self, status):
                self.status = status
            def request(self, operation, payload):
                self.operation = operation
                return struct.pack('<25I', 5, 123, 456, self.status, *([0] * 21))
        for status in (259, 0xdeadbeef):
            event, context = Broker(Client(status)).run()
            self.assertEqual(event['exit_code'], status)
            self.assertIsNone(context)

    def test_layout_bounds_and_real_app_identity_are_required(self):
        for code in ([[0, 129]], [[0, 70], [64, 90]], [[0, 32]]):
            self.layout['code'] = code
            with self.assertRaises(ProtocolError):
                mask_image(self.original, self.registry, self.layout, 16)
        self.assertNotIn('capstone', sys.modules)
        self.assertNotIn('gate', sys.modules)
        self.assertNotIn('arena', sys.modules)

    def test_optional_documentary_layout_evidence(self):
        scratch = Path.home() / '.ai-tmp'
        scratch.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as directory:
            repo = Path(directory)
            self.assertIsNone(layout_evidence(repo, self.layout))
            # There is no witness file, yet all ordinary mask checks still run.
            masked, _, _ = mask_image(self.original, self.registry, self.layout, 16)
            self.assertEqual(masked[64], 0xcc)
            witness = repo / 'layout.json'
            witness.write_bytes(b'documentary layout rationale')
            binding = {'path': witness.name, 'sha256': sha(witness.read_bytes())}
            self.layout['evidence'] = binding
            self.assertEqual(layout_evidence(repo, self.layout), binding)
            for bad in (None, {}, {'path': '../layout.json', 'sha256': binding['sha256']},
                        {'path': witness.name, 'sha256': '0' * 64},
                        {'path': witness.name, 'sha256': 'invalid'}):
                self.layout['evidence'] = bad
                with self.assertRaises(ProtocolError):
                    layout_evidence(repo, self.layout)
            self.layout['evidence'] = binding
            witness.unlink()
            with self.assertRaises(OSError):
                layout_evidence(repo, self.layout)


unittest.main()
