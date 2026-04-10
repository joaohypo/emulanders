use crate::area;
use crate::fsext;
use crate::rc;

use super::{bin, compat, fmt};
use alloc::string::ToString;
use nx::fs;
use nx::ipc::sf::ncm;
use nx::result::*;
use alloc::string::String;

// Virtual amiibo format used in emuiibo v0.1
// It's not an actual virtual amiibo format, raw bin dumps were directly read/used

#[derive(Debug)]
pub struct VirtualAmiibo {
    raw_bin_path: String,
    raw_bin: bin::RawFormat
}

impl super::VirtualAmiiboFormat for VirtualAmiibo {
    fn try_load(path: String) -> Result<Self> {
        // Avoid loading any file as valid
        result_return_unless!(path.ends_with(".bin"), rc::ResultInvalidDeprecatedVirtualAmiibo);

        let mut file = fs::open_file(path.as_str(), fs::FileOpenOption::Read())?;
        let raw_bin: bin::RawFormat = file.read_val()?;

        // Verification
        result_return_unless!(raw_bin.st_2.unk_0xa5 == 0xA5, rc::ResultInvalidDeprecatedVirtualAmiibo);

        Ok(Self { raw_bin_path: path, raw_bin })
    }
}

impl compat::DeprecatedVirtualAmiiboFormat for VirtualAmiibo {
    fn convert(&self, key_set: Option<bin::RetailKeySet>) -> Result<fmt::VirtualAmiibo> {
        // Convert (and decrypt if possible) raw format
        let conv_bin = bin::ConvertedFormat::from_raw(&self.raw_bin);
        
        let plain_bin = match key_set {
            Some(key_set_v) => bin::PlainFormat::decrypt_from_converted(&conv_bin, &key_set_v)?,
            None => bin::PlainFormat::from_converted(&conv_bin)
        };

        // Save converted amiibo
        let name = fsext::get_path_file_name_without_extension(self.raw_bin_path.clone());
        let path = Self::find_convert_path(String::new(), name);
        fs::create_directory(path.as_str())?;

        let mii_charinfo_name = "mii-charinfo.bin".to_string();
        let mut amiibo = plain_bin.to_virtual_amiibo(path.clone(), mii_charinfo_name)?;

        // Save application area if present
        if plain_bin.dec_data.settings.flags.contains(bin::Flags::ApplicationAreaUsed()) {
            let access_id = plain_bin.dec_data.settings.access_id_be.swap_bytes();
            let program_id = ncm::ProgramId(plain_bin.dec_data.settings.program_id_be.swap_bytes());
            let bin_area = area::ApplicationArea::from(&amiibo, access_id);
            bin_area.create(plain_bin.dec_data.app_area.as_ptr(), plain_bin.dec_data.app_area.len(), false)?;

            amiibo.ensure_area_registered(access_id, program_id);
        }

        amiibo.save()?;

        // Save deprecated amiibo inside /v1 dir
        let deprecated_path = format!("{}/v1", path);
        fs::create_directory(deprecated_path.as_str())?;

        let new_raw_bin_path = format!("{}/raw-format.bin", deprecated_path);
        fs::rename_file(self.raw_bin_path.as_str(), new_raw_bin_path.as_str())?;

        {
            let conv_bin_path = format!("{}/converted-format.bin", deprecated_path);
            let mut conv_bin_file = fs::open_file(conv_bin_path.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
            conv_bin_file.write_val::<_, true>(&conv_bin)?;
        }
        {
            let plain_bin_path = format!("{}/plain-format.bin", deprecated_path);
            let mut plain_bin_file = fs::open_file(plain_bin_path.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
            plain_bin_file.write_val::<_, true>(&plain_bin)?;
        }

        Ok(amiibo)
    }
}