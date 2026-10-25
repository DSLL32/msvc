"""Byte-bound Rust descriptor adapter for the existing runtime instruction gate.

No CFG discovery, implicit ownership, automatic acceptance, CRT exemption or
original capture launch. Caller supplies a reviewed source/build/report policy.
"""
from dataclasses import asdict
import hashlib
import json
from pathlib import Path
import struct

from gate import Function,Gate,Hold,Span


def require(condition,message):
    if not condition:raise Hold(message)

def sha(raw):return hashlib.sha256(raw).hexdigest()

def canonical(value):
    return json.dumps(value,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()


class Descriptors:
    def __init__(self,path,expected_report_sha256,producer,expected_producer_sha256,expected_inputs):
        raw=Path(path).read_bytes()
        require(sha(raw)==expected_report_sha256,'Rust instruction report hash changed')
        require(sha(Path(producer).read_bytes())==expected_producer_sha256,'Rust producer binary changed')
        self.report=json.loads(raw)
        export=self.report.get('decoded_instructions',{})
        require(export.get('schema')==1 and export.get('inputs')==expected_inputs,
                'Rust instruction report schema/input bindings differ')
        if 'image_inputs' in self.report:
            require(all(self.report['image_inputs'].get(side)==expected_inputs.get('images',{}).get(side)
                        for side in ('original','rebuilt')),'Rust instruction image bindings differ')
        self.images=export['inputs']['images']
        self.rows=export.get('functions',{})

    def selected(self,authorization):
        require(authorization and isinstance(authorization,dict),'empty explicit instruction authorization')
        result=[]
        for logical,policy in authorization.items():
            require(policy.get('role') in ('accepted-prefix','candidate','proved-support','authored-control'),
                    'instruction owner lacks explicit admission role')
            row=self.rows.get(logical,{}).get('original')
            require(isinstance(row,dict),'selected owner missing from Rust export: '+logical)
            record=row['record']
            require(not record.get('pending_extent',True),'pending owner cannot execute')
            require(row['record_sha256']==policy['record_sha256']==sha(canonical(record)),
                    'selected owner record binding changed')
            require(row['effective_roots']==[record['start']],
                    'multiple/alternate roots require a separate ABI policy')
            cleanup=policy['cleanup']
            require(isinstance(cleanup,int) and 0<=cleanup<=65535,'invalid typed RET cleanup')
            local=row['local_ret_cleanup_set'];checked=row['checked_ret_cleanup_set']
            runtime_tail=(policy.get('role')=='authored-control' and local==[] and checked is None and
                          policy.get('runtime_no_local_ret_cleanup')==cleanup)
            require((local in ([],[cleanup]) and checked in ([],[cleanup])) or runtime_tail,
                    'RET cleanup lacks checked agreement or explicit local-RET-free runtime contract')
            instructions=row['instructions'];require(instructions,'selected owner has no instructions')
            addresses=[item['address'] for item in instructions]
            require(addresses==sorted(set(addresses)),'instruction rows are not unique/sorted')
            result.append((logical,policy,row))
        return result

    def cf(self,image,base,snapshot,authorization):
        identity=image.compare(snapshot,base)
        meta=self.images['original']
        header={'allocation_size':image.allocation_size,'entry_offset':image.entry_offset,
                'header_file_offset':image.header_offset,'image_file_offset':image.load_offset,
                'initialized_size':image.loaded_size,'relocation_offset':image.relocation_offset}
        require(meta.get('format')=='watcom-cf' and meta.get('coordinates')=='image-offset' and
                meta.get('analysis_base')==0 and meta.get('header')==header and
                meta.get('original_file_sha256')==image.container_sha256 and
                meta.get('initialized_sha256')==sha(image.initialized) and
                meta.get('relocation_sha256')==sha(image.initialized[image.relocation_offset:]) and
                meta.get('relocation_count')==len(image.fixups),'Rust CF image identity differs')
        relocated=image.relocated(base);functions=[]
        for logical,policy,row in self.selected(authorization):
            code={}
            for item in row['instructions']:
                address=item['address'];raw=bytes.fromhex(item['bytes_hex'])
                require(0<=address and 1<=len(raw)<=15 and address+len(raw)<=image.loaded_size,
                        'CF instruction outside initialized byte identity')
                require(image.initialized[address:address+len(raw)]==raw,'Rust canonical instruction bytes differ')
                code[base+address]=relocated[address:address+len(raw)]
            functions.append(Function(logical,base+row['record']['start'],code,policy['cleanup']))
        return functions,identity

    def fixed_pe(self,file_path,actual_base,memory,authorization,installed_masks=None):
        """Authored setup support only; PE rebasing is deliberately unsupported."""
        meta=self.images['original']
        require(meta.get('format')=='pe32' and meta.get('coordinates')=='preferred-va' and
                meta.get('preferred_base')==actual_base and
                meta.get('original_file_sha256')==sha(Path(file_path).read_bytes()),'fixed authored PE identity differs')
        functions=[]
        for logical,policy,row in self.selected(authorization):
            code={}
            for item in row['instructions']:
                address=item['address'];raw=bytes.fromhex(item['bytes_hex'])
                require(1<=len(raw)<=15,'authored PE instruction width')
                observed=bytearray(memory(address,len(raw)))
                require(len(observed)==len(raw),'short authored PE instruction read')
                for masked,saved in (installed_masks or {}).items():
                    if address<=masked<address+len(raw):
                        offset=masked-address
                        require(observed[offset]==0xcc and raw[offset]==saved and saved!=0xcc,
                                'authored PE mask lacks exact saved-byte ownership')
                        observed[offset]=saved
                require(observed==raw,'authored PE actual instruction differs')
                code[address]=raw
            functions.append(Function(logical,row['record']['start'],code,policy['cleanup']))
        return functions


def observed_stack(broker,event,context):
    top,bottom=struct.unpack('<II',broker.read(event['teb']+4,8))
    region=broker.region(context['esp']-64)
    require(region['state']==0x1000 and region['protect'] in (4,8,0x40,0x80),
            'actual stack is not committed/writable')
    low=max(bottom,region['base']);high=min(top,region['base']+region['size'])
    require(low<=context['esp']-64<context['esp']<=high,'actual TEB stack intersection is too small')
    return Span(low,high-low,'actual committed writable TEB stack',write=True),{
        'teb_top':top,'teb_bottom':bottom,'region':region,'start':low,'end':high}


class Session:
    def __init__(self,broker,event,context,image,base,snapshot,functions,data_spans,
                 *,support_data_spans=(),installed_masks=None,max_steps=1000,seconds=20):
        # Re-read the complete CURRENT arena, never trust an old snapshot as
        # current process identity. Only explicit owned trap bytes may differ.
        image.compare(snapshot,base)
        actual=bytearray(b''.join(broker.read(base+offset,min(512,image.loaded_size-offset))
                         for offset in range(0,image.loaded_size,512)))
        for address,saved in (installed_masks or {}).items():
            offset=address-base
            require(0<=offset<image.loaded_size and actual[offset]==0xcc and
                    isinstance(saved,int) and saved==snapshot[offset] and saved!=0xcc,
                    'installed mask lacks exact observed saved-byte ownership')
            actual[offset]=saved
        self.identity=image.compare(bytes(actual),base)
        self.broker=broker;self.event=event;self.base=base;self.snapshot=snapshot
        self.stack,self.stack_proof=observed_stack(broker,event,context)
        flat,self.segment_proof=broker.flat_selectors(context)
        arena=Span(base,image.allocation_size,'observed byte-verified CF allocation')
        require(not (self.stack.start<base+image.allocation_size and base<self.stack.start+self.stack.size),
                'CF allocation overlaps observed stack')
        spans=[self.stack]
        for span in data_spans:
            require(arena.contains(span.start,span.size) and span.read,'data owner outside actual CF arena')
            spans.append(span)
        for span in support_data_spans:
            region=broker.region(span.start)
            require(span.read and not span.write and region['allocation_base']==event['base'] and
                    region['type']==0x1000000 and region['state']==0x1000 and
                    region['base']<=span.start and span.start+span.size<=region['base']+region['size'],
                    'support data lacks exact readonly owned PE-image span')
            spans.append(span)
        for function in functions:
            spans.extend(Span(address,len(raw),function.name+' instruction')
                         for address,raw in function.instructions.items())
        self.gate=Gate(functions,broker.read,spans,self.stack,event['tid'],arena,
                       flat_segments=flat,max_steps=max_steps,max_events=max_steps*3+20,seconds=seconds)
        self.gate.event('initial-stop',event['tid']);self.records=[];self.current=context
        # No arbitrary range is registered: the exact full initialized CF image
        # has just compared, with actual requested allocation and region checks.
        broker.arena(base,image.allocation_size)

    def consume_owned_entry(self,index,before):
        require(before['eip']-1 in self.gate.code,'entry is unknown; retain its installed INT3')
        expected=dict(before);expected['eip']-=1
        self.broker.consume(index);actual=self.broker.context()
        require(actual==expected,'owned entry consume changed unrelated full context')
        return actual

    def run(self,current):
        for _ in range(self.gate.max_steps):
            self.current=current
            decision=self.gate.plan(current,self.event['tid'])
            if decision.action=='frontier':
                require(decision.next_pc is not None,'unclassified current entry cannot execute')
                offset=decision.next_pc-self.base
                require(0<=offset<len(self.snapshot),'unclassified target lacks initialized-byte provenance')
                saved=self.snapshot[offset:offset+1];before=self.broker.read(decision.next_pc,1)
                require(saved==before and saved!=b'\xcc','unknown target pre-mask byte changed')
                index=self.broker.trap(decision.next_pc,saved[0])
                self.gate.arm_frontier(decision,current,self.event['tid'],saved,before,True,1,True)
                armed=self.broker.arm(current,decision);event,after=self.broker.step()
                require(event['pid']==self.event['pid'] and event['tid']==self.event['tid'],
                        'foreign frontier process/thread')
                frontier=self.gate.observe_frontier(after,self.event['tid'])
                trap_event,trap_context=self.broker.frontier_trap(index,after)
                require(trap_event['pid']==self.event['pid'] and trap_event['tid']==self.event['tid'],
                        'foreign frontier INT3 process/thread')
                self.records.append({'before':current,'decision':asdict(decision),'armed':armed,'after':after})
                return {'frontier':frontier,'actual_int3_event':trap_event,'actual_int3_context':trap_context,
                        'trap_index':index,'steps':self.records,'identity':self.identity,
                        'stack_proof':self.stack_proof,'segment_proof':self.segment_proof,
                        'application_acceptance':False}
            armed=self.broker.arm(current,decision);event,after=self.broker.step()
            require(event['pid']==self.event['pid'] and event['tid']==self.event['tid'],'foreign process/thread')
            self.gate.observe(after,self.event['tid'])
            self.records.append({'before':current,'decision':asdict(decision),'armed':armed,'after':after})
            current=after;self.current=current
        raise Hold('runtime session instruction budget exhausted')
