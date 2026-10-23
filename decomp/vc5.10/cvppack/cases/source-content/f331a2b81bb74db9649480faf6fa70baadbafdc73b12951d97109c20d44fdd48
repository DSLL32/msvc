"""Prepare an encountered-only native dependency response; no Wine or PE runtime."""
import importlib.util
import json
from pathlib import Path
import struct
from state import BINARY,sha


def prepare(output):
    case=BINARY/'cases/hello-debug-dll'
    binding=json.loads((case/'providers/binding.json').read_text())
    receipt=BINARY/binding['source_receipt']
    assert sha(receipt)==binding['source_receipt_sha256']
    assert json.loads(receipt.read_text())['actual_loaded_mspdb']['path']==binding['observed_path']
    provider=BINARY/binding['owned_artifact']
    assert sha(provider)==binding['sha256'] and provider.stat().st_size==binding['bytes']
    spec=importlib.util.spec_from_file_location('widen_contract',BINARY/'scripts/widen-contract.py')
    decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
    contract=decoder.decode(receipt.parent,provider_artifact=provider)
    assert contract==json.loads((case/'widen-contract.json').read_text())
    source=case/'original-probe-0001/input.dll';assert sha(source)==contract['input_dll_sha256']
    raw=bytearray(b'RWP1'+struct.pack('<II',21,source.stat().st_size)+contract['provider_sha256'].encode('ascii')+source.read_bytes())
    for instance in contract['instances']:
        factory,query,emit=instance['factory'],instance['query'],instance['emit']
        assert instance['last_error_sequence']==[0]*7
        values=[factory['capacity'],factory['flags'],factory['result'],query['input_bytes'],query['output_capacity'],query['fixup_capacity'],emit['written_bytes'],emit['written_fixups']]
        raw+=struct.pack('<8I',*values)+bytes.fromhex(query['input'])+bytes.fromhex(emit['output'])+bytes.fromhex(emit['fixups'])
    output.write_bytes(raw)
    return dict(provider_sha256=sha(output),contract_sha256=sha(case/'widen-contract.json'),input_dll_sha256=contract['input_dll_sha256'],external_module_sha256=contract['provider_sha256'],instances=21)
