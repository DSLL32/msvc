
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

0004c9f5 <.text+0x359f5>:
   4c9f5:	56                   	push   esi
   4c9f6:	8b 74 24 08          	mov    esi,DWORD PTR [esp+0x8]
   4c9fa:	80 3e 79             	cmp    BYTE PTR [esi],0x79
   4c9fd:	75 0a                	jne    0x4ca09
   4c9ff:	6a 1a                	push   0x1a
   4ca01:	e8 ca e0 ff ff       	call   0x4aad0
   4ca06:	83 c4 04             	add    esp,0x4
   4ca09:	80 3d 48 b1 05 00 2c 	cmp    BYTE PTR ds:0x5b148,0x2c
   4ca10:	75 11                	jne    0x4ca23
   4ca12:	a1 50 b1 05 00       	mov    eax,ds:0x5b150
   4ca17:	0b c0                	or     eax,eax
   4ca19:	74 08                	je     0x4ca23
   4ca1b:	80 3e 00             	cmp    BYTE PTR [esi],0x0
   4ca1e:	74 03                	je     0x4ca23
   4ca20:	50                   	push   eax
   4ca21:	eb 0e                	jmp    0x4ca31
   4ca23:	0f b6 05 48 b1 05 00 	movzx  eax,BYTE PTR ds:0x5b148
   4ca2a:	ff 34 85 a0 30 07 00 	push   DWORD PTR [eax*4+0x730a0]
   4ca31:	e8 ee 18 00 00       	call   0x4e324
   4ca36:	50                   	push   eax
   4ca37:	68 90 01 00 00       	push   0x190
   4ca3c:	e8 8f df ff ff       	call   0x4a9d0
   4ca41:	83 c4 0c             	add    esp,0xc
   4ca44:	c7 05 2c b1 05 00 01 	mov    DWORD PTR ds:0x5b12c,0x1
   4ca4b:	00 00 00 
   4ca4e:	5e                   	pop    esi
   4ca4f:	c2 04 00             	ret    0x4
