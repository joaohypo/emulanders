use super::{bin, compat, fmt};
use crate::{fsext, miiext};
use alloc::{
    string::{String, ToString},
    vec::Vec,
};
use nx::{fs, result::*, service::mii};
use serde::{Deserialize, Serialize};

// Virtual amiibo format used in emuiibo v0.3, v0.3.1 and v0.4
// Consists on the following:
/*
- mii charinfo file (name specified below, generated on first boot)

- common.json --> example:
{
    "writeCounter": 1234,
    "lastWriteDate": "2020-12-12",
    "version": 0
}

- tag.json --> example:
{
    "uuid": "001122334455AABBCCDD",
    "randomUuid": true
}

- model.json --> example:
{
    "amiiboId": "00224466CCDDEEFF"
}

- register.json --> example:
{
    "name": "MyCoolAmiibo",
    "miiCharInfo": "mii-charinfo.bin",
    "firstWriteDate": "2020-12-12"
}

*/

#[derive(Serialize, Deserialize, Clone, Debug)]
#[allow(non_snake_case)]
pub struct VirtualAmiiboCommonInfo {
    pub writeCounter: u16,
    pub lastWriteDate: String,
    pub version: u16,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
#[allow(non_snake_case)]
pub struct VirtualAmiiboTagInfo {
    pub uuid: Option<String>,
    pub randomUuid: bool,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
#[allow(non_snake_case)]
pub struct VirtualAmiiboModelInfo {
    pub amiiboId: String,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
#[allow(non_snake_case)]
pub struct VirtualAmiiboRegisterInfo {
    pub name: String,
    pub miiCharInfo: String,
    pub firstWriteDate: String,
}

// Why did I use this format back then... it's quite annoying to parse

fn convert_date(info_date: &String) -> fmt::VirtualAmiiboDate {
    let y_str = &info_date[0..4];
    let y = u16::from_str_radix(y_str, 10).unwrap();

    let m_str = &info_date[5..7];
    let m = u8::from_str_radix(m_str, 10).unwrap();

    let d_str = &info_date[8..10];
    let d = u8::from_str_radix(d_str, 10).unwrap();

    fmt::VirtualAmiiboDate { y, m, d }
}

#[inline]
fn convert_from_hex(hex: String) -> Vec<u8> {
    (0..hex.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&hex[i..i + 2], 16))
        .collect::<core::result::Result<Vec<u8>, _>>()
        .unwrap()
}

#[inline]
fn get_raw_hex_bytes<const S: usize>(hex: String) -> [u8; S] {
    let mut raw: [u8; S] = [0; S];
    let bytes = convert_from_hex(hex);

    for i in 0..S {
        raw[i] = bytes[i];
    }

    raw
}

fn convert_amiibo_id(info_id: String) -> fmt::VirtualAmiiboId {
    let raw_id = get_raw_hex_bytes::<8>(info_id);
    unsafe { core::mem::transmute(raw_id) }
}

fn convert_uuid(info_uuid: String) -> Vec<u8> {
    convert_from_hex(info_uuid)
}

#[derive(Clone, Debug)]
pub struct VirtualAmiibo {
    path: String,
    common_info: VirtualAmiiboCommonInfo,
    tag_info: VirtualAmiiboTagInfo,
    model_info: VirtualAmiiboModelInfo,
    register_info: VirtualAmiiboRegisterInfo,
    mii_charinfo: mii::CharInfo,
}

impl super::VirtualAmiiboFormat for VirtualAmiibo {
    fn try_load(path: String) -> Result<Self> {
        let common_json_path = format!("{}/common.json", path);
        let common_info = read_deserialize_json!(common_json_path.as_str() => VirtualAmiiboCommonInfo)?;

        let tag_json_path = format!("{}/tag.json", path);
        let tag_info = read_deserialize_json!(tag_json_path.as_str() => VirtualAmiiboTagInfo)?;

        let model_json_path = format!("{}/model.json", path);
        let model_info = read_deserialize_json!(model_json_path.as_str() => VirtualAmiiboModelInfo)?;

        let register_json_path = format!("{}/register.json", path);
        let register_info =
            read_deserialize_json!(register_json_path.as_str() => VirtualAmiiboRegisterInfo)?;

        let mii_charinfo_path = format!("{}/{}", path, register_info.miiCharInfo);
        // If newly generated, charinfo may not exist yet
        let mii_charinfo = match fs::get_entry_type(mii_charinfo_path.as_str()).is_err() {
            true => {
                let mii_charinfo = miiext::generate_random_mii()?;
                let mut file = fs::open_file(
                    mii_charinfo_path.as_str(),
                    fs::FileOpenOption::Create()
                        | fs::FileOpenOption::Write()
                        | fs::FileOpenOption::Append(),
                )?;
                file.write_val::<_, true>(&mii_charinfo)?;
                mii_charinfo
            }
            false => {
                let mut file = fs::open_file(mii_charinfo_path.as_str(), fs::FileOpenOption::Read())?;
                file.read_val()?
            }
        };

        Ok(Self {
            path,
            common_info,
            tag_info,
            model_info,
            register_info,
            mii_charinfo,
        })
    }
}

impl compat::DeprecatedVirtualAmiiboFormat for VirtualAmiibo {
    fn convert(&self, _key_set: Option<bin::RetailKeySet>) -> Result<fmt::VirtualAmiibo> {
        // Save converted amiibo
        let name = fsext::get_path_file_name(self.path.clone());
        let path = Self::find_convert_path(self.path.clone(), name);
        let _ = fs::create_directory(path.as_str());

        let old_areas_path = format!("{}/areas", self.path);
        let new_areas_path = format!("{}/areas", path);
        let _ = fs::rename_directory(old_areas_path.as_str(), new_areas_path.as_str())?;

        let uuid = match self
            .tag_info
            .uuid
            .as_ref()
            .map(|uuid| convert_uuid(uuid.clone()))
        {
            Some(uuid) => uuid,
            None => {
                let mut uuid = [0u8; 10];
                super::generate_random_uuid(&mut uuid)?;
                uuid.to_vec()
            }
        };
        let mut amiibo = fmt::VirtualAmiibo {
            info: fmt::VirtualAmiiboInfo {
                first_write_date: convert_date(&self.register_info.firstWriteDate),
                id: convert_amiibo_id(self.model_info.amiiboId.clone()),
                last_write_date: convert_date(&self.common_info.lastWriteDate),
                mii_charinfo_file: "mii-charinfo.bin".to_string(),
                name: self.register_info.name.clone(),
                uuid,
                use_random_uuid: false,
                version: self.common_info.version as u8,
                write_counter: self.common_info.writeCounter,
            },
            mii_charinfo: self.mii_charinfo,
            areas: fmt::VirtualAmiiboAreaInfo::empty(),
            path: path.clone(),
        };

        let existing_access_id = fmt::generate_areas_json(path.clone())?;
        if let Some(existing_id) = existing_access_id {
            amiibo.ensure_area_registered(existing_id, fmt::DEFAULY_EMPTY_AREA_PROGRAM_ID);
        }

        amiibo.save()?;

        // Save deprecated amiibo inside /v3 dir
        let deprecated_path = format!("{}/v3", path);
        fs::create_directory(deprecated_path.as_str())?;

        // put them all in scopes so we can keep the max number of in-flight strings at 3.
        {
            let old_common_json_path = format!("{}/common.json", self.path);
            let new_common_json_path = format!("{}/common.json", deprecated_path);
            fs::rename_file(old_common_json_path.as_str(), new_common_json_path.as_str())?;
        }
        {
            let old_tag_json_path = format!("{}/tag.json", self.path);
            let new_tag_json_path = format!("{}/tag.json", deprecated_path);
            fs::rename_file(old_tag_json_path.as_str(), new_tag_json_path.as_str())?;
        }
        {
            let old_model_json_path = format!("{}/model.json", self.path);
            let new_model_json_path = format!("{}/model.json", deprecated_path);
            fs::rename_file(old_model_json_path.as_str(), new_model_json_path.as_str())?;
        }
        {
            let old_register_json_path = format!("{}/register.json", self.path);
            let new_register_json_path = format!("{}/register.json", deprecated_path);
            fs::rename_file(
                old_register_json_path.as_str(),
                new_register_json_path.as_str(),
            )?;
        }
        {
            let old_mii_charinfo_path = format!("{}/{}", self.path, self.register_info.miiCharInfo);
            let new_mii_charinfo_path =
                format!("{}/{}", deprecated_path, self.register_info.miiCharInfo);
            fs::rename_file(
                old_mii_charinfo_path.as_str(),
                new_mii_charinfo_path.as_str(),
            )?;
        }
        if self.path != path {
            fs::remove_dir_all(self.path.as_str())?;
        }

        Ok(amiibo)
    }
}
