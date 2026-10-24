#!/usr/bin/env python3
"""Concrete writer/padding whole-body and finite live-node proof packet."""
import hashlib
import json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM
from binary import PE
from state import BINARY,ROOT,sha

pair=BINARY/'cases/hello-debug-dll/evidence/0107-public-page-padding';config=json.loads((pair/'original/configuration.json').read_text());target=ROOT/config['target']['path'];assert sha(target)==config['target']['sha256'];pe=PE(target);decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
intervals=[]
for name,start,end,expected_calls,allowed in [
 ('padding_helper',0x40f960,0x40fad2,[(0x40f9c1,'0x40b1d0'),(0x40fa5b,'0x40b1d0')],set()),
 ('public_writer_continuation',0x40d62f,0x40d66a,[(0x40d665,'0x40e920')],{0x4132d4,0x412cf4,0x413274})]:
 body=pe.read(start,end-start);code=list(decoder.disasm(body,start));assert sum(i.size for i in code)==len(body)
 assert [(i.address,i.op_str) for i in code if i.mnemonic=='call']==expected_calls
 assert not any(i.mnemonic.startswith(('f','v','xsave','xrstor')) for i in code),'new rawFP consumer'
 absolute=[dict(site=i.address,address=o.mem.disp,width=o.size) for i in code for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index];assert all(row['address'] in allowed for row in absolute)
 intervals.append(dict(name=name,extent=[start,end],bytes=len(body),sha256=hashlib.sha256(body).hexdigest(),instructions=len(code),calls=expected_calls,absolute_operands=absolute,returns=[dict(site=i.address,bytes=i.bytes.hex(),cleanup=i.op_str) for i in code if i.mnemonic.startswith('ret')]))
