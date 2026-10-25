#!/usr/bin/env python3
"""Pure byte-verifier controls, not evidence from an executing CF payload."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "watcom"))
from cf_identity import IdentityError, Image, sha


def denied(action,message):
    try:action()
    except IdentityError as error:
        assert message in str(error),(message,str(error))
        return str(error)
    raise AssertionError("identity failure accepted: "+message)


def fixture():
    raw=bytearray(256+16);raw[:2]=b"MZ";struct.pack_into("<H",raw,8,5)
    header=(0x4643,256,16,8,32,4);struct.pack_into("<6I",raw,80,*header)
    struct.pack_into("<I",raw,256,0xfffffff0)
    struct.pack_into("<4H",raw,264,1,0,0,0)
    return bytes(raw),header


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True);args=parser.parse_args()
    root=Path(__file__).resolve().parents[3]
    started=time.monotonic();controls=[];images=[]
    hashes={"WCC386":"c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b",
            "WPP386":"647f4e754cd27fc76b3c44a47227ae3baa4ff2e2f253a8ec27216414653b1e6f",
            "WASM":"a79e0dc11a26b098728aa988cfc974113e7ac5d28c81baded1f077ed27be0513"}
    headers={"WCC386":(0x4643,0x2200,0x8209c,0x7c448,0x91000,0x2fe00),
             "WPP386":(0x4643,0x2200,0xc3a44,0xbc438,0xe0000,0x664f4),
             "WASM":(0x4643,0x2200,0x28282,0x26bae,0x2f000,0x1da16)}
    for name,expected in hashes.items():
        raw=(root/'toolchains/watcom10.0a/binb'/(name+'.EXE')).read_bytes()
        image=Image.parse(raw,expected,headers[name])
        relocated=image.relocated(0x12000000)
        images.append({"name":name,**image.compare(relocated,0x12000000)})
        occupied={offset+delta for offset in image.fixups for delta in range(4)}
        plain=next(index for index in range(image.loaded_size) if index not in occupied)
        for label,position in [('relocation',image.fixups[0]),('nonfixup',plain)]:
            corrupted=bytearray(relocated);corrupted[position]^=0x80
            controls.append({"name":name+'_'+label+'_corruption',
                             "error":denied(lambda:image.compare(bytes(corrupted),0x12000000),'byte mismatch')})
        controls.append({"name":name+'_truncated_snapshot',
                         "error":denied(lambda:image.compare(relocated[:-1],0x12000000),'snapshot size')})
    raw,header=fixture();image=Image.parse(raw,sha(raw),header)
    actual=bytearray(raw[256:]);struct.pack_into('<I',actual,0,0x0ffffff0)
    image.compare(bytes(actual),0x10000000)
    controls.append({"name":"out_of_allocation_content_wraps_modulo32","passed":True})
    controls.append({"name":"immutable_container_identity",
                     "error":denied(lambda:Image.parse(raw,'0'*64,header),'hash mismatch')})
    controls.append({"name":"exact_header_identity",
                     "error":denied(lambda:Image.parse(raw,sha(raw),(0,)*6),'header identity')})
    bad_header=list(header);bad_header[5]=header[3]
    corrupt=bytearray(raw);struct.pack_into('<6I',corrupt,80,*bad_header)
    controls.append({"name":"entry_cannot_be_relocation_trailer",
                     "error":denied(lambda:Image.parse(bytes(corrupt),sha(corrupt),bad_header),'header bounds')})
    controls.append({"name":"allocation_wrap",
                     "error":denied(lambda:image.relocated(0xfffffff0),'allocation base')})
    corrupt=bytearray(raw);struct.pack_into('<H',corrupt,268,14)
    controls.append({"name":"relocation_write_site_outside_image",
                     "error":denied(lambda:Image.parse(bytes(corrupt),sha(corrupt),header),'write outside')})
    corrupt=bytearray(raw);struct.pack_into('<H',corrupt,268,7)
    controls.append({"name":"relocation_cannot_modify_future_stream",
                     "error":denied(lambda:Image.parse(bytes(corrupt),sha(corrupt),header),'immutable stream')})
    corrupt=bytearray(raw);struct.pack_into('<H',corrupt,264,2)
    controls.append({"name":"relocation_stream_missing_terminator",
                     "error":denied(lambda:Image.parse(bytes(corrupt),sha(corrupt),header),'truncated')})
    for delta,expected in [(0,bytes.fromhex('f0ffff1f00')),(1,bytes.fromhex('f0ffff0f10'))]:
        repeated=bytearray(raw)+b'\0\0';repeated_header=list(header);repeated_header[2]=18
        struct.pack_into('<6I',repeated,80,*repeated_header)
        struct.pack_into('<5H',repeated,264,2,0,0,delta,0)
        repeated_image=Image.parse(bytes(repeated),sha(repeated),repeated_header)
        observed=bytearray(repeated[256:]);observed[:5]=expected
        repeated_image.compare(bytes(observed),0x10000000)
        controls.append({"name":"sequential_"+('repeated' if not delta else 'overlapping')+"_fixup",
                         "passed":True})
    files=[Path(__file__).resolve(),Path(__file__).resolve().parents[1]/'watcom/cf_identity.py']
    receipt={"scope":"Pure byte controls; no runtime/base observation, code ownership or application acceptance",
             "sources":{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
             "images":images,"controls":controls,"elapsed_seconds":time.monotonic()-started}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(receipt,indent=2)+'\n')
    print('PASS: %d CF byte controls across three original containers in %.3fs; no execution'%
          (len(controls),receipt['elapsed_seconds']))


if __name__=='__main__':main()
