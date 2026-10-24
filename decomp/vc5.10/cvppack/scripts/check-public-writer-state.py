#!/usr/bin/env python3
"""Focused initialized-local/argument/schema controls before the padding child."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native',default='native',help='Retained native capture directory within the pair')
args=parser.parse_args()
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0106-write-publics-prefix';native_dir=pair/args.native

def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
raw=module('public_raw','raw-state.py');public=module('public_state','public-writer-state.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();original=raw.decode(events);native=json.loads((native_dir/'state.json').read_text())
def compare(a,b):return public.validate(config,data,a,b)
baseline=compare(original,native);rejected=[]
def reject(name,mutate,reason):
 a=copy.deepcopy(original);a['stack']=list(a['stack']);b=copy.deepcopy(native);mutate(a,b)
 try:compare(a,b)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('wrong-original-child',lambda a,b:a.update(address=0),'first real child')
reject('wrong-original-child-return',lambda a,b:a['stack'].__setitem__(0,0),'first real child')
reject('wrong-native-parent',lambda a,b:b.update(frontier_caller='flush_packed_file'),'first real child')
reject('wrong-child-reserve',lambda a,b:b['parameters'].update(reserve=0),'child argument schema')
reject('wrong-child-global-role',lambda a,b:b['parameters']['buckets'].update(global_name='directory_entries'),'child argument schema')
reject('wrong-original-buckets',lambda a,b:a['stack'].__setitem__(1,a['stack'][1]+4),'bucket input/reserve')
reject('wrong-original-reserve',lambda a,b:a['stack'].__setitem__(2,0),'bucket input/reserve')
reject('wrong-parent-owner',lambda a,b:b['public_writer_frame'].update(owner='main'),'parent identities')
reject('wrong-parent-caller',lambda a,b:b['public_writer_frame'].update(caller='main'),'parent identities')
reject('wrong-parent-original-return',lambda a,b:a['stack'].__setitem__(24,0),'original parent arguments')
reject('wrong-parent-original-entry',lambda a,b:a['stack'].__setitem__(25,a['stack'][25]-12),'original parent arguments')
reject('wrong-parent-original-offset',lambda a,b:a['stack'].__setitem__(26,0),'original parent arguments')
reject('wrong-parent-native-entry',lambda a,b:b['public_writer_frame'].update(entry=b['public_writer_frame']['entry']-12),'native parent arguments')
reject('wrong-parent-native-offset',lambda a,b:b['public_writer_frame'].update(offset=0),'native parent arguments')
reject('wrong-header-field-width',lambda a,b:b['public_writer_frame']['header_layout']['name_method'].update(width=4),'header fields')
reject('wrong-header-field-offset',lambda a,b:b['public_writer_frame']['header_layout']['symbol_bytes'].update(offset=8),'header fields')
reject('zero-header-identity',lambda a,b:b['public_writer_frame'].update(header_address=0),'initialized objects overlap')
reject('overlapping-padding-identity',lambda a,b:b['public_writer_frame'].update(padding_address=b['public_writer_frame']['header_address']+15),'initialized objects overlap')
reject('changed-native-header',lambda a,b:b['public_writer_frame'].update(header='01'+'00'*15),'initialized header/padding')
reject('truncated-native-header',lambda a,b:b['public_writer_frame'].update(header='00'*15),'initialized header/padding')
reject('changed-native-padding',lambda a,b:b['public_writer_frame'].update(padding=1),'initialized header/padding')
def raw_local(a,b,offset):
 value=bytearray(a['memory'][a['esp']]);value[offset]^=1;a['memory'][a['esp']]=bytes(value)
reject('changed-original-padding-byte',lambda a,b:raw_local(a,b,28),'initialized header/padding')
reject('changed-original-header-byte',lambda a,b:raw_local(a,b,32),'initialized header/padding')
reject('wrong-stream-width',lambda a,b:b['public_writer_frame']['uninitialized_streams'][0].update(width=20),'stream types')
reject('wrong-stream-first-offset',lambda a,b:b['public_writer_frame']['uninitialized_streams'][0].update(first_offset=16),'stream types')
reject('wrong-stream-local-name',lambda a,b:b['public_writer_frame']['uninitialized_streams'][0].update(local='unknown'),'stream types')
reject('missing-stream',lambda a,b:b['public_writer_frame']['uninitialized_streams'].pop(),'stream types')
reject('claimed-uninitialized-stream-contents',lambda a,b:b['public_writer_frame']['uninitialized_streams'][0].update(contents='00'*24),'unknown stream contents')
reject('overlapping-stream',lambda a,b:b['public_writer_frame']['uninitialized_streams'][0].update(address=b['public_writer_frame']['header_address']),'automatic object identities')
reject('changed-uncompared-stream-extent',lambda a,b:b['public_writer_frame'].update(uninitialized_stream_bytes_uncompared=0),'unknown stream state')
reject('changed-output-count',lambda a,b:b['globals'].update(output_used='00000000'),'modified output')
reject('changed-directory-count',lambda a,b:b['globals'].update(directory_count='00000000'),'modified output')
# Incidental raw transport bits in both uninitialized stream objects are not a
# typed application input here. Prove changing them alone does not constrain C.
a=copy.deepcopy(original);value=bytearray(a['memory'][a['esp']]);value[48:96]=bytes(range(48));a['memory'][a['esp']]=bytes(value);assert compare(a,native)==baseline
result=dict(schema=1,native_only=True,scope='Production initialized public-writer local/argument validator over raw-decoded actual pair; actual DWARF schema asserted by frozen GDB observer, full production acceptance separately checks prefix/375sort/58copy/lifetimes. Two24B stream contents remain uninitialized/uncompared, including incidental original fixed-stack transport bits.',fresh_negative_count=len(rejected),rejected=rejected,positive_uninitialized_stream_own_bits=True,baseline=baseline,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/public-writer-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(native_dir/'receipt.json'),reused=dict(path='cases/hello-debug-dll/evidence/0105-write-subsections/subsection-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0105-write-subsections/subsection-checks.json'),scope='Previous24 copier controls,24 allocation controls and32 sort/FP controls keep their own historical scope. This fresh whole-prefix pair repeats actual allocation/375callbacks/58copies; focused controls here cover only new prologue/parent/child state.'))
(pair/'public-writer-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
