#!/usr/bin/env python3
"""Pure stopped-broker controls; no process, Wine, or target execution."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'watcom'))
from inspect_stopped import Inspection, Memory, MAX_BYTES, MAX_HITS
from ipc import ProtocolError
from debugger import mask_image


class Broker:
    def __init__(self, image):
        self.image = bytearray(image)
        self.base = 0x10000
        self.traps = {}
        self.ctx = dict(eip=0, esp=self.base + 200, ebp=self.base + 220, eax=17)
        self.protect = 0x40
        self.reads = []

    def read(self, address, size):
        self.reads.append((address, size))
        return bytes(self.image[address-self.base:address-self.base+size])

    def region(self, address):
        return dict(base=self.base, allocation_base=self.base,
                    allocation_protect=0x40, size=len(self.image), state=0x1000,
                    protect=self.protect, type=0x20000)

    def trap(self, address, expected):
        at = address - self.base
        assert address not in (v[0] for v in self.traps.values())
        assert self.image[at] == expected
        self.image[at] = 0xcc
        index = max(self.traps, default=0) + 1
        self.traps[index] = (address, expected)
        return index

    def consume(self, index):
        address, original = self.traps.pop(index)
        assert self.ctx['eip'] == address + 1
        self.image[address-self.base] = original
        self.ctx['eip'] = address

    def context(self):
        return dict(self.ctx)

    def stop(self, rva):
        self.ctx['eip'] = self.base + rva + 1
        return dict(exception_address=self.base+rva, pid=7, tid=8), dict(self.ctx)


class Controls(unittest.TestCase):
    def setUp(self):
        scratch = Path.home() / '.ai-tmp/msvc2'
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix='cf-read-controls-', dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / 'inspect.py'
        self.path.write_text('''calls = 0
memory_seen = None
def select(stop, memory):
    global calls, memory_seen
    calls += 1
    memory_seen = memory
    assert memory.read(stop['image_base']+stop['stop_rva'],5)[0] == 0xe8
    assert stop['registers']['eip']+1 == stop['trap_registers']['eip']
    return calls == 2
def snapshot(stop, memory):
    assert memory is memory_seen
    return {'raw': memory.read(stop['image_base']+180,4).hex()}
''')
        self.config = dict(call_rva=32, script=str(self.path),
                           sha256=hashlib.sha256(self.path.read_bytes()).hexdigest())
        self.image = bytearray(b'\x90' * 256)
        self.image[32:37] = b'\xe8' + struct.pack('<i', 80 - 37)
        self.registry = [dict(name='caller', rva=16, definition=4, implementation=1,
                              code=[[16,64]], text_data=[]),
                         dict(name='callee', rva=80, definition=4, implementation=1,
                              code=[[80,100]], text_data=[])]
        self.layout = dict(code=[[0,128]], text_data=[])

    def inspector(self, **changes):
        args = dict(config=self.config, registry=self.registry, layout=self.layout,
                    snapshot=bytes(self.image), masked=bytes(self.image))
        args.update(changes)
        return Inspection(**args)

    def test_real_context_call_return_rearm_then_snapshot(self):
        inspection = self.inspector()
        broker = Broker(self.image)
        inspection.arm(broker, broker.base, (7,8))
        event, ctx = broker.stop(32)
        self.assertIs(inspection.handle(broker,event,ctx), True)
        self.assertEqual(broker.ctx['eip'], broker.base+32)
        self.assertNotEqual(broker.image[32],0xcc)
        self.assertEqual(broker.image[37],0xcc)
        event, ctx = broker.stop(37)
        self.assertIs(inspection.handle(broker,event,ctx), True)
        self.assertEqual(broker.image[32],0xcc)
        event, ctx = broker.stop(32)
        self.assertEqual(inspection.handle(broker,event,ctx),'selected')
        self.assertEqual(inspection.receipt['snapshot'],dict(raw='90909090'))
        self.assertEqual(inspection.budget['bytes'],sum(n for _, n in broker.reads) +
                         sum(row['read_bytes_reserved'] for row in inspection.budget['patch_checks']))
        self.assertNotIn('oracle_written',inspection.receipt)
        json.dumps(inspection.receipt,allow_nan=False)

    def test_source_hash_and_exact_call_ownership(self):
        for edit in ('hash','entry','pending','overlap','data','masked','opcode','target'):
            args = dict(config=copy.deepcopy(self.config),registry=copy.deepcopy(self.registry),
                        layout=copy.deepcopy(self.layout),snapshot=bytearray(self.image),
                        masked=bytearray(self.image))
            if edit=='hash': args['config']['sha256']='0'*64
            if edit=='entry': args['registry'].append(dict(self.registry[0],rva=37))
            if edit=='pending': args['registry'][0]['implementation']=0
            if edit=='overlap': args['registry'].append(dict(self.registry[0],rva=20))
            if edit=='data': args['layout']['text_data']=[[37,38]]
            if edit=='masked': args['masked'][37]=0xcc
            if edit=='opcode': args['snapshot'][32]=0x90
            if edit=='target': args['snapshot'][33:37]=b'\0'*4
            with self.subTest(edit=edit), self.assertRaises(ProtocolError):
                self.inspector(**args)

    def test_return_owner_sp_ebp_and_byte_failures(self):
        for edit in ('pid','sp','ebp','call','return'):
            inspection=self.inspector(); broker=Broker(self.image)
            inspection.arm(broker,broker.base,(7,8))
            inspection.handle(broker,*broker.stop(32))
            event,ctx=broker.stop(37)
            if edit=='pid': event['pid']=9
            if edit=='sp': ctx['esp']+=4
            if edit=='ebp': ctx['ebp']+=4
            if edit=='call': broker.image[33]^=1
            if edit=='return': broker.image[37]=0x90
            with self.subTest(edit=edit),self.assertRaises(ProtocolError):
                inspection.handle(broker,event,ctx)

    def test_every_registered_entry_and_ambiguous_identity_conflict(self):
        for definition in (0,1,2,3,4):
            for rva in (32,37):
                rows=copy.deepcopy(self.registry)
                rows.append(dict(name='conflict',rva=rva,definition=definition,
                                 implementation=0,code=[],text_data=[]))
                with self.subTest(definition=definition,rva=rva),self.assertRaises(ProtocolError):
                    self.inspector(registry=rows)
        for extra in (dict(self.registry[1]),dict(self.registry[1],definition=3),
                      dict(self.registry[0])):
            with self.assertRaises(ProtocolError):
                self.inspector(registry=self.registry+[extra])

    def test_memory_bounds_protection_and_aggregate(self):
        budget=dict(bytes=0,region_queries=0,reads=[]); broker=Broker(self.image)
        memory=Memory(broker,budget)
        self.assertEqual(memory.read(broker.base,4),b'\x90'*4)
        self.assertEqual(memory.bytes_used,4)
        for address,size in ((0,4),(broker.base,0),(broker.base,513),
                             (0xffffffff,4),(broker.base+255,2)):
            with self.assertRaises(ProtocolError): memory.read(address,size)
        budget['bytes']=MAX_BYTES-1
        with self.assertRaises(ProtocolError): memory.read(broker.base,2)
        for protect in (1,0x100,0x104,0x10):
            broker.protect=protect
            with self.assertRaises(ProtocolError): Memory(broker,dict(bytes=0,region_queries=0,reads=[])).read(broker.base,1)

    def test_hit_cap_no_match_and_unowned_return(self):
        inspection=self.inspector(); broker=Broker(self.image)
        inspection.arm(broker,broker.base,(7,8))
        with self.assertRaises(ProtocolError): inspection.handle(broker,*broker.stop(37))
        inspection.hits=MAX_HITS
        with self.assertRaises(ProtocolError): inspection.handle(broker,*broker.stop(32))

    def test_default_mask_and_unrelated_entry_accounting_are_unchanged(self):
        before=mask_image(bytes(self.image),self.registry,self.layout,0)
        inspection=self.inspector(); broker=Broker(self.image)
        inspection.arm(broker,broker.base,(7,8))
        self.assertIs(inspection.handle(broker,*broker.stop(80)),False)
        self.assertEqual(mask_image(bytes(self.image),self.registry,self.layout,0),before)


if __name__=='__main__':
    unittest.main()
