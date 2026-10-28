//! TLINK32 audit adapter. Recon export/runtime qualification reuses this core.
use crate::pe::Pe;
use anyhow::{Context, Result};
use std::{fs, path::Path};
#[path = "../../shared/tlink_map.rs"]
mod common;
pub use common::Publics;
fn parse_text(text: &str, pe: &Pe) -> Result<Publics> {
    common::parse_text(
        text,
        &common::Image {
            data: &pe.data,
            sections: pe
                .sections
                .iter()
                .map(|s| common::Section {
                    name: &s.name,
                    start: s.start,
                    size: s.size,
                    virtual_size: s.virtual_size,
                    off: s.off,
                    flags: s.flags,
                })
                .collect(),
        },
    )
}
pub fn parse(path: &Path, pe: &Pe) -> Result<Publics> {
    parse_text(&fs::read_to_string(path).context("read TLINK32 MAP")?, pe)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> (Pe, String) {
        let mut pe = crate::tests::image(&[0xc3; 0x20]);
        pe.sections[0].name = "CODE".into();
        pe.sections[0].start = 0x410000;
        pe.sections[0].size = 0x100;
        pe.sections[0].virtual_size = 0x100;
        pe.sections[0].flags = 0x60000000;
        for (name, start, size, off, flags) in [
            (".icode", 0x420000, 0x100, 0x300, 0x60000020),
            ("DATA", 0x430000, 0x10, 0x400, 0xc0000040),
        ] {
            pe.sections.push(crate::pe::Section {
                name: name.into(),
                start,
                size,
                virtual_size: 0x100,
                off,
                flags,
            });
        }
        let mut map = String::from(
            "\r\n Start         Length     Name                   Class\r\n 0001:00000000 000000020H _TEXT CODE\r\n 0002:00000000 000000010H _DATA DATA\r\n 0002:00000010 000000020H _BSS BSS\r\n 0002:00000030 000000000H _TLSCBA TLSCBA\r\n\r\nDetailed map of segments\r\n 0001:00000000 00000020 C=CODE S=_TEXT G=(none) M=one.cpp ACBP=29\r\n 0002:00000000 00000010 C=DATA S=_DATA G=DGROUP M=one.cpp ACBP=A9\r\n 0002:00000010 00000020 C=BSS S=_BSS G=DGROUP M=clib\\c0nt.ASM ACBP=A9\r\n",
        );
        for title in ["Name", "Value"] {
            map.push_str(&format!("\r\n  Address         Publics by {title}\r\n\r\n 0001:00000001       _function\r\n 0001:00000001  Idle _alias\r\n 0001:00000002       operator delete(void*)\r\n 0001:00000002       @Thing@method$qv\r\n 0002:00000004       _data\r\n 0002:00000014       _bss\r\n"));
        }
        (pe, map)
    }
    #[test]
    fn tlink32_real_logical_segments_aliases_and_cpp_display_names() {
        let (pe, map) = fixture();
        let parsed = parse_text(&map, &pe).unwrap();
        assert_eq!(parsed.addresses["_function"], 0x410001);
        assert_eq!(parsed.addresses["_alias"], 0x410001);
        assert_eq!(parsed.addresses["operator delete(void*)"], 0x410002);
        assert_eq!(parsed.addresses["@Thing@method$qv"], 0x410002);
        assert_eq!(parsed.addresses["_data"], 0x430004);
        assert_eq!(parsed.addresses["_bss"], 0x430014);
        assert_eq!(parsed.contributions.len(), 3);
        assert!(pe.file_backed_code(0x410000, 0x410020));
        assert_eq!(
            parsed.provenance["logical_segment_anchors"][1]["pe_section"],
            "DATA"
        );
        let duplicate = map.replace(
            " 0001:00000001       _function\r\n",
            " 0001:00000001       _function\r\n 0001:00000001       _function\r\n",
        );
        assert_eq!(
            parse_text(&duplicate, &pe).unwrap().addresses,
            parsed.addresses
        );
    }
    #[test]
    fn tlink32_rejects_unsupported_variants_and_ambiguous_or_unowned_publics() {
        let (pe, map) = fixture();
        for bad in [
            map.replace("Detailed map of segments","Detailed map of segments (16-bit)"),
            map.replace("0001:00000001","0001:0001"),
            map.replace("  Idle "," Other "),
            map.replace("0001:00000001       _function","0000:00000001       _function"),
            map.replace("0001:00000001       _function","0003:00000001       _function"),
            map.replace("0001:00000001       _function","0001:00000030       _function"),
            map.replacen("0001:00000001       _function","0001:00000002       _function",1),
            map.replace("000000020H _TEXT CODE","000000010H _TEXT CODE"),
            map.replace("000000020H _TEXT CODE","100000000H _TEXT CODE"),
            map.replace("C=DATA S=_DATA","C=CODE S=_DATA"),
            map.replace(" 0001:00000000 00000020 C=CODE", " 0001:00000001 00000020 C=CODE"),
            map.replace("\r\n  Address         Publics by Name", "\r\n 0001:00000001 00000001 C=CODE S=_TEXT G=(none) M=two.cpp ACBP=29\r\n  Address         Publics by Name"),
            map.replace(" 0001:00000001       _function", " 0001:00000001       _function\r\n 0001:00000002       _function"),
            map.split("  Address         Publics by Value").next().unwrap().into(),
        ] {
            assert!(parse_text(&bad,&pe).is_err(),"unexpectedly accepted {bad}");
        }
        let mut noncode = pe.clone();
        noncode.sections[0].flags = 0x40000040;
        assert!(parse_text(&map, &noncode).is_err());
        let mut truncated = pe.clone();
        truncated.sections[0].size = 0x10;
        assert!(parse_text(&map, &truncated).is_err());
        let mut ambiguous = pe.clone();
        ambiguous.sections.push(pe.sections[0].clone());
        assert!(parse_text(&map, &ambiguous).is_err());
        let mut wrong = pe;
        wrong.sections[2].name = ".data".into();
        assert!(parse_text(&map, &wrong).is_err());
    }
}
