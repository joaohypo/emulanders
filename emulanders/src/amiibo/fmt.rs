use nx::ipc::sf::ncm;
use nx::result::*;
use serde::{Serialize, Deserialize};
use alloc::string::String;
use alloc::vec::Vec;
use nx::fs;
use nx::util;
use nx::ipc::sf::mii;
use nx::ipc::sf::nfp;
use crate::{rc, area, fsext, miiext};

// Current virtual amiibo format, used since emuiibo v0.5 (with slight modifications)

#[derive(nx::ipc::sf::Request, nx::ipc::sf::Response, Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct VirtualAmiiboUuidInfo {
    pub use_random_uuid: bool,
    pub uuid: [u8; 10]
}

#[derive(nx::ipc::sf::Request, nx::ipc::sf::Response, Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct VirtualAmiiboData {
    pub uuid_info: VirtualAmiiboUuidInfo,
    pub name: util::ArrayString<41>,
    pub first_write_date: nfp::Date,
    pub last_write_date: nfp::Date,
    pub mii_charinfo: mii::CharInfo
}

// Note: actual amiibo ID in amiibos (nfp services have a different ID type)

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct VirtualAmiiboId {
    pub game_character_id: u16,
    pub character_variant: u8,
    pub figure_type: u8,
    pub model_number: u16,
    pub series: u8
}

impl VirtualAmiiboId {
    pub const fn empty() -> Self {
        Self { character_variant: 0, figure_type: 0, game_character_id: 0, model_number: 0, series: 0 }
    }
}

#[derive(Serialize, Deserialize, Copy, Clone, Debug)]
pub struct VirtualAmiiboDate {
    pub y: u16,
    pub m: u8,
    pub d: u8
}

impl VirtualAmiiboDate {
    pub const fn from_date(date: nfp::Date) -> Self {
        Self { y: date.year, m: date.month, d: date.day }
    }

    pub const fn to_date(&self) -> nfp::Date {
        nfp::Date { year: self.y, month: self.m, day: self.d }
    }
}

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct VirtualAmiiboInfo {
    pub first_write_date: VirtualAmiiboDate,
    pub id: VirtualAmiiboId,
    pub last_write_date: VirtualAmiiboDate,
    pub mii_charinfo_file: String,
    pub name: String,
    pub uuid: Vec<u8>,
    pub use_random_uuid: bool,
    pub version: u8,
    pub write_counter: u16
}

// Note: temporary fix

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct VirtualAmiiboInfoOptional {
    pub first_write_date: VirtualAmiiboDate,
    pub id: VirtualAmiiboId,
    pub last_write_date: VirtualAmiiboDate,
    pub mii_charinfo_file: String,
    pub name: String,
    pub uuid: Option<Vec<u8>>,
    pub use_random_uuid: Option<bool>,
    pub version: u8,
    pub write_counter: u16
}

impl VirtualAmiiboInfoOptional {
    pub fn convert_to_info(self) -> VirtualAmiiboInfo {
        VirtualAmiiboInfo {
            first_write_date: self.first_write_date,
            id: self.id,
            last_write_date: self.last_write_date,
            mii_charinfo_file: self.mii_charinfo_file,
            name: self.name,
            uuid: self.uuid.unwrap(),
            use_random_uuid: self.use_random_uuid.unwrap(),
            version: self.version,
            write_counter: self.write_counter
        }
    }
}

#[derive(nx::ipc::sf::Request, nx::ipc::sf::Response, Copy, Clone, Serialize, Deserialize, Debug)]
#[repr(C)]
pub struct VirtualAmiiboAreaEntry {
    pub program_id: u64,
    pub access_id: nfp::AccessId
}

// Retail Interactive Display Menu (quite a symbolic ID)
pub const DEFAULY_EMPTY_AREA_PROGRAM_ID: ncm::ProgramId = ncm::ProgramId(0x0100069000078000);

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct VirtualAmiiboAreaInfo {
    pub areas: Vec<VirtualAmiiboAreaEntry>,
    pub current_area_access_id: nfp::AccessId
}

impl VirtualAmiiboAreaInfo {
    pub const fn empty() -> Self {
        Self {
            areas: Vec::new(),
            current_area_access_id: 0
        }
    }
}

