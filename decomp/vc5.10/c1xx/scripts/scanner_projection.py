"""Encountered initialized lexer dispatch/flag data; no section-gap ownership."""
TABLES=[('classes',0xcc008,2028),('flags',0xcc108,2284)]
INITIALIZED_SIZE=2540
KNOWN_INITIALIZED_BYTES=2489
UNKNOWN_INITIALIZED_BYTES=6031

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
