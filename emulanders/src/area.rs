use nx::result::*;
use nx::fs;
use nx::ipc::sf::nfp;
use alloc::string::String;
use alloc::vec::Vec;
use serde::{Serialize, Deserialize};
use crate::{amiibo, fsext};

pub struct ApplicationArea {
    area_file: String
}

impl ApplicationArea {
    pub fn new() -> Self {
        Self { area_file: String::new() }
    }

    pub fn from_id(virtual_amiibo: &amiibo::fmt::VirtualAmiibo, program_id: u64, access_id: nfp::AccessId) -> Self {
        log!("Saving {:#X} -> {:#X} cache!\n", program_id, access_id);
        let rc = push_access_id_cache(program_id, access_id);
        log!("Cache result: {:?}\n", rc);
        Self::from(virtual_amiibo, access_id)
    }
    
    pub fn from(virtual_amiibo: &amiibo::fmt::VirtualAmiibo, access_id: nfp::AccessId) -> Self {
        let areas_dir = format!("{}/areas", virtual_amiibo.path);
        let _ = fs::create_directory(areas_dir.as_str());
        Self { area_file: format!("{}/0x{:08X}.bin", areas_dir, access_id) }
    }

    pub fn delete(&mut self) -> Result<()> {
        if self.is_valid() {
            fs::remove_file(self.area_file.as_str())?;
            self.area_file.clear();
        }

        Ok(())
    }

    pub fn is_valid(&self) -> bool {
        !self.area_file.is_empty()
    }

    pub fn exists(&self) -> bool {
        if self.is_valid() {
            fsext::exists_file(self.area_file.clone())
        }
        else {
            false
        }
    }

    pub fn create(&self, data: *const u8, data_size: usize, _recreate: bool) -> Result<()> {
        // TODO: difference between create and recreate commands?
        // write already overwrites the area file
        self.write(data, data_size)
    }

    /// SAFETY: `data` must be a valid, non-null pointer
    pub fn write(&self, data: *const u8, data_size: usize) -> Result<()> {
        let _ = fs::remove_file(self.area_file.as_str());
        let mut file = fs::open_file(self.area_file.as_str(), fs::FileOpenOption::Create() | fs::FileOpenOption::Write() | fs::FileOpenOption::Append())?;
        file.write_array::<_, true>(unsafe { core::slice::from_raw_parts(data,data_size) })?;
        Ok(())
    }

    /// SAFETY: `data` must be a valid, non-null pointer
    pub unsafe fn read(&self, data: *mut u8, data_size: usize) -> Result<()> {
        let mut file = fs::open_file(self.area_file.as_str(), fs::FileOpenOption::Read())?;
        file.read_array(unsafe { core::slice::from_raw_parts_mut(data, data_size) })?;
        Ok(())
    }

    pub fn get_size(&self) -> Result<usize> {
        let mut file = fs::open_file(self.area_file.as_str(), fs::FileOpenOption::Read())?;
        file.get_size()
    }
}

pub const ACCESS_ID_CACHE_PATH: &'static str = "sdmc:/emuiibo/access_id_cache.json";

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct AccessIdCache {
    pub cache: Vec<amiibo::fmt::VirtualAmiiboAreaEntry>
}

impl AccessIdCache {
    pub const fn new() -> Self {
        Self {
            cache: Vec::new()
        }
    }
}

#[inline(always)]
pub(crate) fn read_access_id_cache() -> Result<AccessIdCache> {
    read_deserialize_json!(ACCESS_ID_CACHE_PATH => AccessIdCache)
}

pub fn push_access_id_cache(program_id: u64, access_id: nfp::AccessId) -> Result<()> {
    let mut cache_json = read_access_id_cache().unwrap_or(AccessIdCache::new());

    let mut found = false;
    for entry in cache_json.cache.iter_mut() {
        if entry.program_id == program_id {
            entry.access_id = access_id;
            found = true;
            break;
        }
    }

    if !found {
        cache_json.cache.push(amiibo::fmt::VirtualAmiiboAreaEntry { program_id, access_id });
    }
    write_serialize_json!(ACCESS_ID_CACHE_PATH, &cache_json)?;
    Ok(())
}