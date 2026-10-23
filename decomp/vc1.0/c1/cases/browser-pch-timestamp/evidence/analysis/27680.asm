
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00027680 <.text+0x10680>:
   27680:	83 ec 18             	sub    esp,0x18
   27683:	56                   	push   esi
   27684:	57                   	push   edi
   27685:	be 7c 38 06 00       	mov    esi,0x6387c
   2768a:	8d 7c 24 08          	lea    edi,[esp+0x8]
   2768e:	b9 06 00 00 00       	mov    ecx,0x6
   27693:	f3 a5                	rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
   27695:	c6 44 24 08 10       	mov    BYTE PTR [esp+0x8],0x10
   2769a:	8d 44 24 08          	lea    eax,[esp+0x8]
   2769e:	50                   	push   eax
   2769f:	e8 ac f5 ff ff       	call   0x26c50
   276a4:	c7 05 74 38 06 00 01 	mov    DWORD PTR ds:0x63874,0x1
   276ab:	00 00 00 
   276ae:	5f                   	pop    edi
   276af:	5e                   	pop    esi
   276b0:	83 c4 18             	add    esp,0x18
   276b3:	c3                   	ret
