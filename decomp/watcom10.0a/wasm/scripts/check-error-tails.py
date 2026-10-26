#!/usr/bin/env python3
"""Finite RET residual for the cold InternalError and pure usage-exit paths.

The shared audit still checks all App graphs/extents/references and the
ordinary returning contracts. Pending InternalError remains masked and is never reconstructed.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import struct
import subprocess
import sys

project = Path(__file__).resolve().parent.parent
repo = project.parents[2]
profile = json.loads((project / 'harness/cf-profile.json').read_text())
original = repo / profile['target']['path']
raw = original.read_bytes()
assert hashlib.sha256(raw).hexdigest() == profile['target']['sha256']
payload = profile['target']['header']['image_file_offset']
rebuilt = project / 'build/WASM.EXE'


def instructions(path, start, end, raw_offset=0):
    args = ['objdump', '-D', '-Mintel', '--no-show-raw-insn']
    if raw_offset:
        args += ['-b', 'binary', '-m', 'i386']
    text = subprocess.check_output(args + ['--start-address=' + hex(start + raw_offset),
        '--stop-address=' + hex(end + raw_offset), str(path)], text=True)
    return [(int(a, 16) - raw_offset, m, operand.strip()) for a, m, operand in
        re.findall(r'^[ \t]*([0-9a-f]+):[ \t]+(\S+)[ \t]*(.*)$', text, re.M)]


def external_jumps(code, start, end, raw_offset=0):
    return [(a, m, int(operand, 16) - raw_offset) for a, m, operand in code
        if m.startswith('j') and re.fullmatch(r'0x[0-9a-f]+', operand)
        and not start <= int(operand, 16) - raw_offset < end]


def pe_bytes(address, size):
    data = rebuilt.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count, optional = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    base = struct.unpack_from('<I', data, pe + 52)[0]
    for k in range(count):
        at = pe + 24 + optional + 40 * k
        rva, length, offset = struct.unpack_from('<III', data, at + 12)
        if base + rva <= address and address + size <= base + rva + length:
            return data[offset + address - base - rva:offset + address - base - rva + size]
    raise AssertionError('unmapped rebuilt ABI bytes')


symbols = {name: 0x410000 + int(offset, 16) for offset, name in re.findall(
    r'^0001:([0-9a-fA-F]{8})[ *+]\s+(\S+)', (project / 'build/wasm.map').read_text(), re.M)}
owners = [
    ('FixGenPRef', 115804, 115923, '64f089edd9c4cf1c64c88f1c020348f2b191cb8124438ace3993e65fcfc44efa', [115906, 115922], [115824, 115843, 115872]),
    ('writeMisc32', 109844, 110026, '8a79847028db59cf4007bd80ed5aa94b9ac5b7f8e9038ccc80c122b34bd2b520', [110025], [109869, 109890, 109909, 109929]),
    ('writeLidata', 111420, 111756, 'd6b1acd281be263c4b25818df392a723703a8aeff0a45d042e18fd244d2bdb4d', [111755], [111445, 111466, 111485, 111505]),
    ('writeComdat', 112204, 112617, 'db15e5089e0288d532748739828c063dcb22d03452a34b595837c09554db9cbd', [112616], [112229, 112250, 112269, 112289, 112516]),
    ('writeLinsym', 113304, 113486, '772942f092c4d9b15bb589ba1b3324e05bd4a368244f74e1641e0a9fd00afe34', [113485], [113324, 113345, 113364, 113384]),
    ('LifixAdd', 116620, 116680, '6c09eb30492eeab47d47fb45acda0e6d3f7d04bb14c4aa92ece5a9d8dfec9795', [116679], [116648]),
    ('ObjGet8', 114232, 114284, 'f801e0ef07e8af37990f0ccbffb9868504cf4c94e9f575fc1797477fb245ac34', [114283], [114256]),
    ('ObjGet16', 114284, 114331, 'ee07eee83bf8f8b79c03f13afa85691fbdee82f2716305455a3d64f468a273b4', [114330], [114308]),

    ('fill_in_objfile_space', 82244, 82463, '07df95d39d02c40c475b2d2c2155334888d98d917b7cea35451f35db4665ce15', [82462], [82311]),
    ('FixOverride',0x124cc,0x12599,'5d4c76290c60a4069427a2dfccad96a1922d6f4ea279f62ad2d8952c3bac5402',[0x12540,0x12585,0x12598],[0x124f0,0x12523,0x12568]),
    ('AddFloatingPointEmulationFixup',0x192c0,0x19381,'6a9ca32fd35ad0d2faec80efa8a91ee5cb98a786f255243c65e478fdd974539a',[0x19380],[0x1933a]),
    ('ObjPut16',0x1bfdc,0x1c010,'b508785272d5472ce71da69c6daf858a72ad5da10b629e22c4f6045da1a803b3',[0x1c00f],[0x1bff6]),
    ('write_flat_grp',0xe188,0xe1f4,'824c49c6361866147b56fe3664a7d11ceea2b0400e5530d40a0a50d0b588a627',[0xe1f3],[0xe1a4]),
    ('FixGenRef', 115924, 116032, 'a0f6a48aaf04e9c5548369bd69ecaad9717ce618146819777f8cbe9402ab4501', [116018, 116031], [115949, 115968, 115997]),
    ('writeLinnumData', 112940, 113133, '190e156b55f2f3389d97622a4439f2a66234026143a5accda891c4013b4ba072', [113042, 113132], [112961, 112980, 113104]),
    ('writeLinnum', 113136, 113302, 'cc6e4e566baa12a19dc2b31d0671c7b17aeb630074e0aa41d98e481a0e482b45', [113301], [113156, 113177, 113196, 113216]),
    ('get_sr_rm_byte', 21396, 21472, 'f0c59408536b93aabb9608824f60f025a6b0fcd796f3679b0eca8fe59adf5cf6', [21436, 21440, 21444, 21448, 21452, 21456], [21467]),
    ('LocalDef', 76272, 76964, '6a903d48cb53699ce2658570e4e67eacec986b8dc9fea08a77a520dba0e02ada', [76405, 76432, 76463, 76654, 76704, 76751, 76807, 76933, 76963], [76331]),
    ('SimSeg',0x116e8,0x11d74,'e605a11234c07b99e152d6131df36b5c32d7e3ba15243810c28b99f6b9073c64',[0x118ce,0x11928,0x119b1,0x119fe,0x11a19,0x11a34,0x11ba7,0x11be2,0x11d64],[0x117d5,0x11d6f]),
    ('ObjWriteClose',0x1a610,0x1a645,'6cacc973890e604b9d4905677fe441210a124a158c8757e89b432d8fb4f57af4',[0x1a644],[0x1a622]),
    ('CarveDestroy',0x1aae0,0x1ab19,'21e6a6b7b2662fc98b5ab8a6c6f02ed002f5b306438ce2a2a161bdc1025f5a4a',[0x1ab18],[0x1aaf3]),
    ('writeModend',0x1b4f8,0x1b608,'e88cbe1ffee9784ea1b908cd3ea396667d9650cd67ef45b2bbfb3ed8e0666c3a',[0x1b607],[0x1b511,0x1b526,0x1b539,0x1b54d]),
    ('putTargetDatum',0x1c330,0x1c377,'319328c9403ba862a8586dcd60e6c2d9b43232ee09969a073355882b032b9139',[0x1c365,0x1c376],[0x1c347]),
    ('putFrameDatum',0x1c2dc,0x1c330,'56bd0231445b7c14456321118413aaa72921ae4847aaffbba4998999826a9ee4',[0x1c317,0x1c328,0x1c32f],[0x1c2f3]),
    ('FixGenLRef',0x1c378,0x1c459,'d27f777be1af5b1b63d0947d7c1ac0f9293a9c590d736812c46cb973ad3da8fe',[0x1c458],[0x1c392,0x1c3a5,0x1c3c2]),
    ('FixGenFix',0x1c55c,0x1c6e3,'c1fc967589c3c42bdcce505d6405cc2171ba1cd659bad343694526f0cb605364',[0x1c6e2],[0x1c574,0x1c587,0x1c5a4,0x1c619,0x1c67d,0x1c69f]),
    ('writeFixup',0x1b03c,0x1b14e,'2e964fc10e6d9c6fa224be2a5b64f00650821f0956e29c4cccc80062b7e84c36',[0x1b14d],[0x1b056,0x1b06b,0x1b07e,0x1b093]),
    ('GetDirIdx',0x10260,0x102bf,'82ac75195d3e5e942b690f2db4bc29d103fad9d57eedc9e31a4e226f927c0c76',[0x102a7,0x102af],[0x10279,0x102ba]),
    ('GetCurrSegStart',0x10374,0x103c9,'376b5ac75ae0a72aae8ea44ddf1a9d2c9da0d632e3ca12b17cdf8438e5949978',[0x10386,0x103c8],[0x103b2]),
    ('CreateFixupRec',0x92e0,0x962f,'17684e470097a71206d6e7ae9008307b9de36c26a9499f9ae0c99866b676634c',[0x9421,0x946c,0x9512,0x9625,0x962e],[0x9571]),
    ('writeLedata',0x1b150,0x1b284,'adc248cdf1aad00389a0c8f8469efe8af7521845281b88ffd8d973310dbda84b',[0x1b283],[0x1b169,0x1b17e,0x1b191,0x1b1a5,0x1b256]),
    ('SetCurrSeg',0x10b14,0x10bd1,'7ada3ad66bbd7da633268ea4d5e6ea2c4465a8b32ccc86fb8deb1ed9c6b7208b',[0x10bd0],[0x10b72]),
    ('writeComent',0x1adcc,0x1ae81,'13204052612e027c9863ecd3f609e58dc9125b68b77a4079def86ee91d5801cd',[0x1ae80],[0x1ade2,0x1adf7,0x1ae0a,0x1ae1e]),
    ('ObjWrite16',0x1a7fc,0x1a83a,'c0459978a42dd19ad531bde67b81b915d67fba0d05948ebd996111d24fc44d34',[0x1a839],[0x1a816]),
    ('ObjWrite',0x1a8ac,0x1a950,'7e61e66359fde97c71e2d0887046452130dc10319366694074c8148d61cd744a',[0x1a94f],[0x1a8cd]),
    ('writePubdef',0x1b7ec,0x1b92c,'41c1698456c7b899cd3ce56726b60dcaf5d732d32638ab01d7700710ba36f374',[0x1b92b],[0x1b806,0x1b822,0x1b835,0x1b849]),
    ('write_comm',0xe7d4,0xea66,'0b510f56d66591a66e5faadb7a5156b53c6039ebb48c089742416de704debe57',[0xea65],[0xe9e8]),
    ('ObjPutIndex',0x1c044,0x1c08f,'979297c984194dc1bf491d7e5fc45ac1dfb56c7d796e608cd8c7f90789fded62',[0x1c08e],[0x1c05e]),
    ('write_grp',0xe1f4,0xe339,'2728c1b3cfb1b8a431a0749f0fab2fc6aac0371e1ee2ce1c848a554c86d62496',[0xe338],[0xe24f]),
    ('objWFlushBuffer',0x1a6b0,0x1a718,'3191466412b9b5a1de611b757d48d16757326463647ff15bdb3fa4fdd16ed4fe',[0x1a717],[0x1a6c7]),
    ('ObjWEndRec',0x1a718,0x1a7bb,'c8f57d1c5464324be791c54b17b434f1c916393c1fb3874b2946e755d938edcb',[0x1a7ba],[0x1a734]),
    ('ObjWrite32',0x1a83c,0x1a879,'5330f339312a0a1865cf953bce9d4361135f29ecd13f39be66ff1291e64994f8',[0x1a878],[0x1a856]),
    ('ObjWrite8',0x1a7bc,0x1a7fb,'9ec1a21a9444527f9ae3d84f37a72ef63d08323abb0b91a6be8395141b4dfd8e',[0x1a7fa],[0x1a7d6]),
    ('ObjWBegRec',0x1a648,0x1a6ae,'9b47940dcf64c61b02944e76140f96b8fb1ff91cace52fb66a3f23bb40e6d597',[0x1a6ad],[0x1a667]),
    ('writeSegdef',0x1aea0,0x1b039,'2ed1da948cdf9b5b7dc1d3c09c6a899fd4c7b0b28965faf99968a95871721e90',[0x1b038],[0x1aeb9,0x1aece,0x1aee1,0x1aef5,0x1af8b]),
    ('ObjAttachData',0x1bdbc,0x1bdea,'fd7737594ae3abc0cedc4449cac0a9fb1715e2936f80ebf2f30858483958319e',[0x1bde9],[0x1bdd2]),
    ('write_lnames',0xe3c4,0xe519,'e9e97c2f924d05024f0904ac701f649b61a11af062eb4ea2f13a1b9e626c87da',[0xe518],[0xe424,0xe4f5]),
    ('ObjDetachData',0x1bdec,0x1be36,'a5ad5e926dbc02766cbe54a116831a36d24f0acac2afa95c992b52b76c16d4cc',[0x1be35],[0x1bdfe,0x1be13]),
    ('ObjWriteRec',0x1a968,0x1aa1e,'c74cc543228722c8663cc5754750e520f7ddb5cb91307197a56f085e26f80b09',[0x1aa1d],[0x1a98c]),
    ('ObjGet',0x1bf50,0x1bfa3,'3cbdda03443979a972b6a9e283ad8fd043fd8464eeb74ec7c7d3ccf7412fb496',[0x1bfa2],[0x1bf6b,0x1bf98]),
    ('writeMisc',0x1ac70,0x1ad11,'2a52d66c4e938a8e4598f652005374a996afa8ca38246f976ef1245fae1f1360',[0x1ad10],[0x1ac89,0x1ac9e,0x1acb1,0x1acc5]),
    ('writeTheadr',0x1b48c,0x1b4f7,'1505310b7e580da8096e666c58c701e486513ce5c783ded34f1b95d3cd587680',[0x1b4f6],[0x1b4a0,0x1b4b5,0x1b4c8,0x1b4dc]),
    ('write_record',0xdee8,0xdf37,'8cde4cabf85b4a69e97598186e5212b46b60ab085264e8839acb6344f39710d2',[0xdf36],[0xdf00]),
    ('ObjPut',0x1c090,0x1c0da,'90c9b88dc10843a45a336bf0eab2a0810fc331703bceefb50d3cae5d9e195c86',[0x1c0d9],[0x1c0ac]),
    ('ObjPut8',0x1bfa4,0x1bfda,'211985beae11d1e3769b9f1ff54cabb3fe428bf70b2c8c70510818b2dc193c4a',[0x1bfd9],[0x1bfbe]),
    ('ObjPutName',0x1c0dc,0x1c11b,'40cc1148bfff571f7956c321ba0dbf401b59ebe10e3e3b48ed6200ff0277fd58',[0x1c11a],[0x1c0f6]),
    ('ObjAllocData',0x1bd84,0x1bdbc,'71b56d04be8cacff6be077d19e057f95f99c993b138e807c3f115505ffd6e60c',[0x1bdbb],[0x1bd9a]),
    ('write_epilogue',0x13844,0x138e7,'eb48d66ae8cf88c5b5a28dc5c155402e024ebaff26a03b9c14b996283c9fa246',[0x138e6],[0x1385d]),
    ('conditional_assembly_directive',0x17fe0,0x1842f,'eaa55463902e2eab14c5082c10a74945611ca394ba2f759552aba90a673ae901',[98384, 98479, 98508, 98720, 98802, 98843, 98881, 98919, 98958, 98996, 99048, 99097, 99149, 99198, 99220, 99236, 99260, 99294, 99313, 99358, 99374],[0x180d7]),
    ('jmp',0xbd94,0xc707,'2cff7ef5b30785b311eee53f98d7e9540a9500e806586180e7678d0c50757e3e',[48601, 49183, 49228, 49261, 49353, 49518, 49872, 50078, 50092, 50208, 50700, 50950],[0xc5d5]),
    ('check_assume',0x4064,0x4162,'0872476c2e10036abdb214595b7d2eb4442b0ac122debec28318c2851c0e7a0e',[16627, 16640, 16667, 16681, 16695, 16709, 16723, 16737],[0x4091]),
    ('get_precedence',0x14340,0x1450c,'86b6e78318f691b21aa7424185138ea3d45e579d7d95bae76e5954b8d53fdf2c',[82957, 82964, 83098, 83112, 83133, 83140, 83147, 83154, 83161, 83168, 83175, 83182, 83189, 83211],[0x143a9,0x143bd,0x14500]),
    ('my_sprintf',0x171f0,0x17320,'c450878fcce81ad6d12f3c4088b91eadb9112044e930b02dedf17daa50fd28d7',[0x1731f],[0x172bb]),
    ('seg_override',0x3f34,0x404a,'f3a053c91cdbab6629b0a57b3917ecb359609f322641aafdb0848203596ef334',[0x4049],[0x3fc0]),
    ('mem',0x4278,0x4b4a,'2f80a128983af3df35e217d1486907531685008b017fe5ab48d4dc8979f0a08c',[17448, 17504, 17658, 17712, 17772, 17905, 17944, 18110, 18219, 18246, 18273, 18348, 18395, 18434, 18564, 18598, 18657, 18716, 18805, 18888, 18919, 19032, 19088, 19202, 19273],[0x4aa8]),
    ('AddFixup',0x8fe4,0x90c9,'ae374efce71afdbf53ffa5298c3602f72b3750330f349529dccd12b6da9b5dd1',[0x90c8],[0x9078]),
    ('optr_enqueue',0x14620,0x14655,'83964da07b5808558d234450f5fa41c0e67cd03e6e8bdcf80734df283766db21',[0x14654],[0x14645]),
    ('ExpandProcString',0x19534,0x199de,'656d368d513dbcecf383bc7be3f17430e33df9c1b0142baa3f1efb8a68970bc2',
     [0x1968d,0x196c2,0x196ec,0x199dd],[0x195f5]),
    ('WritePrologue',0x13398,0x137e3,'9844c8513b0713769090ebae1db4bd93a9f61018eac5f7dbc9bdd06e5e631e99',
     [0x137e2],[0x133d5]),
    ('proc_exam',0x12cd8,0x131c6,'126a2b3f86fe84fee948307748bc09e96d80bd0835b6076ec282936b527373a0',
     [0x12f4f,0x12fb1,0x13007,0x1302a,0x13046,0x13065,0x131a6,0x131c5],[0x12f67]),
    ('ProcDef',0x131c8,0x132ef,'cbf9c5f5bafa4ec2ac13b54127f5e6abb259f98c02725ed0173d517a77ef3898',
     [0x13203,0x13236,0x13299,0x132ee],[0x132a8,0x132d1]),
    ('GrpDef',0x10990,0x10b14,'7c59e413eb18d3ed725788e638ad27e7bb7f579bf3461d9d3635141b7254ca8c',
     [0x10a5d,0x10b13],[0x109ea]),
    ('pop_seg',0xf9a4,0xf9c9,'d2d4498bd9deb750ddd96823be1aa2a0115c8b1612d6dfd6699bfeb9cf7677d5',
     [0xf9c8],[0xf9b8]),
    ('CarveAlloc',0x1ab1c,0x1ab83,'5aed6e97bbf296c47f4315bf96ab7e22736478837efb6f61b7661690f4ea9c08',
     [0x1ab82],[0x1ab31,0x1ab58]),
    ('SegDef',0x10cd0,0x1139d,'192c618bce52109da248f1b47db71674c1d18531c73c6c61b3f322c364947514',
     [69457, 69543, 69583, 69799, 69935, 69960, 69993, 70126, 70233, 70345, 70379, 70415, 70451, 70528, 70556],[0x11296]),
    ('fix',0x15950,0x15e00,'3ba71525430f2266e75db412bec24db75c0bda82df10c8428ce401ff02d789bb',
     [0x15dff],[0x15dbe]),
    ('CarveFree',0x1ab84,0x1aba8,'a883f1a473c712d97090b65fc601313b3ee8b45aee9e28b63eddf0f8215babfd',
     [0x1aba7],[0x1ab98]),
    ('ObjKillRec',0x1bce4,0x1bd84,'f1a49b377fa55dd0e476e3c4aa7433b595ccb9b9dcd429996cb3159ba1fbe7e6',
     [0x1bd83],[0x1bcf6,0x1bd68]),
    ('replace_parm',0x169ac,0x16b77,'cca6b84767e82413f7b85f93638ee08cc0edd945146622b9c4315fcfc484796d',
     [0x16b4f,0x16b76],[0x16aaf]),
    ('MacroDef',0x17938,0x17a2d,'bcb10cc6ed473613634d6776e3c47cd2aa4c74d267585080cff0a16c95be2b76',
     [0x179bd,0x17a2c],[0x1795b]),
    ('evaluate',0x1529c,0x15699,'1e93d57ee0858d1b572ffb6a27be769a93c748e031bd52b06c53f09f7e1be22f',
     [86826, 87039, 87103, 87177, 87226, 87265, 87296, 87450, 87585, 87652, 87704],[0x15346]),
    ('get_module_name', 0x11f58, 0x11f99, '63ee3d4c43d3daa31f75b14d199201a24267ffdbc99f85efcf359ac437d6d6b0',
     [0x11f98], [0x11f74]),
    ('QEnqueue', 0x1c190, 0x1c1ca, '3d54da321de293c1394077c9f468d612ac6395de08dfbb38d2a4cf24801d8858',
     [0x1c1c9], [0x1c1b4]),
    ('dir_insert', 0xfa3c, 0xfce6, '2c4a71433d309f2841f5c0f4133f58208025f160e00134a81e3f37b12e7d534f',
     [0xfb2d, 0xfbbd, 0xfc51, 0xfc64, 0xfc73, 0xfcc1, 0xfce5], [0xfad2]),
    ('fix_final', 0x15e08, 0x15f55, '0da2e6cfb45e02dabbbd2a2916c67246b9528ecf38fa39847d485425aeece9d1',
     [0x15f54], [0x15e67, 0x15eec]),
]
report = []
selected_owner = os.environ.get("WASM_RET_OWNER")
for name, start, end, digest, returns, tails in owners:
    if selected_owner and name != selected_owner:
        continue
    assert hashlib.sha256(raw[payload + start:payload + end]).hexdigest() == digest
    code = instructions(original, start, end, payload)
    assert [(a, operand) for a, m, operand in code if m.startswith('ret')] == [(a, '') for a in returns]
    assert external_jumps(code, start, end, payload) == [(a, 'jmp', 0xf6f8) for a in tails]
    begin = symbols[name + '_']
    limit = min(a for a in symbols.values() if a > begin)
    if name == 'dir_insert':
        # The thirteen-way switch lives between the cold assertion tail and
        # dispatch. Its relocated addresses can decode as RET opcodes.
        span = json.loads((project / 'harness/audit-policy.json').read_text())[            'graph_metadata']['functions']['dir_insert']['rebuilt']['text_data'][0]
        assert [item['rebuilt_symbol'] for item in span] == ['dir_insert_'] * 2
        table_begin, table_end = [begin + item['offset'] for item in span]
        assert table_end - table_begin == 52
        targets = struct.unpack('<13I', pe_bytes(table_begin, 52))
        assert all(table_end <= target < limit for target in targets)
        assert pe_bytes(table_end, 10) == bytes.fromhex('837df00c0f877d020000')
        assert pe_bytes(table_end + 16, 3) == bytes.fromhex('2effa0')
        assert struct.unpack('<I', pe_bytes(table_end + 19, 4))[0] == table_begin
        code = instructions(rebuilt, begin, table_begin) + instructions(rebuilt, table_end, limit)
    elif name == 'check_assume':
        # Exact six-way switch data, also declared to the ordinary graph audit.
        span = json.loads((project / 'harness/audit-policy.json').read_text())['graph_metadata']['functions'][name]['rebuilt']['text_data'][0]
        assert [item['rebuilt_symbol'] for item in span] == ['check_assume_'] * 2
        table_begin, table_end = [begin + item['offset'] for item in span]
        assert table_end - table_begin == 24
        assert all(table_end <= target < limit for target in struct.unpack('<6I',pe_bytes(table_begin,24)))
        assert pe_bytes(table_end,12) == bytes.fromhex('837df805774d8b45f8c1e002')
        assert pe_bytes(table_end+12,3) == bytes.fromhex('2effa0')
        assert struct.unpack('<I',pe_bytes(table_end+15,4))[0] == table_begin
        code = instructions(rebuilt,begin,table_begin) + instructions(rebuilt,table_end,limit)
    else:
        # Declared text data is data even when relocated target bytes resemble
        # RET opcodes. Keep decoding every instruction outside those ranges.
        metadata = json.loads((project / 'harness/audit-policy.json').read_text())
        spans = metadata['graph_metadata']['functions'].get(name, {}).get(
            'rebuilt', {}).get('text_data', [])
        ranges = []
        for span in spans:
            assert [item['rebuilt_symbol'] for item in span] == [name + '_'] * 2
            low, high = [begin + item['offset'] for item in span]
            assert begin <= low < high <= limit
            ranges.append((low, high))
        code = []
        cursor = begin
        for low, high in sorted(ranges):
            assert cursor <= low
            code += instructions(rebuilt, cursor, low)
            cursor = high
        code += instructions(rebuilt, cursor, limit)
    rebuilt_returns = [(a, operand) for a, m, operand in code if m.startswith('ret')]
    assert rebuilt_returns and all(not operand for _, operand in rebuilt_returns), (name, rebuilt_returns)
    jumps = external_jumps(code, begin, limit)
    # The period -Od build merges get_precedence's three original default
    # assertion tails into one shared default block. SimSeg's two original
    # switch-default copies similarly merge to its one source default. Both end
    # only at pending InternalError; ordinary decoded RET0 checks stay above.
    expected_rebuilt_tails = 1 if name in ('get_precedence','SimSeg') else len(tails)
    assert len(jumps) == expected_rebuilt_tails and all(m == 'jmp' and target == symbols['InternalError_'] for _, m, target in jumps)
    report.append({'name': name, 'original_sha256': digest, 'original_ret0': returns,
        'original_error_tails': tails, 'rebuilt_extent': [begin, limit], 'rebuilt_ret0': rebuilt_returns,
        'rebuilt_error_tails': jumps, 'rebuilt_sha256': hashlib.sha256(pe_bytes(begin, limit - begin)).hexdigest()})

# Exact error-limit terminator; same standard exit(1) closure as usage_msg.
if not selected_owner or selected_owner == 'AsmSuicide':
    assert raw[payload+0x164a4:payload+0x164ae] == bytes.fromhex('b801000000e983660000')
    original_code = instructions(original,0x164a4,0x164ae,payload)
    assert not any(m.startswith('ret') for _,m,_ in original_code)
    assert external_jumps(original_code,0x164a4,0x164ae,payload) == [(0x164a9,'jmp',0x1cb31)]
    begin = symbols['AsmSuicide_']
    limit = min(a for a in symbols.values() if a > begin)
    code = instructions(rebuilt,begin,limit)
    calls = [(a,int(operand,16)) for a,m,operand in code if m == 'call']
    assert [target for _,target in calls] == [symbols['exit_']]
    assert pe_bytes(calls[0][0]-5,5) == bytes.fromhex('b801000000')
    assert not external_jumps(code,begin,limit)
    report.append({'name':'AsmSuicide','original_exit':[0x164a9,0x1cb31,1],
        'rebuilt_exit_call':calls[0][0],'status':1,
        'rebuilt_sha256':hashlib.sha256(pe_bytes(begin,limit-begin)).hexdigest()})

# The CF callback-table flag cannot interpret relocated pointers. Reuse the
# shared CF identity reader here for this finite six-record fatal contract.
if not selected_owner or selected_owner == 'Fatal':
    sys.path.insert(0, str(repo / 'tooling/recon-tool/watcom'))
    from cf_identity import Image
    target = profile['target']
    header = target['header']
    image = Image.parse(raw,target['sha256'],[0x4643,
        header['image_file_offset'],header['initialized_size'],
        header['relocation_offset'],header['allocation_size'],header['entry_offset']])
    assert hashlib.sha256(image.initialized).hexdigest() == target['initialized_sha256']
    assert hashlib.sha256(image.initialized[image.relocation_offset:]).hexdigest() == target['relocation_sha256']
    assert len(image.fixups) == target['relocation_count'] and image.groups == target['relocation_groups']
    table = next(row for row in json.loads((project / 'harness/audit-policy.json').read_text())[
        'graph_metadata']['callback_tables'] if row['label'] == 'Fatal_Msg')
    assert table['original'] == [0xf540,0xf5a0]
    assert (table['count'],table['stride'],table['pointer_offset']) == (6,16,8)
    assert table['expected_functions'] == ['AsmShutDown',None,None,'AsmShutDown','AsmShutDown',None]
    table_begin = symbols[table['rebuilt_begin']]
    table_fixups = [offset for offset in image.fixups if 0xf540 <= offset < 0xf5a0]
    assert table_fixups == [0xf548,0xf578,0xf588]
    for index,name in enumerate(table['expected_functions']):
        left = struct.unpack_from('<4I',image.initialized,0xf540+16*index)
        right = struct.unpack('<4I',pe_bytes(table_begin+16*index,16))
        assert left[:2]+left[3:] == right[:2]+right[3:]
        if name:
            assert left[2] == 0xf5a0 and right[2] == symbols[name+'_']
            assert 0xf548+16*index in image.fixups
        else:
            assert left[2] == right[2] == 0
            assert not any(0xf548+16*index-3 <= x <= 0xf548+16*index+3 for x in image.fixups)
    # A NULL action skips the indirect call; the exit argument comes from
    # the same row's final DWORD. Both facts are about actual instructions.
    assert raw[payload+0xf67d:payload+0xf68c] == bytes.fromhex('83ba48f50000007406ff9248f50000')
    assert raw[payload+0xf693:payload+0xf699] == bytes.fromhex('8b804cf50000')
    code = instructions(original,0xf614,0xf69e,payload)
    assert not any(m.startswith('ret') for _,m,_ in code)
    assert external_jumps(code,0xf614,0xf69e,payload) == [(0xf699,'jmp',0x1cb31)]
    begin = symbols['Fatal_']
    limit = min(a for a in symbols.values() if a > begin)
    body = pe_bytes(begin,limit-begin)
    callback = bytes.fromhex('ff90')+struct.pack('<I',table_begin+8)
    assert body.count(callback) == 1
    callback_at = begin+body.index(callback)
    guarded = bytes.fromhex('83b8')+struct.pack('<I',table_begin+8)+bytes.fromhex('0074')
    assert body.count(guarded) == 1
    guard_at = begin+body.index(guarded)
    assert guard_at+9+struct.unpack('<b',pe_bytes(guard_at+8,1))[0] == callback_at+6
    load = bytes.fromhex('8b80')+struct.pack('<I',table_begin+12)
    assert body.count(load) == 1
    exit_call = begin+body.index(load)+len(load)
    code = instructions(rebuilt,begin,limit)
    assert (exit_call,'call',hex(symbols['exit_'])) in code
    assert not external_jumps(code,begin,limit)
    report.append({'name':'Fatal','original_table':[0xf540,0xf5a0],
        'callback_fixups':table_fixups,'rebuilt_table':table_begin,
        'original_exit_tail':[0xf699,0x1cb31],'rebuilt_exit_call':exit_call,
        'selected_missing_source_row':1,'selected_callback':None,'selected_exit':1,
        'rebuilt_sha256':hashlib.sha256(body).hexdigest()})

if selected_owner:
    assert len(report) == 1, "unknown finite RET owner"
    if len(sys.argv) == 2:
        Path(sys.argv[1]).write_text(json.dumps(report, indent=2) + '\n')
    print('PASS finite target RET/termination contract: ' + selected_owner)
    sys.exit(0)

# FixNew delegates directly to the same CarveAlloc whose normal RET0 and
# cold assertion tails are checked above. No cleanup is invented for the tail.
assert raw[payload+0x1c274:payload+0x1c27e] == bytes.fromhex('a1987b0200e99ee8ffff')
code = instructions(original,0x1c274,0x1c27e,payload)
assert not any(m.startswith('ret') for _,m,_ in code)
assert external_jumps(code,0x1c274,0x1c27e,payload) == [(0x1c279,'jmp',0x1ab1c)]
begin = symbols['FixNew_']
limit = min(a for a in symbols.values() if a > begin)
code = instructions(rebuilt,begin,limit)
assert [int(o,16) for _,m,o in code if m == 'call'] == [symbols['CarveAlloc_']]
assert not external_jumps(code,begin,limit)
returns = [(a,o) for a,m,o in code if m.startswith('ret')]
assert returns and all(not o for _,o in returns)
report.append({'name':'FixNew','original_tail':[0x1c279,0x1ab1c],
    'qualified_target':'CarveAlloc normal RET0/cold InternalError residual above',
    'rebuilt_ret0':returns,'rebuilt_sha256':hashlib.sha256(pe_bytes(begin,limit-begin)).hexdigest()})

# FixFini's exact10B tail delegates to CarveDestroy. Its complete57B
# body has one normal RET0 and one canonical cold assertion exit; this is
# a finite static ABI fact; runtime body acceptance is tracked separately.
assert raw[payload+0x1c268:payload+0x1c272] == bytes.fromhex('a1987b0200e96ee8ffff')
assert hashlib.sha256(raw[payload+0x1aae0:payload+0x1ab19]).hexdigest() == '21e6a6b7b2662fc98b5ab8a6c6f02ed002f5b306438ce2a2a161bdc1025f5a4a'
code = instructions(original,0x1aae0,0x1ab19,payload)
assert [(a,o) for a,m,o in code if m.startswith('ret')] == [(0x1ab18,'')]
assert external_jumps(code,0x1aae0,0x1ab19,payload) == [(0x1aaf3,'jmp',0xf6f8)]
assert [int(o,16)-payload for _,m,o in code if m == 'call'] == [0x16900,0x16900]
begin = symbols['FixFini_']
limit = min(a for a in symbols.values() if a > begin)
code = instructions(rebuilt,begin,limit)
assert [int(o,16) for _,m,o in code if m == 'call'] == [symbols['CarveDestroy_']]
assert not external_jumps(code,begin,limit)
returns = [(a,o) for a,m,o in code if m.startswith('ret')]
assert returns and all(not o for _,o in returns)
report.append({'name':'FixFini','original_tail':[0x1c26d,0x1aae0],
    'target_sha256':'21e6a6b7b2662fc98b5ab8a6c6f02ed002f5b306438ce2a2a161bdc1025f5a4a','target_normal_ret0':[0x1ab18],
    'target_cold_tail':[0x1aaf3,0xf6f8],'rebuilt_ret0':returns,
    'rebuilt_sha256':hashlib.sha256(pe_bytes(begin,limit-begin)).hexdigest()})

# ObjRecFini uses the other actual carve owner and the same proven target.
assert raw[payload+0x1bca8:payload+0x1bcb2] == bytes.fromhex('a1947b0200e92eeeffff')
code = instructions(original,0x1bca8,0x1bcb2,payload)
assert not any(m.startswith('ret') for _,m,_ in code)
assert external_jumps(code,0x1bca8,0x1bcb2,payload) == [(0x1bcad,'jmp',0x1aae0)]
begin = symbols['ObjRecFini_']
limit = min(a for a in symbols.values() if a > begin)
code = instructions(rebuilt,begin,limit)
assert [int(o,16) for _,m,o in code if m == 'call'] == [symbols['CarveDestroy_']]
assert not external_jumps(code,begin,limit)
returns = [(a,o) for a,m,o in code if m.startswith('ret')]
assert returns and all(not o for _,o in returns)
report.append({'name':'ObjRecFini','original_tail':[0x1bcad,0x1aae0],
    'qualified_target':'CarveDestroy normalRET0/cold InternalError residual above',
    'rebuilt_ret0':returns,'rebuilt_sha256':hashlib.sha256(pe_bytes(begin,limit-begin)).hexdigest()})

# safeSeek has a conditional cold tail to a distinct pending nonreturning App.
# Neither that body nor its unobserved failure behavior receives acceptance.
seek = raw[payload + 0x1a5b0:payload + 0x1a5bf]
assert hashlib.sha256(seek).hexdigest() == '9ebfdc08d18232faec66547925662f781e56ef025ce5a71a8584195ad495149d'
code = instructions(original,0x1a5b0,0x1a5bf,payload)
assert [(a,o) for a,m,o in code if m.startswith('ret')] == [(0x1a5be,'')]
assert external_jumps(code,0x1a5b0,0x1a5bf,payload) == [(0x1a5b8,'je',0xf72c)]
assert [(a,int(o,16)-payload) for a,m,o in code if m == 'call'] == [(0x1a5b0,0x1e4d9)]
disk = raw[payload + 0xf72c:payload + 0xf740]
assert hashlib.sha256(disk).hexdigest() == '3239512190deb71f1c1eb3d861718d8bb9cfc8e9365732923ba423cf6b747ae2'
code = instructions(original,0xf72c,0xf740,payload)
assert not any(m.startswith('ret') for _,m,_ in code)
assert external_jumps(code,0xf72c,0xf740,payload) == [(0xf73b,'jmp',0x1cb31)]
assert disk[10:15] == bytes.fromhex('b8ff000000')
begin = symbols['safeSeek_']
limit = min(a for a in symbols.values() if a > begin)
code = instructions(rebuilt,begin,limit)
returns = [(a,o) for a,m,o in code if m.startswith('ret')]
assert returns and all(not o for _,o in returns)
assert not external_jumps(code,begin,limit)
assert [int(o,16) for _,m,o in code if m == 'call'] == [symbols['lseek_'],symbols['DiskError_']]
report.append({'name':'safeSeek','original_sha256':'9ebfdc08d18232faec66547925662f781e56ef025ce5a71a8584195ad495149d',
    'original_ret0':[0x1a5be],'original_error_tail':[0x1a5b8,'je',0xf72c],
    'pending_DiskError_sha256':'3239512190deb71f1c1eb3d861718d8bb9cfc8e9365732923ba423cf6b747ae2','pending_DiskError_exit':[0xf73b,0x1cb31,255],
    'rebuilt_ret0':returns,'rebuilt_sha256':hashlib.sha256(pe_bytes(begin,limit-begin)).hexdigest()})

# Actual assertion helper has no RET and terminates through standard exit(255).
error = raw[payload + 0xf6f8:payload + 0xf724]
assert hashlib.sha256(error).hexdigest() == '2cabf02bd1a565689928a4133792fa341fc1c40c92e4c52649ab295bb7c2098f'
code = instructions(original, 0xf6f8, 0xf724, payload)
assert not any(m.startswith('ret') for _, m, _ in code)
assert external_jumps(code, 0xf6f8, 0xf724, payload) == [(0xf71f, 'jmp', 0x1cb31)]
assert raw[payload + 0xf71a:payload + 0xf71f] == bytes.fromhex('b8ff000000')
assert '#pragma aux InternalError aborts;' in (project / 'src/wasm.h').read_text()
registry = (project / 'binary.c').read_text().split('.name = "InternalError"', 1)[1].split('},', 1)[0]
assert 'RECON_UNIMPLEMENTED' in registry and '.code =' not in registry

# Pure help termination owns no ordinary RET contract. The rebuilt CALL exit(1)
# cannot return, even if this compiler emits an unreachable fallback epilogue.
assert hashlib.sha256(raw[payload + 0xcb60:payload + 0xcb74]).hexdigest() == '7edf180b312e91c27027d4ee9838f4ee251a361a77d90af5746251d72e18ce02'
assert raw[payload + 0xcb6a:payload + 0xcb6f] == bytes.fromhex('b801000000')
code = instructions(original, 0xcb60, 0xcb74, payload)
assert not any(m.startswith('ret') for _, m, _ in code)
assert external_jumps(code, 0xcb60, 0xcb74, payload) == [(0xcb6f, 'jmp', 0x1cb31)]
begin = symbols['usage_msg_']
limit = min(a for a in symbols.values() if a > begin)
code = instructions(rebuilt, begin, limit)
calls = [(a, operand) for a, m, operand in code if m == 'call']
assert [int(operand, 16) for _, operand in calls] == [symbols['PrintfUsage_'], symbols['exit_']]
assert pe_bytes(calls[-1][0] - 5, 5) == bytes.fromhex('b801000000')
report.append({'name': 'usage_msg', 'rebuilt_exit_call': calls[-1][0], 'status': 1,
    'rebuilt_sha256': hashlib.sha256(pe_bytes(begin, limit - begin)).hexdigest()})
if len(sys.argv) == 2:
    output = Path(sys.argv[1])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
print('PASS finite RET0/cold InternalError→exit(255) residuals; usage_msg→exit(1), pending code stays masked')
