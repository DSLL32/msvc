"""Extract exact current C bodies for an isolated verification build only."""
from pathlib import Path
import re,sys
source=Path('src/listing.c').read_text();bodies=[]
names = (['write_listing_bytes', 'write_listing_spaces', 'pad_listing_column']
         if len(sys.argv) > 2 and sys.argv[2] == 'bytes'
         else ['write_instruction_listing', 'pad_listing_column'])
for name in names:
    matches=re.findall(r'^[^\n]*\b'+name+r'\([^\n]*\)\n\{.*?^\}',source,re.M|re.S)
    assert len(matches)==1,(name,len(matches));bodies.append(matches[0])
header=''.join('#include "'+str(Path('src',name).resolve())+'"\n'for name in ['listing.h','options.h','instruction-encoding.h','machine-layout.h'])+'#include <recon.h>\n'
Path(sys.argv[1]).write_text('/* Verification only: exact app C bodies, never an application build input. */\n'+header+'\n\n'.join(bodies)+'\n')
