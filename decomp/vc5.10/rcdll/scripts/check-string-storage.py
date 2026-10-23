#!/usr/bin/env python3
"""Check actual production union objects and interior pointers in plain Linux."""
import argparse,json,shutil,subprocess,time
from pathlib import Path
from protocol import HERE,sha
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
assert not a.output.exists();a.output.mkdir(parents=True)
for source,name in [('build/native/string-storage-host','probe.elf'),('build/native-source.json','production-source.json'),('build/native-string-storage-source.json','probe-source.json')]:shutil.copy2(HERE/source,a.output/name)
start=time.perf_counter();r=subprocess.run([str(a.output/'probe.elf')],capture_output=True,timeout=5);elapsed=time.perf_counter()-start
(a.output/'stdout.bin').write_bytes(r.stdout);(a.output/'stderr.bin').write_bytes(r.stderr)
assert r.returncode==0 and not r.stderr and r.stdout==b'2 0 1 0 4090 2048 4660 205 171\n'
result=dict(status='pass',seconds=elapsed,extra_bytes=2,hash_offset=0,length_offset=1,string_length_offset=0,buffer_bytes=4090,number_pointer_byte_offset=2048,byte_pair_34_12_reads_WORD=4660,WORD_abcd_reads_bytes=[205,171],scope='Host mutates the actual linked production union object in a separate native process. No reconstructed function is replaced or accepted by this auxiliary layout check. Buffer defaults, actual reader writes/tails and original writer/bound instructions are proved by the fresh masked pair.',files={f.name:sha(f)for f in a.output.iterdir()if f.is_file()},checker_sha256=sha(Path(__file__)))
(a.output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