assert pe.read(0x40fad2,14)==b'\x90'*14,'interfunction gap differs'
assert pe.read(0x40f991,13)==bytes.fromhex('8b4424288d500423d58954241c'),'actual reserve truncation differs'
assert pe.read(0x40fa29,14)==bytes.fromhex('668b43108bf38b1b6605020003c8'),'actual WORD record-length addition differs'
assert pe.read(0x40fa52,9)==bytes.fromhex('837c24280472596a18'),'actual final reserve comparison/allocation differs'
def ref(p):return dict(path=str(p.relative_to(BINARY)),sha256=sha(p))
positive=json.loads((pair/'pair_check.log').read_text());checks=json.loads((pair/'padding-checks.json').read_text());native=json.loads((pair/'native/receipt.json').read_text());assert (pair/'build-source-inputs.json').read_bytes()==(pair/'native/compiled-sources.json').read_bytes(),'actual compilation snapshot drift'
result=dict(schema=1,status='candidate-complete-affected-checks-awaiting-review',candidate='insert_symbol_page_padding',target_sha256=sha(target),intervals=intervals,excluded_gap_bytes=14,actual_state=positive['typed_state']['public_padding'],public_writer_state=positive['typed_state']['public_writer'],next_actual_parameters=positive['typed_state']['parameters'],
 source_review=[
 'Whole370B readable cdecl C routine, only two calls to existing allocate_public_block. Actual4096 buckets and linked nodes,16-bit reserve+4 and recordlen+2 arithmetic, lowWORD position, conditional4096-byte page boundary fill, typed S_ALIGN0402 node header, actual block linking and8B end marker. memset intrinsic preserves original in-function stores, no invented call vertices.14NOP gap bytes excluded.',
 'Current case has64 initialized old nodes/2108 record bytes, all fit below the first page with reserve8. Therefore actual internal padding path is unvisited/static-only; if a future input crosses a page boundary, controller explicitly holds pending fresh concrete contract, never manufactures zero behavior.',
 'Original independent before observer and native actual entry observer freeze entire16384B bucket table, all64 full owned nodes and actual arena cursor/remaining. Controller traverses each complete chain, binds input records/owner offsets/identities, then derives exact changed final bucket and fully initialized24B marker.4095 other bucket slots and all previous node contents/links remain checked. No arbitrary post-state copies are replayed as input.',
 'Actual bump cursor advances24 and remaining decreases24 within same observed8100B arena generation; no new allocator generation/free/CRT body. End marker hashffff/offset0/nameffff/segment0/record0600,0402,ffffffff is typed data at its natural allocation, never a bulk image. Generic node decoder admits nameless0402 only for this observed public-table role, with full header/payload and differential controller proof; unrelated tables retain their normal rules.',
 'Actual helper caller return40d62f/result8/entry-vs-return ESP and all4 nonvolatile registers match, then original59B writer continuation consumes return into public_symbol_bytes2116 and the header symbol count. Directory/output5056B/current positions remain unchanged. Old freed roots remain identity-only; no new read/compare/free/serialization/order/escape of retired payload in helper or continuation.',
 'Next genuine cdecl6 namehash boundary binds callback symbol_record_key, initialized zero WORD header.name_method, initialized zero DWORD header.name_bytes, actual own uninitialized names24 stream, count64 and same buckets. Actual GDB pointer/formal width/pointee inventory and named parent local/field identity are independent from metadata; stream contents never read/seeded. Whole local header20B initialized state now contains only actual2116 symbol count,48B unknown streams remain own.',
 'Entire helper/continuation integer-only and original ownFP/error baseline preserved through nextchild under prior narrow027f/0/ffff predicate. Actual full375callbacks/58copies repeat in fresh pair; no new FP/CRT/PDB/shared semantics.'
 ],source=ref(BINARY/'src/writer.c'),header=ref(BINARY/'src/writer.h'),stubs=ref(BINARY/'src/stubs.c'),controller=ref(BINARY/'scripts/public-padding-state.py'),parent_controller=ref(BINARY/'scripts/public-writer-state.py'),raw_decoder=ref(BINARY/'scripts/raw-state.py'),native_observer=ref(BINARY/'scripts/gdb-state.py'),original_observer=ref(BINARY/'scripts/public-padding-observer.inc'),original_host=ref(BINARY/'scripts/debug-run.c'),static_scope=ref(BINARY/'scripts/main-free-evidence.py'),checker=ref(Path(__file__)),policy=ref(BINARY/config['main_free_policy']['path']),original_receipt=ref(pair/'original/receipt.json'),native_receipt=ref(pair/'native/receipt.json'),audit=ref(pair/'original/audit.json'),pair_check=ref(pair/'pair_check.log'),controls=ref(pair/'padding-checks.json'),compiled_sources=ref(pair/'build-source-inputs.json'),
 seconds=dict(fresh_stages=json.loads((pair/'timings.json').read_text()),native=native['seconds'],fresh_controls=checks['seconds']),retained_diagnostics=[
 'Initial source omitted intrinsic memset, qualified-import graph rejected extra CRT call. Corrected pragma before reference execution; first static logs retained in preflight diagnostics.',
 'Initial informal extent byte count372 was wrong: actual[40f960,40fad2) is370. Extent itself was correct, but policy hash accidentally covered372 incl2NOPs. Static hash guard rejected before Wine child creation; failed preflight evidence retained as0107-public-page-padding-preflight-held. Corrected exact370B hash, no coverage/acceptance from failed attempt.',
 'First executed pair omitted pre-padding inputs due debugger full_graphs gate at accepted-entry events. Pair rejected, no before-state inferred from older cases. Custom/generic return labels also had a parser collision, corrected by exact generic-label exclusion.0107-public-page-padding-missing-before-held retains both failures and actual raw. Corrected entry-only before observer compiled; required fresh original22.790s/native4.936s pair now complete.',
 'Focused controls derive same allocation identities from streams already independently validated by full production positive; controls do not implement another CFG or target routine. Previous controls referenced only at their original scope, new33 address all newly exposed node/bump/field/callback/stream contracts.'
 ])
(pair/'padding-review.json').write_text(json.dumps(result,indent=2)+'\n');print(sha(pair/'padding-review.json'))