pub fn generate_areas_json(path: String) -> Result<Option<nfp::AccessId>> {
    let mut access_ids: Vec<nfp::AccessId> = Vec::new();

    let areas_dir = format!("{}/areas", path);
    log!("Analyzing areas dir: {}\n", areas_dir);
    if let Ok(mut dir) = fs::open_directory(areas_dir.as_str(), fs::DirectoryOpenMode::ReadFiles()) {
        loop {
            if let Ok(Some(entry)) = dir.read_next() {
                if let Ok(name) = entry.name.get_str() {
                    // 0x<hex-access-id>.bin
                    if name.ends_with(".bin") && name.starts_with("0x") && (name.len() == 2 + 8 + 4) {
                        let hex_access_id_str = &name[2..2 + 8];
                        if let Ok(access_id) = u32::from_str_radix(hex_access_id_str, 16) {
                            access_ids.push(access_id);
                        }
                    }
                }
            }
            else {
                break;
            }
        }
    }

    let mut areas = VirtualAmiiboAreaInfo::empty();
    for access_id in &access_ids {
        let area_entry = VirtualAmiiboAreaEntry {
            program_id: DEFAULY_EMPTY_AREA_PROGRAM_ID.0,
            access_id: *access_id
        };
        areas.areas.push(area_entry);
    }
    let access_id = access_ids.get(0).map(|id_ref| *id_ref);
    if let Some(id) = access_id {
        areas.current_area_access_id = id;
    }

    let areas_json_path= format!("{}/areas.json", path);
    write_serialize_json!(areas_json_path.as_str(), &areas)?;

    Ok(access_id)
}

#[derive(Clone, Debug)]
pub struct VirtualAmiibo {
    pub info: VirtualAmiiboInfo,
    pub mii_charinfo: mii::CharInfo,
    pub areas: VirtualAmiiboAreaInfo,
    pub path: String
}

impl VirtualAmiibo {
    pub fn new(info: VirtualAmiiboInfo, areas: VirtualAmiiboAreaInfo, path: String) -> Result<Self> {
        let mut amiibo = Self {
            info: info,
            mii_charinfo: Default::default(),
            areas: areas,
            path: path
        };
        amiibo.mii_charinfo = amiibo.load_mii_charinfo()?;
        Ok(amiibo)
    }

    pub fn is_valid(&self) -> bool {
        !self.path.is_empty()
    }

