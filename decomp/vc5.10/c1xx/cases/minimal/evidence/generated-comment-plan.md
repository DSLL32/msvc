# Generated compiler comment

`initialize_generated_comment` owns RVA38af9..38bdb,226 bytes, cdecl(void).
The incoming ECX is not consumed: the local ECX value set from globalc6b64 is
immediately overwritten before use. No stack arguments, calls or SEH roots occur.
The wrapper and cleanup chunk remain separately owned.

The inline string operations concatenate the four literals at their points of
use. The encountered value has73 characters plus NUL. This proves74 semantic
character bytes atc6ba8, not the size of the original buffer. Surrounding original
bytes remain unmodeled and unchanged under the complete BSS guard. The prefix
before `XXXXXXXX).` has length63; total length73. Those unsigned values are
stored atc6cb8 andc6c44. The later header output and file-comparison paths a722b,
a728f,a91c0..a91f6 consume the text/total length; a935a..a9370 uses the prefix
offset for formatting a value. These consumers remain unaccepted.

The int atc6b64 is a COM initialization flag: a1c55 writes1 and calls imported
CoInitialize; destructor346d4 tests it and cold6f28f clears it before the genuine
CoUninitialize tail import. Only its constructor reset is now reconstructed.
Cleanup remains an uninvoked named stub.

Schema26 adds86 semantic BSS bytes and two natural native alignment bytes.
The complete modeled BSS is1045 semantic bytes plus31 native padding bytes;
8431 initialized original bytes remain unmodeled. Three ordinary/restore pairs
and an explicitly auxiliary nonzero actual-entry pair verify all new fields.
No code, allocator, callback or return is mocked. Raw entry/cleanup stack slots
and actual caller PCs are checked. The native preprologue formal-variable issue
was corrected with a fresh native-only run, retaining the unchanged original
and earlier failed native capture as immutable records.

`comment-constructor-trials.json` records the pairs and22 fresh negative controls.
