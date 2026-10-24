"""Encountered initialized lexer dispatch/flag data; no section-gap ownership."""
TABLES=[('classes',0x5e008,1836),('flags',0x5e108,2092)]
INITIALIZED_SIZE=2348
KNOWN_INITIALIZED_BYTES=2348
UNKNOWN_INITIALIZED_BYTES=4200

def defaults():
 classes=[33]*256
 groups={0:[0,26,63,92],1:[9,11,12,13,32],3:[47],4:[40],5:[10],
  6:list(range(65,76))+list(range(77,91))+[95]+list(range(97,123)),7:[76],8:[42],9:[46],
  10:[33],11:[35],12:[37],13:[61],14:[94],15:[124],16:[38],17:[43],18:[45],
  19:[60],20:[62],21:[39],22:[34],23:[126],24:[125],25:[93],26:[58],27:[44],28:[41],
  29:list(range(48,58)),30:[123],31:[91],32:[59],34:[64],36:[36,96]}
 for kind,characters in groups.items():
  for character in characters:classes[character]=kind
 flags=[0]*256
 groups={0x20:[42],0x40:[9,11,12,13],
  0x60:list(range(1,9))+list(range(14,26))+list(range(27,32))+[127]+list(b'!#$%&()+,-:;<=>@[]^`{|}~'),
  0x64:[46],0x65:list(b'GIJKLMNPRSTUVWXYZ_gijklmnprstuvwxyz'),0x66:list(b'89'),
  0x67:list(b'ACEFacef'),0x6e:list(b'01234567'),0x75:list(b'HOQhoq'),0x77:list(b'BDbd'),
  0x80:[0,26,92],0xa0:[10,63],0xc0:[32,47],0xe0:[34,39]}
 seen=set()
 for kind,characters in groups.items():
  for character in characters:
   assert character not in seen;seen.add(character);flags[character]=kind
 assert seen==set(range(128))
 return dict(classes=classes,flags=flags)

def field_rows():
 values=defaults()
 return [dict(name='scanner.'+name,original_rva=rva,bytes=256,native_field='c1_data.scanner.'+name,native_offset=offset,
  snapshot_original_offset=rva-0x5e000,snapshot_native_offset=offset,default=values[name],c_type='BYTE[256]',
  default_proof='complete own canonical pre-entry defaults; unsigned byte lexer consumers; actual72-byte multibyte initializer writes these exact tables')for name,rva,offset in TABLES]

def read(raw):
 return {name:list(raw[rva-0x5e000:rva-0x5e000+256])for name,rva,offset in TABLES}

def seed(which):
 return {name:[((69+index)*7+i+(11 if which=='snapshot'else 31))&255 for i in range(256)]for index,(name,rva,offset)in enumerate(TABLES)}

def expected_fields(fields):
 rows=[row for row in fields['modeled_initialized']if row['name'].startswith('scanner.')]
 assert rows==field_rows()
 assert fields['multibyte_scanner']==dict(initialized_bytes=512,native_offset=1836,native_size=512,known_initialized_bytes=2348,unknown_initialized_bytes=4200,
  classes_rva=0x5e008,flags_rva=0x5e108,initializer_chunks=[[0x12f08,0x12f3e],[0x307c0,0x307d2]],lead_class=35,lead_flag_or=128,mbctype_mask=4)
 return {r['name']:(r['original_rva'],r['bytes'],r['native_offset'])for r in rows}
