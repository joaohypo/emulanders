use nx::{result::*, fs};
use crate::amiibo::bin;

use super::{v1, v2, v3, fmt, VirtualAmiiboFormat};
use alloc::string::String;

pub(crate) const RETAIL_KEY_SET_FILE: &str = "sdmc:/switch/key_retail.bin";

pub trait DeprecatedVirtualAmiiboFormat: super::VirtualAmiiboFormat {
    fn convert(&self, key_set: Option<bin::RetailKeySet>) -> Result<fmt::VirtualAmiibo>;

    fn find_convert_path(old_path: String, name: String) -> String {
        let mut path = format!("{}/{}", super::VIRTUAL_AMIIBO_DIR, name);

        if path == old_path {
            return path;
        }

        let mut name_idx: usize = 0;
        loop {
            // Path already exists
            if fs::get_entry_type(path.as_str()).is_ok() {
                name_idx += 1;
                path = format!("{}/{}_{}", super::VIRTUAL_AMIIBO_DIR, name, name_idx);
            }
            else {
                break;
            }
        }

        path
    }
}

fn convert_deprecated_virtual_amiibos_in_dir(path: &str) -> Result<()> {
    let mut key_set: Option<bin::RetailKeySet> = None;
    let key_set_file = fs::open_file(RETAIL_KEY_SET_FILE, fs::FileOpenOption::Read());
    if let Ok(mut key_set_file) = key_set_file {
        if let Ok(key_set_v) = key_set_file.read_val::<bin::RetailKeySet>() {
            log!("Found key_retail.bin --- old amiibo / raw dump conversions will include encrypted sections too!\n");
            key_set = Some(key_set_v);
        }
    }

    let mut dir = fs::open_directory(path, fs::DirectoryOpenMode::ReadDirectories() | fs::DirectoryOpenMode::ReadFiles())?;

    loop {
        if let Some(entry) = dir.read_next()? {
            let entry_path = format!("{}/{:?}", path, entry.name);
            log!("Analyzing entry '{}'...\n", entry_path);

            let maybe_new_amiibo = {
                if let Ok(v1_amiibo) = v1::VirtualAmiibo::try_load(entry_path.clone()) {
                    log!("Loaded v1 amiibo {:?} - converting it...\n", v1_amiibo);
                    Some(v1_amiibo.convert(key_set))
                }
                else if let Ok(v2_amiibo) = v2::VirtualAmiibo::try_load(entry_path.clone()) {
                    log!("Loaded v2 amiibo {:?} - converting it...\n", v2_amiibo);
                    Some(v2_amiibo.convert(key_set))
                }
                else if let Ok(v3_amiibo) = v3::VirtualAmiibo::try_load(entry_path.clone()) {
                    log!("Loaded v3 amiibo {:?} - converting it...\n", v3_amiibo);
                    Some(v3_amiibo.convert(key_set))
                }
                else {
                    log!("Not a deprecated amiibo format, skipping...\n");
                    None
                }
            };
            
            if let Some(new_amiibo_rc) = maybe_new_amiibo {
                match new_amiibo_rc {
                    Ok(new_amiibo) => log!("Converted new amiibo: {:?}\n", new_amiibo),
                    Err(rc) => log!("Conversion failed: {0} / {0:?}\n", rc)
                };
            }
        }
        else {
            break;
        }
    }

    Ok(())
}

pub fn convert_deprecated_virtual_amiibos() {
    log!("Analyzing deprecated dir...\n");
    let _ = convert_deprecated_virtual_amiibos_in_dir(super::DEPRECATED_VIRTUAL_AMIIBO_DIR);
    log!("Done analyzing deprecated dir...\n");

    log!("Analyzing regular dir...\n");
    let _ = convert_deprecated_virtual_amiibos_in_dir(super::VIRTUAL_AMIIBO_DIR);
    log!("Done analyzing regular dir...\n");
}
