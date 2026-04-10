use alloc::{string::{String, ToString}, vec::Vec};
use serde::{Serialize, Deserialize};
use nx::{fs, ipc::sf::ncm, result::*, service::mii};
use crate::{area, fsext, miiext};
use super::{bin, compat, fmt};

// Virtual amiibo format used in emuiibo v0.2 and v0.2.1
// Consists on the following:
/*
- mii.dat file (with mii charinfo, generated on first boot)
- amiibo.bin raw dump (amiibo id, uuid, etc. were grabbed from here when needed)
- amiibo.json --> example:
{
    "name": "MyCoolAmiibo",
    "firstWriteDate": [ 2020, 12, 12 ],
    "lastWriteDate": [ 2020, 12, 12 ],
    "applicationAreaSize": 216,
    "randomizeUuid": true
}
*/

#[derive(Serialize, Deserialize, Clone, Debug)]
#[allow(non_snake_case)]
pub struct VirtualAmiiboInfo {
    name: String,
    firstWriteDate: Vec<u32>,
    lastWriteDate: Vec<u32>,
    applicationAreaSize: u32,
    randomizeUuid: bool
}

fn convert_date(json_date: &Vec<u32>) -> fmt::VirtualAmiiboDate {
    fmt::VirtualAmiiboDate {
        y: json_date[0] as u16,
        m: json_date[1] as u8,
        d: json_date[2] as u8
    }
}

#[derive(Clone, Debug)]
pub struct VirtualAmiibo {
    path: String,
    info: VirtualAmiiboInfo,
    mii_charinfo: mii::CharInfo,
    raw_bin: bin::RawFormat
}

impl super::VirtualAmiiboFormat for VirtualAmiibo {
    fn try_load(path: String) -> Result<Self> {
        let raw_bin: bin::RawFormat = {
            let raw_bin_path = format!("{}/amiibo.bin", path);
            let mut file = fs::open_file(raw_bin_path.as_str(), fs::FileOpenOption::Read())?;
            file.read_val()?
        };

        let amiibo_json_path = format!("{}/amiibo.json", path);
        let amiibo_json = read_deserialize_json!(amiibo_json_path.as_str() => VirtualAmiiboInfo)?;

        let mii_charinfo_path = format!("{}/mii.dat", path);
        // If newly generated, charinfo may not exist yet
        let mii_charinfo = match fs::get_entry_type(mii_charinfo_path.as_str()).is_err() {
            true => {
                let mii_charinfo = miiext::generate_random_mii()?;
                let mut file = fs::open_file(mii_charinfo_path.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
                file.write_val::<_, true>(&mii_charinfo)?;
                mii_charinfo
            },
            false => {
                let mut file = fs::open_file(mii_charinfo_path.as_str(), fs::FileOpenOption::Read())?;
                file.read_val()?
            }
        };
        Ok(Self { path, info: amiibo_json, mii_charinfo, raw_bin })
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
        let name = fsext::get_path_file_name(self.path.as_str());
        let path = Self::find_convert_path(self.path.clone(), name);
        let _ = fs::create_directory(path.as_str());

        let old_areas_path = format!("{}/areas", self.path);
        let new_areas_path = format!("{}/areas", path);
        let _ = fs::rename_directory(old_areas_path.as_str(), new_areas_path.as_str())?;

        let mii_charinfo_name = "mii-charinfo.bin".to_string();
        let mut amiibo = plain_bin.to_virtual_amiibo(path.clone(), mii_charinfo_name)?;

        // Prefer existing mii/app-area over raw bin mii/app-area
        amiibo.mii_charinfo = self.mii_charinfo;
        amiibo.info.name = self.info.name.clone();
        amiibo.info.use_random_uuid = self.info.randomizeUuid;
        amiibo.info.uuid = {
            let mut uuid = [0u8;10];
            super::generate_random_uuid(&mut uuid)?;
            uuid.to_vec()
        };
        amiibo.info.first_write_date = convert_date(&self.info.firstWriteDate);
        amiibo.info.last_write_date = convert_date(&self.info.lastWriteDate);

        let existing_access_id = fmt::generate_areas_json(path.clone())?;
        if let Some(existing_id) = existing_access_id {
            amiibo.ensure_area_registered(existing_id, fmt::DEFAULY_EMPTY_AREA_PROGRAM_ID);
        }

        // Save application area if present
        if plain_bin.dec_data.settings.flags.contains(bin::Flags::ApplicationAreaUsed()) {
            let access_id = plain_bin.dec_data.settings.access_id_be.swap_bytes();
            let program_id = ncm::ProgramId(plain_bin.dec_data.settings.program_id_be.swap_bytes());
            let existing_id = existing_access_id.unwrap_or(0);

            if existing_access_id.is_none() || (existing_id != access_id) {
                let bin_area = area::ApplicationArea::from(&amiibo, access_id);
                // TSAFETY: This is fine as we're writing from a valid memory range
                bin_area.create(plain_bin.dec_data.app_area.as_ptr(), plain_bin.dec_data.app_area.len(), false)?;

                amiibo.ensure_area_registered(access_id, program_id);
            }
        }

        amiibo.save()?;

        // Save deprecated amiibo inside /v2 dir
        let deprecated_path = format!("{}/v2", path);
        fs::create_directory(deprecated_path.as_str())?;

        let old_raw_bin_path = format!("{}/amiibo.bin", self.path);
        let new_raw_bin_path = format!("{}/amiibo.bin", deprecated_path);
        fs::rename_file(old_raw_bin_path.as_str(), new_raw_bin_path.as_str())?;

        {
            let conv_bin_path = format!("{}/amiibo-converted.bin", deprecated_path);
            let mut conv_bin_file = fs::open_file(conv_bin_path.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
            conv_bin_file.write_val::<_, true>(&conv_bin)?;
        }
        {
            let plain_bin_path = format!("{}/amiibo-plain.bin", deprecated_path);
            let mut plain_bin_file = fs::open_file(plain_bin_path.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
            plain_bin_file.write_val::<_, true>(&plain_bin)?;
        }

        let old_mii_charinfo_path = format!("{}/mii.dat", self.path);
        let new_mii_charinfo_path = format!("{}/mii.dat", deprecated_path);
        fs::rename_file(old_mii_charinfo_path.as_str(), new_mii_charinfo_path.as_str())?;

        let old_amiibo_json_path = format!("{}/amiibo.json", self.path);
        let new_amiibo_json_path = format!("{}/amiibo.json", deprecated_path);
        fs::rename_file(old_amiibo_json_path.as_str(), new_amiibo_json_path.as_str())?;

        if self.path != path {
            fs::remove_dir_all(self.path.as_str())?;
        }

        Ok(amiibo)
    }
}