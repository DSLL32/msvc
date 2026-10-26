//! Watcom MAP audit adapter; export uses the same coordinate parser.
use crate::pe::Pe;
use anyhow::{Context, Result};
use std::{fs, path::Path};
#[path = "../../shared/watcom_map.rs"]
mod common;
pub use common::Publics;
fn parse_text(text: &str, pe: &Pe) -> Result<Publics> {
    let image = common::Image {
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
    };
    common::parse_text(text, &image)
}
pub fn parse(path: &Path, pe: &Pe) -> Result<Publics> {
    parse_text(&fs::read_to_string(path).context("read Watcom MAP")?, pe)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> (Pe, String) {
        let mut b = vec![0u8; 0x700];
        b[..2].copy_from_slice(b"MZ");
        b[60..64].copy_from_slice(&0x80u32.to_le_bytes());
        b[0x80..0x84].copy_from_slice(b"PE\0\0");
        b[0x86..0x88].copy_from_slice(&3u16.to_le_bytes());
        b[0x94..0x96].copy_from_slice(&224u16.to_le_bytes());
        b[0x98..0x9a].copy_from_slice(&0x10bu16.to_le_bytes());
        b[0xb4..0xb8].copy_from_slice(&0x400000u32.to_le_bytes());
        for (n, name, rva, size, off, flags) in [
            (0, "BEGTEXT", 0x10000u32, 0x100u32, 0x400u32, 0x60000020u32),
            (1, "DGROUP", 0x20000, 0x40, 0x500, 0xc0000040),
            (2, ".bss", 0x30000, 0x40, 0, 0xc0000080),
        ] {
            let p = 0x178 + n * 40;
            b[p..p + name.len()].copy_from_slice(name.as_bytes());
            for (o, v) in [(12, rva), (16, size), (20, off), (36, flags)] {
                b[p + o..p + o + 4].copy_from_slice(&v.to_le_bytes());
            }
        }
        let mut map = String::from(
            "WATCOM Linker Version 10.0\ncreating a PE executable\n|   Groups   |\nDGROUP 0002:00000000 00010040\n|   Segments   |\nSegment                Class          Group          Address         Size\n",
        );
        for (name, class, group, address, size) in [
            ("BEGTEXT", "CODE", "", "0001:00000000", "00000040"),
            ("_DATA", "DATA", "DGROUP", "0002:00000000", "00000040"),
            ("_BSS", "BSS", "DGROUP", "0002:00010000", "00000040"),
        ] {
            map.push_str(&format!(
                "{name:<23}{class:<15}{group:<15}{address:<16}{size}\n"
            ));
        }
        map.push_str("|   Memory Map   |\n0001:00000010+ Foo_\n0001:00000010* Alias_\n0002:00010004  _bss_public\n0000:00001234  __DOSseg__\n");
        (Pe::parse(b).unwrap(), map)
    }
    #[test]
    fn actual_pe_group_coordinates_reach_bss_and_keep_aliases() {
        let (pe, map) = fixture();
        let p = parse_text(&map, &pe).unwrap();
        assert_eq!(p.addresses["Foo_"], 0x410010);
        assert_eq!(p.addresses["Alias_"], 0x410010);
        assert_eq!(p.addresses["_bss_public"], 0x430004);
        assert_eq!(p.absolutes["__DOSseg__"], 0x1234);
        assert!(!p.addresses.contains_key("__DOSseg__"));
        let map = map + "0001:00000010  Foo_\n";
        assert!(parse_text(&map, &pe).is_ok());
    }
    #[test]
    fn empty_data_section_end_is_metadata_not_a_function_public() {
        let (pe, map) = fixture();
        let segment = format!(
            "{:<23}{:<15}{:<15}{:<16}{}\n",
            "TI", "DATA", "DGROUP", "0002:00000040", "00000000"
        );
        let declared = map.replace("|   Memory Map   |", &(segment + "|   Memory Map   |"));
        let positive = declared.clone() + "0002:00000040 __start_TI\n0002:00000040 __end_TI\n";
        let parsed = parse_text(&positive, &pe).unwrap();
        assert_eq!(
            parsed.provenance["boundary_symbols"]["__start_TI"],
            0x420040
        );
        assert_eq!(parsed.provenance["boundary_symbols"]["__end_TI"], 0x420040);
        assert!(!parsed.addresses.contains_key("__start_TI"));
        assert!(!parsed.absolutes.contains_key("__start_TI"));
        assert_eq!(parsed.addresses, parse_text(&map, &pe).unwrap().addresses);
        for wrong in [
            map + "0002:00000040 __start_TI\n",
            positive.replace("00000040", "00000080"),
            positive.replace("TI                     DATA", "TI                     CODE"),
            positive.replace("0002:00000040   00000000", "0002:00000040   00000001"),
        ] {
            assert!(parse_text(&wrong, &pe).is_err(), "{wrong}");
        }
        assert!(
            parse_text(&declared, &pe)
                .unwrap()
                .provenance
                .get("boundary_symbols")
                .is_none()
        );
    }

    #[test]
    fn map_conflicts_unmapped_and_unowned_publics_reject() {
        let (pe, map) = fixture();
        for changed in [
            map.clone() + "0001:00000011 Foo_\n",
            map.replace("0002:00010004  _bss_public", "0002:00020004  _bss_public"),
            map.replace("0001:00000010+ Foo_", "0001:00000050+ Foo_"),
            map.replace(
                "DGROUP 0002:00000000 00010040",
                "DGROUP 0003:00000000 00010040",
            ),
            map.replace("_BSS                   BSS", "_BSS                   CODE"),
            map.replace("Version 10.0", "Version 11.0"),
        ] {
            assert!(parse_text(&changed, &pe).is_err());
        }
    }
}
