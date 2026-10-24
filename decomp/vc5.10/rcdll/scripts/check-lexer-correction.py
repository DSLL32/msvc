#!/usr/bin/env python3
"""Fresh zero-credit lexer correction checkpoint and affected alias controls."""
import argparse,json,time
from pathlib import Path
from protocol import sha
from rich_protocol import _include_search_checkpoint
p=argparse.ArgumentParser();p.add_argument('--original',required=True,type=Path);p.add_argument('--native',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();start=time.perf_counter()
def validate():
    return _include_search_checkpoint(a.original,a.native,lookup=True,evaluate=True,conditional=True,logical_or=True,logical_and=True,bitwise_or=True,bitwise_xor=True,bitwise_and=True,equality=True,relational=True,shift=True,additive=True,multiplicative=True,unary=True,lexer_correction=True)
proof=validate();path=a.native/'snapshot.json';receipt=a.native/'receipt.json';raw=path.read_bytes();receipt_raw=receipt.read_bytes();controls=[]
for name,mutate in [('same-value-duplicate-listing-storage',lambda s:s['arguments']['preprocessor']['globals']['rc_macro_listing_enabled'].update(address=s['arguments']['preprocessor']['globals']['rc_source_depth']['address'])),('wrong-listing-state',lambda s:s['arguments']['preprocessor']['globals']['rc_macro_listing_enabled'].update(bytes='01000000')),('expansion-depth-storage-alias',lambda s:s['arguments']['preprocessor']['globals']['rc_expanding_macro'].update(address=s['arguments']['preprocessor']['globals']['rc_source_depth']['address']))]:
    try:
        state=json.loads(raw);mutate(state);path.write_text(json.dumps(state));assert path.read_bytes()!=raw
        transport=json.loads(receipt_raw);transport['files']['snapshot.json']=dict(bytes=path.stat().st_size,sha256=sha(path));receipt.write_text(json.dumps(transport))
        try:validate()
        except AssertionError as error:
            trace=error.__traceback__
            while trace.tb_next:trace=trace.tb_next
            assert trace.tb_frame.f_code.co_filename.endswith('/rich_protocol.py'),name
            controls.append(dict(name=name,semantic_rejection=True,predicate_function=trace.tb_frame.f_code.co_name,predicate_line=trace.tb_lineno))
        else:raise AssertionError('affected alias mutation passed: '+name)
    finally:path.write_bytes(raw);receipt.write_bytes(receipt_raw)
proof.update(scope='Zero-credit source correction at accepted81 prefix. Current pair stops before matcher stub; production-object auxiliary branches and exact original/rebuilt operand identities separately prove corrected lexer dispatch. No marker/listing positive behavior or matcher body acceptance.',original_receipt_sha256=sha(a.original/'receipt.json'),native_receipt_sha256=sha(receipt),fresh_affected_controls=controls,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)))
a.output.write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(dict(status='pass',controls=len(controls),seconds=proof['seconds'])))
