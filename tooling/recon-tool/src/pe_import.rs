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

// Some linkers put the bound-import directory in otherwise spare header space.
// Its module-name offsets are relative to this block, so preserve the complete
// validated block and relocate only its directory RVA in the scratch image.
fn bound_header_metadata(
    image: &[u8],
    pe: usize,
    optional: usize,
    optional_size: usize,
    new_header: usize,
    header_end: usize,
    headers_size: usize,
    table: usize,
    count: usize,
) -> Result<Vec<u8>> {
    let directories = dword(image, optional + 92)? as usize;
    ensure!(
        directories > 11 && directories <= (optional_size - 96) / 8,
        "occupied section header lacks bounded import metadata"
    );
    let start = dword(image, optional + 96 + 11 * 8)? as usize;
    let size = dword(image, optional + 100 + 11 * 8)? as usize;
    let end = start
        .checked_add(size)
        .context("bound import header overflow")?;
    ensure!(
        size >= 8
            && size <= 65536
            && start >= new_header
            && end <= headers_size
            && start < header_end
            && image.get(start..end).is_some(),
        "bound imports are not spare header metadata"
    );
    ensure!(
        image[new_header..header_end]
            .iter()
            .enumerate()
            .all(|(i, &byte)| byte == 0 || (start..end).contains(&(new_header + i))),
        "section header contains other data"
    );
    for i in 0..directories {
        if i == 11 {
            continue;
        }
        let rva = dword(image, optional + 96 + i * 8)? as usize;
        let len = dword(image, optional + 100 + i * 8)? as usize;
        if rva == 0 {
            continue;
        }
        let last = rva.checked_add(len).context("PE directory overflow")?;
        ensure!(
            !(rva < end && (len == 0 || last > start))
                && !(rva < header_end && (len == 0 || last > new_header)),
            "bound imports overlap another PE directory"
        );
    }
    let symbol_table = dword(image, pe + 12)? as usize;
    ensure!(
        symbol_table == 0 || symbol_table >= headers_size,
        "COFF symbol table occupies headers"
    );
    for i in 0..count {
        for field in [24, 28] {
            let raw = dword(image, table + i * 40 + field)? as usize;
            ensure!(
                raw == 0 || raw >= headers_size,
                "COFF relocation/line table occupies headers"
            );
        }
    }
    let block = &image[start..end];
    let mut at = 0usize;
    let mut names = Vec::new();
    loop {
        ensure!(
            at.checked_add(8).is_some_and(|end| end <= size),
            "unterminated bound import table"
        );
        let stamp = dword(block, at)?;
        let name = word(block, at + 4)? as usize;
        let forwards = word(block, at + 6)? as usize;
        at += 8;
        if stamp == 0 && name == 0 && forwards == 0 {
            break;
        }
        ensure!(name != 0, "bound import lacks module name");
        names.push(name);
        ensure!(
            forwards <= (size - at) / 8,
            "bound forwarders exceed directory"
        );
        for _ in 0..forwards {
            names.push(word(block, at + 4)? as usize);
            ensure!(
                word(block, at + 6)? == 0,
                "bound forwarder reserved field is nonzero"
            );
            at += 8;
        }
    }
    ensure!(
        !names.is_empty(),
        "empty bound import directory does not own header data"
    );
    for name in names {
        ensure!(
            name >= at && name < size,
            "bound module name outside name area"
        );
        let tail = &block[name..];
        let end = tail
            .iter()
            .position(|&v| v == 0)
            .context("unterminated bound module name")?;
        ensure!(
            end > 0 && end <= 255 && tail[..end].is_ascii(),
            "invalid bound module name"
        );
    }
    Ok(block.to_vec())
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
        header_end <= headers_size && headers_size <= image.len(),
        "no spare PE section-header capacity; header relocation is unsupported"
    );
    let bound_in_slot = if optional_size >= 192 && dword(image, optional + 92)? > 11 {
        let start = dword(image, optional + 184)? as usize;
        let size = dword(image, optional + 188)? as usize;
        let end = start
            .checked_add(size)
            .context("bound import header overflow")?;
        start != 0 && start < header_end && (size == 0 || end > new_header)
    } else {
        false
    };
    let bound_metadata = if !bound_in_slot && image[new_header..header_end].iter().all(|&v| v == 0)
    {
        None
    } else {
        Some(bound_header_metadata(
            image,
            pe,
            optional,
            optional_size,
            new_header,
            header_end,
            headers_size,
            table,
            count,
        )?)
    };
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
    let bound_offset = align(data_size, 4)?;
    let data_size = if let Some(block) = &bound_metadata {
        bound_offset
            .checked_add(u32::try_from(block.len())?)
            .context("bound metadata size overflow")?
    } else {
        data_size
    };
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
    result[new_header..header_end].fill(0);
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
    if bound_metadata.is_some() {
        put(
            &mut result,
            optional + 96 + 11 * 8,
            new_rva
                .checked_add(bound_offset)
                .context("bound metadata RVA overflow")?,
        );
    }
    let data = &mut result[raw_offset as usize..end as usize];
    if let Some(block) = &bound_metadata {
        data[bound_offset as usize..bound_offset as usize + block.len()].copy_from_slice(block);
    }
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
    fn bound_fixture() -> Vec<u8> {
        let mut b = fixture();
        put(&mut b, 0x150, 0x1a0);
        put(&mut b, 0x154, 40);
        put(&mut b, 0x1a0, 0x11223344);
        b[0x1a4..0x1a6].copy_from_slice(&24u16.to_le_bytes());
        b[0x1a6..0x1a8].copy_from_slice(&1u16.to_le_bytes());
        put(&mut b, 0x1a8, 0x55667788);
        b[0x1ac..0x1ae].copy_from_slice(&30u16.to_le_bytes());
        b[0x1b8..0x1be].copy_from_slice(b"A.dll\0");
        b[0x1be..0x1c4].copy_from_slice(b"B.dll\0");
        b
    }
    #[test]
    fn relocates_bound_forwarders_and_rejects_malformed_header_ownership() {
        let b = bound_fixture();
        let after = add_helper_import(&b, "probe.dll", "probe").unwrap();
        let image = crate::original_image::Image::parse(&after).unwrap();
        let rva = dword(&after, 0x150).unwrap();
        let offset = image.file_offset(rva, 40).unwrap();
        assert_eq!(&after[offset..offset + 40], &b[0x1a0..0x1c8]);
        assert_eq!(&after[0x200..b.len()], &b[0x200..]);
        for (offset, value) in [
            (0x1a4, 0xffffu16),
            (0x1a6, 0xffff),
            (0x1ac, 39),
            (0x1ae, 1),
            (0x1a4, 1),
        ] {
            let mut bad = b.clone();
            bad[offset..offset + 2].copy_from_slice(&value.to_le_bytes());
            if offset == 0x1ac && value == 39 {
                bad[0x1c7] = b'X';
            }
            assert!(
                add_helper_import(&bad, "probe.dll", "probe").is_err(),
                "malformed offset {offset:x}"
            );
        }
        let mut empty = fixture();
        put(&mut empty, 0x150, 0x1a0);
        put(&mut empty, 0x154, 8);
        assert!(add_helper_import(&empty, "probe.dll", "probe").is_err());
        let mut bad = b.clone();
        put(&mut bad, 0x154, 0x100);
        assert!(add_helper_import(&bad, "probe.dll", "probe").is_err());
        let mut bad = b.clone();
        put(&mut bad, 0xf8, 0x1a0);
        put(&mut bad, 0xfc, 8);
        assert!(add_helper_import(&bad, "probe.dll", "probe").is_err());
        let mut bad = b.clone();
        put(&mut bad, 0x8c, 0x1a0);
        assert!(add_helper_import(&bad, "probe.dll", "probe").is_err());
        let mut bad = b.clone();
        put(&mut bad, 0x1b0, 1);
        assert!(add_helper_import(&bad, "probe.dll", "probe").is_err());
    }
    #[test]
    fn actual_cl_bound_header_relocation_preserves_existing_image_bytes() {
        let repo = std::env::var_os("RECON_TOOL_TEST_REPO")
            .map(std::path::PathBuf::from)
            .unwrap_or_else(|| std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../.."));
        let before = std::fs::read(repo.join("toolchains/5.10/bin/CL.EXE")).unwrap();
        let after = add_helper_import(
            &before,
            "recon-direct-probe.dll",
            "recon_original_completion_trap",
        )
        .unwrap();
        let pe = dword(&before, 0x3c).unwrap() as usize;
        let optional = pe + 24;
        let table = optional + word(&before, pe + 20).unwrap() as usize;
        let count = word(&before, pe + 6).unwrap() as usize;
        let header = table + count * 40;
        let bound = dword(&before, optional + 184).unwrap() as usize;
        let size = dword(&before, optional + 188).unwrap() as usize;
        assert_eq!((bound, size), (0x240, 88));
        assert_eq!(&after[table..header], &before[table..header]);
        let allowed = [
            (pe + 6, pe + 8),
            (optional + 8, optional + 12),
            (optional + 56, optional + 60),
            (optional + 64, optional + 68),
            (optional + 104, optional + 112),
            (optional + 184, optional + 188),
            (header, header + 40),
        ];
        for (i, &byte) in before.iter().enumerate() {
            if !allowed.iter().any(|&(start, end)| start <= i && i < end) {
                assert_eq!(after[i], byte, "changed original byte {i:x}");
            }
        }
        let image = crate::original_image::Image::parse(&after).unwrap();
        let moved = dword(&after, optional + 184).unwrap();
        let offset = image.file_offset(moved, size).unwrap();
        assert_eq!(&after[offset..offset + size], &before[bound..bound + size]);
        let original_image = crate::original_image::Image::parse(&before).unwrap();
        let mut imports = crate::original_loader::imports(&original_image, &before).unwrap();
        imports.push("recon-direct-probe.dll".into());
        assert_eq!(
            crate::original_loader::imports(&image, &after).unwrap(),
            imports
        );
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
