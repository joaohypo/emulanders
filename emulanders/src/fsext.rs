use alloc::string::ToString;
use nx::result::*;
use nx::fs;
use alloc::string::String;
use crate::{amiibo, miiext};

#[inline]
pub fn exists_file(path: impl AsRef<str>) -> bool {
    match fs::get_entry_type(path.as_ref()) {
        Ok(ent_type) => ent_type == fs::DirectoryEntryType::File,
        Err(_) => false
    }
}

pub const BASE_DIR: &'static str = "sdmc:/emulanders";

pub const FLAGS_DIR: &'static str = "sdmc:/emulanders/flags";
pub const SKYLANDER_DIR: &'static str = "sdmc:/emulanders/skylanders";

#[inline(always)]
pub fn make_flag_path(name: &str) -> String {
    format!("{}/{}.flag", FLAGS_DIR, name)
}

pub fn has_flag(name: &str) -> bool {
    exists_file(make_flag_path(name))
}

pub fn set_flag(name: &str, enabled: bool) {
    let flag_path = make_flag_path(name);
    if enabled {
        let _ = fs::create_file(flag_path.as_str(), 0, fs::FileAttribute::None());
    }
    else {
        let _ = fs::remove_file(flag_path.as_str());
    }
}

pub fn get_path_without_extension(path: impl AsRef<str>) -> String {
    let path = path.as_ref();
    match path.rfind('.') {
        Some(offset) => {
            path[..offset].to_string()
        },
        None => path.to_string()
    }
}

pub fn get_path_file_name(path: impl AsRef<str>) -> String {
    path.as_ref().split('/').last().unwrap_or("").to_string()
}

#[inline]
pub fn get_path_file_name_without_extension(path: String) -> String {
    get_path_file_name(get_path_without_extension(path))
}

#[inline]
pub fn recreate_directory(path: impl AsRef<str>) -> Result<()> {
    let path = path.as_ref();
    // The directory might not already exist, thus this attempt to delete it could fail
    if let Err(rc) = fs::remove_dir_all(path) {
        log!("Error removing directory {}: {:?}\n", path, rc);
    }
    fs::create_directory(path)?;
    Ok(())
}

// Note: using macros instead of fns to avoid having to deal with serde_json's lifetime stuff

macro_rules! read_deserialize_json {
    ($path:expr => $t:ty) => {{
        let json_data = {
            let mut file = nx::fs::open_file($path, nx::fs::FileOpenOption::Read())?;
            let mut data: alloc::vec::Vec<u8> = vec![0; file.get_size()?];
            file.read_array(&mut data)?;
            data
        };
    
        serde_json::from_slice::<$t>(&json_data).map_err(|_| $crate::rc::ResultInvalidJsonDeserialization::make())
    }};
}

macro_rules! write_serialize_json {
    ($path:expr, $t:expr) => {{
        if let Ok(json_data) = serde_json::to_vec_pretty($t) {
            let _ = nx::fs::remove_file($path);
            let mut file = nx::fs::open_file($path, nx::fs::FileOpenOption::Create() | nx::fs::FileOpenOption::Write() | nx::fs::FileOpenOption::Append())?;
            file.write_array::<_, true>(&json_data)?;
            Ok(())
        }
        else {
            Err($crate::rc::ResultInvalidJsonSerialization::make())
        }
    }};
}

pub fn ensure_directories() -> Result<()> {
    let _ = fs::create_directory(BASE_DIR);
    let _ = fs::create_directory(amiibo::VIRTUAL_AMIIBO_DIR);
    let _ = fs::create_directory(SKYLANDER_DIR);
    let _ = fs::create_directory(FLAGS_DIR);
    recreate_directory(miiext::EXPORTED_MIIS_DIR)?;

    Ok(())
}