    pub fn load_mii_charinfo(&self) -> Result<mii::CharInfo> {
        let mii_charinfo_path = format!("{}/{}", self.path, self.info.mii_charinfo_file);
        if fsext::exists_file(mii_charinfo_path.clone()) {
            let mut mii_charinfo_file = fs::open_file(mii_charinfo_path.as_str(), fs::FileOpenOption::Read())?;
            Ok(mii_charinfo_file.read_val()?)
        }
        else {
            let random_mii = miiext::generate_random_mii()?;
            let mut mii_charinfo_file = fs::open_file(mii_charinfo_path.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
            mii_charinfo_file.write_val::<_, true>(&random_mii)?;
            Ok(random_mii)
        }
    }

    #[inline]
    pub fn has_any_application_areas(&self) -> bool {
        !self.areas.areas.is_empty()
    }

    pub fn has_application_area(&self, access_id: nfp::AccessId) -> bool {
        for area_entry in &self.areas.areas {
            if area_entry.access_id == access_id {
                return true;
            }
        }

        false
    }

    #[inline]
    pub fn register_area(&mut self, access_id: nfp::AccessId, program_id: ncm::ProgramId) -> bool {
        if self.has_application_area(access_id) {
            false
        }
        else {
            self.areas.areas.push(VirtualAmiiboAreaEntry { program_id: program_id.0, access_id });
            true
        }
    }

    #[inline]
    pub fn ensure_area_registered(&mut self, access_id: nfp::AccessId, program_id: ncm::ProgramId) {
        self.register_area(access_id, program_id);
    }

    pub fn set_current_area(&mut self, access_id: nfp::AccessId) -> bool {
        self.ensure_area_registered(access_id, DEFAULY_EMPTY_AREA_PROGRAM_ID);

        for area_entry in &self.areas.areas {
            if area_entry.access_id == access_id {
                self.areas.current_area_access_id = access_id;
                return true;
            }
        }

        false
    }

    pub fn get_current_area(&self) -> Option<VirtualAmiiboAreaEntry> {
        for area_entry in &self.areas.areas {
            if area_entry.access_id == self.areas.current_area_access_id {
                return Some(*area_entry);
            }
        }

        None
    }

    pub fn delete_current_area(&mut self) -> Result<()> {
        self.areas.areas.retain(|&area_entry| area_entry.access_id != self.areas.current_area_access_id);
        area::ApplicationArea::from(self, self.areas.current_area_access_id).delete()?;

        self.areas.current_area_access_id = match self.areas.areas.is_empty() {
            true => 0,
            false => self.areas.areas[0].access_id
        };

        self.notify_written()
    }

    pub fn delete_all_areas(&mut self) -> Result<()> {
        for area_entry in &self.areas.areas {
            area::ApplicationArea::from(self, area_entry.access_id).delete()?;
        }
        self.areas.areas.clear();
        self.areas.current_area_access_id = 0;

        self.notify_written()
    }

    pub fn update_area_program_id(&mut self, access_id: nfp::AccessId, program_id: ncm::ProgramId) -> Result<()> {
        self.ensure_area_registered(access_id, DEFAULY_EMPTY_AREA_PROGRAM_ID);
        
        for area_entry in &mut self.areas.areas {
            if area_entry.access_id == access_id {
                area_entry.program_id = program_id.0;
                break;
            }
        }

        self.save()
    }

    pub fn set_uuid_info(&mut self, uuid_info: VirtualAmiiboUuidInfo) -> Result<()> {
        self.info.uuid = Vec::from(uuid_info.uuid);
        self.info.use_random_uuid = uuid_info.use_random_uuid;

        self.save()
    }

    pub fn produce_data(&self) -> Result<VirtualAmiiboData> {
        let mut data: VirtualAmiiboData = Default::default();

        for i in 0..data.uuid_info.uuid.len() {
            data.uuid_info.uuid[i] = self.info.uuid[i];
        }
        data.uuid_info.use_random_uuid = self.info.use_random_uuid;
        data.name.set_str(self.info.name.as_str())?;
        data.first_write_date = self.info.first_write_date.to_date();
        data.last_write_date = self.info.last_write_date.to_date();
        data.mii_charinfo = self.mii_charinfo;
        Ok(data)
    }

    pub fn produce_tag_info(&self) -> Result<nfp::TagInfo> {
        let mut tag_info = nfp::TagInfo {
            uid: nfp::TagId {
                uuid: [0; 0xA],
                uuid_length: 0,
                reserved_1: [0; 0x15],
            },
            protocol: u32::MAX,
            tag_type: u32::MAX,
            reserved_2: [0; 0x30]
        };
        
        if self.info.use_random_uuid {
            super::generate_random_uuid(&mut tag_info.uid.uuid)?;
            tag_info.uid.uuid_length = tag_info.uid.uuid.len() as u8;
        }
        else {
            unsafe {
                let uuid_len = self.info.uuid.len().min(tag_info.uid.uuid.len());
                tag_info.uid.uuid_length = uuid_len as u8;
                core::ptr::copy(self.info.uuid.as_ptr(), tag_info.uid.uuid.as_mut_ptr(), uuid_len);
            }
        }

        Ok(tag_info)
    }

    pub fn produce_register_info(&self) -> Result<nfp::RegisterInfo> {
        Ok(nfp::RegisterInfo {
            mii_charinfo: self.mii_charinfo,
            first_write_date: self.info.first_write_date.to_date(),
            name: util::ArrayString::from_str(&self.info.name[0..self.info.name.len().min(10)]),
            font_region: 0,
            reserved: [0; 0x7A]
        })
    }

    pub fn produce_common_info(&self) -> Result<nfp::CommonInfo> {
        Ok(nfp::CommonInfo {
            last_write_date: self.info.last_write_date.to_date(),
            write_counter: self.info.write_counter,
            version: self.info.version,
            pad: 0,
            application_area_size: 0xD8, // This value is also hardcoded in official nfp commands
            reserved: [0; 0x34]
        })
    }

    pub fn produce_model_info(&self) -> Result<nfp::ModelInfo> {
        // Dummy code until a proper function is implemented

        #[derive(Copy, Clone)]
        struct A {
            game_character_id: u16,
            character_variant: u8
        }

        #[derive(Copy, Clone)]
        union CharacterId {
            a: A,
            character_id: [u8; 3]
        }

        let character_id = CharacterId { a: A { game_character_id: self.info.id.game_character_id, character_variant: self.info.id.character_variant } };

        // TODO: adapt and do what nfc actually does
        Ok(nfp::ModelInfo {
            character_id: unsafe { character_id.character_id },
            series_id: self.info.id.series,
            numbering_id: self.info.id.model_number, // Note: we should technically reverse it since nfp wants it reversed... but it only works this way?
            nfp_type: self.info.id.figure_type,
            reserved: [0; 0x39]
        })
    }

    pub fn produce_register_info_private(&self) -> Result<nfp::RegisterInfoPrivate> {
        Ok(nfp::RegisterInfoPrivate {
            mii_store_data: mii::StoreData::from_charinfo(self.mii_charinfo)?,
            first_write_date: self.info.first_write_date.to_date(),
            name: util::ArrayString::from_str(&self.info.name.clone()[0..self.info.name.len().min(10)]),
            font_region: 0,
            reserved: [0; 0x8E]
        })
    }

    pub fn produce_admin_info(&self) -> Result<nfp::AdminInfo> {
        let cur_area = self.get_current_area();

        let program_id = match cur_area {
            Some(ref area_entry) => ncm::ProgramId(area_entry.program_id),
            None => DEFAULY_EMPTY_AREA_PROGRAM_ID
        };
        let app_area_version = {
            // TODO: change this and do what nfc actually does
            // 0x0100 for Switch, 0x0004 for 3DS, 0x0005 for Wii U
            match program_id.0 >> 48 {
                0x0100 => nfp::ApplicationAreaVersion::NintendoSwitch,
                0x0004 => nfp::ApplicationAreaVersion::Nintendo3DS,
                0x0005 => nfp::ApplicationAreaVersion::NintendoWiiU,
                _ => nfp::ApplicationAreaVersion::Default
            }
        };

        Ok(nfp::AdminInfo {
            app_id: program_id,
            access_id: match cur_area {
                Some(ref area_entry) => area_entry.access_id,
                None => 0
            },
            terminal_id_crc32_change_counter: 0, // TODO: just stub this?
            flags: match cur_area {
                Some(_) => nfp::AdminInfoFlags::IsInitialized() | nfp::AdminInfoFlags::HasApplicationArea(),
                None => nfp::AdminInfoFlags::IsInitialized()
            },
            unk: 0x2,
            app_area_version,
            pad: [0; 0x7],
            reserved: [0; 0x28]
        })
    }

    pub fn update_from_register_info_private(&mut self, register_info_private: &nfp::RegisterInfoPrivate) -> Result<()> {
        self.mii_charinfo = register_info_private.mii_store_data.to_charinfo()?;
        self.info.first_write_date = VirtualAmiiboDate::from_date(register_info_private.first_write_date);
        self.info.name = register_info_private.name.get_string()?;

        self.notify_written()
    }

    pub fn save(&self) -> Result<()> {
        let amiibo_json_path = format!("{}/amiibo.json", self.path);
        write_serialize_json!(amiibo_json_path.as_str(), &self.info)?;

        let amiibo_flag_path = format!("{}/amiibo.flag", self.path);
        let _ = fs::create_file(amiibo_flag_path.as_str(), 0, fs::FileAttribute::None());

        let mii_charinfo_path = format!("{}/{}", self.path, self.info.mii_charinfo_file);
        let _ = fs::remove_file(mii_charinfo_path.as_str());
        let mut mii_charinfo_file = fs::open_file(mii_charinfo_path.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
        mii_charinfo_file.write_val::<_, true>(&self.mii_charinfo)?;

        let areas_json_path = format!("{}/areas.json", self.path);
        write_serialize_json!(areas_json_path.as_str(), &self.areas)?;

        Ok(())
    }

    pub fn notify_written(&mut self) -> Result<()> {
        if self.info.write_counter < 0xFFFF {
            self.info.write_counter += 1;
        }
        self.save()
    }
}

impl super::VirtualAmiiboFormat for VirtualAmiibo {
    fn try_load(path: String) -> Result<Self>  {
        let amiibo_flag_path = format!("{}/amiibo.flag", path);
        result_return_unless!(fsext::exists_file(amiibo_flag_path), rc::ResultVirtualAmiiboFlagNotFound);

        let amiibo_json_path = format!("{}/amiibo.json", path);
        result_return_unless!(fsext::exists_file(amiibo_json_path.clone()), rc::ResultVirtualAmiiboJsonNotFound);

        let areas_json_path = format!("{}/areas.json", path);

        if !fsext::exists_file(areas_json_path.clone()) {
            generate_areas_json(path.clone())?;
        }
        result_return_unless!(fsext::exists_file(areas_json_path.clone()), rc::ResultVirtualAmiiboAreasJsonNotFound);

        let mut needs_save = false;

        let mut amiibo_json_opt = read_deserialize_json!(amiibo_json_path.as_str() => VirtualAmiiboInfoOptional)?;
        // Fix for those which lack uuids
        if amiibo_json_opt.uuid.is_none() {
            let mut uuid = [0u8; 10];
            super::generate_random_uuid(&mut uuid)?;
            amiibo_json_opt.uuid = Some(uuid.to_vec());
            amiibo_json_opt.use_random_uuid = Some(true);
            needs_save = true;
        }
        if amiibo_json_opt.use_random_uuid.is_none() {
            amiibo_json_opt.use_random_uuid = Some(false);
            needs_save = true;
        }

        let areas_json = read_deserialize_json!(areas_json_path.as_str() => VirtualAmiiboAreaInfo)?;
        for entry in areas_json.areas.iter() {
            area::push_access_id_cache(entry.program_id, entry.access_id)?;
        }

        let amiibo = VirtualAmiibo::new(amiibo_json_opt.convert_to_info(), areas_json, path.clone())?;
        if needs_save {
            amiibo.save()?;
        }
        Ok(amiibo)
    }
}