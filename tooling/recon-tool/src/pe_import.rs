//! Add a normal loader dependency to an original-only scratch PE32 image.
//! Existing code, imports, section locations and overlays stay in place.
use anyhow::{Context, Result, ensure};

fn word(bytes: &[u8], offset: usize) -> Result<u16> {
    let end = offset.checked_add(2).context("PE word offset overflow")?;
    Ok(u16::from_le_bytes(
        bytes
            .get(offset..end)
            .context("truncated PE word")?
            .try_into()?,
    ))
}
fn dword(bytes: &[u8], offset: usize) -> Result<u32> {
    let end = offset.checked_add(4).context("PE dword offset overflow")?;
    Ok(u32::from_le_bytes(
        bytes
            .get(offset..end)
            .context("truncated PE dword")?
            .try_into()?,
    ))
}
fn put(bytes: &mut [u8], offset: usize, value: u32) {
    bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
}
fn align(value: u32, alignment: u32) -> Result<u32> {
    ensure!(alignment.is_power_of_two(), "invalid PE alignment");
    Ok(value
        .checked_add(alignment - 1)
        .context("PE alignment overflow")?
        & !(alignment - 1))
}

/// The DLL must be a basename; the caller places its verified helper beside the
/// scratch executable. The symbol is its actual PE export spelling.
pub fn add_helper_import(image: &[u8], dll: &str, symbol: &str) -> Result<Vec<u8>> {
    ensure!(
        !dll.is_empty()
            && dll.len() <= 255
            && dll
                .bytes()
                .all(|b| b.is_ascii_alphanumeric() || b"._-".contains(&b))
            && dll != "."
            && dll != "..",
        "helper DLL must be a simple ASCII basename"
    );
    ensure!(
        !symbol.is_empty()
            && symbol.len() <= 4096
            && symbol.bytes().all(|b| (33..=126).contains(&b)),
        "invalid helper import symbol"
    );
    ensure!(image.get(..2) == Some(b"MZ"), "scratch image is not PE");
    let pe = dword(image, 0x3c)? as usize;
    ensure!(
        pe <= image.len().saturating_sub(24),
        "PE header beyond file"
    );
    ensure!(
        image.get(pe..pe + 4) == Some(b"PE\0\0") && word(image, pe + 4)? == 0x14c,
        "scratch image must be i386 PE"
    );
    let optional = pe + 24;
    let optional_size = word(image, pe + 20)? as usize;
    ensure!(
        optional_size <= image.len() - optional,
        "optional PE header beyond file"
    );
    ensure!(
        optional_size >= 112
            && word(image, optional)? == 0x10b
            && dword(image, optional + 92)? >= 2,
        "scratch image requires PE32 import directories"
    );
    let count = word(image, pe + 6)? as usize;
    ensure!(count > 0 && count < 96, "unsupported PE section count");
    let table = optional + optional_size;
    let new_header = table + count * 40;
    let header_end = new_header + 40;
    let headers_size = dword(image, optional + 60)? as usize;
    let file_alignment = dword(image, optional + 36)?;
    let section_alignment = dword(image, optional + 32)?;
    ensure!(
        file_alignment.is_power_of_two()
            && (512..=65536).contains(&file_alignment)
            && section_alignment.is_power_of_two()
            && section_alignment >= file_alignment,
        "unsupported PE section/file alignment"
    );
    ensure!(
        header_end <= headers_size
            && headers_size <= image.len()
            && image
                .get(new_header..header_end)
                .is_some_and(|b| b.iter().all(|&v| v == 0)),
        "no unused PE section-header slot; header relocation is unsupported"
    );
    // Authenticode cannot remain valid after a transformation. Do not silently
    // retain a security directory that would claim to authenticate this copy.
    if optional_size >= 136 && dword(image, optional + 92)? > 4 {
        ensure!(
            dword(image, optional + 128)? == 0 && dword(image, optional + 132)? == 0,
            "signed scratch PE images are unsupported"
        );
    }
    let mut sections = Vec::new();
    let mut virtual_end = u32::try_from(headers_size)?;
    for i in 0..count {
        let s = table + i * 40;
        let size = dword(image, s + 8)?;
        let rva = dword(image, s + 12)?;
        let raw_size = dword(image, s + 16)?;
        let raw = dword(image, s + 20)?;
        virtual_end = virtual_end.max(
            rva.checked_add(size.max(raw_size))
                .context("PE section RVA overflow")?,
        );
        // Old linkers give uninitialized sections a size but no file offset.
        if raw_size != 0 && raw != 0 {
            let end = raw
                .checked_add(raw_size)
                .context("PE raw section overflow")?;
            ensure!(
                raw as usize >= headers_size && end as usize <= image.len(),
                "invalid PE raw section"
            );
            ensure!(
                header_end <= raw as usize,
                "section header overlaps PE data"
            );
            sections.push((rva, raw, raw_size));
        }
    }
    sections.sort_unstable_by_key(|s| s.0);
    for pair in sections.windows(2) {
        ensure!(
            pair[0].0 + pair[0].2 <= pair[1].0,
            "overlapping initialized PE RVA ranges"
        );
    }
    let mapped = |address: u32| -> Result<(usize, usize)> {
        let &(rva, raw, size) = sections
            .iter()
            .find(|&&(rva, _, size)| address >= rva && address - rva < size)
            .context("import reference is not in initialized section data")?;
        let relative = address - rva;
        Ok(((raw + relative) as usize, (size - relative) as usize))
    };
    let import_rva = dword(image, optional + 104)?;
    let import_size = dword(image, optional + 108)?;
    let mut descriptors = Vec::new();
    if import_rva != 0 {
        let (start, available) = mapped(import_rva)?;
        let limit = if import_size == 0 {
            available
        } else {
            available.min(import_size as usize)
        };
        let mut terminated = false;
        for offset in (0..limit.saturating_sub(19)).step_by(20) {
            let descriptor = &image[start + offset..start + offset + 20];
            if descriptor.iter().all(|&v| v == 0) {
                terminated = true;
                break;
            }
            ensure!(
                dword(descriptor, 12)? != 0 && dword(descriptor, 16)? != 0,
                "invalid original import descriptor"
            );
            let (name, available) = mapped(dword(descriptor, 12)?)?;
            let bytes = &image[name..name + available];
            let end = bytes
                .iter()
                .position(|&b| b == 0)
                .context("unterminated original import name")?;
            ensure!(
                !bytes[..end].eq_ignore_ascii_case(dll.as_bytes()),
                "helper DLL name collides with an existing import"
            );
            descriptors.extend_from_slice(descriptor);
        }
        ensure!(terminated, "unterminated original import directory");
    } else {
        ensure!(import_size == 0, "import size without directory RVA");
    }
    let new_rva = align(
        virtual_end.max(dword(image, optional + 56)?),
        section_alignment,
    )?;
    let raw_offset = align(u32::try_from(image.len())?, file_alignment)?;
    let descriptor_bytes = u32::try_from(descriptors.len() + 40)?;
    let int_offset = descriptor_bytes;
    let iat_offset = int_offset + 8;
    let dll_offset = iat_offset + 8;
    let name_offset = align(dll_offset + u32::try_from(dll.len() + 1)?, 2)?;
    let data_size = name_offset
        .checked_add(u32::try_from(symbol.len() + 3)?)
        .context("helper import size overflow")?;
    let raw_size = align(data_size, file_alignment)?;
    let image_size = align(
        new_rva
            .checked_add(data_size)
            .context("helper image size overflow")?,
        section_alignment,
    )?;
    let end = raw_offset
        .checked_add(raw_size)
        .context("helper file size overflow")?;
    let mut result = image.to_vec();
    result.resize(end as usize, 0);
    result[new_header..new_header + 8].copy_from_slice(b".recon\0\0");
    for (off, value) in [
        (8, data_size),
        (12, new_rva),
        (16, raw_size),
        (20, raw_offset),
        (36, 0xc0000040),
    ] {
        put(&mut result, new_header + off, value);
    }
    result[pe + 6..pe + 8].copy_from_slice(&((count + 1) as u16).to_le_bytes());
    put(&mut result, optional + 56, image_size);
    put(&mut result, optional + 64, 0); // The old checksum describes the pristine file.
    put(&mut result, optional + 104, new_rva);
    put(&mut result, optional + 108, descriptor_bytes);
    let initialized = dword(image, optional + 8)?
        .checked_add(raw_size)
        .context("initialized data size overflow")?;
    put(&mut result, optional + 8, initialized);
    let data = &mut result[raw_offset as usize..end as usize];
    data[..descriptors.len()].copy_from_slice(&descriptors);
    let helper = descriptors.len();
    put(data, helper, new_rva + int_offset);
    put(data, helper + 12, new_rva + dll_offset);
    put(data, helper + 16, new_rva + iat_offset);
    put(data, int_offset as usize, new_rva + name_offset);
    put(data, iat_offset as usize, new_rva + name_offset);
    data[dll_offset as usize..dll_offset as usize + dll.len()].copy_from_slice(dll.as_bytes());
    data[name_offset as usize + 2..name_offset as usize + 2 + symbol.len()]
        .copy_from_slice(symbol.as_bytes());
    Ok(result)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn fixture() -> Vec<u8> {
        let mut b = vec![0u8; 0x600];
        b[..2].copy_from_slice(b"MZ");
        put(&mut b, 0x3c, 0x80);
        b[0x80..0x84].copy_from_slice(b"PE\0\0");
        b[0x84..0x86].copy_from_slice(&0x14cu16.to_le_bytes());
        b[0x86..0x88].copy_from_slice(&1u16.to_le_bytes());
        b[0x94..0x96].copy_from_slice(&224u16.to_le_bytes());
        b[0x98..0x9a].copy_from_slice(&0x10bu16.to_le_bytes());
        for (o, v) in [
            (0xb8, 0x1000),
            (0xbc, 0x200),
            (0xd0, 0x2000),
            (0xd4, 0x200),
            (0xf4, 16),
            (0x100, 0x1100),
            (0x104, 40),
        ] {
            put(&mut b, o, v);
        }
        b[0x178..0x180].copy_from_slice(b".text\0\0\0");
        for (o, v) in [
            (0x180, 0x400),
            (0x184, 0x1000),
            (0x188, 0x400),
            (0x18c, 0x200),
            (0x19c, 0x60000020),
            (0x300, 0x1140),
            (0x30c, 0x1180),
            (0x310, 0x1160),
        ] {
            put(&mut b, o, v);
        }
        b[0x200] = 0xc3;
        // Keep an ordinal import, its IAT, and an overlay byte distinguishable.
        put(&mut b, 0x340, 0x80000007);
        put(&mut b, 0x360, 0x80000007);
        b[0x380..0x38d].copy_from_slice(b"KERNEL32.dll\0");
        b.push(0xa5);
        b
    }
    #[test]
    fn preserves_existing_sections_imports_and_overlay() {
        let before = fixture();
        let after = add_helper_import(&before, "probe.dll", "_probe@0").unwrap();
        assert_eq!(&after[0x178..0x1a0], &before[0x178..0x1a0]);
        assert_eq!(&after[0x200..before.len()], &before[0x200..]);
        assert_eq!(word(&after, 0x86).unwrap(), 2);
        assert_eq!(dword(&after, 0x100).unwrap(), 0x2000);
        assert_eq!(dword(&after, 0x104).unwrap(), 60);
        assert_eq!(&after[0x800..0x814], &before[0x300..0x314]);
        assert!(after[0x828..0x83c].iter().all(|&v| v == 0));
        assert_eq!(dword(&after, 0x814).unwrap(), 0x203c);
        assert_eq!(dword(&after, 0x824).unwrap(), 0x2044);
        assert_eq!(dword(&after, 0x83c).unwrap(), 0x2056);
        assert_eq!(&after[0x858..0x861], b"_probe@0\0");
    }
    #[test]
    fn rejects_occupied_headers_bad_extents_and_unterminated_imports() {
        let mut b = fixture();
        b[0x1a0] = 1;
        assert!(add_helper_import(&b, "probe.dll", "probe").is_err());
        let mut b = fixture();
        put(&mut b, 0x18c, 0x580);
        assert!(add_helper_import(&b, "probe.dll", "probe").is_err());
        let mut b = fixture();
        put(&mut b, 0x104, 20);
        assert!(add_helper_import(&b, "probe.dll", "probe").is_err());
        for dll in ["../probe.dll", "C:probe.dll", "", ".."] {
            assert!(add_helper_import(&fixture(), dll, "probe").is_err());
        }
    }
    #[test]
    fn supports_images_without_imports_but_rejects_signatures() {
        let mut b = fixture();
        put(&mut b, 0x100, 0);
        put(&mut b, 0x104, 0);
        let after = add_helper_import(&b, "probe.dll", "probe").unwrap();
        assert_eq!(dword(&after, 0x104).unwrap(), 40);
        put(&mut b, 0x118, 0x600);
        put(&mut b, 0x11c, 1);
        assert!(add_helper_import(&b, "probe.dll", "probe").is_err());
    }
    #[test]
    fn rejects_existing_provider_collision_and_ambiguous_mapping() {
        assert!(add_helper_import(&fixture(), "kernel32.DLL", "probe").is_err());
        let mut b = fixture();
        b[0x86..0x88].copy_from_slice(&2u16.to_le_bytes());
        b.copy_within(0x178..0x1a0, 0x1a0);
        assert!(add_helper_import(&b, "probe.dll", "probe").is_err());
        let mut b = fixture();
        put(&mut b, 0x3c, u32::MAX);
        assert!(add_helper_import(&b, "probe.dll", "probe").is_err());
    }
}
