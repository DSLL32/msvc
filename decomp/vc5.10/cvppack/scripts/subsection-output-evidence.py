#!/usr/bin/env python3
"""Finite copier/caller byte and consumer witness; common tool owns graph work."""
import importlib.util
import json
import hashlib
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM
from binary import PE
from state import BINARY,ROOT,sha

pair=BINARY/'cases/hello-debug-dll/evidence/0105-write-subsections';config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
path=ROOT/config['target']['path'];assert sha(path)==config['target']['sha256'];pe=PE(path);decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
intervals=[]
for name,start,end,allowed,calls in [
 ('subsection_copier',0x4068a0,0x4069da,{0x4153b8,0x412d34,0x412d38,0x412d40,0x415660,0x415654,0x415348},[(0x4068c9,'0x4054d0'),(0x40691b,'0x410140'),(0x406931,'0x4054d0'),(0x4069b8,'0x410140'),(0x4069ce,'0x4054d0')]),
 ('writer_loop_continuation',0x406c17,0x406c5a,{0x412124,0x412d40,0x415640,0x415660,0x415654,0x4153b8},[(0x406c20,'0x401a80'),(0x406c55,'0x40d600')])]:
 body=pe.read(start,end-start);instructions=list(decoder.disasm(body,start));assert sum(i.size for i in instructions)==len(body)
 assert not any(i.mnemonic.startswith(('f','v','xsave','xrstor')) for i in instructions),'new raw-FP consumer'
 assert [(i.address,i.op_str) for i in instructions if i.mnemonic=='call']==calls
 absolute=[]
 for instruction in instructions:
  for operand in instruction.operands:
   if operand.type==X86_OP_MEM and not operand.mem.base and not operand.mem.index:
    assert operand.mem.disp in allowed,'new consumer accesses an unreviewed root'
    absolute.append(dict(site=instruction.address,address=operand.mem.disp,width=operand.size))
 intervals.append(dict(name=name,extent=[start,end],sha256=hashlib.sha256(body).hexdigest(),absolute_operands=absolute,calls=[dict(site=a,target=b) for a,b in calls],integer_mnemonics=sorted({i.mnemonic for i in instructions})))
assert pe.read(0x4068fb,2)==bytes.fromhex('f3a5') and pe.read(0x406902,2)==bytes.fromhex('f3a4') and pe.read(0x406994,2)==bytes.fromhex('f3a5') and pe.read(0x40699b,2)==bytes.fromhex('f3a4'),'original integer copy loops differ'
assert pe.read(0x412580,4)==bytes(4),'encountered padding object differs'
def ref(p):return dict(path=str(p.relative_to(BINARY)),sha256=sha(p))
checks=json.loads((pair/'subsection-checks.json').read_text());positive=json.loads((pair/'final-pair-check.log').read_text())
result=dict(schema=1,status='candidate-complete-affected-checks-awaiting-independent-review',candidate='write_subsection',target_sha256=sha(path),intervals=intervals,actual_state=positive['typed_state']['subsection_output'],allocation_state=positive['typed_state']['output_buffer'],frontier_parameters=positive['typed_state']['parameters'],
 source_review=['Whole314B readable C routine uses actual12B directory rows and the existing in-body buffer macro; nonempty source copied, row rewritten to relative file offset, then contextual shared4B zero object supplies DWORD alignment. No generated assembly/new call vertices. Original6B interfunction NOP gap excluded.','Actual58 pre/post records bind eachindex, complete5002 source bytes to independently current live ModuleInfo-owned inputs, actual cdeclstack/returns and all4 ordinary nonvolatiles; integer REPcopy/padding paths stay within observed newoutput owner. All fallback/fatal branches remain unvisited static-only.','Sorted840B input snapshot is historical after processing.58current rows now hold numeric offsets; current696B plus unchanged initialized144B tail are compared independently against actual calls.72B spare remains unobserved. New output-prefix5056 is derived from header8+5002input+46padding, not an input snapshot relabeled as output.','Caller passes genuine spare entry58 within912B directory allocation and relativeoffset5056; original2argument stack and actual GDB4Bpointer/12Bpointee+4Bscalar schema checked. Callee is still namedtrap, so no output-entry body/read/initialization credit.','Existing malloc generation remains separate even when originalstaletype_records aliasesnewbuffer; only newowner5056 initialized bytes observed,60456spareunwritten. No stale-root dereference/compare/free/serialization/order/escape in finite newcopier/caller interval. Futureunknown write_publics body reopens consumer scope.','Original routine integer-only; actual each-own full rawFP/errors preserved to newfrontier under prior027f/zero/all-empty predicate. No general liveFP/CL rule. Ordinary native memcpy does not add an original graph vertex.'],
 source=ref(BINARY/'src/writer.c'),header=ref(BINARY/'src/writer.h'),new_controller=ref(BINARY/'scripts/subsection-output-state.py'),directory_controller=ref(BINARY/'scripts/output-directory-state.py'),allocation_controller=ref(BINARY/'scripts/output-buffer-state.py'),lifetime_controller=ref(BINARY/'scripts/main-free-state.py'),raw_decoder=ref(BINARY/'scripts/raw-state.py'),original_observer=ref(BINARY/'scripts/subsection-observer.inc'),native_observer=ref(BINARY/'scripts/gdb-state.py'),static_scope=ref(BINARY/'scripts/main-free-evidence.py'),checker=ref(Path(__file__)),policy=ref(BINARY/config['main_free_policy']['path']),original_receipt=ref(pair/'original/receipt.json'),native_receipt=ref(pair/'native/receipt.json'),audit=ref(pair/'final-audit/audit.json'),pair_check=ref(pair/'final-pair-check.log'),controls=ref(pair/'subsection-checks.json'),
 reused_lifecycle_review=ref(BINARY/'cases/hello-debug-dll/evidence/0104-output-buffer-initialization/buffer-lifetime-review.json'),seconds=dict(stages=json.loads((pair/'timings.json').read_text()),fresh_affected_controls=checks['seconds']),observer_correction='Initial unit attempted to clobber original ESI by setting0, but first callerESI already0; unchanged-state mutant correctlypassed. Replaced by actual XOR1 clobber, full24 checks nowPASS. No target/controller behavior change or acceptance from incomplete test campaign.',report_refinement='Fresh whole pair passed all new validator assertions; its first report omitted subsection_proof summary. Added reportfield only and derived finalstatic/check-only receipt on unchanged original/native raw, no recapture.')
(pair/'subsection-review.json').write_text(json.dumps(result,indent=2)+'\n');print(sha(pair/'subsection-review.json'))
