
../../../toolchains/1.0-nt/bin/C13232.EXE:     file format pei-i386


Disassembly of section .text:

00032580 <.text+0x1b580>:
   32580:	53                   	push   ebx
   32581:	56                   	push   esi
   32582:	57                   	push   edi
   32583:	55                   	push   ebp
   32584:	8b 5c 24 14          	mov    ebx,DWORD PTR [esp+0x14]
   32588:	8b 73 08             	mov    esi,DWORD PTR [ebx+0x8]
   3258b:	66 8b 43 02          	mov    ax,WORD PTR [ebx+0x2]
   3258f:	66 3d e8 03          	cmp    ax,0x3e8
   32593:	7c 0b                	jl     0x325a0
   32595:	66 05 e8 03          	add    ax,0x3e8
   32599:	eb 0c                	jmp    0x325a7
   3259b:	05 00 00 00 00       	add    eax,0x0
   325a0:	8d 04 45 04 00 00 00 	lea    eax,[eax*2+0x4]
   325a7:	66 89 43 02          	mov    WORD PTR [ebx+0x2],ax
   325ab:	0f bf 43 02          	movsx  eax,WORD PTR [ebx+0x2]
   325af:	8d 3c 85 03 00 00 00 	lea    edi,[eax*4+0x3]
   325b6:	83 e7 fc             	and    edi,0xfffffffc
   325b9:	8b 43 04             	mov    eax,DWORD PTR [ebx+0x4]
   325bc:	0b c0                	or     eax,eax
   325be:	75 08                	jne    0x325c8
   325c0:	57                   	push   edi
   325c1:	e8 da 25 01 00       	call   0x44ba0
   325c6:	eb 2a                	jmp    0x325f2
   325c8:	83 f8 ff             	cmp    eax,0xffffffff
   325cb:	75 02                	jne    0x325cf
   325cd:	2b c0                	sub    eax,eax
   325cf:	c1 e0 02             	shl    eax,0x2
   325d2:	8d ac 80 d0 e1 06 00 	lea    ebp,[eax+eax*4+0x6e1d0]
   325d9:	39 7d 0c             	cmp    DWORD PTR [ebp+0xc],edi
   325dc:	7d 0b                	jge    0x325e9
   325de:	55                   	push   ebp
   325df:	68 c8 e1 06 00       	push   0x6e1c8
   325e4:	e8 17 80 01 00       	call   0x4a600
   325e9:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
   325ec:	29 7d 0c             	sub    DWORD PTR [ebp+0xc],edi
   325ef:	01 7d 08             	add    DWORD PTR [ebp+0x8],edi
   325f2:	89 43 08             	mov    DWORD PTR [ebx+0x8],eax
   325f5:	66 83 3b 00          	cmp    WORD PTR [ebx],0x0
   325f9:	7e 17                	jle    0x32612
   325fb:	53                   	push   ebx
   325fc:	e8 1f 00 00 00       	call   0x32620
   32601:	8b 7b 08             	mov    edi,DWORD PTR [ebx+0x8]
   32604:	8b c8                	mov    ecx,eax
   32606:	c1 e9 02             	shr    ecx,0x2
   32609:	f3 a5                	rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
   3260b:	8b c8                	mov    ecx,eax
   3260d:	83 e1 03             	and    ecx,0x3
   32610:	f3 a4                	rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]
   32612:	8b c3                	mov    eax,ebx
   32614:	5d                   	pop    ebp
   32615:	5f                   	pop    edi
   32616:	5e                   	pop    esi
   32617:	5b                   	pop    ebx
   32618:	c2 04 00             	ret    0x4
