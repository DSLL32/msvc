"""Extract exact current C bodies for an isolated verification build only."""
from pathlib import Path
import re,sys
source=Path('src/listing.c').read_text();bodies=[]
groups = dict(bytes=['write_listing_bytes', 'write_listing_spaces', 'pad_listing_column'],
              special=['write_special_instruction'], text=['write_instruction_text'],
              mnemonic=['write_instruction_mnemonic', 'write_listing_spaces'])
names = groups[sys.argv[2]] if len(sys.argv) > 2 else ['write_instruction_listing', 'pad_listing_column']
for name in names:
    matches=re.findall(r'^[^\n]*\b'+name+r'\([^\n]*\)\n\{.*?^\}',source,re.M|re.S)
    assert len(matches)==1,(name,len(matches));bodies.append(matches[0])
if len(sys.argv) > 2 and sys.argv[2] == 'mnemonic':
    for name in ['listing_mnemonics', 'listing_condition_names']:
        matches=re.findall(r'^const char \*'+name+r'\[.*?^\};',source,re.M|re.S)
        assert len(matches)==1,(name,len(matches));bodies.insert(0,matches[0])
header=''.join('#include "'+str(Path('src',name).resolve())+'"\n'for name in ['listing.h','options.h','instruction-encoding.h','instruction-rewrite.h','machine-layout.h'])+'#include <recon.h>\n'
Path(sys.argv[1]).write_text('/* Verification only: exact app C bodies, never an application build input. */\n'+header+'\n\n'.join(bodies)+'\n')
