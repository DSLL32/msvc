
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00027640 <.text+0x10640>:
   27640:	83 ec 18             	sub    esp,0x18
   27643:	56                   	push   esi
   27644:	57                   	push   edi
   27645:	be 7c 38 06 00       	mov    esi,0x6387c
   2764a:	8d 7c 24 08          	lea    edi,[esp+0x8]
   2764e:	b9 06 00 00 00       	mov    ecx,0x6
   27653:	f3 a5                	rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
   27655:	c6 44 24 08 0f       	mov    BYTE PTR [esp+0x8],0xf
   2765a:	8b 44 24 24          	mov    eax,DWORD PTR [esp+0x24]
   2765e:	50                   	push   eax
   2765f:	e8 1c f7 ff ff       	call   0x26d80
   27664:	89 44 24 18          	mov    DWORD PTR [esp+0x18],eax
   27668:	8d 44 24 0c          	lea    eax,[esp+0xc]
   2766c:	83 c4 04             	add    esp,0x4
   2766f:	50                   	push   eax
   27670:	e8 db f5 ff ff       	call   0x26c50
   27675:	5f                   	pop    edi
   27676:	5e                   	pop    esi
   27677:	83 c4 18             	add    esp,0x18
   2767a:	c2 04 00             	ret    0x4
